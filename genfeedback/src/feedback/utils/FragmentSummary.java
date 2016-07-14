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
import com.microsoft.z3.Solver;
import com.microsoft.z3.Status;
import com.microsoft.z3.Z3Exception;

import feedback.logging.Logger;
import feedback.update.parser.ExprLexer;
import feedback.update.parser.ExprParser;
import feedback.update.parser.ExprParser.startrule_return;
import feedback.update.parser.ExpressionTreeWalker;
import feedback.utils.Expression.ExprType;
import feedback.utils.LoopStatement.Direction;


// Store the summary of processing a single loop statement/straight line fragment
public class FragmentSummary {
	
	// encoding the max function in the SMTLIB2 format
	public static String smtmax = "(define-fun max ((x Int) (y Int)) Int\n"+
			"  (ite (<= x y) y x))\n";
	
	/*Data structures to store the result of processing this summary*/
	public Set<Expression> lhsUpdated;	// the expressions updated in statements in the loop body
	public LoopStatement header;    //  the outermost loop header if this is a loop statement

        // the statements in the loop body/straight line fragment
        // after processing, will contain the statements
        // with ids and array accesses renamed afresh
	public List<Statement> stmts;   
                                        
									
	// map from id's and array expressions in statements to fresh variables
	public Map<Expression, Expression> bindings;
	
	// map from ids in the rewritten formula to their types. We need this 
	// to tell the SMT solver what the types are
	public Map<Expression, String> types;
	
        // the type declarations for the SMTLIB2 formula
	public String decls;			

        // the SMTLIB2 formula representing the 
        // body of the loop with guards made disjoint
	// and arrays made uninterpreted
	public String formula;			
	

        // if this is from a submission, the correctness condition
        // against the particular reference it was processed against.
	public String correctnessCondition; 
										
	// if this is from a reference implementation 
        // generate an smt forumla representing the bounds
        // only consider bounds that are numeric
	public String boundsCondition;		
										
        /*Does the submission execute anything the reference does not?*/				
	public String spuriousStatementCondition;
	
        /*Does the submission miss anything the reference executes?*/
	public String missingStatementCondition;
	
	public boolean conditionallyMatched;
	
	int score;

	public Set<Expression> iterationSpace;

	private Map<Expression, Expression> rename;

	public Map<Expression, Expression> getRename() {
		return rename;
	}

	public int getScore(){
		return score;
	}
	
	public Set<Expression> getLhsUpdated() {
		return lhsUpdated;
	}

	public Map<Expression, Expression> getBindings() {
		return bindings;
	}

	public Map<Expression, String> getTypes() {
		return types;
	}

	public String getDecls() {
		return decls;
	}

	public String getFormula() {
		return formula;
	}

	public LoopStatement getHeader() {
		return header;
	}

	public FragmentSummary(LoopStatement header, boolean populate) {
		super();
		this.header = header;
		types = new HashMap<Expression, String>();
		boundsCondition = new String();
		iterationSpace = new HashSet<Expression>();
		rename = new HashMap<Expression, Expression>();
		conditionallyMatched = false;
		score = 0;
		if(populate)
			populateSummary();
	}
	
	public FragmentSummary(List<Statement> stmts, boolean populate) {
		super();
		this.stmts = stmts;
		types = new HashMap<Expression, String>();
		conditionallyMatched = false;
		iterationSpace = new HashSet<Expression>();
		rename = new HashMap<Expression, Expression>();
		score = 0;
		if(populate)
			populateSummary();
	}
	
        /*Compute the formula for a reference implementation*/
	public void populateSummary(){
		if(header!=null)
			stmts = header.getBody();
//		Set<Expression> iVars = header.getIndexVariables();
		Set<Expression> iVars = new HashSet<Expression>();
		lhsUpdated = HelperFunctions.getAllLHS(stmts);
		bindings = HelperFunctions.getNewNames(stmts, "ref", iVars);
		HelperFunctions.renameStatements(stmts, bindings);
		for(Statement s : stmts){
			AssignmentStatement aStmt = (AssignmentStatement) s;
			aStmt.renameLhsInRhs();
		}
		HelperFunctions.getTypes(stmts, types);
		computeBoundsCondition();
		decls = HelperFunctions.getSMTFormula(types);
		formula = HelperFunctions.getSMTStringForStatements(stmts);
		
	}

