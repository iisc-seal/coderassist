tree grammar Rewrite;
options {
    tokenVocab=Output;      
    ASTLabelType=CommonTree; // we're using CommonTree nodes
    output=AST;              // build ASTs from input AST
    filter=true;             // tree pattern matching mode
	backtrack=true;          // allow backtracking if necessary
}

@header {
  package feedback.output.parser;
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
topdown : rewriteDivGET
        | rewriteDivGETParen
        | rewriteDivGT
        | rewriteDivGTParen
        | rewriteDivLET
        | rewriteDivLETParen
        | rewriteDivLT
        | rewriteDivLTParen
        ;

rewriteDivGET:
    ^('>=' ^('/' (exp1+=.) (exp2+=.)) Number{$Number.text.compareTo("0")==0}?)
     ->
     ^('&&' ^('>=' $exp1 Number["0"]) ^('>=' $exp2 Number["0"]))    
;

rewriteDivGETParen:
    ^('>=' ^(LOOKUP ^('/' (exp1+=.) (exp2+=.))) Number{$Number.text.compareTo("0")==0}?)
     ->
     ^('&&' ^('>=' $exp1 Number["0"]) ^('>=' $exp2 Number["0"]))
     ;
      
rewriteDivGT:
    ^('>' ^('/' (exp1+=.) (exp2+=.)) Number{$Number.text.compareTo("0")==0}?)
     ->
     ^('&&' ^('>=' $exp1 Number["0"]) ^('>=' $exp2 Number["0"]))    
;

rewriteDivGTParen:
    ^('>' ^(LOOKUP ^('/' (exp1+=.) (exp2+=.))) Number{$Number.text.compareTo("0")==0}?)
     ->
     ^('&&' ^('>=' $exp1 Number["0"]) ^('>=' $exp2 Number["0"]))
     ;     
     
rewriteDivLET:
    ^('<=' ^('/' (exp1+=.) (exp2+=.)) Number{$Number.text.compareTo("0")==0}?)
     ->
     ^('&&' ^('<=' $exp1 Number["0"]) ^('>' $exp2 Number["0"]))    
;

rewriteDivLETParen:
    ^('<=' ^(LOOKUP ^('/' (exp1+=.) (exp2+=.))) Number{$Number.text.compareTo("0")==0}?)
     ->
     ^('&&' ^('<=' $exp1 Number["0"]) ^('>' $exp2 Number["0"]))
     ;
     
rewriteDivLT:
    ^('<' ^('/' (exp1+=.) (exp2+=.)) Number{$Number.text.compareTo("0")==0}?)
     ->
     ^('&&' ^('<' $exp1 Number["0"]) ^('>' $exp2 Number["0"]))    
;

rewriteDivLTParen:
    ^('<' ^(LOOKUP ^('/' (exp1+=.) (exp2+=.))) Number{$Number.text.compareTo("0")==0}?)
     ->
     ^('&&' ^('<' $exp1 Number["0"]) ^('>' $exp2 Number["0"]))
     ;          