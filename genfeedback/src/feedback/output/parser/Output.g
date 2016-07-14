// Code inspired by the design of Bart Keirs
// Source: http://bkiers.blogspot.in/2011/03/5-building-ast.html

grammar Output;

options {
  language = Java;
  output=AST;
  ASTLabelType=CommonTree;
}

tokens {
  BLOCK;
  GUARD;
  CONSTANT;
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
  CAST;
}

@parser::header {
  package feedback.output.parser;
}

@lexer::header {
  package feedback.output.parser;
}

primary
  :  Output statement* EOF -> ^(BLOCK statement*) 
  ;

statement
  :  guard expression newline? ';' -> ^(STATEMENT guard expression newline?)
  ;

newline
    : '[N]'
    ;
    
guard
  :  expression ':' -> ^(GUARD expression)
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
  : (OParen predefined_type) => cast_expression 
  | '-' atom -> ^(UNARY_MIN atom)
  |  '!' atom -> ^(NEGATE atom)
  |  atom
  ;

// The sequence of tokens is correct grammar for a type, and the token immediately
// following the closing parentheses is the token OParen,
// an IDENTIFIER, a number.
scan_for_cast_generic_precedence
: OParen predefined_type CParen cast_disambiguation_token
;

// One of these tokens must follow a valid cast in an expression, in order to
// eliminate a grammar ambiguity.
cast_disambiguation_token
	: OParen 
	| Id 
	| Number
	;

cast_expression
    : OParen predefined_type CParen unaryExpr  -> ^(CAST predefined_type unaryExpr)
    ;

predefined_type :
	| 'byte'
	| 'char'
	| 'double'
	| 'float'
	| 'int'
	| 'long'
	| 'sbyte'
	| 'short'
	| 'uint'
	| 'ulong'
	| 'ushort'
;

atom
  :  Number
  |  Bool
  |  Null
  | functionCall
  |  lookup
  ;

//list
//  :  '[' exprList? ']' -> ^(LIST exprList?)
//  ;

lookup
//:  functionCall indexes?       -> ^(LOOKUP functionCall indexes?) // we disallow f()[][] for now
//|  list indexes?               -> ^(LOOKUP list indexes?)
  :  Id indexes?         -> ^(LOOKUP Id indexes?)
//  | '(' expression ')'   -> ^(LOOKUP expression)
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
Output   : 'OUTPUT:';
Loop     : 'loop'; 
Range    : '$'; 


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