	public boolean populateSummary(FragmentSummary ref, Logger errorLogger, 
			Logger feedbackLogger){
//		if(HelperFunctions.getLhs(header).getDimension() != 
//				ref.updateLhs.getDimension()){
//			feedbackLogger.writeLog("Mismatched LHS array dimensions");
//			score += 5;
//			return false;				
//		}		
		if((header == null && ref.header != null) || (header!=null && ref.header == null)){
			feedbackLogger.writeLog("Mismatched fragment types: loop vs straight line");
			score += 5;
			return false;
		}
		
		if(header != null)
			stmts = header.getBody();
		
		Map<String, String> reconcile = new HashMap<String, String>();
				
		// We're comparing two loop fragments
		if(header != null && ref.header != null){

			int match = ref.header.match(header);
			if(match > 0){
				score += 5;
				feedbackLogger.writeLog("Comparing fragments mismatched loop nests (depth/direction)");
				return false;
			}
		 
			// After the call to reconcile,
		    // the map rename maps submission index variables
            // to reference index variables
			int s = header.reconcile(rename, ref.header, feedbackLogger);

			if(s > 0)
			    conditionallyMatched = true;
			
			score += s;
			
			boolean flag = false;
			
			// local rename is old sub index name -> new sub index name
			Map<String, String> localRename = new HashMap<String, String>();
			
//			// Handling cases such as j -> i and i -> j
			for(Entry<Expression, Expression> entry : rename.entrySet()){
				Expression key = entry.getKey();
				Expression value = entry.getValue();
				if(rename.containsKey(value)){
//					Expression rebound = (Expression) value.clone();
//					String name = value.getName();
//					String reBoundName = "sub_" + name;
//					rebound.setName(reBoundName);
//					localRename.put(name, reBoundName);
//					entry.setValue(rebound);
					flag = true;
					break;
				}
			}
			
			// reconcile maps index vars from sub to ref
			reconcile.putAll(getRenameBindingsAsString(rename));
			
			if(flag){
				HelperFunctions.renameIdentifiers(stmts, reconcile);
				HelperFunctions.renameIndices(header, reconcile);
			}
			
		}
			
		lhsUpdated = HelperFunctions.getAllLHS(stmts);
//		Set<Expression> renamedLhsUpdate = new HashSet<Expression>();
//		for(Expression e: lhsUpdated){
//			Expression copy = (Expression) e.clone();
//			copy.renameIdentifiers(reconcile);
//			renamedLhsUpdate.add(copy);
//		}
		
		if(!lhsUpdated.equals(ref.getLhsUpdated())){
			score += 4;
			feedbackLogger.writeLog("Comparing fragments that update different locations");
			return false;
		}
		
		
		bindings = HelperFunctions.getNewNames(stmts, "sub");

		Map<Expression, Expression> equalityConstraints = 
				new HashMap<Expression, Expression>();
		
		Map<Expression, Expression> refBindings = ref.bindings;
		
		/*Obtain equality constraints on array variables*/
		equalityConstraints = getEqualityConstraints(ref, refBindings);
				
		// perform the renaming
		HelperFunctions.renameStatements(stmts, bindings);
		
		for(Statement s : stmts){
			AssignmentStatement aStmt = (AssignmentStatement) s;
			aStmt.renameLhsInRhs();
		}
		
		Expression conjunctionOfGuards = HelperFunctions.getConjunctionOfGuards(stmts);
		if(HelperFunctions.checkSAT(conjunctionOfGuards)){
			List<AssignmentStatement> l = new ArrayList<AssignmentStatement>();
			for(Statement s: stmts){
				AssignmentStatement aStmt = (AssignmentStatement) s;
				//aStmt.renameLhsInRhs();
				l.add(aStmt);
			}
			
			List<AssignmentStatement> acc = new ArrayList<AssignmentStatement>();
			AssignmentStatement.makeDisjoint(l, acc);
			stmts.clear();
			stmts.addAll(acc);
		}
		
		HelperFunctions.getTypes(stmts, types);
				
		//checkSanity(ref.getTypes(), types);
				
		// remove those key, value pairs from subTypes, where the key is already
		// in refTypes. This is so that we don't emit multiple type decls in the
		// SMT formula string
		for (Expression e : ref.types.keySet()) {
			if(types.containsKey(e))
				types.remove(e);
		}
	
		StringBuilder eqConstraint = new StringBuilder();
		if(!HelperFunctions.testIdentity(rename)){
			//HelperFunctions.renameIdentifiers(stmts, reconcile);
			for(Entry<Expression, Expression> entry : rename.entrySet()){
				Expression val = entry.getValue();
				eqConstraint.append("(assert (= " + entry.getKey() + " " + 
							val + "))\n");
				// Track index variables of submission, if they aren't gonna be
				// declared by ref
				if(!types.containsKey(val) && !ref.types.containsKey(val))
						types.put(entry.getValue(), "Int");
			}
		}
		
		for(Entry<Expression, Expression> entry : equalityConstraints.entrySet()){
				eqConstraint.append("(assert (= " + entry.getKey() + " " + 
							entry.getValue() + "))\n");
		}
		
		decls = HelperFunctions.getSMTFormula(types);
		formula = HelperFunctions.getSMTStringForStatements(stmts) + eqConstraint.toString();
		System.out.println(formula);
		
		return true;
	}

