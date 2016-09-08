#ifndef BLOCKDETAILS_H
#define BLOCKDETAILS_H

#include "FunctionAnalysis.h"
#include "GetSymbolicExprVisitor.h"
#include <z3++.h>

class MainFunction;
class BlockUpdateStmt;
class StmtDetails;

enum BlockStmtKind {
    INPUT, INPUTSCALAR, INPUTARRAY, INPUTEND, INIT, UPDT, OUT
};

class BlockStmt {
private:
    const BlockStmtKind kind;

public:
    StmtDetails* stmt;
    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > guards;
    FunctionDetails* func;
    bool toDelete;
    
    BlockStmt(BlockStmtKind k): kind(k) {
	stmt = NULL;
	func = NULL;
	toDelete = false;
    }

    bool isInput() {
	return (getKind() >= INPUT && getKind() <= INPUTEND);
    }

    bool isInit() {
	return (getKind() == INIT);
    }

    bool isUpdate() {
	return (getKind() == UPDT);
    }

    bool isOutput() {
	return (getKind() == OUT);
    }

    z3::expr getZ3ExprForGuard(z3::context* c, const MainFunction* mObj,
    std::vector<LoopDetails> surroundingLoops, bool inputFirst, bool &rv);

    z3::expr getLoopBoundsAsZ3Expr(z3::context* c, const MainFunction* mObj,
    std::vector<LoopDetails> surroundingLoops, bool inputFirst, bool &rv);

    virtual void print();

    virtual void prettyPrint(std::ofstream &logFile, 
    const MainFunction* mObj, bool &rv, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>());

    virtual void prettyPrintHere() {
    }

#if 0
    void prettyPrintGuards(std::ofstream &logFile, 
    const MainFunction* mObj, bool &rv) {
	rv = true;
	if (guards.size() == 0) {
	    logFile << "(true): ";
	    return;
	}

	logFile << "(";
	for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt
		= guards.begin(); gIt != guards.end(); gIt++) {
	    logFile << "(";
	    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator vIt = 
		    gIt->begin(); vIt != gIt->end(); vIt++) {
		if (!(vIt->second)) logFile << "!";
		logFile << "(";
		vIt->first->prettyPrintSummary(logFile, mObj, true, rv, false);
		if (!rv) {
		    llvm::outs() << "ERROR: While prettyprinting guards\n";
		    return;
		}
		logFile << ")";
		if (vIt+1 != gIt->end()) logFile << " && ";
	    }
	    logFile << ")";
	    if (gIt+1 != guards.end()) logFile << " || ";
	}
	logFile << "): ";
    }
#endif

    void prettyPrintGuards(std::ofstream &logFile,
    const MainFunction* mObj, bool &rv, bool inputFirst = false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	if (guards.size() == 0) {
	    logFile << "(true)";
	    return;
	}
	bool foundTrue = false;
	for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
		>::iterator gIt = guards.begin(); gIt != guards.end(); gIt++) {
	    if (gIt->size() == 0) {
		foundTrue = true; break;
	    }
	}
	if (foundTrue) {
	    logFile << "(true)";
	    return;
	}
	logFile << "(";
	bool printed = false;
	for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
		>::iterator gIt = guards.begin(); gIt != guards.end(); gIt++) {
	    if (gIt->size() == 0) {
		printed = printed || true;
		logFile << "true";
	    }
	    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		    gIt->begin(); sIt != gIt->end(); sIt++) {
		printed = printed || sIt->first->prettyPrintGuardExprs(logFile,
		    mObj, rv, inputFirst, inResidual, surroundingLoops);
		if (!rv) return;
		if (sIt+1 != gIt->end() && printed) logFile << " && ";
	    }
	    if (gIt+1 != guards.end() && printed) logFile << " || ";
	}
	if (printed) logFile << " && ";
	for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
		>::iterator gIt = guards.begin(); gIt != guards.end(); gIt++) {
	    if (gIt->size() == 0) logFile << "true";
	    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		    gIt->begin(); sIt != gIt->end(); sIt++) {
		if (!(sIt->second)) logFile << "!";
		logFile << "(";
		sIt->first->prettyPrintSummary(logFile, mObj, false, rv, false,
		    inResidual, surroundingLoops);
		if (!rv) {
		    llvm::outs() << "ERROR: While prettyprinting guard\n";
		    return;
		}
		logFile << ")";
		if (sIt+1 != gIt->end()) logFile << " && ";
	    }
	    if (gIt+1 != guards.end()) logFile << " || ";
	}
	logFile << ")";
    }

    void prettyPrintGuardsHere() {
	if (guards.size() == 0) {
	    llvm::outs() << "(true): ";
	    return;
	}

	llvm::outs() << "(";
	for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt
		= guards.begin(); gIt != guards.end(); gIt++) {
	    llvm::outs() << "(";
	    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator vIt = 
		    gIt->begin(); vIt != gIt->end(); vIt++) {
		if (!(vIt->second)) llvm::outs() << "!";
		llvm::outs() << "(";
		vIt->first->prettyPrint(false);
		llvm::outs() << ")";
		if (vIt+1 != gIt->end()) llvm::outs() << " && ";
	    }
	    llvm::outs() << ")";
	    if (gIt+1 != guards.end()) llvm::outs() << " || ";
	}
	llvm::outs() << "): ";
    }

    virtual ~BlockStmt() {}
    virtual BlockStmt* clone(bool &rv) {
	rv = true;
	BlockStmt* cloneStmt = new BlockStmt(getKind());
	cloneStmt->stmt = stmt;
	cloneStmt->func = func;
	return cloneStmt;
    }

    virtual void replaceVarsWithExprs(std::vector<VarDetails> parameters, 
    std::vector<SymbolicExpr*> argExprs, bool &rv) {
    }

    void replaceGuardVarsWithExprs(std::vector<VarDetails> parameters,
    std::vector<SymbolicExpr*> argExprs, bool &rv) {
	for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator 
		gIt = guards.begin(); gIt != guards.end(); gIt++) {
	    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator vIt = 
		    gIt->begin(); vIt != gIt->end(); vIt++) {
		vIt->first->replaceVarsWithExprs(parameters, argExprs, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While replacing guards in BlockStmt\n";
		    return;
		}
	    }
	}
    }

    virtual bool containsVar(VarDetails var, bool &rv) {
	rv = false;
	return false;
    }
    std::string getVarIDofStmt(bool &rv);

    BlockStmtKind getKind() const {return kind;}
};

class BlockInputStmt : public BlockStmt {
public:
    BlockInputStmt(): BlockStmt(INPUT) {
    }

    BlockInputStmt(BlockStmtKind k): BlockStmt(k) {
    }

    static bool classof (const BlockStmt* B) {
	return (B->getKind() >= INPUT && B->getKind() <= INPUTEND);
    }

    static std::vector<BlockInputStmt*> getBlockStmt(StmtDetails* st, 
    GetSymbolicExprVisitor* visitor, FunctionDetails* func, bool &rv);
};

class BlockScalarInputStmt : public BlockInputStmt {
public:
    VarDetails inputVar;
    SymbolicExpr* substituteExpr;

    BlockScalarInputStmt(): BlockInputStmt(INPUTSCALAR) {
	substituteExpr = NULL;
    }

    void print() {
	llvm::outs() << "Var: ";
	inputVar.print();
	llvm::outs() << "\n";
    }

