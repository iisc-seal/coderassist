#ifndef MYASTVISITOR_H
#define MYASTVISITOR_H

#include "includes.h"
#include "Helper.h"
#include "FunctionAnalysis.h"
#include "MainFunction.h"
#include "StmtDetails.h"
#include "clang/AST/RecursiveASTVisitor.h"

// Visitor to obtain all vars present in a Stmt
class GetVarVisitor: public clang::RecursiveASTVisitor<GetVarVisitor> {
public:
    const SourceManager *SM;
    bool error;
    // We store the line number of variable occurence and its details.
    struct varOccurrence {
	int lineNum;
	VarDetails vd;
	int numOfOccurrences;
    };
    std::vector<struct varOccurrence> scalarVarsFound;
    std::vector<struct varOccurrence> arrayVarsFound;

    bool VisitDeclRefExpr(DeclRefExpr *E);
    bool VisitArraySubscriptExpr(ArraySubscriptExpr *E);
    
    GetVarVisitor() {
	SM = NULL;
	error = true;
    }

    GetVarVisitor(const SourceManager *M) {
	SM = M;
	error = false;
    }

    void clear() {
	scalarVarsFound = std::vector<struct varOccurrence>();
	arrayVarsFound = std::vector<struct varOccurrence>();
    }

    bool isScalarVarPresent(std::string name, int declLine) {
	for (std::vector<struct varOccurrence>::iterator vIt =
		scalarVarsFound.begin(); vIt != scalarVarsFound.end(); vIt++) {
	    if (vIt->vd.varName.compare(name) == 0 && 
		    vIt->vd.varDeclLine == declLine)
		return true;
	}
	return false;
    }

    void printVars() {
	llvm::outs() << "Scalar Vars Found:\n";
	for (std::vector<struct varOccurrence>::iterator it = 
		scalarVarsFound.begin(); it != scalarVarsFound.end(); it++)
	    llvm::outs() << "At line " << it->lineNum << ", var " 
			 << it->vd.varName << " (" << it->vd.varDeclLine 
			 << ") - " << it->numOfOccurrences << "\n";

	llvm::outs() << "Array Vars Found:\n";
	for (std::vector<struct varOccurrence>::iterator it = 
		arrayVarsFound.begin(); it != arrayVarsFound.end(); it++)
	    llvm::outs() << "At line " << it->lineNum << ", var " 
			 << it->vd.varName << " (" << it->vd.varDeclLine 
			 << ") - " << it->numOfOccurrences << "\n";
    }
};

class CallExprVisitor: public clang::RecursiveASTVisitor<CallExprVisitor> {
public:
    const SourceManager* SM;
    bool error;	// true if we encountered any error
    std::vector<FunctionDetails*> funcsFound;
    std::vector<std::string> customInputFuncs;
    std::vector<std::string> customOutputFuncs;
    CallGraph* cg;

    CallExprVisitor(const SourceManager* M, CallGraph* g,
	std::vector<std::string> inpFuncs, std::vector<std::string> outFuncs) {
	error = false;
	SM = M;
	cg = g;
	funcsFound = std::vector<FunctionDetails*>();
        customInputFuncs.insert(customInputFuncs.end(), inpFuncs.begin(),
            inpFuncs.end());
        customOutputFuncs.insert(customOutputFuncs.end(), outFuncs.begin(),
            outFuncs.end());
    }

    void clear() {
	funcsFound.clear();
    }

    bool VisitCallExpr(CallExpr* E);
};

class InputStmtVisitor: public clang::RecursiveASTVisitor<InputStmtVisitor> {
public:
    const SourceManager* SM;
    bool error;
    std::vector<int> inputLinesFound;
    std::vector<InputVar> inputVarsFound;
    std::vector<std::string> customInputFuncs;
    std::vector<StmtDetails*> inputStmts;
    FunctionDetails* currFunction;

    InputStmtVisitor() {
	error = true;
	SM = NULL;
    }

