#include "SortingClass.h"

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
const char SelectionSortTempVar[] = "SelectionSortTempVar";
const char SelectionSortArrayVar[] = "SelectionSortArrayVar";
const char SelectionSortSubForStmt[] = "SelectionSortSubForStmt";
const char SelectionSortStmt1[] = "SelectionSortStmt1";
const char SelectionSortStmt2[] = "SelectionSortStmt2";
const char SelectionSortStmt3[] = "SelectionSortStmt3";
const char SelectionSortForStmt[] = "SelectionSortForStmt";
const char SwapWithoutMemoryLHSDecl1[] = "SwapWithoutMemoryLHSDecl1";
const char SwapWithoutMemoryLHSArr1[] = "SwapWithoutMemoryLHSArr1";
const char SwapWithoutMemoryLHSDecl2[] = "SwapWithoutMemoryLHSDecl2";
const char SwapWithoutMemoryLHSArr2[] = "SwapWithoutMemoryLHSArr2";
const char SwapWithoutMemoryRHSArr2[] = "SwapWithoutMemoryRHSArr2";
const char SwapWithoutMemoryLHSDecl3[] = "SwapWithoutMemoryLHSDecl3";
const char SwapWithoutMemoryLHSArr3[] = "SwapWithoutMemoryLHSArr3";
const char SwapWithoutMemoryRHSArr3[] = "SwapWithoutMemoryRHSArr3";
const char SwapWithoutMemoryRHSDecl2[] = "SwapWithoutMemoryRHSDecl2";
const char SwapWithoutMemoryRHSDecl3[] = "SwapWithoutMemoryRHSDecl3";
const char SwapWithoutMemoryLine1[] = "SwapWithoutMemoryLine1";
const char SwapWithoutMemoryLine2[] = "SwapWithoutMemoryLine2";
const char SwapWithoutMemoryLine3[] = "SwapWithoutMemoryLine3";
const char SwapWithoutMemoryFS[] = "SwapWithoutMemoryFS";
const char FunctionWithSwap[] = "FunctionWithSwap";

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

StatementMatcher SelectionSortMatcher = forStmt(hasBody(
    compoundStmt(
	hasAnySubstatement(binaryOperator(hasOperatorName("="),
	    hasLHS(declRefExpr(to(varDecl().bind(SelectionSortTempVar)))),
	    hasRHS(ignoringImpCasts(arraySubscriptExpr(
		hasBase(ignoringImpCasts(declRefExpr(to(varDecl()
		.bind(SelectionSortArrayVar)))))))))
	.bind(SelectionSortStmt1)),
	hasAnySubstatement(forStmt().bind(SelectionSortSubForStmt)),
	hasAnySubstatement(binaryOperator(hasOperatorName("="),
	    hasLHS(arraySubscriptExpr(hasBase(ignoringImpCasts(
		declRefExpr(to(varDecl(equalsBoundNode(SelectionSortArrayVar)))))))),
	    hasRHS(ignoringImpCasts(arraySubscriptExpr(hasBase(ignoringImpCasts(
		declRefExpr(to(varDecl(equalsBoundNode(SelectionSortArrayVar))))))))))
	.bind(SelectionSortStmt2)),
	hasAnySubstatement(binaryOperator(hasOperatorName("="),
	    hasLHS(arraySubscriptExpr(hasBase(ignoringImpCasts(
		declRefExpr(to(varDecl(equalsBoundNode(SelectionSortArrayVar)))))))),
	    hasRHS(ignoringImpCasts(
		declRefExpr(to(varDecl(equalsBoundNode(SelectionSortTempVar)))))))
	.bind(SelectionSortStmt3)))))
    .bind(SelectionSortForStmt);

StatementMatcher SwapWithoutMemoryVarLine1Matcher = anyOf(
    binaryOperator(hasOperatorName("="),
	hasLHS(declRefExpr(to(varDecl().bind(SwapWithoutMemoryLHSDecl1)))),
	hasRHS(binaryOperator(hasOperatorName("+"),
	    hasEitherOperand(ignoringImpCasts(
		declRefExpr(to(varDecl(
		    equalsBoundNode(SwapWithoutMemoryLHSDecl1)))))))))
    .bind(SwapWithoutMemoryLine1),
    binaryOperator(hasOperatorName("+="),
	hasLHS(declRefExpr(to(varDecl().bind(SwapWithoutMemoryLHSDecl1)))))
    .bind(SwapWithoutMemoryLine1)
);