    static bool classof (const BlockStmt* B) {
	return B->getKind() == INPUTSCALAR;
    }

    bool containsVar(VarDetails var, bool &rv) {
	rv = true;
	if (inputVar.equals(var))
	    return true;
	if (substituteExpr) {
	    bool ret = substituteExpr->containsVar(var, rv);
	    if (!rv) return false;
	    return ret;
	}
	return false;
    }

    void prettyPrint(std::ofstream &logFile, const MainFunction* mObj, bool &rv,
    bool inResidual=false, std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>());

    void prettyPrintHere() {
	if (func)
	    llvm::outs() << func->funcName << "()\n";
	else
	    llvm::outs() << "NULL func\n";
	prettyPrintGuardsHere();
	if (substituteExpr)
	    substituteExpr->prettyPrint(false);
	else
	    llvm::outs() << inputVar.varName;
	llvm::outs() << " = any;\n";
    }

    BlockStmt* clone(bool &rv) {
	rv = true;
	BlockScalarInputStmt* cloneStmt = new BlockScalarInputStmt;
	for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
		>::iterator gIt = guards.begin(); gIt != guards.end(); gIt++) {
	    std::vector<std::pair<SymbolicExpr*, bool> > gVec;
	    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator vIt = 
		    gIt->begin(); vIt != gIt->end(); vIt++) {
		SymbolicStmt* cloneExpr = vIt->first->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While cloning guardExpr\n";
		    return cloneStmt;
		}
		if (!isa<SymbolicExpr>(cloneExpr)) {
		    llvm::outs() << "ERROR: Clone of SymbolicExpr is not <SymbolicExpr>\n";
		    rv = false;
		    return cloneStmt;
		}
		gVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr), vIt->second));
	    }
	    cloneStmt->guards.push_back(gVec);
	}
	cloneStmt->stmt = stmt;
	cloneStmt->func = func;
	cloneStmt->inputVar = inputVar.clone();
	return cloneStmt;
    }

    virtual void replaceVarsWithExprs(std::vector<VarDetails> parameters, 
    std::vector<SymbolicExpr*> argExprs, bool &rv) {
	replaceGuardVarsWithExprs(parameters, argExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing guards in BlockScalarInputStmt\n";
	    return;
	}
	for (unsigned i = 0; i < parameters.size(); i++) {
	    if (parameters[i].equals(inputVar)) {
		substituteExpr = argExprs[i];
		return;
	    }
	}
    }
};

class BlockArrayInputStmt : public BlockInputStmt {
public:
    SymbolicArraySubscriptExpr* inputArray;

    BlockArrayInputStmt(): BlockInputStmt(INPUTARRAY) {
	inputArray = NULL;
    }

    void print() {
	llvm::outs() << "Array: ";
	if (inputArray)
	    inputArray->print();
	else
	    llvm::outs() << "NULL Input Array\n";
    }

    static bool classof (const BlockStmt* B) {
	return B->getKind() == INPUTARRAY;
    }

    bool containsVar(VarDetails var, bool &rv) {
	rv = true;
	SymbolicExpr* tempArrayExpr = inputArray;
	while (isa<SymbolicArraySubscriptExpr>(tempArrayExpr)) {
	    tempArrayExpr =
		dyn_cast<SymbolicArraySubscriptExpr>(tempArrayExpr)->baseArray;
	}
	if (!isa<SymbolicDeclRefExpr>(tempArrayExpr)) {
	    llvm::outs() << "ERROR: Something wrong: base of array is not "
			 << "SymbolicDeclRefExpr\n";
	    rv = false;
	    return false;
	}
	SymbolicDeclRefExpr* tempBase =
	    dyn_cast<SymbolicDeclRefExpr>(tempArrayExpr);
	if (tempBase->var.equals(var)) return true;
	if (tempBase->substituteExpr) {
	    bool ret = tempBase->substituteExpr->containsVar(var, rv);
	    if (!rv) return false;
	    return ret;
	}
	return false;
    }

    void prettyPrint(std::ofstream &logFile, const MainFunction* mObj, bool &rv,
    bool inResidual=false, std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	prettyPrintGuards(logFile, mObj, rv, true, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyPrinting guards\n";
	    return;
	}
	logFile << ": ";
	inputArray->prettyPrintSummary(logFile, mObj, false, rv, true,
	    inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyPrinting lhsArray\n";
	    return;
	}
	logFile << " = any;\n";
    }

    void prettyPrintHere() {
	if (func)
	    llvm::outs() << func->funcName << "()\n";
	else
	    llvm::outs() << "NULL func\n";
	prettyPrintGuardsHere();
	inputArray->prettyPrint(false);
	llvm::outs() << " = any;\n";
    }

    BlockStmt* clone(bool &rv) {
	rv = true;
	BlockArrayInputStmt* cloneStmt = new BlockArrayInputStmt;
	for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
		>::iterator gIt = guards.begin(); gIt != guards.end(); gIt++) {
	    std::vector<std::pair<SymbolicExpr*, bool> > gVec;
	    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator vIt = 
		    gIt->begin(); vIt != gIt->end(); vIt++) {
		SymbolicStmt* cloneExpr = vIt->first->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While cloning guardExpr\n";
		    return cloneStmt;
		}
		if (!isa<SymbolicExpr>(cloneExpr)) {
		    llvm::outs() << "ERROR: Clone of SymbolicExpr is not <SymbolicExpr>\n";
		    rv = false;
		    return cloneStmt;
		}
		gVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr), vIt->second));
	    }
	    cloneStmt->guards.push_back(gVec);
	}
	cloneStmt->stmt = stmt;
	cloneStmt->func = func;
	SymbolicStmt* cloneArray = inputArray->clone(rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While cloning inputArray\n";
	    return cloneStmt;
	}
	if (!isa<SymbolicArraySubscriptExpr>(cloneArray)) {
	    llvm::outs() << "ERROR: Clone of inputArray is not "
			 << "SymbolicArraySubscriptExpr\n";
	    rv = false;
	    return cloneStmt;
	}
	cloneStmt->inputArray = dyn_cast<SymbolicArraySubscriptExpr>(cloneArray);
	return cloneStmt;
    }
    
    virtual void replaceVarsWithExprs(std::vector<VarDetails> parameters, 
    std::vector<SymbolicExpr*> argExprs, bool &rv) {
	replaceGuardVarsWithExprs(parameters, argExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing guards in BlockArrayInputStmt\n";
	    return;
	}
	inputArray->replaceVarsWithExprs(parameters, argExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in BlockArrayInputStmt\n";
	    return;
	}
    }
};

class BlockInitStmt : public BlockStmt {
public:
    SymbolicArraySubscriptExpr* lhsArray;
    SymbolicExpr* initExpr;

    BlockInitStmt(): BlockStmt(INIT) {
	lhsArray = NULL;
	initExpr = NULL;
    }

    bool containsVar(VarDetails var, bool &rv) {
	rv = true;
	bool ret = lhsArray->containsVar(var, rv);
	if (!rv) return false;
	if (ret) return true;

	ret = initExpr->containsVar(var, rv);
	if (!rv) return false;
	return ret;
    }

