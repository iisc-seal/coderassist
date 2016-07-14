package feedback.init.parser;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

import org.antlr.runtime.ANTLRStringStream;
import org.antlr.runtime.CharStream;
import org.antlr.runtime.CommonTokenStream;
import org.antlr.runtime.RecognitionException;
import org.antlr.runtime.TokenStream;
import org.antlr.runtime.tree.CommonTreeNodeStream;

import feedback.init.parser.initParseParser.prog_return;
import feedback.init.parser.InitTreeWalker;
import feedback.utils.AssignmentStatement;
import feedback.utils.Expression;
import feedback.utils.LoopStatement;
import feedback.utils.Statement;
import feedback.utils.Expression.ExprType;


public class RegressionTest {
	
	public static AssignmentStatement getEnclosedAssignment(Statement stmt, TreeMap<Integer, LoopStatement> enclosingScope, int depth){
		if(stmt instanceof AssignmentStatement){
			return (AssignmentStatement) stmt;
		}
		else{
			LoopStatement lStmt = (LoopStatement) stmt;
			enclosingScope.put(depth, lStmt);
			return getEnclosedAssignment(lStmt.getBlock().get(0), enclosingScope, depth+1);
		}
	}
	
    public static void main(String[] args) throws RecognitionException {
        CharStream stream =
                new ANTLRStringStream("INIT: dp_Array_1[0][0 ... dp_ProgInput_2] = true;");
        initParseLexer lexer = new initParseLexer(stream);
        TokenStream tokenStream = new CommonTokenStream(lexer);
        initParseParser parser = new initParseParser(tokenStream);
        prog_return prog = parser.prog();
//        System.out.println(prog.getTree().toStringTree());
        CommonTreeNodeStream nodeStream = new CommonTreeNodeStream(prog.getTree());
        InitTreeWalker walker = new InitTreeWalker(nodeStream);
        List<Statement> p = walker.prog();
//        for (Statement stmt: p)
//        	System.out.println(stmt);
        String test = "INIT: dp_Array_1[0 ... 20][0... i+j-1] = 0;";
        String test1 = "INIT: dp_Array_1[0 ... 20][0... i+j-1] = 0; loop (i, 0, dp_ProgInput_3) { loop (j, 0, dp_ProgInput_2) { (i==0||j==0):dp_Array_1[i][j] = 0; } } ";
        String test3 = "INIT:loop(1, i, 0, dp_ProgInput_2 - 1) {loop(1, j, 0, dp_ProgInput_2 - 1) {(true): dp_Array_1[i][j] = 0 ;}}";
        String test4 = "INIT: loop(-1, size, dp_ProgInput_2, 0 + 1) {" +
        		"loop(1, i, 1, dp_ProgInput_2 - size) {" +
        		"(true): dp_Array_1[dp_ProgInput_2 - size][i] = 0 ;" +
        		"}}" +
        		"(true): dp_Array_1[1][1] = dp_ProgInput_3[1][1] ;";
        stream = new ANTLRStringStream(test4);
        lexer = new initParseLexer(stream);
        tokenStream = new CommonTokenStream(lexer);
        parser = new initParseParser(tokenStream);
        prog = parser.prog();
        System.out.println(prog.tree.toStringTree());
        nodeStream = new CommonTreeNodeStream(prog.getTree());
        walker = new InitTreeWalker(nodeStream);
        p = walker.prog();
        for (Statement stmt: walker.stmts){
        	System.out.println(stmt);
        }
        
        List<AssignmentStatement> toProcess = new ArrayList<AssignmentStatement>();
        
        for (Statement stmt: walker.stmts){
        	if(stmt instanceof LoopStatement){        		
        		TreeMap<Integer, LoopStatement> enclosingScope = 
        				new TreeMap<Integer, LoopStatement>();
        		AssignmentStatement enclosedAssignment = getEnclosedAssignment(stmt, enclosingScope, 0);
        		
        		/* We could implement more general matchers here but for now, we only implement these rewrites:
        		 * 1. loop(i, 0, n){loop(j, 0, m){i==0||j==0:array[i][j]=0} --> array[0][0..m] = 0, array[0..n][0] = 0
        		 * 2. loop(i, 0, n){loop(j, 0, m){j==0:array[i][j]=0} --> array[0..n][0] = 0
        		 * 3. loop(i, 0, n){loop(j, 0, m){i==0:array[i][j]=0} --> array[0][0..m] = 0
        		 **/
        		if(enclosingScope.size() == 2){
        			LoopStatement outer = enclosingScope.get(0);
        			LoopStatement inner = enclosingScope.get(1);
        			String lb, ub, lbouter, lbinner, ubouter, ubinner;
        			String outerIndex = outer.getIndex().identifierVisitor().get(0);
        			String innerIndex = inner.getIndex().identifierVisitor().get(0);
        			String opattern = outerIndex + "==0";
        			String ipattern = innerIndex + "==0";
        			String disjointPattern = opattern + "||" + ipattern;
        			String guard = enclosedAssignment.getGuard().printVisitor().replaceAll("\\s", "");
        			
        			/*3. loop(i, 0, n){loop(j, 0, m){i==0:array[i][j]=0} --> array[0][0..m] = 0*/
        			if(guard.compareTo(opattern) == 0){
        				lb = inner.getLowerBound().identifierVisitor().get(0);
        				ub = inner.getUpperBound().identifierVisitor().get(0);
        				
        				/* we don't want our bounds to be expressions: they must be ids or consts*/
        				if(outer.getLowerBound().identifierVisitor().size() == 1 && "0".compareTo(lb) == 0){
        					if(outer.getUpperBound().identifierVisitor().size() == 1 
        							&& inner.getUpperBound().identifierVisitor().size() == 1){
        						
        							Expression dim1 = new Expression(ExprType.CONST, "", "0");
        							Expression dim2 = Expression.CreateRangeExpression(lb, ub);
        							Expression lhs = enclosedAssignment.getLhs();
        							List<Expression> indices = new ArrayList<Expression>();
        							indices.add(dim1);
        							indices.add(dim2);
        							lhs.seteList(indices);
        							toProcess.add(enclosedAssignment);
        						
        					}
        				}        				        				
        			}
        			
        			/*2. loop(i, 0, n){loop(j, 0, m){j==0:array[i][j]=0} --> array[0..n][0] = 0*/
        			else if(guard.compareTo(ipattern) == 0){
        				lb = outer.getLowerBound().identifierVisitor().get(0);
        				ub = outer.getUpperBound().identifierVisitor().get(0);
        				
        				/* we don't want our bounds to be expressions: they must be ids or consts*/
        				if(inner.getLowerBound().identifierVisitor().size() == 1 && "0".compareTo(lb) == 0){
        					if(inner.getUpperBound().identifierVisitor().size() == 1 
        							&& outer.getUpperBound().identifierVisitor().size() == 1){
        						
        							Expression dim1 = Expression.CreateRangeExpression(lb, ub);
        							Expression dim2 = new Expression(ExprType.CONST, "", "0");
        							Expression lhs = enclosedAssignment.getLhs();
        							List<Expression> indices = new ArrayList<Expression>();
        							indices.add(dim1);
        							indices.add(dim2);
        							lhs.seteList(indices);
        							toProcess.add(enclosedAssignment);
        						
        					}
        				}
        			}
        			
        			/*1. loop(i, 0, n){loop(j, 0, m){i==0||j==0:array[i][j]=0} --> array[0][0..m] = 0, array[0..n][0] = 0*/
        			else if(guard.compareTo(disjointPattern) == 0){
        				lbouter = outer.getLowerBound().identifierVisitor().get(0);
        				ubouter = outer.getUpperBound().identifierVisitor().get(0);
        				lbinner = inner.getLowerBound().identifierVisitor().get(0);
        				ubinner = inner.getUpperBound().identifierVisitor().get(0);
        				
        				/*Make sure the bounds are sane*/
        				if("0".compareTo(lbouter) == 0 && "0".compareTo(lbinner)==0){
        					if(inner.getUpperBound().identifierVisitor().size() == 1 
        							&& outer.getUpperBound().identifierVisitor().size() == 1){
        						
        						Expression clonedLhs = (Expression) enclosedAssignment.getLhs().clone();
        						
        						/**/
        						Expression dim1 = new Expression(ExprType.CONST, "", "0");
    							Expression dim2 = Expression.CreateRangeExpression(lbinner, ubinner);
    							Expression lhs = enclosedAssignment.getLhs();
    							List<Expression> indices = new ArrayList<Expression>();
    							indices.add(dim1);
    							indices.add(dim2);
    							lhs.seteList(indices);
    							toProcess.add(enclosedAssignment);
        						
    							/**/
    							dim1 = Expression.CreateRangeExpression(lbouter, ubouter);
    							dim2 = new Expression(ExprType.CONST, "", "0");
    							indices = new ArrayList<Expression>();
    							indices.add(dim1);
    							indices.add(dim2);
    							clonedLhs.seteList(indices);
    							toProcess.add(new AssignmentStatement(enclosedAssignment.getGuard(), clonedLhs, enclosedAssignment.getRhs()));
        						
        					}
        				}
        				
        			}
        		}
        		else 
        			toProcess.add(enclosedAssignment);
        		
        		Iterator it = enclosingScope.entrySet().iterator();
        	    while (it.hasNext()) {
        	        Map.Entry pairs = (Map.Entry)it.next();
        	        System.out.println(pairs.getKey() + " = " + pairs.getValue());        	    
        	    }
        		
        	}
        	else
        		toProcess.add((AssignmentStatement) stmt);
        }
        
        System.out.println(prog.getTree().toStringTree());
        System.out.println(toProcess);
}

}
