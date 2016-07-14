tree grammar ExpressionTreeWalker;

options {
  language = Java;
  tokenVocab = Expr;
  ASTLabelType = CommonTree;  
}

@header {
  package feedback.update.parser;
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
  import java.util.Collections;
  import java.util.Random;
}

@members {
  public Map<String, String> variables = new HashMap<String, String>();
  
  public boolean useRandomOnFailedLookup = true;
  
  public Integer computeValue(String fName, List<Integer> eList){
        if("max".compareTo(fName) == 0){
            return Collections.max(eList);
        }
        else if(useRandomOnFailedLookup)
            return (new Random()).nextInt(10);
        else
            return null;
  }

  
//  @Override
//  protected Object recoverFromMismatchedToken(IntStream input, int ttype, BitSet follow) throws RecognitionException {
//    throw new MismatchedTokenException(ttype, input);
//  }
//
//  @Override
//  public Object recoverFromMismatchedSet(IntStream input, RecognitionException e, BitSet follow) throws RecognitionException {
//    throw e;
//  }
  
}

@rulecatch {
    catch (RecognitionException e) {
        throw e;
    }
}

eval returns [Integer value] 
    : exp=expression {value = exp;}
    ;

functionCall returns [Integer value]
  :  ^(FUNC_CALL Id eList=exprList?) {value = computeValue($Id.text, eList);}
  ;

exprList returns [List<Integer> lst]
      @init{lst = new ArrayList<Integer>(); }
  :  ^(EXP_LIST (exp=expression {lst.add(exp);})+)
  ;

expression returns [Integer value]
  :  ^(TERNARY e1=expression e2=expression e3=expression) {if(e1 != null && e2 != null && e3 != null) {if(e1 > 0) value = e2; else value = e3;} else {value = null;}}
  |  ^('||'  e1=expression e2=expression) {
                                            if(e1 != null && e2 != null) {
                                                    if ((e1!=0) || (e2!=0)) value = 1; else value = 0;
                                            } else{
                                                value = null;
                                            }
                                          }
  |  ^('&&'  e1=expression e2=expression) {
                                            if(e1 != null && e2 != null) {
                                                    if((e1!=0) && (e2!=0)) value = 1; else value = 0;
                                            } else{
                                                value = null;
                                            }
                                          }
                    
  |  ^('=='  e1=expression e2=expression) {
                                            if(e1 != null && e2 != null) {
                                                    if((e1 == e2)) value = 1; else value = 0;
                                            } else{
                                                value = null;
                                            }
                                          } 
                                          
  |  ^('!='  e1=expression e2=expression) {
                                            if(e1 != null && e2 != null) {
                                                    if((e1 != e2)) value = 1; else value = 0;
                                            } else{
                                                value = null;
                                            }
                                          }  
  
  |  ^('>='  e1=expression e2=expression) {
                                            if(e1 != null && e2 != null) {
                                                    if((e1 >= e2)) value = 1; else value = 0;;
                                            } else{
                                                value = null;
                                            }
                                          }  
  
 
  |  ^('<='  e1=expression e2=expression) {
                                            if(e1 != null && e2 != null) {
                                                    if((e1 <= e2)) value = 1; else value = 0;;
                                            } else{
                                                value = null;
                                            }
                                          } 
                                          
  |  ^('>'   e1=expression e2=expression) {
                                            if(e1 != null && e2 != null) {
                                                    if((e1 > e2)) value = 1; else value = 0;;
                                            } else{
                                                value = null;
                                            }
                                          } 
                                          
  |  ^('<'   e1=expression e2=expression) {
                                            if(e1 != null && e2 != null) {
                                                    if((e1 < e2)) value = 1; else value = 0;;
                                            } else{
                                                value = null;
                                            }
                                          } 
                                          
  |  ^('...' e1=expression e2=expression) {value = null;}
  
  |  ^('+'   e1=expression e2=expression) {
                                            if(e1 != null && e2 != null)
                                                    value = e1 + e2;
                                            else
                                                value = null;
                                          } 
                                          
  |  ^('-'   e1=expression e2=expression) {
                                            if(e1 != null && e2 != null)
                                                    value = e1 - e2;
                                            else
                                                value = null;
                                          } 
                                          
  |  ^('*'   e1=expression e2=expression) {
                                            if(e1 != null && e2 != null)
                                                    value = e1 * e2;
                                            else
                                                value = null;
                                          } 
                                          
  |  ^('/'   e1=expression e2=expression) {
                                            if(e1 != null && e2 != null && e2 != 0)
                                                    value = e1 / e2;
                                            else
                                                value = null;
                                          } 
                                          
  |  ^('%'   e1=expression e2=expression) {
                                            if(e1 != null && e2 != null && e2 != 0)
                                                    value = e1 \% e2;
                                            else
                                                value = null;
                                          } 
                                          
  |  ^('^'   e1=expression e2=expression) {
                                            if(e1 != null && e2 != null) 
                                                    value = e1 ^ e2;
                                            else
                                                value = null;
                                          } 
                                          
  |  ^(UNARY_MIN e1=expression) {
                                            if(e1 != null)
                                                    value = -e1;
                                            else
                                                value = null;
                                } 
                                
  |  ^(NEGATE e1=expression) {
                                            if(e1 != null)
                                                    {if (e1 > 0) value = 0; else value = 1;}
                                            else
                                                value = null;
                                }  
  |  Number {Double d = Double.parseDouble($Number.text); value = d.intValue();}
  |  Bool {if("true".compareTo($Bool.text) == 0) value = 1; else value = 0;}
  |  Null {value = null;}
  |  e1=functionCall {value = e1;}
  |  e1=lookup {value = e1;}           
  ;

//list
//  :  ^(LIST exprList?)
//  ;

lookup returns [Integer value]
//:  ^(LOOKUP functionCall indexes?)
//|  ^(LOOKUP list indexes?)
//|  ^(LOOKUP expression indexes?) 
  :  ^(LOOKUP Id (eList=indexes)?) {
        //assert(eList == null);
        String val = variables.get($Id.text);
        if(val == null){
            if(useRandomOnFailedLookup){
              System.out.println("WARNING! Random value used");    
              value = new Random().nextInt(10); 
              //val = i.toString();
            }
            else{
              System.out.println("WARNING! Lookup failed");  
              value = null;
            }
        }
        else if("true".compareTo(val) != 0 && "false".compareTo(val) != 0){
            Double d = Double.parseDouble(val);
            value = d.intValue();
        }
        else if ("true".compareTo(val) == 0)
            value = 1;
        else
            value = 0;
     }
  | ^(LOOKUP String (eList=indexes)?) {
        value = -1;
     }   
  |  ^(LOOKUP exp=expression indexes?) {value = exp;}  
  ;

indexes returns [List<Integer> lst]
      @init{lst = new ArrayList<Integer>(); }
  :  ^(INDEXES (exp=expression {lst.add(exp);})+) 
  ;