	private Map<Expression, Expression> getEqualityConstraints(FragmentSummary ref,
			Map<Expression, Expression> refBindings) {
		Map<Expression, Expression> equalityConstraints = 
				new HashMap<Expression, Expression>();
		for (Map.Entry<Expression, Expression> entry : refBindings.entrySet()) {
			Expression key = entry.getKey();
			for(Map.Entry<Expression, Expression> sentry : bindings.entrySet()){	
				if(key.equals(sentry.getKey()) && ref.lhsUpdated.contains(key) == false){
					equalityConstraints.put(entry.getValue(), sentry.getValue());
					break;
				}
			}
		}
		return equalityConstraints;
	}

	public void computeDisjointnessCondition(FragmentSummary ref,
			Map<Expression, Expression> rename) {
		Set<Expression> toConjunct = new HashSet<Expression>();
		computeBoundsCondition();
		
		/*Are there index valuations outside the iteration space of the reference
		 * but in the iteration space of the submission?*/
		
		/*index valuations outside the iteration space of the reference*/
		Expression outsideRefSpace = null;
		for(Expression e : ref.iterationSpace){
			outsideRefSpace = Expression.disjunct(outsideRefSpace, Expression.negate(e));
		}
		toConjunct.add(outsideRefSpace);
		
		/*index valuations in the iteration space of the submission*/
		toConjunct.addAll(iterationSpace);
		
		/*The index variables may not agree*/
		/* Examine if there are differing valuations to index vars within the 
		 * iteration space of both ref and submission*/
		Expression indexMatch = null;
		for(Entry<Expression, Expression> entry : rename.entrySet()){
			Expression e = new Expression(ExprType.BINARY, "==", 
					entry.getKey(), entry.getValue(), "");
			indexMatch = Expression.conjunct(indexMatch, e);
		}
		
		toConjunct.add(indexMatch);
		
		Expression conjunct = null;
		for(Expression e : toConjunct){
			conjunct = Expression.conjunct(conjunct, e);
		}
		
		Expression disjOfGuards = HelperFunctions.getDisjunctionOfGuards(stmts); 
		
		spuriousStatementCondition = ref.getDecls() + decls + 
				"(assert " + Expression.conjunct(conjunct, disjOfGuards).SMTVisitor() + ")";
		/*--------------------------------------------------------------------*/
		
		/*Are there index valuation outside the iteration space of the submission
		 * but in the iteration space of the reference?*/
		
		toConjunct.clear();
		
		/*index valuations in the iteration space of the reference*/
		toConjunct.addAll(ref.iterationSpace);
		
		/*index valuations outside the iteration space of the submission*/
		Expression outsideSubSpace = null;
		for(Expression e : iterationSpace){
			outsideSubSpace = Expression.disjunct(outsideSubSpace, Expression.negate(e));
		}
		toConjunct.add(outsideSubSpace);
		
		toConjunct.add(indexMatch);
		
		conjunct = null;
		for(Expression e : toConjunct){
			conjunct = Expression.conjunct(conjunct, e);
		}
		
		missingStatementCondition = ref.getDecls() + decls + 
				"(assert " + Expression.conjunct(conjunct, 
				HelperFunctions.getDisjunctionOfGuards(ref.stmts)).SMTVisitor() + ")";
		
	}

