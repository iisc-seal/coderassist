package feedback.utils;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import com.microsoft.z3.Z3Exception;

import feedback.inputs.ProgramInputs;
import feedback.utils.Expression.ExprType;
import feedback.utils.LoopStatement.Direction;
import feedback.utils.SimplifyPrint.SimplifyResult;

public class AssignmentStatement implements Statement {
	boolean isGlobalInit;
	boolean newline;
	Expression guard;
	Expression lhs;
	Expression rhs;
	Expression originalLhs;
	Expression originalRhs;
	Expression originalGuard;
	
	public AssignmentStatement(boolean isGlobal, Expression guard, Expression lhs, Expression rhs) {
		super();
		this.isGlobalInit = isGlobal;
		this.guard = guard;
		this.lhs = lhs;
		this.rhs = rhs;
		fixTypes();
	}

	public AssignmentStatement(Expression guard, Expression lhs, Expression rhs) {
		super();
		this.isGlobalInit = false;
		this.guard = guard;
		this.lhs = lhs;
		this.rhs = rhs;
		fixTypes();
	}
	
	public AssignmentStatement(Expression guard, Expression lhs, Expression rhs, boolean newline) {
		super();
		this.isGlobalInit = false;
		this.guard = guard;
		this.lhs = lhs;
		this.rhs = rhs;
		this.newline = newline;
		fixTypes();
	}
	
	public AssignmentStatement(boolean globalInit, Expression guard,
			Expression lhs, Expression rhs, boolean newline) {
		this.isGlobalInit = globalInit;
		this.guard = guard;
		this.lhs = lhs;
		this.rhs = rhs;
		this.newline = newline;
		fixTypes();
	}

	private void fixTypes() {
		Map<String, String> types = new HashMap<String, String>();
		if(guard != null){
			originalGuard = (Expression) guard.clone();
			guard.fixTypes(types, "Bool");
			if("Bool".compareTo(guard.inferType(types)) != 0)
				guard = guard.getAsBoolean();
			if(!originalGuard.equals(guard))
				System.out.println("Rewrote "+originalGuard+" to "+guard);
		}
		if(rhs != null){
			originalRhs = (Expression) rhs.clone();
			rhs.fixTypes(types);
			
			if (	rhs.inferType(types) != null &&
					rhs.inferType(types).compareTo("Bool") == 0 &&
					rhs.getName().compareTo("any") != 0) {
//				rhs.fixTypes(types, lhs.getType(null));
				rhs = rhs.getAsInteger();
			}
			if(!originalRhs.equals(rhs))
				System.out.println("Rewrote "+originalRhs+" to "+rhs);
		}
	}

	
	@Override
	public String toString() {
		StringBuilder stmtString = new StringBuilder();
		if(guard != null)
			stmtString.append(guard.printVisitor() + ':');
		if(lhs != null && rhs != null) {
			if (lhs.getEType() == ExprType.ID && lhs.getName().equals("out"))
				stmtString.append("Print " + rhs.printVisitor() + "\n");
			else
				stmtString.append(lhs.printVisitor() + "=" + rhs.printVisitor() + "\n");
		}
		return stmtString.toString();
	}
	
	public String toOriginalString() {
		StringBuilder stmtString = new StringBuilder();
		if(guard != null)
			stmtString.append(originalGuard.printVisitor() + ':');
		if(lhs != null && rhs != null) {
			if (lhs.getEType() == ExprType.ID && lhs.getName().equals("out"))
				stmtString.append("Print " + originalRhs.printVisitor() + "\n");
			else
				stmtString.append(lhs.printVisitor() + "=" + originalRhs.printVisitor() + "\n");
		}
		return stmtString.toString();
	}
	
	public String toStringFeedback(boolean isOutputBlock) {
		StringBuilder stmtString = new StringBuilder();
		if(guard != null) {
			if (guard.printVisitor().equals("true"))
				;
			else
//				stmtString.append("if (" + guard.printVisitor() + ") ");
				stmtString.append("if (" + SimplifyPrint.simplifiedExpressionAsString(guard).simplifiedString + ") ");
		}
		if(lhs != null && rhs != null) {
			if (isOutputBlock)
				stmtString.append("Print " + rhs.printVisitor().replace("Prime", "") + "\n");
			else
				stmtString.append("\n\t" + lhs.printVisitor() + "=" + rhs.printVisitor().replace("Prime", "") + "\n");
		}
		return stmtString.toString();
	}	
	