    InputStmtVisitor(const SourceManager* M, std::vector<std::string> ifuncs) {
	error = false;
	SM = M;
	customInputFuncs.insert(customInputFuncs.end(), ifuncs.begin(),
	    ifuncs.end());
    }

    InputStmtVisitor(const SourceManager* M, std::string ifunc) {
	error = false;
	SM = M;
	customInputFuncs.push_back(ifunc);
    }

    InputStmtVisitor(const SourceManager* M) {
	error = false;
	SM = M;
    }

    void addInputFunc(std::string ifunc) {
	if (std::find(customInputFuncs.begin(), customInputFuncs.end(), ifunc) ==
		customInputFuncs.end())
	    customInputFuncs.push_back(ifunc);
	else
	    llvm::outs() << "WARNING: Custom input function " << ifunc
			 << " already recorded\n";
    }

    void setCurrFunction(FunctionDetails* f) {
	currFunction = f;
    }

    bool isInputStmtRecorded(StmtDetails* s) {
	for (std::vector<StmtDetails*>::iterator sIt = inputStmts.begin();
		sIt != inputStmts.end(); sIt++) {
	    if (s->equals(*sIt)) return true;
	}

	return false;
    }

    bool VisitCallExpr(CallExpr* E);
    bool VisitBinaryOperator(BinaryOperator* B);
};

class SequenceInputsVisitor: public RecursiveASTVisitor<SequenceInputsVisitor> {
public:
    const SourceManager* SM;
    CallGraph* cg;
    std::vector<InputVar> sequencedInputVars;
    std::vector<InputVar> inputsInCurrFunc;
    FunctionDetails* currFunc;
    bool error;
    MainFunction* MObj;

    SequenceInputsVisitor() {
	error = true;
	cg = NULL;
	MObj = NULL;
	SM = NULL;
	currFunc = NULL;
    }

    SequenceInputsVisitor(const SourceManager* S, CallGraph* C, MainFunction* M,
    FunctionDetails* f, std::vector<InputVar> in) {
	error = false;
	cg = C;
	MObj = M;
	SM = S;
	currFunc = f->cloneAsPtr();
	inputsInCurrFunc.insert(inputsInCurrFunc.end(), in.begin(), in.end());

	// sort inputs in terms of line number
	std::sort(inputsInCurrFunc.begin(), inputsInCurrFunc.end());
#if 0
	llvm::outs() << "DEBUG: SequenceInputsVisitor(): inputsInCurrFunc:\n";
	for (std::vector<InputVar>::iterator it = inputsInCurrFunc.begin(); it 
		!= inputsInCurrFunc.end(); it++)
	    it->print();
#endif
    }

    bool TraverseFunctionDecl(FunctionDecl* fd);
    bool VisitCallExpr(CallExpr* E);
};

class SpecialStmtVisitor : public RecursiveASTVisitor<SpecialStmtVisitor> {
public:
    const SourceManager* SM;
    bool error;
    std::vector<SpecialExpr> exprsFound;

    SpecialStmtVisitor() {
	error = true;
	SM = NULL;
    }

    SpecialStmtVisitor(const SourceManager* M) {
	error = false;
	SM = M;
	exprsFound = std::vector<SpecialExpr>();
    }

    bool VisitBinaryOperator(BinaryOperator* B);
};

class InputOutputStmtVisitor: public RecursiveASTVisitor<InputOutputStmtVisitor> {
public:
    const SourceManager* SM;
    bool error;
    std::vector<std::string> customInputFuncs;
    std::vector<std::string> customOutputFuncs;

    std::vector<int> inputLinesFound;
    std::vector<int> outputLinesFound;

    std::vector<InputVar> inputVarsFound;

    std::vector<StmtDetails*> inputStmts;
    std::vector<StmtDetails*> outputStmts;

    MainFunction* mObj;
    FunctionDetails* currFunction;
    unsigned currBlock;

    std::vector<CallExprDetails*> callExprsVisited;

    // Allow only those constructors that set all the pointer members
#if 0
    InputOutputStmtVisitor() {
	error = true;
	SM = NULL;
	mObj = NULL;
	currBlock = -1;
    }
#endif