	public Map<String, String> getRenameBindingsAsString(Map<Expression, Expression> rename) {
		Map<String, String> reconcile = new HashMap<String, String>();
		for (Map.Entry<Expression, Expression> entry : rename.entrySet()) {
			  Expression key = entry.getKey();
			  Expression value = entry.getValue();
			 
			if(value.EType == ExprType.ID)
				  reconcile.put(value.toString(), key.toString());
		}
		return reconcile;
	}
	
	public void computeBoundsCondition(){
		
		if(header == null){
			boundsCondition = "";
			return;
		}
		iterationSpace.clear();
		List<LoopStatement> currentHeaders = new ArrayList<LoopStatement>();
		currentHeaders.add(header);
		for(Statement s: header.getBlock()){
			if(s instanceof LoopStatement)
				currentHeaders.add((LoopStatement) s);
		}
		String str = "";
		for(LoopStatement s: currentHeaders){
			Expression lb = s.getLowerBound();
			Expression ub = s.getUpperBound();
			
			/*Populate type information for the lower and upper bounds*/
			if(lb.EType == ExprType.ID){
				if(!types.containsKey(lb))
					types.put(lb, "Int");
			}
			else{
				lb.typeVisitor(types, "");
			}
			
			if(ub.EType == ExprType.ID){
				if(!types.containsKey(ub))
					types.put(ub, "Int");
			}
			else{
				ub.typeVisitor(types, "");
			}
			
			Expression index = s.getIndex();
			if(!types.containsKey(index))
				types.put(index, "Int");
			Expression e1, e2;
			if(s.dir == Direction.UP){
				e1 = new Expression(ExprType.BINARY, ">=", index, lb, "");
				str += "(assert "+ e1.SMTVisitor() + ")\n";
				e2 = new Expression(ExprType.BINARY, "<=", index, ub, "");
				str += "(assert "+ e2.SMTVisitor() + ")\n";
			}
			else{
				e1 = new Expression(ExprType.BINARY, "<=", index, lb, "");
				str += "(assert "+ e1.SMTVisitor() + ")\n";
				e2 = new Expression(ExprType.BINARY, ">=", index, ub, "");
				str += "(assert "+ e2.SMTVisitor() + ")\n";
			}
			boundsCondition = str;
			iterationSpace.add(e1);
			iterationSpace.add(e2);
		}
		boundsCondition = str;
	}
	
