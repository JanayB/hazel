open Sexplib.Std;

type cursor_term = CursorInfo.cursor_term;

type start_from_insertion = bool;
type delete_edit =
  | Term(cursor_term, start_from_insertion)
  | Space
  | EmptyLine
  | TypeAnn;

type edit_term = {
  start_from: cursor_term,
  end_with: cursor_term,
};
type var_edit =
  | Insert(cursor_term)
  | Edit(edit_term);

type edit_action =
  | Var(var_edit)
  | DeleteEdit(delete_edit)
  | ConstructEdit(Action.shape)
  /* SLine in Action.shape stands for both empty line and case rule,
     so an extra type CaseRule is added for construction */
  | CaseRule
  | Init;

type cursor_term_info = {
  cursor_term_before: cursor_term,
  cursor_term_after: cursor_term,
  zexp_before: ZExp.t,
  zexp_after: ZExp.t,
  prev_is_empty_line: bool,
  next_is_empty_line: bool,
};
type timestamp = float;
type undo_history_entry = {
  /* cardstacks after non-movement action applied */
  cardstacks_after_action: Cardstacks.t,
  /* cardstacks_after_move is initially the same as cardstacks_after_action.
     if there is a movement action, update it. */
  cardstacks_after_move: Cardstacks.t,
  cursor_term_info,
  previous_action: Action.t,
  edit_action,
  timestamp,
};

type undo_history_group = {
  group_entries: ZList.t(undo_history_entry, undo_history_entry),
  is_expanded: bool,
};

[@deriving sexp]
type t = {
  groups: [@sexp.opaque] ZList.t(undo_history_group, undo_history_group),
  all_hidden_history_expand: bool,
  /* disable the auto-scrolling when disable_auto_scrolling is true */
  disable_auto_scrolling: bool,
  preview_on_hover: bool,
  hover_recover_group_id: int,
  hover_recover_elt_id: int,
  cur_group_id: int,
  cur_elt_id: int,
};

let update_disable_auto_scrolling = (disable_auto_scrolling: bool, history: t) => {
  {...history, disable_auto_scrolling};
};

let disable_redo = (undo_history: t): bool => {
  /* if there is no entry after current entry,
     then history is at the latest entry. redo should be disabled */
  List.length(ZList.prj_prefix(undo_history.groups)) == 0
  && List.length(
       ZList.prj_prefix(ZList.prj_z(undo_history.groups).group_entries),
     )
  == 0;
};
let disable_undo = (undo_history: t): bool => {
  /* if there is no entry before current entry,
     then history is at the initial entry, undo should be disabled */
  List.length(ZList.prj_suffix(undo_history.groups)) == 0
  && List.length(
       ZList.prj_suffix(ZList.prj_z(undo_history.groups).group_entries),
     )
  == 0;
};

let get_cardstacks = (history: t, ~is_after_move: bool): Cardstacks.t => {
  let cur_entry = ZList.prj_z(ZList.prj_z(history.groups).group_entries);
  if (is_after_move) {
    cur_entry.cardstacks_after_move;
  } else {
    cur_entry.cardstacks_after_action;
  };
};

let get_cursor_pos = (cursor_term: cursor_term): CursorPosition.t => {
  switch (cursor_term) {
  | Exp(cursor_pos, _)
  | Pat(cursor_pos, _)
  | Typ(cursor_pos, _)
  | ExpOp(cursor_pos, _)
  | PatOp(cursor_pos, _)
  | TypOp(cursor_pos, _)
  | Line(cursor_pos, _)
  | Rule(cursor_pos, _) => cursor_pos
  };
};

let push_history_entry =
    (
      ~prev_group: undo_history_group,
      ~new_entry: undo_history_entry,
      ~is_expanded: bool,
    )
    : undo_history_group => {
  {
    group_entries: (
      [],
      new_entry,
      [
        ZList.prj_z(prev_group.group_entries),
        ...ZList.prj_suffix(prev_group.group_entries),
      ],
    ),
    is_expanded,
  };
};