    InputOutputStmtVisitor(const SourceManager* M, MainFunction* main, 
    std::vector<std::string> ifuncs, std::vector<std::string> ofuncs) {
	error = false;
	SM = M;
	mObj = main;
	currBlock = -1;
	customInputFuncs.insert(customInputFuncs.end(), ifuncs.begin(),
	    ifuncs.end());
	customOutputFuncs.insert(customOutputFuncs.end(), ofuncs.begin(),
	    ofuncs.end());
    }

    InputOutputStmtVisitor(const SourceManager* M, MainFunction* main,
    std::string ifunc, std::string ofunc) {
	error = false;
	SM = M;
	mObj = main;
	currBlock = -1;
	customInputFuncs.push_back(ifunc);
	if (ofunc.compare("") != 0)
	    customOutputFuncs.push_back(ofunc);
    }

    InputOutputStmtVisitor(const SourceManager* M, MainFunction* main) {
	error = false;
	SM = M;
	mObj = main;
	currBlock = -1;
    }

    InputOutputStmtVisitor(InputOutputStmtVisitor* v) {
	error = false;
	SM = v->SM;
	mObj = v->mObj;
	customInputFuncs.insert(customInputFuncs.end(),
	    v->customInputFuncs.begin(), v->customInputFuncs.end());
	customOutputFuncs.insert(customOutputFuncs.end(),
	    v->customOutputFuncs.begin(), v->customOutputFuncs.end());
    }

    void addInputFunc(std::string ifunc) {
	if (std::find(customInputFuncs.begin(), customInputFuncs.end(), ifunc) ==
		customInputFuncs.end())
	    customInputFuncs.push_back(ifunc);
	else
	    llvm::outs() << "WARNING: Custom input function " << ifunc
			 << " already recorded\n";
    }

    void addOutputFunc(std::string ofunc) {
	if (std::find(customOutputFuncs.begin(), customOutputFuncs.end(), ofunc) == 
		customOutputFuncs.end())
	    customOutputFuncs.push_back(ofunc);
	else
	    llvm::outs() << "WARNING: Custom output function " << ofunc
			 << " already recorded\n";
    }

    void setCurrFunction(FunctionDetails* f) {
	currFunction = f;
    }

    void setCurrBlock(unsigned blockID) {
	currBlock = blockID;
    }

    bool isInputStmtRecorded(StmtDetails* s) {
	for (std::vector<StmtDetails*>::iterator sIt = inputStmts.begin();
		sIt != inputStmts.end(); sIt++) {
	    if (s->equals(*sIt)) return true;
	}

	return false;
    }

    // If input stmt found is not exacty equal to any of the recorded input
    // stmts, it does not mean that it is completely distinct from any of the
    // recorded stmts. It could be that the current stmt adds another argument
    // to one of the recorded stmts.
    StmtDetails* getSimilarInputStmt(StmtDetails* s, bool &rv) {
	rv = true;
	if (!isa<CallExprDetails>(s)) return NULL;
	CallExprDetails* ced = dyn_cast<CallExprDetails>(s);
	for (std::vector<StmtDetails*>::iterator sIt = inputStmts.begin();
		sIt != inputStmts.end(); sIt++) {
	    if (!((*sIt)->StmtDetails::equals(s))) continue;
	    if (!isa<CallExprDetails>(*sIt)) continue;
#if 0
	    if (s->getKind() != (*sIt)->getKind()) continue;
	    if (s->blockID != (*sIt)->blockID) continue;
	    if (s->lineNum != (*sIt)->lineNum) continue;
#endif
	    CallExprDetails* existingCED = dyn_cast<CallExprDetails>(*sIt);
	    if (!existingCED->callee || !ced->callee) {
		llvm::outs() << "ERROR: NULL Callee in InputStmt\n";
		if (!existingCED->callee) existingCED->print();
		if (!ced->callee) ced->print();
		rv = false;
		return NULL;
	    }
	    if (!(existingCED->callee->equals(ced->callee))) continue;
	    if (existingCED->callArgExprs.size() >= ced->callArgExprs.size())
		continue;
	    for (unsigned i = 0 ; i < existingCED->callArgExprs.size(); i++) {
		if (!existingCED->callArgExprs[i] || !ced->callArgExprs[i]) {
		    llvm::outs() << "ERROR: NULL ArgExpr in InputStmt\n";
		    if (!existingCED->callArgExprs[i]) existingCED->print();
		    if (!ced->callArgExprs[i]) ced->print();
		    rv = false;
		    return NULL;
		}

		if (!(existingCED->callArgExprs[i]->equals(ced->callArgExprs[i]))) {
		    llvm::outs() << "ERROR: Conflicting ArgExpr in InputStmt\n";
		    existingCED->print();
		    ced->print();
		    rv = false;
		    return NULL;
		}
	    }
	    return existingCED;
	}
	return NULL;
    }

