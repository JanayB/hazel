module Doc = Pretty.Doc;
open Sexplib.Std;

[@deriving sexp]
type token_shape =
  | Text
  | Op
  | Delim(DelimIndex.t);

[@deriving sexp]
type t =
  | Indent
  | Padding
  | HoleLabel({len: int})
  | Token({
      shape: token_shape,
      len: int,
      has_cursor: option(int),
    })
  | SpaceOp
  | UserNewline
  | OpenChild({is_inline: bool})
  | ClosedChild({is_inline: bool})
  | DelimGroup
  | EmptyLine
  | LetLine
  | Step(int)
  | Term(term_data)
  | LivelitName
  | LivelitView({
      llu: MetaVar.t,
      llview:
        [@sexp.opaque] (Livelits.trigger_serialized => Livelits.LivelitView.t),
      sim_dargs_opt:
        option(
          (
            SpliceInfo.splice_map(option(DHExp.t)),
            list((Var.t, option(DHExp.t))),
          ),
        ),
    })
[@deriving sexp]
and term_data = {
  has_cursor: bool,
  shape: term_shape,
  sort: TermSort.t,
}
[@deriving sexp]
and term_shape =
  | Rule
  | Case({err: ErrStatus.t})
  | Var({
      err: ErrStatus.t,
      verr: VarErrStatus.t,
      show_use: bool,
    })
  | Operand({err: ErrStatus.t})
  | FreeLivelit
  | ApLivelit
  | BinOp({
      op_index: int,
      err: ErrStatus.t,
    })
  | NTuple({
      comma_indices: list(int),
      err: ErrStatus.t,
    })
  | SubBlock({hd_index: int});

let mk_Var =
    (
      ~err: ErrStatus.t=NotInHole,
      ~verr: VarErrStatus.t=NotInVarHole,
      ~show_use=false,
      (),
    ) =>
  Var({err, verr, show_use});

let mk_Operand = (~err: ErrStatus.t=NotInHole, ()) => Operand({err: err});

let mk_Token = (~has_cursor=None, ~len: int, ~shape: token_shape, ()) =>
  Token({has_cursor, len, shape});
let mk_Term =
    (~has_cursor=false, ~shape: term_shape, ~sort: TermSort.t, ()): t =>
  Term({has_cursor, shape, sort});
let mk_OpenChild = (~is_inline: bool, ()) =>
  OpenChild({is_inline: is_inline});
let mk_ClosedChild = (~is_inline: bool, ()) =>
  ClosedChild({is_inline: is_inline});