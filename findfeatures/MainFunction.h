#ifndef MAINFUNCTION_H
#define MAINFUNCTION_H

#include "includes.h"
#include "Helper.h"
#include "clang/Analysis/CFG.h"
#include "clang/Analysis/CallGraph.h"
#include "FunctionAnalysis.h"
#include "StmtDetails.h"
#include "BlockDetails.h"

class FunctionAnalysis;
class InputOutputStmtVisitor;
class ReadArrayAsScalarVisitor;
class InitUpdateStmtVisitor;
class GlobalVarSymExprVisitor;
class BlockStmtVisitor;
class DeclStmtVisitor;

class MainFunction : public MatchFinder::MatchCallback {
private:
    FunctionDetails* main;
    std::string progID;
    enum problemName {MARCHA1, MGCRNK, PPTEST, SUMTRIAN};
    problemName problem;

    void findTestCaseVar(bool &rv);
    FunctionDetails* getFunctionDetailsFromCallGraphNode(CallGraphNode* n, bool &rv);
    void topologicalSort(CallGraphNode* currNode, 
    std::vector<FunctionDetails*> &visitedFunctions, 
    std::stack<FunctionDetails*> &stack, bool &rv);
public:

    bool error; // true if we encountered any error.
    InputVar testCaseVar;
    const SourceManager *SM;
    std::vector<FunctionDetails*> functions;
    std::vector<FunctionAnalysis*> analysisInfo;
    std::vector<InputVar> inputs;
    
    std::vector<std::string> customInputFuncs;
    std::vector<std::string> customOutputFuncs;
    std::vector<VarDeclDetails*> globalVars;
    std::vector<std::pair<VarDeclDetails*, SymbolicExpr*> >
    globalVarInitialExprs;

    std::vector<StmtDetails*> inputStmts;
    std::vector<StmtDetails*> initStmts;
    std::vector<StmtDetails*> updateStmts;
    std::vector<StmtDetails*> outStmts;
    std::vector<StmtDetails*> residualStmts;

    std::vector<BlockDetails*> originalSummaryBlocks;
    std::vector<BlockDetails*> blocksOutsideTestCaseLoop;
    std::vector<BlockDetails*> summaryBlocksWithoutTestCaseVar;
    std::vector<BlockDetails*> summaryBlocksWithoutTestCaseLoop;
    std::vector<BlockDetails*> combinedBlocks;
    std::vector<BlockDetails*> homogeneousBlocks;
    std::vector<BlockDetails*> finalBlocks;
    std::vector<BlockDetails*> blocksAfterUnifyingInputs;
    std::vector<BlockDetails*> residualBlocks;
    std::vector<BlockStmt*> residualInits;

    CallGraph cg;

    std::vector<DPVar> allDPVars;

    std::vector<std::pair<SymbolicSpecialExpr*, std::string> >
    z3varsForSpecialExprs;

    void writeInput(std::ofstream &logFile, bool &rv);
    void writeInit(std::ofstream &logFile, bool &rv);
    void writeUpdate(std::ofstream &logFile, bool &rv);
    void writeOutput(std::ofstream &logFile, bool &rv);
    void writeResidual(std::ofstream &logFile, bool &rv);
    void writeTypeMap(std::ofstream &logFile, bool &rv);
    void writeClusteringFeatures(bool &rv);
    void writeOtherInfo(std::ofstream &logFile, bool &rv);

    void logSummary(bool &rv);

    MainFunction() {
	error = true;
    }

    void setProblem(std::string p) {
	if (p.compare("MARCHA1") == 0)
	    problem = problemName::MARCHA1;
	else if (p.compare("MGCRNK") == 0)
	    problem = problemName::MGCRNK;
	else if (p.compare("PPTEST") == 0)
	    problem = problemName::PPTEST;
	else if (p.compare("SUMTRIAN") == 0)
	    problem = problemName::SUMTRIAN;
    }

    void setProgID(std::string id) {
	progID = id;
    }

    void setGlobalVars(std::vector<VarDeclDetails*> gv) {
	globalVars.insert(globalVars.end(), gv.begin(), gv.end());
    }

