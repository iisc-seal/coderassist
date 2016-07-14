package feedback.utils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import org.antlr.runtime.ANTLRStringStream;
import org.antlr.runtime.CharStream;
import org.antlr.runtime.CommonTokenStream;
import org.antlr.runtime.RecognitionException;
import org.antlr.runtime.TokenStream;
import org.antlr.runtime.tree.CommonTreeNodeStream;

import com.microsoft.z3.BoolExpr;
import com.microsoft.z3.Context;
import com.microsoft.z3.FuncDecl;
import com.microsoft.z3.Model;
import com.microsoft.z3.Params;
import com.microsoft.z3.Solver;
import com.microsoft.z3.Status;
import com.microsoft.z3.Z3Exception;

import feedback.ResultStats;
import feedback.SummaryModule;
import feedback.logging.Logger;
import feedback.update.parser.ExprLexer;
import feedback.update.parser.ExprParser;
import feedback.update.parser.ExpressionTreeWalker;
import feedback.update.parser.ExprParser.startrule_return;
import feedback.utils.Expression.ExprType;
import feedback.utils.SimplifyPrint.SimplifyResult;

public class VCGen {
	static Context ctx = null;
	static Solver sol = null;
	
	static{
		try {
			HashMap<String, String> cfg = new HashMap<String, String>();
			ctx = new Context(cfg);
			Params p = ctx.mkParams();
			p.add("timeout", 3000);
			sol = ctx.mkSolver();
			sol.setParameters(p);
		} catch (Z3Exception e){
			e.printStackTrace();
		}
	}
	
	private static final int THRESHOLD = 10;
	private static final int PARSE_FAIL_PENALTY = 3;
	Block reference;
	Block submission;
	int score;
	Expression inputConstraint;
	// correspondenceMap maps SAT vars from ref to submission
	Map<Expression, Expression> correspondenceMap;
	Logger feedback;
	boolean matched;
	List<SimplifyResult> repairs;
	public static String smtmax = "(define-fun max ((x Int) (y Int)) Int\n"+
			"(ite (< x y) y x))\n";
	
	private final int timeoutPenalty = 4;
	private final int incorrectHeaderPenalty = 3;
	private Map<String, Variable> referenceProgramVariables;
	private Map<String, Variable> submissionProgramVariables;
	private HashMap<String, String> SATVarToSummaryVarAsString;
	private HashMap<String, String> summaryVarToProgramVarAsString;
	private ResultStats rStats;
	List<Expression> fallThroughSet;
	
	public VCGen(Block ref, Block sub, Expression inputConstraint, Logger l, 
			Map<String, Variable> refererenceProgramVariables, 
			Map<String, Variable> submissionProgramVariables){
		reference = ref;
		submission = sub;
		this.inputConstraint = inputConstraint;		
		correspondenceMap = new HashMap<Expression, Expression>();
		score = 0;
		feedback = l;
		matched = true;
		this.referenceProgramVariables = refererenceProgramVariables;
		this.submissionProgramVariables = submissionProgramVariables;
		rStats = new ResultStats(true, 0);
		repairs = new ArrayList<SimplifyResult>();
		fallThroughSet = new ArrayList<Expression>();
	}

	public boolean checkDepthAndDirection(){
		if(reference.depth == submission.depth)
			if(reference.directionVector.equals(submission.directionVector))
				return true;
		
		return false;
	}
	
	public Set<Expression> getArraysReferredTo(Expression guard){
		Set<Expression> arraysReferredTo = new HashSet<Expression>();
		if(guard == null)
			return arraysReferredTo;
		Set<Expression> variables = guard.variableVisitor();
		for(Expression variable : variables){
			if(submission.SATVarToSummaryVar.containsKey(variable)){
				Expression summaryVar = 
						submission.SATVarToSummaryVar.get(variable);
				if(summaryVar.EType == ExprType.ARRAY)
					arraysReferredTo.add(summaryVar);
			}
				
		}
		return arraysReferredTo;
	}
	
	public class Pair<T, U> {         
	    public final T first;
	    public final U second;

	    public Pair(T t, U u) {         
	        this.first= t;
	        this.second= u;
	     }
	 }
	
	/*
	 * For every array accessed in the set
	 * map the array name to (index expression, dimension)
	 * Eg. arr[i-1][j+2] will be stored in the map as arr -> {(i-1,0), (j+2,1)}
	 * We assert if another array or function is used to index into an array
	 * */
	public Map<String, Set<Pair<Expression, Integer>>> getIndexExpressions(Set<Expression> arrayAccesses){
		Map<String, Set<Pair<Expression, Integer>>> arrayNameToIndexExpressions = 
				new HashMap<String, Set<Pair<Expression, Integer>>>();
		for(Expression arr : arrayAccesses){
			Set<Pair<Expression, Integer>> indexExpressions = 
					new HashSet<Pair<Expression, Integer>>();
			int count = 0;
			for(Expression index : arr.eList){
				if(index.EType != ExprType.ARRAY || index.EType != ExprType.FN ||
						index.EType != ExprType.FN)
					indexExpressions.add(new Pair<Expression, Integer>(index, count));
				else{
					assert false: "Unknown index expression!";
				}
				count++;
			}
			String arrName = arr.getName();
			arrName = arrName.replace("Prime", "");
			if(!indexExpressions.isEmpty()){
				if(arrayNameToIndexExpressions.containsKey(arrName)){
					arrayNameToIndexExpressions.get(arrName).addAll(indexExpressions);
				}
				else
					arrayNameToIndexExpressions.put(arrName, indexExpressions);
			}
		}
		return arrayNameToIndexExpressions;
	}
	