	public SimplifyResult toSimplifiedResult() {
		StringBuilder stmtString = new StringBuilder();
		SimplifyResult res = null;
		if(guard != null)
		res = SimplifyPrint.simplifiedExpressionAsString(guard);
		stmtString.append(res.simplifiedString + ':');		
		if(lhs != null && rhs != null)
			stmtString.append(lhs.printVisitor() + "=" + rhs.printVisitor() + "\n");
		return new SimplifyResult(stmtString.toString(), res.originalSize, res.newSize);
	}
	
	public Expression getGuard() {
		return guard;
	}

	public Expression getLhs() {
		return lhs;
	}
	
	public boolean isGlobalInit() {
		return isGlobalInit;
	}

	public void setGlobalInit(boolean isGlobalInit) {
		this.isGlobalInit = isGlobalInit;
	}

	public Expression getRhs() {
		return rhs;
	}

	public void setLhs(Expression lhs) {
		this.lhs = lhs;
	}

	public void setRhs(Expression rhs) {
		this.rhs = rhs;
	}

	public void setGuard(Expression guard) {
		this.guard = guard;
	}
	
	@Override
	public AssignmentStatement clone() {
		Expression g =  guard == null ? null : (Expression) guard.clone();
		Expression l =  lhs == null ? null : (Expression) lhs.clone();
		Expression r =  rhs == null ? null : (Expression) rhs.clone();
		return new AssignmentStatement(isGlobalInit, g, l, r, newline);
	}

	
	
	// return a set of assignment statements that assign to rectangular regions
	// of the array that are assigned to by 'earlier' but not 'later'
	// The assumption here is that 'earlier' and 'later' are assignment 
	// statements that assign to 2D arrays.
	// the origin is at top left. rows increase to the south, and cols to the 
	// east
	// This is really finding set difference: earlier - later.
	public static Set<AssignmentStatement> find2DIntersection(AssignmentStatement earlier, 
			AssignmentStatement later){
		Set<AssignmentStatement> s = new HashSet<AssignmentStatement>();
		//bounds for the earlier assignment
		Expression eRowMin = getRowMin(earlier);
		Expression eRowMax = getRowMax(earlier);
		Expression eColMin = getColMin(earlier);
		Expression eColMax = getColMax(earlier);
		
		//bounds for the later assignment
		Expression lRowMin = getRowMin(later);
		Expression lRowMax = getRowMax(later);
		Expression lColMin = getColMin(later);
		Expression lColMax = getColMax(later);
		
		Expression rhs = earlier.getRhs();
		
		AssignmentStatement top = null, bottom = null, left = null, right = null;
		
		if(Expression.lessThan(eRowMin, lRowMin)){
			Expression temp = create2DSubArray(earlier, eRowMin, Expression.subtractOne(lRowMin), 
					eColMin, eColMax);
			top = new AssignmentStatement(earlier.getGuard(), temp, rhs);
		}
		
		if(Expression.lessThan(lRowMax, eRowMax)){
//		if(Expression.lessThan(lRowMax, eRowMax)){
			Expression temp = create2DSubArray(earlier, Expression.addOne(lRowMax), 
					eRowMax, eColMin, eColMax);
			bottom = new AssignmentStatement(earlier.getGuard(), temp, rhs);
		}
		
		if(Expression.lessThan(eColMin, lColMin)){
			Expression temp = create2DSubArray(earlier,  
			  Expression.getMax(eRowMin, lRowMin), Expression.getMin(eRowMax, lRowMax),
			  eColMin, Expression.subtractOne(lColMin));
			left = new AssignmentStatement(earlier.getGuard(), temp, rhs);
		}
		
//		if(Expression.lessThanOrEquals(lColMax, eColMax)){
		if(Expression.lessThan(lColMax, eColMax)){
			Expression temp = create2DSubArray(earlier,  
					  Expression.getMax(eRowMin, lRowMin), Expression.getMin(eRowMax, lRowMax),
					  Expression.addOne(lColMax), eColMax);
			right = new AssignmentStatement(earlier.getGuard(), temp, rhs);
		}
		
		if(top!=null)
			s.add(top);
		
		if(bottom!=null)
			s.add(bottom);
		
		if(left!=null)
			s.add(left);
		
		if(right!=null)
			s.add(right);
		
		return s;
	}
	
