package feedback.utils;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import com.microsoft.z3.ApplyResult;
import com.microsoft.z3.BoolExpr;
import com.microsoft.z3.Context;
import com.microsoft.z3.Expr;
import com.microsoft.z3.Goal;
import com.microsoft.z3.Params;
import com.microsoft.z3.Quantifier;
import com.microsoft.z3.Tactic;
import com.microsoft.z3.Z3Exception;

import feedback.thirdparty.SystemCommandExecutor;

public class SimplifyPrint {
	public static final String pythonCommand = "python";
	public static final String simplifierPath = "/home/anirudh/software/z3/build/example.py";
	static Context z3;
	static Tactic elimTermIte, uss, nnf, propIneq, solverSimplify, 
	simplifyTactic, satPrepro, elimAnd, purifyArith, solveEqs, splitClause, skip, trySplit;
	
	public static class SimplifyResult{
		public SimplifyResult(String string, int originalSize,
				int newSize) {
			simplifiedString = string;
			this.originalSize = originalSize;
			this.newSize = newSize;
		}
		String simplifiedString;
		int originalSize = 0;
		int newSize = 0;
	}
	
	static {
		try{
			z3 = new Context();
			elimTermIte = z3.mkTactic("elim-term-ite");
			solveEqs = z3.mkTactic("solve-eqs");
			skip = z3.mkTactic("skip");			
			splitClause = z3.mkTactic("split-clause");
			trySplit = z3.orElse(splitClause, skip);
			
			purifyArith = z3.mkTactic("purify-arith");
			elimAnd = z3.mkTactic("elim-and");
			uss = z3.mkTactic("unit-subsume-simplify");
			nnf = z3.mkTactic("nnf");
			propIneq = z3.mkTactic("propagate-ineqs");
			solverSimplify = z3.mkTactic("ctx-solver-simplify");
			satPrepro = z3.mkTactic("sat-preprocess");
			simplifyTactic = z3.then(propIneq, solverSimplify, nnf, trySplit, propIneq, solverSimplify);
		}catch (Z3Exception e){
			e.printStackTrace();
		}
	}
	
