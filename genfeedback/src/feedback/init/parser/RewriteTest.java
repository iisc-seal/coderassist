package feedback.init.parser;

import java.util.List;
import java.util.TreeMap;

import org.antlr.runtime.ANTLRStringStream;
import org.antlr.runtime.CharStream;
import org.antlr.runtime.CommonTokenStream;
import org.antlr.runtime.RecognitionException;
import org.antlr.runtime.TokenStream;
import org.antlr.runtime.tree.CommonTree;
import org.antlr.runtime.tree.CommonTreeNodeStream;

import feedback.init.parser.initParseParser.prog_return;
import feedback.init.parser.InitTreeWalker;
import feedback.utils.AssignmentStatement;
import feedback.utils.LoopStatement;
import feedback.utils.Statement;


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
        String test = "INIT: dp_Array_1[1 ... n][0] = 0 [g]; dp_Array_1[1 ... n][0] = 0 [g];";
        String test1 = "INIT: dp_Array_1[0 ... 20][0... i+j-1] = 0; loop (i, 1, dp_ProgInput_3) { loop (j, 1, dp_ProgInput_2) { (i==0||j==0):dp_Array_1[i][j] = 0; } } ";
        String s1 = "INIT: loop (i, 0, n) { loop (j, 0, m) { (i==1||j==1):dp_Array_1[i][j] = 0; } } ";
        
        String s2 = "INIT: loop (1, i, 0, n) { loop (1, j, 1, m) { (j==0):dp_Array_1[i][j] = 0; } } ";
        String s3 = "INIT: loop (1, i, 1, n) { loop (1, j, 1, m) { (i==1):dp_Array_1[i][j] = 0; } } ";
        
        String s11 = "INIT: loop (i, 1, n) { loop (j, 1, m) { dp_Array_1[1...n][0] = 0; dp_Array_1[0][1...m] = 0; } } ";
        
        String sub = "INIT:" +
        		//" (true):dp_Array_1[0 ... 20][0 ... 20000] = 0 [g];" +
        		" loop (1, i, 0, dp_ProgInput_2-1) {" +
        		"   loop (1, j, 0, dp_ProgInput_3-3) {" +
        		"     (i==0 || j == 0):dp_Array_1[i][j] = 0;" +
        		"  	}" +
        		" }" ;
        		//" (true):dp_Array_1[0 ... 20][0 ... 20000] = 0;";

        String example = "INIT:" +
        		//" (true):dp_Array_1[0 ... 20][0 ... 20000] = 0 [g];" +
        		" loop (1, i, 0, dp_ProgInput_2-1) {" +
        		"   loop (1, j, 0, dp_ProgInput_3-1) {" +
        		"     (true):dp_Array_1[0][0] = dp_ProgInput_3[0][0];" +
        		"  	}" +
        		" }" ;
        
        stream = new ANTLRStringStream(sub);
        lexer = new initParseLexer(stream);
        tokenStream = new CommonTokenStream(lexer);
        parser = new initParseParser(tokenStream);
        prog = parser.prog();
        System.out.println(prog.tree.toStringTree());
        
        CommonTree t = (CommonTree)prog.getTree();
        CommonTreeNodeStream nodes = new CommonTreeNodeStream(t);
        Rewrite s = new Rewrite(nodes);
        t = (CommonTree)s.downup(t, true); // walk t, trace transforms
        System.out.println("Simplified tree: "+t.toStringTree());
        
        nodeStream = new CommonTreeNodeStream(t);
        walker = new InitTreeWalker(nodeStream);
        p = walker.prog();
        for (Statement stmt: walker.stmts){
        	System.out.println(stmt);
        }
//        
//        System.out.println(prog.getTree().toStringTree());
        
    }

}
