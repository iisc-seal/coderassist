tree grammar Rewrite;
options {
    tokenVocab=Update;      
    ASTLabelType=CommonTree; // we're using CommonTree nodes
    output=AST;              // build ASTs from input AST
    filter=true;             // tree pattern matching mode
	backtrack=true;          // allow backtracking if necessary
}

@header {
  package feedback.update.parser;
}

@members {
  /*Check whether two tokens are equal*/  
  public boolean equals(CommonTree t1, CommonTree t2) {
    //System.out.println(t1.getText());
    //System.out.println(t2.getText());
    return t1.getText().compareTo(t2.getText()) == 0;
  }
  
}

// START: strategy
topdown : scrunchedAssignment 
//        | rewriteToIndex
      ; // tell ANTLR when to attempt which rule
// END: strategy

//Something both before and after
// rewrite (i==0 || j==0):a[i][j] = 0;

//(STATEMENT 
//    (LOOP (INDEX (LOOKUP i)) (LB 1) (UB (- (LOOKUP dp_ProgInput_2) 1)) 
//        (BODY (GUARD (LOOKUP true) (ASSIGNMENT x (LOOKUP y))))
//    )
//    )


scrunchedAssignment:
    ^(STATEMENT
        ((before+=.)+)? 
        ^(LOOP ^(DIR (dir+=.)+) ^(INDEX ^(LOOKUP index=Id)) ^(LB (lb+=.)+) ^(UB (ub+=.)+) ^(LINE (line+=.)+)
           ^(BODY 
            ^(GUARD ^(LOOKUP s1b1=Bool{"true".compareTo($s1b1.text)==0}?) ^(ASSIGNMENT ^(LHS (s1lhs+=.)+) ^(RHS (s1rhs+=.)+) ))
            ^(GUARD ^(LOOKUP s2b1=Bool{"true".compareTo($s2b1.text)==0}?) ^(ASSIGNMENT ^(LHS (s2lhs+=.)+) ^(RHS (s2rhs+=.)+) )) 
//            ^(GUARD ^(LOOKUP b2=Bool) ^(ASSIGNMENT (a2+=.)+))
            )
          )
        ((after+=.)+)?    
     )
     ->
     ^(STATEMENT 
        $before?
        ^(LOOP ^(DIR $dir)+ ^(INDEX ^(LOOKUP $index)) ^(LB $lb)+ ^(UB $ub)+ ^(LINE $line)+
            ^(BODY 
                ^(GUARD ^(LOOKUP $s1b1) ^(ASSIGNMENT ^(LHS $s1lhs+) ^(RHS $s1rhs+)))
            )
         )
        ^(LOOP ^(DIR $dir)+ ^(INDEX ^(LOOKUP $index)+) ^(LB $lb)+ ^(UB $ub)+ ^(LINE $line)+
            ^(BODY 
                ^(GUARD ^(LOOKUP $s2b1) ^(ASSIGNMENT ^(LHS $s2lhs+) ^(RHS $s2rhs+)))
            )
         )
         $after?
      )
;

//duplicateBody:
//    ^(STATEMENT
//        ((before+=.)+)? 
//        ^(LOOP ^(INDEX ^(LOOKUP index=.)) ^(LB lb=.) ^(UB ub=.) ^(BODY (b+=.)+))
//        ((after+=.)+)?    
//     )
//     ->
//     ^(STATEMENT 
//        $before?
//        ^(LOOP ^(INDEX ^(LOOKUP $index)) ^(LB $lb) ^(UB $ub) ^(BODY $b $b))
//        $after?    
//     )
//;

//rewriteToIndex:
//
//           ^(LOOP ^(DIR (oDir+=.)+) ^(INDEX ^(LOOKUP oIndex=Id)) ^(LB (oLb+=.)+) ^(UB (oUb+=.)+)
//             ^(BODY
//               ^(LOOP ^(DIR (iDir+=.)+) ^(INDEX ^(LOOKUP iIndex=Id)) ^(LB (iLb+=.)+) ^(UB (iUb+=.)+)
//                 ^(BODY
//                   ^(GUARD
//                           ^(LOOKUP ^('&&'
//                                   
//                                      ^(LOOKUP ^('==' ^(LOOKUP g1=Id{equals($oIndex, $g1)}?) c1=Number))
//                                      ^(LOOKUP ^('==' ^(LOOKUP g2=Id{equals($iIndex, $g2)}?) c2=Number))
//                                     ))
//                           ^(ASSIGNMENT
//                                ^(LHS lBase=Id ^(INDEXES ^(LOOKUP dim1=Id{equals($oIndex, $dim1)}?) ^(LOOKUP dim2=Id{equals($iIndex, $dim2)}?)))
//                                ^(RHS ^(LOOKUP rBase=Id ^(INDEXES row=Number{equals($row, $c1)}? col=Number{equals($col, $c2)}?))))
//                    )
//                 
//                 ((after+=.)+)?
//                 )
//               )
//             )
//           )
//
//->
//
//           ^(LOOP ^(DIR $oDir)+ ^(INDEX ^(LOOKUP $oIndex)) ^(LB $oLb)+ ^(UB $oUb)+
//             ^(BODY
//               ^(LOOP ^(DIR $iDir)+ ^(INDEX ^(LOOKUP $iIndex)) ^(LB $iLb)+ ^(UB $iUb)+
//                 ^(BODY
//                   ^(GUARD
//                           ^(LOOKUP ^('&&' ^(LOOKUP ^('==' ^(LOOKUP $g1 $c1) ^('==' ^(LOOKUP $g2 $c2))))))
//                           ^(ASSIGNMENT
//                                ^(LHS $lBase ^(INDEXES ^(LOOKUP $dim1) ^(LOOKUP $dim2)))
//                                ^(RHS ^(LOOKUP $rBase ^(INDEXES ^(LOOKUP $dim1) ^(LOOKUP $dim2)))))
//                  )
//                 
//                 $after?
//                  )
//                )
//             )
//           )
//
//;