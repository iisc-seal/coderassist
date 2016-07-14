package feedback.output;

import feedback.utils.Expression;

public class OutputStatement {
	Expression guard;
	Expression output;
	boolean newline;
	
	public OutputStatement(Expression guard, Expression output, boolean newline) {
		super();
		this.guard = guard;
		this.output = output;
		this.newline = true;
	}
		
	public Expression getGuard() {
		return guard;
	}

	public Expression getOutput() {
		return output;
	}

	public boolean hasNewline() {
		return newline;
	}
	
	@Override
	public String toString() {
		if(guard != null)
			return guard.printVisitor() + ":" + output.printVisitor();
		else
			return output.printVisitor();	
	}
	
	public boolean equals(OutputStatement other){		
		if(guard != null)
			if(guard.equalsVisitor(other.getGuard()) && output.equals(other.getOutput()))
				return true;
		
		if(guard == null)
			if(other.getGuard() == null)
				return output.equalsVisitor(other.getOutput());
		
		return false;  	
	}
		
}