    VarDetails getDPVar(bool &rv) {
	rv = true;
	VarDetails vd;
	SymbolicDeclRefExpr* sdre = lhsArray->baseArray;
	while (sdre->substituteExpr) {
	    if (isa<SymbolicDeclRefExpr>(sdre->substituteExpr)) {
		sdre = dyn_cast<SymbolicDeclRefExpr>(sdre->substituteExpr);
	    } else if (isa<SymbolicArraySubscriptExpr>(sdre->substituteExpr)) {
		SymbolicArraySubscriptExpr* sase =
		    dyn_cast<SymbolicArraySubscriptExpr>(sdre->substituteExpr);
		sdre = sase->baseArray;
	    } else {
		llvm::outs() << "ERROR: SubstituteExpr of lhsArray is neither "
			     << "SymbolicDeclRefExpr nor SymbolicArraySubscriptExpr\n";
		rv = false;
		return vd;
	    }
	}

	return sdre->var;
    }

    void prettyPrint(std::ofstream &logFile, const MainFunction* mObj, bool &rv,
    bool inResidual=false, std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	prettyPrintGuards(logFile, mObj, rv, false, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyPrinting guards\n";
	    return;
	}
	logFile << ": ";
#ifdef DEBUG
	llvm::outs() << "DEBUG: lhsArray:\n";
	lhsArray->prettyPrint(false);
	llvm::outs() << (inResidual? "residual" : "not residual");
#endif
	lhsArray->prettyPrintSummary(logFile, mObj, false, rv, false,
	    inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyPrinting lhsArray\n";
	    return;
	}
	logFile << " = ";
#ifdef DEBUG
	llvm::outs() << "DEBUG: initExpr:\n";
	initExpr->prettyPrint(false);
#endif
	initExpr->prettyPrintSummary(logFile, mObj, false, rv, false,
	    inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyPrinting initExpr\n";
	    return;
	}
	logFile << " ;\n";
    }

    void prettyPrintHere() {
	if (func)
	    llvm::outs() << func->funcName << "()\n";
	else
	    llvm::outs() << "NULL func\n";
	prettyPrintGuardsHere();
	lhsArray->prettyPrint(false);
	llvm::outs() << " = ";
	initExpr->prettyPrint(false);
	llvm::outs() << ";\n";
    }

    void print() {
	llvm::outs() << "Init Stmt:\n";
	llvm::outs() << "LHS array:\n";
	if (lhsArray)
	    lhsArray->print();
	else
	    llvm::outs() << "NULL\n";
	llvm::outs() << "Init expr:\n";
	if (initExpr)
	    initExpr->print();
	else
	    llvm::outs() << "NULL\n";
    }

    static bool classof (const BlockStmt* B) {
	return B->getKind() == INIT;
    }

    BlockStmt* clone(bool &rv) {
	rv = true;
	BlockInitStmt* cloneStmt = new BlockInitStmt;
	for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
		>::iterator gIt = guards.begin(); gIt != guards.end(); gIt++) {
	    std::vector<std::pair<SymbolicExpr*, bool> > gVec;
	    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator vIt = 
		    gIt->begin(); vIt != gIt->end(); vIt++) {
		SymbolicStmt* cloneExpr = vIt->first->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While cloning guardExpr\n";
		    return cloneStmt;
		}
		if (!isa<SymbolicExpr>(cloneExpr)) {
		    llvm::outs() << "ERROR: Clone of SymbolicExpr is not <SymbolicExpr>\n";
		    rv = false;
		    return cloneStmt;
		}
		gVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr), vIt->second));
	    }
	    cloneStmt->guards.push_back(gVec);
	}
	cloneStmt->stmt = stmt;
	cloneStmt->func = func;
	SymbolicStmt* cloneArray = lhsArray->clone(rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While cloning lhsArray\n";
	    return cloneStmt;
	}
	if (!isa<SymbolicArraySubscriptExpr>(cloneArray)) {
	    llvm::outs() << "ERROR: Clone of lhsArray is not "
			 << "SymbolicArraySubscriptExpr\n";
	    rv = false;
	    return cloneStmt;
	}
	cloneStmt->lhsArray = dyn_cast<SymbolicArraySubscriptExpr>(cloneArray);
	SymbolicStmt* cloneExpr = initExpr->clone(rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While cloning initExpr\n";
	    return cloneStmt;
	}
	if (!isa<SymbolicExpr>(cloneExpr)) {
	    llvm::outs() << "ERROR: Clone of initExpr is not SymbolicExpr\n";
	    rv = false;
	    return cloneStmt;
	}
	cloneStmt->initExpr = dyn_cast<SymbolicExpr>(cloneExpr);
	return cloneStmt;
    }

    virtual void replaceVarsWithExprs(std::vector<VarDetails> parameters, 
    std::vector<SymbolicExpr*> argExprs, bool &rv) {
	replaceGuardVarsWithExprs(parameters, argExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing guards in BlockInitStmt\n";
	    return;
	}
	lhsArray->replaceVarsWithExprs(parameters, argExprs, rv);
	if (!rv) return;
	initExpr->replaceVarsWithExprs(parameters, argExprs, rv);
	if (!rv) return;
    }

    static std::pair<std::vector<BlockInitStmt*>, std::vector<BlockUpdateStmt*> > 
    getBlockStmt(StmtDetails* st, GetSymbolicExprVisitor* visitor, 
    FunctionDetails* func, bool &rv);
};

class BlockUpdateStmt : public BlockStmt {
public:
    std::vector<std::pair<SymbolicArraySubscriptExpr*, SymbolicExpr*> > 
    updateStmts;
    //SymbolicArraySubscriptExpr* lhsArray;
    //SymbolicExpr* rhsExpr;

    bool containsVar(VarDetails var, bool &rv) {
	rv = true;
	for (std::vector<std::pair<SymbolicArraySubscriptExpr*, SymbolicExpr*>
		>::iterator sIt = updateStmts.begin(); sIt != updateStmts.end();
		sIt++) {
	    bool ret = sIt->first->containsVar(var, rv);
	    if (!rv) return false;
	    if (ret) return true;

	    ret = sIt->second->containsVar(var, rv);
	    if (!rv) return false;
	    if (ret) return true;
	}
	return false;
    }

    VarDetails getDPVar(bool &rv) {
	rv = true;
	VarDetails vd;
	SymbolicDeclRefExpr* sdre = updateStmts[0].first->baseArray;
	while (sdre->substituteExpr) {
	    if (isa<SymbolicDeclRefExpr>(sdre->substituteExpr)) {
		sdre = dyn_cast<SymbolicDeclRefExpr>(sdre->substituteExpr);
	    } else if (isa<SymbolicArraySubscriptExpr>(sdre->substituteExpr)) {
		SymbolicArraySubscriptExpr* sase =
		    dyn_cast<SymbolicArraySubscriptExpr>(sdre->substituteExpr);
		sdre = sase->baseArray;
	    } else {
		llvm::outs() << "ERROR: SubstituteExpr of lhsArray is neither "
			     << "SymbolicDeclRefExpr nor SymbolicArraySubscriptExpr\n";
		rv = false;
		return vd;
	    }
	}

	return sdre->var;
    }

    BlockUpdateStmt(): BlockStmt(UPDT) {
	//lhsArray = NULL;
	//rhsExpr = NULL;
    }

