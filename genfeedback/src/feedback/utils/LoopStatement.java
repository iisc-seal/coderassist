package feedback.utils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import feedback.logging.Logger;
import feedback.utils.Expression.ExprType;

public class LoopStatement implements Statement {
	
	public enum Direction {
	    UP, DOWN;

		public String printVisitor() {
			if(this == UP){
//				return ""+1;
				return "+";
			}
//			return ""+-1;
			return "-";
		} 
	}
	
	Direction dir;
	Expression index, lowerBound, upperBound;
	List<Statement> block;
	List<LoopStatement> headers;
	List<AssignmentStatement> body;
	int depth;
	int lineNumberOfHeader;
	
	private int getDepth() {
		return depth;
	}
	
	private List<LoopStatement> getHeaders() {
		return headers;
	}

	public List<LoopStatement> computeHeaders() {
		List<LoopStatement> currentHeaders = new ArrayList<LoopStatement>();
		currentHeaders.add(this);
		for(Statement s: block){
			if(s instanceof LoopStatement)
				currentHeaders.addAll(((LoopStatement)s).computeHeaders());
		}
		return currentHeaders;
	}
	
	public List<AssignmentStatement> computeBody() {
		List<AssignmentStatement> bodyStmts = new ArrayList<AssignmentStatement>();
		for(Statement s: block){
			System.out.println(s);
			if(s instanceof AssignmentStatement)
				bodyStmts.add((AssignmentStatement) s);
			else
				bodyStmts.addAll(((LoopStatement)s).computeBody());
		}
		return bodyStmts;
	}
	
	public LoopStatement(Expression dir, Expression index, Expression lowerBound,
			Expression upperBound, List<Statement> block) {
		super();
//		assert dir.isNumeric();
		this.lineNumberOfHeader = -1;
 		this.dir = dir.printVisitor().compareTo("1") == 0? Direction.UP : Direction.DOWN;
		this.index = index;
		this.lowerBound = lowerBound;
		this.upperBound = upperBound;
		this.block = block;
		depth = getNestingDepth();
		headers = computeHeaders();
		body = computeBody();
		assert(!body.isEmpty());
	}
	
	public LoopStatement(Expression lineNumber, Expression dir, Expression index, Expression lowerBound,
			Expression upperBound, List<Statement> block) {
		super();
//		assert dir.isNumeric();
		if (lineNumber.isNumeric())
			this.lineNumberOfHeader = Expression.getNumericValue(lineNumber);
		else
			this.lineNumberOfHeader = -1;
 		this.dir = dir.printVisitor().compareTo("1") == 0? Direction.UP : Direction.DOWN;
		this.index = index;
		this.lowerBound = lowerBound;
		this.upperBound = upperBound;
		this.block = block;
		depth = getNestingDepth();
		headers = computeHeaders();
		body = computeBody();
		assert(!body.isEmpty());
	}
	
//	public LoopStatement(Expression index, Expression lowerBound,
//			Expression upperBound, List<Statement> block) {
//		this.index = index;
//		this.dir = Direction.UP;
//		this.lowerBound = lowerBound;
//		this.upperBound = upperBound;
//		this.block = block;
//		depth = getNestingDepth();
//		headers = computeHeaders();
//	}

	@Override
	public String toString() {
		StringBuilder s = new StringBuilder();
		s.append("loop(");
//		s.append(dir.printVisitor());
//		s.append(",");
		s.append(index.printVisitor());
		s.append(",");
		s.append(lowerBound.printVisitor());
		s.append(",");
		s.append(upperBound.printVisitor());
		s.append(",");
		s.append(dir.printVisitor());
		s.append(")");
		s.append("{\n");
		for(Statement stmt : block){
			s.append(stmt.toString());
		}
		s.append("}\n");
		return s.toString();
	}
	
	public String toStringFeedback(boolean isOutputBlock) {
		StringBuilder s = new StringBuilder();
		s.append("for(");
		s.append(index.printVisitor());
		s.append(" = ");
		s.append(lowerBound.printVisitor());
		s.append("; ");
		s.append(index.printVisitor());
		if (dir == Direction.UP) {
			s.append(" <= ");
			s.append(upperBound.printVisitor());
			s.append("; ");
			s.append(index.printVisitor());
			s.append("++");
		} else {
			s.append(" >= ");
			s.append(upperBound.printVisitor());
			s.append("; ");
			s.append(index.printVisitor());
			s.append("--");
		}
		s.append(")");
		s.append("{\n");
		for(Statement stmt : block){
			s.append(stmt.toStringFeedback(isOutputBlock));
		}
		s.append("}\n");
		return s.toString();
	}

	public String toStringHeader() {
		StringBuilder s = new StringBuilder();
		s.append("loop(");
		s.append(dir.printVisitor());
		s.append(",");
		s.append(index.printVisitor());
		s.append(",");
		s.append(lowerBound.printVisitor());
		s.append(",");
		s.append(upperBound.printVisitor());
		s.append(")");
//		s.append("{\n");
//		for(Statement stmt : block){
//			s.append(stmt.toString());
//		}
//		s.append("}\n");
		return s.toString();
	}