	public Set<Expression> getBoundsCondition(AssignmentStatement stmt){
		Set<Expression> constraints = new HashSet<Expression>();	
		Set<Expression> arraysReferredToGuard = new HashSet<Expression>();
		if(SimplifyPrint.simplifiedExpressionAsString(stmt.guard).simplifiedString.contains("sub"))
			arraysReferredToGuard = getArraysReferredTo(stmt.guard);
		Set<Expression> arraysReferredToAssignment = getArraysReferredTo(stmt.lhs);
		arraysReferredToAssignment.addAll(getArraysReferredTo(stmt.rhs));
		
		
		Map<String, Set<Pair<Expression, Integer>>> guardIdxExpr = getIndexExpressions(arraysReferredToGuard);
		Map<String, Set<Pair<Expression, Integer>>> assgnIdxExpr = getIndexExpressions(arraysReferredToAssignment);
		Expression antecedent = Expression.conjunct(reference.getIterSpace(), inputConstraint);
		antecedent = Expression.conjunct(antecedent, stmt.guard);
		getOutOfBoundsConstraints(constraints, guardIdxExpr, antecedent);
		getOutOfBoundsConstraints(constraints, assgnIdxExpr, antecedent);
		return constraints;
	}

	private void getOutOfBoundsConstraints(Set<Expression> constraints,
			Map<String, Set<Pair<Expression, Integer>>> indexExpressionsGuard,
			Expression antecedent) {
		for(Map.Entry<String, Set<Pair<Expression,Integer>>> entry : indexExpressionsGuard.entrySet()){
			String name = entry.getKey();
//			name = name.replace("Prime", "");
			Set<Pair<Expression, Integer>> indexExprs = entry.getValue();
			Variable v = referenceProgramVariables.get(name);
			assert v != null : "Unknown program Variable" + name + referenceProgramVariables;
			
			Expression upper;	
			for(Pair<Expression, Integer> e: indexExprs){
				if(e.second == 0)
					upper = HelperFunctions.parseAsExpression(v.dim1);
				else
					upper = HelperFunctions.parseAsExpression(v.dim2);
//				assert upper != null : "Trouble parsing " + v.dim1 + " " + v.dim2 ;
				if(upper == null){
					constraints.add(Expression.getTrue());
					return;
				}
				// Special handling for range expression in indexExprs
				if (e.first.getOperator().equals("...")) {
					Expression lb = new Expression(ExprType.BINARY, "<=",
							Expression.getConst("0"), e.first.geteList().get(0), "");
					Expression ub = new Expression(ExprType.BINARY, "<=",
							e.first.geteList().get(0), Expression.subtractOne(upper), "");
					Expression consequent = Expression.conjunct(lb, ub);
					constraints.add(Expression.conjunct(antecedent, Expression.negate(consequent)));
					
					lb = new Expression(ExprType.BINARY, "<=",
							Expression.getConst("0"), e.first.geteList().get(1), "");
					ub = new Expression(ExprType.BINARY, "<=",
							e.first.geteList().get(1), Expression.subtractOne(upper), "");
					consequent = Expression.conjunct(lb, ub);
					constraints.add(Expression.conjunct(antecedent, Expression.negate(consequent)));
				} else {
					Expression lb = new Expression(ExprType.BINARY, "<=", 
						Expression.getConst("0"), e.first, "");
					Expression ub = new Expression(ExprType.BINARY, "<=",
							e.first, Expression.subtractOne(upper), "");
					Expression consequent = Expression.conjunct(lb, ub);
					constraints.add(Expression.conjunct(antecedent, Expression.negate(consequent)));				
				}
			}
		}
	}
	
	public boolean checkSameLHSUpdated(){
		Set<Expression> updatedS = submission.getUpdated();
		Set<Expression> updatedR = new HashSet<Expression>();
		for(Expression e: reference.updated){
			if(correspondenceMap.containsKey(e)){
				updatedR.add(correspondenceMap.get(e));
			}
			// An update the reference makes is not made by the submission
			else
				return false;
		}
		if(updatedR.equals(updatedS))
			return true;
		return false;
	}
	
	public Map<String, String> computeSubIndexToRefIndex(){
		Map<String, String> remappedIndices = new HashMap<String, String>();
		if(reference.depth == 0)
			return remappedIndices;
		for(int i = 0; i < submission.header.size(); i++){
			LoopStatement sub = submission.header.get(i);
			LoopStatement ref = reference.header.get(i);
			remappedIndices.put(sub.getIndex().toString(), ref.getIndex().toString());
		}
		return remappedIndices;
	}
	