	public static Set<AssignmentStatement> find1DIntersection(AssignmentStatement earlier, 
			AssignmentStatement later){
		Set<AssignmentStatement> s = new HashSet<AssignmentStatement>();
		//bounds for the earlier assignment
		Expression eColMin = getColMin(earlier);
		Expression eColMax = getColMax(earlier);
		
		//bounds for the later assignment
		Expression lColMin = getColMin(later);
		Expression lColMax = getColMax(later);
		
		Expression rhs = earlier.getRhs();
		
		AssignmentStatement left = null, right = null;
		
		if(Expression.lessThan(eColMin, lColMin)){
			Expression temp = create1DSubArray(earlier, eColMin, Expression.subtractOne(lColMin));
			left = new AssignmentStatement(earlier.getGuard(), temp, rhs);
		}
		
		if(Expression.lessThan(lColMax, eColMax)){
			Expression temp = create1DSubArray(earlier, Expression.addOne(lColMax), eColMax);
			right = new AssignmentStatement(earlier.getGuard(), temp, rhs);
		}
		
		if(left!=null)
			s.add(left);
		
		if(right!=null)
			s.add(right);
		
		return s;
	}
	
	// Finding intersection between two statements.
	public static AssignmentStatement find2DStmtIntersection(AssignmentStatement firstStmt, 
			AssignmentStatement secondStmt){
//		Set<AssignmentStatement> s = new HashSet<AssignmentStatement>();
		//bounds for the first assignment
		Expression fRowMin = getRowMin(firstStmt);
		Expression fRowMax = getRowMax(firstStmt);
		Expression fColMin = getColMin(firstStmt);
		Expression fColMax = getColMax(firstStmt);
		
		//bounds for the second assignment
		Expression sRowMin = getRowMin(secondStmt);
		Expression sRowMax = getRowMax(secondStmt);
		Expression sColMin = getColMin(secondStmt);
		Expression sColMax = getColMax(secondStmt);
		
		Expression rhs = firstStmt.getRhs();
		
		Expression intersectRowMin, intersectRowMax, intersectColMin, intersectColMax;
		
		if(Expression.lessThan(fRowMin, sRowMin)){
			intersectRowMin = sRowMin;
		} else {
			intersectRowMin = fRowMin;
		}
		
		if(Expression.lessThanOrEquals(sRowMax, fRowMax)){
			intersectRowMax = sRowMax;
		} else {
			intersectRowMax = fRowMax;
		}
		
		if(Expression.lessThan(fColMin, sColMin)){
			intersectColMin = sColMin;
		} else {
			intersectColMin = fColMin;
		}
		
		if(Expression.lessThanOrEquals(sColMax, fColMax)){
			intersectColMax = sColMax;
		} else {
			intersectColMax = fColMax;
		}
		
		Expression temp = create2DSubArray(firstStmt, intersectRowMin, intersectRowMax, intersectColMin, intersectColMax);
		// If the intersection we find has invalid ranges, it means empty intersection.
		if (Expression.lessThan(intersectRowMax, intersectRowMin) || Expression.lessThan(intersectColMax, intersectColMin))
			return null;
		AssignmentStatement s = new AssignmentStatement(firstStmt.getGuard(), temp, rhs);
		return s;
	}
	
	
	public static AssignmentStatement find1DStmtIntersection(AssignmentStatement firstStmt, 
			AssignmentStatement secondStmt){
//		Set<AssignmentStatement> s = new HashSet<AssignmentStatement>();
		//bounds for the first assignment
		Expression fColMin = getColMin(firstStmt);
		Expression fColMax = getColMax(firstStmt);
		
		//bounds for the second assignment
		Expression sColMin = getColMin(secondStmt);
		Expression sColMax = getColMax(secondStmt);
		
		Expression rhs = firstStmt.getRhs();
		
		Expression intersectColMin, intersectColMax;
		
		if(Expression.lessThan(fColMin, sColMin)){
			intersectColMin = sColMin;
		} else {
			intersectColMin = fColMin;
		}
		
		if(Expression.lessThanOrEquals(sColMax, fColMax)){
			intersectColMax = sColMax;
		} else {
			intersectColMax = fColMax;
		}
		
		Expression temp = create1DSubArray(firstStmt, intersectColMin, intersectColMax);
		// If the intersection we find has invalid ranges, it means empty intersection.
		if (Expression.lessThan(intersectColMax, intersectColMin))
			return null;
		AssignmentStatement s = new AssignmentStatement(firstStmt.getGuard(), temp, rhs);
		return s;
	}