StatementMatcher SwapWithoutMemoryArrLine1Matcher = anyOf(
    binaryOperator(hasOperatorName("="),
	hasLHS(arraySubscriptExpr(hasBase(ignoringImpCasts(
	    declRefExpr(to(varDecl().bind(SwapWithoutMemoryLHSArr1))))))),
	hasRHS(binaryOperator(hasOperatorName("+"),
	    hasEitherOperand(ignoringImpCasts(
	    arraySubscriptExpr(hasBase(ignoringImpCasts(
		declRefExpr(to(varDecl(
		    equalsBoundNode(SwapWithoutMemoryLHSArr1))))))))))))
    .bind(SwapWithoutMemoryLine1),
    binaryOperator(hasOperatorName("+="),
	hasLHS(arraySubscriptExpr(hasBase(ignoringImpCasts(
	declRefExpr(to(varDecl().bind(SwapWithoutMemoryLHSArr1))))))))
    .bind(SwapWithoutMemoryLine1)
);

StatementMatcher SwapWithoutMemoryVarLine2Matcher = 
    binaryOperator(hasOperatorName("="),
	hasLHS(declRefExpr(to(varDecl().bind(SwapWithoutMemoryLHSDecl2)))),
	hasRHS(binaryOperator(hasOperatorName("-"),
	    hasLHS(ignoringImpCasts(
		declRefExpr(to(varDecl().bind(SwapWithoutMemoryRHSDecl2))))),
	    hasRHS(ignoringImpCasts(
		declRefExpr(to(varDecl(
		    equalsBoundNode(SwapWithoutMemoryLHSDecl2)))))))))
    .bind(SwapWithoutMemoryLine2);

StatementMatcher SwapWithoutMemoryArrLine2Matcher = 
    binaryOperator(hasOperatorName("="),
	hasLHS(arraySubscriptExpr(hasBase(ignoringImpCasts(
	    declRefExpr(to(varDecl().bind(SwapWithoutMemoryLHSArr2))))))),
	hasRHS(binaryOperator(hasOperatorName("-"),
	    hasLHS(ignoringImpCasts(arraySubscriptExpr(hasBase(ignoringImpCasts(
		declRefExpr(to(varDecl().bind(SwapWithoutMemoryRHSArr2)))))))),
	    hasRHS(ignoringImpCasts(arraySubscriptExpr(hasBase(ignoringImpCasts(
		declRefExpr(to(varDecl(
		    equalsBoundNode(SwapWithoutMemoryLHSArr2))))))))))))
    .bind(SwapWithoutMemoryLine2);

StatementMatcher SwapWithoutMemoryVarArrLine2Matcher = 
    binaryOperator(hasOperatorName("="),
	hasLHS(declRefExpr(to(varDecl().bind(SwapWithoutMemoryLHSDecl2)))),
	hasRHS(binaryOperator(hasOperatorName("-"),
	    hasLHS(ignoringImpCasts(arraySubscriptExpr(hasBase(ignoringImpCasts(
		declRefExpr(to(varDecl().bind(SwapWithoutMemoryRHSArr2)))))))),
	    hasRHS(ignoringImpCasts(arraySubscriptExpr(hasBase(ignoringImpCasts(
		declRefExpr(to(varDecl(
		    equalsBoundNode(SwapWithoutMemoryLHSDecl2))))))))))))
    .bind(SwapWithoutMemoryLine2);

StatementMatcher SwapWithoutMemoryArrVarLine2Matcher = 
    binaryOperator(hasOperatorName("="),
	hasLHS(arraySubscriptExpr(hasBase(ignoringImpCasts(
	    declRefExpr(to(varDecl().bind(SwapWithoutMemoryLHSArr2))))))),
	hasRHS(binaryOperator(hasOperatorName("-"),
	    hasLHS(ignoringImpCasts(
		declRefExpr(to(varDecl().bind(SwapWithoutMemoryRHSArr2))))),
	    hasRHS(ignoringImpCasts(arraySubscriptExpr(hasBase(ignoringImpCasts(
		declRefExpr(to(varDecl(
		    equalsBoundNode(SwapWithoutMemoryLHSArr2))))))))))))
    .bind(SwapWithoutMemoryLine2);

