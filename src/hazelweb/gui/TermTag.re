module Vdom = Virtual_dom.Vdom;

type tag_typ =
  | Exp
  | Pat
  | Typ;

let get_cursor_term_tag_typ =
    (cursor_term: CursorInfo_common.cursor_term): tag_typ => {
  switch (cursor_term) {
  | Exp(_, _) => Exp
  | Pat(_, _) => Pat
  | Typ(_, _) => Typ
  | ExpOp(_, _) => Exp
  | PatOp(_, _) => Pat
  | TypOp(_, _) => Typ
  | Line(_, _)
  | Rule(_, _) => Exp
  };
};

let term_tag_view =
    (tag: tag_typ, ~show_tooltip: bool=false, add_classes: list(string))
    : Vdom.Node.t => {
  switch (tag) {
  | Exp =>
    let classes =
      Vdom.Attr.classes(["term-tag", "term-tag-exp", ...add_classes]);
    let attrs =
      show_tooltip
        ? [Vdom.Attr.create("title", "Expression"), classes] : [classes];
    Vdom.(Node.div(attrs, [Node.text("EXP")]));
  | Pat =>
    let classes =
      Vdom.Attr.classes(["term-tag", "term-tag-pat", ...add_classes]);
    let attrs =
      show_tooltip
        ? [Vdom.Attr.create("title", "Pattern"), classes] : [classes];
    Vdom.(Node.div(attrs, [Node.text("PAT")]));
  | Typ =>
    let classes =
      Vdom.Attr.classes(["term-tag", "term-tag-typ", ...add_classes]);
    let attrs =
      show_tooltip
        ? [Vdom.Attr.create("title", "Type"), classes] : [classes];
    Vdom.(Node.div(attrs, [Node.text("TYP")]));
  };
};