/* return true if caret jump to another term when new action applied */
let caret_jump =
    (prev_group: undo_history_group, new_cardstacks_before: Cardstacks.t)
    : bool => {
  let prev_entry = ZList.prj_z(prev_group.group_entries);
  let prev_step =
    prev_entry.cardstacks_after_action
    |> Cardstacks.get_program
    |> Program.get_steps;
  let new_step =
    new_cardstacks_before |> Cardstacks.get_program |> Program.get_steps;
  prev_step != new_step;
};

/* return true if new edit_action can be grouped with the preivous edit_action */
let group_edit_action =
    (edit_action_prev: edit_action, edit_action_next: edit_action): bool =>
  switch (edit_action_prev, edit_action_next) {
  | (CaseRule, CaseRule) => true
  | (CaseRule, _) => false
  | (Var(_), Var(_)) => true
  | (Var(_), DeleteEdit(delete_edit)) =>
    switch (delete_edit) {
    | Term(_, _) => true
    | Space
    | EmptyLine
    | TypeAnn => false
    }
  | (Var(_), ConstructEdit(construct_edit)) =>
    switch (construct_edit) {
    | SLet
    | SCase => true
    | _ => false
    }
  | (Var(_), _) => false
  /* "insert" and "insert and delete" should be grouped together */
  | (DeleteEdit(Term(_, true)), Var(Insert(_))) => true
  | (DeleteEdit(_), _) => false
  | (ConstructEdit(_), _) => false
  | (Init, _) => false
  };

/* return true if new entry can be grouped into the previous group */
let group_entry =
    (
      ~prev_group: undo_history_group,
      ~new_cardstacks_before: Cardstacks.t,
      ~new_edit_action: edit_action,
    )
    : bool => {
  let prev_entry = ZList.prj_z(prev_group.group_entries);
  group_edit_action(prev_entry.edit_action, new_edit_action)
  && !caret_jump(prev_group, new_cardstacks_before);
};

type comp_len_typ =
  | MaxLen
  | MinLen
  | Len(int);

let comp_len_lt =
    (cursor_len_prev: comp_len_typ, cursor_len_next: comp_len_typ): bool => {
  switch (cursor_len_prev, cursor_len_next) {
  | (MaxLen, MaxLen) => false
  | (_, MaxLen) => true
  | (MinLen, _) => true
  | (_, MinLen) => false
  | (MaxLen, _) => false
  | (Len(len1), Len(len2)) => len1 <= len2
  };
};

let cursor_term_len = (cursor_term: cursor_term): comp_len_typ => {
  switch (cursor_term) {
  | Exp(_, operand) =>
    switch (operand) {
    | EmptyHole(_) => MinLen
    | Var(_, _, var) => Len(Var.length(var))
    | IntLit(_, num)
    | FloatLit(_, num) => Len(String.length(num))
    | BoolLit(_, _)
    | ListNil(_)
    | Lam(_, _, _, _)
    | Inj(_, _, _)
    | Case(_, _, _, _)
    | Parenthesized(_) => MaxLen
    | ApPalette(_, _, _, _) => failwith("ApPalette not implemented")
    }
  | Pat(_, operand) =>
    switch (operand) {
    | EmptyHole(_) => MinLen
    | Wild(_) => Len(1)
    | Var(_, _, var) => Len(Var.length(var))
    | IntLit(_, num)
    | FloatLit(_, num) => Len(String.length(num))
    | BoolLit(_, _)
    | ListNil(_)
    | Parenthesized(_)
    | Inj(_, _, _) => MaxLen
    }
  | Typ(_, operand) =>
    switch (operand) {
    | Hole => MinLen
    | Unit
    | Int
    | Float
    | Bool
    | Parenthesized(_)
    | List(_) => MaxLen
    }
  | ExpOp(_, _)
  | PatOp(_, _)
  | TypOp(_, _)
  | Rule(_, _) => MaxLen
  | Line(_, line) =>
    switch (line) {
    | EmptyLine => MinLen
    | LetLine(_, _, _)
    | ExpLine(_) => MaxLen
    }
  };
};
let comp_len_larger =
    (cursor_term_prev: cursor_term, cursor_term_next: cursor_term)
    : cursor_term =>
  if (comp_len_lt(
        cursor_term_len(cursor_term_prev),
        cursor_term_len(cursor_term_next),
      )) {
    cursor_term_next;
  } else {
    cursor_term_prev;
  };

