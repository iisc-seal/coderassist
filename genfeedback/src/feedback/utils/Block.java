package feedback.utils;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import feedback.utils.Expression.ExprType;
import feedback.utils.LoopStatement.Direction;

public class Block {

	List<LoopStatement> header;
	List<AssignmentStatement> body;
	/*typeMap maps summaryVariables to their types*/
	Map<Expression, String> typeMap;
	Map<Expression, Expression> summaryVarToSATVar;
	int depth;
	List<LoopStatement.Direction> directionVector;
	Expression bFormula, iterSpace, guard;
	Set<Expression> iVars, used, updated;
	Map<Expression, Expression> SATVarToSummaryVar;
	Expression fallThroughExpression = null;
	boolean fallThroughExists;
	
	/*
	 * Expects the outermost header of a loop  
	 */
	public Block(LoopStatement s){	
		header = new ArrayList<LoopStatement>();
		body = new ArrayList<AssignmentStatement>();
		header.addAll(s.headers);
		body.addAll(s.body);	
		depth = s.depth;
		directionVector = new ArrayList<LoopStatement.Direction>();
		for(LoopStatement stmt : header){
			directionVector.add(stmt.dir);
		}
		fallThroughExists = false;
	}
	
	@Override
	public String toString() {
		StringBuilder sb = new StringBuilder();
		if(header!=null)			
				sb.append(header.get(0).toString());
		else
			for(AssignmentStatement stmt: body)
				sb.append(stmt.toString() + "\n");
		return sb.toString();
	}

	public String toStringFeedback(boolean isOutputBlock) {
		StringBuilder sb = new StringBuilder();
		if(header!=null)			
				sb.append(header.get(0).toStringFeedback(isOutputBlock));
		else
			for(AssignmentStatement stmt: body)
				sb.append(stmt.toStringFeedback(isOutputBlock) + "\n");
		return sb.toString();
	}
	
	public String toStringFeedbackHeader() {
		StringBuilder sb = new StringBuilder();
		if (header != null) {
			LoopStatement l = header.get(0);
			sb.append(l.toStringHeaderC(false));
			sb.append(" {\n");
			while (!l.block.isEmpty()) {
				Statement s = l.block.get(0);
				if (s instanceof LoopStatement) {
					l = (LoopStatement) s;
					sb.append(l.toStringHeaderC(false));
					sb.append(" {\n");
				} else
					break;
			}
		}
		return sb.toString();
	}
	
	/*
	 * Expects a list of guarded assignments  
	 */
	public Block(List<AssignmentStatement> s){	
		body = new ArrayList<AssignmentStatement>();
		body.addAll(s);
		depth = 0;
		directionVector = new ArrayList<LoopStatement.Direction>();
	}
	
	public void renameIndices(Map<String, String> remappedIndices){
		HelperFunctions.renameIndices(header, remappedIndices);
		List<AssignmentStatement> renamed = new ArrayList<AssignmentStatement>();
		for (AssignmentStatement stmt: body){
        	AssignmentStatement rn = stmt.clone();
        	rn.renameIdentifiers(remappedIndices);
        	renamed.add(rn);
		}
		body.clear();
		body.addAll(renamed);
	}
	
	/*
	 * This method should populate summaryVarToSATVar 
	 */
	public void rename(String basename){
		summaryVarToSATVar = HelperFunctions.getNewNames(body, basename);
		SATVarToSummaryVar = HelperFunctions.invert(summaryVarToSATVar);
		List<AssignmentStatement> renamed = new ArrayList<AssignmentStatement>();
		for (AssignmentStatement stmt: body){
        	AssignmentStatement rn = stmt.clone();
        	rn.renameVisitor(summaryVarToSATVar);
//        	rn.renameLhsInRhs();
        	renamed.add(rn);
		}
		body.clear();
		body.addAll(renamed);
	}
	
	public void makeDisjoint(){
		boolean flag = false;
		for(Expression e : HelperFunctions.getPairwiseConjunctionOfGuards(body)){
			if(HelperFunctions.checkSAT(e))
				flag = true;
				break;
		}
		// Expression conjunctionOfGuards = HelperFunctions.getConjunctionOfGuards(body);
		/*If the guards are not disjoint, make it so!*/
		if(flag){
			List<AssignmentStatement> acc = new ArrayList<AssignmentStatement>();
			AssignmentStatement.makeDisjoint(body, acc);
			body.clear();
			for(AssignmentStatement s:acc){
				// Only add guards that are feasible	
				if(s.guard != null && HelperFunctions.checkSAT(s.guard)){
					body.add(s);
				}
				if(s.guard == null)
					body.add(s);
			}
//			body.addAll(acc);
		}
		
		/*Add a skip statement representing fall through*/
		Expression fallThroughGuard = 
				HelperFunctions.getConjunctionOfNegatedGuards(body);
		if (HelperFunctions.checkSAT(fallThroughGuard)) {
			AssignmentStatement fallThrough = 
				new AssignmentStatement(fallThroughGuard, null, null);
			body.add(fallThrough);
			fallThroughExists = true;
		}
	}
	
