package feedback.inputs;

public class InputVar {
	public String inputVarName;
	public String lowerBound, upperBound;
	public boolean isArrayVar;
	
	public InputVar(String name, String lower, String upper) {
		isArrayVar = false;
		inputVarName = name;
		lowerBound = lower;
		upperBound = upper;
	}
	
	public InputVar(boolean array, String name, String lower, String upper) {
		isArrayVar = array;
		inputVarName = name;
		lowerBound = lower;
		upperBound = upper;
	}
}