let get_initial_entry_in_group =
    (undo_history_group: undo_history_group): option(undo_history_entry) => {
  switch (List.rev(ZList.join(undo_history_group.group_entries))) {
  | [] => None
  | [head, ..._] => Some(head)
  };
};

let is_var_insert = (edit_action): bool => {
  switch (edit_action) {
  | Var(var_edit) =>
    switch (var_edit) {
    | Insert(_) => true
    | Edit(_) => false
    }
  | _ => false
  };
};

let is_var_edit = (edit_action): bool => {
  switch (edit_action) {
  | Var(var_edit) =>
    switch (var_edit) {
    | Insert(_) => false
    | Edit(_) => true
    }
  | _ => false
  };
};
let get_delete_edit_action =
    (
      prev_group: undo_history_group,
      new_cardstacks_before: Cardstacks.t,
      new_cursor_term_info: cursor_term_info,
    )
    : edit_action =>
  if (group_entry(
        ~prev_group,
        ~new_cardstacks_before,
        ~new_edit_action=
          DeleteEdit(Term(new_cursor_term_info.cursor_term_before, true)),
      )) {
    let prev_entry = ZList.prj_z(prev_group.group_entries);
    if (is_var_edit(prev_entry.edit_action)) {
      /* if delete group start from var edition, the deleted term is the initial term in this group */
      switch (get_initial_entry_in_group(prev_group)) {
      | None =>
        DeleteEdit(Term(new_cursor_term_info.cursor_term_before, false))
      | Some(initial_entry) =>
        DeleteEdit(
          Term(initial_entry.cursor_term_info.cursor_term_before, false),
        )
      };
    } else if (is_var_insert(prev_entry.edit_action)) {
      /* if delete group start from var insertion, the deleted term is the longest term in this group */
      let rec max_len_term =
              (ls: list(undo_history_entry), cursor_term: cursor_term)
              : cursor_term => {
        switch (ls) {
        | [] => cursor_term
        | [elt] =>
          switch (
            elt.cursor_term_info.cursor_term_before,
            elt.cursor_term_info.cursor_term_after,
          ) {
          | (Exp(_, exp_1), Exp(_, exp_2)) =>
            switch (exp_1, exp_2) {
            | (Var(_, _, _), Var(_, _, _))
            | (IntLit(_, _), IntLit(_, _))
            | (FloatLit(_, _), FloatLit(_, _))
            | (BoolLit(_, _), BoolLit(_, _)) =>
              comp_len_larger(
                cursor_term,
                comp_len_larger(
                  elt.cursor_term_info.cursor_term_after,
                  elt.cursor_term_info.cursor_term_before,
                ),
              )
            | _ =>
              comp_len_larger(
                cursor_term,
                elt.cursor_term_info.cursor_term_after,
              )
            }
          | (Pat(_, pat_1), Pat(_, pat_2)) =>
            switch (pat_1, pat_2) {
            | (Var(_, _, _), Var(_, _, _))
            | (IntLit(_, _), IntLit(_, _))
            | (FloatLit(_, _), FloatLit(_, _))
            | (BoolLit(_, _), BoolLit(_, _)) =>
              comp_len_larger(
                cursor_term,
                comp_len_larger(
                  elt.cursor_term_info.cursor_term_after,
                  elt.cursor_term_info.cursor_term_before,
                ),
              )
            | _ =>
              comp_len_larger(
                cursor_term,
                elt.cursor_term_info.cursor_term_after,
              )
            }
          | _ =>
            comp_len_larger(
              cursor_term,
              elt.cursor_term_info.cursor_term_after,
            )
          }
        | [head, ...tail] =>
          let larger_term =
            comp_len_larger(
              cursor_term,
              head.cursor_term_info.cursor_term_after,
            );
          max_len_term(tail, larger_term);
        };
      };
      DeleteEdit(
        Term(
          max_len_term(
            [
              ZList.prj_z(prev_group.group_entries),
              ...ZList.prj_suffix(prev_group.group_entries),
            ],
            new_cursor_term_info.cursor_term_before,
          ),
          true,
        ),
      );
    } else {
      DeleteEdit(Term(new_cursor_term_info.cursor_term_before, false));
    };
  } else {
    DeleteEdit(Term(new_cursor_term_info.cursor_term_before, false));
  };
