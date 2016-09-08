#ifndef FUNCTIONANALYSIS_H
#define FUNCTIONANALYSIS_H

#include "includes.h"
#include "clang/Analysis/CFG.h"
#include "Helper.h"
//#include "StmtDetails.h"

class ExprDetails;
class StmtDetails;
class SymbolicExpr;
class GetSymbolicExprVisitor;
class MainFunction;
class InputVar;
class FunctionAnalysis;

class Definition {
public:
    int lineNumber;
    unsigned blockID;
    const Expr* expression;
    std::string expressionStr;
    std::set<std::pair<unsigned, unsigned> >  backEdgesTraversed;
    bool throughBackEdge; // true if the definition traversed a backedge in the CFG.
    bool isSpecialExpr; // true if this definition is a special expr identified

    bool toDelete; // additional member declared to use std::remove_if

    Definition() {
	lineNumber = -1;
	expression = NULL;
	expressionStr = "";
	backEdgesTraversed = std::set<std::pair<unsigned, unsigned> >();
	throughBackEdge = false;
	isSpecialExpr = false;
	toDelete = false;
    }

    bool operator<(const Definition& rhs) const {
	return ((lineNumber < rhs.lineNumber) || ((lineNumber ==
	    rhs.lineNumber) && (expressionStr.compare(rhs.expressionStr) < 0)));
    }

    bool equals(Definition d) {
	if (d.lineNumber != lineNumber) return false;
	if (d.blockID != blockID) return false;
	if (d.expressionStr.compare(expressionStr) != 0) return false;
	if (d.throughBackEdge != throughBackEdge) return false;
	if (d.isSpecialExpr != isSpecialExpr) return false;
	if (d.toDelete != toDelete) return false;
	if (d.backEdgesTraversed.size() != backEdgesTraversed.size())
	    return false;
	std::set<std::pair<unsigned, unsigned> >::iterator dIt, it;
	dIt = d.backEdgesTraversed.begin();
	it = backEdgesTraversed.begin();
	while (dIt != d.backEdgesTraversed.end() && it !=
		    d.backEdgesTraversed.end()) {
	    if (dIt->first != it->first) return false;
	    if (dIt->second != it->second) return false;
	    dIt++;it++;
	}
	if (dIt != d.backEdgesTraversed.end()) return false;
	if (it != backEdgesTraversed.end()) return false;

	return true;
    }

    Definition clone() {
	Definition cloneObj;
	cloneObj.lineNumber = lineNumber;
	cloneObj.blockID = blockID;
	cloneObj.expression = expression;
	cloneObj.expressionStr = expressionStr;
	cloneObj.throughBackEdge = throughBackEdge;
	cloneObj.isSpecialExpr = isSpecialExpr;
	for (std::set<std::pair<unsigned, unsigned> >::iterator it = 
		backEdgesTraversed.begin(); it != backEdgesTraversed.end();
		it++) {
	    std::pair<unsigned, unsigned> e;
	    e.first = it->first;
	    e.second = it->second;
	    cloneObj.backEdgesTraversed.insert(e);
	}

	return cloneObj;
    }
    void print() const;

};

typedef std::pair<ExprDetails*, bool> GUARD;

class GuardedDefinition {
public:
    // A guard is a pair <expression, truth value>. truth value tells whether
    // the expression occurs in its true form or its negated form in the guard.
    std::vector<std::vector<GUARD> > guards;
    Definition def;

    GuardedDefinition clone() {
	GuardedDefinition cloneObj;
	for (std::vector<std::vector<GUARD> >::iterator gIt = guards.begin(); gIt != 
		guards.end(); gIt++) {
	    std::vector<GUARD> guardVec;
	    for (std::vector<GUARD>::iterator vIt = gIt->begin(); vIt !=
		    gIt->end(); vIt++)
		guardVec.push_back(std::make_pair(vIt->first, vIt->second));
	    //cloneObj.guards.push_back(std::pair<ExprDetails*,
		//bool>(gIt->first->clone(), gIt->second));
	    cloneObj.guards.push_back(guardVec);
	}
	cloneObj.def = def.clone();
	return cloneObj;
    }

    bool equals(GuardedDefinition g);

    void print();
};

class LoopDetails {
public:
    struct inductionVar {
	//std::string varName;
	//int varDeclLine;
	VarDetails var;
	std::vector<std::pair<int, int> > inductionLinesAndSteps;
	bool toDelete;
    };