StatementMatcher SwapWithoutMemoryVarLine3Matcher = anyOf(
    binaryOperator(hasOperatorName("="),
	hasLHS(declRefExpr(to(varDecl().bind(SwapWithoutMemoryLHSDecl3)))),
	hasRHS(binaryOperator(hasOperatorName("-"),
	    hasLHS(ignoringImpCasts(
		declRefExpr(to(varDecl(equalsBoundNode(SwapWithoutMemoryLHSDecl3)))))),
	    hasRHS(ignoringImpCasts(
		declRefExpr(to(varDecl().bind(SwapWithoutMemoryRHSDecl3))))))))
    .bind(SwapWithoutMemoryLine3),
    binaryOperator(hasOperatorName("-="),
	hasLHS(declRefExpr(to(varDecl().bind(SwapWithoutMemoryLHSDecl3)))),
	hasRHS(ignoringImpCasts(declRefExpr(to(varDecl().bind(SwapWithoutMemoryRHSDecl3))))))
    .bind(SwapWithoutMemoryLine3));

StatementMatcher SwapWithoutMemoryArrLine3Matcher = anyOf(
    binaryOperator(hasOperatorName("="),
	hasLHS(arraySubscriptExpr(hasBase(ignoringImpCasts(
	    declRefExpr(to(varDecl().bind(SwapWithoutMemoryLHSArr3))))))),
	hasRHS(binaryOperator(hasOperatorName("-"),
	    hasLHS(ignoringImpCasts(arraySubscriptExpr(hasBase(ignoringImpCasts(
		declRefExpr(to(varDecl(equalsBoundNode(SwapWithoutMemoryLHSArr3))))))))),
	    hasRHS(ignoringImpCasts(arraySubscriptExpr(hasBase(ignoringImpCasts(
		declRefExpr(to(varDecl().bind(SwapWithoutMemoryRHSArr3)))))))))))
    .bind(SwapWithoutMemoryLine3),
    binaryOperator(hasOperatorName("-="),
	hasLHS(arraySubscriptExpr(hasBase(ignoringImpCasts(
	    declRefExpr(to(varDecl().bind(SwapWithoutMemoryLHSArr3))))))),
	hasRHS(ignoringImpCasts(arraySubscriptExpr(hasBase(ignoringImpCasts(
	    declRefExpr(to(varDecl().bind(SwapWithoutMemoryRHSArr3)))))))))
    .bind(SwapWithoutMemoryLine3));

StatementMatcher SwapWithoutMemoryVarArrLine3Matcher = anyOf(
    binaryOperator(hasOperatorName("="),
	hasLHS(declRefExpr(to(varDecl().bind(SwapWithoutMemoryLHSDecl3)))),
	hasRHS(binaryOperator(hasOperatorName("-"),
	    hasLHS(ignoringImpCasts(
		declRefExpr(to(varDecl(equalsBoundNode(SwapWithoutMemoryLHSDecl3)))))),
	    hasRHS(ignoringImpCasts(arraySubscriptExpr(hasBase(ignoringImpCasts(
		declRefExpr(to(varDecl().bind(SwapWithoutMemoryRHSArr3)))))))))))
    .bind(SwapWithoutMemoryLine3),
    binaryOperator(hasOperatorName("-="),
	hasLHS(declRefExpr(to(varDecl().bind(SwapWithoutMemoryLHSDecl3)))),
	hasRHS(ignoringImpCasts(arraySubscriptExpr(hasBase(ignoringImpCasts(
	    declRefExpr(to(varDecl().bind(SwapWithoutMemoryRHSArr3)))))))))
    .bind(SwapWithoutMemoryLine3));

StatementMatcher SwapWithoutMemoryArrVarLine3Matcher = anyOf(
    binaryOperator(hasOperatorName("="),
	hasLHS(arraySubscriptExpr(hasBase(ignoringImpCasts(
	    declRefExpr(to(varDecl().bind(SwapWithoutMemoryLHSArr3))))))),
	hasRHS(binaryOperator(hasOperatorName("-"),
	    hasLHS(ignoringImpCasts(arraySubscriptExpr(hasBase(ignoringImpCasts(
		declRefExpr(to(varDecl(equalsBoundNode(SwapWithoutMemoryLHSArr3))))))))),
	    hasRHS(ignoringImpCasts(
		declRefExpr(to(varDecl().bind(SwapWithoutMemoryRHSArr3))))))))
    .bind(SwapWithoutMemoryLine3),
    binaryOperator(hasOperatorName("-="),
	hasLHS(arraySubscriptExpr(hasBase(ignoringImpCasts(
	    declRefExpr(to(varDecl().bind(SwapWithoutMemoryLHSArr3))))))),
	hasRHS(ignoringImpCasts(
	    declRefExpr(to(varDecl().bind(SwapWithoutMemoryRHSArr3))))))
    .bind(SwapWithoutMemoryLine3));

