// Declaring binding names
const char SortingLine1LHSVar[] = "SortingLine1LHSVar";
const char SortingLine1RHSArrayExpr[] = "SortingLine1RHSArrayExpr";
const char SortingLine2LHSArrayExpr[] = "SortingLine2LHSArrayExpr";
const char SortingLine2RHSArrayExpr[] = "SortingLine2RHSArrayExpr";
const char SortingLine3LHSArrayExpr[] = "SortingLine3LHSArrayExpr";
const char SortingLine3RHSVar[] = "SortingLine3RHSVar";
const char SortingLine1ForLoop[] = "SortingLine1ForLoop";
const char SortingLine2ForLoop[] = "SortingLine2ForLoop";
const char SortingLine3ForLoop[] = "SortingLine3ForLoop";

// Matching sorting template - 3 lines for swapping
// Matching for loop with line 1: tempvar = arrayexpr
StatementMatcher SortLine1Matcher_1 = forStmt(hasDescendant(
    compoundStmt(hasDescendant(
    // Assignment statement
    binaryOperator(hasOperatorName("="),
    // LHS with temp var
    hasLHS(declRefExpr(to(varDecl().bind(SortingLine1LHSVar)))),
    // RHS with array expr
    hasRHS(hasDescendant(arraySubscriptExpr(
	hasDescendant(declRefExpr().bind(SortingLine1RHSArrayExpr))))))))))
    .bind(SortingLine1ForLoop);

// Matching for loop with line 1: type tempvar = arrayexpr
StatementMatcher SortLine1Matcher_2 = forStmt(hasDescendant(
    compoundStmt(hasDescendant(
    // Declaration stmt
    declStmt(containsDeclaration(0, 
    // variable declared
    varDecl(hasInitializer(ignoringImpCasts(
	// Initialization of variable with array expr
	arraySubscriptExpr(hasDescendant(
	declRefExpr().bind(SortingLine1RHSArrayExpr))))))
    .bind(SortingLine1LHSVar))))))).bind(SortingLine1ForLoop);

// Matching for loop with line 2: arrayexpr1 = arrayexpr2
StatementMatcher SortLine2Matcher = forStmt(hasDescendant(
    compoundStmt(hasDescendant(
    // Assignment statement
    binaryOperator(hasOperatorName("="),
    // LHS with array expr
    hasLHS(arraySubscriptExpr(hasDescendant(
	declRefExpr().bind(SortingLine2LHSArrayExpr)))),
    // RHS with array expr
    hasRHS(hasDescendant(arraySubscriptExpr(
	hasDescendant(declRefExpr().bind(SortingLine2RHSArrayExpr))))))))))
    .bind(SortingLine2ForLoop);

// Matching for loop with line 3: arrayexpr2 = tempvar
StatementMatcher SortLine3Matcher = forStmt(hasDescendant(
    compoundStmt(hasDescendant(
    // Assignment Statement
    binaryOperator(hasOperatorName("="),
    // LHS with array expr
    hasLHS(arraySubscriptExpr(hasDescendant(
	declRefExpr().bind(SortingLine3LHSArrayExpr)))),
    // RHS with temp var
    hasRHS(ignoringImpCasts(declRefExpr().bind(SortingLine3RHSVar))))))))
    .bind(SortingLine3ForLoop);

class SortingClass : public MatchFinder::MatchCallback {
  public :
    // Class to store details of one statement in the sorting (swap) template
    class swapStmt {
      public:
	int loopStartLine;
	int loopEndLine;
	int swapStmtLine1;
	int swapStmtLine2;
	int swapStmtLine3;
	std::string tempVarName;
	std::string arrayName;
    };

    // Class to store details of all matched swap/sorting statements
    class sortingMatchesList {
      public:
	std::vector<swapStmt> stmt1;
	std::vector<swapStmt> stmt2;
	std::vector<swapStmt> stmt3;
    };

    sortingMatchesList sortStmtMatches;

