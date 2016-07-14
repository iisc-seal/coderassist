tree grammar InitTreeWalker;

options {
  language = Java;
  tokenVocab = initParse;
  ASTLabelType = CommonTree;  
}

@header {
  package feedback.init.parser;
  import feedback.utils.Expression;
  import feedback.utils.AssignmentStatement;
  import feedback.utils.LoopStatement;
  import feedback.init.BlockStatement;
  import feedback.utils.Statement;
  import feedback.utils.Assignment;
  import feedback.utils.Expression.ExprType;
  import java.util.Map;
  import java.util.HashMap;
  import java.util.TreeMap;
  import java.util.ArrayList;
  import java.util.List;
  import java.util.Set;
  import java.util.HashSet;
}

@members {
  private Map<String, Integer> variables = new HashMap<String, Integer>();
  public List<Statement> stmts = new ArrayList<Statement>();
  @Override
  protected Object recoverFromMismatchedToken(IntStream input, int ttype, BitSet follow) throws RecognitionException {
    throw new MismatchedTokenException(ttype, input);
  }

  @Override
  public Object recoverFromMismatchedSet(IntStream input, RecognitionException e, BitSet follow) throws RecognitionException {
    throw e;
  }
  
}

@rulecatch {
    catch (RecognitionException e) {
        throw e;
    }
}

prog returns [List<Statement> lst]
  : b=block {lst = b;} 
  ;

block returns [List<Statement> lst]
  @init{lst = new ArrayList<Statement>(); }
  : ^(BLOCK ^(STATEMENT (s = statement {
	                                        if(s instanceof AssignmentStatement){
	                                            if(!((AssignmentStatement) s).isGlobalInit())
	                                                stmts.add(s); lst.add(s);
	                                        } else{
	                                            stmts.add(s); lst.add(s);
	                                        } 
                                                
                                        })* 
     )) 
  ;

statement returns [Statement stmt]
  : lStmt=loopStatement {stmt = lStmt;} 
  | aStmt=assignmentStatement {//System.out.println("Matched aStmt "+aStmt); 
        stmt = aStmt;}  
  ;

loopStatement returns [LoopStatement lStmt]
  @init{List<Statement> lst = new ArrayList<Statement>(); }
  : ^(LOOP dir=expression index=expression lb=expression ^(UB ub=expression) line=expression (s=statement {lst.add(s);})*){
    lStmt = new LoopStatement(line, dir, index, lb, ub, lst);
  }
  ;

assignmentStatement returns [AssignmentStatement aStmt]
  : ^(GUARD exp=expression? assign=assignment) {
    aStmt = new AssignmentStatement(assign.isGlobal(), exp, assign.getLHS(), assign.getRHS());
    }
  ;
  
assignment returns [Assignment assign]
  : ^(ASSIGNMENT Id (eList=indexes)? rhs=expression (isGlobal=global)?) {
    Expression lhs = new Expression(ExprType.ARRAY,  eList, $Id.text);
    assign = new Assignment(isGlobal, lhs, rhs); 
  }
  ;

global returns [boolean isGlobal]
    @init{isGlobal = false;}
  : ^(GLOBAL 'g') {isGlobal = true;}
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
  |  ^('OR'  e1=expression e2=expression) {expr = new Expression(ExprType.BINARY, "OR" , e1, e2, "");}
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
  ;

//list
//  :  ^(LIST exprList?)
//  ;

lookup returns [Expression expr]
//:  ^(LOOKUP functionCall indexes?)
//|  ^(LOOKUP list indexes?)
//|  ^(LOOKUP expression indexes?) 
  :  ^(LOOKUP Id (eList=indexes)?) {
        if (eList==null) 
          expr = new Expression(ExprType.ID, "", $Id.text);
        else
          expr = new Expression(ExprType.ARRAY, eList, $Id.text);
     }
  |  ^(LOOKUP exp=expression indexes?) {expr = exp;} 
//|  ^(LOOKUP String indexes?)
  ;

indexes returns [List<Expression> lst]
      @init{lst = new ArrayList<Expression>(); }
  :  ^(INDEXES (exp=expression {lst.add(exp);})+) 
  ;