	public static SimplifyResult simplifiedExpressionAsString(Expression exp){
		if("true".compareTo(exp.toString()) == 0 
				|| "false".compareTo(exp.toString()) == 0)
			return new SimplifyResult(exp.toString(), 1, 1);
		String s = HelperFunctions.getSMTFormula(exp);
		// We cannot simplify the guards true and false further
		
//		System.out.println("===============");
		int originalSize = Expression.getAstSize(exp);
//		System.out.println("Simplified " + exp + " of size " + originalSize);
		SimplifyResult res = null;
		try {
			res = obtainSimplified(s, Expression.getAstSize(exp));
		} catch (Z3Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		if("".compareTo(res.simplifiedString) == 0)
			return new SimplifyResult(exp.toString(), originalSize, originalSize);		
		return res;
	}
	
	public static String simplifyExpression(String formula){
		List<String> commands = new ArrayList<String>();
	    commands.add("python");
	    commands.add("/home/anirudh/software/z3/build/example.py");
	    commands.add(formula);

	    // execute the command
	    SystemCommandExecutor commandExecutor = new SystemCommandExecutor(commands);
	    int result = 0;
		try {
			result = commandExecutor.executeCommand();
		} catch (IOException | InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

	    // get the stdout and stderr from the command that was run
	    StringBuilder stdout = commandExecutor.getStandardOutputFromCommand();
	    StringBuilder stderr = commandExecutor.getStandardErrorFromCommand();
	    
	    // print the stdout and stderr
	    System.out.println("The numeric result of the command was: " + result);
	    System.out.println("STDOUT:");
	    System.out.println(stdout);
	    System.out.println("STDERR:");
	    System.out.println(stderr);
		return stdout.toString();
	}
	
	public static SimplifyResult obtainSimplified(String formula, int originalSize) throws Z3Exception{
		
		if(formula.contains("max(")){
			formula += "(define-fun max ((x Int) (y Int)) Int\n"+
					"(ite (< x y) y x))\n";
		}
		BoolExpr expr;
		
		if(!HelperFunctions.tryParse(formula)){
			return new SimplifyResult(formula, -1, -1);
		}
		
		try {
			expr = z3.parseSMTLIB2String(formula, null, null, null, null);
		} catch (Z3Exception e) {
			// TODO Auto-generated catch block
			System.out.println(formula);
			e.printStackTrace();
			return new SimplifyResult(formula, -1, -1);
		}
		Goal g = z3.mkGoal(true, false, false);
		g.add(expr);		
		ApplyResult apply = simplifyTactic.apply(g);
		int newSize = getAstSize(apply);
//		String s = getStringRepresentation(apply);	
		String s = getInfixExpr(apply);
//		System.out.println("To " + s + " of size " + newSize);
//		System.out.println("===============");
		g.dispose();
		apply.dispose();
		return new SimplifyResult(s, originalSize, newSize);
	}

	public static String getStringRepresentation(ApplyResult apply) throws Z3Exception {
		Goal[] subgoals = apply.getSubgoals();
		int size = subgoals.length; 
		if(size == 0)
			return "false";
		if(size == 1)
			return getStringRepresentation(subgoals[0]);
		
		StringBuilder result = new StringBuilder();
		result.append("(or");
		for(Goal goal : apply.getSubgoals()){
			assert goal.isPrecise() : "Found under/over approximation of z3 goal";
			result.append(getStringRepresentation(goal));
		}
		result.append(")"); 
		return result.toString();
	}

	public static String getStringRepresentation(Goal goal) throws Z3Exception {
		BoolExpr[] formulas = goal.getFormulas();
		int size = formulas.length;
		if(size == 0)
			return "true";
		if(size == 1)
			return formulas[0].getSExpr();
		StringBuilder result = new StringBuilder();
		result.append("\n (and");
		for(BoolExpr b : formulas){
//			result.append("\n" + b.getSExpr());
			result.append("\n" + getInfixExpr(b));
		}
		result.append(")");
		return result.toString();
	}

	public static int getAstSize(Expr exp) throws Z3Exception{
		if(exp.isVar() || exp.isConst() || exp.isNumeral())
			return 1;
		if(exp.isQuantifier()){
			Quantifier q = (Quantifier) exp;
			return 1 + getAstSize(q.getBody());
		}
//		System.out.println(exp);
		assert exp.isApp() : exp;
		int count = 1;
		for(Expr e: exp.getArgs()){
			count += getAstSize(e);
		}
		return count;
	}	
	
	public static int getAstSize(Goal goal) throws Z3Exception {
		BoolExpr[] formulas = goal.getFormulas();
		int size = formulas.length;
		if(size == 0)
			return 0;
		if(size == 1)
			return getAstSize(formulas[0]);
		int accumulator = size-1;  // standing for the 'and' at this level 
								 // even though we emit only one and,
								 // we count there being n-1 ands for the
								 // comparision to be fair: the original formula
								// would not have ands with more than 2 operators
		
		for(BoolExpr b : formulas){
			accumulator += getAstSize(b);
		}
		return accumulator;
	}
	
	public static int getAstSize(ApplyResult apply) throws Z3Exception {
		Goal[] subgoals = apply.getSubgoals();
		int size = subgoals.length; 
		if(size == 0)
			return 0;
		if(size == 1)
			return getAstSize(subgoals[0]);
		
		int accumulator = size -1; 	//	standing for the "or" at this level;
		for(Goal goal : apply.getSubgoals()){
			accumulator += getAstSize(goal);
		}
		return accumulator;
	}

	public static String getInfixExpr(Expr e) throws Z3Exception{
		String ret = "";
		if (e.isVar() || e.isConst() || e.isNumeral())
			ret = e.getSExpr();
		else if (e.isApp()) {
			String op = "";
			if (e.isAdd()) op = "+";
			else if (e.isAnd()) op = "&&";
			else if (e.isDiv()) op = "/";
			else if (e.isIDiv()) op = "/";
			else if (e.isEq()) op = "==";
			else if (e.isGE()) op = ">=";
			else if (e.isGT()) op = ">";
			else if (e.isIff()) op = "<=>";
			else if (e.isImplies()) op = "=>";
			else if (e.isLE()) op = "<=";
			else if (e.isLT()) op = "<";
			else if (e.isMul()) op = "*";
			else if (e.isNot()) op = "!";
			else if (e.isOr()) op = "||";
			else if (e.isSub()) op = "-";
			else if (e.isUMinus()) op = "-";
			for(Expr argE: e.getArgs()){
				if (ret.isEmpty() || ret == "") {
					if (argE.isVar() || argE.isConst() || argE.isNumeral())
						if (e.isNot() || e.isUMinus())
							ret = op + getInfixExpr(argE);
						else
							ret = getInfixExpr(argE);
					else
						if (e.isNot() || e.isUMinus())
							ret = op + "(" + getInfixExpr(argE) + ")";
						else
							ret = "(" + getInfixExpr(argE) + ")";
				} else {
					if (argE.isVar() || argE.isConst() || argE.isNumeral())
						ret = ret + " " + op + " " + getInfixExpr(argE);
					else
						ret = ret + " " + op + " (" + getInfixExpr(argE) + ")";
				}
			}
		}
		return ret;
	}
	
	public static String getInfixExpr(Goal goal) throws Z3Exception {
		BoolExpr[] formulas = goal.getFormulas();
		int size = formulas.length;
		if(size == 0)
			return "true";
		if(size == 1)
			return getInfixExpr(formulas[0]);
		String ret = "";
		for(BoolExpr b : formulas){
			if (ret.isEmpty() || ret.equals(""))
				ret = getInfixExpr(b);
			else
				ret = ret + " && " + getInfixExpr(b);
		}
		return ret;
	}
	
	public static String getInfixExpr(ApplyResult apply) throws Z3Exception {
		Goal[] subgoals = apply.getSubgoals();
		int size = subgoals.length; 
		if(size == 0)
			return "false";
		if(size == 1)
			return getInfixExpr(subgoals[0]);
		
		String ret = "";
		for(Goal goal : apply.getSubgoals()){
			if (ret.isEmpty() || ret.equals(""))
				ret = getInfixExpr(goal);
			else
				ret = ret + " || " + getInfixExpr(goal);
		}
		return ret;
	}
}