    bool isOutputStmtRecorded(StmtDetails* s) {
	for (std::vector<StmtDetails*>::iterator sIt = outputStmts.begin();
		sIt != outputStmts.end(); sIt++) {
	    if (s->equals(*sIt)) return true;
	}

	return false;
    }

    StmtDetails* getSimilarOutputStmt(StmtDetails* s, bool &rv) {
	rv = true;
	if (!isa<CallExprDetails>(s)) return NULL;
	CallExprDetails* ced = dyn_cast<CallExprDetails>(s);
	for (std::vector<StmtDetails*>::iterator sIt = outputStmts.begin();
		sIt != outputStmts.end(); sIt++) {
	    if (!((*sIt)->StmtDetails::equals(s))) continue;
	    if (!isa<CallExprDetails>(*sIt)) continue;
	    CallExprDetails* existingCED = dyn_cast<CallExprDetails>(*sIt);
	    if (!existingCED->callee || !ced->callee) {
		llvm::outs() << "ERROR: NULL Callee in OutputStmt\n";
		if (!existingCED->callee) existingCED->print();
		if (!ced->callee) ced->print();
		rv = false;
		return NULL;
	    }
	    if (!(existingCED->callee->equals(ced->callee))) continue;
	    if (existingCED->callArgExprs.size() >= ced->callArgExprs.size())
		continue;
	    for (unsigned i = 0 ; i < existingCED->callArgExprs.size(); i++) {
		if (!existingCED->callArgExprs[i] || !ced->callArgExprs[i]) {
		    llvm::outs() << "ERROR: NULL ArgExpr in OutputStmt\n";
		    if (!existingCED->callArgExprs[i]) existingCED->print();
		    if (!ced->callArgExprs[i]) ced->print();
		    rv = false;
		    return NULL;
		}

		if (!(existingCED->callArgExprs[i]->equals(ced->callArgExprs[i]))) {
		    llvm::outs() << "ERROR: Conflicting ArgExpr in OutputStmt\n";
		    existingCED->print();
		    ced->print();
		    rv = false;
		    return NULL;
		}
	    }
	    return existingCED;
	}
	return NULL;
    }

    bool VisitCallExpr(CallExpr* E);
    bool VisitBinaryOperator(BinaryOperator* B);
    bool VisitDeclStmt(DeclStmt* D);
};

class InitUpdateStmtVisitor: public RecursiveASTVisitor<InitUpdateStmtVisitor> {
public:
    const SourceManager* SM;
    bool error;
    std::vector<std::string> customInputFuncs;
    std::vector<std::string> customOutputFuncs;

    std::vector<int> initLinesFound;
    std::vector<int> updateLinesFound;

    std::vector<StmtDetails*> initStmts;
    std::vector<StmtDetails*> updateStmts;

    // data structures to record ordering of stmts
    std::vector<std::pair<CallExprDetails*, std::vector<StmtDetails*> > >
    callExprToInitStmts;
    std::vector<std::pair<CallExprDetails*, std::vector<StmtDetails*> > >
    callExprToUpdateStmts;

