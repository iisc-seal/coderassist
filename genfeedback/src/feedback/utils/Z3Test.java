package feedback.utils;

import static org.junit.Assert.*;

import org.junit.BeforeClass;
import org.junit.Test;

import com.microsoft.z3.ApplyResult;
import com.microsoft.z3.ArithExpr;
import com.microsoft.z3.BoolExpr;
import com.microsoft.z3.Context;
import com.microsoft.z3.Expr;
import com.microsoft.z3.Goal;
import com.microsoft.z3.Solver;
import com.microsoft.z3.Tactic;
import com.microsoft.z3.Z3Exception;

public class Z3Test {

	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
	}

	@Test
	public void test() throws Z3Exception {
		Context z3 = new Context();
		Expr i = z3.mkIntConst("i");
		Expr j = z3.mkIntConst("j");
		BoolExpr c1 = z3.mkGe((ArithExpr) i, z3.mkInt(0));
		BoolExpr c2 = z3.mkLe((ArithExpr) i, z3.mkInt(0));
		BoolExpr c3 = z3.mkGe((ArithExpr) j, z3.mkInt(0));
		BoolExpr c4 = z3.mkNot(z3.mkEq(j, z3.mkInt(0)));
		BoolExpr guard = z3.mkAnd(c1, c2, c3, c4);
		
		Tactic elimAnd = z3.mkTactic("nnf");
		Tactic propValues = z3.mkTactic("qflia");
		Tactic propIneq = z3.mkTactic("propagate-ineqs");
		Tactic simplify = z3.mkTactic("der");
		Tactic simplifyTactic = z3.then(propIneq, simplify);
		
		Goal g = z3.mkGoal(true, false, false);
		g.add(guard);		
		ApplyResult apply = simplifyTactic.apply(g);
		System.out.println(SimplifyPrint.getStringRepresentation(apply));
		try{
			BoolExpr f = z3.parseSMTLIB2String("(assert (and tru false false))", null, null, null, null);
		}catch (Z3Exception e){
			z3 = new Context();
			
		}
		try{
		z3.parseSMTLIB2String("(assert (and true false false))", null, null, null, null);
		}catch (Z3Exception e){
		System.out.println(e);
			
		}
	}
}