    VarDetails loopIndexVar;
    int loopStartLine;
    int loopEndLine;
    int loopStep;

    std::string loopIndexInitVal;
    Definition loopIndexInitValDef;
    SymbolicExpr* loopIndexInitValSymExpr;

    std::string loopIndexFinalVal;
    Definition loopIndexFinalValDef;
    SymbolicExpr* loopIndexFinalValSymExpr;

    bool strictBound; // true if the finalval is a strict upper/lower bound
    bool upperBound; // true if the finalval is an upper bound

    unsigned loopHeaderBlock;
    std::pair<unsigned, unsigned> backEdge;
    bool isDoWhileLoop;
    bool hasBreak;
    std::map<unsigned, std::set<unsigned> >
    blockIDsOfAllBreaksInLoopToBlocksGuardingThem;
    std::vector<LoopDetails*> loopsNestedWithin;

    std::vector<struct inductionVar> inductionVars;
    //std::vector<Definition> setOfLoopIndexInitValDefs;
    std::vector<std::pair<Definition, SymbolicExpr*> >
    setOfLoopIndexInitValDefs;
    std::vector<LoopDetails*> loopIndexInitValFromLoop; // In case the initial value is from the loop index of another loop

    LoopDetails() {
	loopIndexInitValSymExpr = NULL;
	loopIndexFinalValSymExpr = NULL;
	isDoWhileLoop = false;
	hasBreak = false;
    }

    ~LoopDetails() {
    }

    void prettyPrint(std::ofstream &logFile, const MainFunction* mObj, bool &rv,
    bool inputFirst=true, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>());
    void prettyPrintHere();

    bool equals(LoopDetails l) {
	if (!(loopIndexVar.equals(l.loopIndexVar))) return false;
	if (loopStartLine != l.loopStartLine) return false;
	if (loopEndLine != l.loopEndLine) return false;
	if (loopStep != l.loopStep) return false;
	if (!(loopIndexInitValDef.equals(l.loopIndexInitValDef))) return false;
	if (!(loopIndexFinalValDef.equals(l.loopIndexFinalValDef))) return false;
	if (strictBound != l.strictBound) return false;
	if (upperBound != l.upperBound) return false;
	if (loopHeaderBlock != l.loopHeaderBlock) return false;
	if (backEdge.first != l.backEdge.first) return false;
	if (backEdge.second != l.backEdge.second) return false;
	return true;
    }

    bool equals(LoopDetails* l) {
	if (!(loopIndexVar.equals(l->loopIndexVar))) return false;
	if (loopStartLine != l->loopStartLine) return false;
	if (loopEndLine != l->loopEndLine) return false;
	if (loopStep != l->loopStep) return false;
	if (!(loopIndexInitValDef.equals(l->loopIndexInitValDef))) return false;
	if (!(loopIndexFinalValDef.equals(l->loopIndexFinalValDef))) return false;
	if (strictBound != l->strictBound) return false;
	if (upperBound != l->upperBound) return false;
	if (loopHeaderBlock != l->loopHeaderBlock) return false;
	if (backEdge.first != l->backEdge.first) return false;
	if (backEdge.second != l->backEdge.second) return false;
	return true;
    }

