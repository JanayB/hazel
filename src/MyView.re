module Vdom = Virtual_dom.Vdom;

type id = string;
type cls = string;

type snode_shape =
  | Block
  | EmptyLine
  | LetLine
  | EmptyHole
  | Var
  | Wild
  | NumLit
  | BoolLit
  | ListNil
  | Lam
  | Inj
  | Case
  | Rule
  | Parenthesized
  | OpSeq
  | Unit
  | Num
  | Bool;

type snode = {
  steps: Path.steps,
  shape: snode_shape,
  err_status,
  var_err_status,
  is_multi_line: bool,
  slines: list(sline),
}
and sline = {
  parent_shape: snode_shape,
  line_no: int,
  swords: list(sword),
}
and sword =
  | Node(snode)
  | Token(stoken)
and stoken =
  | Delim(delim_index, string)
  | Text(string);

let mk_snode =
    (
      steps: Path.steps,
      shape: snode_shape,
      ~err_status=NotInHole,
      ~var_err_status=NotInVHole,
      ~is_multi_line=false,
      swords_by_line: list(list(sword)),
    ) => {
  {
    steps,
    shape,
    err_status,
    var_err_status,
    is_multi_line,
    slines:
      swords_by_line
      |> List.mapi((line_no, swords) =>
           {parent_shape: shape, line_no, swords}
         ),
  };
};

/* TODO */
let id_of_snode = (_: snode): id => "";
let id_of_delim = (_: Path.t): id => "";
let id_of_text = (_: Path.steps): id => "";

let clss_of_snode_shape = (_shape: snode_shape): list(cls) => [""];

let clss_of_snode = (snode: snode): list(cls) => {
  let shape_clss = [
    switch (snode.shape) {
    | Block => "Block"
    | EmptyLine => "EmptyLine"
    | LetLine => "LetLine"
    | EmptyHole => "EmptyHole"
    | Var => "Var"
    | Wild => "Wild"
    | NumLit => "NumLit"
    | BoolLit => "BoolLit"
    | ListNil => "ListNil"
    | Lam => "Lam"
    | Inj => "Inj"
    | Case => "Case"
    | Rule => "Rule"
    | Parenthesized => "Parenthesized"
    | OpSeq => "OpSeq"
    | Unit => "Unit"
    | Num => "Num"
    | Bool => "Bool"
    },
  ];
  let err_status_clss =
    switch (snode.err_status) {
    | NotInHole => []
    | InHole(_, u) => ["in_err_hole", "in_err_hole_" ++ string_of_int(u)]
    };
  let var_err_status_clss =
    switch (snode.var_err_status) {
    | NotInVHole => []
    | InVHole(Free, u) => ["InVHole", "InVHole_" ++ string_of_int(u)]
    | InVHole(Keyword(_), u) => [
        "InVHole",
        "InVHole_" ++ string_of_int(u),
        "Keyword",
      ]
    };
  let multi_line_clss = snode.is_multi_line ? ["multi-line"] : [];
  shape_clss @ err_status_clss @ var_err_status_clss @ multi_line_clss;
};
let clss_of_sline = (_: sline): list(cls) => [""];
let clss_of_stoken = (_: stoken): list(cls) => [""];

/*
 let delim_txt_click_handler = evt => {
   switch (Js.Opt.to_option(evt##.target)) {
   | None => JSUtil.move_caret_to(delim_before_elem)
   | Some(target) =>
     let x = evt##.clientX;
     x * 2 <= target##.offsetWidth
       ? JSUtil.move_caret_to(delim_before_elem)
       : JSUtil.move_caret_to(delim_after_elem)
   };
   false;
 }
 */

let rec vdom_of_snode = (do_action: Action.t => unit, snode: snode) => {
  let vdom_lines =
    snode.slines |> List.map(vdom_of_sline(do_action, snode.steps));
  Vdom.Node.div(
    [
      Vdom.Attr.id(id_of_snode(snode)),
      Vdom.Attr.classes(clss_of_snode(snode)),
    ],
    vdom_lines,
  );
}
and vdom_of_sline =
    (do_action: Action.t => unit, node_steps: Path.steps, sline: sline) => {
  let vdom_words =
    sline.swords
    |> List.map(sword =>
         switch (sword) {
         | Node(snode) => vdom_of_snode(do_action, snode)
         | Token(stoken) => vdom_of_stoken(do_action, node_steps, stoken)
         }
       );
  Vdom.Node.div([Vdom.Attr.classes(clss_of_sline(sline))], vdom_words);
}
and vdom_of_stoken =
    (_do_action: Action.t => unit, node_steps: Path.steps, stoken: stoken) => {
  let clss = clss_of_stoken(stoken);
  switch (stoken) {
  | Delim(k, s) =>
    let delim_before =
      Vdom.Node.div(
        [
          Vdom.Attr.id(id_of_delim((node_steps, (k, Before)))),
          Vdom.Attr.classes(["delim-before"]),
        ],
        [],
      );
    let delim_after =
      Vdom.Node.div(
        [
          Vdom.Attr.id(id_of_delim((node_steps, (k, After)))),
          Vdom.Attr.classes(["delim-after"]),
        ],
        [],
      );
    let delim_txt =
      Vdom.Node.div(
        [
          Vdom.Attr.create("contenteditable", "false"),
          Vdom.Attr.classes(["delim-txt"]),
        ],
        [Vdom.Node.text(s)],
      );
    Vdom.Node.div(
      [Vdom.Attr.classes(clss)],
      [delim_before, delim_txt, delim_after],
    );
  | Text(s) =>
    Vdom.Node.div(
      [Vdom.Attr.id(id_of_text(node_steps)), Vdom.Attr.classes(clss)],
      [Vdom.Node.text(s)],
    )
  };
};