	public void prepare(){
		submission.computeConstraintsAsSubmission(computeSubIndexToRefIndex());
		for(Entry<Expression, Expression> entry : reference.SATVarToSummaryVar.entrySet()){
			Expression subVar = submission.summaryVarToSATVar.get(entry.getValue());
			if(subVar != null)
				correspondenceMap.put(entry.getKey(), subVar);
		}
		
		SATVarToSummaryVarAsString = new HashMap<String, String>();
		summaryVarToProgramVarAsString = new HashMap<String, String>();
		
		for (Entry<Expression, Expression> entry : reference.SATVarToSummaryVar.entrySet()){
			String SATVarName = entry.getKey().getName();
			SATVarToSummaryVarAsString.put(SATVarName, entry.getValue().toString());
			Expression exp = (Expression) entry.getKey().clone();
			exp.setName(exp.getName().replace("Prime", ""));
			if(correspondenceMap.containsKey(exp)){
				String usedName = correspondenceMap.get(exp).getName();
				Variable v = submissionProgramVariables.get(usedName);
				if(v != null)
					summaryVarToProgramVarAsString.put(entry.getValue().getName(), v.realName);
			}
			
		}
		for (Entry<Expression, Expression> entry : submission.SATVarToSummaryVar.entrySet()){
			String SATVarName = entry.getKey().getName();
			SATVarToSummaryVarAsString.put(SATVarName, entry.getValue().toString());
			String name = entry.getValue().getName().replace("Prime", "");
			Variable v = submissionProgramVariables.get(name);
			if(v != null)
				summaryVarToProgramVarAsString.put(entry.getValue().getName(), v.realName);
		}
		for( Entry<String, Variable>  entry : submissionProgramVariables.entrySet()){
			if(entry.getKey().contains("Input")){
				summaryVarToProgramVarAsString.put(entry.getKey(), entry.getValue().realName);
			}
		}
	}
	
	public String rewriteInTermsOfSummaryVars(String input){
		String rewritten = input;
		for (Entry<String, String> entry : SATVarToSummaryVarAsString.entrySet()){
			rewritten = rewritten.replace(entry.getKey(), entry.getValue());
		}
		return rewritten;
	}
	
	public String rewriteInTermsOfProgramVars(String input){
		String rewritten = rewriteInTermsOfSummaryVars(input);		
		for (Entry<String, String> entry : summaryVarToProgramVarAsString.entrySet()){
			rewritten = rewritten.replace(entry.getKey(), entry.getValue());
		}
		return rewritten;
	}
	
	public Expression getPre(){
		Expression pre = null;
		for(Expression usedR: reference.used){
			Expression usedS = correspondenceMap.get(usedR);
			if(usedS != null){
				Expression eqConstraint = 
						new Expression(ExprType.BINARY, "=", usedR, usedS, "");
				pre = Expression.conjunct(pre, eqConstraint);
			}
		}
		
		/*Equate values of updated variable entering the loop*/
		for(Entry<Expression, Expression> entry : correspondenceMap.entrySet()){
			Expression key = entry.getKey();
			if(key.getName().contains("Prime")){
				Expression eqConstraint = 
						new Expression(ExprType.BINARY, "=", key, entry.getValue(), "");
				pre = Expression.conjunct(pre, eqConstraint);
			}
		}
		
		return pre;
	}
	
	public String getPreSMT(){
		StringBuilder pre = new StringBuilder();
		for(Expression usedR: reference.used){
			Expression usedS = correspondenceMap.get(usedR);
			if(usedS != null)
				pre.append("(assert (= " + usedR + " " + usedS + "))\n");
		}
		for(Entry<Expression, Expression> entry : correspondenceMap.entrySet()){
			Expression key = entry.getKey();
			if(key.getName().contains("Prime")){
				pre.append("(assert (= " + key + " " + entry.getValue() + "))\n");
			}
		}
		return pre.toString();
	}
	
	public Expression getPost(){
		Expression post = null;
//		System.out.println(reference.updated);
//		System.out.println(correspondenceMap);
		for(Expression updatedR: reference.updated){
			Expression updatedS = correspondenceMap.get(updatedR);
			if(updatedS != null){
				Expression eqConstraint = 
						new Expression(ExprType.BINARY, "=", updatedR, updatedS, "");
//				System.out.println(post);
//				System.out.println(eqConstraint);
				post = Expression.conjunct(post, eqConstraint);
			}
		}		
		return Expression.negate(post);
	}
	
	public String getPostSMT(){
		StringBuilder post = new StringBuilder();
		for(Expression updatedR: reference.updated){
			Expression updatedS = correspondenceMap.get(updatedR);
			post.append("(assert (not (= " + updatedR + " " + updatedS + ")))");
		}		
		return post.toString();
	}
	
	public Expression getMissingOrSpurious(){
		if(reference.iterSpace == null || submission.iterSpace == null)
			return null;
		Expression referenceCanUpdate = 
				Expression.conjunct(reference.iterSpace, reference.guard);
		Expression submissionCanUpdate = 
				Expression.conjunct(submission.iterSpace, submission.guard);
		Expression refImpliesSub =
				new Expression(ExprType.BINARY, "=>", 
						referenceCanUpdate, submissionCanUpdate, "");
		Expression subImpliesRef =
				new Expression(ExprType.BINARY, "=>", 
						submissionCanUpdate, referenceCanUpdate, "");
		return Expression.conjunct(refImpliesSub, subImpliesRef);
	}
	
	public String getMissingOrSpuriousSMT(){
		if(reference.iterSpace == null || submission.iterSpace == null)
			return "";
		StringBuilder str = new StringBuilder();
		str.append("(assert (=> (and " + reference.iterSpace.SMTVisitor() + " ");
		str.append(reference.guard.SMTVisitor() + ")))\n");
		str.append("(assert (=> (and " + submission.iterSpace.SMTVisitor() + " ");
		str.append(submission.guard.SMTVisitor() + ")))\n");		
		return str.toString();
	}
	
	public Expression getMissing(){
		//iter-R ∧ ¬iter-S ∧ idx ∧ guard-R
		if(reference.iterSpace == null)
			return null;
		//idx is accounted for by renaming the indices of the submission
		Expression missing = null;
		missing = Expression.conjunct(reference.iterSpace, reference.guard);
		missing = Expression.conjunct(missing, Expression.negate(submission.iterSpace));
		return missing;
	}
	