	public void computeCorrectnessCondition(FragmentSummary ref){
		StringBuilder incomingEqual = new StringBuilder();
		StringBuilder subFallThrough = new StringBuilder();
		StringBuilder refFallThrough = new StringBuilder();
		StringBuilder outputConstr = new StringBuilder();
		String refGuardsFallThroughPC = 
				HelperFunctions.getConjunctionOfNegatedGuards(ref.stmts).SMTVisitor();
		String subGuardsFallThroughPC = 
				HelperFunctions.getConjunctionOfNegatedGuards(stmts).SMTVisitor();

		for(Expression e: lhsUpdated){
			String subLhs = bindings.get(e).getName();
			String subLhsRename = subLhs+"Prime";
			Expression sameLhs = ((Expression)e.clone());
			sameLhs.renameIdentifiers(getRenameBindingsAsString(rename));
			String refLhs = ref.bindings.get(sameLhs).getName();
			String refLhsRename = refLhs+"Prime";
			
			incomingEqual.append("(assert (= ");
			incomingEqual.append(refLhsRename);
			incomingEqual.append(" ");
			incomingEqual.append(subLhsRename);
			incomingEqual.append("))\n");
			if(!decls.contains(subLhsRename)){
				decls += "(declare-const "+ subLhsRename +" "+
						types.get(bindings.get(e))+")\n";
			}
			subFallThrough.append("(assert (=> " + subGuardsFallThroughPC + " " + 
					"(= " + subLhs + " " + subLhsRename + ")))\n");

			if(!ref.getDecls().contains(refLhsRename)){
				ref.decls += 
						"(declare-const "+ refLhsRename +" "+
				ref.types.get(ref.bindings.get(sameLhs))+")\n";
			}
			refFallThrough.append("(assert (=> " + refGuardsFallThroughPC + " " + 
					"(= " + refLhs + " " + refLhsRename + ")))\n");
			
			outputConstr.append("(assert (not (= "+ refLhs + " " + subLhs +")))\n");
		}
		
		correctnessCondition =  smtmax + ref.getDecls() + decls + incomingEqual + ref.boundsCondition +
				ref.getFormula() + formula + refFallThrough + 
				subFallThrough + outputConstr;
		
		System.out.println(correctnessCondition);
	}
	
	public boolean checkSAT(FragmentSummary ref, Logger feedbackLogger, String formula) 
			throws Z3Exception {
		HashMap<String, String> cfg = new HashMap<String, String>();
		cfg.put("model", "true");
		Context ctx = null;
		ctx = new Context(cfg);
		BoolExpr prop = ctx.parseSMTLIB2String(formula, null, null, null, null);
		//System.out.println("prop " + prop);
		Solver sol = ctx.mkSolver();
		sol.add(prop);
		if (sol.check() == Status.UNSATISFIABLE){
			System.out.println("UNSAT");
			sol.dispose();
			ctx.dispose();
//			System.gc();
			return false;
		}
		else if(sol.check() == Status.SATISFIABLE){
			feedbackLogger.writeLog("Differing valuation for loop index vars and bounds ");
			feedbackLogger.writeLog(sol.getModel().toString());
			System.out.println(sol.getModel().toString());
		}
		else if(sol.check() == Status.UNKNOWN){
			System.out.println(sol.getReasonUnknown());
		}
		sol.dispose();
		ctx.dispose();
//		System.gc();
		return true;
	}
	
	public boolean checkCorrectnessCondition(FragmentSummary ref, Logger feedbackLogger) {
		HashMap<String, String> cfg = new HashMap<String, String>();
		cfg.put("model", "true");
		Context ctx = null;
		try {
			ctx = new Context(cfg);
			BoolExpr prop = ctx.parseSMTLIB2String(
					correctnessCondition, null, null, null, null);
			//System.out.println("prop " + prop);
			Solver sol = ctx.mkSolver();
			sol.add(prop);
			if (sol.check() == Status.UNSATISFIABLE){
				System.out.println("UNSAT");
				return false;
			}
			else if(sol.check() == Status.SATISFIABLE){
				score += 4;
				generateFeedback(ref, sol, feedbackLogger);
				sol.dispose();
			}
			else if(sol.check() == Status.UNKNOWN){
				System.out.println("STATUS:" + sol.getReasonUnknown());
				System.out.println("Timeout?");
			}
		} catch (Z3Exception e) {
			e.printStackTrace();
			feedbackLogger.writeLog("Failed to parse sat formula: " + e.getMessage());
			score += 4;
		}
		
		ctx.dispose();
//		System.gc();
		return true;
	}