/* TODO */
let is_multi_line_block = (_: UHExp.block) => false;
let is_multi_line_exp = (_: UHExp.t) => false;

/* TODO */
let snode_of_typ = (steps: Path.steps, _uty: UHTyp.t): snode =>
  mk_snode(steps, EmptyHole, [[]]);
let snode_of_pat = (steps: Path.steps, _p: UHPat.t): snode =>
  mk_snode(steps, EmptyHole, [[]]);

let rec snode_of_block =
        (steps: Path.steps, Block(line_items, e): UHExp.block): snode => {
  let snode_line_items =
    line_items |> List.mapi((i, li) => snode_of_line_item(steps @ [i], li));
  let snode_e = snode_of_exp(steps @ [List.length(line_items)], e);
  let swords_by_line =
    snode_line_items @ [snode_e] |> List.map(snode => [Node(snode)]);
  let is_multi_line = List.length(line_items) != 0;
  mk_snode(steps, Block, ~is_multi_line, swords_by_line);
}
and snode_of_line_item = (steps: Path.steps, li: UHExp.line): snode =>
  switch (li) {
  | EmptyLine => mk_snode(steps, EmptyLine, [[]])
  | ExpLine(e) => snode_of_exp(steps, e) /* ghost node */
  | LetLine(p, ann, def) =>
    let snode_p = snode_of_pat(steps @ [0], p);
    let swords_ann =
      switch (ann) {
      | None => []
      | Some(uty) => [
          Token(Delim(1, ":")),
          Node(snode_of_typ(steps @ [1], uty)),
        ]
      };
    let snode_def = snode_of_block(steps @ [2], def);
    let is_multi_line = is_multi_line_block(def);
    mk_snode(
      steps,
      LetLine,
      ~is_multi_line,
      [
        [Token(Delim(0, "let")), Node(snode_p)]
        @ swords_ann
        @ [Token(Delim(2, "="))],
        [Node(snode_def)],
      ],
    );
  }
and snode_of_exp = (steps: Path.steps, e: UHExp.t): snode =>
  switch (e) {
  /* outer nodes */
  | EmptyHole(u) =>
    mk_snode(steps, EmptyHole, [[Token(Delim(0, string_of_int(u + 1)))]])
  | Var(err_status, var_err_status, x) =>
    mk_snode(steps, Var, ~err_status, ~var_err_status, [[Token(Text(x))]])
  | NumLit(err_status, n) =>
    mk_snode(
      steps,
      NumLit,
      ~err_status,
      [[Token(Text(string_of_int(n)))]],
    )
  | BoolLit(err_status, b) =>
    mk_snode(
      steps,
      BoolLit,
      ~err_status,
      [[Token(Text(string_of_bool(b)))]],
    )
  | ListNil(err_status) =>
    mk_snode(steps, ListNil, ~err_status, [[Token(Delim(0, "[]"))]])
  /* inner nodes */
  | Lam(err_status, arg, ann, body) =>
    let snode_arg = snode_of_pat(steps @ [0], arg);
    let swords_ann =
      switch (ann) {
      | None => []
      | Some(uty) => [
          Token(Delim(1, ":")),
          Node(snode_of_typ(steps @ [1], uty)),
        ]
      };
    let snode_body = snode_of_block(steps @ [2], body);
    let is_multi_line = is_multi_line_block(body);
    mk_snode(
      steps,
      Lam,
      ~err_status,
      ~is_multi_line,
      [
        [Token(Delim(0, LangUtil.lamSym)), Node(snode_arg)]
        @ swords_ann
        @ [Token(Delim(2, "."))],
        [Node(snode_body)],
      ],
    );
  | Inj(err_status, side, body) =>
    let snode_body = snode_of_block(steps @ [0], body);
    let is_multi_line = is_multi_line_block(body);
    mk_snode(
      steps,
      Inj,
      ~err_status,
      ~is_multi_line,
      [
        [Token(Delim(0, "inj[" ++ LangUtil.string_of_side(side) ++ "]("))],
        [Node(snode_body)],
        [Token(Delim(1, ")"))],
      ],
    );
  | Case(err_status, scrut, rules, ann) =>
    let snode_scrut = snode_of_block(steps @ [0], scrut);
    let slines_rules =
      rules
      |> List.mapi((i, rule) => snode_of_rule(steps @ [i], rule))
      |> List.map(snode => [Node(snode)]);
    let swords_end =
      switch (ann) {
      | None => [Token(Delim(1, "end"))]
      | Some(uty) => [
          Token(Delim(1, "end :")),
          Node(snode_of_typ(steps @ [List.length(rules) + 1], uty)),
        ]
      };
    mk_snode(
      steps,
      Case,
      ~err_status,
      ~is_multi_line=true,
      [[Token(Delim(0, "case")), Node(snode_scrut)]]
      @ slines_rules
      @ [swords_end],
    );
  | Parenthesized(body) =>
    let snode_body = snode_of_block(steps @ [0], body);
    let is_multi_line = is_multi_line_block(body);
    mk_snode(
      steps,
      Parenthesized,
      ~is_multi_line,
      [
        [Token(Delim(0, "("))],
        [Node(snode_body)],
        [Token(Delim(1, ")"))],
      ],
    );
  | OpSeq(_skel, _seq) =>
    /* TODO */
    mk_snode(steps, OpSeq, [[]])
  | ApPalette(_, _, _, _) => raise(InvariantViolated)
  }
/* TODO */
and snode_of_rule = (steps: Path.steps, Rule(_pat, _clause): UHExp.rule) =>
  mk_snode(steps, Rule, [[]]);
