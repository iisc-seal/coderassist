#ifndef GETSYMBOLICEXPRVISITOR_H
#define GETSYMBOLICEXPRVISITOR_H

#include "includes.h"
#include "Helper.h"
#include "FunctionAnalysis.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include <string>
#include <z3++.h>

// Data structure to store symbolic expressions
class SymbolicExpr;
class SymbolicDeclRefExpr;
class SymbolicVarDecl;
class MainFunction;
class InputVar;

class SymbolicStmt {
private:
    const StmtKind kind;

public:
    bool resolved;
    bool isDummy;
    std::vector<std::vector<std::pair<unsigned, bool> > > guardBlocks;
    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > guards;
    // Keep track of all the DeclRefExpr part of this SymbolicStmt
    std::vector<SymbolicDeclRefExpr*> varsReferenced;

    SymbolicStmt(StmtKind k): kind(k) {
	resolved = false;
	isDummy = false;
    }

    virtual void print() {
	llvm::outs() << "Guards:\n";
	printGuards();
	printVarRefs();
    }

    virtual SymbolicStmt* clone(bool &rv);
    virtual void replaceVarsWithExprs(std::vector<VarDetails> origVars,
    std::vector<SymbolicExpr*> substituteExprs, bool &rv);
    virtual void replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr, 
    SymbolicExpr* substituteExpr, bool &rv);

    virtual ~SymbolicStmt();

    StmtKind getKind() const {return kind;}

    virtual bool equals(SymbolicStmt* st);
    bool isConstantLiteral();

    void printGuards() {
	llvm::outs() << "Guards: ";
	for (std::vector<std::vector<std::pair<unsigned, bool> > >::iterator gIt =
		guardBlocks.begin(); gIt != guardBlocks.end(); gIt++) {
	    for (std::vector<std::pair<unsigned, bool> >::iterator sIt =
		    gIt->begin(); sIt != gIt->end(); sIt++) {
		llvm::outs() << "(" << sIt->first << ", " 
			     << (sIt->second? "true": "false") << ")";
		if (sIt+1 != gIt->end()) llvm::outs() << " && ";
	    }
	    if (gIt+1 != guardBlocks.end()) llvm::outs() << "\n||";
	    llvm::outs() << "\n";
	}
	llvm::outs() << "\n";
    }

    void printGuardExprs();
    void printVarRefs();

    void prettyPrintGuardExprs();
    virtual bool prettyPrintGuardExprs(std::ofstream &logFile, 
    const MainFunction *mObj, bool &rv, bool inputFirst=false, bool inResidual=false, 
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>());

    static void prettyPrintGuards(
    std::vector<std::vector<std::pair<unsigned, bool> > > guardBlocks) {
	llvm::outs() << "Guards: ";
	for (std::vector<std::vector<std::pair<unsigned, bool> > >::iterator gIt =
		guardBlocks.begin(); gIt != guardBlocks.end(); gIt++) {
	    for (std::vector<std::pair<unsigned, bool> >::iterator sIt =
		    gIt->begin(); sIt != gIt->end(); sIt++) {
		llvm::outs() << "(" << sIt->first << ", " 
			     << (sIt->second? "true": "false") << ")";
		if (sIt+1 != gIt->end()) llvm::outs() << " && ";
	    }
	    if (gIt+1 != guardBlocks.end()) llvm::outs() << "\n||";
	    llvm::outs() << "\n";
	}
	llvm::outs() << "\n";
    }

    virtual void prettyPrint(bool g) {
	if (g) {
	    printGuards();
	    llvm::outs() << "\n";
	    SymbolicStmt::prettyPrintGuardExprs();
	}
    }

    //static void prettyPrintTheseGuardExprs(
    //std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > guards);
};

#if 0
class SymbolicBreakStmt : public SymbolicStmt {
public:
    SymbolicBreakStmt(): SymbolicStmt(BREAK) {
    }

    SymbolicStmt* clone() {
	SymbolicBreakStmt* cloneObj = new SymbolicBreakStmt;
	// SymbolicStmt details
	cloneObj->resolved = resolved;
	cloneObj->isDummy = isDummy;
	cloneObj->guardBlocks.insert(guardBlocks.begin(), guardBlocks.end());
	cloneObj->guards.insert(cloneObj->guards.end(), guards.begin(),
	    guards.end());
	return cloneObj;
    }

    void print() {
	llvm::outs() << "In SymbolicBreakStmt::print()\n";
	printGuards();
    }

    static bool classof(const SymbolicStmt* S) {
	return S->getKind() == BREAK;
    }
};
#endif

class SymbolicDeclStmt : public SymbolicStmt {
public:

    SymbolicVarDecl* var;

    SymbolicDeclStmt(): SymbolicStmt(DECL) {
	var = NULL;
    }

    SymbolicDeclStmt(StmtKind k): SymbolicStmt(k) {
	var = NULL;
    }

    virtual SymbolicStmt* clone(bool &rv);
    virtual void replaceVarsWithExprs(std::vector<VarDetails> origVars,
    std::vector<SymbolicExpr*> substituteExprs, bool &rv);
    virtual void replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr, 
    SymbolicExpr* substituteExpr, bool &rv);

    virtual void print();
    virtual void prettyPrint(bool g);
    virtual bool equals(SymbolicStmt* st);
    //virtual ~SymbolicDeclStmt();

    static bool classof(const SymbolicStmt* S) {
	return S->getKind() >= DECL && S->getKind() <= DECLEND;
    }
};

class SymbolicVarDecl : public SymbolicDeclStmt {
public:
    VarDetails varD;
    SymbolicExpr* initExpr;

    SymbolicVarDecl(): SymbolicDeclStmt(VARDECL) {
	initExpr = NULL;
    }

    virtual SymbolicStmt* clone(bool &rv);
    virtual void replaceVarsWithExprs(std::vector<VarDetails> origVars,
    std::vector<SymbolicExpr*> substituteExprs, bool &rv);
    virtual void replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr, 
    SymbolicExpr* substituteExpr, bool &rv);

    virtual void print();
    virtual void prettyPrint(bool g);
    virtual bool equals(SymbolicStmt* st);
    //~SymbolicVarDecl();

    bool hasInit() {
	if (initExpr != NULL)
	    return true;
	else
	    return false;
    }

    static bool classof(const SymbolicStmt* S) {
	return S->getKind() == VARDECL;
    }
};

class SymbolicReturnStmt: public SymbolicStmt {
public:
    SymbolicExpr* retExpr;

    SymbolicReturnStmt(): SymbolicStmt(RETURN) {
	retExpr = NULL;
    }

    virtual SymbolicStmt* clone(bool &rv);
    virtual void replaceVarsWithExprs(std::vector<VarDetails> origVars,
    std::vector<SymbolicExpr*> substituteExprs, bool &rv);
    virtual void replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr, 
    SymbolicExpr* substituteExpr, bool &rv);

    virtual void print();
    virtual void prettyPrint(bool g);
    virtual bool equals(SymbolicStmt* st);
    //virtual ~SymbolicReturnStmt();

    static bool classof(const SymbolicStmt* S) {
	return S->getKind() == RETURN;
    }
};

class SymbolicExpr : public SymbolicStmt {
public:
    SymbolicExpr(): SymbolicStmt(EXPR) {
    }

    SymbolicExpr(StmtKind k): SymbolicStmt(k) {
    }

    virtual z3::expr getZ3Expr(z3::context* c, const MainFunction* mObj, 
    std::vector<LoopDetails> surroundingLoops,
    bool inputFirst, bool &rv) {
	rv = false;
	z3::expr retExpr(*c);
	return retExpr;
    }

    virtual SymbolicStmt* clone(bool &rv);

    virtual void print() {
	llvm::outs() << "In SymbolicExpr::print()\n";
	printGuards();
	printVarRefs();
    }

    virtual void prettyPrintSummary(std::ofstream &logFile, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false, 
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
    }

    virtual void toString(std::stringstream &logStr, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false, 
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
    }

    virtual bool prettyPrintGuardExprs(std::ofstream &logFile, 
    const MainFunction* mObj, bool &rv, bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	return false;
    }

    virtual void prettyPrint(bool g) {
	if (g) {
	    printGuards();
	    llvm::outs() << "\n";
	    SymbolicStmt::prettyPrintGuardExprs();
	}
#if 0
	if (resolved) llvm::outs() << " resolved";
	else    llvm::outs() << " not resolved";
#endif
    }

    virtual bool containsVar(VarDetails v, bool &rv) {
	rv = true;
	return false;
    }

    virtual std::string getArrayIndices(std::vector<LoopDetails> loops,
    std::vector<InputVar> inputs) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Entering getArrayIndices of ";
	prettyPrint(false);
	llvm::outs() << " in SymbolicExpr\n";
#endif
	return "";
    }

    virtual void replaceArrayExprWithExpr(
    std::vector<SymbolicArraySubscriptExpr*> origExpr,
    std::vector<SymbolicExpr*> subsituteExpr, bool &rv) {
	rv = false;
    }

    static bool classof(const SymbolicStmt* S) {
	return S->getKind() >= EXPR && S->getKind() <= EXPREND;
    }
};

class SymbolicDeclRefExpr : public SymbolicExpr {
public:
    enum VarKind {
	ARRAY, LOOPINDEX, SPECIALSCALAR, DEFVAR, INPUTVAR, FUNCPARAM, FUNC,
	GARBAGE, GLOBALVAR, OTHER
    };

    VarDetails var;
    VarKind vKind;
    FunctionDetails* fd;
    SymbolicExpr* substituteExpr;
    // In case this corresponds to a dummy def of global var at a call expr, the
    // following string stores the the callExprID
    std::string callExprID;

    SymbolicDeclRefExpr(): SymbolicExpr(DECLREFEXPR) {
	substituteExpr = NULL;
	callExprID = "";
    }

    SymbolicDeclRefExpr(VarDetails v, VarKind k, bool res):
	SymbolicExpr(DECLREFEXPR) {
	var = v;
	vKind = k;
	resolved = res;
	substituteExpr = NULL;
	fd = NULL;
	callExprID = "";
    }

    SymbolicDeclRefExpr(FunctionDetails* f, bool res):
	SymbolicExpr(DECLREFEXPR) {
	vKind = VarKind::FUNC;
	fd = f;
	resolved = res;
	substituteExpr = NULL;
	callExprID = "";
    }

    z3::expr getZ3Expr(z3::context* c, const MainFunction* mObj,
    std::vector<LoopDetails> surroundingLoops, 
    bool inputFirst, bool &rv);

    std::string getArrayIndices(std::vector<LoopDetails> loops,
    std::vector<InputVar> inputs) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Entering getArrayIndices of ";
	prettyPrint(false);
	llvm::outs() << "\n";
#endif
	std::string returnStr = "";
	if (substituteExpr) {
	    return substituteExpr->getArrayIndices(loops, inputs);
	}
	if (vKind == VarKind::LOOPINDEX) {
	    unsigned pos = 0;
	    for (std::vector<LoopDetails>::iterator lIt = loops.begin();
		    lIt != loops.end(); lIt++) {
		pos++;
		if (var.equals(lIt->loopIndexVar)) {
		    returnStr = "l";
		    returnStr.append(std::to_string(pos));
		}
	    }
	} else if (vKind == VarKind::INPUTVAR) {
	    for (std::vector<InputVar>::iterator iIt = inputs.begin(); iIt != 
		    inputs.end(); iIt++) {
		if (var.equals(iIt->var)) {
		    returnStr = "i";
		    returnStr.append(std::to_string(iIt->progID));
		}
	    }
	} else {
	    returnStr = "var";
	}
	return returnStr;
    }

    virtual bool containsVar(VarDetails v, bool &rv) {
	rv = true;
	bool returnVal = false;
	if (var.equals(v)) return true;

	if (substituteExpr) {
	    returnVal = substituteExpr->containsVar(v, rv);
	    if (!rv) return returnVal;
	    if (returnVal) return true;
	}

	return false;
    }

    virtual void prettyPrintSummary(std::ofstream &logFile, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>());

    virtual void toString(std::stringstream &logStr, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>());

    virtual bool prettyPrintGuardExprs(std::ofstream &logFile, 
    const MainFunction* mObj, bool &rv, bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	return false;
    }

    virtual void prettyPrint(bool g) {
	if (g){
	    printGuards();
	    llvm::outs() << "\n";
	    SymbolicStmt::prettyPrintGuardExprs();
	}
	if (vKind == VarKind::FUNC) {
	    if (fd) {
		if (fd->funcName.compare("std::max") == 0 ||
			fd->funcName.compare("fName") == 0)
		    llvm::outs() << "stdmax";
		else if (fd->funcName.compare("std::min") == 0)
		    llvm::outs() << "stdmin";
		else
		    llvm::outs() << fd->funcName;
	    } else {
		llvm::outs() << var.varName;
	    }
	} else if (vKind == VarKind::GARBAGE) {
	    llvm::outs() << "garbage";
	} else {
	    if (substituteExpr) {
		llvm::outs() << "SubstituteExpr: ";
		substituteExpr->prettyPrint(false);
	    } else {
		llvm::outs() << var.varName;
		if (vKind == VarKind::LOOPINDEX) llvm::outs() << " loopindex";
		else if (vKind == VarKind::INPUTVAR) llvm::outs() << " input";
		else if (vKind == VarKind::ARRAY) llvm::outs() << " array";
		else if (vKind == VarKind::GLOBALVAR) {
		    llvm::outs() << " global " << callExprID;
		}
	    }
	}
#if 0
	if (resolved) llvm::outs() << " resolved";
	else    llvm::outs() << " not resolved";
#endif
    }

    virtual bool equals(SymbolicStmt* st);

    virtual SymbolicStmt* clone(bool &rv);
    virtual void replaceVarsWithExprs(std::vector<VarDetails> origVars,
    std::vector<SymbolicExpr*> substituteExprs, bool &rv);
    virtual void replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr, 
    SymbolicExpr* substituteExpr, bool &rv);
    void replaceArrayExprWithExpr(
    std::vector<SymbolicArraySubscriptExpr*> origExpr,
    std::vector<SymbolicExpr*> subExpr, bool &rv) {
	rv = true;
	if (substituteExpr) {
	    substituteExpr->replaceArrayExprWithExpr(origExpr, subExpr, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing arrary expr with expr\n";
		return;
	    }
	}
    }

    virtual void print() {
	printGuards();
	if (vKind == OTHER)
	    llvm::outs() << "Not Var";
	else {
	    if (vKind == ARRAY) llvm::outs() << "ARRAY ";
	    else if (vKind == LOOPINDEX) llvm::outs() << "LOOPINDEX ";
	    else if (vKind == SPECIALSCALAR) llvm::outs() << "SPECIALSCALAR";
	    else if (vKind == DEFVAR) llvm::outs() << "DEFVAR ";
	    else if (vKind == INPUTVAR) llvm::outs() << "INPUTVAR ";
	    else if (vKind == FUNCPARAM) llvm::outs() << "FUNCPARAM ";
	    else if (vKind == FUNC) llvm::outs() << "FUNC ";
	    else llvm::outs() << "UNKNOWN ";
	    if (vKind == FUNC) {
		llvm::outs() << "Function: ";
		fd->print();
	    } else {
		llvm::outs() << "Var: ";
		var.print();
	    }
	}
	llvm::outs() << "\n";
	if (substituteExpr) {
	    llvm::outs() << "Substitute Expr:\n";
	    substituteExpr->print();
	    llvm::outs() << "\n";
	} else {
	    llvm::outs() << "NULL Substitute Expr\n";
	}
	llvm::outs() << "varsReferenced in SymbolicDeclRefExpr: "
		     << varsReferenced.size() << "\n";
	for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
		varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	    if (!(*vrIt)) {
		llvm::outs() << "ERROR: NULL SymbolicDeclRefExpr::VarRef\n";
		return;
	    }
	    if (this->equals(*vrIt)) {
		llvm::outs() << "Same as base DeclRefExpr\n";
	    } else {
		(*vrIt)->print();
	    }
	}
    }

    static bool classof(const SymbolicStmt* S) {
	return S->getKind() == DECLREFEXPR;
    }
};