	public Expression getFormulaBody(){
		Expression bodyEncoding = null;
		int count = 0;
		for(AssignmentStatement s: body){
			Expression eq = null, lhs = s.getLhs(), rhs = s.getRhs();
			if(lhs == null && rhs == null){
				Expression fallThrough = new Expression(ExprType.BINARY, "=>", 
						s.guard, fallThroughExpression,"");
				bodyEncoding = Expression.conjunct(bodyEncoding, fallThrough);
				count++;
			}
			else{
				eq = new Expression(ExprType.BINARY, "=", lhs, rhs, "");
				Expression encode = new Expression(ExprType.BINARY, "=>", s.guard, eq, "");
				bodyEncoding = Expression.conjunct(bodyEncoding, encode);
			}
		}
//		assert count <= 1 : "only one fall through branch but saw = " + count;
		return bodyEncoding;
	}
	
	public Expression getGuards(){
		if(fallThroughExists)
			return HelperFunctions.getDisjunctionOfGuards(body.subList(0, body.size()-1));
		else
			return HelperFunctions.getDisjunctionOfGuards(body.subList(0, body.size()));
	}
	
	
	
	public Expression getIterSpace(){
		if(header == null)
			return null;
		Expression conjunct = HelperFunctions.getIterSpace(header); 
		conjunct.renameVisitor(summaryVarToSATVar);
		return conjunct;
	}
	
	public Set<Expression> getIVars(){
		iVars = new HashSet<Expression>(); 
		if (header == null)
			return iVars;
		for(LoopStatement s: header){
			iVars.add(s.getIndex());
		}
		return iVars;
	}
	
	public Set<Expression> getUsed(){
		used = new HashSet<Expression>(); 
		for(AssignmentStatement s: body){
			if(s.getRhs() != null) {
				used.addAll(s.getRhs().variableVisitor());
				if (s.guard != null)
					used.addAll(s.guard.variableVisitor());
			}
		}
		return used;
	}
	
	public Set<Expression> getUpdated(){
		updated = new HashSet<Expression>(); 
		for(AssignmentStatement s: body){
			Expression lhs = s.getLhs();
			if(lhs != null)
				updated.add(lhs);
		}
		return updated;
	}
	
	public void rewriteRHS(){
//		Set<Expression> arrayVars = new HashSet<Expression>();
//		for(AssignmentStatement stmt : body)
//			if(stmt.rhs != null && stmt.rhs.arrayExpressionVisitor() != null)
//				arrayVars.addAll(stmt.rhs.arrayExpressionVisitor());
//		arrayVars.addAll(updated);
		for(Expression e: updated){
		  Expression ePrime = (Expression) e.clone();
		  ePrime.setName(e.getName() + "Prime");
		  for(AssignmentStatement s: body){	 
			    if(s.getRhs() != null)
			    	s.getRhs().replace(e, ePrime);
		  }
		  // e.g array[i][j] -> ref1. Need to rename rhs occurrence to ref1Prime
		  // so store arrayPrime[i][j] -> ref1Prime in summaryVarToSatVar
		  // and ref1Prime -> arrayPrime[i][j] in SATVarToSummaryVar
		  // wrt submission, arrayPrime[i][j] -> sub1Prime
		  // sub1Prime -> arrayPrime[i][j]
		  // so ref1Prime -> arrayPrime[i][j] -> sub1Prime on composition
		  Expression orig = SATVarToSummaryVar.get(e);
		  // Note that only array accesses are renamed
		  // if we have i = i + 1, then it is renamed to i = iPrime + 1
		  // and equality constraints are not inferred for i in VCGen subsequently
		  if(orig != null){
			  Expression dummyVar = (Expression) orig.clone();
			  dummyVar.setName(dummyVar.getName() + "Prime");
			  summaryVarToSATVar.put(dummyVar, ePrime);
			  SATVarToSummaryVar.put(ePrime, dummyVar);
			  Expression fallThroughEquivalence = new Expression(ExprType.BINARY, "=", e, ePrime, "");
			  fallThroughExpression = Expression.conjunct(fallThroughExpression, fallThroughEquivalence);
		  }
		}
	}
	
	public void computeConstraintsAsRef(){
		rename("ref");
		makeDisjoint();
		updated = getUpdated();
		rewriteRHS();
		bFormula = getFormulaBody();
		iterSpace = getIterSpace();
		guard = getGuards();
		iVars = getIVars();
		used = getUsed();
	}
	
	public void computeConstraintsAsSubmission(Map<String, String> remappedIndices){
		renameIndices(remappedIndices);
		rename("sub");
		updated = getUpdated();
		rewriteRHS();
		makeDisjoint();
		bFormula = getFormulaBody();
		iterSpace = getIterSpace();
		guard = getGuards();
		iVars = getIVars();
		used = getUsed();
	}
	
}