    void prettyPrint(std::ofstream &logFile, const MainFunction* mObj, bool &rv,
    bool inResidual=false, std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	prettyPrintGuards(logFile, mObj, rv, false, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyPrinting guards\n";
	    return;
	}
	logFile << ": ";
	for (std::vector<std::pair<SymbolicArraySubscriptExpr*, SymbolicExpr*>
		>::iterator sIt = updateStmts.begin(); sIt != updateStmts.end();
		sIt++) {
	    sIt->first->prettyPrintSummary(logFile, mObj, false, rv, false,
		inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyPrinting lhsArray\n";
		return;
	    }
	    logFile << " = ";
	    sIt->second->prettyPrintSummary(logFile, mObj, false, rv, false, inResidual,
		surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyPrinting rhsExpr\n";
		return;
	    }
	    if (sIt+1 != updateStmts.end())
		logFile << ", ";
	    else
		logFile << " ; ";
	}
	logFile << "\n";
	if (updateStmts.size() > 1) {
	    llvm::outs() << "STAT: Multiple Stmts\n";
	}
    }

    void prettyPrintHere();

    void print() {
      for (std::vector<std::pair<SymbolicArraySubscriptExpr*, SymbolicExpr*>
	    >::iterator sIt = updateStmts.begin(); sIt != updateStmts.end();
	    sIt++) {
	llvm::outs() << "Update Stmt:\n";
	llvm::outs() << "LHS array:\n";
	if (sIt->first)
	    sIt->first->print();
	else
	    llvm::outs() << "NULL\n";
	llvm::outs() << "RHS expr:\n";
	if (sIt->second)
	    sIt->second->print();
	else
	    llvm::outs() << "NULL\n";
      }
    }

    static bool classof (const BlockStmt* B) {
	return B->getKind() == UPDT;
    }

    BlockStmt* clone(bool &rv) {
	rv = true;
	BlockUpdateStmt* cloneStmt = new BlockUpdateStmt;
	for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
		>::iterator gIt = guards.begin(); gIt != guards.end(); gIt++) {
	    std::vector<std::pair<SymbolicExpr*, bool> > gVec;
	    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator vIt = 
		    gIt->begin(); vIt != gIt->end(); vIt++) {
		SymbolicStmt* cloneExpr = vIt->first->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While cloning guardExpr\n";
		    return cloneStmt;
		}
		if (!isa<SymbolicExpr>(cloneExpr)) {
		    llvm::outs() << "ERROR: Clone of SymbolicExpr is not <SymbolicExpr>\n";
		    rv = false;
		    return cloneStmt;
		}
		gVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr), vIt->second));
	    }
	    cloneStmt->guards.push_back(gVec);
	}
	cloneStmt->stmt = stmt;
	cloneStmt->func = func;
	for (std::vector<std::pair<SymbolicArraySubscriptExpr*, SymbolicExpr*>
	    >::iterator sIt = updateStmts.begin(); sIt != updateStmts.end();
	    sIt++) {
	    SymbolicStmt* cloneArray = sIt->first->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While cloning lhsArray\n";
		return cloneStmt;
	    }
	    if (!isa<SymbolicArraySubscriptExpr>(cloneArray)) {
		llvm::outs() << "ERROR: Clone of lhsArray is not "
			     << "SymbolicArraySubscriptExpr\n";
		rv = false;
		return cloneStmt;
	    }
	    SymbolicArraySubscriptExpr* cloneLHSArray = dyn_cast<SymbolicArraySubscriptExpr>(cloneArray);
	    SymbolicStmt* cloneExpr = sIt->second->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While cloning rhsExpr\n";
		return cloneStmt;
	    }
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of rhsExpr is not "
			     << "SymbolicExpr\n";
		rv = false;
		return cloneStmt;
	    }
	    cloneStmt->updateStmts.push_back(std::make_pair(cloneLHSArray,
		dyn_cast<SymbolicExpr>(cloneExpr)));
	}
	return cloneStmt;
    }

    virtual void replaceVarsWithExprs(std::vector<VarDetails> parameters, 
    std::vector<SymbolicExpr*> argExprs, bool &rv) {
	replaceGuardVarsWithExprs(parameters, argExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing guards in BlockUpdateStmt\n";
	    return;
	}
	for (std::vector<std::pair<SymbolicArraySubscriptExpr*, SymbolicExpr*>
	    >::iterator sIt = updateStmts.begin(); sIt != updateStmts.end();
	    sIt++) {
	    sIt->first->replaceVarsWithExprs(parameters, argExprs, rv);
	    if (!rv) return;
	    sIt->second->replaceVarsWithExprs(parameters, argExprs, rv);
	    if (!rv) return;
	}
    }

    static std::vector<BlockUpdateStmt*> getBlockStmt(StmtDetails* st,
    GetSymbolicExprVisitor* visitor, FunctionDetails* func, bool &rv);
};

class BlockOutputStmt : public BlockStmt {
public:
    SymbolicExpr* outExpr;

    BlockOutputStmt() : BlockStmt(OUT) {
	outExpr = NULL;
    }

    bool containsVar(VarDetails var, bool &rv) {
	rv = true;
	bool ret = outExpr->containsVar(var, rv);
	if (!rv) return false;
	return ret;
    }

    void print() {
	llvm::outs() << "Output Stmt:\n";
	if (outExpr)
	    outExpr->print();
	else
	    llvm::outs() << "NULL\n";
    }

    void prettyPrint(std::ofstream &logFile, const MainFunction* mObj, bool &rv,
    bool inResidual=false, std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	if (isa<SymbolicStringLiteral>(outExpr)) {
	    SymbolicStringLiteral* outStringExpr = dyn_cast<SymbolicStringLiteral>(outExpr);
	    StringRef trimmedString = outStringExpr->val.trim();
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Output trimmedString size: " 
			 << trimmedString.str().size() << "\n";
#endif
	    if (trimmedString.str().size() == 0)
		return;
	} else if (isa<SymbolicCharacterLiteral>(outExpr)) {
	    SymbolicCharacterLiteral* outChar =
		dyn_cast<SymbolicCharacterLiteral>(outExpr);
	    if (isspace(outChar->val)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Output is whitespace\n";
#endif
		return;
	    }
	}
	prettyPrintGuards(logFile, mObj, rv, false, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyPrinting guards\n";
	    return;
	}
	logFile << ": ";
	// Do not print "out = " if outExpr is "insane :)" special expr
	if (!isa<SymbolicSpecialExpr>(outExpr))
	    logFile << "out = ";
	else {
	    // if it is sane
	    SymbolicSpecialExpr* sse = dyn_cast<SymbolicSpecialExpr>(outExpr);
	    if (sse->sane)
		logFile << "out = ";
	}
	outExpr->prettyPrintSummary(logFile, mObj, false, rv, false, inResidual, 
	    surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyPrinting outExpr\n";
	    return;
	}
	if (!isa<SymbolicSpecialExpr>(outExpr))
	    logFile << " ;\n";
	else {
	    SymbolicSpecialExpr* sse = dyn_cast<SymbolicSpecialExpr>(outExpr);
	    if (sse->sane)
		logFile << " ;\n";
	}
    }

    void prettyPrintHere() {
	if (func)
	    llvm::outs() << func->funcName << "()\n";
	else
	    llvm::outs() << "NULL func\n";
	prettyPrintGuardsHere();
	outExpr->prettyPrint(false);
	llvm::outs() << "\n";
    }

    static bool classof (const BlockStmt* B) {
	return B->getKind() == OUT;
    }