	public String toStringHeaderC(boolean printLineNumber) {
		StringBuilder s = new StringBuilder();
		s.append("for(");
		s.append(index.printVisitor());
		s.append(" = ");
		s.append(lowerBound.printVisitor());
		s.append("; ");
		s.append(index.printVisitor());
		if (dir == Direction.UP) {
			s.append(" <= ");
			s.append(upperBound.printVisitor());
			s.append("; ");
			s.append(index.printVisitor());
			s.append("++");
		} else {
			s.append(" >= ");
			s.append(upperBound.printVisitor());
			s.append("; ");
			s.append(index.printVisitor());
			s.append("--");
		}
		s.append(")");
		if (printLineNumber) {
			s.append(" at line ");
			s.append(lineNumberOfHeader);
		}
		return s.toString();
	}
	
	public Direction getDirection(){
		return dir;
	}
	
	public Expression getIndex() {
		return index;
	}

	public Expression getLowerBound() {
		return lowerBound;
	}

	public Expression getUpperBound() {
		return upperBound;
	}

	public List<Statement> getBlock() {
		return block;
	}

	public int getNestingDepth(){
		int depth = 1;
		for(Statement s: block){
			if(s instanceof LoopStatement)
				depth++;
		}
		return depth;
	}
	
	// does 'this' represent as many nested loops as 'other'
	public boolean isNestingDepthSame(LoopStatement other){
		if(getNestingDepth() == other.getNestingDepth())
			return true;
		
		return false;
	}
	
	public Set<Expression> getIndexVariables(){
		Set<Expression> iVars = new HashSet<Expression>();
		iVars.add(this.getIndex());
		for(Statement s: block){
			if(s instanceof LoopStatement)
				iVars.add(((LoopStatement) s).getIndex());
		}
		
			return iVars;
	}
	
	// get the body of a loop nest
	public List<Statement> getBody(){
		LoopStatement l = this;
		while(l.getBlock().get(0) instanceof LoopStatement)
			l = (LoopStatement) l.getBlock().get(0);
		return l.getBlock();
	}
	
	public int reconcile(Map<Expression, Expression> correspondence, 
			LoopStatement other, 
			Logger feedbackLogger){
		List<LoopStatement> otherHeaders;
		otherHeaders = other.getHeaders();
		int score = 0;
		for(int i = 0; i < headers.size(); i++){
			LoopStatement tis = headers.get(i);
			LoopStatement otr = otherHeaders.get(i);
			score += extractCorrespondenceForCurrentHeader(correspondence, i, tis, otr, feedbackLogger);
		}
		return score;
	}

	public int extractCorrespondenceForCurrentHeader(
			Map<Expression, Expression> correspondence, int i,
			LoopStatement tis, LoopStatement otr, Logger feedbackLogger) {
		int score = 0;
		if(!tis.index.equalsVisitor(otr.index)){
			printFeedbackMessage(tis.index, otr.index, i, "Index", feedbackLogger);
		}
		
		if(!tis.lowerBound.equalsVisitor(otr.lowerBound)){
			printFeedbackMessage(tis.lowerBound, otr.lowerBound, i, "Lower Bound", feedbackLogger);
			score++;
		}
		
		if(!tis.upperBound.equalsVisitor(otr.upperBound)){
			printFeedbackMessage(tis.upperBound, otr.upperBound, i, "Upper Bound", feedbackLogger);
			score++;
		}
		
		correspondence.put(tis.index, otr.index);

//		detectConflictOrUpdateCorrespondence(correspondence, tis.lowerBound, otr.lowerBound);
//		detectConflictOrUpdateCorrespondence(correspondence, tis.upperBound, otr.upperBound);	
		return score;
	}

	public void detectConflictOrUpdateCorrespondence(
			Map<Expression, Expression> correspondence, Expression tis,
			Expression otr) {
		if(correspondence.containsKey(tis)){
			if(!correspondence.get(tis).equalsVisitor(otr)){
				if(!(tis.EType == ExprType.CONST && otr.EType == ExprType.CONST))
				assert false : "A lowerbound reused by this, " +
						"but mapped to distinct other lower bounds by other" +
						correspondence + " vs " + tis + "->" + otr;
			}
		}
//		else
//			correspondence.put(tis, otr);
	}
	
	private void printFeedbackMessage(Expression tis, Expression otr,
			int i, String errorString, Logger feedbackLogger) {
		feedbackLogger.writeLog(errorString +" in loop header mismatch at depth " + (i+1));
		feedbackLogger.writeLog("Expected "+tis.printVisitor());
		feedbackLogger.writeLog("Got "+otr.printVisitor());
	}

	public Set<Expression> variableVisitor(){
		Set<Expression> vars = new HashSet<Expression>();
		for(Statement stmt : block){
			vars.addAll(stmt.variableVisitor());
		}
		return vars;
		
	}

	public int match(LoopStatement other) {
		List<LoopStatement> otherHeaders;		
		if(depth != other.getDepth())
			return 5;
		otherHeaders = other.getHeaders();

		for(int i = 0; i < headers.size(); i++){
			LoopStatement tis = headers.get(i);
			LoopStatement otr = otherHeaders.get(i);
			if(tis.dir != otr.dir){
				return 5;
			}
		}
		return 0;
	}

	
	
}
