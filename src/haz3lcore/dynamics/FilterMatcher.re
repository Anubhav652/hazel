let rec matches_exp =
        (
          ~denv: ClosureEnvironment.t,
          d: DHExp.t,
          ~fenv: ClosureEnvironment.t,
          f: DHExp.t,
        )
        : bool => {
  let matches_exp = (~denv=denv, ~fenv=fenv, d, f) =>
    matches_exp(~denv, d, ~fenv, f);
  switch (d, f) {
  | (Constructor("$e"), _) => failwith("$e in matched expression")
  | (Constructor("$v"), _) => failwith("$v in matched expression")

  // HACK[Matt]: ignore fixpoints in comparison, to allow pausing on fixpoint steps
  | (FixF(dp, _, dc), f) =>
    matches_exp(
      ~denv=
        Transition.evaluate_extend_env(
          Environment.singleton((dp, dc)),
          denv,
        ),
      dc,
      f,
    )
  | (dexp, FixF(fp, _, fc)) =>
    matches_exp(
      dexp,
      ~fenv=
        Transition.evaluate_extend_env(
          Environment.singleton((fp, fc)),
          fenv,
        ),
      f,
    )

  | (_, Constructor("$v")) =>
    switch (ValueChecker.check_value(denv, d)) {
    | Indet
    | Value => true
    | Expr => false
    }

  | (_, EmptyHole(_))
  | (_, Constructor("$e")) => true

  | (_, Closure(fenv, f)) => matches_exp(~fenv, d, f)
  | (_, Cast(f, _, _)) => matches_exp(d, f)
  | (_, FailedCast(f, _, _)) => matches_exp(d, f)

  | (Closure(denv, d), _) => matches_exp(~denv, d, f)
  | (Cast(d, _, _), _) => matches_exp(d, f)
  | (FailedCast(d, _, _), _) => matches_exp(d, f)
  | (Filter(Residue(_), d), _) => matches_exp(d, f)

  | (BoundVar(dx), BoundVar(fx)) =>
    switch (
      ClosureEnvironment.lookup(denv, dx),
      ClosureEnvironment.lookup(fenv, fx),
    ) {
    | (Some(d), Some(f)) => matches_exp(d, f)
    | (Some(_), None) => false
    | (None, Some(_)) => false
    | (None, None) => true
    }
  | (BoundVar(dx), _) =>
    switch (ClosureEnvironment.lookup(denv, dx)) {
    | Some(d) => matches_exp(d, f)
    | None => false
    }
  | (_, BoundVar(fx)) =>
    switch (ClosureEnvironment.lookup(fenv, fx)) {
    | Some(f) => matches_exp(d, f)
    | None => false
    }

  | (EmptyHole(_), _) => false

  | (Filter(df, dd), Filter(ff, fd)) =>
    DH.DHFilter.fast_equal(df, ff) && matches_exp(dd, fd)
  | (Filter(_), _) => false

  | (BoolLit(dv), BoolLit(fv)) => dv == fv
  | (BoolLit(_), _) => false

  | (IntLit(dv), IntLit(fv)) => dv == fv
  | (IntLit(_), _) => false

  | (FloatLit(dv), FloatLit(fv)) => dv == fv
  | (FloatLit(_), _) => false

  | (StringLit(dv), StringLit(fv)) => dv == fv
  | (StringLit(_), _) => false

  | (Constructor(_), Ap(Constructor("~MVal"), Tuple([]))) => true
  | (Constructor(dt), Constructor(ft)) => dt == ft
  | (Constructor(_), _) => false

  | (BuiltinFun(dn), BuiltinFun(fn)) => dn == fn
  | (BuiltinFun(_), _) => false

  | (Fun(dp1, dty1, d1, _), Fun(fp1, fty1, f1, _)) =>
    matches_pat(dp1, fp1) && dty1 == fty1 && matches_exp(d1, f1)
  | (Fun(_), _) => false

  | (FreeVar(du, di, dx), FreeVar(fu, fi, fx)) =>
    du == fu && di == fi && dx == fx
  | (FreeVar(_), _) => false

  | (Let(dp, d1, d2), Let(fp, f1, f2)) =>
    matches_pat(dp, fp) && matches_exp(d1, f1) && matches_exp(d2, f2)
  | (Let(_), _) => false

  | (Ap(d1, d2), Ap(f1, f2)) => matches_exp(d1, f1) && matches_exp(d2, f2)
  | (Ap(_), _) => false

  | (IfThenElse(dc, d1, d2, d3), IfThenElse(fc, f1, f2, f3)) =>
    dc == fc
    && matches_exp(d1, f1)
    && matches_exp(d2, f2)
    && matches_exp(d3, f3)
  | (IfThenElse(_), _) => false

  | (Sequence(d1, d2), Sequence(f1, f2)) =>
    matches_exp(d1, f1) && matches_exp(d2, f2)
  | (Sequence(_), _) => false

  | (Test(id1, d2), Test(id2, f2)) => id1 == id2 && matches_exp(d2, f2)
  | (Test(_), _) => false

  | (Cons(d1, d2), Cons(f1, f2)) =>
    matches_exp(d1, f1) && matches_exp(d2, f2)
  | (Cons(_), _) => false

  | (ListLit(_, _, dt, dv), ListLit(_, _, ft, fv)) =>
    dt == ft
    && List.fold_left2(
         (acc, d, f) => acc && matches_exp(d, f),
         true,
         dv,
         fv,
       )
  | (ListLit(_), _) => false

  | (Tuple(dv), Tuple(fv)) =>
    List.fold_left2((acc, d, f) => acc && matches_exp(d, f), true, dv, fv)
  | (Tuple(_), _) => false

  | (BinBoolOp(d_op_bin, d1, d2), BinBoolOp(f_op_bin, f1, f2)) =>
    d_op_bin == f_op_bin && matches_exp(d1, f1) && matches_exp(d2, f2)

  | (BinBoolOp(_), _) => false

  | (BinIntOp(d_op_bin, d1, d2), BinIntOp(f_op_bin, f1, f2)) =>
    d_op_bin == f_op_bin && matches_exp(d1, f1) && matches_exp(d2, f2)
  | (BinIntOp(_), _) => false

  | (BinFloatOp(d_op_bin, d1, d2), BinFloatOp(f_op_bin, f1, f2)) =>
    d_op_bin == f_op_bin && matches_exp(d1, f1) && matches_exp(d2, f2)
  | (BinFloatOp(_), _) => false

  | (BinStringOp(d_op_bin, d1, d2), BinStringOp(f_op_bin, f1, f2)) =>
    d_op_bin == f_op_bin && matches_exp(d1, f1) && matches_exp(d2, f2)
  | (BinStringOp(_), _) => false

  | (ListConcat(_), _) => false

  | (
      ConsistentCase(Case(dscrut, drule, _)),
      ConsistentCase(Case(fscrut, frule, _)),
    )
  | (
      InconsistentBranches(_, _, Case(dscrut, drule, _)),
      InconsistentBranches(_, _, Case(fscrut, frule, _)),
    ) =>
    matches_exp(dscrut, fscrut)
    && (
      switch (
        List.fold_left2(
          (res, drule, frule) =>
            res && matches_rul(~denv, drule, ~fenv, frule),
          true,
          drule,
          frule,
        )
      ) {
      | exception (Invalid_argument(_)) => false
      | res => res
      }
    )
  | (ConsistentCase(_), _)
  | (InconsistentBranches(_), _) => false

  | (NonEmptyHole(_), _) => false
  | (ExpandingKeyword(_), _) => false
  | (InvalidText(_), _) => false
  | (InvalidOperation(_), _) => false

  | (ApBuiltin(dname, darg), ApBuiltin(fname, farg)) =>
    dname == fname && matches_exp(darg, farg)
  | (ApBuiltin(_), _) => false

  | (Prj(dv, di), Prj(fv, fi)) => matches_exp(dv, fv) && di == fi
  | (Prj(_), _) => false
  };
}
and matches_pat = (d: DHPat.t, f: DHPat.t): bool => {
  switch (d, f) {
  | (_, EmptyHole(_)) => true
  | (Wild, Wild) => true
  | (Wild, _) => false
  | (IntLit(dv), IntLit(fv)) => dv == fv
  | (IntLit(_), _) => false
  | (FloatLit(dv), FloatLit(fv)) => dv == fv
  | (FloatLit(_), _) => false
  | (BoolLit(dv), BoolLit(fv)) => dv == fv
  | (BoolLit(_), _) => false
  | (StringLit(dv), StringLit(fv)) => dv == fv
  | (StringLit(_), _) => false
  | (ListLit(dty1, dl), ListLit(fty1, fl)) =>
    switch (
      List.fold_left2((res, d, f) => res && matches_pat(d, f), true, dl, fl)
    ) {
    | exception (Invalid_argument(_)) => false
    | res => matches_typ(dty1, fty1) && res
    }
  | (ListLit(_), _) => false
  | (Constructor(dt), Constructor(ft)) => dt == ft
  | (Constructor(_), _) => false
  | (Var(dx), Var(fx)) => dx == fx
  | (Var(_), _) => false
  | (Tuple(dl), Tuple(fl)) =>
    switch (
      List.fold_left2((res, d, f) => res && matches_pat(d, f), true, dl, fl)
    ) {
    | exception (Invalid_argument(_)) => false
    | res => res
    }
  | (Tuple(_), _) => false
  | (Ap(d1, d2), Ap(f1, f2)) => matches_pat(d1, f1) && matches_pat(d2, f2)
  | (Ap(_), _) => false
  | (BadConstructor(_, _, dt), BadConstructor(_, _, ft)) => dt == ft
  | (BadConstructor(_), _) => false
  | (Cons(d1, d2), Cons(f1, f2)) =>
    matches_pat(d1, f1) && matches_pat(d2, f2)
  | (Cons(_), _) => false
  | (EmptyHole(_), _) => false
  | (NonEmptyHole(_), _) => false
  | (ExpandingKeyword(_), _) => false
  | (InvalidText(_), _) => false
  };
}
and matches_typ = (d: Typ.t, f: Typ.t) => {
  Typ.eq(d, f);
}
and matches_rul = (~denv, d: DHExp.rule, ~fenv, f: DHExp.rule) => {
  switch (d, f) {
  | (Rule(dp, d), Rule(fp, f)) =>
    matches_pat(dp, fp) && matches_exp(~denv, d, ~fenv, f)
  };
};

let matches =
    (~env: ClosureEnvironment.t, ~exp: DHExp.t, ~flt: Filter.t)
    : option(FilterAction.t) =>
  if (matches_exp(~denv=env, exp, ~fenv=env, flt.pat)) {
    Some(flt.act);
  } else {
    None;
  };

let matches =
    (~env: ClosureEnvironment.t, ~exp: DHExp.t, ~act: FilterAction.t, flt_env)
    : (FilterAction.t, int) => {
  let len = List.length(flt_env);
  let rec matches' = (~env, ~exp, ~act, flt_env, idx) => {
    switch (flt_env) {
    | [] => (act, idx)
    | [hd, ...tl] =>
      switch (matches(~env, ~exp, ~flt=hd)) {
      | Some(act) => (act, idx)
      | None => matches'(~env, ~exp, ~act, tl, idx + 1)
      }
    };
  };
  let (act, idx) = matches'(~env, ~exp, ~act, flt_env, 0);
  (act, len - idx);
};