    MainFunction* mObj;
    GetSymbolicExprVisitor* symVisitor;
    FunctionDetails* currFunction;
    unsigned currBlock;

    InitUpdateStmtVisitor(const SourceManager* M, MainFunction* main,
    GetSymbolicExprVisitor* v) {
	error = false;
	SM = M;
	mObj = main;
	symVisitor = v;
    }

    InitUpdateStmtVisitor(const SourceManager* M, MainFunction* main,
    GetSymbolicExprVisitor* v, std::vector<std::string> ifuncs, 
    std::vector<std::string> ofuncs) {
	error = false;
	SM = M;
	mObj = main;
	symVisitor = v;
	customInputFuncs.insert(customInputFuncs.end(), ifuncs.begin(),
	ifuncs.end());
	customOutputFuncs.insert(customOutputFuncs.end(), ofuncs.begin(),
	ofuncs.end());
    }

    InitUpdateStmtVisitor(InitUpdateStmtVisitor* v, GetSymbolicExprVisitor*
    visitor) {
	error = false;
	SM = v->SM;
	mObj = v->mObj;
	symVisitor = visitor;
	customInputFuncs.insert(customInputFuncs.end(),
	    v->customInputFuncs.begin(), v->customInputFuncs.end());
	customOutputFuncs.insert(customOutputFuncs.end(), 
	    v->customOutputFuncs.begin(), v->customOutputFuncs.end());
    }

    void setCurrFunction(FunctionDetails* f) {
	currFunction = f;
    }

    void setCurrBlock(unsigned blockID) {
	currBlock = blockID;
    }
    
    bool VisitCallExpr(CallExpr* C);
    bool VisitBinaryOperator(BinaryOperator* B);
    bool VisitVarDecl(VarDecl* VD);
};

class ReturnStmtVisitor: public RecursiveASTVisitor<ReturnStmtVisitor> {
public:
    bool error;
    const SourceManager* SM;
    unsigned currBlock;
    std::vector<ReturnStmtDetails*> returnStmtsFound;

    ReturnStmtVisitor(const SourceManager* M) {
	error = false;
	SM = M;
	currBlock = -1;
    }

    void setCurrBlock(unsigned b) {
	currBlock = b;
    }

    bool VisitReturnStmt(ReturnStmt* R);
};

class BlockStmtVisitor: public RecursiveASTVisitor<BlockStmtVisitor> {
public:
    bool error;
    const SourceManager* SM;
    
    std::vector<std::string> customInputFuncs;
    std::vector<std::string> customOutputFuncs;
    std::vector<StmtDetails*> inputStmts, initStmts, updateStmts, outputStmts;

    std::vector<BlockDetails*> blocksIdentified;
    std::vector<std::pair<CallExprDetails*, std::vector<BlockDetails*> > >
    callExprToBlocksIdentified;

    std::vector<CallExprDetails*> callsVisitedAlready;
    std::vector<StmtDetails*> residualStmts;

    MainFunction* mObj;
    GetSymbolicExprVisitor* symVisitor;
    FunctionDetails* currFunction;
    unsigned currBlock;

    BlockStmtVisitor(const SourceManager* M, MainFunction* main,
    GetSymbolicExprVisitor* v) {
	error = false;
	SM = M;
	mObj = main;
	symVisitor = v;
    }

    BlockStmtVisitor(const SourceManager* M, MainFunction* main,
    GetSymbolicExprVisitor* v, std::vector<std::string> ifuncs, 
    std::vector<std::string> ofuncs, std::vector<StmtDetails*> inpStmts, 
    std::vector<StmtDetails*> inStmts, std::vector<StmtDetails*> updtStmts,
    std::vector<StmtDetails*> outStmts) {
	error = false;
	SM = M;
	mObj = main;
	symVisitor = v;
	customInputFuncs.insert(customInputFuncs.end(), ifuncs.begin(),
	ifuncs.end());
	customOutputFuncs.insert(customOutputFuncs.end(), ofuncs.begin(),
	ofuncs.end());
	inputStmts.insert(inputStmts.end(), inpStmts.begin(), inpStmts.end());
	initStmts.insert(initStmts.end(), inStmts.begin(), inStmts.end());
	updateStmts.insert(updateStmts.end(), updtStmts.begin(), updtStmts.end());
	outputStmts.insert(outputStmts.end(), outStmts.begin(), outStmts.end());
    }

