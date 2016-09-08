#include "RecursionClass.h"

// Declaring binding names
const char RecursionCallee[] = "RecursionCallee";
const char RecursionCallExpr[] = "RecursionCallExpr";
const char RecursionCaller[] = "RecursionCaller";
const char MemberRecursionCallee[] = "MemberRecursionCallee";
const char MemberRecursionCallExpr[] = "MemberRecursionCallExpr";
const char MemberRecursionCaller[] = "MemberRecursionCaller";
const char StructMatched[] = "StructMatched";
const char TemplateFuncMatched[] = "TemplateFuncMatched";
const char ArrayFound[] = "ArrayFound";
const char GotoFound[] = "GotoFound";

// Matching function containing function call expr
DeclarationMatcher RecursionMatcher = functionDecl(
    forEachDescendant(
    // Function call expr
    callExpr(hasDescendant(
	// Callee Expression
    	declRefExpr().bind(RecursionCallee)))
    .bind(RecursionCallExpr))).bind(RecursionCaller);

DeclarationMatcher recMatcher = functionDecl(
    forEachDescendant(callExpr().bind(RecursionCallExpr))).bind(RecursionCaller);

DeclarationMatcher MemberRecursionMatcher = cxxMethodDecl(  // methodDecl in clang-3.6
    forEachDescendant(
    cxxMemberCallExpr(hasDescendant(	// memberCallExpr in clang-3.6
	memberExpr().bind(MemberRecursionCallee)))
    .bind(MemberRecursionCallExpr))).bind(MemberRecursionCaller);

DeclarationMatcher RRecordDeclMatcher = recordDecl().bind(StructMatched);

DeclarationMatcher TemplateFuncMatcher =
    functionTemplateDecl().bind(TemplateFuncMatched);

StatementMatcher ArrayMatcher =
    arraySubscriptExpr().bind(ArrayFound);

StatementMatcher GotoMatcher = gotoStmt().bind(GotoFound);

void RecursionClass::run(const MatchFinder::MatchResult &Result) {
    if (recursionFound || classOrStructFound || templateFuncFound || gotoFound)
	return;

    SourceManager &SM = Result.Context->getSourceManager();

    const GotoStmt* gs = Result.Nodes.getNodeAs<GotoStmt>(GotoFound);
    if (gs) {
	SourceLocation SL = SM.getExpansionLoc(gs->getLocStart());
	if (SM.isInSystemHeader(SL) || !SM.isWrittenInMainFile(SL))
	    return;
	gotoFound = true;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found goto statement\n";
#endif
	return;
    }

    const ArraySubscriptExpr *ase =
	Result.Nodes.getNodeAs<ArraySubscriptExpr>(ArrayFound);
    if (ase) {
	SourceLocation SL = SM.getExpansionLoc(ase->getLocStart());
	if (SM.isInSystemHeader(SL) || !SM.isWrittenInMainFile(SL))
	    return;
	arrayFound = true;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found array\n";
#endif
    }

    const FunctionTemplateDecl *temp =
	Result.Nodes.getNodeAs<FunctionTemplateDecl>(TemplateFuncMatched);
    if (temp) {
	SourceLocation SL = SM.getExpansionLoc(temp->getLocation());
	if (SM.isInSystemHeader(SL) || !SM.isWrittenInMainFile(SL))
	    return;
	templateFuncFound = true;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found template func\n";
#endif
    }

    const CXXRecordDecl *record1 = Result.Nodes.getNodeAs<CXXRecordDecl>(StructMatched);
    if (record1) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Some record decl\n";
#endif
	SourceLocation SL = SM.getExpansionLoc(record1->getLocation());
	if (SM.isInSystemHeader(SL) || !SM.isWrittenInMainFile(SL))
	    return;
	classOrStructFound = true;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found class or struct\n";
#endif
    }
    const RecordDecl *record2 = Result.Nodes.getNodeAs<RecordDecl>(StructMatched);
    if (record2) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Some record decl\n";