    LoopDetails clone() {
	LoopDetails cloneObj;
	//cloneObj.loopIndexVar.varName = loopIndexVar.varName;
	//cloneObj.loopIndexVar.varDeclLine = loopIndexVar.varDeclLine;
	cloneObj.loopIndexVar = loopIndexVar.clone();
	cloneObj.loopStartLine = loopStartLine;
	cloneObj.loopEndLine = loopEndLine;
	cloneObj.loopStep = loopStep;
	cloneObj.loopIndexInitVal = loopIndexInitVal;
	cloneObj.loopIndexInitValDef = loopIndexInitValDef.clone();
	cloneObj.loopIndexInitValSymExpr = loopIndexInitValSymExpr;
	cloneObj.loopIndexInitValFromLoop.insert(cloneObj.loopIndexInitValFromLoop.end(),
	    loopIndexInitValFromLoop.begin(), loopIndexInitValFromLoop.end());
	cloneObj.loopIndexFinalVal = loopIndexFinalVal;
	cloneObj.loopIndexFinalValDef = loopIndexFinalValDef.clone();
	cloneObj.loopIndexFinalValSymExpr = loopIndexFinalValSymExpr;
	cloneObj.strictBound = strictBound;
	cloneObj.upperBound = upperBound;
	cloneObj.loopHeaderBlock = loopHeaderBlock;
	cloneObj.backEdge.first = backEdge.first;
	cloneObj.backEdge.second = backEdge.second;
	cloneObj.isDoWhileLoop = isDoWhileLoop;
	cloneObj.hasBreak = hasBreak;
	cloneObj.blockIDsOfAllBreaksInLoopToBlocksGuardingThem.insert(
	    blockIDsOfAllBreaksInLoopToBlocksGuardingThem.begin(), 
	    blockIDsOfAllBreaksInLoopToBlocksGuardingThem.end());

	cloneObj.loopsNestedWithin.insert(cloneObj.loopsNestedWithin.end(),
	    loopsNestedWithin.begin(), loopsNestedWithin.end());

	for (std::vector<struct inductionVar>::iterator it =
		inductionVars.begin(); it != inductionVars.end(); it++) {
	    struct inductionVar iv;
	    //iv.varName = it->varName;
	    //iv.varDeclLine = it->varDeclLine;
	    iv.var = it->var.clone();
	    for (std::vector<std::pair<int, int> >::iterator iIt =
		    it->inductionLinesAndSteps.begin(); iIt !=
		    it->inductionLinesAndSteps.end(); iIt++) {
		std::pair<int, int> ils;
		ils.first = iIt->first;
		ils.second = iIt->second;
		iv.inductionLinesAndSteps.push_back(ils);
	    }

	    cloneObj.inductionVars.push_back(iv);
	}

	for (std::vector<std::pair<Definition, SymbolicExpr*> >::iterator it =
		setOfLoopIndexInitValDefs.begin(); it !=
		setOfLoopIndexInitValDefs.end(); it++) {
	    std::pair<Definition, SymbolicExpr*> p;
	    p.first = it->first.clone();
	    p.second = it->second;
	    cloneObj.setOfLoopIndexInitValDefs.push_back(p);
	}

	return cloneObj;
    }

    void print();
    void getSymbolicValuesForLoopHeader(const SourceManager* SM,
    GetSymbolicExprVisitor* symVisitor, bool &rv);

    // Checks whether the number of iterations of the loop is equal to the
    // variable given as argument. We only need this for the testcase var.
    bool isTestCaseLoop(InputVar* testCaseVar, bool &rv);

    void setLoopIndexInitValDef(FunctionAnalysis* FA, bool &rv);
};

class SpecialExpr: public Definition {
public:
    SpecialExprKind kind;
    VarDetails scalarVar;
    VarDetails arrayVar;
    // lower bound and upper bound of array range
    std::pair<Expr*, Expr*> arrayRange;
    std::pair<std::string, std::string> arrayRangeStr;
    std::vector<Expr*> arrayIndexExprsAtAssignLine;
    // initial value of special expr
    //std::vector<Definition> initialVal;
    //std::vector<std::string> initialValStr;
    Definition initialVal;
    std::string initialValStr;
    // Loop associated with this special expr
    std::vector<LoopDetails*> loops;
    bool isLoopSet; // true if we found the loop

    int assignmentLine;
    unsigned assignmentBlock;
    int guardLine;
    Expr* guardExpr;
    bool fromTernaryOperator;
    std::vector<int> stmtsPartOfExpr;

    bool sane; // true if sane, false if not

    SpecialExpr() {
	kind = UNRESOLVED;
	guardExpr = NULL;
	arrayRange = std::pair<Expr*, Expr*>(NULL, NULL);
	arrayIndexExprsAtAssignLine = std::vector<Expr*>();
	stmtsPartOfExpr = std::vector<int>();
	isLoopSet = false;
	assignmentBlock = 0;
	assignmentLine = -1;
	guardLine = -1;
	fromTernaryOperator = false;
	sane = true;
    }

