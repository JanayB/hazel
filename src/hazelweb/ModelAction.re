module EditAction = Action_common;
module Sexp = Sexplib.Sexp;
open Sexplib.Std;

[@deriving sexp]
type move_input =
  | Key(MoveKey.t)
  | Click((CursorMap.Row.t, CursorMap.Col.t));

[@deriving sexp]
type shift_history_info = {
  id: UndoHistory.id,
  call_by_mouseenter: bool,
};
[@deriving sexp]
type t =
  | EditAction(EditAction.t)
  | MoveAction(move_input)
  | ToggleLeftSidebar
  | ToggleRightSidebar
  | LoadExample(Examples.id)
  | LoadCardstack(int)
  | NextCard
  | PrevCard
  // Result computation toggles
  | ToggleComputeResults
  | ToggleShowCaseClauses
  | ToggleShowFnBodies
  | ToggleShowCasts
  | ToggleShowUnevaluatedExpansion
  // Time measurement toggles
  | ToggleMeasureTimes
  | ToggleMeasureModel_perform_edit_action
  | ToggleMeasureProgram_get_doc
  | ToggleMeasureLayoutOfDoc_layout_of_doc
  | ToggleMeasureUHCode_view
  | ToggleMeasureCell_view
  | ToggleMeasurePage_view
  | ToggleMeasureHazel_create
  | ToggleMeasureUpdate_apply_action
  //
  | ToggleMemoizeDoc
  | SelectHoleInstance(HoleInstance.t)
  | SelectCaseBranch(CursorPath_common.steps, int)
  | InvalidVar(string)
  | FocusCell
  | BlurCell
  | Redo
  | Undo
  | ShiftHistory(shift_history_info)
  | ShiftWhenScroll
  | ToggleHistoryGroup(UndoHistory.group_id)
  | ToggleHiddenHistoryAll
  | TogglePreviewOnHover
  | UpdateFontMetrics(FontMetrics.t)
  | UpdateIsMac(bool);
