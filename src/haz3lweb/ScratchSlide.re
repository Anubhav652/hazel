open Haz3lcore;

[@deriving (show({with_path: false}), sexp, yojson)]
type state = (Id.t, Editor.t);

[@deriving (show({with_path: false}), sexp, yojson)]
type persistent_state = (Id.t, PersistentZipper.t);

let editor_of_state = ((_, editor): state) => editor;

let id_of_state = ((id, _): state) => id;

let put_editor_and_id = ((_, _): state, id, editor) => (id, editor);

let scratch_key = "scratch";
let spliced_elabs = (~ctx_init: Ctx.t, (_, editor: Editor.t)) => {
  let seg =
    Zipper.smart_seg(
      editor.state.zipper,
      ~erase_buffer=true,
      ~dump_backpack=true,
    );
  print_endline("zipper: " ++ Zipper.show(editor.state.zipper));

  /*print_endline(
      "spliced_elabs: smart_seg:" ++ Printer.of_segment(~holes=Some("?"), seg),
    );*/
  print_endline("spliced_elabs: seg:" ++ Segment.show(seg));
  let (term, _) = MakeTerm.go(seg);
  print_endline("spliced_elabs: term:" ++ Term.UExp.show(term));
  let info_map = Statics.mk_map_ctx(ctx_init, term);
  [(scratch_key, Interface.elaborate(info_map, term))];
};

let persist = ((id, editor: Editor.t)) => {
  (id, PersistentZipper.persist(editor.state.zipper));
};

let unpersist = ((id, zipper): persistent_state) => {
  let (id, zipper) = PersistentZipper.unpersist(zipper, id);
  (id, Editor.init(zipper, ~read_only=false));
};

let serialize = (state: state) => {
  persist(state) |> sexp_of_persistent_state |> Sexplib.Sexp.to_string;
};

let deserialize = (data: string) => {
  Sexplib.Sexp.of_string(data) |> persistent_state_of_sexp |> unpersist;
};

let deserialize_opt = (data: string) => {
  let sexp =
    try(Some(Sexplib.Sexp.of_string(data) |> persistent_state_of_sexp)) {
    | _ => None
    };
  sexp |> Option.map(sexp => sexp |> unpersist);
};

let export = (state: state) => {
  state |> persist |> yojson_of_persistent_state;
};

let import = (data: string) => {
  data |> Yojson.Safe.from_string |> persistent_state_of_yojson |> unpersist;
};

let export_init = (state: state) => {
  state |> persist |> show_persistent_state;
};
