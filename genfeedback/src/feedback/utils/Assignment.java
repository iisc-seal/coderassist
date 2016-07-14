package feedback.utils;

public class Assignment {
	Expression LHS, RHS;
	private boolean isGlobalInit;

	public Assignment(boolean isGlobal, Expression lHS, Expression rHS) {
		super();
		isGlobalInit = isGlobal;
		LHS = lHS;
		RHS = rHS;
	}

	public Assignment(Expression lHS, Expression rHS) {
		super();
		isGlobalInit = false;
		LHS = lHS;
		RHS = rHS;
	}
	
	public Expression getLHS() {
		return LHS;
	}

	public Expression getRHS() {
		return RHS;
	}
	
	public boolean isGlobal() {
		return isGlobalInit;
	}
	
}
