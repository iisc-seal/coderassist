package feedback.utils;

import static org.junit.Assert.*;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.antlr.runtime.ANTLRStringStream;
import org.antlr.runtime.CharStream;
import org.antlr.runtime.CommonTokenStream;
import org.antlr.runtime.TokenStream;
import org.antlr.runtime.tree.CommonTreeNodeStream;
import org.junit.BeforeClass;
import org.junit.Test;

import com.microsoft.z3.BoolExpr;
import com.microsoft.z3.Context;
import com.microsoft.z3.Solver;
import com.microsoft.z3.Z3Exception;

import feedback.init.InitSummary;
import feedback.inputs.ProgramInputs;
import feedback.logging.Logger;
import feedback.update.parser.ExprLexer;
import feedback.update.parser.ExprParser;
import feedback.update.parser.ExpressionTreeWalker;
import feedback.update.parser.UpdateLexer;
import feedback.update.parser.UpdateParser;
import feedback.update.parser.UpdateTreeWalker;
import feedback.update.parser.UpdateParser.prog_return;

public class HelperFunctionsTest {

	static String errLog = "/home/anirudh/workspace/fixdp/testFiles/errorLog";
	static String progName = "/home/anirudh/workspace/fixdp/testFiles/testProg";

	static String expr = "((((((dp_ProgInput_2 == 1) && !((1 == 1))) && !(((((sub0 + sub4) < 0) && (sub3 > 0)) && (((sub2 + sub4) < 0) && (sub3 > 0))))) && !(((((sub0 + sub4) < 0) && (sub3 > 0)) && (((sub2 + sub4) < 0) && (sub3 > 0))))) && (((sub0 + sub4) / sub3) >= ((sub2 + sub4) / sub3))) && (((sub0 + sub4) / sub3) >= ((sub2 + sub4) / sub3)))";
	
	static String test = "UPDATE: " +
			"(i == 0): a[i][j] = a[i][j]+1;" +
			"(i == 0 && j == 0): a[i+1][j] = a[i][j]-1;" +
			"(i >= 0): a[i][j] = a[i][j]-1;" +
			"(i >= 0 && j >= 0): a[i+1][j] = a[i+1][j]+10;" 
			;
	
	static String submission = "UPDATE:" +
			"loop (1, i, 0, dp_ProgInput_2-1) {" +
			"loop (1, j, 0, i) {" +
			"((i > 0) && (j == 0)):dp_Array_1[i][j] = dp_Array_1[i][j] + (dp_Array_1[i - 1][j]);" +
			"((i > 0) && (!(j == 0))):" +
			"dp_Array_1[i][j] = dp_Array_1[i][j] + " +
			"((dp_Array_1[i - 1][j] > dp_Array_1[i - 1][j - 1] ? " +
			"dp_Array_1[i - 1][j] : dp_Array_1[i - 1][j - 1]));}}";

	static String reference = "UPDATE:" +
			"loop (1, i, 1, dp_ProgInput_2-1) {" +
			"loop (1, j, 0, i) {" +
			"(j == 0):dp_Array_1[i][j] = dp_Array_1[i - 1][j] + dp_Array_1[i][j];" +
			"((!(j == 0)) && (j == i)):" +
			"dp_Array_1[i][j] = dp_Array_1[i - 1][j - 1] + dp_Array_1[i][j];" +
			"((!(j == 0)) && (!(j == i)) && (dp_Array_1[i - 1][j] > dp_Array_1[i - 1][j - 1])):" +
			"dp_Array_1[i][j] = dp_Array_1[i - 1][j] + dp_Array_1[i][j];" +
			"((!(j == 0)) && (!(j == i)) && (!(dp_Array_1[i - 1][j] > dp_Array_1[i - 1][j - 1]))):" +
			"dp_Array_1[i][j] = dp_Array_1[i - 1][j - 1] + dp_Array_1[i][j];}}";

			
	static CharStream refStream =
            new ANTLRStringStream(reference);
    static UpdateLexer refLexer = new UpdateLexer(refStream);
    static TokenStream refTokenStream = new CommonTokenStream(refLexer);
    static UpdateParser refParser = new UpdateParser(refTokenStream);

    static CharStream subStream =
            new ANTLRStringStream(submission);
    static UpdateLexer subLexer = new UpdateLexer(subStream);
    static TokenStream subTokenStream = new CommonTokenStream(subLexer);
    static UpdateParser subParser = new UpdateParser(subTokenStream);
    
	private static List<Statement> p;
    static List<AssignmentStatement> rList = new ArrayList<AssignmentStatement>();
    static List<AssignmentStatement> sList = new ArrayList<AssignmentStatement>();
    static Block ref, sub;
	
	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
		 	prog_return refPrimary = refParser.prog();
		    CommonTreeNodeStream nodeStream = new CommonTreeNodeStream(refPrimary.getTree());
		    UpdateTreeWalker walker = new UpdateTreeWalker(nodeStream);
		    p = walker.prog();
		    
		    ref = HelperFunctions.getAsBlocks(p).get(0);
		    
		    prog_return subPrimary = subParser.prog();
		    nodeStream = new CommonTreeNodeStream(subPrimary.getTree());
		    walker = new UpdateTreeWalker(nodeStream);
		    sub = HelperFunctions.getAsBlocks(walker.prog()).get(0);
		    
	}

	@Test
	public void test() throws Z3Exception {
//		ref.rename("ref");
//		for(AssignmentStatement s: ref.body){
//			System.out.println(s);
//		}
//		ref.computeConstraintsAsRef();
//		VCGen v = new VCGen(ref, sub, Expression.getTrue(), null, null);
//		v.prepare();
//		Expression equiv = v.getEquiv();
//		System.out.println(equiv);
//		Solver sol = v.testExpression(equiv);
//		System.out.println(v.getEquivSMT());
//		System.out.println(sol.getModel().toString());
	}
	
	@Test
	public void testSimplify(){
		Logger ErrorLogger = new Logger(errLog);
		InitSummary refSum = new InitSummary(progName, ErrorLogger, null);
		refSum.parse();
		ProgramInputs p = new ProgramInputs();
		p.readInputConstriantsMGCRNK();
		
		AssignmentStatement simplified = 
				HelperFunctions.simplify((LoopStatement) refSum.allInits.get(0));
		System.out.println(simplified);
		Map<String, String> valuation = new HashMap<String,String>();
		valuation.put("ref0", "-3");
		valuation.put("dp_ProgInput_2", "0");
		valuation.put("ref1", "-562001673");
		valuation.put("ref1Prime", "1");
		valuation.put("sub1Prime", "1");
		valuation.put("sub1", "1");
		
		HelperFunctions.evaluateExpression(valuation, expr);
	}

//	public static void main(String[] args) {
////		String expression = "(and (= i (- N 1)) (= j (+ (- 1) N)))";
//		String expression = "(declare-const int a) (declare-const int b) (assert (+ a b))";
//		Context ctx;
//		HashMap<String, String> cfg = new HashMap<String, String>();
//    	cfg.put("model", "true");
//		ctx = new Context(cfg);
//		BoolExpr testExpr = ctx.parseSMTLIB2String(expression, null, null, null, null);
//		System.out.println(testExpr.toString());
////		System.out.println(SimplifyPrint.getInfixExpr(testExpr));
//	}
}