    BlockStmtVisitor(BlockStmtVisitor* v, GetSymbolicExprVisitor*
    visitor) {
	error = false;
	SM = v->SM;
	mObj = v->mObj;
	symVisitor = visitor;
	customInputFuncs.insert(customInputFuncs.end(),
	    v->customInputFuncs.begin(), v->customInputFuncs.end());
	customOutputFuncs.insert(customOutputFuncs.end(), 
	    v->customOutputFuncs.begin(), v->customOutputFuncs.end());
	inputStmts.insert(inputStmts.end(), v->inputStmts.begin(), 
	    v->inputStmts.end());
	initStmts.insert(initStmts.end(), v->initStmts.begin(),
	    v->initStmts.end());
	updateStmts.insert(updateStmts.end(), v->updateStmts.begin(),
	    v->updateStmts.end());
	outputStmts.insert(outputStmts.end(), v->outputStmts.begin(),
	    v->outputStmts.end());
    }

    void setCurrFunction(FunctionDetails* f) {
	currFunction = f;
    }

    void setCurrBlock(unsigned blockID) {
	currBlock = blockID;
    }
    
    bool VisitCallExpr(CallExpr* C);
};

class DeclStmtVisitor: public RecursiveASTVisitor<DeclStmtVisitor> {
public:
    bool error;
    const SourceManager* SM;
    
    std::vector<std::string> customInputFuncs;
    std::vector<std::string> customOutputFuncs;

    std::vector<CallExprDetails*> callsVisitedAlready;
    std::vector<InputVar> inputVarsUpdated;
    std::vector<DPVar> dpVarsUpdated;

    MainFunction* mObj;
    GetSymbolicExprVisitor* symVisitor;
    FunctionDetails* currFunction;
    unsigned currBlock;

    DeclStmtVisitor(const SourceManager* M, MainFunction* main,
    GetSymbolicExprVisitor* v) {
	error = false;
	SM = M;
	mObj = main;
	symVisitor = v;
    }

    DeclStmtVisitor(const SourceManager* M, MainFunction* main,
    GetSymbolicExprVisitor* v, std::vector<std::string> ifuncs, 
    std::vector<std::string> ofuncs) {
	error = false;
	SM = M;
	mObj = main;
	symVisitor = v;
	customInputFuncs.insert(customInputFuncs.end(), ifuncs.begin(),
	ifuncs.end());
	customOutputFuncs.insert(customOutputFuncs.end(), ofuncs.begin(),
	ofuncs.end());
    }

    DeclStmtVisitor(DeclStmtVisitor* v, GetSymbolicExprVisitor*
    visitor) {
	error = false;
	SM = v->SM;
	mObj = v->mObj;
	symVisitor = visitor;
	customInputFuncs.insert(customInputFuncs.end(),
	    v->customInputFuncs.begin(), v->customInputFuncs.end());
	customOutputFuncs.insert(customOutputFuncs.end(), 
	    v->customOutputFuncs.begin(), v->customOutputFuncs.end());
    }

    void setCurrFunction(FunctionDetails* f) {
	currFunction = f;
    }

    void setCurrBlock(unsigned blockID) {
	currBlock = blockID;
    }
    
    bool VisitCallExpr(CallExpr* C);
    bool VisitVarDecl(VarDecl* VD);
};

class SubExprVisitor: public RecursiveASTVisitor<SubExprVisitor> {
    FunctionAnalysis* analysisObj;
public:
    bool error;

    std::vector<std::pair<unsigned, unsigned> > subExprs;

    SubExprVisitor(FunctionAnalysis* FA) {
	error = false;
	analysisObj = FA;
    }

