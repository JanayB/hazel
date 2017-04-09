(* generates divs from terms. styled exclusively using CSS (www/style.css) *)
module HTMLView = struct
  module Html = Tyxml_js.Html
  open Html
  open Hazel_semantics

  let hzdiv cls contents =  Html.(div ~a:[a_class ["HZElem";cls]] contents)

  let string_of_side side = match side with 
    | HExp.L -> "L"
    | HExp.R -> "R"

  let rec of_htype (htype : HTyp.t ) : [> Html_types.div ] Html.elt  =
    match htype with
    | HTyp.Num -> hzdiv "Num" []
    | HTyp.Arrow (fst,snd) -> hzdiv "Arrow" [
        hzdiv "leftParens" []; 
        hzdiv "arrowLeft" [of_htype (fst)]; 
        hzdiv "arrow" []; 
        hzdiv "arrowRight" [of_htype (snd)];
        hzdiv "rightParens" []]
    | HTyp.Hole ->  hzdiv "Hole" []
    | HTyp.Sum (fst,snd) -> hzdiv "Sum" [
        hzdiv "leftParens" []; 
        hzdiv "sumLeft" [of_htype (fst)]; 
        hzdiv "plusSign" []; 
        hzdiv "sumRight" [of_htype (snd)]; 
        hzdiv "rightParens" []]

  let rec of_hexp (hexp : HExp.t ) : [> Html_types.div ] Tyxml_js.Html.elt  =
    match hexp with
    | HExp.Let (x, e1, e2) -> hzdiv "Let" [
        hzdiv "leftParens" [];
        hzdiv "letKeyword" []; 
        hzdiv "varExp" [pcdata x];
        hzdiv "isChar" [];
        hzdiv "letRhsExp" [of_hexp e1];
        hzdiv "inChar" []; 
        hzdiv "letBodyExp" [of_hexp e2];
        hzdiv "rightParens" []]
    | HExp.Lam (var,exp) -> hzdiv "Lam" [
        hzdiv "leftParens" []; 
        hzdiv "lambda" []; 
        hzdiv "lambdaVar" [pcdata var]; 
        hzdiv "dot" [];
        hzdiv "lambdaBody" [of_hexp exp];
        hzdiv "rightParens" []]
    | HExp.Asc (hexp,htype) -> hzdiv "Asc" [
        hzdiv "ascExp" [of_hexp hexp]; 
        hzdiv "ascChar" []; 
        hzdiv "ascType" [of_htype htype]]
    | HExp.Var var -> hzdiv "Var" [pcdata var]
    | HExp.Ap (e1, e2) -> hzdiv "Ap" [
        hzdiv "apLeft" [of_hexp e1]; 
        hzdiv "leftParens" []; 
        hzdiv "apRight" [of_hexp e2]; 
        hzdiv "rightParens" []]
    | HExp.NumLit num -> hzdiv "NumLit" [
        pcdata (string_of_int num)]
    | HExp.Plus (n1,n2) -> hzdiv "Plus" [
        hzdiv "leftParens" [];
        hzdiv "plusLeft" [of_hexp n1]; 
        hzdiv "plusSign" [];
        hzdiv "plusRight" [of_hexp n2]; 
        hzdiv "rightParens" []]
    | HExp.EmptyHole ->  hzdiv "EmptyHole" []
    | HExp.NonEmptyHole hc -> hzdiv "NonEmptyHole" [
        hzdiv "lNE" [];
        hzdiv "bNE" [of_hexp hc]; 
        hzdiv "rNE" []]
    | HExp.Inj (side,exp) -> hzdiv "Inj" [
        hzdiv ("i<t_%9>nj" ^ string_of_side side) []; 
        hzdiv "leftParens" []; 
        hzdiv "injBody" [of_hexp exp]; 
        hzdiv "rightParens" []]
    | HExp.Case (e,(var1,exp1),(var2,exp2)) -> hzdiv "Case" [
        hzdiv "case" [];
        hzdiv "leftParens" [];
        hzdiv "caseScrutinee" [of_hexp e];
        hzdiv "comma" [];
        hzdiv "var1" [pcdata var1];
        hzdiv "dot" [];
        hzdiv "caseExp1" [of_hexp exp1];
        hzdiv "comma" [];
        hzdiv "var2" [pcdata var2];
        hzdiv "dot" [];
        hzdiv "caseExp2" [of_hexp exp2];
        hzdiv "rightParens" []
      ]

  let rec of_ztype (ztype : ZTyp.t ) : [> Html_types.div ] Tyxml_js.Html.elt  =
    match ztype with
    | ZTyp.CursorT htype -> hzdiv "CursorT" [
        hzdiv "lCursor" []; 
        hzdiv "bCursor" [of_htype htype];
        hzdiv "rCursor" []]
    | ZTyp.LeftArrow  (ztype, htype) -> hzdiv "LeftArrow"  [
        hzdiv "leftParens" [];
        hzdiv "arrowLeft" [of_ztype ztype]; 
        hzdiv "arrow" []; 
        hzdiv "arrowRight" [of_htype htype]; 
        hzdiv "rightParens" []]
    | ZTyp.RightArrow (htype, ztype) -> hzdiv "RightArrow" [
        hzdiv "leftParens" [];
        hzdiv "arrowLeft" [of_htype htype]; 
        hzdiv "arrow" [];
        hzdiv "arrowRight" [of_ztype ztype]; 
        hzdiv "rightParens" []]
    | ZTyp.LeftSum (ztype, htype) -> hzdiv "LeftSum"  [
        hzdiv "leftParens" [];
        hzdiv "sumLeft" [of_ztype ztype]; 
        hzdiv "plusSign" []; 
        hzdiv "sumRight" [of_htype htype]; 
        hzdiv "rightParens" []]
    | ZTyp.RightSum (htype,ztype) -> hzdiv "RightSum" [
        hzdiv "leftParens" [];
        hzdiv "sumLeft" [of_htype htype]; 
        hzdiv "plusSign" []; 
        hzdiv "sumRight" [of_ztype ztype]; 
        hzdiv "rightParens" []]

  let rec of_zexp (zexp : ZExp.t ) :  [> Html_types.div ] Tyxml_js.Html.elt  =
    match zexp with
    | ZExp.RightAsc (e, asc) -> hzdiv "RightAsc" [
        hzdiv "ascExp" [of_hexp e]; 
        hzdiv "ascChar" []; 
        hzdiv "ascType" [of_ztype asc]]
    | ZExp.LeftAsc (e, asc) -> hzdiv "LeftAsc" [
        hzdiv "ascExp" [of_zexp e]; 
        hzdiv "ascChar" []; 
        hzdiv "ascType" [of_htype asc]]
    | ZExp.CursorE hexp -> hzdiv "CursorE" [
        hzdiv "lCursor" []; 
        hzdiv "bCursor" [of_hexp hexp]; 
        hzdiv "rCursor" []]
    | ZExp.LetZ1 (x, ze1, e2) -> hzdiv "LetZ1" [ 
        hzdiv "leftParens" [];
        hzdiv "letKeyword" []; 
        hzdiv "varExp" [pcdata x];
        hzdiv "isChar" [];
        hzdiv "letRhsExp" [of_zexp ze1];
        hzdiv "inChar" []; 
        hzdiv "letBodyExp" [of_hexp e2];
        hzdiv "rightParens" []]
    | ZExp.LetZ2 (x, e1, ze2) -> hzdiv "LetZ2" [
        hzdiv "leftParens" [];
        hzdiv "letKeyword" []; 
        hzdiv "varExp" [pcdata x];
        hzdiv "isChar" [];
        hzdiv "letRhsExp" [of_hexp e1];
        hzdiv "inChar" []; 
        hzdiv "letBodyExp" [of_zexp ze2];
        hzdiv "rightParens" []]
    | ZExp.LamZ (var,exp) -> hzdiv "LamZ" [
        hzdiv "leftParens" [];
        hzdiv "lambda" [];
        hzdiv "lambdaVar" [pcdata var];
        hzdiv "dot" [];
        hzdiv "lambdaBody" [of_zexp exp];
        hzdiv "rightParens" []]
    | ZExp.LeftAp (e1,e2) -> hzdiv "LeftAp" [
        hzdiv "apLeft" [of_zexp e1]; 
        hzdiv "leftParens" []; 
        hzdiv "apRight" [of_hexp e2];
        hzdiv "rightParens" []]
    | ZExp.RightAp (e1,e2) -> hzdiv "RightAp" [
        hzdiv "apLeft" [of_hexp e1]; 
        hzdiv "leftParens" []; 
        hzdiv "apRight" [of_zexp e2];
        hzdiv "rightParens" []]
    | ZExp.LeftPlus (e1,e2) -> hzdiv "LeftPlus" [
        hzdiv "leftParens" [];
        hzdiv "plusLeft" [of_zexp e1];
        hzdiv "plusSign" [];
        hzdiv "plusRight" [of_hexp e2];
        hzdiv "rightParens" []]
    | ZExp.RightPlus (e1,e2) -> hzdiv "RightPlus" [
        hzdiv "leftParens" [];
        hzdiv "plusLeft" [of_hexp e1];
        hzdiv "plusSign" [];
        hzdiv "plusRight" [of_zexp e2];
        hzdiv "rightParens" []]
    | ZExp.NonEmptyHoleZ e -> hzdiv "NonEmptyHoleZ" [
        hzdiv "lNE" []; 
        hzdiv "bNE" [of_zexp e]; 
        hzdiv "rNE" []]
    | ZExp.InjZ (side,exp) -> hzdiv "Inj" [
        hzdiv ("inj" ^ string_of_side side) []; 
        hzdiv "leftParens" []; 
        hzdiv "injBody" [of_zexp exp]; 
        hzdiv "rightParens" []]
    | ZExp.CaseZ1 (e,(var1,exp1) ,(var2,exp2)) -> hzdiv "Case" [
        hzdiv "case" []; 
        hzdiv "leftParens" []; 
        hzdiv "caseScrutinee" [of_zexp e];
        hzdiv "comma" []; 
        hzdiv "var1" [pcdata var1];
        hzdiv "dot" []; 
        hzdiv "caseExp1" [of_hexp exp1];
        hzdiv "comma" []; 
        hzdiv "var2" [pcdata var2];
        hzdiv "dot" []; 
        hzdiv "caseExp2" [of_hexp exp2];
        hzdiv "rightParens" []
      ]
    | ZExp.CaseZ2 (e,(var1,exp1) ,(var2,exp2)) -> hzdiv "Case" [
        hzdiv "case" []; 
        hzdiv "leftParens" []; 
        hzdiv "caseScrutinee" [of_hexp e];
        hzdiv "comma" []; 
        hzdiv "var1" [pcdata var1];
        hzdiv "dot" []; 
        hzdiv "caseExp1" [of_zexp exp1];
        hzdiv "comma" []; 
        hzdiv "var2" [pcdata var2];
        hzdiv "dot" []; 
        hzdiv "caseExp2" [of_hexp exp2];
        hzdiv "rightParens" []
      ]
    | ZExp.CaseZ3 (e,(var1,exp1) ,(var2,exp2)) -> hzdiv "Case" [
        hzdiv "case" []; 
        hzdiv "leftParens" []; 
        hzdiv "caseScrutinee" [of_hexp e];
        hzdiv "comma" []; 
        hzdiv "var1" [pcdata var1];
        hzdiv "dot" []; 
        hzdiv "caseExp1" [of_hexp exp1];
        hzdiv "comma" []; 
        hzdiv "var2" [pcdata var2];
        hzdiv "dot" []; 
        hzdiv "caseExp2" [of_zexp exp2];
        hzdiv "rightParens" []
      ]