let has_typ_ann = (cursor_term: cursor_term): bool => {
  switch (cursor_term) {
  | Exp(_, exp) =>
    switch (exp) {
    | Lam(_, _, _, _) => true
    | _ => false
    }
  | Line(_, line_content) =>
    switch (line_content) {
    | LetLine(_, _, _) => true
    | _ => false
    }
  | _ => false
  };
};

let is_move_action = (cursor_term_info: cursor_term_info): bool => {
  ZExp.erase(cursor_term_info.zexp_before)
  == ZExp.erase(cursor_term_info.zexp_after);
};
let delete_edit =
    (
      ~prev_group: undo_history_group,
      ~new_cardstacks_before: Cardstacks.t,
      ~new_cursor_term_info: cursor_term_info,
    )
    : option(edit_action) =>
  if (CursorInfo.is_empty_line(new_cursor_term_info.cursor_term_before)) {
    Some(DeleteEdit(EmptyLine));
  } else if (CursorInfo.is_empty_line(new_cursor_term_info.cursor_term_after)
             || CursorInfo.is_hole(new_cursor_term_info.cursor_term_after)) {
    /* if the term becomes hole or empty line, the edit action is deleting the whole term */
    Some(
      get_delete_edit_action(
        prev_group,
        new_cardstacks_before,
        new_cursor_term_info,
      ),
    );
  } else {
    /* the edit action is editing the term */
    let prev_entry = ZList.prj_z(prev_group.group_entries);
    if (!caret_jump(prev_group, new_cardstacks_before)
        && is_var_insert(prev_entry.edit_action)) {
      Some(Var(Insert(new_cursor_term_info.cursor_term_after)));
    } else {
      let initial_term =
        if (group_entry(
              ~prev_group,
              ~new_cardstacks_before,
              ~new_edit_action=
                Var(
                  Edit({
                    start_from: new_cursor_term_info.cursor_term_before,
                    end_with: new_cursor_term_info.cursor_term_after,
                  }),
                ),
            )) {
          switch (get_initial_entry_in_group(prev_group)) {
          | None => new_cursor_term_info.cursor_term_before
          | Some(initial_entry) =>
            initial_entry.cursor_term_info.cursor_term_before
          };
        } else {
          new_cursor_term_info.cursor_term_before;
        };
      Some(
        Var(
          Edit({
            start_from: initial_term,
            end_with: new_cursor_term_info.cursor_term_after,
          }),
        ),
      );
    };
  };
let delim_edge_handle =
    (~new_cursor_term_info: cursor_term_info, ~adjacent_is_empty_line: bool)
    : option(edit_action) =>
  if (adjacent_is_empty_line) {
    /* delete adjacent empty line */
    Some(DeleteEdit(EmptyLine));
  } else if (CursorInfo.is_hole(new_cursor_term_info.cursor_term_before)) {
    /* delete space */
    Some(DeleteEdit(Space));
  } else {
    None;
        /* jump to next term */
  };
