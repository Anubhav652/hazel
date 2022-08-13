open Virtual_dom.Vdom;
open Node;
open Util.Web;

let ty_view = (cls: string, s: string): Node.t =>
  div([clss(["typ-view", cls])], [text(s)]);

let prov_view: Core.Typ.type_provenance => Node.t =
  fun
  | Internal => div([], [])
  | TypeHole => div([clss(["typ-mod", "type-hole"])], [text("𝜏")])
  | SynSwitch => div([clss(["typ-mod", "syn-switch"])], [text("⇒")]);

let rec view = (ty: Core.Typ.t): Node.t =>
  //TODO(andrew): parens on ops when ambiguous
  switch (ty) {
  | Unknown(prov) =>
    div(
      [clss(["typ-view", "atom", "unknown"])],
      [text("?"), prov_view(prov)],
    )
  | Unit => ty_view("()", "()")
  | Int => ty_view("Int", "Int")
  | Float => ty_view("Float", "Float")
  | Bool => ty_view("Bool", "Bool")
  | List(t) =>
    div(
      [clss(["typ-view", "atom", "List"])],
      [text("["), view(t), text("]")],
    )
  | Arrow(t1, t2) =>
    div([clss(["typ-view", "Arrow"])], [view(t1), text("->"), view(t2)])
  | Prod(t1, t2) =>
    div([clss(["typ-view", "Prod"])], [view(t1), text(","), view(t2)])
  };