    SpecialExpr clone() {
	SpecialExpr cloneObj;
	// Populate base class fields
	cloneObj.lineNumber = lineNumber;
	cloneObj.blockID = blockID;
	cloneObj.expression = expression;
	cloneObj.expressionStr = expressionStr;
	cloneObj.throughBackEdge = throughBackEdge;
	cloneObj.isSpecialExpr = isSpecialExpr;
	for (std::set<std::pair<unsigned, unsigned> >::iterator it = 
		backEdgesTraversed.begin(); it != backEdgesTraversed.end();
		it++) {
	    std::pair<unsigned, unsigned> e;
	    e.first = it->first;
	    e.second = it->second;
	    cloneObj.backEdgesTraversed.insert(e);
	}

	cloneObj.kind = kind;
	cloneObj.scalarVar = scalarVar.clone();
	cloneObj.arrayVar = arrayVar.clone();
	cloneObj.arrayRange.first = arrayRange.first;
	cloneObj.arrayRange.second = arrayRange.second;
	cloneObj.arrayRangeStr.first = arrayRangeStr.first;
	cloneObj.arrayRangeStr.second = arrayRangeStr.second;
	cloneObj.arrayIndexExprsAtAssignLine.insert(cloneObj.arrayIndexExprsAtAssignLine.end(), 
	    arrayIndexExprsAtAssignLine.begin(), arrayIndexExprsAtAssignLine.end());
	cloneObj.initialVal = initialVal.clone();
	cloneObj.initialValStr = initialValStr;
	cloneObj.loops.insert(cloneObj.loops.end(), loops.begin(), loops.end());
	cloneObj.isLoopSet = isLoopSet;
	cloneObj.assignmentLine = assignmentLine;
	cloneObj.assignmentBlock = assignmentBlock;
	cloneObj.guardLine = guardLine;
	cloneObj.guardExpr = guardExpr;
	cloneObj.fromTernaryOperator = fromTernaryOperator;
	cloneObj.stmtsPartOfExpr.insert(cloneObj.stmtsPartOfExpr.end(), 
	    stmtsPartOfExpr.begin(), stmtsPartOfExpr.end());
	cloneObj.sane = sane;
	return cloneObj;
    }

    void clone(SpecialExpr* cloneObj) {
	// Populate base class fields
	cloneObj->lineNumber = lineNumber;
	cloneObj->blockID = blockID;
	cloneObj->expression = expression;
	cloneObj->expressionStr = expressionStr;
	cloneObj->throughBackEdge = throughBackEdge;
	cloneObj->isSpecialExpr = isSpecialExpr;
	for (std::set<std::pair<unsigned, unsigned> >::iterator it = 
		backEdgesTraversed.begin(); it != backEdgesTraversed.end();
		it++) {
	    std::pair<unsigned, unsigned> e;
	    e.first = it->first;
	    e.second = it->second;
	    cloneObj->backEdgesTraversed.insert(e);
	}

	cloneObj->kind = kind;
	cloneObj->scalarVar = scalarVar.clone();
	cloneObj->arrayVar = arrayVar.clone();
	cloneObj->arrayRange.first = arrayRange.first;
	cloneObj->arrayRange.second = arrayRange.second;
	cloneObj->arrayRangeStr.first = arrayRangeStr.first;
	cloneObj->arrayRangeStr.second = arrayRangeStr.second;
	cloneObj->arrayIndexExprsAtAssignLine.insert(cloneObj->arrayIndexExprsAtAssignLine.end(), 
	    arrayIndexExprsAtAssignLine.begin(), arrayIndexExprsAtAssignLine.end());
	cloneObj->initialVal = initialVal.clone();
	cloneObj->initialValStr = initialValStr;
	cloneObj->loops.insert(cloneObj->loops.end(), loops.begin(), loops.end());
	cloneObj->isLoopSet = isLoopSet;
	cloneObj->assignmentLine = assignmentLine;
	cloneObj->assignmentBlock = assignmentBlock;
	cloneObj->guardLine = guardLine;
	cloneObj->guardExpr = guardExpr;
	cloneObj->fromTernaryOperator = fromTernaryOperator;
	cloneObj->stmtsPartOfExpr.insert(cloneObj->stmtsPartOfExpr.end(), 
	    stmtsPartOfExpr.begin(), stmtsPartOfExpr.end());
	cloneObj->sane = sane;
    }

    bool equals(SpecialExpr sp) {
	if (!scalarVar.equals(sp.scalarVar)) return false;
	if (!arrayVar.equals(sp.arrayVar)) return false;
	if (assignmentLine != sp.assignmentLine) return false;
	if (guardLine != sp.guardLine) return false;
	return true;
    }

    void print();
};

class FunctionAnalysis {
public:

    const SourceManager* SM;
    ASTContext* Context;
    bool error;
    FunctionDetails* function;
    MainFunction* mainObj;

