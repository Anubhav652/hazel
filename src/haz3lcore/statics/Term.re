/* TERM

   These data structures define the term structures on which
   the static and dynamic semantics of the language are based.
   Each sort has a corresponding U<Sort> module.

   The contained cls type lists the terms of that sort, and
   should be in 1-1 correspondence with the term type which
   is used to build composite terms.

   This is wrapped in a record type to associate a unique id
   with each term. These unique ids are the same as from the
   tile structure from the syntax module, as there is a 1-1
   correspondence between terms and tiles.

   TODO: add tests to check if there are forms and/or terms
   without correponding syntax classes */

open Util;
include TermBase.Any;

type any = t;
module UTyp = {
  [@deriving (show({with_path: false}), sexp, yojson)]
  type cls =
    | Invalid
    | EmptyHole
    | MultiHole
    | Int
    | Float
    | Bool
    | String
    | Arrow
    | Tuple
    | Sum
    | List
    | Var
    | Constructor
    | Parens
    | Ap;

  include TermBase.UTyp;

  let rep_id = ({ids, _}: t) => {
    assert(ids != []);
    List.hd(ids);
  };

  let hole = (tms: list(any)) =>
    switch (tms) {
    | [] => EmptyHole
    | [_, ..._] => MultiHole(tms)
    };

  let cls_of_term: term => cls =
    fun
    | Invalid(_) => Invalid
    | EmptyHole => EmptyHole
    | MultiHole(_) => MultiHole
    | Int => Int
    | Float => Float
    | Bool => Bool
    | String => String
    | List(_) => List
    | Arrow(_) => Arrow
    | Var(_) => Var
    | Constructor(_) => Constructor
    | Tuple(_) => Tuple
    | Parens(_) => Parens
    | Ap(_) => Ap
    | Sum(_) => Sum;

  let show_cls: cls => string =
    fun
    | Invalid => "Invalid Type"
    | EmptyHole => "Empty Type Hole"
    | MultiHole => "Multi Type Hole"
    | Int
    | Float
    | String
    | Bool => "Base Type"
    | Var => "Type Variable"
    | Constructor => "Sum Constructor"
    | List => "List Type"
    | Arrow => "Function Type"
    | Tuple => "Product Type"
    | Sum => "Sum Type"
    | Parens => "Parenthesized Type Term"
    | Ap => "Sum Constructor Application";

  let rec is_arrow = (typ: t) => {
    switch (typ.term) {
    | Parens(typ) => is_arrow(typ)
    | Arrow(_) => true
    | Invalid(_)
    | EmptyHole
    | MultiHole(_)
    | Int
    | Float
    | Bool
    | String
    | List(_)
    | Tuple(_)
    | Var(_)
    | Constructor(_)
    | Ap(_)
    | Sum(_) => false
    };
  };

  /* Converts a syntactic type into a semantic type */
  let rec to_typ: (Ctx.t, t) => Typ.t =
    (ctx, utyp) =>
      switch (utyp.term) {
      | Invalid(_)
      | MultiHole(_) => Unknown(Internal)
      | EmptyHole => Unknown(TypeHole)
      | Bool => Bool
      | Int => Int
      | Float => Float
      | String => String
      | Var(name) =>
        switch (Ctx.lookup_tvar(ctx, name)) {
        | Some(_) => Var(name)
        | None => Unknown(Free(name))
        }
      | Arrow(u1, u2) => Arrow(to_typ(ctx, u1), to_typ(ctx, u2))
      | Tuple(us) => Prod(List.map(to_typ(ctx), us))
      | Sum(uts) => Sum(to_ctr_map(ctx, uts))
      | List(u) => List(to_typ(ctx, u))
      | Parens(u) => to_typ(ctx, u)
      /* The below cases should occur only inside sums */
      | Constructor(_)
      | Ap(_) => Unknown(Internal)
      }
  and to_variant:
    (Ctx.t, variant) => option(ConstructorMap.binding(option(Typ.t))) =
    ctx =>
      fun
      | Variant(ctr, _, u) => Some((ctr, Option.map(to_typ(ctx), u)))
      | BadEntry(_) => None
  and to_ctr_map = (ctx: Ctx.t, uts: list(variant)): Typ.sum_map => {
    List.fold_left(
      (acc, ut) =>
        List.find_opt(((ctr, _)) => ctr == fst(ut), acc) == None
          ? acc @ [ut] : acc,
      [],
      List.filter_map(to_variant(ctx), uts),
    );
  };
};