	public String getMissingSMT(){
		//iter-R ∧ ¬iter-S ∧ idx ∧ guard-R
		if(reference.iterSpace == null)
			return "";
		//idx is accounted for by renaming the indices of the submission
		StringBuilder missing = new StringBuilder();
		missing.append("(assert " + reference.iterSpace.SMTVisitor() + ")\n");
		missing.append("(assert " + reference.guard.SMTVisitor() + ")\n");
		missing.append("(assert (not " + submission.iterSpace.SMTVisitor() + "))\n");
		return missing.toString();
	}
	
	public Expression getSpurious(){
		//¬iter-R ∧ iter-S ∧ idx ∧ guard-S
		if(reference.iterSpace == null)
			return null;
		Expression spurious = null;
		spurious = Expression.conjunct(submission.guard, submission.iterSpace);
		spurious = Expression.conjunct(spurious, Expression.negate(reference.iterSpace));
		return spurious;	
	}
	
	public String getSpuriousSMT(){
		//¬iter-R ∧ iter-S ∧ idx ∧ guard-S
		if(reference.iterSpace == null)
			return "";
		//idx is accounted for by renaming the indices of the submission
		StringBuilder spurious = new StringBuilder();
		spurious.append("(assert (not " + reference.iterSpace.SMTVisitor() + "))\n");
		spurious.append("(assert " + submission.guard.SMTVisitor() + ")\n");
		spurious.append("(assert " + submission.iterSpace.SMTVisitor() + ")\n");
		return spurious.toString();
	}
	
	public Expression getEquiv(){
		//iter-R ∧ idx ∧ Pre ∧ body-R ∧ body-S ∧ Post [not ⇒ Post: we want to force Pre]
		Expression equiv = null;
		Expression ref = Expression.conjunct(reference.getIterSpace(), reference.getFormulaBody());
//		Expression sub = Expression.conjunct(submission.getIterSpace(), submission.getFormulaBody());
		Expression sub = submission.getFormulaBody();
		Expression pre = Expression.conjunct(getPre(), inputConstraint);
		Expression antecedent = Expression.conjunct(pre, Expression.conjunct(ref, sub));
		equiv = Expression.conjunct(antecedent, getPost());
		return equiv;
	}
	
	public String getEquivSMT(){
		//iter-R ∧ iter-S ∧ idx ∧ Pre ∧ body-R ∧ body-S ∧ Post [not ⇒ Post: we want to force Pre]
		StringBuilder equiv = new StringBuilder();
		
		equiv.append(";Type declarations:\n");
		Set<Expression> variables = getEquiv().variableVisitor();
		// for now, type everything as integer
		for(Expression e : variables)
			equiv.append("(declare-const "+e.printVisitor()+" "+"Int)\n");
		equiv.append(";Custom functions\n");
		equiv.append(smtmax);
		equiv.append(";Rename Schema:\n");
		for(Entry<Expression, Expression> entry : reference.SATVarToSummaryVar.entrySet()){
			equiv.append(";" + entry.getKey() + " <-> " + entry.getValue() + "\n");
		}
		for(Entry<Expression, Expression> entry : submission.SATVarToSummaryVar.entrySet()){
			equiv.append(";" + entry.getKey() + " <-> " + entry.getValue() + "\n");
		}
		equiv.append("; Reference iterspace and body \n");
		if(reference.iterSpace != null)
			equiv.append("(assert " + reference.iterSpace.SMTVisitor() + ")\n");
		for(AssignmentStatement s: reference.body){
			if(s.lhs == null && s.rhs == null){
				equiv.append(fallThroughSMT(s));
				continue;
			}
			String[] components = s.getSMTFormula().split(";");
			for (Entry<Expression, Expression> entry : reference.SATVarToSummaryVar.entrySet()){
				components[1] = components[1].replace(entry.getKey().getName(), entry.getValue().toString());
			}
			equiv.append(components[0] + " ;" + components[1]);
		}
		equiv.append("; Submission iterspace and body \n");
		if(submission.iterSpace != null)
			equiv.append("(assert " + submission.iterSpace.SMTVisitor() + ")\n");
		for(AssignmentStatement s: submission.body){
			if(s.lhs == null && s.rhs == null){
				equiv.append(fallThroughSMT(s));
				continue;
			}
			String[] components = s.getSMTFormula().split(";");
			for (Entry<Expression, Expression> entry : submission.SATVarToSummaryVar.entrySet()){
				components[1] = components[1].replace(entry.getKey().getName(), entry.getValue().toString());
			}
			equiv.append(components[0] + " ;" + components[1]);
		}
		equiv.append("; Pre \n");
		equiv.append(getPreSMT());
		equiv.append("; Post \n");
		equiv.append(getPostSMT());
		return equiv.toString();
	}

	public String fallThroughSMT(AssignmentStatement s) {
		StringBuilder str = new StringBuilder();
		String eqConstraint = reference.fallThroughExpression.SMTVisitor();
		str.append( "(assert (=> ");
		str.append(s.guard.SMTVisitor());
		str.append(eqConstraint);
		str.append(")");
		str.append(")\n");
		return str.toString();
	}
	
	// TODO: the variable declarations simply assume that all variables are integers
	// this needs to infer the right types using information gleaned from the
	// front end.
	public BoolExpr obtainZ3Expression(Expression exp){
		String formulaString = HelperFunctions.getSMTFormula(exp);
		System.out.println(formulaString);
		BoolExpr prop = null;
		// Ok, so a failed parse renders the context object unusable :-/
		// this means that we have to try parsing using a fresh object,
		// and use our static object only if the parse succeeded.
		if(!HelperFunctions.tryParse(formulaString)){
			return null;
		}
		
		try {
			prop = ctx.parseSMTLIB2String(formulaString, null, null, null, null);
		} catch (Z3Exception e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
			return null;
		}
		
		return prop;
	}
	