end


module PPView = struct
  module PP = Pretty.PP
  open Hazel_semantics
  let (^^) = PP.(^^)

  let rec concat x1 x2 xs = match xs with 
    | [] -> x1 ^^ x2
    | [x3] -> x1 ^^ x2 ^^ x3
    | x3 :: x4 :: xs' -> x1 ^^ x2 ^^ (concat x3 x4 xs')


  let taggedText tag s = 
    PP.tagged tag (PP.text s)
  let kw = taggedText "kw"
  let parens = taggedText "paren" 
  let op = taggedText "op"
  let var = taggedText "var" 
  let space = taggedText "space" " "
  let term tag doc = 
    let tag' = "new-" ^ tag in 
    PP.tagged tag' doc

  let rec of_htype tau = match tau with 
    | HTyp.Num -> 
      term "Num" (kw "num")
    | HTyp.Arrow (tau1, tau2) -> 
      term "Arrow" (
        (parens "(") ^^ (PP.nestRelative 0 
                           (of_htype tau1) ^^ 
                         (op " ->") ^^
                         PP.optionalBreak ^^
                         (of_htype tau2) ^^ 
                         (parens ")")
                        ))
    | HTyp.Sum (tau1, tau2) -> 
      term "Sum" (
        (parens "(") ^^ (PP.nestRelative 0 
                           (of_htype tau1) ^^ 
                         (op " +") ^^
                         PP.optionalBreak ^^
                         (of_htype tau2) ^^ 
                         (parens ")")))
    | HTyp.Hole -> 
      term "Hole" (taggedText "hole" "⦇⦈")

  let string_of_side side = match side with 
    | HExp.L -> "L"
    | HExp.R -> "R"

  let rec of_hexp e = match e with 
    | HExp.Asc (e', tau) -> 
      term "Asc" (PP.nestAbsolute 2 (
          (parens "(" ^^ (PP.nestRelative 0 (
               (of_hexp e') ^^
               (op " :") ^^
               PP.optionalBreak ^^ 
               (of_htype tau) ^^ 
               (parens ")"))))))
    | HExp.Var x -> term "Var" (var x)
    | HExp.Let (x, e, e') -> 
      term "Let" (
        (PP.blockBoundary) ^^ 
        (kw "let") ^^ space ^^ 
        (var x) ^^ space ^^ 
        (op "=") ^^ (PP.nestAbsolute 2 (
            PP.optionalBreak ^^ 
            (of_hexp e))) ^^ 
        (PP.mandatoryBreak) ^^ 
        (of_hexp e'))
    | HExp.Lam (x, e) -> 
      term "Lam" (
        (kw "λ") ^^ 
        (var x) ^^ 
        (kw ".") ^^ 
        (PP.nestRelative 4 (
            of_hexp e)))
    | HExp.Ap (e1, e2) -> 
      term "Ap" (
        (of_hexp e1) ^^ PP.optionalBreak ^^ (of_hexp e2))
    | HExp.NumLit n -> 
      term "NumLit" (
        taggedText "number" (string_of_int n))
    | HExp.Plus (e1, e2) -> 
      term "Plus" (
        (of_hexp e1) ^^ PP.optionalBreak ^^ 
        (op "+") ^^ PP.optionalBreak ^^ (of_hexp e2)) 
    | HExp.Inj (side, e) -> 
      term "Inj" (
        (kw "inj") ^^ (parens "[") ^^ 
        (kw (string_of_side side)) ^^ (parens "]") ^^ 
        (parens "(") ^^ PP.optionalBreak ^^ (of_hexp e) ^^ (parens ")"))
    | HExp.Case (e1, (x, e2), (y, e3)) -> 
      term "Case" (
        (kw "case") ^^ space ^^ 
        (of_hexp e1) ^^ space ^^ 
        (kw "of") ^^ (PP.nestAbsolute 2 (
            PP.mandatoryBreak ^^ 
            (kw "L") ^^ (parens "(") ^^ (var x) ^^ (parens ")") ^^ 
            space ^^ (op "=>") ^^ space ^^ (PP.nestAbsolute 2 (
                of_hexp e2)) ^^ 
            PP.mandatoryBreak ^^ 
            (kw "R") ^^ (parens "(") ^^ (var y) ^^ (parens ")") ^^ 
            space ^^ (op "=>") ^^ space ^^ (PP.nestAbsolute 2 (
                of_hexp e3)))))
    | HExp.EmptyHole -> 
      term "EmptyHole" (taggedText "hole" "⦇⦈")
    | HExp.NonEmptyHole e -> 
      term "NonEmptyHole" (
        (taggedText "hole" "⦇") ^^ (of_hexp e) ^^ 
        (taggedText "hole" "⦈")) 
end