module UTPat = {
  [@deriving (show({with_path: false}), sexp, yojson)]
  type cls =
    | Invalid
    | EmptyHole
    | MultiHole
    | Var;

  include TermBase.UTPat;

  let rep_id = ({ids, _}) => {
    assert(ids != []);
    List.hd(ids);
  };

  let hole = (tms: list(any)) =>
    switch (tms) {
    | [] => EmptyHole
    | [_, ..._] => MultiHole(tms)
    };

  let cls_of_term: term => cls =
    fun
    | Invalid(_) => Invalid
    | EmptyHole => EmptyHole
    | MultiHole(_) => MultiHole
    | Var(_) => Var;

  let show_cls: cls => string =
    fun
    | Invalid => "Invalid Type Variable"
    | EmptyHole => "Empty Type Variable Hole"
    | MultiHole => "Multi Type Variable Hole"
    | Var => "Type Variable";
};

module UPat = {
  [@deriving (show({with_path: false}), sexp, yojson)]
  type cls =
    | Invalid
    | EmptyHole
    | MultiHole
    | Wild
    | Int
    | Float
    | Bool
    | String
    | Triv
    | ListLit
    | Constructor
    | Cons
    | Var
    | Tuple
    | Parens
    | Ap
    | TypeAnn;

  include TermBase.UPat;

  let rep_id = ({ids, _}: t) => {
    assert(ids != []);
    List.hd(ids);
  };

  let hole = (tms: list(any)) =>
    switch (tms) {
    | [] => EmptyHole
    | [_, ..._] => MultiHole(tms)
    };

  let cls_of_term: term => cls =
    fun
    | Invalid(_) => Invalid
    | EmptyHole => EmptyHole
    | MultiHole(_) => MultiHole
    | Wild => Wild
    | Int(_) => Int
    | Float(_) => Float
    | Bool(_) => Bool
    | String(_) => String
    | Triv => Triv
    | ListLit(_) => ListLit
    | Constructor(_) => Constructor
    | Cons(_) => Cons
    | Var(_) => Var
    | Tuple(_) => Tuple
    | Parens(_) => Parens
    | Ap(_) => Ap
    | TypeAnn(_) => TypeAnn;

  let show_cls: cls => string =
    fun
    | Invalid => "Invalid Pattern"
    | EmptyHole => "Empty Pattern Hole"
    | MultiHole => "Multi Pattern Hole"
    | Wild => "Wildcard Pattern"
    | Int => "Integer Pattern Literal"
    | Float => "Float Pattern Literal"
    | Bool => "Boolean Pattern Literal"
    | String => "String Pattern Literal"
    | Triv => "Trivial Pattern Literal"
    | ListLit => "List Literal Pattern"
    | Constructor => "Constructor Pattern"
    | Cons => "Cons Pattern"
    | Var => "Pattern Variable"
    | Tuple => "Tuple Pattern"
    | Parens => "Parenthesized Pattern"
    | Ap => "Constructor Application Pattern"
    | TypeAnn => "Type Annotation";

  let rec is_var = (pat: t) => {
    switch (pat.term) {
    | Parens(pat) => is_var(pat)
    | Var(_) => true
    | TypeAnn(_)
    | Invalid(_)
    | EmptyHole
    | MultiHole(_)
    | Wild
    | Int(_)
    | Float(_)
    | Bool(_)
    | String(_)
    | Triv
    | ListLit(_)
    | Cons(_, _)
    | Tuple(_)
    | Constructor(_)
    | Ap(_) => false
    };
  };