let delete =
    (
      ~prev_group: undo_history_group,
      ~new_cardstacks_before: Cardstacks.t,
      ~new_cursor_term_info: cursor_term_info,
    )
    : option(edit_action) => {
  let cursor_pos = get_cursor_pos(new_cursor_term_info.cursor_term_before);

  switch (cursor_pos) {
  | OnText(_) =>
    if (CursorInfo.caret_is_after_zoperand(new_cursor_term_info.zexp_before)) {
      delim_edge_handle(
        ~new_cursor_term_info,
        ~adjacent_is_empty_line=new_cursor_term_info.next_is_empty_line,
      );
    } else {
      //edit var
      delete_edit(
        ~prev_group,
        ~new_cardstacks_before,
        ~new_cursor_term_info,
      );
    }
  | OnDelim(pos, side) =>
    switch (side) {
    | Before =>
      if (pos == 1 && has_typ_ann(new_cursor_term_info.cursor_term_before)) {
        /* num==1 is the position of ':' in an expression */
        Some(
          DeleteEdit(TypeAnn),
        );
      } else {
        /* delete the whole term */
        Some(
          get_delete_edit_action(
            prev_group,
            new_cardstacks_before,
            new_cursor_term_info,
          ),
        );
      }
    | After =>
      delim_edge_handle(
        ~new_cursor_term_info,
        ~adjacent_is_empty_line=new_cursor_term_info.next_is_empty_line,
      )
    }
  | OnOp(side) =>
    switch (side) {
    | Before =>
      /* delete and reach a hole */
      Some(
        get_delete_edit_action(
          prev_group,
          new_cardstacks_before,
          new_cursor_term_info,
        ),
      )
    | After =>
      /* move caret to next term, just ignore this move */
      None
    }
  };
};

let backspace =
    (
      ~prev_group: undo_history_group,
      ~new_cardstacks_before: Cardstacks.t,
      ~new_cursor_term_info: cursor_term_info,
    )
    : option(edit_action) => {
  let cursor_pos = get_cursor_pos(new_cursor_term_info.cursor_term_before);
  switch (cursor_pos) {
  | OnText(_) =>
    if (CursorInfo.caret_is_before_zoperand(new_cursor_term_info.zexp_before)) {
      delim_edge_handle(
        ~new_cursor_term_info,
        ~adjacent_is_empty_line=new_cursor_term_info.prev_is_empty_line,
      );
    } else {
      //edit var
      delete_edit(
        ~prev_group,
        ~new_cardstacks_before,
        ~new_cursor_term_info,
      );
    }
  | OnDelim(pos, side) =>
    switch (side) {
    | Before =>
      delim_edge_handle(
        ~new_cursor_term_info,
        ~adjacent_is_empty_line=new_cursor_term_info.prev_is_empty_line,
      )
    | After =>
      if (pos == 1 && has_typ_ann(new_cursor_term_info.cursor_term_before)) {
        /* num==1 is the position of ':' in an expression */
        Some(
          DeleteEdit(TypeAnn),
        );
      } else {
        /* delete the whole term */
        Some(
          get_delete_edit_action(
            prev_group,
            new_cardstacks_before,
            new_cursor_term_info,
          ),
        );
      }
    }
  | OnOp(side) =>
    switch (side) {
    | Before =>
      /* move caret to next term, just ignore this move */
      None
    | After =>
      /* delete and reach a hole */
      Some(
        get_delete_edit_action(
          prev_group,
          new_cardstacks_before,
          new_cursor_term_info,
        ),
      )
    }
  };
};

