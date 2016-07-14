package feedback.update;

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
import org.antlr.runtime.tree.CommonTree;
import org.antlr.runtime.tree.CommonTreeNodeStream;

//import feedback.update.parser.Rewrite;
import feedback.update.parser.Rewrite;
import feedback.update.parser.UpdateLexer;
import feedback.update.parser.UpdateParser;
import feedback.update.parser.UpdateParser.prog_return;
import feedback.update.parser.UpdateTreeWalker;
import feedback.utils.AssignmentStatement;
import feedback.utils.Expression;
import feedback.utils.LoopStatement;
import feedback.utils.Statement;
import feedback.utils.Expression.ExprType;


public class RewriteTest {
	
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
       
    	String realex = "UPDATE:" +
    			"loop (1, i, 1, dp_ProgInput_2-1) {" +
    			"loop (1, j, 1, dp_ProgInput_2) {" +
    			"(true):dp_Array_1[i][0] = dp_Array_1[i - 1][0] + dp_ProgInput_3[i][0];" +
    			"(true):dp_Array_1[i][j] = dp_Array_1[i][j - 1] + dp_ProgInput_3[i][j];" +
    			"}}";


    	
        String sub = "UPDATE:" +
        		"((!(j == 0 && i == 0)) && (!(j == 0)) && " +
        		"(dp_Array_1[i - 1][j - 1] && dp_Array_1[i - 1][j])):" +
        		"dp_Array_1[i][j] = " +
        		"((((dp_Array_1[i - 1][j - 1]) > (dp_Array_1[i - 1][j]) ? " +
        		"(dp_Array_1[i - 1][j - 1]) : (dp_Array_1[i - 1][j])))) + dp_ProgInput_3[i][j];";
        
        String rewritten = "UPDATE:" +
        		"loop (1, i, 1, dp_ProgInput_2-1) {" +
        		"(true):dp_Array_1[0][i] = dp_Array_1[0][i - 1] + dp_Array_1[0][i];" +
        		"(true):dp_Array_1[i][0] = dp_Array_1[i][0] + dp_Array_1[i - 1][0];" +
        		"}" +
        		"loop (-1, i, 1, dp_ProgInput_2-1) {" +
        		"loop (1, j, 1, dp_ProgInput_2-1) {" +
        		"(true):dp_Array_1[i][j] = dp_Array_1[i][j] + max(dp_Array_1[i - 1][j], dp_Array_1[i][j - 1]);" +
        		"}" +
        		"}";

        String test = "UPDATE: loop (1, i, 0, dp_ProgInput_2-1){loop (1, j, 0, dp_ProgInput_2-1) {((i == 0) && (j == 0)):dp_Array_1[i][j] = dp_Array_1[0][0];" +
        		"((!((i == 0) && (j == 0))) && (j == 0)):dp_Array_1[i][j] = dp_Array_1[i][j] + (dp_Array_1[i - 1][j]);}}";
        		
        ANTLRStringStream stream = new ANTLRStringStream(sub);
        UpdateLexer lexer = new UpdateLexer(stream);
        CommonTokenStream tokenStream = new CommonTokenStream(lexer);
        UpdateParser parser = new UpdateParser(tokenStream);
        prog_return prog = parser.prog();
        System.out.println(prog.getTree().toStringTree());
        
        CommonTree t = (CommonTree)prog.getTree();
        CommonTreeNodeStream nodes = new CommonTreeNodeStream(t);
        Rewrite s = new Rewrite(nodes);
        t = (CommonTree)s.downup(t, true); // walk t, trace transforms
        System.out.println("Simplified tree: "+t.toStringTree());
        
        CommonTreeNodeStream nodeStream = new CommonTreeNodeStream(t);
        UpdateTreeWalker walker = new UpdateTreeWalker(nodeStream);
        List<Statement> p = walker.prog();
        for (Statement stmt: walker.stmts){
        		System.out.println(stmt);
        }
//        
//        System.out.println(prog.getTree().toStringTree());
        
    }

}