  let rec is_fun_var = (pat: t) => {
    switch (pat.term) {
    | Parens(pat) => is_fun_var(pat)
    | TypeAnn(pat, typ) => is_var(pat) && UTyp.is_arrow(typ)
    | Invalid(_)
    | EmptyHole
    | MultiHole(_)
    | Wild
    | Int(_)
    | Float(_)
    | Bool(_)
    | String(_)
    | Triv
    | ListLit(_)
    | Cons(_, _)
    | Var(_)
    | Tuple(_)
    | Constructor(_)
    | Ap(_) => false
    };
  };

  let rec is_tuple_of_arrows = (pat: t) =>
    is_fun_var(pat)
    || (
      switch (pat.term) {
      | Parens(pat) => is_tuple_of_arrows(pat)
      | Tuple(pats) => pats |> List.for_all(is_fun_var)
      | Invalid(_)
      | EmptyHole
      | MultiHole(_)
      | Wild
      | Int(_)
      | Float(_)
      | Bool(_)
      | String(_)
      | Triv
      | ListLit(_)
      | Cons(_, _)
      | Var(_)
      | TypeAnn(_)
      | Constructor(_)
      | Ap(_) => false
      }
    );

  let rec has_var_def = (pat: t, var: Var.t) => {
    switch (pat.term) {
    | Var(var') => var == var'
    | Parens(pat)
    | TypeAnn(pat, _)
    | Ap(_, pat) => has_var_def(pat, var)
    | ListLit(pats)
    | Tuple(pats) => List.exists(pat => has_var_def(pat, var), pats)
    | Cons(pat1, pat2) => has_var_def(pat1, var) || has_var_def(pat2, var)
    | Invalid(_)
    | EmptyHole
    | MultiHole(_)
    | Wild
    | Int(_)
    | Float(_)
    | Bool(_)
    | String(_)
    | Triv
    | Constructor(_) => false
    };
  };

  let rec get_var = (pat: t) => {
    switch (pat.term) {
    | Parens(pat) => get_var(pat)
    | Var(x) => Some(x)
    | TypeAnn(_)
    | Invalid(_)
    | EmptyHole
    | MultiHole(_)
    | Wild
    | Int(_)
    | Float(_)
    | Bool(_)
    | String(_)
    | Triv
    | ListLit(_)
    | Cons(_, _)
    | Tuple(_)
    | Constructor(_)
    | Ap(_) => None
    };
  };

  let rec get_fun_var = (pat: t) => {
    switch (pat.term) {
    | Parens(pat) => get_fun_var(pat)
    | TypeAnn(pat, typ) =>
      if (UTyp.is_arrow(typ)) {
        get_var(pat) |> Option.map(var => var);
      } else {
        None;
      }
    | Invalid(_)
    | EmptyHole
    | MultiHole(_)
    | Wild
    | Int(_)
    | Float(_)
    | Bool(_)
    | String(_)
    | Triv
    | ListLit(_)
    | Cons(_, _)
    | Var(_)
    | Tuple(_)
    | Constructor(_)
    | Ap(_) => None
    };
  };

  let rec get_recursive_bindings = (pat: t) => {
    switch (get_fun_var(pat)) {
    | Some(x) => Some([x])
    | None =>
      switch (pat.term) {
      | Parens(pat) => get_recursive_bindings(pat)
      | Tuple(pats) =>
        let fun_vars = pats |> List.map(get_fun_var);
        if (List.exists(Option.is_none, fun_vars)) {
          None;
        } else {
          Some(List.map(Option.get, fun_vars));
        };
      | Invalid(_)
      | EmptyHole
      | MultiHole(_)
      | Wild
      | Int(_)
      | Float(_)
      | Bool(_)
      | String(_)
      | Triv
      | ListLit(_)
      | Cons(_, _)
      | Var(_)
      | TypeAnn(_)
      | Constructor(_)
      | Ap(_) => None
      }
    };
  };

  let ctr_name = (p: t): option(Constructor.t) =>
    switch (p.term) {
    | Constructor(name) => Some(name)
    | _ => None
    };

  let rec get_pats = (pat: t) => {
    switch (pat.term) {
    | Parens(pat) => get_pats(pat)
    | Tuple(pats) => Some(pats)
    // TODO
    | Var(_)
    | TypeAnn(_)
    | Invalid(_)
    | EmptyHole
    | MultiHole(_)
    | Wild
    | Int(_)
    | Float(_)
    | Bool(_)
    | String(_)
    | Triv
    | ListLit(_)
    | Cons(_, _)
    | Constructor(_)
    | Ap(_) => None
    };
  };
};

module UExp = {
  include TermBase.UExp;