	public void generateFeedback(Model m) throws Z3Exception{
		Map<String, String> valuation = extractValuation(m);
		
		AssignmentStatement aRef = findAssignmentTriggered(reference, valuation); 
		String divergeRef = (aRef == null)? "Fall Through" : aRef.toString();
		AssignmentStatement aSub = findAssignmentTriggered(submission, valuation);
		String divergeSub = (aSub == null)? "Fall Through" : aSub.toString();
		
		HashMap<String, String> renamedValuations = rewriteValuationToSummaryVars(valuation);
		
		/*Rewrite feedback in terms of the canonical names, i.e. summary variables,
		 *rather than the SAT variables*/
		if(aRef != null)
			divergeRef = rewriteInTermsOfProgramVars(divergeRef);
		if(aSub != null)
			divergeSub = rewriteInTermsOfProgramVars(divergeSub);
		
		feedback.writeLog("Assignment statements that behave differently on the same input:");
		feedback.writeLog(divergeSub);
		feedback.writeLog(divergeRef);
		feedback.writeLog("Under this valuation:");
		feedback.writeLog(renamedValuations.toString());
	}

	public LocalizationInformation localize(Model model) throws Z3Exception{
		LocalizationInformation loc = new LocalizationInformation();
		Map<String, String> valuation = extractValuation(model);
		
		loc.subLocation = findAssignmentLocation(submission, valuation);
		loc.refLocation = findAssignmentLocation(reference, valuation);		
		Map<String, String> renamedValuations = rewriteValuationToSummaryVars(valuation);
		loc.valuation = renamedValuations;
		return loc;
	}
	
	public Map<String, String> extractValuation(Model model) throws Z3Exception {
		Map<String, String> valuation = new HashMap<String, String>();
		for(FuncDecl f : model.getDecls()){
			valuation.put(f.getName().toString(), 
					model.getConstInterp(f).toString());
		}
		return valuation;
	}

	public HashMap<String, String> rewriteValuationToSummaryVars(
			Map<String, String> valuation) {
		HashMap<String, String> renamedValuations = new HashMap<String, String>();
		
		/*Rewrite the valuation in terms of summary variables*/
		for (Entry<String, String> entry : valuation.entrySet()){
			if(SATVarToSummaryVarAsString.get(entry.getKey()) != null)
			renamedValuations.put(SATVarToSummaryVarAsString.get(entry.getKey()), 
					entry.getValue());
			else
				renamedValuations.put(entry.getKey(), 
						entry.getValue());
		}
		return renamedValuations;
	}

	public AssignmentStatement findAssignmentTriggered(Block block, Map<String, String> valuation){
		for(AssignmentStatement s : block.body){
			int value = HelperFunctions.evaluateExpression(valuation, s.getGuard());
				if(value != 0){
				return s;
			}
		}
		return null;
	}
	
	public int findAssignmentLocation(Block block, Map<String, String> valuation){
		int i = 0;
		for(AssignmentStatement s : block.body){
			int value = HelperFunctions.evaluateExpression(valuation, s.getGuard());
			if(value != 0)
				return i;
			i++;
		}
		return -1;
	}

	/*Will return UNSATISFIABLE if nothing is spurious*/
	public Status testSpurious() throws Z3Exception{
		Expression spurious = getSpurious();
		if(spurious == null)
			return Status.UNSATISFIABLE;
		BoolExpr b = obtainZ3Expression(spurious);
		if(b == null)
			return Status.UNKNOWN;
		assert sol.getNumAssertions() == 0 && sol.getNumScopes() == 0:
			"We're using a stale solver";
		sol.push();
		sol.add(b);
		Status stat = sol.check();
		if(stat == Status.SATISFIABLE){
			recordHeaderError("Submission will execute update ref won't under:", extractValuation(sol.getModel()));
		}
		sol.pop();
		return stat;
	}
	
	/*Will return UNSATISFIABLE if nothing is missing or spurious*/
	public Status testMissingOrSpurious() throws Z3Exception{
		// We need to test the validity of the following formula
		Expression missingOrSpurious = getMissingOrSpurious();
		if(missingOrSpurious == null)
			return Status.UNSATISFIABLE;
		// So negate it, and check for satisfiability
		BoolExpr b = obtainZ3Expression(Expression.negate(missingOrSpurious));
		if(b == null)
			return Status.UNKNOWN;
		assert sol.getNumAssertions() == 0 && sol.getNumScopes() == 0:
			"We're using a stale solver";
		sol.push();
		sol.add(b);
		Status stat = sol.check();
		if(stat == Status.SATISFIABLE){
			recordHeaderError("Ref and submission disagree under:", extractValuation(sol.getModel()));
		}
		sol.pop();
		return stat;
	}
	
	/*Will return UNSATISFIABLE if nothing is missing*/
	public Status testMissing() throws Z3Exception{
		Expression missing = getMissing();
		if(missing == null)
			return Status.UNSATISFIABLE;
		BoolExpr b = obtainZ3Expression(missing);
		if(b == null)
			return Status.UNKNOWN;
		assert sol.getNumAssertions() == 0 && sol.getNumScopes() == 0:
			"We're using a stale solver";
		sol.push();
		sol.add(b);
		Status stat = sol.check();
		
		if(stat == Status.SATISFIABLE){
			recordHeaderError("Ref will execute update submission won't under:", extractValuation(sol.getModel()));
		}
		sol.pop();
		return stat;
	}