let get_new_edit_action =
    (
      ~prev_group: undo_history_group,
      ~new_cardstacks_before: Cardstacks.t,
      ~new_cursor_term_info: cursor_term_info,
      ~action: Action.t,
    )
    : option(edit_action) =>
  if (is_move_action(new_cursor_term_info)) {
    None;
        /* It's a caret movement */
  } else {
    switch (action) {
    | Delete =>
      delete(~prev_group, ~new_cardstacks_before, ~new_cursor_term_info)
    | Backspace =>
      backspace(~prev_group, ~new_cardstacks_before, ~new_cursor_term_info)
    | Construct(shape) =>
      switch (shape) {
      | SLine =>
        switch (CursorInfo.get_outer_zrules(new_cursor_term_info.zexp_before)) {
        | None => Some(ConstructEdit(shape))
        | Some(zrules_before) =>
          switch (
            CursorInfo.get_outer_zrules(new_cursor_term_info.zexp_after)
          ) {
          | None => Some(ConstructEdit(shape))
          | Some(zrules_after) =>
            if (ZList.length(zrules_before) < ZList.length(zrules_after)) {
              Some(CaseRule);
            } else {
              Some(ConstructEdit(shape));
            }
          }
        }
      | SParenthesized
      | SList
      | SAsc
      | SLam
      | SListNil
      | SInj(_)
      | SLet
      | SCase => Some(ConstructEdit(shape))
      | SChar(_) =>
        if (group_entry(
              ~prev_group,
              ~new_cardstacks_before,
              ~new_edit_action=
                Var(
                  Edit({
                    start_from: new_cursor_term_info.cursor_term_before,
                    end_with: new_cursor_term_info.cursor_term_after,
                  }),
                ),
            )) {
          switch (get_initial_entry_in_group(prev_group)) {
          | None =>
            Some(Var(Insert(new_cursor_term_info.cursor_term_after)))
          | Some(initial_entry) =>
            if (is_var_insert(initial_entry.edit_action)) {
              Some(Var(Insert(new_cursor_term_info.cursor_term_after)));
            } else {
              Some(
                Var(
                  Edit({
                    start_from:
                      initial_entry.cursor_term_info.cursor_term_before,
                    end_with: new_cursor_term_info.cursor_term_after,
                  }),
                ),
              );
            }
          };
        } else if (CursorInfo.is_hole(new_cursor_term_info.cursor_term_before)
                   || CursorInfo.is_empty_line(
                        new_cursor_term_info.cursor_term_before,
                      )
                   || !
                        CursorInfo.cursor_term_is_editable(
                          new_cursor_term_info.cursor_term_before,
                        )) {
          Some(Var(Insert(new_cursor_term_info.cursor_term_after)));
        } else {
          Some(
            Var(
              Edit({
                start_from: new_cursor_term_info.cursor_term_before,
                end_with: new_cursor_term_info.cursor_term_after,
              }),
            ),
          );
        }

      | SOp(op) =>
        switch (op) {
        | SMinus
        | SPlus
        | STimes
        | SLessThan
        | SGreaterThan
        | SEquals
        | SComma
        | SArrow
        | SVBar
        | SCons
        | SAnd
        | SOr => Some(ConstructEdit(shape))
        | SSpace =>
          switch (new_cursor_term_info.cursor_term_before) {
          | Exp(pos, uexp_operand) =>
            switch (uexp_operand) {
            | Var(_, InVarHole(Keyword(k), _), _) =>
              switch (k) {
              | Let =>
                switch (
                  get_cursor_pos(new_cursor_term_info.cursor_term_before)
                ) {
                | OnText(pos) =>
                  if (pos == 3) {
                    /* the caret is at the end of "let" */
                    Some(
                      ConstructEdit(SLet),
                    );
                  } else {
                    Some(ConstructEdit(SOp(SSpace)));
                  }
                | OnDelim(_, _)
                | OnOp(_) => Some(ConstructEdit(SOp(SSpace)))
                }

              | Case =>
                switch (
                  get_cursor_pos(new_cursor_term_info.cursor_term_before)
                ) {
                | OnText(pos) =>
                  if (pos == 4) {
                    /* the caret is at the end of "case" */
                    Some(
                      ConstructEdit(SCase),
                    );
                  } else {
                    Some(ConstructEdit(SOp(SSpace)));
                  }
                | OnDelim(_, _)
                | OnOp(_) => Some(ConstructEdit(SOp(SSpace)))
                }
              }
            | Var(_, _, var) =>
              switch (pos) {
              | OnText(index) =>
                let (left_var, _) = Var.split(index, var);
                if (Var.is_let(left_var)) {
                  Some(ConstructEdit(SLet));
                } else if (Var.is_case(left_var)) {
                  Some(ConstructEdit(SCase));
                } else {
                  Some(ConstructEdit(SOp(SSpace)));
                };
              | OnDelim(_, _)
              | OnOp(_) => Some(ConstructEdit(SOp(SSpace)))
              }

            | ApPalette(_, _, _, _) =>
              failwith("ApPalette is not implemented")
            | _ => Some(ConstructEdit(SOp(SSpace)))
            }
          | _ => Some(ConstructEdit(SOp(SSpace)))
          }
        }

      | SApPalette(_) => failwith("ApPalette is not implemented")
      }
    | MoveTo(_)
    | MoveToBefore(_)
    | MoveLeft
    | MoveRight
    | MoveToNextHole
    | MoveToPrevHole
    | SwapUp /* what's that */
    | SwapDown
    | SwapLeft
    | SwapRight
    | Init => None
    | UpdateApPalette(_) =>
      failwith("ApPalette is not implemented in undo_history")
    };
  };