  let hole = (tms: list(any)) =>
    switch (tms) {
    | [] => EmptyHole
    | [_, ..._] => MultiHole(tms)
    };

  let rep_id = ({ids, _}) => {
    assert(ids != []);
    List.hd(ids);
  };

  let cls_of_term: term => cls =
    fun
    | Invalid(_) => Invalid
    | EmptyHole => EmptyHole
    | MultiHole(_) => MultiHole
    | Triv => Triv
    | Deferral(_) => Deferral
    | Bool(_) => Bool
    | Int(_) => Int
    | Float(_) => Float
    | String(_) => String
    | ListLit(_) => ListLit
    | Constructor(_) => Constructor
    | Fun(_) => Fun
    | Tuple(_) => Tuple
    | Var(_) => Var
    | Let(_) => Let
    | TyAlias(_) => TyAlias
    | Ap(_) => Ap
    | DeferredAp(_) => DeferredAp
    | If(_) => If
    | Seq(_) => Seq
    | Test(_) => Test
    | Parens(_) => Parens
    | Cons(_) => Cons
    | UnOp(op, _) => UnOp(op)
    | BinOp(op, _, _) => BinOp(op)
    | Match(_) => Match;

  let show_op_un_int: op_un_int => string =
    fun
    | Minus => "Integer Negation";

  let show_unop: op_un => string =
    fun
    | Int(op) => show_op_un_int(op);

  let show_op_bin_bool: op_bin_bool => string =
    fun
    | And => "Boolean Conjunction"
    | Or => "Boolean Disjunction";

  let show_op_bin_int: op_bin_int => string =
    fun
    | Plus => "Integer Addition"
    | Minus => "Integer Subtraction"
    | Times => "Integer Multiplication"
    | Power => "Integer Exponentiation"
    | Divide => "Integer Division"
    | LessThan => "Integer Less Than"
    | LessThanOrEqual => "Integer Less Than or Equal"
    | GreaterThan => "Integer Greater Than"
    | GreaterThanOrEqual => "Integer Greater Than or Equal"
    | Equals => "Integer Equality";

  let show_op_bin_float: op_bin_float => string =
    fun
    | Plus => "Float Addition"
    | Minus => "Float Subtraction"
    | Times => "Float Multiplication"
    | Power => "Float Exponentiation"
    | Divide => "Float Division"
    | LessThan => "Float Less Than"
    | LessThanOrEqual => "Float Less Than or Equal"
    | GreaterThan => "Float Greater Than"
    | GreaterThanOrEqual => "Float Greater Than or Equal"
    | Equals => "Float Equality";

  let show_op_bin_string: op_bin_string => string =
    fun
    | Equals => "String Equality";

  let show_binop: op_bin => string =
    fun
    | Int(op) => show_op_bin_int(op)
    | Float(op) => show_op_bin_float(op)
    | Bool(op) => show_op_bin_bool(op)
    | String(op) => show_op_bin_string(op);

  let show_cls: cls => string =
    fun
    | Invalid => "Invalid Expression"
    | EmptyHole => "Empty Expression Hole"
    | MultiHole => "Multi Expression Hole"
    | Triv => "Trivial Literal. Pathetic, really."
    | Deferral => "Deferral"
    | Bool => "Boolean Literal"
    | Int => "Integer Literal"
    | Float => "Float Literal"
    | String => "String Literal"
    | ListLit => "List Literal"
    | Constructor => "Constructor"
    | Fun => "Function Literal"
    | Tuple => "Tuple Literal"
    | Var => "Variable Reference"
    | Let => "Let Expression"
    | TyAlias => "Type Alias Definition"
    | Ap => "Function/Constructor Application"
    | DeferredAp => "Partial Function Application"
    | If => "If Expression"
    | Seq => "Sequence Expression"
    | Test => "Test (Effectful)"
    | Parens => "Parenthesized Expression"
    | Cons => "Cons"
    | BinOp(op) => show_binop(op)
    | UnOp(op) => show_unop(op)
    | Match => "Match Expression";