class SymbolicCallExpr : public SymbolicExpr {
public:
    SymbolicExpr* callee;
    std::vector<SymbolicExpr*> callArgExprs;
    //std::vector<SymbolicExpr*> returnExprs;
    SymbolicExpr* returnExpr;

    SymbolicCallExpr(): SymbolicExpr(CALLEXPR) {
	callee = NULL;
	returnExpr = NULL;
    }

    SymbolicCallExpr(StmtKind k): SymbolicExpr(k) {
	callee = NULL;
	returnExpr = NULL;
    }

    z3::expr getZ3Expr(z3::context* c, const MainFunction* mObj,
    std::vector<LoopDetails> surroundingLoops, 
    bool inputFirst, bool &rv);
    virtual bool containsVar(VarDetails v, bool &rv) {
	rv = true;
	bool returnVal = false;
	if (callee) {
	    returnVal = callee->containsVar(v, rv);
	    if (!rv) return returnVal;
	    if (returnVal) return true;
	}
	for (std::vector<SymbolicExpr*>::iterator cIt = callArgExprs.begin();
		cIt != callArgExprs.end(); cIt++) {
	    if (*cIt) {
		returnVal = (*cIt)->containsVar(v, rv);
		if (!rv) return returnVal;
		if (returnVal) return true;
	    }
	}
	if (returnExpr) {
	    returnVal = returnExpr->containsVar(v, rv);
	    if (!rv) return returnVal;
	    if (returnVal) return true;
	}
	return returnVal;
    }

    std::string getArrayIndices(std::vector<LoopDetails> loops,
    std::vector<InputVar> inputs) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Entering getArrayIndices of ";
	prettyPrint(false);
	llvm::outs() << "\n";
#endif
	if (returnExpr)
	    return returnExpr->getArrayIndices(loops, inputs);
	else
	    return SymbolicExpr::getArrayIndices(loops, inputs);
    }

    virtual SymbolicStmt* clone(bool &rv);
    virtual void replaceVarsWithExprs(std::vector<VarDetails> origVars,
    std::vector<SymbolicExpr*> substituteExprs, bool &rv);
    virtual void replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr, 
    SymbolicExpr* substituteExpr, bool &rv);
    void replaceArrayExprWithExpr(
    std::vector<SymbolicArraySubscriptExpr*> origExpr,
    std::vector<SymbolicExpr*> subsituteExpr, bool &rv);

    virtual bool equals(SymbolicStmt* st);
    //virtual ~SymbolicCallExpr();
    virtual void print() {
	printGuards();
	llvm::outs() << "Callee:\n";
	if (callee)
	    callee->print();
	else
	    llvm::outs() << "NULL";
	llvm::outs() << "\n";
	llvm::outs() << "ArgExprs:\n";
	unsigned i;
	for (i = 0; i < callArgExprs.size(); i++) {
	    if (callArgExprs[i]) {
		llvm::outs() << "ArgExpr " << i << ":\n";
		callArgExprs[i]->print();
	    }
	}
	llvm::outs() << "callArgs size: " << callArgExprs.size() << "\n";
#if 0
	llvm::outs() << "ReturnExprs:\n";
	for (i = 0; i < returnExprs.size(); i++) {
	    if (returnExprs[i]) {
		llvm::outs() << "ReturnExpr " << i << ":\n";
		returnExprs[i]->print();
	    }
	}
#endif
	llvm::outs() << "Return Expr:\n";
	if (returnExpr) returnExpr->print();
	else llvm::outs() << "NULL\n";
	printVarRefs();
    }

    virtual void prettyPrintSummary(std::ofstream &logFile, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	if (printAsGuard) {
	    logFile << "(";
	    prettyPrintGuardExprs(logFile, mObj, rv, inputFirst, inResidual,
		surroundingLoops);
#ifdef DEBUG
	    SymbolicStmt::prettyPrintGuardExprs();
#endif
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting guardExpr\n";
		return;
	    }
	    logFile << ") && (";
	}
	if (returnExpr) {
	    if (!isa<SymbolicDeclRefExpr>(returnExpr) &&
		!returnExpr->isConstantLiteral())
		logFile << "(";
	    returnExpr->prettyPrintSummary(logFile, mObj, printAsGuard, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting returnExpr\n";
		return;
	    }
	    if (!isa<SymbolicDeclRefExpr>(returnExpr) &&
		!returnExpr->isConstantLiteral())
		logFile << ")";
	    //if (printAsGuard) logFile << "))";
	    if (printAsGuard) logFile << ")";
	    return;
	}
	if (!isa<SymbolicDeclRefExpr>(callee)) {
	    llvm::outs() << "ERROR: Callee is not SymbolicDeclRefExpr\n";
	    rv = false;
	    return;
	}
	// Check if the call is to memset
	SymbolicDeclRefExpr* calleeDRE = dyn_cast<SymbolicDeclRefExpr>(callee);
	if (calleeDRE->fd->funcName.compare("memset") == 0) {
	    return;
	}
	callee->prettyPrintSummary(logFile, mObj, printAsGuard, rv, inputFirst,
	    inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyprinting callee\n";
	    return;
	}
	logFile << "(";
	for (std::vector<SymbolicExpr*>::iterator sIt = callArgExprs.begin();
		sIt != callArgExprs.end(); sIt++) {
	    (*sIt)->prettyPrintSummary(logFile, mObj, false, rv, inputFirst,
		inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting argExpr\n";
		return;
	    }
	    if (sIt+1 != callArgExprs.end()) logFile << ", ";
	}
	logFile << ")";
	if (printAsGuard) logFile << ")";
    }

    virtual void toString(std::stringstream &logStr, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	if (returnExpr) {
	    if (!isa<SymbolicDeclRefExpr>(returnExpr) &&
		!returnExpr->isConstantLiteral())
		logStr << "(";
	    returnExpr->toString(logStr, mObj, printAsGuard, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting returnExpr\n";
		return;
	    }
	    if (!isa<SymbolicDeclRefExpr>(returnExpr) &&
		!returnExpr->isConstantLiteral())
		logStr << ")";
	    //if (printAsGuard) logStr << "))";
	    if (printAsGuard) logStr << ")";
	    return;
	}
	if (!isa<SymbolicDeclRefExpr>(callee)) {
	    llvm::outs() << "ERROR: Callee is not SymbolicDeclRefExpr\n";
	    rv = false;
	    return;
	}
	// Check if the call is to memset
	SymbolicDeclRefExpr* calleeDRE = dyn_cast<SymbolicDeclRefExpr>(callee);
	if (calleeDRE->fd->funcName.compare("memset") == 0) {
	    return;
	}
	callee->toString(logStr, mObj, printAsGuard, rv, inputFirst,
	    inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyprinting callee\n";
	    return;
	}
	logStr << "(";
	for (std::vector<SymbolicExpr*>::iterator sIt = callArgExprs.begin();
		sIt != callArgExprs.end(); sIt++) {
	    (*sIt)->toString(logStr, mObj, false, rv, inputFirst,
		inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting argExpr\n";
		return;
	    }
	    if (sIt+1 != callArgExprs.end()) logStr << ", ";
	}
	logStr << ")";
    }

    virtual bool prettyPrintGuardExprs(std::ofstream &logFile, 
    const MainFunction* mObj, bool &rv, bool inputFirst=false, bool inResidual=false, 
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	bool printed = false;
	if (returnExpr) {
	    printed = printed ||
		returnExpr->SymbolicStmt::prettyPrintGuardExprs(logFile, mObj,
		rv, inputFirst, inResidual, surroundingLoops);
	    if (!rv) return false;
	} else {
	    printed = printed || callee->prettyPrintGuardExprs(logFile, mObj,
		rv, inputFirst, inResidual, surroundingLoops);
	    if (!rv) return false;
	    for (unsigned i = 0; i < callArgExprs.size(); i++) {
		if (callArgExprs[i])
		    printed = printed ||
			callArgExprs[i]->prettyPrintGuardExprs(logFile, mObj,
			rv, inputFirst, inResidual, surroundingLoops);
	    }
	}
	return printed;
    }

    virtual void prettyPrint(bool g) {
	if (g) {
	    printGuards();
	    llvm::outs() << "\n";
	    SymbolicStmt::prettyPrintGuardExprs();
	}
	if (callee) callee->prettyPrint(false);
	llvm::outs() << "(";
#if 0
	for (std::vector<SymbolicExpr*>::iterator sIt = callArgExprs.begin();
		sIt != callArgExprs.end(); sIt++) {
	    if (*sIt) (*sIt)->prettyPrint(false);
	    if (sIt+1 != callArgExprs.end()) llvm::outs() << ", ";
	}
#endif
	for (unsigned i = 0; i < callArgExprs.size(); i++) {
	    if (callArgExprs[i]) callArgExprs[i]->prettyPrint(false);
	    if (i != callArgExprs.size()-1) llvm::outs() << ", ";
	}
	llvm::outs() << ")\n";
	llvm::outs() << "callArgs size: " << callArgExprs.size() << "\n";
#if 0
	for (std::vector<SymbolicExpr*>::iterator rIt = returnExprs.begin(); 
		rIt != returnExprs.end(); rIt++) {
	    llvm::outs() << "ReturnExpr: ";
	    (*rIt)->prettyPrint(false);
	    llvm::outs() << "\n";
	}
#endif
	if (returnExpr) {
	    llvm::outs() << "returnExpr not NULL\n";
	    returnExpr->SymbolicStmt::prettyPrintGuardExprs();
	    returnExpr->prettyPrint(false);
	} else {
	    llvm::outs() << "returnExpr NULL\n";
	}
#if 0
	if (resolved) llvm::outs() << " resolved";
	else    llvm::outs() << " not resolved";
#endif
    }

    static bool classof(const SymbolicStmt* S) {
	return S->getKind() >= CALLEXPR && S->getKind() <= CALLEXPREND;
    }
};

class SymbolicCXXOperatorCallExpr : public SymbolicCallExpr {
public:

    CXXOperatorKind opKind;

    SymbolicCXXOperatorCallExpr() : SymbolicCallExpr(CXXOPERATORCALLEXPR) {
	opKind = CXXOperatorKind::UNKNOWNOP;
    }

    virtual bool equals(SymbolicStmt* st);

    virtual SymbolicStmt* clone(bool &rv);
    virtual void replaceVarsWithExprs(std::vector<VarDetails> origVars,
    std::vector<SymbolicExpr*> substituteExprs, bool &rv);
    virtual void replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr, 
    SymbolicExpr* substituteExpr, bool &rv);
    void replaceArrayExprWithExpr(
    std::vector<SymbolicArraySubscriptExpr*> origExpr,
    std::vector<SymbolicExpr*> subsituteExpr, bool &rv);

    virtual void print() {
	printGuards();
	llvm::outs() << "Operator: ";
	if (opKind == CIN) llvm::outs() << "CIN";
	else if (opKind == COUT) llvm::outs() << "COUT";
	else llvm::outs() << "UNKNOWN";
	llvm::outs() << "\n";
	//SymbolicCallExpr::print();
	llvm::outs() << "callArgs size: " << callArgExprs.size() << "\n";
    }

    virtual void prettyPrintSummary(std::ofstream &logFile, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	llvm::outs() << "ERROR: Calling prettyPrint on SymbolicCXXOperatorCallExpr\n";
	rv = false;
	return;
    }

    virtual void toString(std::stringstream &logStr, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	llvm::outs() << "ERROR: Calling prettyPrint on SymbolicCXXOperatorCallExpr\n";
	rv = false;
	return;
    }

    virtual bool prettyPrintGuardExprs(std::ofstream &logFile, 
    const MainFunction* mObj, bool &rv, bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	return false;
	//prettyPrintSummary(logFile, mObj, false, rv, false);
	//if (!rv) return;
    }

    virtual void prettyPrint(bool g) {
	if (g) {
	    printGuards();
	    llvm::outs() << "\n";
	    SymbolicStmt::prettyPrintGuardExprs();
	}
	if (opKind == CIN) llvm::outs() << "CIN";
	else if (opKind == COUT) llvm::outs() << "COUT";
	else llvm::outs() << "UNKNOWN";
	llvm::outs() << "\n";
#if 0
	for (std::vector<SymbolicExpr*>::iterator sIt = callArgExprs.begin();
		sIt != callArgExprs.end(); sIt++) {
	    if (*sIt) (*sIt)->prettyPrint(false);
	    if (sIt+1 != callArgExprs.end()) llvm::outs() << ", ";
	}
	llvm::outs() << ")\n";
#endif
	llvm::outs() << "callArgs size: " << callArgExprs.size() << "\n";
#if 0
	if (resolved) llvm::outs() << " resolved";
	else    llvm::outs() << " not resolved";
#endif
    }

    static bool classof(const SymbolicStmt* S) {
	return S->getKind() == CXXOPERATORCALLEXPR;
    }
};