	public static Expression create2DSubArray(AssignmentStatement A, Expression rowMin,
			Expression rowMax, Expression colMin, Expression colMax) {
		Expression rowDim = Expression.CreateRangeExpression(rowMin, rowMax);
		Expression colDim = Expression.CreateRangeExpression(colMin, colMax);
		Expression lhs = Expression.Create2DArray(A.getLhs().getName(), rowDim, colDim);
		return lhs;
	}
	
	public static Expression create1DSubArray(AssignmentStatement A, 
			Expression colMin, Expression colMax) {
		Expression colDim = Expression.CreateRangeExpression(colMin, colMax);
		Expression lhs = Expression.Create1DArray(A.getLhs().getName(), colDim);
		return lhs;
	}
	
	// Does the range "this" assigns to contain the range that B assigns to?
	public boolean contains(AssignmentStatement B, int dimSize){
		if (dimSize == 2) {
			Expression AXMin = getColMin(this);
			Expression AXMax = getColMax(this);
			Expression AYMin = getRowMin(this);
			Expression AYMax = getRowMax(this);
		
			Expression BXMin = getColMin(B);
			Expression BXMax = getColMax(B);
			Expression BYMin = getRowMin(B);
			Expression BYMax = getRowMax(B);
			return(
				Expression.lessThanOrEquals(AXMin, BXMin) &&
				Expression.lessThanOrEquals(BXMax, AXMax) &&
				Expression.lessThanOrEquals(AYMin, BYMin) &&
				Expression.lessThanOrEquals(BYMax, AYMax) 
			  );	
		} else if (dimSize == 1) {
			Expression AXMin = getColMin(this);
			Expression AXMax = getColMax(this);
			Expression BXMin = getColMin(B);
			Expression BXMax = getColMax(B);
			
			return (
				Expression.lessThanOrEquals(AXMin, BXMin) &&
				Expression.lessThanOrEquals(BXMax, AXMax)
			);
		} else
			return false;
	}

	// Does the range this assigns to intersect the range that B assigns to?
	public boolean intersects(AssignmentStatement B, int dimSize){
		if (dimSize == 2) {
			Expression AXMin = getColMin(this);
			Expression AXMax = getColMax(this);
			Expression AYMin = getRowMin(this);
			Expression AYMax = getRowMax(this);
			Expression BXMin = getColMin(B);
			Expression BXMax = getColMax(B);
			Expression BYMin = getRowMin(B);
			Expression BYMax = getRowMax(B);
			return(
				Expression.lessThanOrEquals(AXMin, BXMax) &&
				Expression.lessThanOrEquals(BXMin, AXMax) &&
				Expression.lessThanOrEquals(AYMin, BYMax) &&
				Expression.lessThanOrEquals(BYMin, AYMax) 
			  );
		} else if (dimSize == 1) {
			Expression AXMin = getColMin(this);
			Expression AXMax = getColMax(this);
			Expression BXMin = getColMin(B);
			Expression BXMax = getColMax(B);
			return(
				(Expression.lessThanOrEquals(AXMin, BXMin) && Expression.lessThanOrEquals(BXMin, AXMax)) ||
				(Expression.lessThanOrEquals(BXMin, AXMin) && Expression.lessThanOrEquals(AXMin, BXMax))
				);
		}

		return false;
	}
	
	public static boolean is2DAssignment(AssignmentStatement A){
		Expression lhs = A.getLhs();
		if(lhs.EType == ExprType.ARRAY)
			if(lhs.geteList().size() == 2)
				return true;
		return false;
	}
	