  let rec is_fun = (e: t) => {
    switch (e.term) {
    | Parens(e) => is_fun(e)
    | Fun(_) => true
    | Invalid(_)
    | EmptyHole
    | MultiHole(_)
    | Triv
    | Deferral(_)
    | Bool(_)
    | Int(_)
    | Float(_)
    | String(_)
    | ListLit(_)
    | Tuple(_)
    | Var(_)
    | Let(_)
    | TyAlias(_)
    | Ap(_)
    | DeferredAp(_)
    | If(_)
    | Seq(_)
    | Test(_)
    | Cons(_)
    | UnOp(_)
    | BinOp(_)
    | Match(_)
    | Constructor(_) => false
    };
  };

  let rec is_tuple_of_functions = (e: t) =>
    is_fun(e)
    || (
      switch (e.term) {
      | Parens(e) => is_tuple_of_functions(e)
      | Tuple(es) => es |> List.for_all(is_fun)
      | Invalid(_)
      | EmptyHole
      | MultiHole(_)
      | Triv
      | Deferral(_)
      | Bool(_)
      | Int(_)
      | Float(_)
      | String(_)
      | ListLit(_)
      | Fun(_)
      | Var(_)
      | Let(_)
      | TyAlias(_)
      | Ap(_)
      | DeferredAp(_)
      | If(_)
      | Seq(_)
      | Test(_)
      | Cons(_)
      | UnOp(_)
      | BinOp(_)
      | Match(_)
      | Constructor(_) => false
      }
    );

  let ctr_name = (e: t): option(Constructor.t) =>
    switch (e.term) {
    | Constructor(name) => Some(name)
    | _ => None
    };

  let is_deferral = (e: t) => {
    switch (e.term) {
    | Deferral(_) => true
    | _ => false
    };
  };

  let matched_args = (e: t) => {
    switch (e.term) {
    | Tuple([_, ..._] as es) => es
    | _ => [e]
    };
  };