StatementMatcher SwapWithoutMemoryBothVarMatcher = eachOf(
    SwapWithoutMemoryVarLine1Matcher,
    SwapWithoutMemoryVarLine2Matcher,
    SwapWithoutMemoryVarLine3Matcher);

StatementMatcher SwapWithoutMemoryBothArrMatcher = eachOf(
    SwapWithoutMemoryArrLine1Matcher,
    SwapWithoutMemoryArrLine2Matcher,
    SwapWithoutMemoryArrLine3Matcher);

StatementMatcher SwapWithoutMemoryVarArrMatcher = eachOf(
    SwapWithoutMemoryVarLine1Matcher,
    SwapWithoutMemoryVarArrLine2Matcher,
    SwapWithoutMemoryVarArrLine3Matcher);

StatementMatcher SwapWithoutMemoryArrVarMatcher = eachOf(
    SwapWithoutMemoryArrLine1Matcher,
    SwapWithoutMemoryArrVarLine2Matcher,
    SwapWithoutMemoryArrVarLine3Matcher);

StatementMatcher SwapWithoutMemoryMatcher = eachOf(
    SwapWithoutMemoryBothVarMatcher,
    SwapWithoutMemoryBothArrMatcher,
    SwapWithoutMemoryVarArrMatcher,
    SwapWithoutMemoryArrVarMatcher);

DeclarationMatcher FunctionSwapWithoutMemoryMatcher = functionDecl(forEachDescendant(
    forStmt(forEachDescendant(
	SwapWithoutMemoryBothVarMatcher)).bind(SwapWithoutMemoryFS))).bind(FunctionWithSwap);

