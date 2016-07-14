package feedback.update;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ArrayBlockingQueue;

import org.antlr.runtime.ANTLRStringStream;

import com.microsoft.z3.ApplyResult;
import com.microsoft.z3.BoolExpr;
import com.microsoft.z3.Context;
import com.microsoft.z3.Expr;
import com.microsoft.z3.FuncDecl;
import com.microsoft.z3.Goal;
import com.microsoft.z3.IntExpr;
import com.microsoft.z3.Model;
import com.microsoft.z3.RatNum;
import com.microsoft.z3.Solver;
import com.microsoft.z3.Sort;
import com.microsoft.z3.Status;
import com.microsoft.z3.Symbol;
import com.microsoft.z3.Tactic;
import com.microsoft.z3.Z3Exception;

import feedback.utils.AssignmentStatement;
import feedback.utils.Expression;
import feedback.utils.Expression.ExprType;
import feedback.utils.HelperFunctions;
import feedback.utils.LoopStatement;
import feedback.utils.Statement;

public class SATTest {
	
	@SuppressWarnings("serial")
    class TestFailedException extends Exception
    {
        public TestFailedException()
        {
            super("Check FAILED");
        }
    };
	
    static Model check(Context ctx, BoolExpr f, Status sat) throws Z3Exception,
    TestFailedException
    {
    	Solver s = ctx.mkSolver();
    	s.add(f);
    	if (s.check() != sat){
    		System.out.println("UNSAT!");System.out.println("SAT!");
    		return null;
    	}
    	if (sat == Status.SATISFIABLE)
    		return s.getModel();
    	else
    		return null;
    }
    
    void bigIntCheck(Context ctx, RatNum r) throws Z3Exception
    {
        System.out.println("Num: " + r.getBigIntNumerator());
        System.out.println("Den: " + r.getBigIntDenominator());
    }
    
    ApplyResult applyTactic(Context ctx, Tactic t, Goal g) throws Z3Exception
    {
        System.out.println("\nGoal: " + g);

        ApplyResult res = t.apply(g);
        System.out.println("Application result: " + res);

        Status q = Status.UNKNOWN;
        for (Goal sg : res.getSubgoals())
            if (sg.isDecidedSat())
                q = Status.SATISFIABLE;
            else if (sg.isDecidedUnsat())
                q = Status.UNSATISFIABLE;

        switch (q)
        {
        case UNKNOWN:
            System.out.println("Tactic result: Undecided");
            break;
        case SATISFIABLE:
            System.out.println("Tactic result: SAT");
            break;
        case UNSATISFIABLE:
            System.out.println("Tactic result: UNSAT");
            break;
        }

        return res;
    }
    
	static String ref = "UPDATE:" +
	"loop(1, i, 0, dp_ProgInput2){" +
	"loop(1, j, 0, dp_ProgInput3){" +
	"(j >= a[i-1]): dp[i][j] = dp[i-1][j] || dp[i-1][j-a[i-1]];"+
	"(j < a[i-1]): dp[i][j] = dp[i-1][j];" +
	"}}";
	

	static String submission = "UPDATE:" +
			"loop(1, i, 0, dp_ProgInput2){" +
			"loop(1, j, 0, dp_ProgInput3){" +
	"((j >= a[i-1]) && dp[i-1][j]): dp[i][j] = true;" +
	"((j >= a[i-1]) && dp[i-1][j-a[i-1]]): dp[i][j] = true;" +
	"((j >= a[i-1]) && !dp[i-1][j] && !dp[i-1][j-a[i-1]]): dp[i][j] = false;" +
	"(j < a[i-1]): dp[i][j] = dp[i-1][j];" +
	"}}";

	static String f1 = "(declare-const j0 Int)" +
			"(declare-const a0 Int)" +
			"(declare-const x1 Bool)" +
			"(declare-const x2 Bool)" +
			"(declare-const y0 Bool)" +
			"(declare-const z0 Bool)" +
			"(assert (=> (>= j0 a0) (= x1 (or y0 z0))))" +
			"(assert (=> (< j0 a0) (= x1 y0)))" +
			"(assert (=> (and (>= j0 a0) y0) (= x2 true)))" +
			"(assert (=> (and (>= j0 a0) z0) (= x2 true)))" +
			"(assert (=> (and (>= j0 a0) (and (not y0) (not z0))) (not x2)))" +
			"(assert (=> (< j0 a0) (= x2 y0)))" +
			"(assert (not (= x1 x2)))";
			
