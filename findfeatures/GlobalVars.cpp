#include "GlobalVars.h"

const char globalVar[] = "globalVar";

DeclarationMatcher GVMatcher = 
    varDecl(hasGlobalStorage()).bind(globalVar);

void GlobalVars::run(const MatchFinder::MatchResult &Result) {
    bool rv;
    const SourceManager* SM = Result.SourceManager;

    VarDecl const *globalVD = Result.Nodes.getNodeAs<clang::VarDecl>(globalVar);

    if (!globalVD) {
	llvm::outs() << "ERROR: Cannot get VarDecl of global var\n";
	error = true;
	return;
    }

    SourceLocation SL = SM->getExpansionLoc(globalVD->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL))
	return;

    VarDeclDetails* globalVDD = VarDeclDetails::getObject(SM,
	const_cast<VarDecl*>(globalVD), -1, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get VarDeclDetails of:\n";
	globalVD->print(llvm::outs());
	error = true;
	return;
    }

    bool duplicateFound = false;
    for (std::vector<VarDeclDetails*>::iterator vIt = globalVars.begin(); 
	    vIt != globalVars.end(); vIt++) {
	if (globalVDD->equals(*vIt)) {
	    duplicateFound = true;
	    break;
	}
    }
    if (!duplicateFound) 
	globalVars.push_back(globalVDD);
    return;
}

void GlobalVars::runMatcher(ClangTool &Tool) {
    MatchFinder GlobalFinder;
    GlobalFinder.addMatcher(GVMatcher, this);
    Tool.run(newFrontendActionFactory(&GlobalFinder).get());
}
