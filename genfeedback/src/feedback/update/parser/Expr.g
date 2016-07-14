grammar Expr;

options {
  language = Java;
  output = AST;
  ASTLabelType=CommonTree;
}

tokens {
  BLOCK;
  GUARD;
  CONSTANT;
  LOOP;
  INDEX;
  LB;
  UB;
  BODY;
  LHS;
  RHS;
  STATEMENT;
  ASSIGNMENT;
  FUNC_CALL;
  EXP;
  EXP_LIST;
  ID_LIST;
  IF;
  TERNARY;
  UNARY_MIN;
  NEGATE;
  FUNCTION;
  INDEXES;
  LIST;
  LOOKUP;
}

@header {
  package feedback.update.parser;
}

@lexer::header {
  package feedback.update.parser;
}

@lexer::members {
    @Override
    public void reportError(RecognitionException e) {
        throw new RuntimeException(e);
    }
}    

startrule 
    : expression
    ;
    

expression
  :  condExpr
  ;

functionCall
  :  Id '(' exprList? ')' -> ^(FUNC_CALL Id exprList?)
  ;

exprList
  :  expression (',' expression)* -> ^(EXP_LIST expression+)
  ;



condExpr
  :  (orExpr -> orExpr) 
     ( '?' a=expression ':' b=expression -> ^(TERNARY orExpr $a $b)
//   | In expression                     -> ^(In orExpr expression)
     )?
  ;

orExpr
  :  andExpr ('||'^ andExpr)*
  ;

andExpr
  :  equExpr ('&&'^ equExpr)*
  ;

equExpr
  :  relExpr (('==' | '!=')^ relExpr)*
  ;

relExpr
  :  rangeExpr (('>=' | '<=' | '>' | '<')^ rangeExpr)*
  ;

rangeExpr
  :  addExpr ('...'^ addExpr)*
  ;
  
addExpr
  :  mulExpr (('+' | '-')^ mulExpr)*
  ;

mulExpr
  :  powExpr (('*' | '/' | '%')^ powExpr)*
  ;

powExpr
  :  unaryExpr ('^'^ unaryExpr)*
  ;
  
unaryExpr
  :  '-' atom -> ^(UNARY_MIN atom)
  |  '!' atom -> ^(NEGATE atom)
  |  atom
  ;

atom
  :  Number
  |  Bool
  |  Null
  | functionCall//  |  range
  |  lookup
  ;

//list
//  :  '[' exprList? ']' -> ^(LIST exprList?)
//  ;

lookup
//:  functionCall indexes?       -> ^(LOOKUP functionCall indexes?) // we disallow f()[][] for now
//|  list indexes?               -> ^(LOOKUP list indexes?)
  :  Id indexes?         -> ^(LOOKUP Id indexes?)
  |  String indexes?             -> ^(LOOKUP String indexes?)
  |  '(' expression ')' indexes? -> ^(LOOKUP expression indexes?) // we disallow ( (expr)[][] for now when expr is not ID)
  ;

indexes
  :  ('[' expression ']')+ -> ^(INDEXES expression+)
  ;

//Println  : 'println';
//Print    : 'print';
//Assert   : 'assert';
//Size     : 'size';
//Def      : 'def';
//If       : 'if';
//Else     : 'else';
//Return   : 'return';
//For      : 'for';
//While    : 'while';
//To       : 'to';
//Do       : 'do';
//End      : 'end';
//In       : 'in';
Null     : 'null';

Or       : '||';
And      : '&&';
Equals   : '==';
NEquals  : '!=';
GTEquals : '>=';
LTEquals : '<=';
Pow      : '^';
Excl     : '!';
GT       : '>';
LT       : '<';
Add      : '+';
Subtract : '-';
Multiply : '*';
Divide   : '/';
Modulus  : '%';
OBrace   : '{';
CBrace   : '}';
OBracket : '[';
CBracket : ']';
OParen   : '(';
CParen   : ')';
SColon   : ';';
Assign   : '=';
Comma    : ',';
QMark    : '?';
Colon    : ':';
Update   : 'UPDATE:';
Loop     : 'loop'; 
Range    : '...'; 

Bool
  :  'true' 
  |  'false'
  ;

Number
  :  Int ('.' Digit*)?
  ;

Id
  :  ('a'..'z' | 'A'..'Z' | '_') ('a'..'z' | 'A'..'Z' | '_' | Digit)*
  ;

String
@after {
  setText(getText().substring(1, getText().length()-1).replaceAll("\\\\(.)", "$1"));
}
  :  '"'  (~('"' | '\\')  | '\\' ('\\' | '"'))* '"' 
  |  '\'' (~('\'' | '\\') | '\\' ('\\' | '\''))* '\''
  ;

Comment
  :  '//' ~('\r' | '\n')* {skip();}
  |  '/*' .* '*/'         {skip();}
  ;

Space
  :  (' ' | '\t' | '\r' | '\n' | '\u000C') {$channel=HIDDEN;}
  ;

fragment Int
  :  '1'..'9' Digit*
  |  '0'
  ;
  
fragment Digit 
  :  '0'..'9'
  ;