  let rec is_rec_fun = (pat: UPat.t, e: t) => {
    switch (e.term) {
    | Parens(e) => is_rec_fun(pat, e)
    | Fun(_, e) =>
      switch (UPat.get_var(pat)) {
      | Some(var) => has_fun_var(e, var)
      | None => false
      }
    | _ => false
    };
  }
  and has_fun_var = (e: t, var: Var.t) =>
    switch (e.term) {
    | Var(var') => var == var'
    | Parens(e)
    | TyAlias(_, _, e)
    | Test(e)
    | UnOp(_, e) => has_fun_var(e, var)
    | Cons(e1, e2)
    | Seq(e1, e2)
    | BinOp(_, e1, e2) => has_fun_var(e1, var) || has_fun_var(e2, var)
    | If(e1, e2, e3) =>
      has_fun_var(e1, var) || has_fun_var(e2, var) || has_fun_var(e3, var)
    | ListLit(es)
    | Tuple(es) => List.exists(has_fun_var(_, var), es)
    | Fun(pat, e)
    | Let(pat, _, e) => !UPat.has_var_def(pat, var) && has_fun_var(e, var)
    | Match(e, ruls) =>
      has_fun_var(e, var)
      || List.exists(
           ((pat, e)) =>
             !UPat.has_var_def(pat, var) && has_fun_var(e, var),
           ruls,
         )
    | Ap(fn, arg)
    | DeferredAp(fn, arg) =>
      //has_branch_var(fn, var) ||
      has_fun_var(fn, var) || has_fun_var(arg, var)
    | Invalid(_)
    | EmptyHole
    | MultiHole(_)
    | Triv
    | Deferral(_)
    | Bool(_)
    | Int(_)
    | Float(_)
    | String(_)
    // | Var(_)
    | Constructor(_) => false
    };
  // and has_branch_var = (e: t, var: Var.t) => {
  //   switch (e.term) {
  //   | Var(var') => var == var'
  //   | Parens(e)
  //   | TyAlias(_, _, e)
  //   | Seq(_, e) => has_branch_var(e, var)
  //   | Let(pat, _, e) =>
  //     !UPat.has_var_def(pat, var) && has_branch_var(e, var)
  //   | If(_, e1, e2) => has_branch_var(e1, var) || has_branch_var(e2, var)
  //   | Match(_, ruls) =>
  //     List.exists(
  //       ((pat, e)) =>
  //         !UPat.has_var_def(pat, var) && has_branch_var(e, var),
  //       ruls,
  //     )
  //   | Ap(_)
  //   | DeferredAp(_)
  //   | Fun(_)
  //   | Invalid(_)
  //   | EmptyHole
  //   | MultiHole(_)
  //   | Triv
  //   | Deferral(_)
  //   | Bool(_)
  //   | Int(_)
  //   | Float(_)
  //   | String(_)
  //   | ListLit(_)
  //   | Tuple(_)
  //   | Test(_)
  //   | Cons(_)
  //   | UnOp(_)
  //   | BinOp(_)
  //   | Constructor(_) => false
  //   };
  // };

  let rec is_tuple_of_rec_functions = (pat: UPat.t, e: t) => {
    is_rec_fun(pat, e)
    || (
      switch (e.term) {
      | Parens(e) => is_tuple_of_rec_functions(pat, e)
      | Tuple(es) =>
        switch (UPat.get_pats(pat)) {
        | Some(pats) =>
          List.for_all(pat => List.exists(is_rec_fun(pat), es), pats)
        | None => false
        }
      | Invalid(_)
      | EmptyHole
      | MultiHole(_)
      | Triv
      | Deferral(_)
      | Bool(_)
      | Int(_)
      | Float(_)
      | String(_)
      | ListLit(_)
      | Fun(_)
      | Var(_)
      | Let(_)
      | TyAlias(_)
      | Ap(_)
      | DeferredAp(_)
      | If(_)
      | Seq(_)
      | Test(_)
      | Cons(_)
      | UnOp(_)
      | BinOp(_)
      | Match(_)
      | Constructor(_) => false
      }
    );
  };
};

// TODO(d): consider just folding this into UExp
module URul = {
  include TermBase.URul;

  [@deriving (show({with_path: false}), sexp, yojson)]
  type cls =
    | Rule;

  // example of awkwardness induced by having forms like rules
  // that may have a different-sorted child with no delimiters
  // (eg scrut with no rules)
  let ids = (~any_ids, {ids, term}: t) =>
    switch (ids) {
    | [_, ..._] => ids
    | [] =>
      switch (term) {
      | Hole([tm, ..._]) => any_ids(tm)
      | Rules(scrut, []) => scrut.ids
      | _ => []
      }
    };

  let rep_id = (~any_ids, tm) =>
    switch (ids(~any_ids, tm)) {
    | [] => raise(Invalid_argument("Term.UExp.rep_id"))
    | [id, ..._] => id
    };
};

let rec ids =
  fun
  | Exp(tm) => tm.ids
  | Pat(tm) => tm.ids
  | Typ(tm) => tm.ids
  | TPat(tm) => tm.ids
  | Rul(tm) => URul.ids(~any_ids=ids, tm)
  | Nul ()
  | Any () => [];

// Terms may consist of multiple tiles, eg the commas in an n-tuple,
// the rules of a case expression + the surrounding case-end tile,
// the list brackets tile coupled with the elem-separating commas.
// The _representative id_ is the canonical tile id used to identify
// and look up info about a term.
//
// In instances like case expressions and list literals, where a parent
// tile surrounds the other tiles, the representative id is the parent tile's.
// In other instances like n-tuples, where the commas are all siblings,
// the representative id is one of the comma ids, unspecified which one.
// (This would change for n-tuples if we decided parentheses are necessary.)
let rep_id =
  fun
  | Exp(tm) => UExp.rep_id(tm)
  | Pat(tm) => UPat.rep_id(tm)
  | Typ(tm) => UTyp.rep_id(tm)
  | TPat(tm) => UTPat.rep_id(tm)
  | Rul(tm) => URul.rep_id(~any_ids=ids, tm)
  | Nul ()
  | Any () => raise(Invalid_argument("Term.rep_id"));