	// is to be called only on 2D assignments
	public static Expression getRowMin(AssignmentStatement A){
		Expression lhs = A.getLhs();
		Expression x = lhs.geteList().get(0);
		if(x.EType == ExprType.CONST || x.EType == ExprType.ID)
			return x;
		if(x.EType == ExprType.BINARY){
			if(x.operator.compareTo("...") == 0)
				return x.eList.get(0);
			else return x;
		}
//		assert(false);
		return x;
	}
	
	public static Expression getRowMax(AssignmentStatement A){
		Expression lhs = A.getLhs();
		Expression x = lhs.geteList().get(0);
		if(x.EType == ExprType.CONST || x.EType == ExprType.ID)
			return x;
		if(x.EType == ExprType.BINARY){
			if(x.operator.compareTo("...") == 0)
				return x.eList.get(1);
			else return x;
		}
//		assert(false);
		return x;
	}
	
	public static Expression getColMin(AssignmentStatement A){
		Expression lhs = A.getLhs();
		int dimSize = lhs.geteList().size();
		if (dimSize == 2) {
			Expression y = lhs.geteList().get(1);
			if(y.EType == ExprType.CONST || y.EType == ExprType.ID)
				return y;
			else if(y.EType == ExprType.BINARY){
				if(y.operator.compareTo("...") == 0)
					return y.eList.get(0);
				else return y;
			}	
			return y;	
		} else if (dimSize == 1) {
			Expression y = lhs.geteList().get(0);
			if (y.EType == ExprType.CONST || y.EType == ExprType.ID)
				return y;
			if (y.EType == ExprType.BINARY)
				if (y.operator.compareTo("...") == 0)
					return y.eList.get(0);
			return y;
		}
//				
//		assert(false);
		return null;
	}
	
	public static Expression getColMax(AssignmentStatement A){
		Expression lhs = A.getLhs();
		int dimSize = lhs.geteList().size();
		if (dimSize == 2) {
			Expression y = lhs.geteList().get(1);
			if(y.EType == ExprType.CONST || y.EType == ExprType.ID)
				return y;
			if(y.EType == ExprType.BINARY){
				if(y.operator.compareTo("...") == 0)
					return y.eList.get(1);
				else return y;
			}
			return y;
		} else if (dimSize == 1) {
			Expression y = lhs.geteList().get(0);
			if(y.EType == ExprType.CONST || y.EType == ExprType.ID)
				return y;
			if(y.EType == ExprType.BINARY)
				if(y.operator.compareTo("...") == 0)
					return y.eList.get(1);
			return y;
		}
//				
//		assert(false);
		return null;
	}
	
	public Set<Expression> variableVisitor(){
		Set<Expression> vars = new HashSet<Expression>();
		if(guard!=null)
			vars.addAll(guard.arrayExpressionVisitor());
		if(lhs != null){
			vars.addAll(lhs.arrayExpressionVisitor());
			// Handle dummy variable in output summaries
			if(lhs.EType == ExprType.ID)
				vars.add(lhs);
		}
		if(rhs != null)
			vars.addAll(rhs.arrayExpressionVisitor());
		return vars;	
	}
	
	public void renameVisitor(Map<Expression, Expression> bindings){
		if(guard!=null)
			guard.renameVisitor(bindings);
		if(lhs != null)
			lhs.renameVisitor(bindings);
		if(rhs != null)
			rhs.renameVisitor(bindings);
	}
	
	public int getNestingDepth(){
		return 0;
	}
	
	public void populateTypes(Map<Expression, String> types){
		if(lhs!=null)
			lhs.typeVisitor(types, "");
		if(rhs != null)
			rhs.typeVisitor(types, "");
    	if(guard != null)
    		guard.typeVisitor(types, "");
    	if(types.get(lhs).compareTo("") == 0){
    		assert lhs.EType == ExprType.ID;
    		if(types.containsKey(rhs)) 
    			types.put(lhs, types.get(rhs));
    		else
    			types.put(lhs, rhs.getType(types));
    	}	
	}
	
	public String getSMTFormula(){
		StringBuilder str = new StringBuilder();
		str.append("(assert ");
		StringBuilder eqConstraint = new StringBuilder();
		if(lhs != null &&  rhs != null){
			eqConstraint.append("(= " );
			eqConstraint.append(lhs.SMTVisitor() + " ");
			eqConstraint.append(rhs.SMTVisitor());
			eqConstraint.append(")");
		}
		else{
			eqConstraint.append("True");
		}
		if(guard != null){
			str.append( "(=> ");
			str.append(guard.SMTVisitor());
			str.append(eqConstraint);
			str.append(")");
		}
		else
			str.append(eqConstraint);
		str.append(") ;");
		str.append(this.toString().replace("\n", "") + "\n");
		//str.append("\n");
		return str.toString();
	}
	
