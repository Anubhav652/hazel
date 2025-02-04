open Virtual_dom.Vdom;
open Node;
open Util.Web;
open Util;
open Haz3lcore;

let errc = "error";
let okc = "ok";
let div_err = div(~attr=clss([errc]));
let div_ok = div(~attr=clss([okc]));

let code_err = (code: string): Node.t =>
  div(~attr=clss(["code"]), [text(code)]);

let explain_this_toggle = (~inject, ~show_explain_this: bool): Node.t => {
  let tooltip = "Toggle language documentation";
  let toggle_explain_this = _ =>
    Virtual_dom.Vdom.Effect.Many([
      inject(Update.Set(ExplainThis(ToggleShow))),
      Virtual_dom.Vdom.Effect.Stop_propagation,
    ]);
  div(
    ~attr=clss(["explain-this-button"]),
    [Widgets.toggle(~tooltip, "?", show_explain_this, toggle_explain_this)],
  );
};

let cls_view = (ci: Info.t): Node.t =>
  div(
    ~attr=clss(["syntax-class"]),
    [text(ci |> Info.cls_of |> Term.Cls.show)],
  );

let ctx_toggle = (~inject, context_inspector: bool): Node.t =>
  div(
    ~attr=
      Attr.many([
        Attr.on_click(_ => inject(Update.Set(ContextInspector))),
        clss(["gamma"] @ (context_inspector ? ["visible"] : [])),
      ]),
    [text("Γ")],
  );

let term_view = (~inject, ~settings: Settings.t, ci) => {
  let sort = ci |> Info.sort_of |> Sort.show;
  div(
    ~attr=clss(["ci-header", sort] @ (Info.is_error(ci) ? [errc] : [])),
    [
      ctx_toggle(~inject, settings.context_inspector),
      CtxInspector.view(~inject, ~settings, ci),
      div(~attr=clss(["term-tag"]), [text(sort)]),
      explain_this_toggle(
        ~inject,
        ~show_explain_this=settings.explainThis.show,
      ),
      cls_view(ci),
    ],
  );
};

let elements_noun: Term.Cls.t => string =
  fun
  | Exp(Match | If) => "Branches"
  | Exp(ListLit)
  | Pat(ListLit) => "Elements"
  | Exp(ListConcat) => "Operands"
  | _ => failwith("elements_noun: Cls doesn't have elements");

let common_err_view = (cls: Term.Cls.t, err: Info.error_common) =>
  switch (err) {
  | NoType(BadToken(token)) =>
    switch (Form.bad_token_cls(token)) {
    | BadInt => [text("Integer is too large or too small")]
    | Other => [text(Printf.sprintf("\"%s\" isn't a valid token", token))]
    }
  | NoType(BadTrivAp(ty)) => [
      text("Function argument type"),
      Type.view(ty),
      text("inconsistent with"),
      Type.view(Prod([])),
    ]
  | NoType(FreeConstructor(name)) => [code_err(name), text("not found")]
  | Inconsistent(WithArrow(typ)) => [
      text(":"),
      Type.view(typ),
      text("inconsistent with arrow type"),
    ]
  | Inconsistent(Expectation({ana, syn})) => [
      text(":"),
      Type.view(syn),
      text("inconsistent with expected type"),
      Type.view(ana),
    ]
  | Inconsistent(Internal(tys)) => [
      text(elements_noun(cls) ++ " have inconsistent types:"),
      ...ListUtil.join(text(","), List.map(Type.view, tys)),
    ]
  };

let common_ok_view = (cls: Term.Cls.t, ok: Info.ok_pat) => {
  switch (cls, ok) {
  | (Exp(MultiHole) | Pat(MultiHole), _) => [
      text("Expecting operator or delimiter"),
    ]
  | (Exp(EmptyHole), Syn(_)) => [text("Fillable by any expression")]
  | (Pat(EmptyHole), Syn(_)) => [text("Fillable by any pattern")]
  | (Exp(EmptyHole), Ana(Consistent({ana, _}))) => [
      text("Fillable by any expression of type"),
      Type.view(ana),
    ]
  | (Pat(EmptyHole), Ana(Consistent({ana, _}))) => [
      text("Fillable by any pattern of type"),
      Type.view(ana),
    ]
  | (_, Syn(syn)) => [text(":"), Type.view(syn)]
  | (Pat(Var) | Pat(Wild), Ana(Consistent({ana, _}))) => [
      text(":"),
      Type.view(ana),
    ]
  | (_, Ana(Consistent({ana, syn, _}))) when ana == syn => [
      text(":"),
      Type.view(syn),
      text("equals expected type"),
    ]
  | (_, Ana(Consistent({ana, syn, _}))) => [
      text(":"),
      Type.view(syn),
      text("consistent with expected type"),
      Type.view(ana),
    ]
  | (_, Ana(InternallyInconsistent({ana, nojoin: tys}))) =>
    [
      text(elements_noun(cls) ++ " have inconsistent types:"),
      ...ListUtil.join(text(","), List.map(Type.view, tys)),
    ]
    @ [text("but consistent with expected"), Type.view(ana)]
  };
};

let typ_ok_view = (cls: Term.Cls.t, ok: Info.ok_typ) =>
  switch (ok) {
  | Type(_) when cls == Typ(EmptyHole) => [text("Fillable by any type")]
  | Type(ty) => [Type.view(ty), text("is a type")]
  | TypeAlias(name, ty_lookup) => [
      Type.view(Var(name)),
      text("is an alias for"),
      Type.view(ty_lookup),
    ]
  | Variant(name, sum_ty) => [
      Type.view(Var(name)),
      text("is a sum type constuctor of type"),
      Type.view(sum_ty),
    ]
  | VariantIncomplete(sum_ty) => [
      text("An incomplete sum type constuctor of type"),
      Type.view(sum_ty),
    ]
  };