    class ReachDef {
    public:
	// basic-block-id to (var-id to set-of-defs)
	std::map<unsigned, std::map<std::string, std::set<Definition> > > reachInSet;
	std::map<unsigned, std::map<std::string, std::set<Definition> > > reachOutSet;
	// basic-block-id to (var-id to def)
	std::map<unsigned, std::map<std::string, Definition> > genSet;
	// basic-block-id to (var-id to all-defs-in-block)
	std::map<unsigned, std::map<std::string, std::vector<Definition> > >
	    allDefsInBlock;
	std::map<unsigned, std::vector<std::pair<VarDetails, int> > >
	    blockToVarsDefined;
	std::map<unsigned, std::map<int, std::vector<VarDetails> > >
	    blockLineToVarsUsedInDef;

	bool isVarDefined(VarDetails v, unsigned block, int line) {
	    if (blockToVarsDefined.find(block) == blockToVarsDefined.end())
		return false;
	    for (std::vector<std::pair<VarDetails, int> >::iterator vIt = 
		    blockToVarsDefined[block].begin(); vIt != 
		    blockToVarsDefined[block].end(); vIt++) {
		if (vIt->second != line) continue;
		if (vIt->first.varName.compare(v.varName) == 0 &&
			vIt->first.varDeclLine == v.varDeclLine)
		    return true;
	    }
	    return false;
	}

	bool isVarUsed(VarDetails v, unsigned block, int line) {
	    if (blockLineToVarsUsedInDef.find(block) ==
		    blockLineToVarsUsedInDef.end())
		return false;
	    if (blockLineToVarsUsedInDef[block].find(line) ==
		    blockLineToVarsUsedInDef[block].end())
		return false;
	    for (std::vector<VarDetails>::iterator vIt =
		    blockLineToVarsUsedInDef[block][line].begin(); vIt != 
		    blockLineToVarsUsedInDef[block][line].end(); vIt++) {
		if (vIt->equals(v)) return true;
	    }
	    return false;
	}
    };

    ReachDef rd;

    // Recording all functions
    std::vector<FunctionDetails*> allFunctions;

    // Line number of all input stmts identified in this function.
    std::vector<int> inputLines;
    std::vector<InputVar> inputVars;
    std::vector<StmtDetails*> inputStmts;

    std::vector<VarDetails> parameters;

    // dummy return var generated for this function
    VarDetails returnVar;
    // Symbolic expressions obtained for the return var
    std::vector<SymbolicExpr*> retSymExprs;

    GetSymbolicExprVisitor* symVisitor;
    std::vector<StmtDetails*> residualStmts;

    std::vector<LoopDetails*> allLoopsInFunction;

    // An edge is a pair of basic blocks - (source, dest)
    // In a back edge, destination block is the head of the loop.
    std::set<std::pair<unsigned, unsigned> > backEdges;
    
    // Back edge to set<blocks> containing the loop
    std::map<std::pair<unsigned, unsigned>, std::set<unsigned> > edgesToLoop;

    // Back edge (representing the loop) to loop details
    std::map<std::pair<unsigned, unsigned>, LoopDetails*> edgesToLoopDetails;

    // Basic block to <start-line-number, end-line-number>
    std::map<unsigned, std::pair<int, int> > blockToLines;

    // Line number to basic block ID
    std::map<int, std::set<unsigned> > lineToBlock;

    // If x is control dependent on a and b, then x is mapped to set {a,b}
    // basic-block to set of basic-blocks
    std::map<unsigned, std::set<std::pair<unsigned, bool> > > cdg;

    // Maps a block with a terminator expression to those blocks whose
    // terminator expressions contain the first expression
    std::map<unsigned, std::set<unsigned> > blockToSupersetBlocks;

    // Maps a block with a terminator expression to its subexpressions
    // each of which is represented in basic block notation, i.e. B2.3
    std::map<unsigned, std::vector<std::pair<unsigned, unsigned> > >
    terminatorSubExprs;

    std::map<unsigned, std::vector<std::vector<std::pair<unsigned, bool> > > > 
    transitiveCDG;

    // The dummy entry block used while compute CDG
    unsigned dummyEntryBlockID;

    std::vector<SpecialExpr*> spExprs;

    // Basic block details
    StmtMapTy StmtMap;
    DeclMapTy DeclMap;

    FunctionAnalysis() {
	SM = NULL;
	Context = NULL;
	symVisitor = NULL;
	setMainObjToNULL();
    }

    void setMainObjToNULL();

    FunctionAnalysis(const SourceManager* M, ASTContext* C, MainFunction* main); 

    void populateBasicBlockDetails(std::unique_ptr<CFG> &cfg);

    void getInputVars(std::vector<std::string> customInputFuncs, bool &rv);