    virtual void run (const MatchFinder::MatchResult &Result) {
	SourceManager &SM = Result.Context->getSourceManager();
	SourceLocation SL;

	const ForStmt *FS1 =
	    Result.Nodes.getNodeAs<clang::ForStmt>(SortingLine1ForLoop);
	const VarDecl *lhs1;
	const DeclRefExpr *rhs1, *lhs2, *rhs2, *lhs3, *rhs3;

	if (FS1) {
	    const VarDecl *VD;
	    SL = FS1->getLocEnd();
	    // Check if for loop is inside the main file
	    if (SM.isInSystemHeader(SL) || !SM.isWrittenInMainFile(SL))
		return;

	    lhs1 = Result.Nodes.getNodeAs<clang::VarDecl>(SortingLine1LHSVar);
	    rhs1 = Result.Nodes.getNodeAs<clang::DeclRefExpr>
		(SortingLine1RHSArrayExpr);
	    std::string lhsname = lhs1->getQualifiedNameAsString();
	    VD = dyn_cast<VarDecl>(rhs1->getDecl());
	    std::string rhsname = VD->getQualifiedNameAsString();

	    // Populate details of the statement found and push into stmt1
	    // vector
	    swapStmt s;
	    s.loopStartLine = SM.getExpansionLineNumber(FS1->getLocStart(), 0);
	    s.loopEndLine = SM.getExpansionLineNumber(FS1->getLocEnd(), 0);
	    s.swapStmtLine1 = SM.getExpansionLineNumber(rhs1->getLocation(), 0);
	    s.tempVarName = lhsname;
	    s.arrayName = rhsname;
	    sortStmtMatches.stmt1.push_back(s);
	}

	const ForStmt *FS2 =
	    Result.Nodes.getNodeAs<clang::ForStmt>(SortingLine2ForLoop);
	if (FS2) {
	    const VarDecl *VD1;
	    SL = FS2->getLocEnd();
	    // Check if for loop is inside the main file
	    if (SM.isInSystemHeader(SL) || !SM.isWrittenInMainFile(SL))
		return;

	    int loop2Start = SM.getExpansionLineNumber(FS2->getLocStart(), 0);
	    int loop2End = SM.getExpansionLineNumber(FS2->getLocEnd(), 0);
	    lhs2 = Result.Nodes.getNodeAs<clang::DeclRefExpr>
		(SortingLine2LHSArrayExpr);
	    rhs2 = Result.Nodes.getNodeAs<clang::DeclRefExpr>
		(SortingLine2RHSArrayExpr);
	    const VarDecl *VD = dyn_cast<VarDecl>(lhs2->getDecl());
	    std::string lhsname = VD->getQualifiedNameAsString();
	    VD1 = dyn_cast<VarDecl>(rhs2->getDecl());
	    std::string rhsname = VD1->getQualifiedNameAsString();
	    int line = SM.getExpansionLineNumber(rhs2->getLocation(), 0);
	    // If lhs and rhs contains the same array name
	    if (lhsname.compare(rhsname) == 0) {
		std::vector<swapStmt>::iterator it = sortStmtMatches.stmt1.begin();
		for (; it != sortStmtMatches.stmt1.end(); it++) {
		    // If the stmt1 loop is the same as stmt2 and stmt1 lhs
		    // array name is same as stmt2 and the stmt2 line is either
		    // same as stmt1 line or one line after stmt1
		    if ((it->loopStartLine == loop2Start && 
			 it->loopEndLine == loop2End) &&
			(it->arrayName.compare(lhsname) == 0) &&
			(it->swapStmtLine1 == line || it->swapStmtLine1+1 == line))
			break;
		}

	    	// Populate details of the statement found and push into stmt2
	    	// vector
		swapStmt s;
		if (it != sortStmtMatches.stmt1.end()) {
		    s.loopStartLine = loop2Start;
		    s.loopEndLine = loop2End;
		    s.arrayName = lhsname;
		    s.tempVarName = it->tempVarName;
		    s.swapStmtLine1 = it->swapStmtLine1;
		    s.swapStmtLine2 = line;
		    sortStmtMatches.stmt2.push_back(s);
		}
	    }
	}

	const ForStmt *FS3 =
	    Result.Nodes.getNodeAs<clang::ForStmt>(SortingLine3ForLoop);
	if (FS3) {
	    const VarDecl *VD1;
	    SL = FS3->getLocEnd();
	    // Check if for loop is inside the main file
	    if (SM.isInSystemHeader(SL) || !SM.isWrittenInMainFile(SL))
		return;

	    int loop3Start = SM.getExpansionLineNumber(FS3->getLocStart(), 0);
	    int loop3End = SM.getExpansionLineNumber(FS3->getLocEnd(), 0);
	    lhs3 = Result.Nodes.getNodeAs<clang::DeclRefExpr>
		(SortingLine3LHSArrayExpr);
	    rhs3 = Result.Nodes.getNodeAs<clang::DeclRefExpr>
		(SortingLine3RHSVar);
	    const VarDecl *VD = dyn_cast<VarDecl>(lhs3->getDecl());
	    std::string lhsname = VD->getQualifiedNameAsString();
	    int line = SM.getExpansionLineNumber(rhs3->getLocation(), 0);
	    VD1 = dyn_cast<VarDecl>(rhs3->getDecl());
	    std::string rhsname = VD1->getQualifiedNameAsString();

	    std::vector<swapStmt>::iterator it = sortStmtMatches.stmt2.begin();
	    for (; it != sortStmtMatches.stmt2.end(); it++) {
		// If the stmt2 loop is the same as stmt2 and stmt2
		// array name is same as stmt3 and stmt2 temp var name is same
		// as stmt3 and the stmt3 line is either
		// same as stmt2 line or one line after stmt2
		if ((it->loopStartLine == loop3Start && 
		     it->loopEndLine == loop3End) &&
		    (it->arrayName.compare(lhsname) == 0) &&
		    (it->tempVarName.compare(rhsname) == 0) && 
		    (it->swapStmtLine2 == line || it->swapStmtLine2+1 == line))
		    break;
	    }

	    // Populate details of the statement found and push into stmt3
	    // vector
	    swapStmt s;
	    if (it != sortStmtMatches.stmt2.end()) {
		s.loopStartLine = loop3Start;
		s.loopEndLine = loop3End;
		s.arrayName = lhsname;
		s.swapStmtLine1 = it->swapStmtLine1;
		s.swapStmtLine2 = it->swapStmtLine2;
		s.swapStmtLine3 = line;
		s.tempVarName = rhsname;
		sortStmtMatches.stmt3.push_back(s);
#ifdef DEBUG
		llvm::outs() << "DEBUG: Found sorting: " << s.arrayName 
		    << " in loop (" << s.loopStartLine << "-" << s.loopEndLine 
		    << ") stmt3 at (" << s.swapStmtLine1 << ", "
		    << s.swapStmtLine2 << ", " << s.swapStmtLine3 << ")\n";
#endif
	    }
	}
    }

    void runSortingMatcher(ClangTool &Tool) {
	MatchFinder SortingFinder;
	SortingFinder.addMatcher(SortLine1Matcher_1, this);
	SortingFinder.addMatcher(SortLine1Matcher_2, this);
	SortingFinder.addMatcher(SortLine2Matcher, this);
	SortingFinder.addMatcher(SortLine3Matcher, this);
	Tool.run(newFrontendActionFactory(&SortingFinder).get());
    }
};