let typ_err_view = (ok: Info.error_typ) =>
  switch (ok) {
  | FreeTypeVariable(name) => [Type.view(Var(name)), text("not found")]
  | BadToken(token) => [
      code_err(token),
      text("not a type or type operator"),
    ]
  | WantConstructorFoundAp
  | WantConstructorFoundType(_) => [text("Expected a constructor")]
  | WantTypeFoundAp => [text("Must be part of a sum type")]
  | DuplicateConstructor(name) => [
      Type.view(Var(name)),
      text("already used in this sum"),
    ]
  };

let rec exp_view = (cls: Term.Cls.t, status: Info.status_exp) =>
  switch (status) {
  | InHole(FreeVariable(name)) =>
    div_err([code_err(name), text("not found")])
  | InHole(InexhaustiveMatch(additional_err)) =>
    let cls_str = Term.Cls.show(cls);
    switch (additional_err) {
    | None => div_err([text(cls_str ++ " is inexhaustive")])
    | Some(err) =>
      let cls_str = String.uncapitalize_ascii(cls_str);
      div_err([
        exp_view(cls, InHole(Common(err))),
        text("; " ++ cls_str ++ " is inexhaustive"),
      ]);
    };
  | InHole(UnusedDeferral) =>
    div_err([text("Deferral must appear as a function argument")])
  | InHole(BadPartialAp(NoDeferredArgs)) =>
    div_err([text("Expected at least one non-deferred argument")])
  | InHole(BadPartialAp(ArityMismatch({expected, actual}))) =>
    div_err([
      text(
        "Arity mismatch: expected "
        ++ string_of_int(expected)
        ++ " argument"
        ++ (expected == 1 ? "" : "s")
        ++ ", got "
        ++ string_of_int(actual)
        ++ " arguments",
      ),
    ])
  | InHole(Common(error)) => div_err(common_err_view(cls, error))
  | NotInHole(AnaDeferralConsistent(ana)) =>
    div_ok([text("Expecting type"), Type.view(ana)])
  | NotInHole(Common(ok)) => div_ok(common_ok_view(cls, ok))
  };

let rec pat_view = (cls: Term.Cls.t, status: Info.status_pat) =>
  switch (status) {
  | InHole(ExpectedConstructor) => div_err([text("Expected a constructor")])
  | InHole(Redundant(additional_err)) =>
    switch (additional_err) {
    | None => div_err([text("Pattern is redundant")])
    | Some(err) =>
      div_err([pat_view(cls, InHole(err)), text("; pattern is redundant")])
    }
  | InHole(Common(error)) => div_err(common_err_view(cls, error))
  | NotInHole(ok) => div_ok(common_ok_view(cls, ok))
  };

let typ_view = (cls: Term.Cls.t, status: Info.status_typ) =>
  switch (status) {
  | NotInHole(ok) => div_ok(typ_ok_view(cls, ok))
  | InHole(err) => div_err(typ_err_view(err))
  };

let tpat_view = (_: Term.Cls.t, status: Info.status_tpat) =>
  switch (status) {
  | NotInHole(Empty) => div_ok([text("Fillable with a new alias")])
  | NotInHole(Var(name)) => div_ok([Type.alias_view(name)])
  | InHole(NotAVar(NotCapitalized)) =>
    div_err([text("Must begin with a capital letter")])
  | InHole(NotAVar(_)) => div_err([text("Expected an alias")])
  | InHole(ShadowsType(name, BaseTyp)) =>
    div_err([text("Can't shadow base type"), Type.view(Var(name))])
  | InHole(ShadowsType(name, TyAlias)) =>
    div_err([text("Can't shadow existing alias"), Type.view(Var(name))])
  | InHole(ShadowsType(name, TyVar)) =>
    div_err([
      text("Can't shadow existing type variable"),
      Type.view(Var(name)),
    ])
  };

let secondary_view = (cls: Term.Cls.t) =>
  div_ok([text(cls |> Term.Cls.show)]);

let view_of_info = (~inject, ~settings, ci): Node.t => {
  let wrapper = status_view =>
    div(
      ~attr=clss(["info"]),
      [term_view(~inject, ~settings, ci), status_view],
    );
  switch (ci) {
  | Secondary(_) => wrapper(div([]))
  | InfoExp({cls, status, _}) => wrapper(exp_view(cls, status))
  | InfoPat({cls, status, _}) => wrapper(pat_view(cls, status))
  | InfoTyp({cls, status, _}) => wrapper(typ_view(cls, status))
  | InfoTPat({cls, status, _}) => wrapper(tpat_view(cls, status))
  };
};

let inspector_view = (~inject, ~settings, ci): Node.t =>
  div(
    ~attr=clss(["cursor-inspector"] @ [Info.is_error(ci) ? errc : okc]),
    [view_of_info(~inject, ~settings, ci)],
  );

let view = (~inject, ~settings: Settings.t, cursor_info: option(Info.t)) => {
  let bar_view = div(~attr=Attr.id("bottom-bar"));
  let err_view = err =>
    bar_view([
      div(
        ~attr=clss(["cursor-inspector", "no-info"]),
        [div(~attr=clss(["icon"]), [Icons.magnify]), text(err)],
      ),
    ]);
  switch (cursor_info) {
  | _ when !settings.core.statics => div_empty
  | None => err_view("Whitespace or Comment")
  | Some(ci) =>
    bar_view([
      inspector_view(~inject, ~settings, ci),
      div(
        ~attr=clss(["id"]),
        [text(String.sub(Id.to_string(Info.id_of(ci)), 0, 4))],
      ),
    ])
  };
};