    void populateReachingDefinitions(bool &rv);

    // Get reaching definitions of a var at a given line and given block
    std::vector<Definition> getReachDefsOfVarAtLine(VarDetails var, int line,
	unsigned block, bool &rv);

    // Check if the var is a loop index variable in a given block.
    bool isLoopIndexVar(VarDetails var, unsigned block, bool &rv);

    // Get the set of all loops that a given block is part of.
    std::vector<LoopDetails*> getLoopsOfBlock (unsigned block, bool &rv);

    // Get the immediate (deepest/nearest) loop that a given block is part of.
    LoopDetails* getImmediateLoopOfBlock (unsigned block, bool &rv);

    // Check if given edge is a back edge.
    bool isBackEdge(unsigned src, unsigned dest);

    void getBlockDetails(std::unique_ptr<CFG> &cfg, bool &rv);

    void findBackEdgesInCFG(std::unique_ptr<CFG> &cfg, bool &rv);

    // Given a back edge src->dest, find the set of basic blocks that constitute
    // the natural loop associated with the back edge.
    std::set<unsigned> findLoopFromBackEdge(std::unique_ptr<CFG> &cfg, unsigned src, unsigned dest);

    // Compute loops for all back edges in backEdges
    void computeAllLoops(std::unique_ptr<CFG> &cfg, bool &rv);

    LoopDetails* getIndexVariableForLoop(std::unique_ptr<CFG> &cfg, 
	std::set<unsigned> loop, std::pair<unsigned, unsigned> backedge, bool &rv);

    // This function modifies the CFG
    void computeControlDependences(std::unique_ptr<CFG> &cfg, const FunctionDecl* fd, bool &rv);

    // Compute basic blocks that share the same terminator expression
    void computeBlocksWithSameTerminatorExpr(bool &rv);

    // Call this only after computeControlDependences is called.
    void computeTransitiveControlDependences(bool &rv);

    // Get the set of all blocks that guard the given block
    std::vector<std::vector<std::pair<unsigned, bool> > > getGuardsOfBlock(unsigned currBlock);

    ExprDetails* getTerminatorExpressionOfBlock(unsigned currBlock, bool &rv);

    // Get the set of all guard exprs that guard the given block
    std::vector<std::vector<GUARD> > getGuardExprsOfBlock(unsigned currBlock, bool &rv);

    void computeSymbolicExprs(std::unique_ptr<CFG> &cfg, CompoundStmt
	*funcBody, bool &rv);

    void analyzeFunc(bool &rv);
    void getSpecialExprs(bool &rv);
    bool checkIfSpecialExprsAreSane(bool &rv);
    void backSubstitution(bool &rv);

    std::map<unsigned, std::set<unsigned> > getBlocksContainingBreakInLoop(
    std::unique_ptr<CFG> &cfg, std::set<unsigned> loop, bool &rv);

#if 0
    // A guard is <line-num, T/F, guard-expr>.
    // basic-block-id to guard
    std::map<unsigned, std::pair<int, std::pair<bool, std::string> > > guards;
#endif


#if 0
    class AvailExpr {
    public:
	std::string funcName;
	int funcStartLine;
	int funcEndLine;

	// The domain of available expressions are the RHS expressions of all
	// assignment statements.
	// An expr is a 3-tuple <line number of assignment, basic-block-id of
	// assignment, rhs-expr of assignment>
	// basic-block-id to set of exprs.
	std::map<unsigned, std::set<std::pair<std::pair<int, unsigned>, std::string> > > availOutSet;
	std::map<unsigned, std::set<std::pair<std::pair<int, unsigned>, std::string> > > availInSet;
	// basic-block-id to set of exprs.
	std::map<unsigned, std::set<std::pair<std::pair<int, unsigned>, std::string> > > genSet;
	// basic-block-id to set of <line-num, var>
	std::map<unsigned, std::set<std::pair<std::pair<int, unsigned>, std::string> > > killSet;
	// basic-block-id to set of exprs.
	std::map<unsigned, std::vector<std::pair<std::pair<int, unsigned>, std::string> > >
	    allExprsInBlock;	// Might need this to get available expressions
				// at the statement level
    };

    std::vector<AvailExpr> availableExpressions;

    void populateAvailableExpressions(const MatchFinder::MatchResult &Result,
	CFG* cfg, AvailExpr& ae);
#endif
};

#endif /* FUNCTIONANALYSIS_H */
