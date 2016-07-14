package feedback.utils;

import java.util.List;
import java.util.Map;

import feedback.logging.Logger;
import feedback.utils.Expression.ExprType;
import feedback.ResultStats;

public class Variable {

	String canonicalName;
	String realName;
	String baseType;
	String dim1;
	String dim2;
	boolean isArray;
	int dimension;
	LoopHeaderVariable outer;
	LoopHeaderVariable inner;
	
	public Variable(String canonicalName, String realName, String baseType,
			String dim1, String dim2, LoopHeaderVariable outer, LoopHeaderVariable inner) {
		super();
		this.canonicalName = canonicalName;
		this.realName = realName;
		this.baseType = baseType;
		this.dim1 = dim1;
		this.dim2 = dim2;
		this.isArray = baseType.contains("Array");
		this.dimension = "".compareTo(dim1) != 0 ? ("".compareTo(dim2) != 0? 2: 1) : 0;
		this.outer = outer;
		this.inner = inner;
	}
	
	public String getCanonicalName() {
		return canonicalName;
	}

	public String getRealName() {
		return realName;
	}

	public String getBaseType() {
		return baseType;
	}

	public String getDim1() {
		return dim1;
	}

	public String getDim2() {
		return dim2;
	}

	public boolean isArray() {
		return isArray;
	}

	public int getDimension() {
		return dimension;
	}
	
	public ResultStats getFeedback(Variable other, Logger feedbackLogger, Expression inputConstraint, 
			Map<String, Variable> submissionProgramVariables){
		boolean perfect = true;
		int numReplace = 0;
		if(canonicalName.compareTo(other.getCanonicalName()) != 0){
			assert false: "Should not be comparing variables with different " +
					"canonical names " + canonicalName + " " +
					other.getCanonicalName();
					;
		}
//		feedbackLogger.writeLog("Matching " + other.canonicalName + " i.e., " + other.realName);
		List<String> list = TypeSummary.typeHeirarchy.get(baseType);
		if(list == null || !list.contains(other.getBaseType())){ 
			feedbackLogger.writeLog("For variable " + other.realName);
			feedbackLogger.writeLog("Expected type: " + baseType);
			feedbackLogger.writeLog("Obtained type: " + other.getBaseType());
//			perfect = false;
			numReplace++;
		}
		
		if(dimension != other.getDimension()){
			feedbackLogger.writeLog("For variable " +  other.realName);
			feedbackLogger.writeLog("Expected dimension: " + dimension);
			feedbackLogger.writeLog("Obtained dimension: " + other.getDimension());
			perfect = false;
			numReplace++;
		}
		else{
			if("".compareTo(dim1) != 0){
				Expression refDim = HelperFunctions.parseAsExpression(dim1);
				Expression subDim = HelperFunctions.parseAsExpression(other.dim1);
				Expression constraint = Expression.conjunct(inputConstraint, 
						new Expression(ExprType.BINARY, "<", subDim, refDim, ""));
				if(HelperFunctions.checkSAT(constraint)){
					feedbackLogger.writeLog("For variable " +  other.realName);
					feedbackLogger.writeLog("Expected size for dim 1 is atleast " + 
							HelperFunctions.rewriteRepair(dim1, submissionProgramVariables));
					feedbackLogger.writeLog("Obtained size for dim 1: " + 
							HelperFunctions.rewriteRepair(other.getDim1(), submissionProgramVariables));
					numReplace++;
//					perfect = false;
				}
			}
			if("".compareTo(dim2) != 0){
				Expression refDim = HelperFunctions.parseAsExpression(dim2);
				Expression subDim = HelperFunctions.parseAsExpression(other.dim2);
				Expression constraint = Expression.conjunct(inputConstraint, 
						new Expression(ExprType.BINARY, "<", subDim, refDim, ""));
				if(HelperFunctions.checkSAT(constraint)){
					feedbackLogger.writeLog("For variable " +  other.realName);
					feedbackLogger.writeLog("Expected size for dim 2 is atleast " + 
							HelperFunctions.rewriteRepair(dim1, submissionProgramVariables));
					feedbackLogger.writeLog("Obtained size for dim 2: " + 
							HelperFunctions.rewriteRepair(other.getDim2(), submissionProgramVariables));
					numReplace++;
//					perfect = false;
				}
			}		
		}
//		if(perfect)
//			feedbackLogger.writeLog("Matched!");
//		else
//			feedbackLogger.writeLog("Fishy!");
		ResultStats typeResult = new ResultStats(perfect, numReplace);
		System.out.println("End of v.getFeedback(): type score: " + typeResult.score);
		System.out.println("Local numReplace = " + numReplace);
		return typeResult;
	}

	@Override
	public String toString() {
		return "Variable [canonicalName=" + canonicalName + ", realName="
				+ realName + ", baseType=" + baseType + ", dim1=" + dim1
				+ ", dim2=" + dim2 + ", isArray=" + isArray + ", dimension="
				+ dimension + ", outer=" + outer + ", inner=" + inner + "]";
	}
	
	
	
}