	public void generateFeedback(FragmentSummary ref, Solver sol, Logger feedbackLogger) 
			throws Z3Exception {
	
		
		Map<Expression, Expression> invRef =
				HelperFunctions.invert(ref.getBindings());
		Map<Expression, Expression> invSub =
				HelperFunctions.invert(bindings);
		String expl = sol.getModel().toString();
		Model model = sol.getModel();
		


		Map<String, String> valuation = new HashMap<String, String>();
		for(FuncDecl f : model.getDecls()){
//					System.out.println(f.getName() +"->"+ model.getConstInterp(f));
			valuation.put(f.getName().toString(), 
					model.getConstInterp(f).toString());
		}
		
		AssignmentStatement aRef = findRelevantAssignment(ref, valuation); 
		String divergeRef = (aRef == null)? "Fall Through" : aRef.toString();
		AssignmentStatement aSub = findRelevantAssignment(this, valuation);
		String divergeSub = (aSub == null)? "Fall Through" : aSub.toString();
		
		HashMap<String, String> renamedValuations = new HashMap<String, String>();
		
		for (Entry<Expression, Expression> entry : invSub.entrySet()){
		    String lookFor = entry.getKey().toString();
		    String remap = entry.getValue().toString();
		    expl = expl.replace(lookFor, remap);
		    divergeSub = divergeSub.replace(lookFor, remap);
		    for (Entry<String, String> e : valuation.entrySet()){
		    	String oldName = e.getKey();
		    	if(oldName.contains(lookFor)){
		    		String newName = oldName.replace(lookFor, remap);
		    		renamedValuations.put(newName, e.getValue());
		    	}
		    }
		}
		
		for(Entry<Expression, Expression> entry : rename.entrySet()){
			renamedValuations.put(entry.getKey().toString(), valuation.get(entry.getKey().toString()));
			renamedValuations.put(entry.getValue().toString(), valuation.get(entry.getValue().toString()));
		}
		
		for (Entry<Expression, Expression> entry : invRef.entrySet()){
			String lookFor = entry.getKey().toString();
			String remap = entry.getValue().toString();
		    expl = expl.replace(lookFor, remap);
		    divergeRef = divergeRef.replace(lookFor, remap);
		    for (Entry<String, String> e : valuation.entrySet()){
		    	String oldName = e.getKey();
		    	if(oldName.contains(lookFor)){
		    		String newName = oldName.replace(lookFor, remap);
		    		renamedValuations.put(newName, e.getValue());
		    	}
		    }
		}
		if(renamedValuations.isEmpty())
			renamedValuations.putAll(valuation);
		valuation.clear();
		feedbackLogger.writeLog("Assignment statements that behave differently on the same input:");
		feedbackLogger.writeLog(divergeSub);
		feedbackLogger.writeLog(divergeRef);
		feedbackLogger.writeLog("Under this valuation:");
		feedbackLogger.writeLog(renamedValuations.toString());
	}
	
	public static AssignmentStatement findRelevantAssignment(FragmentSummary updateSum,
			Map<String, String> valuation) {
		
		for(Statement s : updateSum.stmts){
			AssignmentStatement aStmt = (AssignmentStatement) s;
			String exp = aStmt.getGuard().printVisitor();
			
			CharStream strm = new ANTLRStringStream(exp);
			ExprLexer lex = new ExprLexer(strm);
			TokenStream tokenStrm = new CommonTokenStream(lex);
			ExprParser pars = new ExprParser(tokenStrm);
			startrule_return expression = null;
			try {
				expression = pars.startrule();
			} catch (RecognitionException e) {
				e.printStackTrace();
			}
			System.out.println(expression.getTree().toStringTree());
			CommonTreeNodeStream nStream = new CommonTreeNodeStream(expression.getTree());
			ExpressionTreeWalker expWalker = new ExpressionTreeWalker(nStream);
			expWalker.variables.putAll(valuation);
			int value = 0;
			try {
				value = expWalker.eval();
			} catch (RecognitionException e) {
				System.out.println(exp);
				System.out.println(valuation);
				e.printStackTrace();
			}	
			if(value != 0){
				return aStmt;
			}
		}
		
		return null;
	}
}