class SymbolicInitListExpr: public SymbolicExpr {
public:
    bool hasArrayFiller;
    SymbolicExpr* arrayFiller;
    std::vector<SymbolicExpr*> inits;

    SymbolicInitListExpr() : SymbolicExpr(INITLISTEXPR) {
	arrayFiller = NULL;
    }

    virtual bool containsVar(VarDetails v, bool &rv) {
	rv = true;
	bool returnVal = false;
	if (arrayFiller) {
	    returnVal = arrayFiller->containsVar(v, rv);
	    if (!rv) return returnVal;
	    if (returnVal) return true;
	}
	for (std::vector<SymbolicExpr*>::iterator iIt = inits.begin(); iIt != 
		inits.end(); iIt++) {
	    if (*iIt) {
		returnVal = (*iIt)->containsVar(v, rv);
		if (!rv) return returnVal;
		if (returnVal) return true;
	    }
	}
	return returnVal;
    }

    //virtual ~SymbolicInitListExpr();
    virtual bool equals(SymbolicStmt* st);
    virtual SymbolicStmt* clone(bool &rv);
    virtual void replaceVarsWithExprs(std::vector<VarDetails> origVars,
    std::vector<SymbolicExpr*> substituteExprs, bool &rv);
    virtual void replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr, 
    SymbolicExpr* substituteExpr, bool &rv);

    virtual void print() {
	printGuards();
	if (hasArrayFiller) {
	    if (arrayFiller) {
		llvm::outs() << "ArrayFiller:\n";
		arrayFiller->print();
	    }
	}
	llvm::outs() << "Inits:\n";
	for (std::vector<SymbolicExpr*>::iterator iIt = inits.begin();
		iIt != inits.end(); iIt++) {
	    if (*iIt) (*iIt)->print();
	}
    }

    static bool classof(const SymbolicStmt* S) {
	return S->getKind() == INITLISTEXPR;
    }
};

class SymbolicUnaryExprOrTypeTraitExpr: public SymbolicExpr {
public:
    UnaryExprOrTypeTrait kind;
    SymbolicDeclRefExpr* array;
    std::vector<SymbolicExpr*> sizeExprs;
    TypeSourceInfo* Ty;

    SymbolicUnaryExprOrTypeTraitExpr(): SymbolicExpr(UNARYEXPRORTYPETRAITEXPR) {
	array = NULL;
	Ty = NULL;
    }

    virtual bool containsVar(VarDetails v, bool &rv) {
	rv = true;
	bool returnVal = false;
	if (array) {
	    returnVal = array->containsVar(v, rv);
	    if (!rv) return returnVal;
	    if (returnVal) return true;
	}

	for (std::vector<SymbolicExpr*>::iterator sIt = sizeExprs.begin(); 
		sIt != sizeExprs.end(); sIt++) {
	    if (*sIt) {
		returnVal = (*sIt)->containsVar(v, rv);
		if (!rv) return returnVal;
		if (returnVal) return true;
	    }
	}
	return false;
    }

    virtual void prettyPrint(bool g) {
	if (g){
	    printGuards();
	    llvm::outs() << "\n";
	    SymbolicStmt::prettyPrintGuardExprs();
	}
	if (kind == UnaryExprOrTypeTrait::UETT_SizeOf) {
	    llvm::outs() << "sizeof(";
	    if (array) array->prettyPrint(true);
	    else llvm::outs() << "NULL";
	    llvm::outs() << ")\n";
	    llvm::outs() << "sizeExprs: ";
	    for (unsigned i = 0; i < sizeExprs.size(); i++)
		if (sizeExprs[i]) sizeExprs[i]->prettyPrint(true);
		else llvm::outs() << "NULL\n";
	} else {
	    llvm::outs() << "Unsupported UETT kind\n";
	}
#if 0
	if (resolved) llvm::outs() << " resolved";
	else    llvm::outs() << " not resolved";
#endif
    }

    virtual bool equals(SymbolicStmt* st);

    virtual SymbolicStmt* clone(bool &rv);
    virtual void replaceVarsWithExprs(std::vector<VarDetails> origVars,
    std::vector<SymbolicExpr*> substituteExprs, bool &rv);
    virtual void replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr, 
    SymbolicExpr* substituteExpr, bool &rv);

    virtual void print() {
	printGuards();
	if (kind == UnaryExprOrTypeTrait::UETT_SizeOf) {
	    llvm::outs() << "sizeof ";
	    if (array) array->print();
	    else llvm::outs() << "NULL\n";
	} else {
	    llvm::outs() << "Unsupported UETT kind\n";
	}

	llvm::outs() << "varsReferenced in SymbolicUnaryExprOrTypeTraitExpr: "
		     << varsReferenced.size() << "\n";
	for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
		varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	    if (!(*vrIt)) {
		llvm::outs() << "ERROR: NULL SymbolicUnaryExprOrTypeTraitExpr::VarRef\n";
		return;
	    }
	    (*vrIt)->print();
	}
    }

    static bool classof(const SymbolicStmt* S) {
	return S->getKind() == UNARYEXPRORTYPETRAITEXPR;
    }
};

class SymbolicArraySubscriptExpr : public SymbolicExpr {
public:
    SymbolicDeclRefExpr* baseArray;
    std::vector<SymbolicExpr*> indexExprs;
    SymbolicExpr* substituteExpr;

    SymbolicArraySubscriptExpr(): SymbolicExpr(ARRAYSUBEXPR) {
	baseArray = NULL;
	substituteExpr = NULL;
    }

    SymbolicArraySubscriptExpr(StmtKind k): SymbolicExpr(k) {
	baseArray = NULL;
	substituteExpr = NULL;
    }

    z3::expr getZ3Expr(z3::context* c, const MainFunction* mObj,
    std::vector<LoopDetails> surroundingLoops, 
    bool inputFirst, bool &rv);
    std::string getArrayIndices(std::vector<LoopDetails> loops,
    std::vector<InputVar> inputs) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Entering getArrayIndices of ";
	prettyPrint(false);
	llvm::outs() << "\n";
#endif
	if (substituteExpr) {
	    return substituteExpr->getArrayIndices(loops, inputs);
	}
	std::stringstream indexStr;
	for (std::vector<SymbolicExpr*>::iterator iIt = indexExprs.begin(); iIt
		!= indexExprs.end(); iIt++) {
	    indexStr << (*iIt)->getArrayIndices(loops, inputs) << " ";
	}
	return indexStr.str();
    }

    virtual bool containsVar(VarDetails v, bool &rv) {
	rv = true;
	bool returnVal = false;
	if (substituteExpr) {
	    returnVal = substituteExpr->containsVar(v, rv);
	    if (!rv) return false;
	    return returnVal;
	}
	if (baseArray) {
	    returnVal = baseArray->containsVar(v, rv);
	    if (!rv) return returnVal;
	    if (returnVal) return true;
	}
	for (std::vector<SymbolicExpr*>::iterator iIt = indexExprs.begin();
		iIt != indexExprs.end(); iIt++) {
	    if (*iIt) {
		returnVal = (*iIt)->containsVar(v, rv);
		if (!rv) return returnVal;
		if (returnVal) return true;
	    }
	}
	return returnVal;
    }

    virtual void prettyPrintSummary(std::ofstream &logFile, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	if (substituteExpr) {
	    substituteExpr->prettyPrintSummary(logFile, mObj, printAsGuard, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyPrinting substituteExpr\n";
	    }
	    return;
	}
	baseArray->prettyPrintSummary(logFile, mObj, printAsGuard, rv,
	    inputFirst, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyprinting baseArray\n";
	    return;
	}
	for (std::vector<SymbolicExpr*>::iterator iIt = indexExprs.begin();
		iIt != indexExprs.end(); iIt++) {
	    logFile << "[";
	    (*iIt)->prettyPrintSummary(logFile, mObj, false, rv, inputFirst,
		inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting indexexpr\n";
		return;
	    }
	    logFile << "]";
	}
    }

    virtual void toString(std::stringstream &logStr, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	if (substituteExpr) {
	    substituteExpr->toString(logStr, mObj, printAsGuard, rv, inputFirst, 
		inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyPrinting substituteExpr\n";
	    }
	    return;
	}
	baseArray->toString(logStr, mObj, printAsGuard, rv,
	    inputFirst, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyprinting baseArray\n";
	    return;
	}
	for (std::vector<SymbolicExpr*>::iterator iIt = indexExprs.begin();
		iIt != indexExprs.end(); iIt++) {
	    logStr << "[";
	    (*iIt)->toString(logStr, mObj, false, rv, inputFirst,
		inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting indexexpr\n";
		return;
	    }
	    logStr << "]";
	}
    }

    virtual bool prettyPrintGuardExprs(std::ofstream &logFile, 
    const MainFunction* mObj, bool &rv, bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	bool printed = false;
	if (substituteExpr) {
	    printed = substituteExpr->prettyPrintGuardExprs(logFile, mObj, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyPrintGuardExprs()\n";
		return true;
	    }
	    return printed;
	}
	if (baseArray) {
	    printed = printed || baseArray->prettyPrintGuardExprs(logFile, mObj,
		rv, inputFirst, inResidual, surroundingLoops);
	    if (!rv) return printed;
	}
	for (unsigned i = 0; i < indexExprs.size(); i++) {
	    if (indexExprs[i]) 
		printed = printed ||
		    indexExprs[i]->prettyPrintGuardExprs(logFile, mObj, rv,
		    inputFirst, inResidual, surroundingLoops);
	}
	return printed;
    }

    virtual void prettyPrint(bool g) {
	if (substituteExpr) {
	    substituteExpr->prettyPrint(g);
	    return;
	}
	if (g) {
	    printGuards();
	    llvm::outs() << "\n";
	    SymbolicStmt::prettyPrintGuardExprs();
	}
	baseArray->prettyPrint(false);
	for (std::vector<SymbolicExpr*>::iterator iIt = indexExprs.begin();
		iIt != indexExprs.end(); iIt++) {
	    llvm::outs() << "[";
	    (*iIt)->prettyPrint(false);
	    llvm::outs() << "]";
	}
#if 0
	if (resolved) llvm::outs() << " resolved";
	else    llvm::outs() << " not resolved";
#endif
    }

    virtual SymbolicStmt* clone(bool &rv);
    virtual void replaceVarsWithExprs(std::vector<VarDetails> origVars,
    std::vector<SymbolicExpr*> substituteExprs, bool &rv);
    virtual void replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr, 
    SymbolicExpr* substituteExpr, bool &rv);
    void replaceArrayExprWithExpr(
    std::vector<SymbolicArraySubscriptExpr*> origExpr,
    std::vector<SymbolicExpr*> subsituteExpr, bool &rv);

    virtual bool equals(SymbolicStmt* st);
    //virtual ~SymbolicArraySubscriptExpr();
    virtual void print() {
	if (substituteExpr) {
	    substituteExpr->print();
	    return;
	}
	printGuards();
	llvm::outs() << "Base Array var: ";
	if (baseArray)
	    baseArray->print();
	llvm::outs() << "Index Exprs:\n";
	int i = 0;
	for (std::vector<SymbolicExpr*>::iterator iIt = indexExprs.begin();
		iIt != indexExprs.end(); iIt++) {
	    llvm::outs() << "Index " << ++i << ": ";
	    (*iIt)->print();
	}
	printVarRefs();
    }

    static bool classof (const SymbolicStmt* S) {
	return (S->getKind() >= ARRAYSUBEXPR && 
		S->getKind() <= ARRAYSUBEXPREND);
    }
};

class SymbolicArrayRangeExpr : public SymbolicArraySubscriptExpr {
public:
    //SymbolicDeclRefExpr* baseArray;
    std::vector<std::pair<SymbolicExpr*, SymbolicExpr*> > indexRangeExprs;

    SymbolicArrayRangeExpr(): SymbolicArraySubscriptExpr(ARRAYRANGEEXPR) {
	//baseArray = NULL;
    }

    z3::expr getZ3Expr(z3::context* c, const MainFunction* mObj,
    std::vector<LoopDetails> surroundingLoops, 
    bool inputFirst, bool &rv);
    virtual bool containsVar(VarDetails v, bool &rv) {
	rv = true;
	bool returnVal = false;
	for (std::vector<std::pair<SymbolicExpr*, SymbolicExpr*> >::iterator 
		iIt = indexRangeExprs.begin(); iIt != indexRangeExprs.end();
		iIt++) {
	    if (iIt->first) {
		returnVal = iIt->first->containsVar(v, rv);
		if (!rv) return returnVal;
		if (returnVal) return true;
	    }
	    if (iIt->second) {
		returnVal = iIt->second->containsVar(v, rv);
		if (!rv) return returnVal;
		if (returnVal) return true;
	    }
	}
	return returnVal;
    }

    virtual void prettyPrintSummary(std::ofstream &logFile, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
#if 0
	if (printAsGuard) {
	    logFile << "(";
	    prettyPrintGuardExprs(logFile, mObj, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting guardExpr\n";
		return;
	    }
	    logFile << ") && (";
	}
#endif
#ifdef DEBUG
	llvm::outs() << (inResidual? "residual": "not residual");
#endif
	baseArray->prettyPrintSummary(logFile, mObj, printAsGuard, rv,
	    inputFirst, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyPrinting baseArray\n";
	    return;
	}
	for (std::vector<std::pair<SymbolicExpr*, SymbolicExpr*> >::iterator 
		rIt = indexRangeExprs.begin(); rIt != indexRangeExprs.end(); rIt++) {
	    logFile << "[";
#ifdef DEBUG
	    llvm::outs() << "DEBUG: indexRange1:\n";
	    rIt->first->prettyPrint(false);
#endif
	    rIt->first->prettyPrintSummary(logFile, mObj, printAsGuard, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyPrinting range1\n";
		return;
	    }
	    logFile << " ... ";
#ifdef DEBUG
	    llvm::outs() << "DEBUG: indexRange2:\n";
	    rIt->second->prettyPrint(false);
#endif
	    rIt->second->prettyPrintSummary(logFile, mObj, printAsGuard, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyPrinting range2\n";
		return;
	    }
	    logFile << "]";
	}
	//if (printAsGuard) logFile << ")";
    }