let get_cursor_info =
    (
      ~new_cardstacks_after: Cardstacks.t,
      ~new_cardstacks_before: Cardstacks.t,
    )
    : cursor_term_info => {
  let zexp_before =
    new_cardstacks_before |> Cardstacks.get_program |> Program.get_zexp;
  let (prev_is_empty_line, next_is_empty_line) =
    CursorInfo.adjacent_is_emptyline(zexp_before);
  let cursor_info_before =
    new_cardstacks_before |> Cardstacks.get_program |> Program.get_cursor_info;
  let cursor_term_before = cursor_info_before.cursor_term;
  let zexp_after =
    new_cardstacks_after |> Cardstacks.get_program |> Program.get_zexp;
  let cursor_info_after =
    new_cardstacks_after |> Cardstacks.get_program |> Program.get_cursor_info;
  let cursor_term_after = cursor_info_after.cursor_term;

  {
    cursor_term_before,
    cursor_term_after,
    zexp_before,
    zexp_after,
    prev_is_empty_line,
    next_is_empty_line,
  };
};

let push_edit_state =
    (
      undo_history: t,
      new_cardstacks_before: Cardstacks.t,
      new_cardstacks_after: Cardstacks.t,
      action: Action.t,
    )
    : t => {
  let prev_group = ZList.prj_z(undo_history.groups);
  let new_cursor_term_info =
    get_cursor_info(~new_cardstacks_before, ~new_cardstacks_after);
  let new_edit_action =
    get_new_edit_action(
      ~prev_group,
      ~new_cardstacks_before,
      ~new_cursor_term_info,
      ~action,
    );
  let timestamp = Unix.time();
  switch (new_edit_action) {
  | None =>
    let prev_entry = ZList.prj_z(prev_group.group_entries);
    let new_entry = {
      ...prev_entry,
      cardstacks_after_move: new_cardstacks_after,
    };

    let new_group = {
      ...prev_group,
      group_entries: ZList.replace_z(new_entry, prev_group.group_entries),
    };
    {
      ...undo_history,
      groups: ZList.replace_z(new_group, undo_history.groups),
      disable_auto_scrolling: false,
    };
  | Some(new_edit_action) =>
    let new_entry = {
      cardstacks_after_action: new_cardstacks_after,
      cardstacks_after_move: new_cardstacks_after,
      cursor_term_info: new_cursor_term_info,
      previous_action: action,
      edit_action: new_edit_action,
      timestamp,
    };
    if (group_entry(~prev_group, ~new_cardstacks_before, ~new_edit_action)) {
      let new_group =
        push_history_entry(
          ~prev_group,
          ~new_entry,
          ~is_expanded=undo_history.all_hidden_history_expand,
        );
      {
        ...undo_history,
        groups: ([], new_group, ZList.prj_suffix(undo_history.groups)),
        disable_auto_scrolling: false,
        hover_recover_group_id: 0,
        hover_recover_elt_id: 0,
        cur_group_id: 0,
        cur_elt_id: 0,
      };
    } else {
      let new_group = {
        group_entries: ([], new_entry, []),
        is_expanded: undo_history.all_hidden_history_expand,
      };
      {
        ...undo_history,
        groups: (
          [],
          new_group,
          [prev_group, ...ZList.prj_suffix(undo_history.groups)],
        ),
        disable_auto_scrolling: false,
        hover_recover_group_id: 0,
        hover_recover_elt_id: 0,
        cur_group_id: 0,
        cur_elt_id: 0,
      };
    };
  };
};

