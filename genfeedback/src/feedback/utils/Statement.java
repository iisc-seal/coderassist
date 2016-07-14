package feedback.utils;

import java.util.Set;

public interface Statement {

	//boolean compare(Statement s2);
	Set<Expression> variableVisitor();
	int getNestingDepth();
	public String toStringFeedback(boolean isOutputBlock);
}