    bool VisitIfStmt(IfStmt* I);
    //bool VisitStmt(Stmt* S);
    bool VisitForStmt(ForStmt* F);
    bool VisitWhileStmt(WhileStmt* W);
    bool VisitDoStmt(DoStmt* D);
    bool VisitBinaryOperator(BinaryOperator* B);
    bool VisitExpr(Expr* E);
};

class ReadArrayAsScalarVisitor: public
RecursiveASTVisitor<ReadArrayAsScalarVisitor> {
public:
    bool error;
    const SourceManager* SM;
    std::vector<std::string> customInputFuncs;
    std::vector<std::string> customOutputFuncs;
    MainFunction* mObj;
    GetSymbolicExprVisitor* symVisitor;
    FunctionDetails* currFunction;
    FunctionAnalysis* currAnalysisObj;
    unsigned currBlock;

    std::map<int, std::pair<InputVar, StmtDetails*> > inputLineToAssignmentLine;

    ReadArrayAsScalarVisitor(const SourceManager* M, MainFunction* main,
    GetSymbolicExprVisitor* v) {
	error = false;
	SM = M;
	mObj = main;
	symVisitor = v;
	currAnalysisObj = NULL;
    }

    ReadArrayAsScalarVisitor(const SourceManager* M, MainFunction* main,
    GetSymbolicExprVisitor* v, std::vector<std::string> ifuncs, 
    std::vector<std::string> ofuncs) {
	error = false;
	SM = M;
	mObj = main;
	symVisitor = v;
	customInputFuncs.insert(customInputFuncs.end(), ifuncs.begin(),
	ifuncs.end());
	customOutputFuncs.insert(customOutputFuncs.end(), ofuncs.begin(),
	ofuncs.end());
	currAnalysisObj = NULL;
    }

    ReadArrayAsScalarVisitor(ReadArrayAsScalarVisitor* v, GetSymbolicExprVisitor*
    visitor) {
	error = false;
	SM = v->SM;
	mObj = v->mObj;
	symVisitor = visitor;
	customInputFuncs.insert(customInputFuncs.end(),
	    v->customInputFuncs.begin(), v->customInputFuncs.end());
	customOutputFuncs.insert(customOutputFuncs.end(), 
	    v->customOutputFuncs.begin(), v->customOutputFuncs.end());
	currAnalysisObj = NULL;
    }

    void setCurrFunction(FunctionDetails* f) {
	currFunction = f;

	for (std::vector<FunctionAnalysis*>::iterator aIt =
		mObj->analysisInfo.begin(); aIt != mObj->analysisInfo.end();
		aIt++) {
	    if ((*aIt)->function->equals(currFunction))
		currAnalysisObj = *aIt;
	}
    }

    void setCurrBlock(unsigned blockID) {
	currBlock = blockID;
    }
    
    bool VisitBinaryOperator(BinaryOperator* B);
    bool VisitCallExpr(CallExpr* C);
};

class BreakStmtVisitor : public RecursiveASTVisitor<BreakStmtVisitor> {
public:
    const SourceManager* SM;
    bool error;
    bool breakStmtFound;

    BreakStmtVisitor() {
	error = true;
	SM = NULL;
	breakStmtFound = false;
    }

    BreakStmtVisitor(const SourceManager* M) {
	error = false;
	SM = M;
	breakStmtFound = false;
    }

    bool VisitBreakStmt(BreakStmt* B);
    bool VisitReturnStmt(ReturnStmt* R);
};