    virtual void toString(std::stringstream &logStr, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	baseArray->toString(logStr, mObj, printAsGuard, rv,
	    inputFirst, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyPrinting baseArray\n";
	    return;
	}
	for (std::vector<std::pair<SymbolicExpr*, SymbolicExpr*> >::iterator 
		rIt = indexRangeExprs.begin(); rIt != indexRangeExprs.end(); rIt++) {
	    logStr << "[";
#ifdef DEBUG
	    llvm::outs() << "DEBUG: indexRange1:\n";
	    rIt->first->prettyPrint(false);
#endif
	    rIt->first->toString(logStr, mObj, printAsGuard, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyPrinting range1\n";
		return;
	    }
	    logStr << " ... ";
#ifdef DEBUG
	    llvm::outs() << "DEBUG: indexRange2:\n";
	    rIt->second->prettyPrint(false);
#endif
	    rIt->second->toString(logStr, mObj, printAsGuard, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyPrinting range2\n";
		return;
	    }
	    logStr << "]";
	}
    }

    virtual bool prettyPrintGuardExprs(std::ofstream &logFile, 
    const MainFunction* mObj, bool &rv, bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = false;
	bool printed = false;
	if (baseArray) printed = printed ||
	    baseArray->prettyPrintGuardExprs(logFile, mObj, rv, inputFirst, inResidual,
	    surroundingLoops);
	if (!rv) return false;
	for (std::vector<std::pair<SymbolicExpr*, SymbolicExpr*> >::iterator rIt
		= indexRangeExprs.begin(); rIt != indexRangeExprs.end(); rIt++) { 
	    if (rIt->first) printed = printed ||
		rIt->first->prettyPrintGuardExprs(logFile, mObj, rv, inputFirst, inResidual,
		surroundingLoops);
	    if (!rv) return false;
	    if (rIt->second) printed = printed ||
		rIt->second->prettyPrintGuardExprs(logFile, mObj, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) return false;
	}
	return printed;
    }

    virtual void prettyPrint(bool g) {
	if (g) {
	    printGuards();
	    llvm::outs() << "\n";
	    SymbolicStmt::prettyPrintGuardExprs();
	}
	baseArray->prettyPrint(false);
	for (std::vector<std::pair<SymbolicExpr*, SymbolicExpr*> >::iterator 
		rIt = indexRangeExprs.begin(); rIt != indexRangeExprs.end(); rIt++) {
	    llvm::outs() << "[";
	    rIt->first->prettyPrint(false);
	    llvm::outs() << " ... ";
	    rIt->second->prettyPrint(false);
	    llvm::outs() << "]";
	}
#if 0
	if (resolved) llvm::outs() << " resolved";
	else    llvm::outs() << " not resolved";
#endif
    }

    virtual SymbolicStmt* clone(bool &rv);
    virtual void replaceVarsWithExprs(std::vector<VarDetails> origVars,
    std::vector<SymbolicExpr*> substituteExprs, bool &rv);
    virtual void replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr, 
    SymbolicExpr* substituteExpr, bool &rv);
    void replaceArrayExprWithExpr(
    std::vector<SymbolicArraySubscriptExpr*> origExpr,
    std::vector<SymbolicExpr*> subsituteExpr, bool &rv);
    //virtual ~SymbolicArrayRangeExpr();
    virtual void print() {
	printGuards();
	llvm::outs() << "Base Array var: ";
	if (baseArray)
	    baseArray->print();
	llvm::outs() << "Index Exprs:\n";
	int i = 0;
	for (std::vector<std::pair<SymbolicExpr*, SymbolicExpr*> >::iterator 
		iIt = indexRangeExprs.begin(); iIt != indexRangeExprs.end(); iIt++) {
	    llvm::outs() << "Index " << ++i << ": ";
	    iIt->first->print();
	    iIt->second->print();
	}
	printVarRefs();
    }

    static bool classof (const SymbolicStmt* S) {
	return (S->getKind() == ARRAYRANGEEXPR);
    }
};

class SymbolicUnaryOperator : public SymbolicExpr {
public:
    UnaryOperatorKind opKind;
    SymbolicExpr* opExpr;

    SymbolicUnaryOperator(): SymbolicExpr(UNARYOPEXPR) {
	opExpr = NULL;
    }

    z3::expr getZ3Expr(z3::context* c, const MainFunction* mObj,
    std::vector<LoopDetails> surroundingLoops, 
    bool inputFirst, bool &rv);

    virtual std::string getArrayIndices(std::vector<LoopDetails> loops,
    std::vector<InputVar> inputs) {
	std::stringstream returnStr;
	if (opKind == UnaryOperatorKind::UO_Minus)  returnStr << "-";
	else if (opKind == UnaryOperatorKind::UO_LNot) returnStr << "!";
	else if (opKind == UnaryOperatorKind::UO_Plus) ;
	else if (opKind == UnaryOperatorKind::UO_PreInc)
	    returnStr << "++";
	else if (opKind == UnaryOperatorKind::UO_PreDec)
	    returnStr << "--";
        else if (opKind == UnaryOperatorKind::UO_AddrOf)           
	    returnStr << "&";
        else if (opKind == UnaryOperatorKind::UO_Deref)
	    returnStr << "*";
	else if (UnaryOperator::isPrefix(opKind)) {
	    returnStr << "PRE-UOP ";
	}
	returnStr << opExpr->getArrayIndices(loops, inputs);
	if (opKind == UnaryOperatorKind::UO_PostInc)
	    returnStr << "++";
	else if (opKind == UnaryOperatorKind::UO_PostDec)
	    returnStr << "--";
	else if (UnaryOperator::isPostfix(opKind))
	    returnStr << "POST-UOP";
	return returnStr.str();
    }

    virtual bool containsVar(VarDetails v, bool &rv) {
	rv = true;
	bool returnVal = false;
	if (opExpr) {
	    returnVal = opExpr->containsVar(v, rv);
	    if (!rv) return returnVal;
	    if (returnVal) return true;
	}
	return returnVal;
    }

    virtual void prettyPrintSummary(std::ofstream &logFile, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
#if 0
	if (printAsGuard) {
	    logFile << "(";
	    prettyPrintGuardExprs(logFile, mObj, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting guardExpr\n";
		return;
	    }
	    logFile << ") && (";
	}
#endif
	// In case the operator is pointer dereference, then print the entire
	// expression here.
	if (opKind == UnaryOperatorKind::UO_Deref) {
	    SymbolicExpr* baseExpr = opExpr;
	    unsigned dereferenceDepth = 1;
	    while (isa<SymbolicUnaryOperator>(baseExpr)) {
		SymbolicUnaryOperator* suo =
		    dyn_cast<SymbolicUnaryOperator>(baseExpr);
		if (suo->opKind != UnaryOperatorKind::UO_Deref) {
		    llvm::outs() << "ERROR: Prettyprinting UOP that was initially pointer dereference\n";
		    print();
		    rv = false;
		    return;
		}
		dereferenceDepth++;
		baseExpr = suo->opExpr;
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: UnaryOperator UO_Deref has depth " 
			 << dereferenceDepth << " baseArray: ";
	    baseExpr->print();
#endif
	    if (isa<SymbolicDeclRefExpr>(baseExpr)) {
		SymbolicDeclRefExpr* sdre =
		    dyn_cast<SymbolicDeclRefExpr>(baseExpr);
		if (sdre->var.arraySizeInfo.size() != dereferenceDepth) {
		    llvm::outs() << "ERROR: The dereference depth does not match the array size\n";
		    print();
		    rv = false;
		    return;
		}
		sdre->prettyPrintSummary(logFile, mObj, printAsGuard, rv,
		    inputFirst, inResidual, surroundingLoops);
		for (unsigned i = 1; i <= dereferenceDepth; i++) {
		    logFile << "[0]";
		}
	    } else if (isa<SymbolicArraySubscriptExpr>(baseExpr)) {
		SymbolicArraySubscriptExpr* sase =
		    dyn_cast<SymbolicArraySubscriptExpr>(baseExpr);
		SymbolicDeclRefExpr* baseArray =
		    dyn_cast<SymbolicDeclRefExpr>(sase->baseArray);
		if (sase->indexExprs.size() + dereferenceDepth !=
			baseArray->var.arraySizeInfo.size()) {
		    llvm::outs() << "ERROR: The dereference depth does not match the array size\n";
		    print();
		    rv = false;
		    return;
		}
		sase->prettyPrintSummary(logFile, mObj, printAsGuard, rv,
		    inputFirst, inResidual, surroundingLoops);
		for (unsigned i = 1; i <= dereferenceDepth; i++) {
		    logFile << "[0]";
		}
	    } else {
		llvm::outs() << "ERROR: Unsupported pointer dereference operation\n";
		print();
		rv = false;
		return;
	    }
	} else {
	    logFile << "(";
	    if (opKind == UnaryOperatorKind::UO_Minus) logFile << "-";
	    else if (opKind == UnaryOperatorKind::UO_LNot) logFile << "!";
	    else if (opKind == UnaryOperatorKind::UO_Plus) ;
	    else if (opKind == UnaryOperatorKind::UO_PostInc || 
		     opKind == UnaryOperatorKind::UO_PostDec ||
		     opKind == UnaryOperatorKind::UO_PreInc ||
		     opKind == UnaryOperatorKind::UO_PreDec) logFile << "(";
	    else {
		llvm::outs() << "ERROR: Prettyprinting UOP that is not UMinus, "
			     << "UPlus, LogicalNot\n";
		print();
		rv = false;
		return;
	    }
	    if (!isa<SymbolicDeclRefExpr>(opExpr) && !opExpr->isConstantLiteral())
		logFile << "(";
		opExpr->prettyPrintSummary(logFile, mObj, printAsGuard, rv,
		    inputFirst, inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting UnaryOperator\n";
		return;
	    }
	    if (opKind == UnaryOperatorKind::UO_PostInc || opKind ==
		    UnaryOperatorKind::UO_PreInc) {
		logFile << " + 1)";
	    } else if (opKind == UnaryOperatorKind::UO_PostDec || opKind ==
		    UnaryOperatorKind::UO_PreDec) {
		logFile << " - 1)";
	    }
	    if (!isa<SymbolicDeclRefExpr>(opExpr) && !opExpr->isConstantLiteral())
		logFile << ")";
	    logFile << ")";
	}
	//if (printAsGuard) logFile << ")";
    }

    virtual void toString(std::stringstream &logStr, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	// In case the operator is pointer dereference, then print the entire
	// expression here.
	if (opKind == UnaryOperatorKind::UO_Deref) {
	    SymbolicExpr* baseExpr = opExpr;
	    unsigned dereferenceDepth = 1;
	    while (isa<SymbolicUnaryOperator>(baseExpr)) {
		SymbolicUnaryOperator* suo =
		    dyn_cast<SymbolicUnaryOperator>(baseExpr);
		if (suo->opKind != UnaryOperatorKind::UO_Deref) {
		    llvm::outs() << "ERROR: Prettyprinting UOP that was initially pointer dereference\n";
		    print();
		    rv = false;
		    return;
		}
		dereferenceDepth++;
		baseExpr = suo->opExpr;
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: UnaryOperator UO_Deref has depth " 
			 << dereferenceDepth << " baseArray: ";
	    baseExpr->print();
#endif
	    if (isa<SymbolicDeclRefExpr>(baseExpr)) {
		SymbolicDeclRefExpr* sdre =
		    dyn_cast<SymbolicDeclRefExpr>(baseExpr);
		if (sdre->var.arraySizeInfo.size() != dereferenceDepth) {
		    llvm::outs() << "ERROR: The dereference depth does not match the array size\n";
		    print();
		    rv = false;
		    return;
		}
		sdre->toString(logStr, mObj, printAsGuard, rv,
		    inputFirst, inResidual, surroundingLoops);
		for (unsigned i = 1; i <= dereferenceDepth; i++) {
		    logStr << "[0]";
		}
	    } else if (isa<SymbolicArraySubscriptExpr>(baseExpr)) {
		SymbolicArraySubscriptExpr* sase =
		    dyn_cast<SymbolicArraySubscriptExpr>(baseExpr);
		SymbolicDeclRefExpr* baseArray =
		    dyn_cast<SymbolicDeclRefExpr>(sase->baseArray);
		if (sase->indexExprs.size() + dereferenceDepth !=
			baseArray->var.arraySizeInfo.size()) {
		    llvm::outs() << "ERROR: The dereference depth does not match the array size\n";
		    print();
		    rv = false;
		    return;
		}
		sase->toString(logStr, mObj, printAsGuard, rv,
		    inputFirst, inResidual, surroundingLoops);
		for (unsigned i = 1; i <= dereferenceDepth; i++) {
		    logStr << "[0]";
		}
	    } else {
		llvm::outs() << "ERROR: Unsupported pointer dereference operation\n";
		print();
		rv = false;
		return;
	    }
	} else {
	    logStr << "(";
	    if (opKind == UnaryOperatorKind::UO_Minus) logStr << "-";
	    else if (opKind == UnaryOperatorKind::UO_LNot) logStr << "!";
	    else if (opKind == UnaryOperatorKind::UO_Plus) ;
	    else if (opKind == UnaryOperatorKind::UO_PostInc || 
		     opKind == UnaryOperatorKind::UO_PostDec ||
		     opKind == UnaryOperatorKind::UO_PreInc ||
		     opKind == UnaryOperatorKind::UO_PreDec) logStr << "(";
	    else {
		llvm::outs() << "ERROR: Prettyprinting UOP that is not UMinus, "
			     << "UPlus, LogicalNot\n";
		print();
		rv = false;
		return;
	    }
	    if (!isa<SymbolicDeclRefExpr>(opExpr) && !opExpr->isConstantLiteral())
		logStr << "(";
		opExpr->toString(logStr, mObj, printAsGuard, rv,
		    inputFirst, inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting UnaryOperator\n";
		return;
	    }
	    if (opKind == UnaryOperatorKind::UO_PostInc || opKind ==
		    UnaryOperatorKind::UO_PreInc) {
		logStr << " + 1)";
	    } else if (opKind == UnaryOperatorKind::UO_PostDec || opKind ==
		    UnaryOperatorKind::UO_PreDec) {
		logStr << " - 1)";
	    }
	    if (!isa<SymbolicDeclRefExpr>(opExpr) && !opExpr->isConstantLiteral())
		logStr << ")";
	    logStr << ")";
	}
    }

    virtual bool prettyPrintGuardExprs(std::ofstream &logFile, 
    const MainFunction* mObj, bool &rv, bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	bool printed = false;
	if (opExpr)
	    printed = printed || opExpr->prettyPrintGuardExprs(logFile, mObj,
		rv, inputFirst, inResidual, surroundingLoops);
	if (!rv) return false;
	return printed;
    }

    virtual void prettyPrint(bool g) {
	if (g) {
	    printGuards();
	    llvm::outs() << "\n";
	    SymbolicStmt::prettyPrintGuardExprs();
	}
	llvm::outs() << "(";
	if (opKind == UnaryOperatorKind::UO_Minus) llvm::outs() << "-";
	else if (opKind == UnaryOperatorKind::UO_LNot) llvm::outs() << "!";
	else if (opKind == UnaryOperatorKind::UO_Plus) ;
	else if (opKind == UnaryOperatorKind::UO_PreInc)
	    llvm::outs() << "++";
	else if (opKind == UnaryOperatorKind::UO_PreDec)
	    llvm::outs() << "--";
        else if (opKind == UnaryOperatorKind::UO_AddrOf)           
	    llvm::outs() << "&";
        else if (opKind == UnaryOperatorKind::UO_Deref)
	    llvm::outs() << "*";
	else if (UnaryOperator::isPrefix(opKind)) {
	    llvm::outs() << "PRE-UOP ";
	}
	if (!isa<SymbolicDeclRefExpr>(opExpr) && !opExpr->isConstantLiteral())
	    llvm::outs() << "(";
	opExpr->prettyPrint(false);
	if (!isa<SymbolicDeclRefExpr>(opExpr) && !opExpr->isConstantLiteral())
	    llvm::outs() << ")";
	if (opKind == UnaryOperatorKind::UO_PostInc)
	    llvm::outs() << "++";
	else if (opKind == UnaryOperatorKind::UO_PostDec)
	    llvm::outs() << "--";
	else if (UnaryOperator::isPostfix(opKind))
	    llvm::outs() << "POST-UOP";
	llvm::outs() << ")";
#if 0
	if (resolved) llvm::outs() << " resolved";
	else    llvm::outs() << " not resolved";
#endif
    }

    virtual SymbolicStmt* clone(bool &rv);
    virtual void replaceVarsWithExprs(std::vector<VarDetails> origVars,
    std::vector<SymbolicExpr*> substituteExprs, bool &rv);
    virtual void replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr, 
    SymbolicExpr* substituteExpr, bool &rv);
    void replaceArrayExprWithExpr(
    std::vector<SymbolicArraySubscriptExpr*> origExpr,
    std::vector<SymbolicExpr*> subsituteExpr, bool &rv);

    virtual bool equals(SymbolicStmt* st);
    //virtual ~SymbolicUnaryOperator();
    virtual void print() {
	printGuards();
	if (opExpr) {
            if (opKind == UnaryOperatorKind::UO_PostInc)               
                llvm::outs() << "UO_PostInc";
            else if (opKind == UnaryOperatorKind::UO_PostDec)          
                llvm::outs() << "UO_PostDec";                          
            else if (opKind == UnaryOperatorKind::UO_PreInc)           
                llvm::outs() << "UO_PreInc";
            else if (opKind == UnaryOperatorKind::UO_PreDec)           
                llvm::outs() << "UO_PreDec";                           
            else if (opKind == UnaryOperatorKind::UO_AddrOf)           
                llvm::outs() << "UO_AddrOf";                           
            else if (opKind == UnaryOperatorKind::UO_Deref)
                llvm::outs() << "UO_Deref";
            else if (opKind == UnaryOperatorKind::UO_Plus)
                llvm::outs() << "UO_Plus";
            else if (opKind == UnaryOperatorKind::UO_Minus)
                llvm::outs() << "UO_Minus";
            else if (opKind == UnaryOperatorKind::UO_Not)
                llvm::outs() << "UO_Not";
            else if (opKind == UnaryOperatorKind::UO_LNot)
                llvm::outs() << "UO_LNot";
            else
                llvm::outs() << "Unknown opKind";
            llvm::outs() << "\n";

            opExpr->print();

	}
	printVarRefs();
    }

    static bool classof (const SymbolicStmt* S) {
	return S->getKind() == UNARYOPEXPR;
    }
};

class SymbolicFloatingLiteral: public SymbolicExpr {
public:

    llvm::APFloat val;

    SymbolicFloatingLiteral(): SymbolicExpr(FLOATINGLITERAL),
    val(0.0) {
    }
    
    z3::expr getZ3Expr(z3::context* c, const MainFunction* mObj,
    std::vector<LoopDetails> surroundingLoops, 
    bool inputFirst, bool &rv);
    virtual std::string getArrayIndices(std::vector<LoopDetails> loops,
    std::vector<InputVar> inputs) {
	std::stringstream returnStr;
	returnStr << val.convertToDouble() << "L";
	return returnStr.str();
    }

    virtual bool containsVar(VarDetails v, bool &rv) {
	rv = true;
	return false;
    }

    virtual void prettyPrintSummary(std::ofstream &logFile, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
#if 0
	if (printAsGuard) {
	    logFile << "(";
	    prettyPrintGuardExprs(logFile, mObj, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting guardExpr\n";
		return;
	    }
	    logFile << ") && (";
	}
#endif
	logFile << val.convertToDouble();
	//if (printAsGuard) logFile << ")";
    }

    virtual void toString(std::stringstream &logStr, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	logStr << val.convertToDouble();
    }

    virtual bool prettyPrintGuardExprs(std::ofstream &logFile, 
    const MainFunction* mObj, bool &rv, bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	return false;
	//prettyPrintSummary(logFile, mObj, false, rv, false);
	//if (!rv) return;
    }

    virtual void prettyPrint(bool g) {
	if (g) {
	    printGuards();
	    llvm::outs() << "\n";
	    SymbolicStmt::prettyPrintGuardExprs();
	}
	//llvm::outs() << val.toString(10, val.isNegative());
	llvm::outs() << val.convertToDouble();
#if 0
	if (resolved) llvm::outs() << " resolved";
	else    llvm::outs() << " not resolved";
#endif
    }

    virtual SymbolicStmt* clone(bool &rv);

    virtual bool equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicFloat::equals()\n";
#endif
	//if (!SymbolicStmt::equals(st)) return false;
	if (!isa<SymbolicFloatingLiteral>(st)) return false;
	SymbolicFloatingLiteral* sil = dyn_cast<SymbolicFloatingLiteral>(st);
	if (!(val.bitwiseIsEqual(sil->val))) return false;
	return true;
    }

    virtual void print() {
	printGuards();
	llvm::outs() << val.convertToDouble() << "\n";
	printVarRefs();
    }

    void replaceArrayExprWithExpr(
    std::vector<SymbolicArraySubscriptExpr*> origExpr,
    std::vector<SymbolicExpr*> subsituteExpr, bool &rv) {
	rv = true;
    }

    static bool classof (const SymbolicStmt* S) {
	return S->getKind() == FLOATINGLITERAL;
    }
};

class SymbolicIntegerLiteral: public SymbolicExpr {
public:

    llvm::APInt val;

    SymbolicIntegerLiteral(): SymbolicExpr(INTEGERLITERAL) {
    }

    z3::expr getZ3Expr(z3::context* c, const MainFunction* mObj,
    std::vector<LoopDetails> surroundingLoops, 
    bool inputFirst, bool &rv);
    virtual std::string getArrayIndices(std::vector<LoopDetails> loops,
    std::vector<InputVar> inputs) {
	std::stringstream returnStr;
	returnStr << val.toString(10, val.isNegative()) << "L";
	return returnStr.str();
    }

    virtual bool containsVar(VarDetails v, bool &rv) {
	rv = true;
	return false;
    }

    virtual void prettyPrintSummary(std::ofstream &logFile, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
#if 0
	if (printAsGuard) {
	    logFile << "(";
	    prettyPrintGuardExprs(logFile, mObj, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting guardExpr\n";
		return;
	    }
	    logFile << ") && (";
	}
#endif
	//val.print(logFile, val.isNegative());
	logFile << val.toString(10, val.isNegative());
	//if (printAsGuard) logFile << ")";
    }

    virtual void toString(std::stringstream &logStr, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	logStr << val.toString(10, val.isNegative());
    }

    virtual bool prettyPrintGuardExprs(std::ofstream &logFile, 
    const MainFunction* mObj, bool &rv, bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	return false;
	//prettyPrintSummary(logFile, mObj, false, rv, false);
	//if (!rv) return;
    }

    virtual void prettyPrint(bool g) {
	if (g) {
	    printGuards();
	    llvm::outs() << "\n";
	    SymbolicStmt::prettyPrintGuardExprs();
	}
	llvm::outs() << val.toString(10, val.isNegative());
#if 0
	if (resolved) llvm::outs() << " resolved";
	else    llvm::outs() << " not resolved";
#endif
    }

    virtual SymbolicStmt* clone(bool &rv);

    virtual bool equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicIntt::equals()\n";
#endif
	//if (!SymbolicStmt::equals(st)) return false;
	if (!isa<SymbolicIntegerLiteral>(st)) return false;
	SymbolicIntegerLiteral* sil = dyn_cast<SymbolicIntegerLiteral>(st);
	if (val != sil->val) return false;
	return true;
    }

    virtual void print() {
	printGuards();
	val.print(llvm::outs(), val.isNegative());
	llvm::outs() << "\n";
	printVarRefs();
    }

    void replaceArrayExprWithExpr(
    std::vector<SymbolicArraySubscriptExpr*> origExpr,
    std::vector<SymbolicExpr*> subsituteExpr, bool &rv) {
	rv = true;
    }

    static bool classof (const SymbolicStmt* S) {
	return S->getKind() == INTEGERLITERAL;
    }
};

class SymbolicImaginaryLiteral: public SymbolicExpr {
public:

    llvm::APInt val;

    SymbolicImaginaryLiteral(): SymbolicExpr(IMAGINARYLITERAL) {
    }

    z3::expr getZ3Expr(z3::context* c, const MainFunction* mObj,
    std::vector<LoopDetails> surroundingLoops, 
    bool inputFirst, bool &rv);
    virtual std::string getArrayIndices(std::vector<LoopDetails> loops,
    std::vector<InputVar> inputs) {
	std::stringstream returnStr;
	returnStr << val.toString(10, val.isNegative()) << "L";
	return returnStr.str();
    }

    virtual bool containsVar(VarDetails v, bool &rv) {
	rv = true;
	return false;
    }

    virtual void prettyPrintSummary(std::ofstream &logFile, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
#if 0
	if (printAsGuard) {
	    logFile << "(";
	    prettyPrintGuardExprs(logFile, mObj, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting guardExpr\n";
		return;
	    }
	    logFile << ") && (";
	}
#endif
	//val.print(logFile, val.isNegative());
	logFile << val.toString(10, val.isNegative());
	//if (printAsGuard) logFile << ")";
    }

    virtual void toString(std::stringstream &logStr, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	logStr << val.toString(10, val.isNegative());
    }

    virtual bool prettyPrintGuardExprs(std::ofstream &logFile, 
    const MainFunction* mObj, bool &rv, bool inputFirst=false, bool inResidual=false, 
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	return false;
	//prettyPrintSummary(logFile, mObj, false, rv, false);
	//if (!rv) return;
    }

    virtual void prettyPrint(bool g) {
	if (g) {
	    printGuards();
	    llvm::outs() << "\n";
	    SymbolicStmt::prettyPrintGuardExprs();
	}
	llvm::outs() << val.toString(10, val.isNegative());
#if 0
	if (resolved) llvm::outs() << " resolved";
	else    llvm::outs() << " not resolved";
#endif
    }

    virtual SymbolicStmt* clone(bool &rv);

    virtual bool equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicImag::equals()\n";
#endif
	//if (!SymbolicStmt::equals(st)) return false;
	if (!isa<SymbolicImaginaryLiteral>(st)) return false;
	SymbolicImaginaryLiteral* sil = dyn_cast<SymbolicImaginaryLiteral>(st);
	if (val != sil->val) return false;
	return true;
    }

    virtual void print() {
	printGuards();
	val.print(llvm::outs(), val.isNegative());
	llvm::outs() << "\n";
	printVarRefs();
    }

    void replaceArrayExprWithExpr(
    std::vector<SymbolicArraySubscriptExpr*> origExpr,
    std::vector<SymbolicExpr*> subsituteExpr, bool &rv) {
	rv = true;
    }

    static bool classof (const SymbolicStmt* S) {
	return S->getKind() == IMAGINARYLITERAL;
    }
};

class SymbolicCXXBoolLiteralExpr: public SymbolicExpr {
public:

    bool val;

    SymbolicCXXBoolLiteralExpr(): SymbolicExpr(CXXBOOLLITERALEXPR) {
    }

    z3::expr getZ3Expr(z3::context* c, const MainFunction* mObj,
    std::vector<LoopDetails> surroundingLoops, 
    bool inputFirst, bool &rv);
    virtual std::string getArrayIndices(std::vector<LoopDetails> loops,
    std::vector<InputVar> inputs) {
	return (val? "true": "false");
    }

    virtual bool containsVar(VarDetails v, bool &rv) {
	rv = true;
	return false;
    }

    virtual void prettyPrintSummary(std::ofstream &logFile, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
#if 0
	if (printAsGuard) {
	    logFile << "(";
	    prettyPrintGuardExprs(logFile, mObj, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting guardExpr\n";
		return;
	    }
	    logFile << ") && (";
	}
#endif
	if (val) logFile << "true";
	else logFile << "false";
	//if (printAsGuard) logFile << ")";
    }