#endif
	SourceLocation SL = SM.getExpansionLoc(record2->getLocStart());
	if (SM.isInSystemHeader(SL) || !SM.isWrittenInMainFile(SL))
	    return;
	classOrStructFound = true;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found class or struct\n";
#endif
    }

    const DeclRefExpr *Callee =
	Result.Nodes.getNodeAs<clang::DeclRefExpr>(RecursionCallee);
    const CallExpr *callExpr = 
	Result.Nodes.getNodeAs<clang::CallExpr>(RecursionCallExpr);
    const FunctionDecl *F =
	Result.Nodes.getNodeAs<clang::FunctionDecl>(RecursionCaller);
    if (Callee && F) {
	std::string callerName = F->getNameInfo().getAsString();
	const FunctionDecl *FD =
	    dyn_cast<FunctionDecl>(Callee->getDecl());
	if (FD) {
	    std::string calleeName = FD->getNameInfo().getAsString();
	    if (calleeName.compare(callerName) == 0) {
		SourceLocation SL = SM.getExpansionLoc(F->getLocation());
		if (SM.isInSystemHeader(SL) || !SM.isWrittenInMainFile(SL))
		    return;
		recursionFound = true;
#ifdef DEBUG
		llvm::outs() << "DEBUG: Found Recursion\n";
#endif
	    }
	}
    }

    if (callExpr && F) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Potential Recursion?\n";
#endif
	std::string callerName = F->getNameInfo().getAsString();
	if (!callExpr->getDirectCallee()) return;
	std::string calleeName = callExpr->getDirectCallee()->getNameInfo().getAsString();
#ifdef DEBUG
	llvm::outs() << "DEBUG: caller: " << callerName << " callee: " 
		     << calleeName << "\n";
#endif
	if (callerName.compare(calleeName) == 0) {
	    SourceLocation SL = SM.getExpansionLoc(F->getLocation());
	    if (SM.isInSystemHeader(SL) || !SM.isWrittenInMainFile(SL))
		return;
	    recursionFound = true;
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Found Recursion\n";
#endif
	}
    }

    const MemberExpr *memberCallee =
	Result.Nodes.getNodeAs<clang::MemberExpr>(MemberRecursionCallee);
    const CXXMethodDecl* memberF =
	Result.Nodes.getNodeAs<clang::CXXMethodDecl>(MemberRecursionCaller);
    if (memberCallee && memberF) {
	//std::string callerName = memberF->getNameInfo().getAsString();
	std::string callerName = memberF->getNameAsString();
	const CXXMethodDecl *memberFD =
	    dyn_cast<CXXMethodDecl>(memberCallee->getMemberDecl());
	if (memberFD) {
	    //std::string calleeName = memberFD->getNameInfo().getAsString();
	    std::string calleeName = memberFD->getNameAsString();
	    if (calleeName.compare(callerName) == 0) {
		SourceLocation SL = SM.getExpansionLoc(memberF->getLocation());
		if (SM.isInSystemHeader(SL) || !SM.isWrittenInMainFile(SL))
		    return;
		recursionFound = true;
#ifdef DEBUG
		llvm::outs() << "DEBUG: Found Recursion\n";
#endif
	    }
	}
    }
}

void RecursionClass::runRecursionMatcher(ClangTool &Tool) {
    MatchFinder RecursionFinder;
    RecursionFinder.addMatcher(RecursionMatcher, this);
    RecursionFinder.addMatcher(recMatcher, this);
    RecursionFinder.addMatcher(MemberRecursionMatcher, this);
    RecursionFinder.addMatcher(RRecordDeclMatcher, this);
    RecursionFinder.addMatcher(TemplateFuncMatcher, this);
    RecursionFinder.addMatcher(ArrayMatcher, this);
    RecursionFinder.addMatcher(GotoMatcher, this);
    Tool.run(newFrontendActionFactory(&RecursionFinder).get());
}