void SortingClass::run (const MatchFinder::MatchResult &Result) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Starting SortingClass::run()\n";
#endif
    SourceManager &SM = Result.Context->getSourceManager();
    SourceLocation SL;

    const ForStmt *FS1 =
	Result.Nodes.getNodeAs<clang::ForStmt>(SortingLine1ForLoop);
    const VarDecl *lhs1;
    const DeclRefExpr *rhs1, *lhs2, *rhs2, *lhs3, *rhs3;

    if (FS1) {
	const VarDecl *VD;
	SL = SM.getExpansionLoc(FS1->getLocEnd());
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
	SL = SM.getExpansionLoc(FS2->getLocEnd());
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

	    // Populate details of the statement found and push into stmt2 vector
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
	SL = SM.getExpansionLoc(FS3->getLocEnd());
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

    if (!(FS1 && FS2 && FS3)) {
	const ForStmt* SelectionFS = 
	    Result.Nodes.getNodeAs<clang::ForStmt>(SelectionSortForStmt);
	const BinaryOperator* SortStmt1 =
	    Result.Nodes.getNodeAs<clang::BinaryOperator>(SelectionSortStmt1);
	const BinaryOperator* SortStmt2 =
	    Result.Nodes.getNodeAs<clang::BinaryOperator>(SelectionSortStmt2);
	const BinaryOperator* SortStmt3 =
	    Result.Nodes.getNodeAs<clang::BinaryOperator>(SelectionSortStmt3);
	const VarDecl* SortArrayVar =
	    Result.Nodes.getNodeAs<clang::VarDecl>(SelectionSortArrayVar);
	const VarDecl* SortTempVar =
	    Result.Nodes.getNodeAs<clang::VarDecl>(SelectionSortTempVar);
	if (SelectionFS && SortStmt1 && SortStmt2 && SortStmt3 &&
		SortArrayVar && SortTempVar) {

	    SourceLocation SL = SM.getExpansionLoc(SelectionFS->getLocStart());
	    if (SM.isInSystemHeader(SL) || !SM.isWrittenInMainFile(SL))
		return;
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Found selection sort\n";
#endif
	    int loopStart =
		SM.getExpansionLineNumber(SelectionFS->getLocStart(), 0);
	    int loopEnd =
		SM.getExpansionLineNumber(SelectionFS->getLocEnd(), 0);

	    SortingClass::swapStmt s1, s2, s3;
	    s1.loopStartLine = loopStart;
	    s1.loopEndLine = loopEnd;
	    s2.loopStartLine = loopStart;
	    s2.loopEndLine = loopEnd;
	    s3.loopStartLine = loopStart;
	    s3.loopEndLine = loopEnd;

	    int stmt1Line = 
		SM.getExpansionLineNumber(SortStmt1->getLocEnd(), 0);
	    int stmt2Line = 
		SM.getExpansionLineNumber(SortStmt2->getLocEnd(), 0);
	    int stmt3Line = 
		SM.getExpansionLineNumber(SortStmt3->getLocEnd(), 0);

	    s1.swapStmtLine1 = stmt1Line;
	    s2.swapStmtLine1 = stmt1Line;
	    s2.swapStmtLine2 = stmt2Line;
	    s3.swapStmtLine1 = stmt1Line;
	    s3.swapStmtLine2 = stmt2Line;
	    s3.swapStmtLine3 = stmt3Line;

	    std::string arrayName = SortArrayVar->getQualifiedNameAsString();
	    std::string tempName = SortArrayVar->getQualifiedNameAsString();

	    s1.tempVarName = tempName;
	    s1.arrayName = arrayName;
	    s2.tempVarName = tempName;
	    s2.arrayName = arrayName;
	    s3.tempVarName = tempName;
	    s3.arrayName = arrayName;

	    sortStmtMatches.stmt1.push_back(s1);
	    sortStmtMatches.stmt2.push_back(s2);
	    sortStmtMatches.stmt3.push_back(s3);

#ifdef DEBUG
	    llvm::outs() << "DEBUG: Found sorting: " << s3.arrayName 
		<< " in loop (" << s3.loopStartLine << "-" << s3.loopEndLine 
		<< ") stmt3 at (" << s3.swapStmtLine1 << ", "
		<< s3.swapStmtLine2 << ", " << s3.swapStmtLine3 << ")\n";
#endif
	}
    }

    const BinaryOperator* SwapOther1 =
	Result.Nodes.getNodeAs<BinaryOperator>(SwapWithoutMemoryLine1);
    const BinaryOperator* SwapOther2 =
	Result.Nodes.getNodeAs<BinaryOperator>(SwapWithoutMemoryLine2);
    const BinaryOperator* SwapOther3 =
	Result.Nodes.getNodeAs<BinaryOperator>(SwapWithoutMemoryLine3);
    if (SwapOther1) {
	SourceLocation SL = SM.getExpansionLoc(SwapOther1->getLocStart());
	if (SM.isInSystemHeader(SL) || !SM.isWrittenInMainFile(SL))
	    return;
	int swapline = SM.getExpansionLineNumber(SL, 0);
	llvm::outs() << "DEBUG: Found swap without memory line 1 at " 
		     << swapline << "\n";

	swapStmt currStmt1;
	currStmt1.swapStmtLine1 = swapline;

	const VarDecl* LHSDecl1 = 
	    Result.Nodes.getNodeAs<VarDecl>(SwapWithoutMemoryLHSDecl1);
	const VarDecl* LHSArr1 =
	    Result.Nodes.getNodeAs<VarDecl>(SwapWithoutMemoryLHSArr1);
	if (LHSDecl1 && !LHSArr1) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: SwapWithoutMemory: Line1: LHSDecl, not "
			 << "LHSArr\n";
#endif
	    currStmt1.tempVarName = LHSDecl1->getQualifiedNameAsString();
	    currStmt1.tempVarOrArr = true;

	    sortStmtMatches.stmt1.push_back(currStmt1);
	} else if (!LHSDecl1 && LHSArr1) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: SwapWithoutMemory: Line1: LHSArr, not "
			 << "LHSDecl\n";
#endif
	    currStmt1.tempVarName = LHSArr1->getQualifiedNameAsString();
	    currStmt1.tempVarOrArr = false;
	    sortStmtMatches.stmt1.push_back(currStmt1);
	} else if (LHSDecl1 && LHSArr1) {
#ifdef ERROR
	    llvm::outs() << "ERROR: SwapWithoutMemory: Line1: LHS Both!\n";
#endif
	    return;
	} else {
#ifdef ERROR
	    llvm::outs() << "ERROR: SwapWithoutMemory: Line1: LHS Neither!\n";
#endif
	    return;
	}
    }

    if (SwapOther2) {
	SourceLocation SL = SM.getExpansionLoc(SwapOther2->getLocStart());
	if (SM.isInSystemHeader(SL) || !SM.isWrittenInMainFile(SL))
	    return;
	int swapline = SM.getExpansionLineNumber(SL, 0);
	llvm::outs() << "DEBUG: Found swap without memory line 2 at " 
		     << swapline << "\n";

	swapStmt currStmt2;
	currStmt2.swapStmtLine2 = swapline;

	const VarDecl* LHSDecl2 = 
	    Result.Nodes.getNodeAs<VarDecl>(SwapWithoutMemoryLHSDecl2);
	const VarDecl* LHSArr2 = 
	    Result.Nodes.getNodeAs<VarDecl>(SwapWithoutMemoryLHSArr2);
	if (LHSDecl2 && !LHSArr2) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: SwapWithoutMemory: Line2: LHSDecl, not "
			 << "LHSArr\n";
#endif
	    currStmt2.tempVarName = LHSDecl2->getQualifiedNameAsString();
	    currStmt2.tempVarOrArr = true;
	} else if (!LHSDecl2 && LHSArr2) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: SwapWithoutMemory: Line2: LHSArr, not "
			 << "LHSDecl\n";