class GlobalVarSymExprVisitor: public
RecursiveASTVisitor<GlobalVarSymExprVisitor> {
public:
    bool error;
    const SourceManager* SM;
    std::vector<std::string> customInputFuncs;
    std::vector<std::string> customOutputFuncs;
    MainFunction* mObj;
    GetSymbolicExprVisitor* symVisitor;
    FunctionDetails* currFunction;
    FunctionAnalysis* currAnalysisObj;
    unsigned currBlock;

    // These are the symExprs of global vars at the entry of the function
    std::vector<std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> > > 
    globalVarSymExprs;
    // These are the symExprs of global vars at the exit of the function
    std::vector<std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> > >
    globalVarSymExprsAtExit;

    GlobalVarSymExprVisitor(const SourceManager* M, MainFunction* main,
    GetSymbolicExprVisitor* v) {
	error = false;
	SM = M;
	mObj = main;
	symVisitor = v;
	currAnalysisObj = NULL;
    }

    GlobalVarSymExprVisitor(const SourceManager* M, MainFunction* main,
    GetSymbolicExprVisitor* v, std::vector<std::string> ifuncs, 
    std::vector<std::string> ofuncs) {
	error = false;
	SM = M;
	mObj = main;
	symVisitor = v;
	customInputFuncs.insert(customInputFuncs.end(), ifuncs.begin(),
	ifuncs.end());
	customOutputFuncs.insert(customOutputFuncs.end(), ofuncs.begin(),
	ofuncs.end());
	currAnalysisObj = NULL;
    }

    GlobalVarSymExprVisitor(GlobalVarSymExprVisitor* v, 
    GetSymbolicExprVisitor* visitor, FunctionAnalysis* FA) {
	error = false;
	SM = v->SM;
	mObj = v->mObj;
	symVisitor = visitor;
	currAnalysisObj = FA;
	customInputFuncs.insert(customInputFuncs.end(),
	    v->customInputFuncs.begin(), v->customInputFuncs.end());
	customOutputFuncs.insert(customOutputFuncs.end(), 
	    v->customOutputFuncs.begin(), v->customOutputFuncs.end());
    }

    void setCurrFunction(FunctionDetails* f) {
	currFunction = f;

	for (std::vector<FunctionAnalysis*>::iterator aIt =
		mObj->analysisInfo.begin(); aIt != mObj->analysisInfo.end();
		aIt++) {
	    if ((*aIt)->function->equals(currFunction))
		currAnalysisObj = *aIt;
	}
    }

    void setCurrBlock(unsigned blockID) {
	currBlock = blockID;
    }

    void setSymExprsForGlobalVars(
    std::vector<std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> > > gv) {
	//globalVarSymExprs.insert(globalVarSymExprs.end(), gv.begin(), gv.end());
	for (std::vector<std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> >
		>::iterator gvIt = gv.begin(); gvIt != gv.end(); gvIt++) {
	    if (gvIt->first->var.isArray()) continue;
	    globalVarSymExprs.push_back(*gvIt);
	}
    }

    void setSymExprsForGlobalVars(
    std::vector<std::pair<VarDeclDetails*, SymbolicExpr*> > gv) {
	for (std::vector<std::pair<VarDeclDetails*, SymbolicExpr*> >::iterator
		gvIt = gv.begin(); gvIt != gv.end(); gvIt++) {
	    if (gvIt->first->var.isArray()) continue;
	    std::vector<SymbolicStmt*> svec;
	    if (gvIt->second)
		svec.push_back(gvIt->second);
	    std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> > p;
	    p.first = gvIt->first;
	    p.second = svec;
	    globalVarSymExprs.push_back(p);
	}
    }

    void setSymExprsForGlobalVarsAtExit(
    std::vector<std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> > > gv) {
	for (std::vector<std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> >
		>::iterator gvIt = gv.begin(); gvIt != gv.end(); gvIt++) {
	    if (gvIt->first->var.isArray()) continue;
	    globalVarSymExprsAtExit.push_back(*gvIt);
	}
    }

    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
    getGuardExprsOfCurrentBlock(bool &rv);

    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
    getGuardExprsOfBlock(unsigned block, bool &rv);

    std::vector<SymbolicStmt*> getSymExprsOfVarInCurrentBlock(VarDetails gv, 
    int lineNum, std::pair<unsigned, unsigned> offset, bool &rv);

    std::vector<SymbolicStmt*> replaceGlobalVarsWithSymExprs(
    std::vector<std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> > > gvSymExprs, 
    SymbolicStmt* baseStmt, bool &rv);
    
    bool VisitCallExpr(CallExpr* C);
};
#endif /* MYASTVISITOR_H */