	public static void makeDisjoint(List<AssignmentStatement> l, List<AssignmentStatement> result){
		List<Path> acc = new ArrayList<Path>();
		AssignmentStatement first = l.get(0);
		
		Expression lhs, rhs;
		lhs = (Expression)first.getLhs().clone();
		rhs = (Expression)first.getRhs().clone();
		Path pTrue = new Path(first.getGuard(), lhs, rhs);
		pTrue.updateStore(lhs, rhs);
		acc.add(pTrue);
		
		Path pFalse = new Path(Expression.negate(first.getGuard()), null, null);
		
		// under the false guard, the first statement does not modify the lhs
		acc.add(pFalse);
		l.remove(0);
		makeGuardsDisjoint(l, acc, result);
	}
	
	/*Do we need to rewrite guards as well?*/
	private static void makeGuardsDisjoint(List<AssignmentStatement> l, 
										  List<Path> accumulator,
										  List<AssignmentStatement> result){
		if(l.size() == 0){
			for(Path p: accumulator){
				result.add(new AssignmentStatement(p.pathCondition, p.lhs, p.rhs));
			}
			return;
		}
		AssignmentStatement current = l.get(0);
		l.remove(0);
		Expression currentGuard = current.getGuard();
		assert(currentGuard != null);
		List<Path> pcSoFar = new ArrayList<Path>();
		for(Path p: accumulator){
			Expression pathCondition = p.pathCondition;
			
			Expression guardTrueConstraint = Expression.conjunct(pathCondition, currentGuard);
			Expression guardFalseConstraint = Expression.conjunct(pathCondition, Expression.negate(currentGuard));
			
			
			Path pFalse = new Path(guardFalseConstraint, p.lhs, p.rhs, p.store);
			pcSoFar.add(pFalse);
			
			Expression lhs = (Expression) current.getLhs().clone();
			Expression guardTrueRHS = (Expression) current.getRhs().clone();
			for(Map.Entry<Expression, Expression> entry : p.store.entrySet()){
				guardTrueRHS.replace(entry.getKey(), (Expression) entry.getValue().clone());
			}
			
			Path pTrue = new Path(guardTrueConstraint, current.getLhs(), guardTrueRHS, p.store);
			pTrue.updateStore(lhs, guardTrueRHS);
			pcSoFar.add(pTrue);
		}
		accumulator.clear();
		accumulator.addAll(pcSoFar);
		makeGuardsDisjoint(l, accumulator, result);
	}

	public void renameIdentifiers(Map<String, String> bindings) {
		if(guard!=null)
			guard.renameIdentifiers(bindings);
		lhs.renameIdentifiers(bindings);
		rhs.renameIdentifiers(bindings);	
	}
	
	public static void main(String[] args) {
		ProgramInputs Inps = new ProgramInputs();
		Inps.readInputConstraints(args[0]);
		
		AssignmentStatement stmt1;
		Expression guard = new Expression(ExprType.CONST, "", "true");
		Expression lower = new Expression(ExprType.BINARY, "+", new Expression(ExprType.ID, "", "dp_ProgInput_3"), new Expression(ExprType.CONST, "", "1"), "");
		Expression dim1 = Expression.CreateRangeExpression(lower, new Expression(ExprType.CONST, "", "20001"));
		Expression dim2 = Expression.CreateRangeExpression(new Expression(ExprType.CONST, "", "0"), new Expression(ExprType.CONST, "", "21"));
		Expression lhs = Expression.Create2DArray("dp", dim1, dim2);
		Expression rhs = new Expression(ExprType.CONST, "", "false");
		stmt1 = new AssignmentStatement(guard, lhs, rhs);
		
		AssignmentStatement stmt2;
		dim1 = Expression.CreateRangeExpression(new Expression(ExprType.CONST, "", "0"), new Expression(ExprType.ID, "", "dp_ProgInput_2"));
		dim2 = Expression.CreateRangeExpression(new Expression(ExprType.CONST, "", "0"), new Expression(ExprType.CONST, "", "0"));
		lhs = Expression.Create2DArray("dp", dim1, dim2);
		rhs = new Expression(ExprType.CONST, "", "0");
		stmt2 = new AssignmentStatement(guard, lhs, rhs);
		
		AssignmentStatement intersection;
		intersection = AssignmentStatement.find2DStmtIntersection(stmt1, stmt2);
		
		if (intersection != null)
			System.out.println(intersection.toString());
		else
			System.out.println("NULL");
	}