    virtual void toString(std::stringstream &logStr, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	if (val) logStr << "true";
	else logStr << "false";
    }

    virtual bool prettyPrintGuardExprs(std::ofstream &logFile, 
    const MainFunction* mObj, bool &rv, bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	return false;
	//prettyPrintSummary(logFile, mObj, false, rv, false);
	//if (!rv) return;
    }

    virtual void prettyPrint(bool g) {
	if (g) {
	    printGuards();
	    llvm::outs() << "\n";
	    SymbolicStmt::prettyPrintGuardExprs();
	}
	if (val) llvm::outs() << "true";
	else llvm::outs() << "false";
#if 0
	if (resolved) llvm::outs() << " resolved";
	else    llvm::outs() << " not resolved";
#endif
    }

    virtual SymbolicStmt* clone(bool &rv);

    virtual bool equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicBolt::equals()\n";
#endif
	//if (!SymbolicStmt::equals(st)) return false;
	if (!isa<SymbolicCXXBoolLiteralExpr>(st)) return false;
	SymbolicCXXBoolLiteralExpr* sbl =
	    dyn_cast<SymbolicCXXBoolLiteralExpr>(st);
	if (val != sbl->val) return false;
	return true;
    }

    virtual void print() {
	printGuards();
	llvm::outs() << (val? "true": "false") << "\n";
	printVarRefs();
    }

    void replaceArrayExprWithExpr(
    std::vector<SymbolicArraySubscriptExpr*> origExpr,
    std::vector<SymbolicExpr*> subsituteExpr, bool &rv) {
	rv = true;
    }

    static bool classof (const SymbolicStmt* S) {
	return S->getKind() == CXXBOOLLITERALEXPR;
    }
};

class SymbolicCharacterLiteral: public SymbolicExpr {
public:

    unsigned val;

    SymbolicCharacterLiteral(): SymbolicExpr(CHARACTERLITERAL) {
    }

    z3::expr getZ3Expr(z3::context* c, const MainFunction* mObj,
    std::vector<LoopDetails> surroundingLoops, 
    bool inputFirst, bool &rv);
    virtual std::string getArrayIndices(std::vector<LoopDetails> loops,
    std::vector<InputVar> inputs) {
	std::stringstream returnStr;
	returnStr << val << "L";
	return returnStr.str();
    }

    virtual bool containsVar(VarDetails v, bool &rv) {
	rv = true;
	return false;
    }

    virtual void prettyPrintSummary(std::ofstream &logFile, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
#if 0
	if (printAsGuard) {
	    logFile << "(";
	    prettyPrintGuardExprs(logFile, mObj, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting guardExpr\n";
		return;
	    }
	    logFile << ") && (";
	}
#endif
	if (val != '\n')
	logFile << val;
	//if (printAsGuard) logFile << ")";
    }

    virtual void toString(std::stringstream &logStr, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	if (val != '\n')
	logStr << val;
    }

    virtual bool prettyPrintGuardExprs(std::ofstream &logFile, 
    const MainFunction* mObj, bool &rv, bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	return false;
	//prettyPrintSummary(logFile, mObj, false, rv, false);
	//if (!rv) return;
    }

    virtual void prettyPrint(bool g) {
	if (g) {
	    printGuards();
	    llvm::outs() << "\n";
	    SymbolicStmt::prettyPrintGuardExprs();
	}
	if (val != '\n')
	    llvm::outs() << val;
#if 0
	if (resolved) llvm::outs() << " resolved";
	else    llvm::outs() << " not resolved";
#endif
    }

    virtual SymbolicStmt* clone(bool &rv);

    virtual bool equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicChar::equals()\n";
#endif
	//if (!SymbolicStmt::equals(st)) return false;
	if (!isa<SymbolicCharacterLiteral>(st)) return false;
	SymbolicCharacterLiteral* ssl = dyn_cast<SymbolicCharacterLiteral>(st);
	if (val != ssl->val) return false;
	return true;
    }

    virtual void print() {
	printGuards();
	llvm::outs() << val << "\n";
	printVarRefs();
    }

    void replaceArrayExprWithExpr(
    std::vector<SymbolicArraySubscriptExpr*> origExpr,
    std::vector<SymbolicExpr*> subsituteExpr, bool &rv) {
	rv = true;
    }

    static bool classof (const SymbolicStmt* S) {
	return S->getKind() == CHARACTERLITERAL;
    }
};

class SymbolicStringLiteral: public SymbolicExpr {
public:

    StringRef val;

    SymbolicStringLiteral(): SymbolicExpr(STRINGLITERAL) {
    }

    virtual std::string getArrayIndices(std::vector<LoopDetails> loops,
    std::vector<InputVar> inputs) {
	return val.str();
    }

    virtual bool containsVar(VarDetails v, bool &rv) {
	rv = true;
	return false;
    }

    virtual void prettyPrintSummary(std::ofstream &logFile, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
#if 0
	if (printAsGuard) {
	    logFile << "(";
	    prettyPrintGuardExprs(logFile, mObj, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting guardExpr\n";
		return;
	    }
	    logFile << ") && (";
	}
#endif
	StringRef trimmedString = val.trim();
	logFile << "\"" << trimmedString.str() << "\"";
	//if (printAsGuard) logFile << ")";
    }

    virtual void toString(std::stringstream &logStr, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	StringRef trimmedString = val.trim();
	logStr << "\"" << trimmedString.str() << "\"";
    }

    virtual bool prettyPrintGuardExprs(std::ofstream &logFile, 
    const MainFunction* mObj, bool &rv, bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	return false;
	//prettyPrintSummary(logFile, mObj, false, rv, false);
	//if (!rv) return;
    }

    virtual void prettyPrint(bool g) {
	if (g) {
	    printGuards();
	    llvm::outs() << "\n";
	    SymbolicStmt::prettyPrintGuardExprs();
	}
	StringRef trimmedString = val.trim();
	llvm::outs() << "\"" << trimmedString.str() << "\"";
#if 0
	if (resolved) llvm::outs() << " resolved";
	else    llvm::outs() << " not resolved";
#endif
    }

    virtual SymbolicStmt* clone(bool &rv);

    virtual bool equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicString::equals()\n";
#endif
	//if (!SymbolicStmt::equals(st)) return false;
	if (!isa<SymbolicStringLiteral>(st)) return false;
	SymbolicStringLiteral* ssl = dyn_cast<SymbolicStringLiteral>(st);
	if (!(val.equals(ssl->val))) return false;
	return true;
    }

    virtual void print() {
	printGuards();
	llvm::outs() << val.str() << "\n";
	printVarRefs();
    }

    void replaceArrayExprWithExpr(
    std::vector<SymbolicArraySubscriptExpr*> origExpr,
    std::vector<SymbolicExpr*> subsituteExpr, bool &rv) {
	rv = true;
    }

    static bool classof (const SymbolicStmt* S) {
	return S->getKind() == STRINGLITERAL;
    }
};

class SymbolicBinaryOperator: public SymbolicExpr {
public:

    BinaryOperatorKind opKind;
    SymbolicExpr* lhs;
    SymbolicExpr* rhs;

    SymbolicBinaryOperator(): SymbolicExpr(BINARYOPEXPR) {
	lhs = NULL;
	rhs = NULL;
    }

    z3::expr getZ3Expr(z3::context* c, const MainFunction* mObj,
    std::vector<LoopDetails> surroundingLoops, 
    bool inputFirst, bool &rv);
    virtual std::string getArrayIndices(std::vector<LoopDetails> loops,
    std::vector<InputVar> inputs) {
	std::stringstream returnStr;
	returnStr << lhs->getArrayIndices(loops, inputs) << " ";
	returnStr << BinaryOperator::getOpcodeStr(opKind).str() << " ";
	returnStr << rhs->getArrayIndices(loops, inputs);
	return returnStr.str();
    }

    virtual bool containsVar(VarDetails v, bool &rv) {
	rv = true;
	bool returnVal = false;
	if (lhs) {
	    returnVal = lhs->containsVar(v, rv);
	    if (!rv) return returnVal;
	    if (returnVal) return true;
	}
	if (rhs) {
	    returnVal = rhs->containsVar(v, rv);
	    if (!rv) return returnVal;
	    if (returnVal) return true;
	}
	return returnVal;
    }

    virtual void prettyPrintSummary(std::ofstream &logFile, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
#if 0
	if (printAsGuard) {
	    logFile << "(";
	    prettyPrintGuardExprs(logFile, mObj, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting guardExpr\n";
		return;
	    }
	    logFile << ") && (";
	}
#endif
	if (!isa<SymbolicDeclRefExpr>(lhs) && !lhs->isConstantLiteral())
	    logFile << "(";
	lhs->prettyPrintSummary(logFile, mObj, printAsGuard, rv, inputFirst,
	    inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyprinting lhsExpr\n";
	    return;
	}
	if (!isa<SymbolicDeclRefExpr>(lhs) && !lhs->isConstantLiteral())
	    logFile << ")";
	if (opKind == BinaryOperatorKind::BO_Mul) logFile << " * ";
	else if (opKind == BinaryOperatorKind::BO_Div) logFile << " / ";
	else if (opKind == BinaryOperatorKind::BO_Rem) logFile << " % ";
	else if (opKind == BinaryOperatorKind::BO_Add) logFile << " + ";
	else if (opKind == BinaryOperatorKind::BO_Sub) logFile << " - ";
	else if (opKind == BinaryOperatorKind::BO_LT) logFile << " < ";
	else if (opKind == BinaryOperatorKind::BO_GT) logFile << " > ";
	else if (opKind == BinaryOperatorKind::BO_LE) logFile << " <= ";
	else if (opKind == BinaryOperatorKind::BO_GE) logFile << " >= ";
	else if (opKind == BinaryOperatorKind::BO_EQ) logFile << " == ";
	else if (opKind == BinaryOperatorKind::BO_NE) logFile << " != ";
	else if (opKind == BinaryOperatorKind::BO_LAnd) logFile << " && ";
	else if (opKind == BinaryOperatorKind::BO_LOr) logFile << " || ";
	else if (opKind == BinaryOperatorKind::BO_Or) logFile << " || ";
	else if (opKind == BinaryOperatorKind::BO_And) logFile << " & ";
	else {
	    llvm::outs() << "ERROR: Unsupported BinaryOperator\n";
	    prettyPrint(true);
	    rv = false;
	    return;
	}
	if (!isa<SymbolicDeclRefExpr>(rhs) && !rhs->isConstantLiteral())
	    logFile << "(";
	rhs->prettyPrintSummary(logFile, mObj, printAsGuard, rv, inputFirst,
	    inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyprinting rhsExpr\n";
	    return;
	}
	if (!isa<SymbolicDeclRefExpr>(rhs) && !rhs->isConstantLiteral())
	    logFile << ")";
	//if (printAsGuard) logFile << ")";
    }

    virtual void toString(std::stringstream &logStr, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	if (!isa<SymbolicDeclRefExpr>(lhs) && !lhs->isConstantLiteral())
	    logStr << "(";
	lhs->toString(logStr, mObj, printAsGuard, rv, inputFirst,
	    inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyprinting lhsExpr\n";
	    return;
	}
	if (!isa<SymbolicDeclRefExpr>(lhs) && !lhs->isConstantLiteral())
	    logStr << ")";
	if (opKind == BinaryOperatorKind::BO_Mul) logStr << " * ";
	else if (opKind == BinaryOperatorKind::BO_Div) logStr << " / ";
	else if (opKind == BinaryOperatorKind::BO_Rem) logStr << " % ";
	else if (opKind == BinaryOperatorKind::BO_Add) logStr << " + ";
	else if (opKind == BinaryOperatorKind::BO_Sub) logStr << " - ";
	else if (opKind == BinaryOperatorKind::BO_LT) logStr << " < ";
	else if (opKind == BinaryOperatorKind::BO_GT) logStr << " > ";
	else if (opKind == BinaryOperatorKind::BO_LE) logStr << " <= ";
	else if (opKind == BinaryOperatorKind::BO_GE) logStr << " >= ";
	else if (opKind == BinaryOperatorKind::BO_EQ) logStr << " == ";
	else if (opKind == BinaryOperatorKind::BO_NE) logStr << " != ";
	else if (opKind == BinaryOperatorKind::BO_LAnd) logStr << " && ";
	else if (opKind == BinaryOperatorKind::BO_LOr) logStr << " || ";
	else if (opKind == BinaryOperatorKind::BO_Or) logStr << " || ";
	else if (opKind == BinaryOperatorKind::BO_And) logStr << " & ";
	else {
	    llvm::outs() << "ERROR: Unsupported BinaryOperator\n";
	    prettyPrint(true);
	    rv = false;
	    return;
	}
	if (!isa<SymbolicDeclRefExpr>(rhs) && !rhs->isConstantLiteral())
	    logStr << "(";
	rhs->toString(logStr, mObj, printAsGuard, rv, inputFirst,
	    inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyprinting rhsExpr\n";
	    return;
	}
	if (!isa<SymbolicDeclRefExpr>(rhs) && !rhs->isConstantLiteral())
	    logStr << ")";
    }

    virtual bool prettyPrintGuardExprs(std::ofstream &logFile, 
    const MainFunction* mObj, bool &rv, bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	bool printed = false;
	if (lhs) {
	    printed = printed || lhs->prettyPrintGuardExprs(logFile, mObj, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) return false;
	}
	if (rhs) {
	    printed = printed || rhs->prettyPrintGuardExprs(logFile, mObj, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) return false;
	}
	return printed;
	//prettyPrintSummary(logFile, mObj, false, rv, false);
	//if (!rv) return;
    }