    BlockStmt* clone(bool &rv) {
	rv = true;
	BlockOutputStmt* cloneStmt = new BlockOutputStmt;
	for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
		>::iterator gIt = guards.begin(); gIt != guards.end(); gIt++) {
	    std::vector<std::pair<SymbolicExpr*, bool> > gVec;
	    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator vIt = 
		    gIt->begin(); vIt != gIt->end(); vIt++) {
		SymbolicStmt* cloneExpr = vIt->first->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While cloning guardExpr\n";
		    return cloneStmt;
		}
		if (!isa<SymbolicExpr>(cloneExpr)) {
		    llvm::outs() << "ERROR: Clone of SymbolicExpr is not <SymbolicExpr>\n";
		    rv = false;
		    return cloneStmt;
		}
		gVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr), vIt->second));
	    }
	    cloneStmt->guards.push_back(gVec);
	}
	cloneStmt->stmt = stmt;
	cloneStmt->func = func;
	SymbolicStmt* cloneExpr = outExpr->clone(rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While cloning outExpr\n";
	    return cloneStmt;
	}
	if (!isa<SymbolicExpr>(cloneExpr)) {
	    llvm::outs() << "ERROR: Clone of outExpr is not SymbolicExpr\n";
	    rv = false;
	    return cloneStmt;
	}
	cloneStmt->outExpr = dyn_cast<SymbolicExpr>(cloneExpr);
	return cloneStmt;
    }

    virtual void replaceVarsWithExprs(std::vector<VarDetails> parameters, 
    std::vector<SymbolicExpr*> argExprs, bool &rv) {
	replaceGuardVarsWithExprs(parameters, argExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing guards in BlockOutputStmt\n";
	    return;
	}
	outExpr->replaceVarsWithExprs(parameters, argExprs, rv);
	if (!rv) return;
    }

    static std::vector<BlockOutputStmt*> getBlockStmt(StmtDetails* st,
    GetSymbolicExprVisitor* visitor, FunctionDetails* func, bool &rv);
};

class BlockDetails {
public:
    enum BlockKind {
	INPUT, INIT, UPDT, OUT, HETEROGENEOUS, DIFFVARINPUT, DIFFVARINIT,
	DIFFVARUPDT
    };

    BlockKind label;
    std::vector<LoopDetails> loops;
    std::vector<BlockStmt*> stmts;
    bool toDelete;

    BlockDetails() {
	toDelete = false;
    }

    bool containsVar(VarDetails var, bool &rv) {
	rv = true;
	for (std::vector<LoopDetails>::iterator lIt = loops.begin(); lIt != 
		loops.end(); lIt++) {
	    bool ret = lIt->loopIndexInitValSymExpr->containsVar(var, rv);
	    if (!rv) return false;
	    if (ret) return true;

	    ret = lIt->loopIndexFinalValSymExpr->containsVar(var, rv);
	    if (!rv) return false;
	    if (ret) return true;
	}
	for (std::vector<BlockStmt*>::iterator sIt = stmts.begin(); sIt != 
		stmts.end(); sIt++) {
	    bool ret = (*sIt)->containsVar(var, rv);
	    if (!rv) return false;
	    if (ret) return true;
	}
	return false;
    }

    bool isInput() {
	return (label == BlockKind::INPUT);
    }
    bool isInit() {
	return (label == BlockKind::INIT);
    }
    bool isUpdate() {
	return (label == BlockKind::UPDT);
    }
    bool isOutput() {
	return (label == BlockKind::OUT);
    }
    bool isHeterogeneous() {
	return (label == BlockKind::HETEROGENEOUS);
    }
    bool isDiffVar() {
	return (label == BlockKind::DIFFVARINPUT || label ==
	   BlockKind::DIFFVARINIT || label == BlockKind::DIFFVARUPDT);
    }

    std::vector<BlockDetails*> getHomogeneous(bool &rv) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Entering BlockDetails::getHomogeneous()\n";
#endif
	rv = true;
	std::vector<BlockDetails*> homBlocks;
	if (!isHeterogeneous() && !isDiffVar()) {
	    homBlocks.push_back(this);
	    return homBlocks;
	}

	//bool foundOther = false;
	std::vector<SymbolicArraySubscriptExpr*> varsDefinedInOtherStmts;
	for (std::vector<BlockStmt*>::iterator sIt = stmts.begin(); sIt != 
		stmts.end(); sIt++) {
	    if ((*sIt)->isInput()) {
		if (isa<BlockScalarInputStmt>(*sIt)) {
		    continue;
		} else if (isa<BlockArrayInputStmt>(*sIt)) {
		    SymbolicArraySubscriptExpr* def =
			dyn_cast<BlockArrayInputStmt>(*sIt)->inputArray;
		    for (std::vector<SymbolicArraySubscriptExpr*>::iterator vIt =
			    varsDefinedInOtherStmts.begin(); vIt != 
			    varsDefinedInOtherStmts.end(); vIt++) {
			if (def->equals(*vIt)) {
			    rv = false;
			    llvm::outs() << "ERROR: Input after Init/Update/Output\n";
			    return homBlocks;
			}
		    }
		    continue;
		}
	    } else {
		if ((*sIt)->isInit()) {
		    BlockInitStmt* bi = dyn_cast<BlockInitStmt>(*sIt);
		    varsDefinedInOtherStmts.push_back(bi->lhsArray);
		} else if ((*sIt)->isUpdate()) {
		    BlockUpdateStmt* bu  = dyn_cast<BlockUpdateStmt>(*sIt);
		    for (std::vector<std::pair<SymbolicArraySubscriptExpr*, 
			    SymbolicExpr*> >::iterator usIt =
			    bu->updateStmts.begin(); usIt != 
			    bu->updateStmts.end(); usIt++) {
			varsDefinedInOtherStmts.push_back(usIt->first);
		    }
		}
	    }
	}

	BlockDetails* inputBlock = clone(rv);
	if (!rv) return homBlocks;
	inputBlock->stmts.erase(std::remove_if(inputBlock->stmts.begin(),
	    inputBlock->stmts.end(), [](BlockStmt* s) { return !(s->isInput()); }),
	    inputBlock->stmts.end());
	inputBlock->setLabel(rv);
	if (!rv) return homBlocks;
	if (inputBlock->isInput()) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Newly created input block has inputlabel\n";
	    inputBlock->prettyPrintHere();
#endif
	    homBlocks.push_back(inputBlock);
	} else if (inputBlock->label == BlockKind::DIFFVARINPUT) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Newly created input block has DIFFVAR label\n";
	    inputBlock->prettyPrintHere();
#endif
	    std::vector<BlockDetails*> sameVarBlocks = inputBlock->getSameVarBlocks(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Could not obtain same var blocks from "
			     << "DIFFVARINPUT block\n";
		return homBlocks;
	    }
	    homBlocks.insert(homBlocks.end(), sameVarBlocks.begin(),
		sameVarBlocks.end());
	} else if (inputBlock->stmts.size() != 0) {
	    llvm::outs() << "ERROR: Newly created input block does not have "
			 << "input label\n";
	    rv = false;
	    return homBlocks;
	}

	BlockDetails* otherBlock = clone(rv);
	if (!rv) return homBlocks;
	otherBlock->stmts.erase(std::remove_if(otherBlock->stmts.begin(),
	    otherBlock->stmts.end(), [](BlockStmt* s) { return s->isInput(); }),
	    otherBlock->stmts.end());
	otherBlock->setLabel(rv);
	if (!rv) return homBlocks;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Other block after input:\n";
	otherBlock->prettyPrintHere();