#endif
	    currStmt2.tempVarName = LHSArr2->getQualifiedNameAsString();
	    currStmt2.tempVarOrArr = false;
	} else if (LHSDecl2 && LHSArr2) {
#ifdef ERROR
	    llvm::outs() << "ERROR: SwapWithoutMemory: Line2: LHS Both!\n";
#endif
	    return;
	} else {
#ifdef ERROR
	    llvm::outs() << "ERROR: SwapWithoutMemory: Line2: LHS Neither\n";
#endif
	    return;
	}

	const VarDecl* RHSDecl2 =
	    Result.Nodes.getNodeAs<VarDecl>(SwapWithoutMemoryRHSDecl2);
	const VarDecl* RHSArr2 =
	    Result.Nodes.getNodeAs<VarDecl>(SwapWithoutMemoryRHSArr2);
	if (RHSDecl2 && !RHSArr2) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: SwapWithoutMemory: Line2: RHSDecl, not "
			 << "RHSArr\n";
#endif
	    currStmt2.arrayName = RHSDecl2->getQualifiedNameAsString();
	    currStmt2.arrayVarOrArr = true;
	} else if (!RHSDecl2 && RHSArr2) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: SwapWithoutMemory: Line2: RHSArr, not "
			 << "RHSDecl\n";
#endif
	    currStmt2.arrayName = RHSArr2->getQualifiedNameAsString();
	    currStmt2.arrayVarOrArr = false;
	} else if (RHSDecl2 && RHSArr2) {
#ifdef ERROR
	    llvm::outs() << "ERROR: SwapWithoutMemory: Line2: RHS Both!\n";
#endif
	    return;
	} else {
#ifdef ERROR
	    llvm::outs() << "ERROR: SwapWithoutMemory: Line2: RHS Neither!\n";
#endif
	    return;
	}

	for (std::vector<swapStmt>::iterator it = sortStmtMatches.stmt1.begin();
		it != sortStmtMatches.stmt1.end(); it++) {

	    if (it->tempVarName.compare(currStmt2.arrayName) == 0 &&
		it->tempVarOrArr == currStmt2.arrayVarOrArr &&
		(it->swapStmtLine1 == currStmt2.swapStmtLine2 ||
		 it->swapStmtLine1 + 1 == currStmt2.swapStmtLine2)) {

		currStmt2.swapStmtLine1 = it->swapStmtLine1;
		sortStmtMatches.stmt2.push_back(currStmt2);
		break;
	    }
	}
    }

    if (SwapOther3) {
	SourceLocation SL = SM.getExpansionLoc(SwapOther3->getLocStart());
	if (SM.isInSystemHeader(SL) || !SM.isWrittenInMainFile(SL))
	    return;
	int swapline = SM.getExpansionLineNumber(SL, 0);
	llvm::outs() << "DEBUG: Found swap without memory line 3 at " 
		     << swapline << "\n";

	swapStmt currStmt3;
	currStmt3.swapStmtLine3 = swapline;

	const VarDecl* LHSDecl3 = 
	    Result.Nodes.getNodeAs<VarDecl>(SwapWithoutMemoryLHSDecl3);
	const VarDecl* LHSArr3 =
	    Result.Nodes.getNodeAs<VarDecl>(SwapWithoutMemoryLHSArr3);
	if (LHSDecl3 && !LHSArr3) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: SwapWithoutMemory: Line3: LHSDecl, not "
			 << "LHSArr\n";
#endif
	    currStmt3.tempVarName = LHSDecl3->getQualifiedNameAsString();
	    currStmt3.tempVarOrArr = true;
	} else if (!LHSDecl3 && LHSArr3) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: SwapWithoutMemory: Line3: LHSArr, not "
			 << "LHSDecl\n";