	public void renameLhsInRhs() {
		if(rhs.contains(lhs)){
			Expression renamedLhs = ((Expression) lhs.clone());
			renamedLhs.setName(lhs.getName() + "Prime");
			rhs.replace(lhs, renamedLhs);
		}
	}

	public Statement unscrunch(){
		if(lhs.EType != ExprType.ARRAY)
			return this;
		if(!lhs.hasOperator("...")){
			return this;
		}
		if(lhs.eList.size() == 1){
			List<Statement> blk = new ArrayList<Statement>();
			Expression ivar = new Expression(ExprType.ID, "", "i");
			Expression newLhs = (Expression) lhs.clone();
			newLhs.eList.set(0, ivar);
			AssignmentStatement aStmt = new AssignmentStatement(guard, newLhs, rhs);
			blk.add(aStmt);
			return new LoopStatement(Expression.getConst("1"), ivar, 
					lhs.eList.get(0).eList.get(0), 
					lhs.eList.get(0).eList.get(1), blk);
		}
		Expression dim1 = lhs.eList.get(0);
		Expression dim2 = lhs.eList.get(1);
		boolean outer = dim1.hasOperator("...");
		boolean inner = dim2.hasOperator("...");
		if(outer && inner){
			List<Statement> blk = new ArrayList<Statement>();
			Expression outerindex = new Expression(ExprType.ID, "", "i");
			Expression innerindex = new Expression(ExprType.ID, "", "j");
			Expression olb = dim1.eList.get(0);
			Expression oub = dim1.eList.get(1);
			Expression ilb = dim2.eList.get(0);
			Expression iub = dim2.eList.get(1);
			Expression newLhs = (Expression) lhs.clone();
			newLhs.eList.set(0, outerindex);
			newLhs.eList.set(1, innerindex);
			AssignmentStatement aStmt = new AssignmentStatement(guard, newLhs, rhs);
			blk.add(aStmt);
			LoopStatement innerloop = new LoopStatement(Expression.getConst("1"), innerindex, ilb, iub, blk);
			List<Statement> oblk = new ArrayList<Statement>();
			oblk.add(innerloop);
			LoopStatement outerloop = new LoopStatement(Expression.getConst("1"), outerindex, olb, oub, oblk);
			return outerloop;
		}
		if(!outer && inner){
			List<Statement> blk = new ArrayList<Statement>();
			Expression innerindex = new Expression(ExprType.ID, "", "j");
			Expression ilb = dim2.eList.get(0);
			Expression iub = dim2.eList.get(1);
			Expression newLhs = (Expression) lhs.clone();
			newLhs.eList.set(0, lhs.geteList().get(0));
			newLhs.eList.set(1, innerindex);
			AssignmentStatement aStmt = new AssignmentStatement(guard, newLhs, rhs);
			blk.add(aStmt);
			LoopStatement innerloop = new LoopStatement(Expression.getConst("1"), innerindex, ilb, iub, blk);
			return innerloop;
		}
		List<Statement> blk = new ArrayList<Statement>();
		Expression outerindex = new Expression(ExprType.ID, "", "i");
		Expression olb = dim1.eList.get(0);
		Expression oub = dim1.eList.get(1);
		Expression newLhs = (Expression) lhs.clone();
		newLhs.eList.set(0, outerindex);
		newLhs.eList.set(1, lhs.geteList().get(1));
		AssignmentStatement aStmt = new AssignmentStatement(guard, newLhs, rhs);
		blk.add(aStmt);
		LoopStatement outerloop = new LoopStatement(Expression.getConst("1"), outerindex, olb, oub, blk);
		return outerloop;
	}
	
	public boolean hasNewline() {
		return newline;
	}

	public void setNewline() {
		newline = true;
	}
}