	private void recordHeaderError(String errorMsg, Map<String, String> valuation){
//		feedback.writeLog(errorMsg);
//		feedback.writeLog(rewriteValuationToSummaryVars(valuation).toString());
		
		StringBuilder expected = new StringBuilder();
		StringBuilder actual = new StringBuilder();
		StringBuilder actualWithoutLineNumber = new StringBuilder();
		boolean multipleHeaders = (submission.header.size() > 1);
		if(submission.header != null)
			for(LoopStatement lp : submission.header){
//				actual.append(lp.toStringHeader() + "\n");
				actual.append(lp.toStringHeaderC(true) + "\n");
				actualWithoutLineNumber.append(lp.toStringHeaderC(false) + "\n");
			}
		if(reference.header != null)
			for(LoopStatement lp : reference.header){
//				expected.append(lp.toStringHeader() + "\n");
				expected.append(lp.toStringHeaderC(false) + "\n");
			}
		String exp = expected.toString();
		String act = actual.toString();
		String actWithoutLineNumber = actualWithoutLineNumber.toString();
		if(exp.compareTo(actWithoutLineNumber) != 0){
//			feedback.writeLog("Expected Headers: \n");
//			feedback.writeLog(rewriteInTermsOfProgramVars(exp));
//			feedback.writeLog("Found Headers: \n");
//			feedback.writeLog(rewriteInTermsOfProgramVars(act));			
			if (multipleHeaders)
				feedback.writeLog("Found loop headers:");
			else
				feedback.writeLog("Found loop header:");
			feedback.writeLog(rewriteInTermsOfProgramVars(act));
			if (multipleHeaders)
				feedback.writeLog("These should be:");
			else
				feedback.writeLog("It should be:");
			feedback.writeLog(rewriteInTermsOfProgramVars(exp));
			score += incorrectHeaderPenalty;
		}
		else{
//			feedback.writeLog("Your submission either performs updates " +
//					"the reference doesn't or misses updates the reference " +
//					"performs. Please examine the valuation below for points " +
//					"in the iteration space where this occurs.");
		}
	}

	
	/*
	 * guards are equivalent if (g1 <=> g2) is valid or
	 * !(g1<=>g2) is UNSAT
	 * we also append pre
	 * */
	public Status testGuardEquivalence(Expression g1, Expression g2) throws Z3Exception{
		Expression c1 = new Expression(ExprType.BINARY, "=>", g1, g2, "");
		Expression c2 = new Expression(ExprType.BINARY, "=>", g2, g1, "");
		Expression c3 = Expression.negate(Expression.conjunct(c1, c2));
		return checkSAT(Expression.conjunct(getPre(), c3), "");
	}
	
	public Status testArrayOOB(AssignmentStatement stmt) throws Z3Exception{
		Set<Expression> guardConstraints = getBoundsCondition(stmt);
		Status stat;
		for(Expression constraint : guardConstraints){
			stat = checkSAT(constraint, "Array out of bounds");
			if(stat == Status.SATISFIABLE)
				return stat;
		}
		return Status.UNSATISFIABLE;
	}
	
	
	public Status checkSAT(Expression exp, String errorMsg) throws Z3Exception {
		if(exp == null)
			return Status.UNSATISFIABLE;
		BoolExpr b = obtainZ3Expression(exp);
		if(b == null)
			return Status.UNKNOWN;
		Status status = testZ3Expression(b);
		if(status == Status.UNSATISFIABLE){
			return Status.UNSATISFIABLE;
		}
		else if(status == Status.SATISFIABLE){
			return Status.SATISFIABLE;
		}
		feedback.writeLog("Solver failed. Timeout?");
		return Status.UNKNOWN;
	}

	private Status testZ3Expression(BoolExpr b) throws Z3Exception {
		assert sol.getNumAssertions() == 0 && sol.getNumScopes() == 0:
			"We're using a stale solver";
		sol.push();
		sol.add(b);
		Status status = sol.check();
		sol.pop();
		return status;
	}
	