    virtual void prettyPrint(bool g) {
	if (g) {
	    printGuards();
	    llvm::outs() << "\n";
	    SymbolicStmt::prettyPrintGuardExprs();
	}
	llvm::outs() << "(";
	lhs->prettyPrint(false);
	llvm::outs() << ")";
	if (opKind == BinaryOperatorKind::BO_Mul) llvm::outs() << " * ";
	else if (opKind == BinaryOperatorKind::BO_Div) llvm::outs() << " / ";
	else if (opKind == BinaryOperatorKind::BO_Rem) llvm::outs() << " % ";
	else if (opKind == BinaryOperatorKind::BO_Add) llvm::outs() << " + ";
	else if (opKind == BinaryOperatorKind::BO_Sub) llvm::outs() << " - ";
	else if (opKind == BinaryOperatorKind::BO_LT) llvm::outs() << " < ";
	else if (opKind == BinaryOperatorKind::BO_GT) llvm::outs() << " > ";
	else if (opKind == BinaryOperatorKind::BO_LE) llvm::outs() << " <= ";
	else if (opKind == BinaryOperatorKind::BO_GE) llvm::outs() << " >= ";
	else if (opKind == BinaryOperatorKind::BO_EQ) llvm::outs() << " == ";
	else if (opKind == BinaryOperatorKind::BO_NE) llvm::outs() << " != ";
	else if (opKind == BinaryOperatorKind::BO_LAnd) llvm::outs() << " && ";
	else if (opKind == BinaryOperatorKind::BO_LOr) llvm::outs() << " || ";
	else if (opKind == BinaryOperatorKind::BO_Assign) llvm::outs() << " = ";
	else if (opKind == BinaryOperatorKind::BO_Or) llvm::outs() << " | ";
	else if (opKind == BinaryOperatorKind::BO_And) llvm::outs() << " & ";
	else llvm::outs() << "Unsupported BinaryOperator\n";
	llvm::outs() << "(";
	rhs->prettyPrint(false);
	llvm::outs() << ")";
#if 0
	if (resolved) llvm::outs() << " resolved";
	else    llvm::outs() << " not resolved";
#endif
    }

    virtual SymbolicStmt* clone(bool &rv);
    virtual void replaceVarsWithExprs(std::vector<VarDetails> origVars,
    std::vector<SymbolicExpr*> substituteExprs, bool &rv);
    virtual void replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr, 
    SymbolicExpr* substituteExpr, bool &rv);
    void replaceArrayExprWithExpr(
    std::vector<SymbolicArraySubscriptExpr*> origExpr,
    std::vector<SymbolicExpr*> subsituteExpr, bool &rv);

    virtual bool equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicBO::equals()\n";
#endif
	//if (!SymbolicStmt::equals(st)) return false;
	if (!isa<SymbolicBinaryOperator>(st)) return false;
	SymbolicBinaryOperator* sbo = dyn_cast<SymbolicBinaryOperator>(st);
	if (opKind != sbo->opKind) return false;
	if (lhs && !(sbo->lhs)) return false;
	if (!lhs && sbo->lhs) return false;
	if (lhs && sbo->lhs && !(lhs->equals(sbo->lhs))) return false;
	if (rhs && !(sbo->rhs)) return false;
	if (!rhs && sbo->rhs) return false;
	if (rhs && sbo->rhs && !(rhs->equals(sbo->rhs))) return false;
	return true;
    }

    //virtual ~SymbolicBinaryOperator();
    virtual void print() {
	printGuards();
	if (opKind == BO_PtrMemD)
	    llvm::outs() << "BO_PtrMemD";
	else if (opKind == BO_PtrMemI)
	    llvm::outs() << "BO_PtrMemI";
	else if (opKind == BO_Mul)
	    llvm::outs() << "BO_Mul";
	else if (opKind == BO_Div)
	    llvm::outs() << "BO_Div";
	else if (opKind == BO_Rem)
	    llvm::outs() << "BO_Rem";
	else if (opKind == BO_Add)
	    llvm::outs() << "BO_Add";
	else if (opKind == BO_Sub)
	    llvm::outs() << "BO_Sub";
	else if (opKind == BO_Shl)
	    llvm::outs() << "BO_Shl";
	else if (opKind == BO_Shr)
	    llvm::outs() << "BO_Shr";
	else if (opKind == BO_LT)
	    llvm::outs() << "BO_LT";
	else if (opKind == BO_GT)
	    llvm::outs() << "BO_GT";
	else if (opKind == BO_LE)
	    llvm::outs() << "BO_LE";
	else if (opKind == BO_GE)
	    llvm::outs() << "BO_GE";
	else if (opKind == BO_EQ)
	    llvm::outs() << "BO_EQ";
	else if (opKind == BO_NE)
	    llvm::outs() << "BO_NE";
	else if (opKind == BO_And)
	    llvm::outs() << "BO_And";
	else if (opKind == BO_Xor)
	    llvm::outs() << "BO_Xor";
	else if (opKind == BO_Or)
	    llvm::outs() << "BO_Or";
	else if (opKind == BO_LAnd)
	    llvm::outs() << "BO_LAnd";
	else if (opKind == BO_LOr)
	    llvm::outs() << "BO_LOr";
	else if (opKind == BO_Assign)
	    llvm::outs() << "BO_Assign";
	else if (opKind == BO_MulAssign)
	    llvm::outs() << "BO_MulAssign";
	else if (opKind == BO_DivAssign)
	    llvm::outs() << "BO_DivAssign";
	else if (opKind == BO_RemAssign)
	    llvm::outs() << "BO_RemAssign";
	else if (opKind == BO_AddAssign)
	    llvm::outs() << "BO_AddAssign";
	else if (opKind == BO_SubAssign)
	    llvm::outs() << "BO_SubAssign";
	else if (opKind == BO_ShlAssign)
	    llvm::outs() << "BO_ShlAssign";
	else if (opKind == BO_ShrAssign)
	    llvm::outs() << "BO_ShrAssign";
	else if (opKind == BO_AndAssign)
	    llvm::outs() << "BO_AndAssign";
	else if (opKind == BO_XorAssign)
	    llvm::outs() << "BO_XorAssign";
	else if (opKind == BO_OrAssign)
	    llvm::outs() << "BO_OrAssign";
	else if (opKind == BO_Comma)
	    llvm::outs() << "BO_Comma";
	else
	    llvm::outs() << "Unknown operator";
	llvm::outs() << "\n";
	llvm::outs() << "LHS: ";
	lhs->print();
	llvm::outs() << "RHS: ";
	rhs->print();
	printVarRefs();
    }

    static bool classof (const SymbolicStmt* S) {
	return S->getKind() == BINARYOPEXPR;
    }
};

class SymbolicConditionalOperator: public SymbolicExpr {
public:

    SymbolicExpr* condition;
    SymbolicExpr* trueExpr;
    SymbolicExpr* falseExpr;

    SymbolicConditionalOperator(): SymbolicExpr(CONDITIONALOPEXPR) {
	condition = NULL;
	trueExpr = NULL;
	falseExpr = NULL;
    }

    z3::expr getZ3Expr(z3::context* c, const MainFunction* mObj,
    std::vector<LoopDetails> surroundingLoops, 
    bool inputFirst, bool &rv);
    virtual bool containsVar(VarDetails v, bool &rv) {
	rv = true;
	bool returnVal = false;
	if (condition) {
	    returnVal = condition->containsVar(v, rv);
	    if (!rv) return returnVal;
	    if (returnVal) return true;
	}
	if (trueExpr) {
	    returnVal = trueExpr->containsVar(v, rv);
	    if (!rv) return returnVal;
	    if (returnVal) return true;
	}
	if (falseExpr) {
	    returnVal = falseExpr->containsVar(v, rv);
	    if (!rv) return returnVal;
	    if (returnVal) return true;
	}
	return returnVal;
    }

    virtual void prettyPrintSummary(std::ofstream &logFile, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
#if 0
	if (printAsGuard) {
	    logFile << "(";
	    prettyPrintGuardExprs(logFile, mObj, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting guardExpr\n";
		return;
	    }
	    logFile << ") && (";
	}
#endif
	logFile << "(";
	condition->prettyPrintSummary(logFile, mObj, printAsGuard, rv,
	    inputFirst, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyprinting condition\n";
	    return;
	}
	logFile << ")? ";
	if (!isa<SymbolicDeclRefExpr>(trueExpr) &&
	    !trueExpr->isConstantLiteral())
	    logFile << "(";
	trueExpr->prettyPrintSummary(logFile, mObj, printAsGuard, rv,
	    inputFirst, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyprinting trueExpr\n";
	    return;
	}
	if (!isa<SymbolicDeclRefExpr>(trueExpr) &&
	    !trueExpr->isConstantLiteral())
	    logFile << ")";
	logFile << ": ";
	if (!isa<SymbolicDeclRefExpr>(falseExpr) &&
	    !falseExpr->isConstantLiteral())
	    logFile << "(";
	falseExpr->prettyPrintSummary(logFile, mObj, printAsGuard, rv,
	    inputFirst, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyprinting falseExpr\n";
	    return;
	}
	if (!isa<SymbolicDeclRefExpr>(falseExpr) &&
	    !falseExpr->isConstantLiteral())
	    logFile << ")";
	//if (printAsGuard) logFile << ")";
    }

    virtual void toString(std::stringstream &logStr, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	logStr << "(";
	condition->toString(logStr, mObj, printAsGuard, rv,
	    inputFirst, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyprinting condition\n";
	    return;
	}
	logStr << ")? ";
	if (!isa<SymbolicDeclRefExpr>(trueExpr) &&
	    !trueExpr->isConstantLiteral())
	    logStr << "(";
	trueExpr->toString(logStr, mObj, printAsGuard, rv,
	    inputFirst, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyprinting trueExpr\n";
	    return;
	}
	if (!isa<SymbolicDeclRefExpr>(trueExpr) &&
	    !trueExpr->isConstantLiteral())
	    logStr << ")";
	logStr << ": ";
	if (!isa<SymbolicDeclRefExpr>(falseExpr) &&
	    !falseExpr->isConstantLiteral())
	    logStr << "(";
	falseExpr->toString(logStr, mObj, printAsGuard, rv,
	    inputFirst, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyprinting falseExpr\n";
	    return;
	}
	if (!isa<SymbolicDeclRefExpr>(falseExpr) &&
	    !falseExpr->isConstantLiteral())
	    logStr << ")";
    }

    virtual bool prettyPrintGuardExprs(std::ofstream &logFile, 
    const MainFunction* mObj, bool &rv, bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	bool printed = false;
	if (condition) {
	    printed = printed || condition->prettyPrintGuardExprs(logFile, mObj,
		rv, inputFirst, inResidual, surroundingLoops);
	    if (!rv) return false;
	}
	if (trueExpr) {
	    printed = printed || trueExpr->prettyPrintGuardExprs(logFile, mObj,
		rv, inputFirst, inResidual, surroundingLoops);
	    if (!rv) return false;
	}
	if (falseExpr) {
	    printed = printed || falseExpr->prettyPrintGuardExprs(logFile, mObj,
		rv, inputFirst, inResidual, surroundingLoops);
	    if (!rv) return false;
	}
	return printed;
    }

    virtual void prettyPrint(bool g) {
	if (g) {
	    printGuards();
	    llvm::outs() << "\n";
	    SymbolicStmt::prettyPrintGuardExprs();
	}
	llvm::outs() << "(";
	condition->prettyPrint(false);
	llvm::outs() << ")? (";
	trueExpr->prettyPrint(false);
	llvm::outs() << "): (";
	falseExpr->prettyPrint(false);
	llvm::outs() << ")";
#if 0
	if (resolved) llvm::outs() << " resolved";
	else    llvm::outs() << " not resolved";
#endif
    }

    virtual SymbolicStmt* clone(bool &rv);
    virtual void replaceVarsWithExprs(std::vector<VarDetails> origVars,
    std::vector<SymbolicExpr*> substituteExprs, bool &rv);
    virtual void replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr, 
    SymbolicExpr* substituteExpr, bool &rv);
    void replaceArrayExprWithExpr(
    std::vector<SymbolicArraySubscriptExpr*> origExpr,
    std::vector<SymbolicExpr*> subsituteExpr, bool &rv);

    virtual bool equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicCO::equals()\n";
#endif
	//if (!SymbolicStmt::equals(st)) return false;
	if (!isa<SymbolicConditionalOperator>(st)) return false;
	SymbolicConditionalOperator* sbo = dyn_cast<SymbolicConditionalOperator>(st);
	if (condition && !(sbo->condition)) return false;
	if (!condition && sbo->condition) return false;
	if (condition && sbo->condition && !(condition->equals(sbo->condition))) 
	    return false;
	if (trueExpr && !(sbo->trueExpr)) return false;
	if (!trueExpr && sbo->trueExpr) return false;
	if (trueExpr && sbo->trueExpr && !(trueExpr->equals(sbo->trueExpr))) 
	    return false;
	if (falseExpr && !(sbo->falseExpr)) return false;
	if (!falseExpr && sbo->falseExpr) return false;
	if (falseExpr && sbo->falseExpr && !(falseExpr->equals(sbo->falseExpr))) 
	    return false;
	return true;
    }

    //virtual ~SymbolicConditionalOperator();
    virtual void print() {
	printGuards();
	llvm::outs() << "Condition: ";
	condition->print();
	llvm::outs() << "trueExpr: ";
	trueExpr->print();
	llvm::outs() << "falseExpr: ";
	falseExpr->print();
	printVarRefs();
    }

    static bool classof (const SymbolicStmt* S) {
	return S->getKind() == CONDITIONALOPEXPR;
    }
};

class SymbolicSpecialExpr : public SymbolicExpr {
public:
    SpecialExprKind sKind;
    SymbolicDeclRefExpr* arrayVar;
    SymbolicExpr* initExpr;
    std::vector<SymbolicExpr*> indexExprsAtAssignLine;
    typedef std::pair<SymbolicExpr*, SymbolicExpr*> RANGE;
    std::vector<std::pair<RANGE, bool> > // bool - true if the range is strict
	indexRangesAtAssignLine;	 // <1 .. n, true> = 1 .. n-1
    bool sane;
    SpecialExpr* originalSpecialExpr;

    SymbolicSpecialExpr(): SymbolicExpr(SPECIAL) {
	arrayVar = NULL;
	initExpr = NULL;
	sane = true;
	originalSpecialExpr = NULL;
    }

