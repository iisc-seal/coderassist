package feedback.output;

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

import feedback.output.parser.OutputLexer;
import feedback.output.parser.OutputParser;
import feedback.output.parser.OutputParser.primary_return;
import feedback.output.parser.OutputTreeWalker;
import feedback.output.parser.Rewrite;
import feedback.utils.AssignmentStatement;
import feedback.utils.Expression;
import feedback.utils.LoopStatement;
import feedback.utils.Statement;
import feedback.utils.Expression.ExprType;


public class RegressionTest {
	

    public static void main(String[] args) throws RecognitionException {
    	String s = "OUTPUT: " +
    			"(dp_Array_1[dp_ProgInput_2][dp_ProgInput_3] == dp_ProgInput_3):\"YES\"" +
    			" (!(dp_Array_1[dp_ProgInput_2][dp_ProgInput_3] == dp_ProgInput_3)):\"NO\";";
    	
    	String op = "OUTPUT:" +
    			"(dp_Array_1[n - 1][n - 1] < 0): \"Bad Judges\";" +
    			"(!(dp_Array_1[n - 1][n - 1] < 0)): dp_Array_1[n - 1][n - 1] / ((float)(2 * n - 3));";
 
    	String op1 = "OUTPUT:" +
    			"(dp_Array_1[dp_ProgInput_2][dp_ProgInput_2] * 1. / (2 * dp_ProgInput_2 - 3) >= 0): " +
    			"	dp_Array_1[dp_ProgInput_2][dp_ProgInput_2] * 1. / (2 * dp_ProgInput_2) - 3;" +
    			"(!((dp_Array_1[dp_ProgInput_2][dp_ProgInput_2] * 1. / (2 * dp_ProgInput_2 - 3) >= 0))):" +
    			"\"Bad Judges\";";

    	String op2 = "OUTPUT: (x/y >= 0): z;";
    	
    	String op3 = "OUTPUT: " +
    			"((dp_Array_1[dp_ProgInput_2][dp_ProgInput_2]) / ((float)2 * dp_ProgInput_2 - 3) >= 0): " +
    			"((dp_Array_1[dp_ProgInput_2][dp_ProgInput_2]) / ((float)2 * dp_ProgInput_2 - 3)) [N] ;";
    	
    	String testOp = "OUTPUT:" +
    			"(((1. * dp_Array_1[0][0] / (2. * dp_ProgInput_2 - 3))) >= 0): ((1. * dp_Array_1[0][0] / (2. * dp_ProgInput_2 - 3))) [N] ;" +
    			"(!(((1. * dp_Array_1[0][0] / (2. * dp_ProgInput_2 - 3))) >= 0)): \"Bad Judges\" [N] ;";
    	
        CharStream stream =
                new ANTLRStringStream(testOp);
        OutputLexer lexer = new OutputLexer(stream);
        TokenStream tokenStream = new CommonTokenStream(lexer);
        OutputParser parser = new OutputParser(tokenStream);
        primary_return primary = parser.primary();
        System.out.println(primary.getTree().toStringTree());
        
        CommonTree t = (CommonTree)primary.getTree();
        CommonTreeNodeStream nodes = new CommonTreeNodeStream(t);
        Rewrite rw = new Rewrite(nodes);
        t = (CommonTree)rw.downup(t, true); // walk t, trace transforms
//        System.out.println("Simplified tree: "+t.toStringTree());
        
        CommonTreeNodeStream nodeStream = new CommonTreeNodeStream(t);
        OutputTreeWalker walker = new OutputTreeWalker(nodeStream);
        
        List<Statement> p = walker.primary();
        for(Statement stmt: p){
        	System.out.println(stmt.toString());
//        	System.out.println(((AssignmentStatement) stmt).getGuard().computeTypeBottomUp());
//        	System.out.println(stmt.equals(stmt));
        }
}

}
