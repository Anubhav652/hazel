open Virtual_dom.Vdom;
open Haz3lcore;

let get_goal = (~font_metrics: FontMetrics.t, ~target_id, e) => {
  let rect = JsUtil.get_elem_by_id(target_id)##getBoundingClientRect;
  let goal_x = float_of_int(e##.clientX);
  let goal_y = float_of_int(e##.clientY);
  Measured.Point.{
    row: Float.to_int((goal_y -. rect##.top) /. font_metrics.row_height),
    col:
      Float.(
        to_int(round((goal_x -. rect##.left) /. font_metrics.col_width))
      ),
  };
};

let mousedown_overlay = (~inject, ~font_metrics, ~target_id) =>
  Node.div(
    ~attr=
      Attr.many(
        Attr.[
          id("mousedown-overlay"),
          on_mouseup(_ => inject(Update.Mouseup)),
          on_mousemove(e => {
            let goal = get_goal(~font_metrics, ~target_id, e);
            inject(Update.PerformAction(Select(Goal(goal))));
          }),
        ],
      ),
    [],
  );

let mousedown_handler =
    (~inject, ~font_metrics, ~target_id, ~additional_updates=[], e) => {
  let goal = get_goal(~font_metrics, ~target_id, e);
  Virtual_dom.Vdom.Effect.Many(
    List.map(
      inject,
      Update.[
        Mousedown,
        PerformAction(Move(Goal(goal))),
        ...additional_updates,
      ],
    ),
  );
};

let cell_view =
    (
      ~inject,
      ~font_metrics,
      ~clss=[],
      ~selected: bool,
      ~mousedown: bool,
      ~mousedown_updates: list(Update.t)=[],
      ~show_code: bool,
      ~code_id: string,
      ~caption: option(Node.t)=?,
      code: Node.t,
    )
    : Node.t => {
  let mousedown_overlay =
    selected && mousedown
      ? [mousedown_overlay(~inject, ~font_metrics, ~target_id=code_id)] : [];
  Node.div(
    ~attr=Attr.class_("cell-container"),
    [
      Node.div(
        ~attr=
          Attr.many([
            Attr.classes(
              ["cell", ...clss] @ (selected ? ["selected"] : []),
            ),
            Attr.on_mousedown(
              mousedown_handler(
                ~inject,
                ~font_metrics,
                ~target_id=code_id,
                ~additional_updates=mousedown_updates,
              ),
            ),
          ]),
        Option.to_list(caption)
        @ (show_code ? mousedown_overlay @ [code] : []),
      ),
    ],
  );
};

let deco =
    (
      ~zipper,
      ~measured,
      ~segment,
      ~font_metrics,
      ~show_backpack_targets,
      ~info_map,
    ) => {
  module Deco =
    Deco.Deco({
      let font_metrics = font_metrics;
      let map = measured;
      let show_backpack_targets = show_backpack_targets;
    });
  Deco.all(zipper, segment, info_map);
};

let editor_view =
    (
      ~inject,
      ~font_metrics,
      ~show_backpack_targets,
      ~clss=[],
      ~mousedown: bool,
      ~mousedown_updates: list(Update.t)=[],
      ~settings: Model.settings,
      ~selected: bool,
      ~caption: option(Node.t)=?,
      ~code_id: string,
      ~info_map: Statics.map,
      editor: Editor.t,
    ) => {
  //~eval_result: option(option(DHExp.t))

  let zipper = editor.state.zipper;
  let segment = Zipper.zip(zipper);
  let unselected = Zipper.unselect_and_zip(zipper);
  let measured = editor.state.meta.measured;
  let code_base_view =
    Code.view(~font_metrics, ~segment, ~unselected, ~measured, ~settings);
  let deco_view =
    deco(
      ~zipper,
      ~measured,
      ~segment,
      ~font_metrics,
      ~show_backpack_targets,
      ~info_map,
    );
  let code_view =
    Node.div(
      ~attr=Attr.many([Attr.id(code_id), Attr.classes(["code-container"])]),
      [code_base_view] @ deco_view,
    );
  cell_view(
    ~inject,
    ~font_metrics,
    ~clss,
    ~selected,
    ~mousedown,
    ~mousedown_updates,
    ~code_id,
    ~show_code=true,
    ~caption?,
    code_view,
  );
};

let simple_caption = (caption: string) =>
  Node.div(
    ~attr=Attr.many([Attr.classes(["cell-caption"])]),
    [Node.text(caption)],
  );