let update_groups =
    (
      new_groups: ZList.t(undo_history_group, undo_history_group),
      undo_history: t,
    )
    : t => {
  {
    ...undo_history,
    groups: new_groups,
    hover_recover_group_id: List.length(ZList.prj_prefix(new_groups)),
    hover_recover_elt_id:
      List.length(ZList.prj_prefix(ZList.prj_z(new_groups).group_entries)),
    cur_group_id: List.length(ZList.prj_prefix(new_groups)),
    cur_elt_id:
      List.length(ZList.prj_prefix(ZList.prj_z(new_groups).group_entries)),
  };
};
let shift_to_prev = (history: t): t => {
  let cur_group = ZList.prj_z(history.groups);
  /* shift to the previous state in the same group */
  switch (ZList.shift_next(cur_group.group_entries)) {
  | None =>
    /* if current group doesn't have previous state, shfit to the previous group */
    switch (ZList.shift_next(history.groups)) {
    | None => history
    | Some(new_groups) =>
      let new_group = ZList.prj_z(new_groups);
      let new_entries = ZList.shift_begin(new_group.group_entries);

      let new_group' = {
        /* is_expanded=true because the selected group should be expanded */
        group_entries: new_entries,
        is_expanded: true,
      };
      let new_groups = ZList.replace_z(new_group', new_groups);
      update_groups(new_groups, history);
    }
  | Some(new_entries) =>
    let new_group = {
      /* is_expanded=true because the selected group should be expanded */
      group_entries: new_entries,
      is_expanded: true,
    };
    let new_groups = ZList.replace_z(new_group, history.groups);
    update_groups(new_groups, history);
  };
};

let shift_to_next = (history: t): t => {
  let cur_group = ZList.prj_z(history.groups);
  /* shift to the previous state in the same group */
  switch (ZList.shift_prev(cur_group.group_entries)) {
  | None =>
    /* if current group doesn't have previous state, shfit to the previous group */
    switch (ZList.shift_prev(history.groups)) {
    | None => history
    | Some(new_groups) =>
      let new_group = ZList.prj_z(new_groups);
      let new_entries = ZList.shift_end(new_group.group_entries);
      let new_group' = {
        /* is_expanded=true because the selected group should be expanded */
        group_entries: new_entries,
        is_expanded: true,
      };
      let new_groups = ZList.replace_z(new_group', new_groups);
      update_groups(new_groups, history);
    }
  | Some(new_entries) =>
    let new_group = {
      /* is_expanded=true because the selected group should be expanded */
      group_entries: new_entries,
      is_expanded: true,
    };
    let new_groups = ZList.replace_z(new_group, history.groups);
    update_groups(new_groups, history);
  };
};

let shift_history =
    (group_id: int, elt_id: int, is_mouseenter: bool, undo_history: t): t => {
  switch (ZList.shift_to(group_id, undo_history.groups)) {
  | None => failwith("Impossible match, because undo_history is non-empty")
  | Some(new_groups) =>
    let cur_group = ZList.prj_z(new_groups);
    /* shift to the element with elt_id */
    switch (ZList.shift_to(elt_id, cur_group.group_entries)) {
    | None => failwith("Impossible because group_entries is non-empty")
    | Some(new_group_entries) =>
      let (cur_group_id, cur_elt_id) = (group_id, elt_id);
      let (hover_recover_group_id, hover_recover_elt_id) =
        if (is_mouseenter) {
          (
            undo_history.hover_recover_group_id,
            undo_history.hover_recover_elt_id,
          );
        } else {
          (group_id, elt_id);
        };
      {
        ...undo_history,
        groups:
          ZList.replace_z(
            {...cur_group, group_entries: new_group_entries},
            new_groups,
          ),
        disable_auto_scrolling: true,
        hover_recover_group_id,
        hover_recover_elt_id,
        cur_group_id,
        cur_elt_id,
      };
    };
  };
};

let toggle_all_hidden_history = (undo_history: t): t => {
  let hidden_group = (group: undo_history_group) => {
    ...group,
    is_expanded: !undo_history.all_hidden_history_expand,
  };
  {
    ...undo_history,
    groups: (
      List.map(hidden_group, ZList.prj_prefix(undo_history.groups)),
      hidden_group(ZList.prj_z(undo_history.groups)),
      List.map(hidden_group, ZList.prj_suffix(undo_history.groups)),
    ),
    all_hidden_history_expand: !undo_history.all_hidden_history_expand,
  };
};