	/*Return true if the blocks are equivalent*/
	public ResultStats testEquivalence() throws Z3Exception {
		int numInsert = 0;
		int numReplace = 0;
		Expression equiv = getEquiv();
		BoolExpr b = obtainZ3Expression(equiv);
		if(b == null){
			score += PARSE_FAIL_PENALTY;
			return new ResultStats(false, 0, 0, "Z3 Failed to parse", score);
		}
		assert sol.getNumAssertions() == 0 && sol.getNumScopes() == 0:
			"We're using a stale solver";
		sol.push();
		sol.add(b);
//		System.out.println("Solver now: " + sol.toString());
		Status stat = sol.check();
		
		String failure = "";
		boolean failed = false;
		
		if(stat == Status.UNSATISFIABLE){
			sol.pop();
			return rStats;
		} else if (stat == Status.UNKNOWN) {
			sol.pop();
			rStats = new ResultStats(false, numInsert, numReplace, "SAT Timeout", score);
			return rStats;
		}
		
		while(stat == Status.SATISFIABLE){		
			if(numReplace + numInsert > THRESHOLD){
				failure = "Leaving repair incomplete - Threshold exceeded";
				failed = true;
				sol.pop();
				score += THRESHOLD;
				break;
			}
	
			LocalizationInformation loc = localize(sol.getModel());
			sol.pop();
			assert loc.subLocation >= 0 && loc.refLocation >= 0:
				"Localization did not succeed";
			Expression guardS = submission.body.get(loc.subLocation).getGuard();
			Expression guardR = reference.body.get(loc.refLocation).getGuard();
			
			if(testGuardEquivalence(guardS, guardR) == Status.UNSATISFIABLE){
				System.out.println("Replace");
				numReplace++;
				replace(loc.subLocation, loc.refLocation, loc.valuation);
			}
			else if(testGuardEquivalence(guardS, guardR) == Status.SATISFIABLE){
				System.out.println("Insert");
				numInsert++;
				insert(loc.subLocation, loc.refLocation, loc.valuation);
			}
			else if(testGuardEquivalence(guardS, guardR) == Status.UNKNOWN){
				rStats = new ResultStats(false, numInsert, numReplace, "SAT Timeout", score);
				return rStats;
			}
			
			equiv = getEquiv();
			b = obtainZ3Expression(equiv);
			if(b == null)
				return new ResultStats(false, 0, 0, "Z3 Failed to parse", score);
			
			assert sol.getNumAssertions() == 0 && sol.getNumScopes() == 0:
				"We're using a stale solver";
			sol.push();
			sol.add(b);
			stat = sol.check();
		}
		
		while(sol.getNumScopes() >= 1)
			sol.pop();
		
		if(!failed){
			for(AssignmentStatement stmt: submission.body){
				if(stmt.guard != null && !stmt.guard.equalsVisitor(Expression.getFalse()) && stmt.lhs != null){
					if(testArrayOOB(stmt) == Status.SATISFIABLE){
						SimplifyResult res = stmt.toSimplifiedResult();
						String currStmt = rewriteInTermsOfProgramVars(res.simplifiedString);
						feedback.flush();
//						feedback.writeLog("Array OOB in submission at guard " + 
//								rewriteInTermsOfProgramVars(currStmt));
						feedback.writeLog("Use following solution: \n");
//						if (feedback.isOutputMode()) 
//							feedback.writeLog(rewriteInTermsOfProgramVars(reference.toStringFeedback(true)));
//						else
//							feedback.writeLog(rewriteInTermsOfProgramVars(reference.toStringFeedback(false)));
						feedback.writeLog(rewriteInTermsOfProgramVars(reference.toStringFeedbackHeader()));
						for (AssignmentStatement s: reference.body) {
							if (s.getLhs() != null) 
								feedback.writeLog(rewriteInTermsOfProgramVars(s.toStringFeedback(feedback.isOutputMode())));
						}
						for (int i = 1; i <= reference.depth; i++) {
							feedback.writeLog("}");
						}
						rStats = new ResultStats(true, reference.body.size(), submission.body.size(), 0,
								"Array OOB", 
								reference.body.size(), res.originalSize, res.newSize);
						return rStats;
					}
				}
			}
			int origSize = 0, newSize = 0;
//			feedback.writeLog("============================================");
//			feedback.writeLog("The sequence of repairs to be followed: ");
			for(SimplifyResult s: repairs){
				feedback.writeLog(s.simplifiedString);
				origSize += s.originalSize;
				newSize += s.newSize;
			}
			Expression disjunct = null;
			for(Expression e : fallThroughSet){
				disjunct = Expression.disjunct(disjunct, e);
			}
			if(disjunct != null){
				System.out.println(HelperFunctions.getSMTFormula(disjunct));
				String fallThroExpression = SimplifyPrint.
						simplifiedExpressionAsString(disjunct).simplifiedString;
				fallThroExpression = rewriteInTermsOfProgramVars(fallThroExpression);
				feedback.writeLog("Under the guard " + fallThroExpression + ", do not perform updates");
			}
//			System.out.println(getEquivSMT());
//			feedback.writeLog("============================================");
//			feedback.writeLog("The submission summary now looks like: ");
//			for(AssignmentStatement stmt: submission.body){
//				String currStmt = stmt.toSimplifiedResult().simplifiedString;
//				currStmt = rewriteInTermsOfProgramVars(currStmt).replace("Prime", "");
//				if(stmt.rhs != null)
//					feedback.writeLog(currStmt);
//			}
//			feedback.writeLog("============================================");
			System.out.println("Num insert "+ numInsert + " num replace " + numReplace);
//			feedback.writeLog("Num insert "+ numInsert + " num replace " + numReplace);
			score += (numInsert + numReplace);
			rStats = new ResultStats(true, numInsert, numReplace, 0, "", score, origSize, newSize);
		}
		else{
			if(stat == Status.UNKNOWN){
				score += timeoutPenalty;
				rStats = new ResultStats(false, numInsert, numReplace, "Timeout", score);
			}
			else{
//				feedback.writeLog("More than 10 corrections: \n");
//				feedback.writeLog("Use reference solution: \n");
				writeReferenceSummary(failure);
			}
		}
		
		return rStats;
	}

	public void writeReferenceSummary(String failure) {
		feedback.writeLog("Use the following solution:\n");
//				feedback.writeLog(rewriteInTermsOfProgramVars(reference.toStringFeedback(feedback.isOutputMode())));
		feedback.writeLog(rewriteInTermsOfProgramVars(reference.toStringFeedbackHeader()));
		for (AssignmentStatement s: reference.body) {
			if (s.getLhs() != null) 
				feedback.writeLog(rewriteInTermsOfProgramVars(s.toStringFeedback(feedback.isOutputMode())));
		}
		for (int i = 1; i <= reference.depth; i++) {
			feedback.writeLog("}");
		}
		rStats = new ResultStats(true, reference.body.size(), submission.body.size(), failure, score);
	}