	static String f2 = "(declare-const a Bool)" +
			"(declare-const b Bool)" +
			"(define-fun demorgan () Bool" +
			"(= (and a b) (not (or (not a) (not b)))))" +
			"(assert (not demorgan))";
	
	static String f3 = "(benchmark tst :extrafuns ((x Int) (y Int)) :formula (> x y) :formula (> x 0))";
	
	public static void main(String[] args) {
		ANTLRStringStream refStream = new ANTLRStringStream(ref);
		List<Statement> refs = UpdateSummary.parse(refStream);
		
		ANTLRStringStream subStream = new ANTLRStringStream(submission);
		List<Statement> subs = UpdateSummary.parse(subStream);
		
		LoopStatement rHeader = (LoopStatement) refs.get(0);
		LoopStatement sHeader = (LoopStatement) subs.get(0);
		
		Map<Expression, Expression> reconcile = rHeader.reconcile(sHeader);
		
		List<Statement> refStmts = rHeader.getBody();
		List<Statement> subStmts = sHeader.getBody();
		
		Expression refLhs = UpdateDriver.assertSameLHS(refStmts);
		Expression subLhs = UpdateDriver.assertSameLHS(subStmts);
		
		if(!HelperFunctions.testIdentity(reconcile)){
			renameIdentifiers(subStmts, reconcile);
		}
		
		Map<Expression, Expression> refBindings = getNewNames(refStmts, "ref");
		Map<Expression, Expression> subBindings = getNewNames(subStmts, "sub");
		
		
		// compute map to rename the submission's variables in a way 
		// consistent with the ref; only leave out the lhs
		for (Map.Entry<Expression, Expression> entry : subBindings.entrySet()) {
			  Expression key = entry.getKey();
			  Expression value = entry.getValue();
			  if(refBindings.containsKey(key) 
					  && key.equalsVisitor(refLhs) == false){
				  entry.setValue(refBindings.get(key));
			  }
		}
		
		// perform the renaming
		renameStatements(refStmts, refBindings);
		renameStatements(subStmts, subBindings);
		
		// get types
		Map<Expression, String> refTypes = new HashMap<Expression, String>();
		getTypes(refStmts, refTypes);
		
		Map<Expression, String> subTypes = new HashMap<Expression, String>();
		getTypes(subStmts, subTypes);
		
		checkSanity(refTypes, subTypes);
		
		// remove those key, value pairs from subTypes, where the key is already
		// in refTypes. This is so that we don't emit multiple type decls in the
		// SMT formula string
		for (Expression e : refTypes.keySet()) {
			  if(subTypes.containsKey(e))
				  subTypes.remove(e);
		}
		
		// set up SMTLIB2 formulas
		String refStmtFormula = getSMTStringForStatements(refStmts);
		String subStmtFormula = getSMTStringForStatements(subStmts);
		String refStmtDeclFormula = HelperFunctions.getSMTFormula(refTypes);
		String subStmtDeclFormula = HelperFunctions.getSMTFormula(subTypes);
		String equalityConstraint = "(assert (not (= " + refBindings.get(refLhs) 
				+" "+ subBindings.get(subLhs) + ")))";
		String satFormula = refStmtDeclFormula + subStmtDeclFormula +
				refStmtFormula + subStmtFormula + equalityConstraint;
//				
//		for (Statement stmt: subStmts){
//        	System.out.println(stmt);
//        	AssignmentStatement rn = ((AssignmentStatement) stmt).clone();
//        	rn.renameVisitor(subBindings);
//        	System.out.println(rn); 
//        	rn.populateTypes(subTypes);
//        	System.out.println(subTypes);
//        }
		
	
		
		
		
		List<AssignmentStatement> l = new ArrayList<AssignmentStatement>();
		for(Statement s: refStmts){
			l.add((AssignmentStatement) s);
		}
		
		List<AssignmentStatement> acc = new ArrayList<AssignmentStatement>();
		acc.add(l.get(0));
		l.remove(0);
		AssignmentStatement.makeGuardsDisjoint(l, acc);
		System.out.println(acc);
		
		/*test whether the guard is disjoint*/
		Expression guardConjunction = 
				UpdateDriver.getConjunctionOfGuards(refStmts);
		
		guardConjunction.replace(new Expression(ExprType.BINARY, "-", 
				new Expression(ExprType.ID, "", "i"),
				new Expression(ExprType.CONST, "", "1"),
				""), 
				(Expression) guardConjunction.geteList().get(1).clone());
		System.out.println(guardConjunction);
		Set<Expression> variables = ((Expression) guardConjunction.clone()).arrayExpressionVisitor();
		Map<Expression, Expression> renameMap =
				HelperFunctions.renameMap(variables, "grd");
		guardConjunction.renameVisitor(renameMap);
		System.out.println(guardConjunction);
		UpdateDriver.checkSAT(guardConjunction);
		
				
		HashMap<String, String> cfg = new HashMap<String, String>();
        cfg.put("model", "true");
        Context ctx = null;
		try {
			ctx = new Context(cfg);
			SATTest s = new SATTest();
        	//s.basicTests(ctx);
			BoolExpr prop = ctx.parseSMTLIB2String(satFormula, null, null, null, null);
	       
			System.out.println("prop " + prop);

	        @SuppressWarnings("unused")
	        Model m = check(ctx, prop, Status.SATISFIABLE);
	        if(m!=null)
	        	System.out.println(m);
		} catch (Z3Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (TestFailedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		System.out.println("FINIS");
	}

	private static void checkSanity(Map<Expression, String> refTypes,
			Map<Expression, String> subTypes) {
		for (Expression e : refTypes.keySet()) {
			  if(subTypes.containsKey(e))
				  assert(refTypes.get(e) == subTypes.get(e));
		}
	}

	public static void renameIdentifiers(List<Statement> stmts, 
			Map<Expression, Expression> bindings){
		List<Statement> renamed = new ArrayList<Statement>();
		for (Statement stmt: stmts){
        	AssignmentStatement rn = ((AssignmentStatement) stmt).clone();
        	rn.renameIdentifiers(bindings);
        	renamed.add(rn);
		}
		stmts.clear();
		stmts.addAll(renamed);
	}
	
	public static void renameStatements(List<Statement> stmts, 
			Map<Expression, Expression> bindings){
		List<Statement> renamed = new ArrayList<Statement>();
		for (Statement stmt: stmts){
        	AssignmentStatement rn = ((AssignmentStatement) stmt).clone();
        	rn.renameVisitor(bindings);
        	renamed.add(rn);
		}
		stmts.clear();
		stmts.addAll(renamed);
	}
	
	public static void getTypes(List<Statement> stmts, Map<Expression, String> types){
		for (Statement stmt: stmts){
        	AssignmentStatement rn = ((AssignmentStatement) stmt).clone();
        	rn.populateTypes(types);
		}
	}
	
	public static String getSMTStringForStatements(List<Statement> stmts){
		String formula = "";
		for (Statement stmt: stmts){
        	System.out.println(stmt);
        	AssignmentStatement rn = ((AssignmentStatement) stmt);
        	formula += rn.getSMTFormula();
        }
		return formula;
	}

	public static Map<Expression, Expression> getNewNames(
			List<Statement> refStmts, String basename) {
		Set<Expression> vars = new HashSet<Expression>();
		for (Statement stmt: refStmts){
        	System.out.println(stmt);
        	vars.addAll(stmt.variableVisitor());
        	//AssignmentStatement rn = ((AssignmentStatement) stmt).clone();
        	
        }
		System.out.println(vars);
		Map<Expression, Expression> bindings = HelperFunctions.renameMap(vars, basename);
		return bindings;
	}
	
	void basicTests(Context ctx) throws Z3Exception, TestFailedException
    {
        System.out.println("BasicTests");

        Symbol fname = ctx.mkSymbol("f");
        Symbol x = ctx.mkSymbol("x");
        Symbol y = ctx.mkSymbol("y");

        Sort bs = ctx.mkBoolSort();

        Sort[] domain = { bs, bs };
        FuncDecl f = ctx.mkFuncDecl(fname, domain, bs);
        Expr fapp = ctx.mkApp(f, ctx.mkConst(x, bs), ctx.mkConst(y, bs));

        Expr[] fargs2 = { ctx.mkFreshConst("cp", bs) };
        Sort[] domain2 = { bs };
        Expr fapp2 = ctx.mkApp(ctx.mkFreshFuncDecl("fp", domain2, bs), fargs2);

        BoolExpr trivial_eq = ctx.mkEq(fapp, fapp);
        BoolExpr nontrivial_eq = ctx.mkEq(fapp, fapp2);

        Goal g = ctx.mkGoal(true, false, false);
        g.add(trivial_eq);
        g.add(nontrivial_eq);
        System.out.println("Goal: " + g);

        Solver solver = ctx.mkSolver();

        for (BoolExpr a : g.getFormulas())
            solver.add(a);

        if (solver.check() != Status.SATISFIABLE)
            throw new TestFailedException();

        ApplyResult ar = applyTactic(ctx, ctx.mkTactic("simplify"), g);
        if (ar.getNumSubgoals() == 1
                && (ar.getSubgoals()[0].isDecidedSat() || ar.getSubgoals()[0]
                        .isDecidedUnsat()))
            throw new TestFailedException();

        ar = applyTactic(ctx, ctx.mkTactic("smt"), g);
        if (ar.getNumSubgoals() != 1 || !ar.getSubgoals()[0].isDecidedSat())
            throw new TestFailedException();

        g.add(ctx.mkEq(ctx.mkNumeral(1, ctx.mkBitVecSort(32)),
                ctx.mkNumeral(2, ctx.mkBitVecSort(32))));
        ar = applyTactic(ctx, ctx.mkTactic("smt"), g);
        if (ar.getNumSubgoals() != 1 || !ar.getSubgoals()[0].isDecidedUnsat())
            throw new TestFailedException();

        Goal g2 = ctx.mkGoal(true, true, false);
        ar = applyTactic(ctx, ctx.mkTactic("smt"), g2);
        if (ar.getNumSubgoals() != 1 || !ar.getSubgoals()[0].isDecidedSat())
            throw new TestFailedException();

        g2 = ctx.mkGoal(true, true, false);
        g2.add(ctx.mkFalse());
        ar = applyTactic(ctx, ctx.mkTactic("smt"), g2);
        if (ar.getNumSubgoals() != 1 || !ar.getSubgoals()[0].isDecidedUnsat())
            throw new TestFailedException();

        Goal g3 = ctx.mkGoal(true, true, false);
        Expr xc = ctx.mkConst(ctx.mkSymbol("x"), ctx.getIntSort());
        Expr yc = ctx.mkConst(ctx.mkSymbol("y"), ctx.getIntSort());
        g3.add(ctx.mkEq(xc, ctx.mkNumeral(1, ctx.getIntSort())));
        g3.add(ctx.mkEq(yc, ctx.mkNumeral(2, ctx.getIntSort())));
        BoolExpr constr = ctx.mkEq(xc, yc);
        g3.add(constr);
        ar = applyTactic(ctx, ctx.mkTactic("smt"), g3);
        if (ar.getNumSubgoals() != 1 || !ar.getSubgoals()[0].isDecidedUnsat())
            throw new TestFailedException();

        //modelConverterTest(ctx);

        // Real num/den test.
        RatNum rn = ctx.mkReal(42, 43);
        Expr inum = rn.getNumerator();
        Expr iden = rn.getDenominator();
        System.out.println("Numerator: " + inum + " Denominator: " + iden);
        if (!inum.toString().equals("42") || !iden.toString().equals("43"))
            throw new TestFailedException();

        if (!rn.toDecimalString(3).toString().equals("0.976?"))
            throw new TestFailedException();

        bigIntCheck(ctx, ctx.mkReal("-1231231232/234234333"));
        bigIntCheck(ctx, ctx.mkReal("-123123234234234234231232/234234333"));
        bigIntCheck(ctx, ctx.mkReal("-234234333"));
        bigIntCheck(ctx, ctx.mkReal("234234333/2"));

        String bn = "1234567890987654321";

        if (!ctx.mkInt(bn).getBigInteger().toString().equals(bn))
            throw new TestFailedException();

        if (!ctx.mkBV(bn, 128).getBigInteger().toString().equals(bn))
            throw new TestFailedException();

        if (ctx.mkBV(bn, 32).getBigInteger().toString().equals(bn))
            throw new TestFailedException();

        // Error handling test.
        try
        {
            @SuppressWarnings("unused")
            IntExpr i = ctx.mkInt("1/2");
            throw new TestFailedException(); // unreachable
        } catch (Z3Exception e)
        {
        }
    }
	
	
}