#endif
	if (otherBlock->stmts.size() == 0) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Empty block after input\n";
#endif
	    return homBlocks;
	} else if (!otherBlock->isHeterogeneous() &&
		!otherBlock->isDiffVar()) {
#ifdef DEBUG
	    llvm::outs() << "Other block:\n";
	    otherBlock->prettyPrintHere();
#endif

	    homBlocks.push_back(otherBlock);
	    return homBlocks;
	}

	// If the block contains init statements, we can remove them as long as
	// they are not interleaved with update statements and their guards do
	// not involve the dp array
	bool foundOther = false;
	std::vector<std::pair<BlockKind, std::string> > labelsInBlock;
	for (std::vector<BlockStmt*>::iterator sIt = otherBlock->stmts.begin(); 
		sIt != otherBlock->stmts.end(); sIt++) {
	    if ((*sIt)->isInit()) {
		if (foundOther) {
		    llvm::outs() << "ERROR: Init interleaved with other. Not handling..\n";
		    rv = false;
		    return homBlocks;
		}
		BlockInitStmt* st = dyn_cast<BlockInitStmt>(*sIt);
		labelsInBlock.push_back(std::make_pair(BlockKind::INIT,
		    st->lhsArray->baseArray->var.getVarID()));
		// check guards for dp array
		for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
			>::iterator gIt = (*sIt)->guards.begin(); gIt != 
			(*sIt)->guards.end(); gIt++) {
		    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator 
			    vIt = gIt->begin(); vIt != gIt->end(); vIt++) {
			for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt = 
				vIt->first->varsReferenced.begin(); vrIt != 
				vIt->first->varsReferenced.end(); vrIt++) {
			    for (std::vector<SymbolicArraySubscriptExpr*>::iterator 
				    aIt = varsDefinedInOtherStmts.begin(); aIt != 
				    varsDefinedInOtherStmts.end(); aIt++) {
				if ((*aIt)->baseArray->equals(*vrIt)) {
				    llvm::outs() << "ERROR: Init guard contains DP array\n";
				    rv = false;
				    return homBlocks;
				}
			    }
			}
		    }
		}
	    } else if ((*sIt)->isUpdate()) {
		BlockUpdateStmt* st = dyn_cast<BlockUpdateStmt>(*sIt);
		std::string varID = st->updateStmts[0].first->baseArray->var.getVarID();
		for (std::vector<std::pair<BlockKind, std::string> >::iterator
			lIt = labelsInBlock.begin(); lIt != labelsInBlock.end();
			lIt++) {
		    if (lIt->second.compare(varID) == 0) {
			foundOther = true;
			break;
		    }
		}
		//foundOther = true;
	    } else {
		foundOther = true;
	    }
	}

	BlockDetails* initBlock = otherBlock->clone(rv);
	if (!rv) return homBlocks;
	initBlock->stmts.erase(std::remove_if(initBlock->stmts.begin(),
	    initBlock->stmts.end(), [](BlockStmt* s) { return !(s->isInit()); }),
	    initBlock->stmts.end());
	initBlock->setLabel(rv);
	if (!rv) return homBlocks;
	if (initBlock->isInit()) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Newly created init block has initlabel\n";
	    initBlock->prettyPrintHere();
#endif
	    homBlocks.push_back(initBlock);
	} else if (initBlock->label == BlockKind::DIFFVARINIT) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Newly created init block has DIFFVAR label\n";
	    initBlock->prettyPrintHere();
#endif
	    std::vector<BlockDetails*> sameVarBlocks = initBlock->getSameVarBlocks(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Could not obtain same var blocks from "
			     << "DIFFVARINIT block\n";
		return homBlocks;
	    }
	    homBlocks.insert(homBlocks.end(), sameVarBlocks.begin(),
		sameVarBlocks.end());
	} else {
	    llvm::outs() << "ERROR: Newly created init block does not have "
			 << "init label\n";
	    rv = false;
	    return homBlocks;
	}

	//BlockDetails* otherBlock = clone(rv);
	//if (!rv) return homBlocks;
	otherBlock->stmts.erase(std::remove_if(otherBlock->stmts.begin(),
	    otherBlock->stmts.end(), [](BlockStmt* s) { return s->isInit(); }),
	    otherBlock->stmts.end());
	otherBlock->setLabel(rv);
	if (!rv) return homBlocks;
	if (otherBlock->stmts.size() == 0) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Other block empty after init\n";
