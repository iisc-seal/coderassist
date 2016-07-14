tree grammar Rewrite;
options {
    tokenVocab=initParse;      // use tokens from VecMath.g
    ASTLabelType=CommonTree; // we're using CommonTree nodes
    output=AST;              // build ASTs from input AST
    filter=true;             // tree pattern matching mode
	backtrack=true;          // allow backtracking if necessary
}

@header {
  package feedback.init.parser;
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
topdown: disjunctiveGuard1
//          | disjunctiveGuard2  
          | innerGuard 
          | outerGuard
          ;

rewriteLoop: ^(LOOP ^(LOOKUP index=Id) Number {$Number.text.compareTo("1")==0}? ^(LOOKUP ^(UB ub=Id)) (s+=.)+) ->
            ^(LOOP ^(LOOKUP $index) Number["0"] ^('-' $ub Number["1"]) $s)
    ;


// Something both before and after
// rewrite (i==0 || j==0):a[i][j] = 0;
disjunctiveGuard1:
^(STATEMENT 
    ((before+=.)+)? 
    ^(LOOP oDir=Number{$oDir.text.compareTo("1")==0}? ^(LOOKUP oIndex=Id) oLb=Number ^(UB (oUb+=.)+) line=Number
      ^(LOOP iDir=Number{$iDir.text.compareTo("1")==0}? ^(LOOKUP iIndex=Id) iLb=Number ^(UB (iUb+=.)+)  line=Number
        ^(GUARD ^('||' 
                  ^('==' ^(LOOKUP g1=Id{equals($oIndex, $g1) || equals($iIndex, $g1)}?) c1=Number) 
                  ^('==' ^(LOOKUP g2=Id{equals($iIndex, $g2) || equals($oIndex, $g2)}?) c2=Number)
                ) 
           ^(ASSIGNMENT base=Id ^(INDEXES ^(LOOKUP dim1=Id) ^(LOOKUP dim2=Id)) constant=Number)
         ){!equals($g1, $g2)}?
       )
     )
  ((after+=.)+)?
)
 ->
      ^(STATEMENT $before?
      ^(GUARD ^(Bool["true"]) ^(ASSIGNMENT $base ^(INDEXES ^('...' $oLb ^(LOOKUP $oUb)) $c2) $constant)) 
      ^(GUARD ^(Bool["true"]) ^(ASSIGNMENT $base ^(INDEXES $c1 ^('...' $iLb ^(LOOKUP $iUb))) $constant))
      $after?
      )
 ;

disjunctiveGuard2:
^(STATEMENT 
    ((before+=.)+)? 
    ^(LOOP oDir=Number{$oDir.text.compareTo("1")==0}? ^(LOOKUP oIndex=Id) oLb=Number ^(UB (oUb+=.)+) line=Number
      ^(LOOP iDir=Number{$iDir.text.compareTo("1")==0}? ^(LOOKUP iIndex=Id) iLb=Number ^(UB (iUb+=.)+) line=Number
        ^(GUARD ^('||' 
                  ^(LOOKUP ^('==' ^(LOOKUP g1=Id{equals($oIndex, $g1) || equals($iIndex, $g1)}?) c1=Number)) 
                  ^(LOOKUP ^('==' ^(LOOKUP g2=Id{equals($iIndex, $g2) || equals($oIndex, $g2)}?) c2=Number))
                ) 
           ^(ASSIGNMENT base=Id ^(INDEXES ^(LOOKUP dim1=Id) ^(LOOKUP dim2=Id)) constant=Number)
         ){!equals($g1, $g2)}?
       )
     )
  ((after+=.)+)?
)
 ->
      ^(STATEMENT $before?
      ^(GUARD ^(Bool["true"]) ^(ASSIGNMENT $base ^(INDEXES ^('...' $oLb ^(LOOKUP $oUb)) $c2) $constant)) 
      ^(GUARD ^(Bool["true"]) ^(ASSIGNMENT $base ^(INDEXES $c1 ^('...' $iLb ^(LOOKUP $iUb))) $constant))
      $after?
      )
 ;


/*i==0:dp_Array_1[i][j] = 0*/
outerGuard:
^(LOOP oDir=Number{$oDir.text.compareTo("1")==0}? ^(LOOKUP oIndex=Id) oLb=Number ^(UB (oUb+=.)+) line=Number
    ^(LOOP iDir=Number{$iDir.text.compareTo("1")==0}? ^(LOOKUP iIndex=Id) iLb=Number ^(UB (iUb+=.)+) line=Number
        ^(GUARD ^('==' ^(LOOKUP g1=Id{equals($oIndex, $g1)}?) c1=Number)               
           ^(ASSIGNMENT base=Id ^(INDEXES ^(LOOKUP dim1=Id) ^(LOOKUP dim2=Id)) constant=Number)
         )
      )
    //(s+=.)+  
  )

 ->
//^(LOOP ^(LOOKUP $oIndex) $oLb ^(LOOKUP $oUb) 
//    ^(LOOP ^(LOOKUP $iIndex) $iLb ^(LOOKUP $iUb)  
        ^(GUARD ^(Bool["true"]) ^(ASSIGNMENT $base ^(INDEXES $c1 ^('...' $iLb ^(LOOKUP $iUb))) $constant))
//      )
  //$s    
// )
 ;
 
 /*j==0:dp_Array_1[i][j] = 0*/
innerGuard:
^(LOOP oDir=Number{$oDir.text.compareTo("1")==0}? ^(LOOKUP oIndex=Id) oLb=Number ^(UB (oUb+=.)+) line=Number 
    ^(LOOP iDir=Number{$iDir.text.compareTo("1")==0}? ^(LOOKUP iIndex=Id) iLb=Number ^(UB (iUb+=.)+) line=Number
        ^(GUARD ^('==' ^(LOOKUP g1=Id{equals($iIndex, $g1)}?) c1=Number)               
           ^(ASSIGNMENT base=Id ^(INDEXES ^(LOOKUP dim1=Id) ^(LOOKUP dim2=Id)) constant=Number)
         )
      )
    //(s+=.)+  
  )

 ->
//^(LOOP ^(LOOKUP $oIndex) $oLb ^(LOOKUP $oUb) 
//    ^(LOOP ^(LOOKUP $iIndex) $iLb ^(LOOKUP $iUb)  
        ^(GUARD ^(Bool["true"]) ^(ASSIGNMENT $base ^(INDEXES ^(Range $oLb ^(LOOKUP $oUb)) $c1) $constant))
//      )
  //$s    
// )
 ;
 