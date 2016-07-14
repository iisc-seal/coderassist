tree grammar OutputTreeWalker;

options {
  tokenVocab=Output;
  ASTLabelType=CommonTree;
}

@header {
  package feedback.output.parser;
  import java.util.Map;
  import java.util.HashMap;
  import java.util.List;
  import java.util.ArrayList;
  import feedback.utils.Expression;
  import feedback.utils.Expression.ExprType;
  import feedback.utils.Statement;
  import feedback.utils.AssignmentStatement;
}

//@members {
//  public Map<String, String> guardToOutput = new HashMap<String, String>();
//  public List<String> statements = new ArrayList<String>();
//}

//walk
//  :  primary
//  ;

primary returns [List<Statement> lst]
  @init{lst = new ArrayList<Statement>(); }
  :   ^(BLOCK (s=statement {lst.add(s);})*) 
  ;

statement returns [Statement as]
  : ^(STATEMENT g=guard e=expression n=newline?) {
            Expression lhs = new Expression(ExprType.ID, "", "out");
            as = new AssignmentStatement(g, lhs, e, n);}
  ;

newline returns [boolean b]
    @init{b = false;}
    : '[N]' {b = true;}  
    ;

guard returns [Expression e]
  : ^(GUARD exp=expression) {e = exp;}
  ;  

functionCall returns [Expression expr]
  :  ^(FUNC_CALL Id eList=exprList?) {expr = new Expression(ExprType.FN, eList, $Id.text);}
  ;

exprList returns [List<Expression> lst]
      @init{lst = new ArrayList<Expression>(); }
  :  ^(EXP_LIST (exp=expression {lst.add(exp);})+)
  ;

expression returns [Expression expr]
  :  ^(TERNARY e1=expression e2=expression e3=expression) {expr = new Expression(ExprType.TERNARY, "?", e1, e2, e3, "");}
  |  ^('||'  e1=expression e2=expression) {expr = new Expression(ExprType.BINARY, "||" , e1, e2, "");}
  |  ^('&&'  e1=expression e2=expression) {expr = new Expression(ExprType.BINARY, "&&" , e1, e2, "");}
  |  ^('=='  e1=expression e2=expression) {expr = new Expression(ExprType.BINARY, "==" , e1, e2, "");}
  |  ^('!='  e1=expression e2=expression) {expr = new Expression(ExprType.BINARY, "!=" , e1, e2, "");}
  |  ^('>='  e1=expression e2=expression) {expr = new Expression(ExprType.BINARY, ">=" , e1, e2, "");}
  |  ^('<='  e1=expression e2=expression) {expr = new Expression(ExprType.BINARY, "<=" , e1, e2, "");}
  |  ^('>'   e1=expression e2=expression) {expr = new Expression(ExprType.BINARY, ">"  , e1, e2, "");}
  |  ^('<'   e1=expression e2=expression) {expr = new Expression(ExprType.BINARY, "<" ,  e1, e2, "");}
  |  ^('...' e1=expression e2=expression) {expr = new Expression(ExprType.BINARY, "...",  e1, e2, "");}
  |  ^('+'   e1=expression e2=expression) {expr = new Expression(ExprType.BINARY, "+" ,  e1, e2, "");}
  |  ^('-'   e1=expression e2=expression) {expr = new Expression(ExprType.BINARY, "-" ,  e1, e2, "");}
  |  ^('*'   e1=expression e2=expression) {expr = new Expression(ExprType.BINARY, "*" ,  e1, e2, "");}
  |  ^('/'   e1=expression e2=expression) {expr = new Expression(ExprType.BINARY, "/" ,  e1, e2, "");}
  |  ^('%'   e1=expression e2=expression) {expr = new Expression(ExprType.BINARY, "\%" , e1, e2, "");}
  |  ^('^'   e1=expression e2=expression) {expr = new Expression(ExprType.BINARY, "^" ,  e1, e2, "");}
  |  ^(UNARY_MIN e1=expression) {expr = new Expression(ExprType.UNARY, "-" , e1, "");}
  |  ^(NEGATE e1=expression) {expr = new Expression(ExprType.UNARY, "!" , e1, "");}
  |  Number {expr = new Expression(ExprType.CONST, "" , $Number.text);}
  |  Bool {expr = new Expression(ExprType.CONST, "" , $Bool.text);}
  |  Null {expr = new Expression(ExprType.CONST, "" , $Null.text);}
  |  e1=functionCall {expr = e1;}
  |  e1=lookup {expr = e1;}   
  |  e1=cast_expression {expr = e1;}        
  ;

cast_expression returns [Expression expr]
    : ^(CAST s=predefined_type exp=expression) {
        expr = new Expression(ExprType.UNARY, "", exp, "", s);
    }
    ;

predefined_type returns [String s] 
    :
    | 'byte'    {s = "byte";}
    | 'char'    {s = "char";}
    | 'double'  {s = "double";}
    | 'float'   {s = "float";}
    | 'int'     {s = "int";}
    | 'long'    {s = "long";}
    | 'sbyte'   {s = "sbyte";}
    | 'short'   {s = "short";}
    | 'uint'    {s = "unit";}
    | 'ulong'   {s = "ulong";}
    | 'ushort'  {s = "ushort";}
;
    
lookup returns [Expression expr]
  :  ^(LOOKUP Id (eList=indexes)?) {
        if (eList==null) 
          expr = new Expression(ExprType.ID, "", $Id.text);
        else
          expr = new Expression(ExprType.ARRAY, eList, $Id.text);
     }
  | ^(LOOKUP String (eList=indexes)?) {
        if (eList==null) 
          expr = new Expression(ExprType.CONST, "", $String.text);
        else
          expr = new Expression(ExprType.ARRAY, eList, $String.text);
     }   
  |  ^(LOOKUP exp=expression indexes?) {expr = exp;}  
  ;

indexes returns [List<Expression> lst]
      @init{lst = new ArrayList<Expression>(); }
  :  ^(INDEXES (exp=expression {lst.add(exp);})+) 
  ;
  