#endif
	    return homBlocks;
	} else if (!otherBlock-isHeterogeneous()) {
#ifdef DEBUG
	    llvm::outs() << "Other block:\n";
	    otherBlock->prettyPrintHere();
#endif

	    //if (inputBlock->stmts.size() != 0) homBlocks.push_back(inputBlock);
	    //homBlocks.push_back(initBlock);
	    homBlocks.push_back(otherBlock);
	    return homBlocks;
	}

	// Update and output interleaved. TODO
	llvm::outs() << "ERROR: Cannot get homogeneous blocks\n";
	rv = false;
	return homBlocks;
    }

    std::vector<BlockDetails*> getSameVarBlocks(bool &rv);

    void prettyPrint(std::ofstream &logFile, const MainFunction* mObj, bool &rv,
    bool inResidual=false) {
	rv = true;
	if (isInput()) {
	    //logFile << "INPUT:\n";
	    llvm::outs() << "INPUT:\n";
	} else if (isInit()) {
	    //logFile << "INIT:\n";
	    llvm::outs() << "INIT:\n";
	} else if (isUpdate()) {
	    //logFile << "UPDATE:\n";
	    llvm::outs() << "UPDATE:\n";
	} else if (isOutput()) {
	    //logFile << "OUTPUT:\n";
	    llvm::outs() << "OUTPUT:\n";
	} else if (isHeterogeneous()) {
	    llvm::outs() << "ERROR: Prettyprinting heterogeneous block\n";
	    print();
	    rv = false;
	    return;
	} else {
	    llvm::outs() << "ERROR: Prettyprinting unknown block type\n";
	    print();
	    rv = false;
	    return;
	}
#ifdef DEBUG
	llvm::outs() << "loops: " << loops.size() << "\n";
	llvm::outs() << "stmts: " << stmts.size() << "\n";
#endif
	if (stmts.size() == 0) return;
	for (std::vector<LoopDetails>::iterator lIt = loops.begin(); lIt != 
		loops.end(); lIt++) {
	    lIt->prettyPrint(logFile, mObj, rv, false, inResidual,
		std::vector<LoopDetails>(loops.begin(), lIt));
	    if (!rv) return;
	}
	for (std::vector<BlockStmt*>::iterator sIt = stmts.begin(); sIt != 
		stmts.end(); sIt++) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Prettyprinting statement:\n";
	    (*sIt)->prettyPrintHere();
#endif
	    (*sIt)->prettyPrint(logFile, mObj, rv, inResidual, loops);
	}
	for (std::vector<LoopDetails>::iterator lIt = loops.begin(); lIt != 
		loops.end(); lIt++)
	    logFile << "}\n";
    }

    void prettyPrintHere() {
	if (isInput()) 
	    llvm::outs() << "INPUT:\n";
	else if (isInit()) 
	    llvm::outs() << "INIT:\n";
	else if (isUpdate())
	    llvm::outs() << "UPDATE:\n";
	else if (isOutput())
	    llvm::outs() << "OUTPUT:\n";
	else if (isHeterogeneous())
	    llvm::outs() << "HETERO:\n";
	else if (isDiffVar())
	    llvm::outs() << "DIFFVAR:\n";
	else {
	    llvm::outs() << "ERROR: Prettyprinting unknown block type\n";
	    print();
	    return;
	}
#ifdef DEBUG
	llvm::outs() << "loops: " << loops.size() << "\n";
	llvm::outs() << "stmts: " << stmts.size() << "\n";
#endif
        for (std::vector<LoopDetails>::iterator lIt = loops.begin(); lIt != 
		loops.end(); lIt++)
	    lIt->prettyPrintHere();
	for (std::vector<BlockStmt*>::iterator sIt = stmts.begin(); sIt != 
		stmts.end(); sIt++) {
	    (*sIt)->prettyPrintHere();
	}
    }

    void print() {
	if (label == INPUT)
	    llvm::outs() << "Input Block:\n";
	else if (label == INIT)
	    llvm::outs() << "Init Block:\n";
	else if (label == UPDT)
	    llvm::outs() << "Update Block:\n";
	else if (label == OUT)
	    llvm::outs() << "Output Block:\n";
	else if (label == HETEROGENEOUS)
	    llvm::outs() << "Heterogeneous Block:\n";
	else if (label == DIFFVARINPUT)
	    llvm::outs() << "Heterogeneous Input Block with DiffVar:\n";
	else if (label == DIFFVARINIT)
	    llvm::outs() << "Heterogeneous Init Block with DiffVar:\n";
	else if (label == DIFFVARUPDT)
	    llvm::outs() << "Heterogeneous Update Block with DiffVar:\n";
	else
	    llvm::outs() << "Unknown Block Label\n";
	llvm::outs() << "Loops:\n";
	for (std::vector<LoopDetails>::iterator lIt = loops.begin(); lIt != 
		loops.end(); lIt++)
	    lIt->print();
	llvm::outs() << "Stmts:\n";
	for (std::vector<BlockStmt*>::iterator sIt = stmts.begin(); sIt !=
	    stmts.end(); sIt++)
	    (*sIt)->print();
    }

    BlockDetails* clone(bool &rv) {
	rv = true;
	BlockDetails* cloneBlock = new BlockDetails;
	cloneBlock->label = label;
	for (std::vector<LoopDetails>::iterator lIt = loops.begin(); lIt != 
		loops.end(); lIt++) {
	    cloneBlock->loops.push_back(lIt->clone());
	    if (!rv) {
		llvm::outs() << "ERROR: While cloning LoopDetails\n";
		return cloneBlock;
	    }
	}
	for (std::vector<BlockStmt*>::iterator sIt = stmts.begin(); sIt != 
		stmts.end(); sIt++) {
	    cloneBlock->stmts.push_back((*sIt)->clone(rv));
	    if (!rv) {
		llvm::outs() << "ERROR: While cloning BlockStmt\n";
		return cloneBlock;
	    }
	}

	return cloneBlock;
    }

    void replaceVarsWithExprs(std::vector<VarDetails> parameters, 
    std::vector<SymbolicExpr*> argExprs, bool &rv) {
	rv = true;
	for (std::vector<LoopDetails>::iterator lIt = loops.begin(); lIt != 
		loops.end(); lIt++) {
	    if (lIt->loopIndexInitValSymExpr) {
		lIt->loopIndexInitValSymExpr->replaceVarsWithExprs(parameters,
		    argExprs, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While replacing vars in "
				 << "loopIndexInitValSymExpr\n";
		    return;
		}
	    } else {
		llvm::outs() << "ERROR: NULL loopIndexInitValSymExpr\n";
		rv = false;
		return;
	    }
	    if (lIt->loopIndexFinalValSymExpr) {
		lIt->loopIndexFinalValSymExpr->replaceVarsWithExprs(parameters,
		    argExprs, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While replacing vars in "
				 << "loopIndexFinalValSymExpr\n";
		    return;
		}
	    } else {
		llvm::outs() << "ERROR: NULL loopIndexFinalValSymExpr\n";
		rv = false;
		return;
	    }
	}
	for (std::vector<BlockStmt*>::iterator sIt = stmts.begin(); sIt != 
		stmts.end(); sIt++) {
	    (*sIt)->replaceVarsWithExprs(parameters, argExprs, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in BlockStmt\n";
		return;
	    }
	}
    }

    bool isLoopHeaderEqual(BlockDetails* b) {
	if (loops.size() != b->loops.size()) return false;
	for (unsigned i = 0; i < loops.size(); i++) {
	    if (!(loops[i].equals(b->loops[i]))) return false;
	}
	return true;
    }

    // Check if current block has a subset of loops as compared to the given
    // block. Do not call this if loop headers are equal.
    bool isLoopHeaderPartiallyEqual(BlockDetails* b) {
	if (loops.size() == 0) return false;
	if (loops.size() >= b->loops.size()) return false;
	bool foundSomeLoopEqual = false;
	for (unsigned i = 0; i < loops.size(); i++) {
	    if (!(loops[i].equals(b->loops[i]))) {
		if (foundSomeLoopEqual) return true;
		else return false;
	    }
	}
	return true;
    }

    void setLabel(bool &rv) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Entering BlockDetails::setLabel()\n";
#endif
	rv = true;
	label = BlockKind::HETEROGENEOUS; // assume the block is heterogeneous
	std::vector<std::pair<BlockKind, std::string> > labelsInBlock;
	std::string varID;
	for (std::vector<BlockStmt*>::iterator sIt = stmts.begin(); sIt != 
		stmts.end(); sIt++) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: labelsInBlock: ";
	    for (std::vector<std::pair<BlockKind, std::string> >::iterator lIt =
		    labelsInBlock.begin(); lIt != labelsInBlock.end(); lIt++) {
		llvm::outs() << "(";
		if (lIt->first == INPUT) llvm::outs() << "INPUT";
		else if (lIt->first == INIT) llvm::outs() << "INIT";
		else if (lIt->first == UPDT) llvm::outs() << "UPDT";
		else if (lIt->first == OUT) llvm::outs() << "OUTPUT";
		else if (lIt->first == HETEROGENEOUS) llvm::outs() << "HETEROGENEOUS";
		else if (lIt->first == DIFFVARINPUT) llvm::outs() << "DIFFVARINPUT";
		else if (lIt->first == DIFFVARINIT) llvm::outs() << "DIFFVARINIT";
		else if (lIt->first == DIFFVARUPDT) llvm::outs() << "DIFFVARUPDT";
		else llvm::outs() << "UNKNOWN";
		llvm::outs() << ", " << lIt->second << ") ";
	    }
	    llvm::outs() << "\n";
