package feedback;

import java.util.List;

import org.antlr.runtime.ANTLRStringStream;
import org.antlr.runtime.CharStream;
import org.antlr.runtime.CommonTokenStream;
import org.antlr.runtime.RecognitionException;
import org.antlr.runtime.TokenStream;
import org.antlr.runtime.tree.CommonTreeNodeStream;

import feedback.init.parser.initParseLexer;
import feedback.init.parser.initParseParser;
import feedback.init.parser.initParseParser.prog_return;
import feedback.init.parser.InitTreeWalker;
import feedback.utils.Statement;


public class LoopRewriter {
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
        String test1 = "INIT: dp_Array_1[0 ... 20][0... i+j-1] = 0; loop (i, 0, dp_ProgInput_3) { loop (j, 0, dp_ProgInput_2) { (i == 0 || j == 0):dp_Array_1[i][j] = 0; } } ";
        stream = new ANTLRStringStream(test1);
        lexer = new initParseLexer(stream);
        tokenStream = new CommonTokenStream(lexer);
        parser = new initParseParser(tokenStream);
        prog = parser.prog();
        
        nodeStream = new CommonTreeNodeStream(prog.getTree());
        walker = new InitTreeWalker(nodeStream);
        p = walker.prog();
        for (Statement stmt: walker.stmts)
        	System.out.println(stmt);
        
        System.out.println(prog.getTree().toStringTree());
}

}