#endif
	    currStmt3.tempVarName = LHSArr3->getQualifiedNameAsString();
	    currStmt3.tempVarOrArr = false;
	} else if (LHSDecl3 && LHSArr3) {
#ifdef ERROR
	    llvm::outs() << "ERROR: SwapWithoutMemory: Line3: Both!\n";
#endif
	    return;
	} else {
#ifdef ERROR
	    llvm::outs() << "ERROR: SwapWithoutMemory: Line3: Neither!\n";
#endif
	    return;
	}

	const VarDecl* RHSDecl3 =
	    Result.Nodes.getNodeAs<VarDecl>(SwapWithoutMemoryRHSDecl3);
	const VarDecl* RHSArr3 =
	    Result.Nodes.getNodeAs<VarDecl>(SwapWithoutMemoryRHSArr3);
	if (RHSDecl3 && !RHSArr3) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: SwapWithoutMemory: Line3: RHSDecl, not "
			 << "RHSArr\n";
#endif
	    currStmt3.arrayName = RHSDecl3->getQualifiedNameAsString();
	    currStmt3.arrayVarOrArr = true;
	} else if (!RHSDecl3 && RHSArr3) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: SwapWithoutMemory: Line3: RHSArr, not "
			 << "RHSDecl\n";
#endif
	    currStmt3.arrayName = RHSArr3->getQualifiedNameAsString();
	    currStmt3.arrayVarOrArr = false;
	} else if (RHSDecl3 && RHSArr3) {
#ifdef ERROR
	    llvm::outs() << "ERROR: SwapWithoutMemory: Line3: RHS Both\n";
#endif
	    return;
	} else {
#ifdef ERROR
	    llvm::outs() << "ERROR: SwapWithoutMemory: Line3: RHS Both!\n";
#endif
	    return;
	}

	for (std::vector<swapStmt>::iterator it = sortStmtMatches.stmt2.begin();
		it != sortStmtMatches.stmt2.end(); it++) {
	    if (it->tempVarName.compare(currStmt3.arrayName) == 0 &&
		it->tempVarOrArr == currStmt3.arrayVarOrArr &&
		it->arrayName.compare(currStmt3.tempVarName) == 0 &&
		it->arrayVarOrArr == currStmt3.tempVarOrArr &&
		(it->swapStmtLine2 == currStmt3.swapStmtLine3 || 
		 it->swapStmtLine2 + 1 == currStmt3.swapStmtLine3)) {

		currStmt3.swapStmtLine2 = it->swapStmtLine2;
		currStmt3.swapStmtLine1 = it->swapStmtLine1;
		sortStmtMatches.stmt3.push_back(currStmt3);
		break;
	    }
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Sort Stmts Found:\n";
    sortStmtMatches.printSortStmt3();
#endif

#ifdef DEBUG
    llvm::outs() << "DEBUG: Finishing SortingClass::run()\n";
#endif
}

void SortingClass::runSortingMatcher(ClangTool &Tool) {
    MatchFinder SortingFinder;
    SortingFinder.addMatcher(SortLine1Matcher_1, this);
    SortingFinder.addMatcher(SortLine1Matcher_2, this);
    SortingFinder.addMatcher(SortLine2Matcher, this);
    SortingFinder.addMatcher(SortLine3Matcher, this);
    SortingFinder.addMatcher(SelectionSortMatcher, this);
#if 0
    SortingFinder.addMatcher(SwapWithoutMemoryVarLine1Matcher, this);
    SortingFinder.addMatcher(SwapWithoutMemoryVarLine2Matcher, this);
    SortingFinder.addMatcher(SwapWithoutMemoryVarLine3Matcher, this);
#endif
    SortingFinder.addMatcher(FunctionSwapWithoutMemoryMatcher, this);
    Tool.run(newFrontendActionFactory(&SortingFinder).get());
}