#endif
	    if (isa<BlockInputStmt>(*sIt)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Stmt is INPUT\n";
#endif
		if (isa<BlockScalarInputStmt>(*sIt)) {
		    BlockScalarInputStmt* st =
			dyn_cast<BlockScalarInputStmt>(*sIt);
		    varID = st->inputVar.getVarID();
		} else if (isa<BlockArrayInputStmt>(*sIt)) {
		    BlockArrayInputStmt* st =
			dyn_cast<BlockArrayInputStmt>(*sIt);
		    varID = st->inputArray->baseArray->var.getVarID();
		} else {
		    varID = "-1";
		}
#ifdef DEBUG
		llvm::outs() << "curr VarID: " << varID << "\n";
#endif
		if (label == BlockKind::HETEROGENEOUS) { // Original assumption
		    label = BlockKind::INPUT;
		    labelsInBlock.push_back(std::make_pair(label, varID));
		} else /*if (label != BlockKind::INPUT)*/ {
		    // Check if the previous label is for a different var
		    bool differentVar = false;
		    for (std::vector<std::pair<BlockKind, std::string>
			    >::iterator lIt = labelsInBlock.begin(); lIt != 
			    labelsInBlock.end(); lIt++) {
			if ((lIt->first == BlockKind::INPUT || 
				lIt->first == BlockKind::DIFFVARINPUT) && 
				lIt->second.compare(varID) == 0) {
			    llvm::outs() << "ERROR: Multiple statements reading "
					 << "the same input var\n";
			    rv = false;
			    return;
			}
			if ((lIt->first == BlockKind::INPUT ||
				lIt->first == BlockKind::DIFFVARINPUT) && 
				lIt->second.compare(varID) != 0) {
			    differentVar = true;
			    //break;
			}
		    }
		    bool hetero = false;
		    for (std::vector<std::pair<BlockKind, std::string>
			    >::iterator lIt = labelsInBlock.begin(); lIt != 
			    labelsInBlock.end(); lIt++) {
			if (lIt->first != BlockKind::INPUT && 
				lIt->first != BlockKind::DIFFVARINPUT //&& 
				/*lIt->second.compare(varID) == 0*/) {
			    hetero = true;
			    break;
			}
		    }
		    if (hetero) {
			label = BlockKind::HETEROGENEOUS;
			return;
		    } else if (differentVar) {
			label = BlockKind::DIFFVARINPUT;
			labelsInBlock.push_back(std::make_pair(label, varID));
		    }
		}
	    } else if (isa<BlockInitStmt>(*sIt)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Stmt is INIT\n";
#endif
		BlockInitStmt* st =
		    dyn_cast<BlockInitStmt>(*sIt);
		varID = st->lhsArray->baseArray->var.getVarID();
		if (label == BlockKind::HETEROGENEOUS) { // Original assumption
		    label = BlockKind::INIT;
		    labelsInBlock.push_back(std::make_pair(label, varID));
		} else /*if (label != BlockKind::INIT)*/ {
		    // Check if the previous label is for a different var
		    bool differentVar = false;
		    for (std::vector<std::pair<BlockKind, std::string>
			    >::iterator lIt = labelsInBlock.begin(); lIt != 
			    labelsInBlock.end(); lIt++) {
			if ((lIt->first == BlockKind::INIT ||
				lIt->first == BlockKind::DIFFVARINIT) && 
				lIt->second.compare(varID) != 0) {
			    differentVar = true;
			    break;
			}
		    }
		    bool hetero = false;
		    for (std::vector<std::pair<BlockKind, std::string>
			    >::iterator lIt = labelsInBlock.begin(); lIt != 
			    labelsInBlock.end(); lIt++) {
			if (lIt->first != BlockKind::INIT && 
				lIt->first != BlockKind::DIFFVARINIT //&& 
				/*lIt->second.compare(varID) == 0*/) {
			    hetero = true;
			    break;
			}
		    }
		    if (labelsInBlock.size() == 0) {
			label = BlockKind::INIT;
			labelsInBlock.push_back(std::make_pair(label, varID));
		    } else {
			if (hetero) {
			    label = BlockKind::HETEROGENEOUS;
			    return;
			} else if (differentVar) {
			    label = BlockKind::DIFFVARINIT;
			    labelsInBlock.push_back(std::make_pair(label, varID));
			} else {
			    // Init with same var
			    label = BlockKind::INIT;
			    labelsInBlock.push_back(std::make_pair(label, varID));
			}
		    }
		}
	    } else if (isa<BlockUpdateStmt>(*sIt)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Stmt is UPDATE\n";
#endif
		BlockUpdateStmt* st =
		    dyn_cast<BlockUpdateStmt>(*sIt);
		varID = st->updateStmts[0].first->baseArray->var.getVarID();
		if (label == BlockKind::HETEROGENEOUS) { // Original assumption
		    label = BlockKind::UPDT;
		    labelsInBlock.push_back(std::make_pair(label, varID));
		} else /*if (label != BlockKind::UPDT)*/ {
		    // Check if the previous label is for a different var
		    bool differentVar = false;
		    for (std::vector<std::pair<BlockKind, std::string>
			    >::iterator lIt = labelsInBlock.begin(); lIt != 
			    labelsInBlock.end(); lIt++) {
			if ((lIt->first == BlockKind::UPDT ||
				lIt->first == BlockKind::DIFFVARUPDT) && 
				lIt->second.compare(varID) != 0) {
			    differentVar = true;
			    break;
			}
		    }
		    bool hetero = false;
		    for (std::vector<std::pair<BlockKind, std::string>
			    >::iterator lIt = labelsInBlock.begin(); lIt != 
			    labelsInBlock.end(); lIt++) {
			if (lIt->first != BlockKind::UPDT && 
				lIt->first != BlockKind::DIFFVARUPDT //&& 
				/*lIt->second.compare(varID) == 0*/) {
			    hetero = true;
			    break;
			}
		    }
		    if (labelsInBlock.size() == 0) {
			label = BlockKind::UPDT;
			labelsInBlock.push_back(std::make_pair(label, varID));
		    } else {
			if (hetero) {
			    label = BlockKind::HETEROGENEOUS;
			    return;
			} else if (differentVar) {
			    label = BlockKind::DIFFVARUPDT;
			    labelsInBlock.push_back(std::make_pair(label, varID));
			} else {
			    // Update with same var
			    label = BlockKind::UPDT;
			    labelsInBlock.push_back(std::make_pair(label, varID));
			}
		    }
		}
	    } else if (isa<BlockOutputStmt>(*sIt)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Stmt is OUTPUT\n";
#endif
		if (label == BlockKind::HETEROGENEOUS) { // Original assumption
		    label = BlockKind::OUT;
		    labelsInBlock.push_back(std::make_pair(label, varID));
		} else if (label != BlockKind::OUT) {
		    label = BlockKind::HETEROGENEOUS;
		    labelsInBlock.push_back(std::make_pair(label, varID));
		    return;
		}
	    } else {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Stmt is UNKNOWN\n";
#endif
	    }
	}
    }

#if 0
    bool equals(BlockDetails* b) {
	if (b->label != label) return false;
	if (b->loops.size() != loops.size()) return false;
	for (unsigned i = 0; i < loops.size(); i++) {
	    if (!(b->loops[i].equals(loops[i]))) return false;
	}
	if (b->stmts.size() != stmts.size()) return false;
	for (unsigned i = 0; i < stmts.size(); i++) {
	    if (b->stmts[i] && !stmts[i]) return false;
	    if (!(b->stmts[i]) && stmts[i]) return false;
	    if (!(b->stmts[i]) && !stmts[i]) return false;
	    if (!(b->stmts[i]->equals(stmts[i]))) return false;
	}
	return true;
    }
#endif

    bool convertBlockToRangeExpr(bool &rv); 
    z3::expr getLoopBoundsAsZ3Expr(z3::context* c, const	   
    MainFunction* mObj, std::vector<LoopDetails> surroundingLoops, 
    bool inputFirst, bool &rv);
};
#endif /* BLOCKDETAILS_H */
