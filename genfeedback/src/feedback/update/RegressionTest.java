package feedback.update;

import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.antlr.runtime.ANTLRStringStream;
import org.antlr.runtime.CharStream;
import org.antlr.runtime.CommonTokenStream;
import org.antlr.runtime.RecognitionException;
import org.antlr.runtime.TokenStream;
import org.antlr.runtime.tree.CommonTreeNodeStream;

import feedback.update.parser.ExprLexer;
import feedback.update.parser.ExprParser;
import feedback.update.parser.ExpressionTreeWalker;
import feedback.update.parser.UpdateLexer;
import feedback.update.parser.UpdateParser;
import feedback.update.parser.UpdateParser.expression_return;
import feedback.update.parser.UpdateParser.prog_return;
import feedback.update.parser.UpdateTreeWalker;
import feedback.utils.Statement;

public class RegressionTest {
	private static final Map<String, String> variables;
    static {
        Map<String, String> aMap = new HashMap<String, String>();
        aMap.put("i", "10");
        aMap.put("j", "13");
        aMap.put("a", "14");
        aMap.put("b", "99");
        aMap.put("pred", "true");
        aMap.put("ref0", "-1");
        aMap.put("ref3", "0");
        variables = Collections.unmodifiableMap(aMap);
    };
	
    public static void main(String[] args) throws RecognitionException {
    	String s = "UPDATE:" +
    			"loop(1, i, 1, non){" +
    			"loop(1, w, 1, amount){" +
    			"(arr[i] <= w):dp_Array_1[i][w] = dp_Array_1[i-1][w] || dp_Array_1[i-1][w-arr[i]];" +
    			"!(arr[i] <= w):dp_Array_1[i][w] = dp_Array_1[i-1][w];}}";
    	
    	String str = "UPDATE:" +
    		"loop (1, i, 1, dp_ProgInput_2-1) {" +
    		"loop (1, j, 1, dp_ProgInput_2-1) {" +
    		"(true):dp_Array_1[i][j] = dp_Array_1[i][j] + (dp_Array_1[i - 1][j] > dp_Array_1[i][j - 1]) ? (dp_Array_1[i - 1][j]) : (dp_Array_1[i][j - 1]);" +
    		"}}";
    	
    	String exp = "(i >= j && !(a<b) || pred)";
    	String exp1 = "(ref0 > ref3)";
    	
    	String test = "UPDATE: loop (1, i, 0, dp_ProgInput_2-1){loop (1, j, 0, dp_ProgInput_2-1) {((i == 0) && (j == 0)):dp_Array_1[i][j] = dp_Array_1[0][0];}}";
    	
    	CharStream strm = new ANTLRStringStream(exp1);
    	ExprLexer lex = new ExprLexer(strm);
    	TokenStream tokenStrm = new CommonTokenStream(lex);
    	ExprParser pars = new ExprParser(tokenStrm);
    	feedback.update.parser.ExprParser.expression_return expression = pars.expression();
    	System.out.println(expression.getTree().toStringTree());
    	CommonTreeNodeStream nStream = new CommonTreeNodeStream(expression.getTree());
    	ExpressionTreeWalker expWalker = new ExpressionTreeWalker(nStream);
    	expWalker.variables.putAll(variables);
    	int value = expWalker.eval();
    	System.out.println(value);
    	
        CharStream stream =
                new ANTLRStringStream(test);
        UpdateLexer lexer = new UpdateLexer(stream);
        TokenStream tokenStream = new CommonTokenStream(lexer);
        UpdateParser parser = new UpdateParser(tokenStream);
        prog_return primary = parser.prog();
        System.out.println(primary.getTree().toStringTree());
        CommonTreeNodeStream nodeStream = new CommonTreeNodeStream(primary.getTree());
        UpdateTreeWalker walker = new UpdateTreeWalker(nodeStream);
        List<Statement> p = walker.prog();
        for(Statement stmt: p){
        	System.out.println(stmt.toString());
        	System.out.println(stmt.equals(stmt));
        }
}

}
