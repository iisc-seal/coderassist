// Declaring binding names
const char RecursionCallee[] = "RecursionCallee";
const char RecursionCallExpr[] = "RecursionCallExpr";
const char RecursionCaller[] = "RecursionCaller";

// Matching function containing function call expr
DeclarationMatcher RecursionMatcher = functionDecl(
    forEachDescendant(
    // Function call expr
    callExpr(hasDescendant(
	// Callee Expression
    	declRefExpr().bind(RecursionCallee)))
    .bind(RecursionCallExpr))).bind(RecursionCaller);

// Class to check for recursion
class RecursionClass : public MatchFinder::MatchCallback {
  public :
    bool recursionFound;	// records whether recursion was found

    RecursionClass() {
	recursionFound = false;
    }

    virtual void run(const MatchFinder::MatchResult &Result) {
	if (recursionFound)
	    return;

	const DeclRefExpr *Callee =
	    Result.Nodes.getNodeAs<clang::DeclRefExpr>(RecursionCallee);
	const FunctionDecl *F =
	    Result.Nodes.getNodeAs<clang::FunctionDecl>(RecursionCaller);
	if (Callee && F) {
	    std::string callerName = F->getNameInfo().getAsString();
	    const FunctionDecl *FD =
		dyn_cast<FunctionDecl>(Callee->getDecl());
	    if (FD) {
		std::string calleeName = FD->getNameInfo().getAsString();
		if (calleeName.compare(callerName) == 0) {
		    SourceManager &SM = Result.Context->getSourceManager();
		    SourceLocation SL = F->getLocation();
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

    void runRecursionMatcher(ClangTool &Tool) {
	MatchFinder RecursionFinder;
	RecursionFinder.addMatcher(RecursionMatcher, this);
	Tool.run(newFrontendActionFactory(&RecursionFinder).get());
    }
};