	public int getScore() {
		return score;
	}

	public boolean getMatched() {
		return matched;
	}
	
	private void suggestFeedback(Map<String, String> valuation,
			String divergeRef, String divergeSub) {
//		feedback.writeLog("Assignment statements that behave differently on the same input:");
//		feedback.writeLog("Ref: " + divergeRef);
//		feedback.writeLog("Sub: "+ divergeSub);
//		feedback.writeLog("Under this valuation:");
//		feedback.writeLog(valuation.toString());
		
	}
	
	public void genReplace(int faultySubIndex, int refIndex, 
			Map<String, String> valuation, AssignmentStatement replacement){
		AssignmentStatement aRef = reference.body.get(refIndex); 
		String divergeRef = aRef.toSimplifiedResult().simplifiedString;
		AssignmentStatement aSub = submission.body.get(faultySubIndex);
		String divergeSub = aSub.toSimplifiedResult().simplifiedString;
	
		/*Rewrite feedback in terms of the canonical names, i.e. summary variables,
		 *rather than the SAT variables*/
		if(aRef != null)
			divergeRef = rewriteInTermsOfProgramVars(divergeRef);
		if(aSub != null)
			divergeSub = rewriteInTermsOfProgramVars(divergeSub);
		divergeSub = divergeSub.replace("Prime", "");
		divergeRef = divergeRef.replace("Prime", "");
		suggestFeedback(valuation, divergeRef, divergeSub);
		
		setUpFeedback(replacement, aSub);
	}

	public void genInsert(int faultySubIndex, int refIndex,
			Map<String, String> valuation, AssignmentStatement toInsert){
		
		AssignmentStatement aRef = reference.body.get(refIndex); 
		String divergeRef = aRef.toSimplifiedResult().simplifiedString;
		AssignmentStatement aSub = submission.body.get(faultySubIndex);
		String divergeSub = aSub.toSimplifiedResult().simplifiedString;
	
		/*Rewrite feedback in terms of the canonical names, i.e. summary variables,
		 *rather than the SAT variables*/
		if(aRef != null)
			divergeRef = rewriteInTermsOfProgramVars(divergeRef);
		if(aSub != null)
			divergeSub = rewriteInTermsOfProgramVars(divergeSub);
		divergeSub = divergeSub.replace("Prime", "");
		divergeRef = divergeRef.replace("Prime", "");
		suggestFeedback(valuation, divergeRef, divergeSub);
		
		setUpFeedback(toInsert, aSub);
	}
	
	private void setUpFeedback(AssignmentStatement replacement, AssignmentStatement faultySub) {
		SimplifyResult res = replacement.toSimplifiedResult();
		String repl = res.simplifiedString;
		if(replacement.getRhs() == null){
			fallThroughSet.add(replacement.guard);
		}
		repl = rewriteInTermsOfProgramVars(repl);
		repl = repl.replace("Prime", "");
		repl = repl.replace("symex_max", "max");
		String[] split = new String[2];
		split[0] = repl.substring(0,repl.indexOf(':'));
		split[1] = repl.substring(repl.indexOf(':') + 1);
		String feedbackString = "";
		String insteadOf = "";
		if(faultySub != null)
			if(faultySub.rhs != null){
				insteadOf = rewriteInTermsOfProgramVars(faultySub.lhs.toString() + " = " + faultySub.rhs.toString());
				insteadOf = insteadOf.replace("Prime", "");
				insteadOf = insteadOf.replace("symex_max", "max");
			}
				
//		if(split.length > 1){
			if("".compareTo(split[1]) != 0){
//				feedback.writeLog("The fix for this problem is: ");
				if (feedback.isOutputMode())
					feedbackString = "Under the guard " + split[0].trim() + ", print " + split[1] + " instead of " + insteadOf;
				else
					feedbackString = "Under the guard " + split[0].trim() + ", do " + split[1] + " instead of " + insteadOf;
			}
//		}
//		feedback.writeLog(feedbackString);
		repairs.add(new SimplifyResult(feedbackString, res.originalSize, res.newSize));
	}
	
	public void replace(int faultySubIndex, int refIndex, Map<String, String> valuation){
		AssignmentStatement replacement = reference.body.get(refIndex).clone();
		replacement.renameVisitor(correspondenceMap);
		genReplace(faultySubIndex, refIndex, valuation, replacement);
		submission.body.set(faultySubIndex, replacement);
	}
	
	public void insert(int faultySubIndex, int refIndex, Map<String, String> valuation){
		AssignmentStatement replacement = reference.body.get(refIndex).clone();
		replacement.renameVisitor(correspondenceMap);
		AssignmentStatement toInsert = submission.body.get(faultySubIndex).clone();
		Expression summaryGuard = (Expression) toInsert.getGuard().clone();
		Expression referenceGuard = (Expression) replacement.getGuard().clone();
		Expression refinedGuard = Expression.conjunct(summaryGuard, replacement.getGuard());
		replacement.setGuard(refinedGuard);
		genInsert(faultySubIndex, refIndex, valuation, replacement);
		submission.body.set(faultySubIndex, replacement);
		
		Expression guardS = (Expression) toInsert.getGuard().clone();
		Expression negGuardR = Expression.negate(referenceGuard);
		Expression insertGuard = Expression.conjunct(guardS, negGuardR);
		toInsert.setGuard(insertGuard);
		submission.body.add(faultySubIndex+1, toInsert);
					
	}
	
}