    z3::expr getZ3Expr(z3::context* c, const MainFunction* mObj,
    std::vector<LoopDetails> surroundingLoops, 
    bool inputFirst, bool &rv);
    virtual bool containsVar(VarDetails v, bool &rv) {
	rv = true;
	bool returnVal = false;
	if (arrayVar) {
	    returnVal = arrayVar->containsVar(v, rv);
	    if (!rv) return returnVal;
	    if (returnVal) return true;
	}
	if (initExpr) {
	    returnVal = initExpr->containsVar(v, rv);
	    if (!rv) return returnVal;
	    if (returnVal) return true;
	}
	for (std::vector<SymbolicExpr*>::iterator iIt =
		indexExprsAtAssignLine.begin(); iIt != 
		indexExprsAtAssignLine.end(); iIt++) {
	    if (*iIt) {
		returnVal = (*iIt)->containsVar(v, rv);
		if (!rv) return returnVal;
		if (returnVal) return true;
	    }
	}
	for (std::vector<std::pair<RANGE, bool> >::iterator iIt = 
		indexRangesAtAssignLine.begin(); iIt != 
		indexRangesAtAssignLine.end(); iIt++) {
	    if (iIt->first.first) {
		returnVal = iIt->first.first->containsVar(v, rv);
		if (!rv) return returnVal;
		if (returnVal) return true;
	    }
	    if (iIt->first.second) {
		returnVal = iIt->first.second->containsVar(v, rv);
		if (!rv) return returnVal;
		if (returnVal) return true;
	    }
	}
	return returnVal;
    }

    virtual void prettyPrintSummary(std::ofstream &logFile, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>());

    virtual void toString(std::stringstream &logStr, 
    const MainFunction* mObj, bool printAsGuard, bool &rv, 
    bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>());

    virtual bool prettyPrintGuardExprs(std::ofstream &logFile, 
    const MainFunction* mObj, bool &rv, bool inputFirst=false, bool inResidual=false,
    std::vector<LoopDetails> surroundingLoops=std::vector<LoopDetails>()) {
	rv = true;
	bool printed = false;
	if (initExpr) {
	    printed = printed || initExpr->prettyPrintGuardExprs(logFile, mObj,
		rv, inputFirst, inResidual, surroundingLoops);
	    if (!rv) return false;
	}
	if (arrayVar) {
	    printed = printed || arrayVar->prettyPrintGuardExprs(logFile, mObj,
		rv, inputFirst, inResidual, surroundingLoops);
	    if (!rv) return false;
	}
	for (unsigned i = 0; i < indexRangesAtAssignLine.size(); i++) {
	    if (indexRangesAtAssignLine[i].first.first) {
		printed = printed ||
		    indexRangesAtAssignLine[i].first.first->prettyPrintGuardExprs(logFile,
		    mObj, rv, inputFirst, inResidual, surroundingLoops);
		if (!rv) return false;
	    }
	    if (indexRangesAtAssignLine[i].first.second) {
		printed = printed ||
		    indexRangesAtAssignLine[i].first.second->prettyPrintGuardExprs(logFile,
		    mObj, rv, inputFirst, inResidual, surroundingLoops);
		if (!rv) return false;
	    }
	}
	return printed;
	//prettyPrintSummary(logFile, mObj, false, rv, false);
	//if (!rv) return;
    }

    virtual void prettyPrint(bool g) {
	if (g) {
	    printGuards();
	    llvm::outs() << "\n";
	    SymbolicStmt::prettyPrintGuardExprs();
	}
	if (sKind == SpecialExprKind::MAX) llvm::outs() << "symex_max";
	else if (sKind == SpecialExprKind::MIN) llvm::outs() << "symex_min";
	else if (sKind == SpecialExprKind::SUM) llvm::outs() << "symex_sum";
	else llvm::outs() << "Unresolved SpecialExprKind\n";
	llvm::outs() << "(";
	initExpr->prettyPrint(false);
	llvm::outs() << ", ";
	arrayVar->prettyPrint(false);
	for (std::vector<std::pair<RANGE, bool> >::iterator iIt =
		indexRangesAtAssignLine.begin(); iIt != 
		indexRangesAtAssignLine.end(); iIt++) {
	    llvm::outs() << "[";
	    if (iIt->first.first->equals(iIt->first.second)) {
		iIt->first.first->prettyPrint(false);
	    } else {
		iIt->first.first->prettyPrint(false);
		llvm::outs() << " ... ";
		iIt->first.second->prettyPrint(false);
		if (iIt->second) {
		    // Strict bound
		    llvm::outs() << " - 1";
		}
	    }
	    llvm::outs() << "]";
	}
	llvm::outs() << ")";
#if 0
	if (resolved) llvm::outs() << " resolved";
	else    llvm::outs() << " not resolved";
#endif
    }

    virtual bool equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicSpecial::equals()\n";
#endif
	//if (!SymbolicStmt::equals(st)) return false;
	if (!isa<SymbolicSpecialExpr>(st)) return false;
	SymbolicSpecialExpr* sse = dyn_cast<SymbolicSpecialExpr>(st);
	if (sKind != sse->sKind) return false;
	if (arrayVar && !(sse->arrayVar)) return false;
	if (!arrayVar && sse->arrayVar) return false;
	if (arrayVar && sse->arrayVar && !(arrayVar->equals(sse->arrayVar)))
	    return false;
	if (initExpr && !(sse->initExpr)) return false;
	if (!initExpr && sse->initExpr) return false;
	if (initExpr && sse->initExpr && !(initExpr->equals(sse->initExpr)))
	    return false;
	if (indexExprsAtAssignLine.size() != sse->indexExprsAtAssignLine.size())
	    return false;
	for (unsigned i = 0; i < indexExprsAtAssignLine.size(); i++) {
	    if (indexExprsAtAssignLine[i] && !(sse->indexExprsAtAssignLine[i]))
		return false;
	    if (!(indexExprsAtAssignLine[i]) && sse->indexExprsAtAssignLine[i])
		return false;
	    if (indexExprsAtAssignLine[i] && sse->indexExprsAtAssignLine[i] &&
		!(indexExprsAtAssignLine[i]->equals(sse->indexExprsAtAssignLine[i])))
		return false;
	}

	return true;
    }

    virtual SymbolicStmt* clone(bool &rv);
    virtual void replaceVarsWithExprs(std::vector<VarDetails> origVars,
    std::vector<SymbolicExpr*> substituteExprs, bool &rv);
    virtual void replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr, 
    SymbolicExpr* substituteExpr, bool &rv);
    void replaceArrayExprWithExpr(
    std::vector<SymbolicArraySubscriptExpr*> origExpr,
    std::vector<SymbolicExpr*> subsituteExpr, bool &rv);

    //virtual ~SymbolicSpecialExpr();
    virtual void print() {
	printGuards();
	if (sKind == MAX) llvm::outs() << "MAX";
	else if (sKind == MIN) llvm::outs() << "MIN";
	else if (sKind == SUM) llvm::outs() << "SUM";
	else llvm::outs() << "UNRESOLVED";
	llvm::outs() << "Array: ";
	if (arrayVar)
	    arrayVar->print();
	else
	    llvm::outs() << "NULL";
	llvm::outs() << "\n";
	llvm::outs() << "Init Expr: ";
	if (initExpr)
	    initExpr->print();
	else
	    llvm::outs() << "NULL";
	llvm::outs() << "\n";
	llvm::outs() << "Index Exprs at Assign Line:\n";
	for (std::vector<SymbolicExpr*>::iterator sIt =
		indexExprsAtAssignLine.begin(); sIt != 
		indexExprsAtAssignLine.end(); sIt++) {
	    if (*sIt)
		(*sIt)->print();
	}
	llvm::outs() << "Index Range Exprs at Assign Line:\n";
	for (std::vector<std::pair<RANGE, bool> >::iterator 
		sIt = indexRangesAtAssignLine.begin(); sIt != 
		indexRangesAtAssignLine.end(); sIt++) {
	    if (sIt->first.first && sIt->first.second) {
		if (sIt->first.first->equals(sIt->first.second)) {
		    llvm::outs() << "Both ends of range are equal\n";
		    sIt->first.first->print();
		} else {
		    sIt->first.first->print();
		    llvm::outs() << " ... ";
		    sIt->first.second->print();
		    if (sIt->second)
			llvm::outs() << "Strict\n";
		    else
			llvm::outs() << "Not strict\n";
		}
	    }
	}
	printVarRefs();
    }

    static bool classof(const SymbolicStmt* S) {
	return S->getKind() == SPECIAL;
    }
};

class GetSymbolicExprVisitor: public clang::RecursiveASTVisitor<GetSymbolicExprVisitor> {
public:
    const SourceManager *SM;
    FunctionAnalysis *FA;
    MainFunction* mainObj;

    std::vector<std::pair<StmtDetails*, std::vector<SymbolicStmt*> > > symExprMap;
    std::vector<std::pair<VarDetails, std::vector<SymbolicExpr*> > > varSizeSymExprMap;
    bool error; // true if we failed any of the soundness checks
    unsigned currBlock;
    std::vector<std::vector<std::pair<unsigned, bool> > > currGuards;
    std::vector<StmtDetails*> residualStmts;
    bool waitingSet;
    unsigned waitingForBlock;

#if 0
    GetSymbolicExprVisitor() {
	SM = NULL;
	FA = NULL;
	mainObj = NULL;
	error = true;
	currBlock = -1;
    }
#endif

    GetSymbolicExprVisitor(const SourceManager* M, FunctionAnalysis* F,
    MainFunction* main) {
	SM = M;
	FA = F;
	mainObj = main;
	error = false;
	currBlock = -1;
	waitingForBlock = 0;
	waitingSet = false;
    }

    void unsetWaitingForBlock() {
	waitingSet = false;
    }

    SymbolicExpr* getSymbolicExprForType(const ArrayType* T, bool &rv);
    std::vector<SymbolicExpr*> getSymExprsForArrayType(VarDetails vd) {
	for (std::vector<std::pair<VarDetails, std::vector<SymbolicExpr*> >
		>::iterator it = varSizeSymExprMap.begin(); it != 
		varSizeSymExprMap.end(); it++) {
	    if (it->first.equals(vd)) 
		return it->second;
	}
	return std::vector<SymbolicExpr*>();
    }

    void printSymExprMap(bool printGuardExprs=false);

    void prettyPrintSymExprMap(bool printGuardExprs=false);

    void printSymExprs(StmtDetails* ed);

    bool isExprPresentInSymMap(StmtDetails* ed);

    // This function returns true if the insertion was successful. If there was
    // an already existing entry for the expression ed, then the function
    // returns false.
    bool insertExprInSymMap(StmtDetails* sd, SymbolicStmt* sst);

    bool replaceExprInSymMap(StmtDetails* sd, SymbolicStmt* sst);

    std::vector<SymbolicStmt*> getSymbolicExprs(StmtDetails* sd, bool &rv);

    void removeSymExprs(StmtDetails* sd, bool &rv);

    bool isSizeResolved(VarDetails vd) {
	if (!vd.isArray()) return true;
	bool found = false;
	for (std::vector<std::pair<VarDetails, std::vector<SymbolicExpr*> >
		>::iterator vIt = varSizeSymExprMap.begin(); vIt != 
		varSizeSymExprMap.end(); vIt++) {
#ifdef DEBUGFULL
	    llvm::outs() << "DEBUG: Var: ";
	    vIt->first.print();
	    llvm::outs() << "\n";
#endif
	    if (!vIt->first.equals(vd)) continue;
	    found = true;
	    if (vIt->second.size() == 0) return false;
	    for (std::vector<SymbolicExpr*>::iterator sIt = vIt->second.begin();
		    sIt != vIt->second.end(); sIt++) {
#ifdef DEBUGFULL
		(*sIt)->prettyPrint(true);
		llvm::outs() << "\n";
#endif
		if (!(*sIt)->resolved) return false;
	    }
	}
	if (!found) return false;
	return true;
    }

    bool isResolved(StmtDetails* sd) {
	bool rv;
	std::vector<SymbolicStmt*> symbolicExprs = getSymbolicExprs(sd, rv);
	if (!rv) {
	    // Could not find symbolic exprs for sd, then it is not resolved
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Could not find symbolicexprs\n";
#endif
	    return false;
	}

#ifdef DEBUGFULL1
	llvm::outs() << "DEBUG: Symbolic exprs for:\n";
	sd->print();
	for (std::vector<SymbolicStmt*>::iterator sIt = symbolicExprs.begin();
		sIt != symbolicExprs.end(); sIt++)
	    (*sIt)->print();
#endif

	for (std::vector<SymbolicStmt*>::iterator sIt = symbolicExprs.begin();
		sIt != symbolicExprs.end(); sIt++) {
	    if (!((*sIt)->resolved)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: SymExpr ";
		(*sIt)->prettyPrint(true);
		llvm::outs() << " is not resolved\n";
#endif
		return false;
	    }
	}

	return true;
    }

    void setCurrBlock(unsigned b) {
	currBlock = b;
    }

    void setCurrGuards(std::vector<std::vector<std::pair<unsigned, bool> > > g) {
	currGuards.clear();
	currGuards.insert(currGuards.end(), g.begin(), g.end());
    }

    void clearCurrGuards() {
	currGuards.clear();
    }

    void getGuardExprs(bool &rv);

    // visitors
    bool VisitDeclRefExpr(DeclRefExpr* E);
    bool VisitArraySubscriptExpr(ArraySubscriptExpr* A);

    bool VisitDeclStmt(DeclStmt* D);
    bool VisitVarDecl(VarDecl* V);
    bool VisitReturnStmt(ReturnStmt* R);

    bool VisitIntegerLiteral(IntegerLiteral* I);
    bool VisitImaginaryLiteral(ImaginaryLiteral* I);
    bool VisitFloatingLiteral(FloatingLiteral* F);
    bool VisitCXXBoolLiteralExpr(CXXBoolLiteralExpr* B);
    bool VisitCharacterLiteral(CharacterLiteral* S);
    bool VisitStringLiteral(StringLiteral* S);
    bool VisitInitListExpr(InitListExpr* I);
    bool VisitUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr* U);
    bool VisitCXXOperatorCallExpr(CXXOperatorCallExpr* C);
    bool VisitCallExpr(CallExpr* C);
    bool VisitUnaryOperator(UnaryOperator* U);
    bool VisitBinaryOperator(BinaryOperator* B);
    bool VisitConditionalOperator(ConditionalOperator* C);
};


#endif /* GETSYMBOLICEXPRVISITOR_H */