    void addCustomInputFunc(std::string fName) {
	if (std::find(customInputFuncs.begin(), customInputFuncs.end(), fName) 
		== customInputFuncs.end())
	    customInputFuncs.push_back(fName);
	else
	    llvm::outs() << "WARNING: Custom input function " << fName 
			 << " already recorded\n";
    }

    void addCustomOutputFunc(std::string fName) {
	if (std::find(customOutputFuncs.begin(), customOutputFuncs.end(), fName)
		== customOutputFuncs.end())
	    customOutputFuncs.push_back(fName);
	else
	    llvm::outs() << "WARNING: Custom output function " << fName
			 << " already recorded\n";
    }

    bool isCustomInputOutputFunction(std::string fName) {
	if (std::find(customInputFuncs.begin(), customInputFuncs.end(),fName)
		== customInputFuncs.end())
	    if (std::find(customOutputFuncs.begin(), customOutputFuncs.end(),fName)
		    == customOutputFuncs.end())
		return false;
	    else
		return true;
	else
	    return true;
    }

    bool isCustomInputFunction(std::string fName) {
	if (std::find(customInputFuncs.begin(), customInputFuncs.end(),fName)
		== customInputFuncs.end())
	    return false;
	else
	    return true;
    }

    bool isCustomOutputFunction(std::string fName) {
	if (std::find(customOutputFuncs.begin(), customOutputFuncs.end(),fName)
		== customOutputFuncs.end())
	    return false;
	else
	    return true;
    }

    FunctionAnalysis* getAnalysisObjOfFunction(FunctionDetails* f, 
    bool &rv);
    void getInputOutputStmts(bool &rv);
    void getInitUpdateStmts(bool &rv);
    void getBlocks(bool &rv);
    void removeInputBlockForTestCaseVar(bool &rv);
    void removeTestCaseLoop(bool &rv);
    void combineBlocks(bool &rv);
    //void sequenceInputs(bool &rv);
    FunctionDetails* getDetailsOfFunction(std::string fName, int
    startLine, int endLine, bool &rv);
    void getDPVars(bool &rv);
    void getSizeExprsOfSummaryVars(bool &rv);
    bool isDPVarRecorded(VarDetails v);
    void removeInitToInputVars(bool &rv);
    void getSymExprsForInitialExprsOfGlobalVars(bool &rv);
    void checkIfArraysAreReadAsScalars(bool &rv);
    void checkIfInputArraysAreReadMultipleTimes(bool &rv);
    void removeInputsAfterOutput(bool &rv);
    void makeDisjointGuards(BlockDetails* block, bool &rv);
    z3::expr getProblemInputConstraints(z3::context* c, bool &rv);
    //void relabelInitAsUpdate(bool &rv);

    static BlockDetails* getBlockDetailsOfStmt(StmtDetails* st, BlockStmtKind label,
    GetSymbolicExprVisitor* visitor, FunctionDetails* func, bool &rv);
    static void runVisitorOnInlinedProgram(std::unique_ptr<CFG> &cfg,
    InputOutputStmtVisitor* ioVisitor, bool &rv);
    static void runVisitorOnInlinedProgram(std::unique_ptr<CFG> &cfg,
    ReadArrayAsScalarVisitor* rasVisitor, bool &rv);
    static void runVisitorOnInlinedProgram(std::unique_ptr<CFG> &cfg,
    InitUpdateStmtVisitor* iuVisitor, bool &rv);
    static void runVisitorOnInlinedProgram(std::unique_ptr<CFG> &cfg, 
    FunctionAnalysis* funcAnalysis, BlockStmtVisitor* bVisitor, bool &rv);
    static void runVisitorOnInlinedProgram(std::unique_ptr<CFG> &cfg, 
    FunctionAnalysis* funcAnalysis, DeclStmtVisitor* bVisitor, bool &rv);
    static void runVisitorOnInlinedProgram(std::unique_ptr<CFG> &cfg,
    GlobalVarSymExprVisitor* gvVisitor, bool &rv);

    virtual void run(const MatchFinder::MatchResult &Result);
    void runMatcher(ClangTool &Tool);
};

#endif /* MAINFUNCTION_H */
