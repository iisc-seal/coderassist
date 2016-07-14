package feedback.inputs;

import java.util.ArrayList;
import java.util.List;

import feedback.utils.Expression;
import feedback.utils.Expression.ExprType;

public class ProgramInputs {
	public static List<InputVar> inputs;
	private static Expression inputConstraints = null;
	
	public ProgramInputs() {
		inputs = new ArrayList<InputVar>();
	}
	
	public void readInputConstriantsSUMTRIAN(){
		InputVar var1 = new InputVar("dp_ProgInput_1", "1", "1000");
//		InputVar var2 = new InputVar("dp_ProgInput_2", "1", "100");
		InputVar var2 = new InputVar("dp_ProgInput_2", "1", "99");
		InputVar var3 = new InputVar(true, "dp_ProgInput_3", "0", "99");
	
		inputs.add(var1);
		inputs.add(var2);
		inputs.add(var3);
	}
	
	public void readInputConstriantsPPTEST(){
		InputVar var1 = new InputVar("dp_ProgInput_1", "1", "100");
		InputVar var2 = new InputVar("dp_ProgInput_2", "1", "100");
		InputVar var3 = new InputVar("dp_ProgInput_3", "1", "100");
		InputVar var4 = new InputVar(true, "dp_ProgInput_4", "1", "100");
		InputVar var5 = new InputVar(true, "dp_ProgInput_5", "1", "100");
		InputVar var6 = new InputVar(true, "dp_ProgInput_6", "1", "100");
		
	
		inputs.add(var1);
		inputs.add(var2);
		inputs.add(var3);
		inputs.add(var4);
		inputs.add(var5);
		inputs.add(var6);	
	}
	
	public void readInputConstriantsMGCRNK(){
		InputVar var1 = new InputVar("dp_ProgInput_1", "1", "20");
		InputVar var2 = new InputVar("dp_ProgInput_2", "2", "100");
		InputVar var3 = new InputVar(true, "dp_ProgInput_3", "-2500", "2500");
	
		inputs.add(var1);
		inputs.add(var2);
		inputs.add(var3);	
	}
	
	public void readInputConstraintsMARCHA1() {
		InputVar var1 = new InputVar("dp_ProgInput_1", "0", "99");
		InputVar var2 = new InputVar("dp_ProgInput_2", "0", "20");
		InputVar var3 = new InputVar("dp_ProgInput_3", "0", "20000");
		InputVar var4 = new InputVar(true, "dp_ProgInput_4", "0", "1000");
		
		// aqfaridi/1748023.ac.cpp
//		InputVar non = new InputVar("non", "0", "20");
//		InputVar amount = new InputVar("amount", "0", "20000");
		
		inputs.add(var1);
		inputs.add(var2);
		inputs.add(var3);
		inputs.add(var4);
//		inputs.add(amount);
//		inputs.add(non);
	}
	
	public static InputVar getInputDetails(String varName) {
		for (InputVar i: inputs) {
			if (i.inputVarName.contentEquals(varName))
				return i;
		}
		
		return null;
	}

	public static Expression getAsConstraint(){
		if(inputConstraints != null)
			return inputConstraints;
		Expression constraint = null;
		for (InputVar ip: inputs) {
			if (ip.isArrayVar) {
			} else {
				Expression id = new Expression(ExprType.ID, "", ip.inputVarName);
				Expression lowerBound = new Expression(ExprType.CONST, "", ip.lowerBound);
				Expression upperBound = new Expression(ExprType.CONST, "", ip.upperBound);
				Expression lbc = new Expression(ExprType.BINARY, ">=", id, lowerBound, ""); 
				Expression ubc = new Expression(ExprType.BINARY, "<=", id, upperBound, "");
				constraint = Expression.conjunct(constraint, lbc);
				constraint = Expression.conjunct(constraint, ubc);
			}
		}
		inputConstraints  = constraint;
		return constraint;
	}
	
	public String readInputConstraints(String refString) {
		if(refString.contains("MARCHA1")){
			readInputConstraintsMARCHA1();
			return "MARCHA1";
		}
		else if(refString.contains("SUMTRIAN")){
			readInputConstriantsSUMTRIAN();
			return "SUMTRIAN";
		}
		else if(refString.contains("PPTEST")){
			readInputConstriantsPPTEST();
			return "PPTEST";
		}
		else if(refString.contains("MGCRNK")){
			readInputConstriantsMGCRNK();
			return "MGCRNK";
		}
		return "SUMTRIAN";
	}
}
