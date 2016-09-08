#include "MyASTVisitor.h"
#include "GetSymbolicExprVisitor.h"
#include "clang/Analysis/AnalysisContext.h"
#include "MainFunction.h"

/*****************************Helper Functions********************************/
InputVar getInputVarFromExpr(const SourceManager* SM, Expr* E, bool &rv) {
    rv = true;
    InputVar v;
    if (isa<UnaryOperator>(E)) {
        UnaryOperator* UO = dyn_cast<UnaryOperator>(E);
	if (UO->getOpcode() == UnaryOperatorKind::UO_AddrOf)
	    E = UO->getSubExpr()->IgnoreParenCasts();
	v.var = Helper::getVarDetailsFromExpr(SM, E, rv);
	if (!rv) return v;
	v.varExpr = E;
    } else if (isa<BinaryOperator>(E)) {
	BinaryOperator* B = dyn_cast<BinaryOperator>(E);
	Expr* lhsExpr = B->getLHS()->IgnoreParenCasts();
	Expr* rhsExpr = B->getRHS()->IgnoreParenCasts();
	VarDetails lhsVarDetails = Helper::getVarDetailsFromExpr(SM, lhsExpr, rv);
	if (!rv) return v;
	VarDetails rhsVarDetails = Helper::getVarDetailsFromExpr(SM, rhsExpr, rv);
	if (!rv) return v;
	if (lhsVarDetails.arraySizeInfo.size() != 0 &&
		rhsVarDetails.arraySizeInfo.size() == 0) {
	    v.var = lhsVarDetails;
	    v.varExpr = lhsExpr;
	} else if (lhsVarDetails.arraySizeInfo.size() == 0 && 
		rhsVarDetails.arraySizeInfo.size() != 0) {
	    v.var = rhsVarDetails;
	    v.varExpr = rhsExpr;
	} else {
	    llvm::outs() << "ERROR: Read as BinaryOperator does not match "
			 << " any of our patterns\n";
	    rv = false;
	    return v;
	}
    } else {
	v.var = Helper::getVarDetailsFromExpr(SM, E, rv);
	if (!rv) return v;
	v.varExpr = E;
    }

    return v;
}
/*****************************************************************************/

bool GetVarVisitor::VisitDeclRefExpr(DeclRefExpr* E) {

    if (E->isLValue()) {
	// Check if this is a VarDecl
	ValueDecl* d = E->getDecl();
	if (!(isa<DeclaratorDecl>(d))) return true;
	DeclaratorDecl* dd = dyn_cast<DeclaratorDecl>(d);
	if (!(isa<VarDecl>(dd))) return true;
	VarDecl* vd = dyn_cast<VarDecl>(dd);

	// Check if the VarDecl is in the main file of the program.
	SourceLocation SL = SM->getExpansionLoc(vd->getLocStart());
	if (SM->isInSystemHeader(SL) || !(SM->isWrittenInMainFile(SL)))
	    return true;

	const int lineNum = SM->getExpansionLineNumber(E->getLocStart(), 0);
	DeclarationNameInfo nameInfo = E->getNameInfo();
	std::string varName = nameInfo.getAsString();

	struct varOccurrence vo;
	vo.lineNum = lineNum;

	bool rv;
	VarDetails var;
	var = Helper::getVarDetailsFromExpr(SM, E, rv);
	if (!rv) {
	    error = true;
	    return false;
	}
	var.setVarID();
	vo.vd = var;

	vo.numOfOccurrences = 1;

	bool foundSameVar = false;
	for (std::vector<struct varOccurrence>::iterator
		vIt = scalarVarsFound.begin(); vIt != scalarVarsFound.end(); vIt++) {
	    if (vIt->lineNum != lineNum) continue;
	    if (vIt->vd.equals(vo.vd)) {
		foundSameVar = true;
		vIt->numOfOccurrences++;
		break;
	    }
	}
	if (!foundSameVar)
	    scalarVarsFound.push_back(vo);
    }

    return true;
}

bool GetVarVisitor::VisitArraySubscriptExpr(ArraySubscriptExpr* E) {

    if (E->isLValue()) {
	// Check if the ArraySubscriptExpr is in the main file of the program.
	SourceLocation SL = SM->getExpansionLoc(E->getLocStart());
	if (SM->isInSystemHeader(SL) || !(SM->isWrittenInMainFile(SL)))
	    return true;

	const int lineNum = SM->getExpansionLineNumber(E->getLocStart(), 0);

	struct varOccurrence vo;
	vo.lineNum = lineNum;

	bool rv;
	VarDetails var;
	var = Helper::getVarDetailsFromExpr(SM, E, rv);
	if (!rv) {
	    error = true;
	    return false;
	}
	var.setVarID();
	vo.vd = var;

	vo.numOfOccurrences = 1;

	bool foundSameVar = false;
	for (std::vector<struct varOccurrence>::iterator
		vIt = arrayVarsFound.begin(); vIt != arrayVarsFound.end(); vIt++) {
	    if (vIt->lineNum != lineNum) continue;
	    if (vIt->vd.equals(vo.vd)) {
		foundSameVar = true;
		vIt->numOfOccurrences++;
		break;
	    }
	}
	if (!foundSameVar)
	    arrayVarsFound.push_back(vo);
    }

    return true;
}

bool CallExprVisitor::VisitCallExpr(CallExpr* E) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering CallExprVisitor::VisitCallExpr(): "
		 << Helper::prettyPrintExpr(E) << "\n";
#endif

    int callLine = SM->getExpansionLineNumber(E->getLocStart());
    FunctionDecl* fd = E->getDirectCallee();
    if (!fd) {
	llvm::outs() << "ERROR: CallExpr at " 
		     << callLine << " has no callee\n";
	error = true;
	return false;
    }
    fd = fd->getMostRecentDecl();
    std::string funcName = fd->getQualifiedNameAsString();
#ifdef DEBUG
    llvm::outs() << "DEBUG: CallExpr of " << funcName << "\n";
#endif
    SourceLocation SL = SM->getExpansionLoc(fd->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	if (funcName.compare("memset") == 0 || funcName.compare("malloc") == 0 ||
	    funcName.compare("memcpy") == 0 || funcName.compare("exit") == 0 || 
	    funcName.compare("free") == 0 || funcName.compare("read") == 0) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: memset or malloc or memcpy or exit or free or read\n";
#endif
	    return true;
	}
	return true;
    }

#if 0
    // I should not return here because after this MainFunction::run() sets the
    // customInputFunc/customOutputFunc flag in FunctionDetails object
    if (std::find(customInputFuncs.begin(), customInputFuncs.end(), funcName) != 
           customInputFuncs.end()) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: custominputfuncs\n";
#endif
       return true;
    }
    if (std::find(customOutputFuncs.begin(), customOutputFuncs.end(), funcName)
           != customOutputFuncs.end()) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: customoutputfuncs\n";
#endif
       return true;
    }
#endif

    SL = SM->getExpansionLoc(E->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Call not in main progr\n";
#endif
       return true;
    }


#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting callExpr\n";
#endif

    int funcStartLine = SM->getExpansionLineNumber(fd->getLocStart());
    int funcEndLine = SM->getExpansionLineNumber(fd->getLocEnd());
    if (!fd->hasBody()) {
	llvm::outs() << "ERROR: Function at (" << funcStartLine << " - " 
		     << funcEndLine << ") has no body\n";
	error = true;
	return false;
    }

    FunctionDecl* calleefd = fd->getMostRecentDecl();
    if (!calleefd) {
	llvm::outs() << "ERROR: Cannot get most recent decl of function "
		     << fd->getQualifiedNameAsString() << "\n";
	error = true;
	return false;
    }
    if (!(calleefd->hasBody())) {
	llvm::outs() << "ERROR: Function declaration has no body\n";
	error = true;
	return false;
    }

    // Add the callee function to call graph
    cg->addToCallGraph(calleefd);

    bool rv;
    FunctionDetails* f = Helper::getFunctionDetailsFromDecl(SM, calleefd, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain FunctionDetails of callee at "
		     << "callExpr line "  << callLine << "\n";
	error = true;
	return false;
    }

    // Check if this function is already recorded
    //for (std::vector<Helper::FunctionDetails>::iterator fIt =
    for (std::vector<FunctionDetails*>::iterator fIt =
	    funcsFound.begin(); fIt != funcsFound.end(); fIt++) {
	if ((*fIt)->funcName.compare(f->funcName) == 0 && (*fIt)->funcStartLine == 
		f->funcStartLine && (*fIt)->funcEndLine == f->funcEndLine) {
	    return true;
	}
    }

    funcsFound.push_back(f);
    return true;
}

bool InputStmtVisitor::VisitCallExpr(CallExpr* E) {
    SourceLocation SL = SM->getExpansionLoc(E->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL))
	return true;

    int callLine = SM->getExpansionLineNumber(E->getLocStart());

    FunctionDecl* calleeFD = E->getDirectCallee();
    if (!calleeFD) {
	llvm::outs() << "ERROR: Cannot get callee FD from callExpr\n";
	error = true;
	return false;
    }

    DeclarationNameInfo fInfo = calleeFD->getNameInfo();
    std::string fName = fInfo.getAsString();

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting callExpr of " << fName << " at line " 
		 << callLine << "\n";
#endif

#if 0
    // Check if this function is one of the standard input functions
    if (fName.compare("operator>>") == 0 || fName.compare("scanf") == 0) {
	llvm::outs() << "DEBUG: Found operator>> or scanf\n";
    }
#endif

    std::vector<Expr*> argExprs;

    bool isCin = false;
    bool isScanf = false, isFScanf = false;
    bool isCustom = false;
    if (calleeFD->isOverloadedOperator()) {
	if (isa<CXXOperatorCallExpr>(E)) {
	    CXXOperatorCallExpr* C = dyn_cast<CXXOperatorCallExpr>(E);
	    if (!C) {
		llvm::outs() << "ERROR: Cannot get CXXOperatorCallExpr from CallExpr\n";
		error = true;
		return false;
	    }
	    bool rv;
	    CXXOperatorCallExprDetails* currDetails =
		CXXOperatorCallExprDetails::getObject(SM, C, -1, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot obtain CXXOperatorCallExprDetails "
			     << "from CXXOperatorCallExpr\n";
		error = true;
		return false;
	    }

	    if (currDetails->opKind == CXXOperatorKind::CIN) {
		isCin = true;
		for (std::vector<ExprDetails*>::iterator aIt =
			currDetails->callArgExprs.begin(); aIt != 
			currDetails->callArgExprs.end(); aIt++) {
		    if (!((*aIt)->origExpr)) {
			llvm::outs() << "ERROR: NULL arg for cin at line " 
				     << callLine << "\n";
			error = true;
			return false;
		    }
		    argExprs.push_back((*aIt)->origExpr);
		}
		if (!isInputStmtRecorded(currDetails))
		    inputStmts.push_back(currDetails);
	    }
	}
#if 0
	if (fName.compare("operator>>") != 0) return true;

	// Found operator>>. Check if the callExpr has exactly 2 arguments
	if (E->getNumArgs() != 2) {
	    llvm::outs() << "ERROR: operator>> has " << calleeFD->param_size()
			 << " arguments\n";
	    error = true;
	    return false;
	}

	// Check if the first argument is cin, then the
	// second argument is the input var
	std::string argName;
	Expr* arg0 = E->getArg(0);
	if (isa<DeclRefExpr>(arg0)) {
	    argName = Helper::prettyPrintExpr(arg0);
	    if (argName.compare("cin") != 0) {
		llvm::outs() << "WARNING: First argument of operator>> is not cin\n";
		return true;
	    }
	} else {
	    llvm::outs() << "WARNING: First argument of operator>> is not DeclRefExpr\n";
	    return true;
	}

	isCin = true;
	// First argument is cin. Get second argument
	Expr* arg1 = E->getArg(1);
	argExprs.push_back(arg1);
#endif
    } else if (fName.compare("scanf") == 0) {
	isScanf = true;
	bool rv;
	CallExprDetails* scanfDetails = CallExprDetails::getObject(SM, E, -1, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot get CallExprDetails from scanf\n";
	    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
	    error = true;
	    return false;
	}

	if (!isInputStmtRecorded(scanfDetails)) {
	    inputStmts.push_back(scanfDetails);
	}

	for (CallExpr::arg_iterator aIt = E->arg_begin(); aIt != E->arg_end();
		aIt = aIt + 1) {
	    // Skip the first argument of scanf, it is the format string.
	    // TODO: Should we skip or should we record it?
	    if (aIt == E->arg_begin()) continue;
	    Expr* arg = *aIt;
	    argExprs.push_back(arg);
	}
    } else if (fName.compare("fscanf") == 0) {
	isFScanf = true;
	bool rv;
	CallExprDetails* fscanfDetails = CallExprDetails::getObject(SM, E, -1, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot get CallExprDetails from fscanf\n";
	    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
	    error = true;
	    return false;
	}

	if (!isInputStmtRecorded(fscanfDetails)) {
	    inputStmts.push_back(fscanfDetails);
	}

	for (CallExpr::arg_iterator aIt = E->arg_begin()+2; aIt != E->arg_end();
		aIt = aIt + 1) {
	    // we skip first two arguments of fscanf, it is the stream and the
	    // format string
	    Expr* arg = *aIt;
	    argExprs.push_back(arg);
	}
    } else if (std::find(customInputFuncs.begin(), customInputFuncs.end(), fName) != 
	    customInputFuncs.end()) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found custom input function\n";
#endif
	isCustom = true;

	// If callexpr does not have any arguments, then the input function
	// returns the input value and we need to match the assignment statement
	// pattern
	if (E->getNumArgs() == 0)
	    return true;

	bool rv;
	CallExprDetails* customCallDetails = CallExprDetails::getObject(SM, E,
	    -1, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot get CallExprDetails of custom input "
			 << "function " << Helper::prettyPrintExpr(E) << "\n";
	    error = true;
	    return false;
	}

	if (!isInputStmtRecorded(customCallDetails)) {
	    inputStmts.push_back(customCallDetails);
	}

	for (CallExpr::arg_iterator aIt = E->arg_begin(); aIt != E->arg_end();
		aIt = aIt + 1) {
	    Expr* arg = *aIt;
	    argExprs.push_back(arg);
	}
    } else 
	return true;

    // Find details of input vars
    for (std::vector<Expr*>::iterator it = argExprs.begin(); it !=
	    argExprs.end(); it++) {
	bool rv;
	InputVar v = getInputVarFromExpr(SM, *it, rv);
	if (!rv) {
	    error = true;
	    return false;
	}
	v.varExpr = *it;
	v.inputCallLine = callLine;
	if (isCin)
	    v.inputFunction = InputVar::InputFunc::CIN;
	else if (isScanf)
	    v.inputFunction = InputVar::InputFunc::SCANF;
	else if (isFScanf)
	    v.inputFunction = InputVar::InputFunc::FSCANF;
	else if (isCustom)
	    v.inputFunction = InputVar::InputFunc::CUSTOM;
	else {
	    llvm::outs() << "ERROR: Cannot resolve input function: not CIN, "
			 << "SCANF or CUSTOM\n";
	    error = true;
	    return false;
	}
#if 0
	llvm::outs() << "DEBUG: Found input var:\n";
	v.print();
#endif

	inputVarsFound.push_back(v);
	if (std::find(inputLinesFound.begin(), inputLinesFound.end(), callLine)
		== inputLinesFound.end()) {
	    inputLinesFound.push_back(callLine);
	}
    }

    return true;
}

bool InputStmtVisitor::VisitBinaryOperator(BinaryOperator* B) {
    SourceLocation SL = SM->getExpansionLoc(B->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL))
	return true;

    int callLine = SM->getExpansionLineNumber(B->getLocStart());

    // Skip if this is not an assignment op.
    if (!B->isAssignmentOp()) return true;

    // Check if the rhs is a call expr.
    Expr* rhsExpr = B->getRHS();
    if (!isa<CallExpr>(rhsExpr)) return true;

    CallExpr* rhsCallExpr = dyn_cast<CallExpr>(rhsExpr);

    FunctionDecl* calleeFD = rhsCallExpr->getDirectCallee();
    if (!calleeFD) {
	llvm::outs() << "ERROR: Cannot get callee FD from callExpr\n";
	error = true;
	return false;
    }

    DeclarationNameInfo fInfo = calleeFD->getNameInfo();
    std::string fName = fInfo.getAsString();

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting assignment with " << fName << " at line " 
		 << callLine << "\n";
#endif

    // If this call is not to one of the custom input functions, skip.
    if (std::find(customInputFuncs.begin(), customInputFuncs.end(), fName) == 
	    customInputFuncs.end())
	return true;
    
    // This pattern is supposed to match those custom input functions that do
    // not take any arguments, but returns the input value read. So the input
    // var in this pattern is the lhs of the assignment. So check if this
    // function takes any argument.
    if (rhsCallExpr->getNumArgs() > 0 || calleeFD->param_size() > 0) {
	llvm::outs() << "ERROR: Custom input function call at line " << callLine 
		     << " both takes an argument and returns a value\n";
	error = true;
	return false;
    }

    bool rv;
    BinaryOperatorDetails* customInputDetails =
	BinaryOperatorDetails::getObject(SM, B, -1, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain BinaryOperatorDetails of custom "
		     << "input function " << Helper::prettyPrintExpr(B) << "\n";
	error = true;
	return false;
    }

    if (!isInputStmtRecorded(customInputDetails)) {
	inputStmts.push_back(customInputDetails);
    }

    // Return LHS as the input var.
    Expr* lhsExpr = B->getLHS();
    InputVar v = getInputVarFromExpr(SM, lhsExpr, rv);
    if (!rv) {
	error = true;
	return false;
    }

    v.varExpr = lhsExpr;
    v.inputCallLine = callLine;
    v.inputFunction = InputVar::InputFunc::CUSTOM;

    inputVarsFound.push_back(v);
    if (std::find(inputLinesFound.begin(), inputLinesFound.end(), callLine)
	    == inputLinesFound.end()) 
	inputLinesFound.push_back(callLine);

    return true;
}

bool SequenceInputsVisitor::VisitCallExpr(CallExpr* E) {
    SourceLocation SL = SM->getExpansionLoc(E->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL))
	return true;

    int callLine = SM->getExpansionLineNumber(E->getLocStart());
    FunctionDecl* calleeFD = E->getDirectCallee();
    if (!calleeFD) {
	llvm::outs() << "ERROR: Cannot get calleeFD from callExpr\n";
	error = true;
	return false;
    }
    SL = SM->getExpansionLoc(calleeFD->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL))
	return true;

    DeclarationNameInfo fInfo = calleeFD->getNameInfo();
    std::string calleeName = fInfo.getAsString();
    int calleeStartLine = SM->getExpansionLineNumber(calleeFD->getLocStart());
    int calleeEndLine = SM->getExpansionLineNumber(calleeFD->getLocEnd());

    // Check if the callee has any children in the call graph
    bool hasChildren = false;
    CallGraphNode* n = cg->getNode(calleeFD);
    if (!n->empty())
	hasChildren = true;

    // Check if the callee has any input vars
    bool hasInputs = false;
    bool rv;
    //Helper::FunctionDetails calleeDetails =
    FunctionDetails* calleeDetails =
	MObj->getDetailsOfFunction(calleeName, calleeStartLine, calleeEndLine,
	rv);
    if (!rv) {
	error = true;
	return false;
    }
    FunctionAnalysis* calleeFA = MObj->getAnalysisObjOfFunction(calleeDetails, rv);
    if (!rv) {
	error = true;
	return false;
    }
    if (calleeFA->inputVars.size() > 0)
	hasInputs = true;

    if (!hasChildren && !hasInputs)
	return true;

    // Copy the inputs before this line to sequencedInputVars
    std::vector<InputVar>::iterator it;
    for (it = inputsInCurrFunc.begin();it != inputsInCurrFunc.end(); it++) {
	if (it->inputCallLine < callLine) {
	    sequencedInputVars.push_back(it->clone());
	    inputsInCurrFunc.erase(it);
	} else
	    break;
    }

    SequenceInputsVisitor siv(SM, cg, MObj, calleeDetails, calleeFA->inputVars);
    siv.TraverseDecl(const_cast<FunctionDecl*>(calleeFD));
    if (siv.error) {
	error = true;
	return false;
    }

    sequencedInputVars.insert(sequencedInputVars.end(),
	siv.sequencedInputVars.begin(), siv.sequencedInputVars.end());
    sequencedInputVars.insert(sequencedInputVars.end(),
	siv.inputsInCurrFunc.begin(), siv.inputsInCurrFunc.end());

    return true;
}

bool SequenceInputsVisitor::TraverseFunctionDecl(FunctionDecl* fd) {
    DeclarationNameInfo fInfo = fd->getNameInfo();
    std::string fName = fInfo.getAsString();
    int startLine = SM->getExpansionLineNumber(fd->getLocStart());
    int endLine = SM->getExpansionLineNumber(fd->getLocEnd());

    bool rv;
    FunctionDetails* fDetails =
	MObj->getDetailsOfFunction(fName, startLine, endLine, rv);
    if (!rv) {
	error = true;
	return false;
    }

    if (fDetails->isCustomInputFunction)
	return false;

    return true;
}

bool SpecialStmtVisitor::VisitBinaryOperator(BinaryOperator* B) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering SpecialStmtVisitor::VisitBinaryOperator()\n";
#endif
    SourceLocation SL = SM->getExpansionLoc(B->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL))
	return true;

    // If it is not an assignment statement, then skip
    if (!B->isAssignmentOp()) return true;

    SpecialExpr se;
    bool rv;

    se.kind = SpecialExprKind::UNRESOLVED;

    int assignLine = SM->getExpansionLineNumber(B->getLocStart());
    se.assignmentLine = assignLine;

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting "
		 << Helper::prettyPrintExpr(B) << "\n";
#endif

    Expr* lhsExpr = B->getLHS();
    if (!lhsExpr) {
	llvm::outs() << "ERROR: Cannot get lhs expr of BinaryOperator\n";
	error = true;
	return false;
    }

    // Check if lhs is a scalar var
    if (!isa<DeclRefExpr>(lhsExpr))
	return true;

    se.scalarVar = Helper::getVarDetailsFromExpr(SM, lhsExpr, rv);
    if (!rv) {
	error = true;
	return false;
    }

    Expr* rhsExpr = B->getRHS();
    if (!rhsExpr) {
	llvm::outs() << "ERROR: Cannot get rhs expr of BinaryOperator\n";
	error = true;
	return false;
    }
    rhsExpr = rhsExpr->IgnoreParenCasts();

    if (isa<ArraySubscriptExpr>(rhsExpr)) {
	ArraySubscriptExpr* rhsArrayExpr = dyn_cast<ArraySubscriptExpr>(rhsExpr);
	se.arrayVar = Helper::getVarDetailsFromExpr(SM, rhsExpr, rv);
	if (!rv) {
	    error = true;
	    return false;
	}

	se.arrayIndexExprsAtAssignLine = Helper::getArrayIndexExprs(rhsArrayExpr);

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting Assignment statement at line "
		 << assignLine << "\n";
#endif

    } else if (isa<BinaryOperator>(rhsExpr)) {
	BinaryOperator* rhsBO = dyn_cast<BinaryOperator>(rhsExpr);
	if (rhsBO->getOpcode() != BinaryOperatorKind::BO_Add) return true;
	se.kind = SpecialExprKind::SUM;
	Expr* lhs = rhsBO->getLHS()->IgnoreParenCasts();
	Expr* rhs = rhsBO->getRHS()->IgnoreParenCasts();
	std::string lhsAssign = Helper::prettyPrintExpr(lhsExpr);
	if (isa<DeclRefExpr>(lhs) && isa<ArraySubscriptExpr>(rhs)) {
	    std::string lhsRHS = Helper::prettyPrintExpr(lhs);
	    if (lhsAssign.compare(lhsRHS) != 0) return true;

	    ArraySubscriptExpr* rhsArray = dyn_cast<ArraySubscriptExpr>(rhs);
	    se.arrayVar = Helper::getVarDetailsFromExpr(SM, rhs, rv);
	    if (!rv) {
		error = true;
		return false;
	    }
	    se.arrayIndexExprsAtAssignLine = Helper::getArrayIndexExprs(rhsArray);
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Visiting Assignment statement at line "
		 << assignLine << "\n";
#endif
	} else if (isa<ArraySubscriptExpr>(lhs) && isa<DeclRefExpr>(rhs)) {
	    std::string rhsRHS = Helper::prettyPrintExpr(rhs);
	    if (lhsAssign.compare(rhsRHS) != 0) return true;

	    ArraySubscriptExpr* lhsArray = dyn_cast<ArraySubscriptExpr>(lhs);
	    se.arrayVar = Helper::getVarDetailsFromExpr(SM, lhs, rv);
	    if (!rv) {
		error = true;
		return false;
	    }
	    se.arrayIndexExprsAtAssignLine = Helper::getArrayIndexExprs(lhsArray);
	} else 
	    return true;
    } else if (isa<ConditionalOperator>(rhsExpr)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found ConditionalOperator\n";
#endif
	se.fromTernaryOperator = true;
        ConditionalOperator* rhsCO = dyn_cast<ConditionalOperator>(rhsExpr);
        Expr* condition = rhsCO->getCond()->IgnoreParenCasts();
        if (!isa<BinaryOperator>(condition)) return true;
        BinaryOperator* condBO = dyn_cast<BinaryOperator>(condition);
        if (!(condBO->isRelationalOp())) return true;
        BinaryOperatorKind op = condBO->getOpcode();
        if (op != BinaryOperatorKind::BO_LT && op != BinaryOperatorKind::BO_GT &&
            op != BinaryOperatorKind::BO_LE && op != BinaryOperatorKind::BO_GE)
                return true;
        Expr* condLHS = condBO->getLHS()->IgnoreParenCasts();
        Expr* condRHS = condBO->getRHS()->IgnoreParenCasts();

        Expr* trueExpr = rhsCO->getTrueExpr()->IgnoreParenCasts();
        Expr* falseExpr = rhsCO->getFalseExpr()->IgnoreParenCasts();

	std::string trueExprStr = Helper::prettyPrintExpr(trueExpr);
	std::string falseExprStr = Helper::prettyPrintExpr(falseExpr);
	std::string condLHSStr = Helper::prettyPrintExpr(condLHS);
	std::string condRHSStr = Helper::prettyPrintExpr(condRHS);
        if (isa<DeclRefExpr>(trueExpr) && isa<ArraySubscriptExpr>(falseExpr)) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: trueExpr: decl and falseExpr: array\n";
	    llvm::outs() << "trueExpr: " << trueExprStr << " falseExpr: "
			 << falseExprStr << "\n";
#endif
	    ArraySubscriptExpr* array = dyn_cast<ArraySubscriptExpr>(falseExpr);
	    se.arrayVar = Helper::getVarDetailsFromExpr(SM, array, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While getting VarDetails from "
			     << "ArraySubscriptExpr: " 
			     << Helper::prettyPrintExpr(array);
		error = true;
		return false;
	    }
	    se.arrayIndexExprsAtAssignLine = Helper::getArrayIndexExprs(array);

            if (isa<DeclRefExpr>(condLHS) && isa<ArraySubscriptExpr>(condRHS)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: condLHS: decl and condRHS: array\n";
		llvm::outs() << "condLHS: " << condLHSStr << " condRHS: "
			     << condRHSStr << "\n";
#endif
		if (trueExprStr.compare(condLHSStr) != 0 ||
			falseExprStr.compare(condRHSStr) != 0) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: trueExprStr: " << trueExprStr
				 << " condLHSStr: " << condLHSStr
				 << " falseExprStr: " << falseExprStr
				 << " condRHSStr: " << condRHSStr << "\n";
#endif
		    return true;
		}
		if (op == BinaryOperatorKind::BO_GT || op ==
			BinaryOperatorKind::BO_GE) {
#ifdef DEBUG
		    llvm::outs() << "SpecialExprKind::MAX\n";
#endif
		    se.kind = SpecialExprKind::MAX;
		} else if (op == BinaryOperatorKind::BO_LT || op == 
			BinaryOperatorKind::BO_LE) {
#ifdef DEBUG
		    llvm::outs() << "SpecialExprKind::MIN\n";
#endif
		    se.kind = SpecialExprKind::MIN;
		} else {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: op is not GT/GE/LT/LE\n";
#endif
		    return true;
		}
            } else if (isa<ArraySubscriptExpr>(condLHS) && isa<DeclRefExpr>(condRHS)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: condLHS: array and condRHS: decl\n";
		llvm::outs() << "condLHS: " << condLHSStr << " condRHS: "
			     << condRHSStr << "\n";
#endif
		if (trueExprStr.compare(condRHSStr) != 0 ||
			falseExprStr.compare(condLHSStr) != 0) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: trueExprStr: " << trueExprStr
				 << " condLHSStr: " << condLHSStr
				 << " falseExprStr: " << falseExprStr
				 << " condRHSStr: " << condRHSStr << "\n";
#endif
		    return true;
		}
		if (op == BinaryOperatorKind::BO_GT || op ==
			BinaryOperatorKind::BO_GE) {
#ifdef DEBUG
		    llvm::outs() << "SpecialExprKind::MIN\n";
#endif
		    se.kind = SpecialExprKind::MIN;
		} else if (op == BinaryOperatorKind::BO_LT || op == 
			BinaryOperatorKind::BO_LE) {
#ifdef DEBUG
		    llvm::outs() << "SpecialExprKind::MAX\n";
#endif
		    se.kind = SpecialExprKind::MAX;
		} else {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: op is not GT/GE/LT/LE\n";
#endif
		    return true;
		}
            } else {
#ifdef DEBUG
		llvm::outs() << "DEBUG: LHS/RHS is not DeclRefExpr/ArraySubscriptExpr\n";
#endif
                return true;
	    }
        } else if (isa<ArraySubscriptExpr>(trueExpr) && isa<DeclRefExpr>(falseExpr)) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: trueExpr: array and falseExpr:decl\n";
#endif
	    ArraySubscriptExpr* array = dyn_cast<ArraySubscriptExpr>(trueExpr);
	    se.arrayVar = Helper::getVarDetailsFromExpr(SM, array, rv);
	    if (!rv) {
		error = true;
		return false;
	    }
	    se.arrayIndexExprsAtAssignLine = Helper::getArrayIndexExprs(array);

            if (isa<DeclRefExpr>(condLHS) && isa<ArraySubscriptExpr>(condRHS)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: condLHS: decl and condRHS: array\n";
#endif
		if (trueExprStr.compare(condRHSStr) != 0 ||
			falseExprStr.compare(condLHSStr) != 0) 
		    return true;
		if (op == BinaryOperatorKind::BO_GT || op ==
			BinaryOperatorKind::BO_GE) 
		    se.kind = SpecialExprKind::MIN;
		else if (op == BinaryOperatorKind::BO_LT || op == 
			BinaryOperatorKind::BO_LE)
		    se.kind = SpecialExprKind::MAX;
		else
		    return true;
            } else if (isa<ArraySubscriptExpr>(condLHS) && isa<DeclRefExpr>(condRHS)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: condLHS: array and condRHS: decl\n";
#endif
		if (trueExprStr.compare(condLHSStr) != 0 ||
			falseExprStr.compare(condRHSStr) != 0) 
		    return true;
		if (op == BinaryOperatorKind::BO_GT || op ==
			BinaryOperatorKind::BO_GE) 
		    se.kind = SpecialExprKind::MAX;
		else if (op == BinaryOperatorKind::BO_LT || op == 
			BinaryOperatorKind::BO_LE)
		    se.kind = SpecialExprKind::MIN;
		else
		    return true;
            } else
                return true;
        } else {
            return true;
        }
    } else 
	return true;

    bool isExprPresent = false;
    for (std::vector<SpecialExpr>::iterator sIt = exprsFound.begin(); sIt != 
	    exprsFound.end(); sIt++) {
	if (sIt->assignmentLine != assignLine) continue;
	if (sIt->scalarVar.varName.compare(se.scalarVar.varName) == 0 && 
		sIt->scalarVar.varDeclLine == se.scalarVar.varDeclLine && 
		sIt->arrayVar.varName.compare(se.arrayVar.varName) == 0 && 
		sIt->arrayVar.varDeclLine == se.arrayVar.varDeclLine) {
	    isExprPresent = true;
	}
    }
    if (!isExprPresent) {
	exprsFound.push_back(se);
    }

    return true;
}

bool InputOutputStmtVisitor::VisitCallExpr(CallExpr* E) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering InputOutputStmtVisitor::VisitCallExpr()\n";
    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
#endif
    SourceLocation SL = SM->getExpansionLoc(E->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: CallExpr is not in main program\n";
	error = true;
	return false;
    }

    int callLine = SM->getExpansionLineNumber(E->getLocStart());

    FunctionDecl* calleeFD = E->getDirectCallee();
    if (!calleeFD) {
	llvm::outs() << "ERROR: Cannot get callee FD from callExpr\n";
	error = true;
	return false;
    }
    calleeFD = calleeFD->getMostRecentDecl();
    if (!calleeFD) {
	llvm::outs() << "ERROR: Cannot get most recent decl of callee\n";
	error = true;
	return false;
    }

    DeclarationNameInfo fInfo = calleeFD->getNameInfo();
    std::string fName = fInfo.getAsString();

    if (fName.compare("exit") == 0) return true;

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting callExpr of " << fName << " at line " 
		 << callLine << "\n";
#endif

    // Check if this call is already visited.
    bool rv;
    CallExprDetails* cDetails = CallExprDetails::getObject(SM, E, currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get callExprDetails\n";
	error = true;
	return false;
    }
    for (std::vector<CallExprDetails*>::iterator sIt = callExprsVisited.begin();
	    sIt != callExprsVisited.end(); sIt++) {
	if ((*sIt)->equals(cDetails)) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: CallExpr already visited. Skipping..\n";
#endif
	    return true;
	}
    }
#ifdef DEBUG
    llvm::outs() << "DEBUG: callExprsVisited:\n";
    for (std::vector<CallExprDetails*>::iterator cIt = callExprsVisited.begin();
	    cIt != callExprsVisited.end(); cIt++) {
	(*cIt)->prettyPrint();
    }
    llvm::outs() << "DEBUG: currentCall:\n";
    cDetails->prettyPrint();
#endif
    callExprsVisited.push_back(cDetails);

    std::vector<Expr*> argExprs;

    bool isCin = false;
    //bool isCout = false;
    bool isScanf = false, isFScanf = false;
    //bool isPrintf = false;
    bool isCustomInput = false;
    //bool isCustomOutput = false;
    if (calleeFD->isOverloadedOperator()) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found overloadedoperator\n";
#endif
	if (isa<CXXOperatorCallExpr>(E)) {
	    CXXOperatorCallExpr* C = dyn_cast<CXXOperatorCallExpr>(E);
	    if (!C) {
		llvm::outs() << "ERROR: Cannot get CXXOperatorCallExpr from CallExpr\n";
		error = true;
		return false;
	    }
	    CXXOperatorCallExprDetails* currDetails =
		CXXOperatorCallExprDetails::getObject(SM, C, currBlock, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot obtain CXXOperatorCallExprDetails "
			     << "from CXXOperatorCallExpr\n";
		error = true;
		return false;
	    }

	    if (currDetails->opKind == CXXOperatorKind::CIN) {
		isCin = true;
		for (std::vector<ExprDetails*>::iterator aIt =
			currDetails->callArgExprs.begin(); aIt != 
			currDetails->callArgExprs.end(); aIt++) {
		    if (!((*aIt)->origExpr)) {
			llvm::outs() << "ERROR: NULL arg for cin at line " 
				     << callLine << "\n";
			error = true;
			return false;
		    }
		    argExprs.push_back((*aIt)->origExpr);
		}
		if (!isInputStmtRecorded(currDetails)) {
		    StmtDetails* similarInputStmt =
			getSimilarInputStmt(currDetails, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While obtaining similar "
				     << "InputStmt\n";
			error = true;
			return false;
		    }
		    if (!similarInputStmt) 
			inputStmts.push_back(currDetails);
		    else {
			if (!isa<CallExprDetails>(similarInputStmt)) {
			    llvm::outs() << "ERROR: Similar InputStmt is not "
					 << "CallExprDetails\n";
			    error = true;
			    return false;
			}
			CallExprDetails* similarCallExpr =
			    dyn_cast<CallExprDetails>(similarInputStmt);
			unsigned numArgs =
			    similarCallExpr->callArgExprs.size();
			for (unsigned i = numArgs; i <
				currDetails->callArgExprs.size(); i++) {
			    similarCallExpr->callArgExprs.push_back(
				currDetails->callArgExprs[i]);
			}
		    }
		}
		if (std::find(inputLinesFound.begin(), inputLinesFound.end(), callLine)
			== inputLinesFound.end())
		    inputLinesFound.push_back(callLine);
	    } else if (currDetails->opKind == CXXOperatorKind::COUT) {
		//isCout = true;
		bool isWhitespaceOutput = true;
		for (std::vector<ExprDetails*>::iterator argIt =
			currDetails->callArgExprs.begin(); argIt != 
			currDetails->callArgExprs.end(); argIt++) {
		    std::string outputString =
			Helper::prettyPrintExpr((*argIt)->origExpr);
		    outputString = trim(outputString);
		    if (outputString.size() != 0) {
			isWhitespaceOutput = false;
			break;
		    }
		}
		if (!isWhitespaceOutput) {
		  for (std::vector<ExprDetails*>::iterator aIt = 
			currDetails->callArgExprs.begin(); aIt != 
			currDetails->callArgExprs.end(); aIt++) {
		    if (!((*aIt)->origExpr)) {
			llvm::outs() << "ERROR: NULL arg for cout at line "
				     << callLine << "\n";
			error = true;
			return false;
		    }
		    argExprs.push_back((*aIt)->origExpr);
		  }
		  if (!isOutputStmtRecorded(currDetails)) {
		    StmtDetails* similarOutputStmt =
			getSimilarOutputStmt(currDetails, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While obtaining similar "
				     << "OutputStmt\n";
			error = true;
			return false;
		    }
		    if (!similarOutputStmt) 
			outputStmts.push_back(currDetails);
		    else {
			if (!isa<CallExprDetails>(similarOutputStmt)) {
			    llvm::outs() << "ERROR: Similar OutputStmt is not "
					 << "CallExprDetails\n";
			    error = true;
			    return false;
			}
			CallExprDetails* similarCallExpr =
			    dyn_cast<CallExprDetails>(similarOutputStmt);
			unsigned numArgs =
			    similarCallExpr->callArgExprs.size();
			for (unsigned i = numArgs; i <
				currDetails->callArgExprs.size(); i++) {
			    similarCallExpr->callArgExprs.push_back(
				currDetails->callArgExprs[i]);
			}
		    }
		  }
		  if (std::find(outputLinesFound.begin(), outputLinesFound.end(), callLine)
			== outputLinesFound.end())
		    outputLinesFound.push_back(callLine);
		}
	    }
	}
    } else if (fName.compare("scanf") == 0) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found scanf\n";
#endif
	// Sanity check: source location of scanf FunctionDecl is not in the
	// main program
	SourceLocation SL = SM->getExpansionLoc(calleeFD->getLocStart());
	if (!(SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL))) {
	    llvm::outs() << "ERROR: Function named scanf has declaration in main "
			 << "program\n";
	    error = true;
	    return false;
	}

	isScanf = true;
	CallExprDetails* scanfDetails = CallExprDetails::getObject(SM, E,
	    currBlock, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot get CallExprDetails from scanf\n";
	    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
	    error = true;
	    return false;
	}

	if (!isInputStmtRecorded(scanfDetails)) {
	    StmtDetails* similarInputStmt =
		getSimilarInputStmt(scanfDetails, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While obtaining similar "
			     << "InputStmt\n";
		error = true;
		return false;
	    }
	    if (!similarInputStmt) 
		inputStmts.push_back(scanfDetails);
	    else {
		if (!isa<CallExprDetails>(similarInputStmt)) {
		    llvm::outs() << "ERROR: Similar InputStmt is not "
				 << "CallExprDetails\n";
		    error = true;
		    return false;
		}
		CallExprDetails* similarCallExpr =
		    dyn_cast<CallExprDetails>(similarInputStmt);
		unsigned numArgs =
		    similarCallExpr->callArgExprs.size();
		for (unsigned i = numArgs; i <
			scanfDetails->callArgExprs.size(); i++) {
		    similarCallExpr->callArgExprs.push_back(
			scanfDetails->callArgExprs[i]);
		}
	    }
	}

	if (std::find(inputLinesFound.begin(), inputLinesFound.end(), callLine)
		== inputLinesFound.end())
	    inputLinesFound.push_back(callLine);
#if 0
	for (CallExpr::arg_iterator aIt = E->arg_begin(); aIt != E->arg_end();
		aIt = aIt + 1) {
	    // Skip the first argument of scanf, it is the format string.
	    // TODO: Should we skip or should we record it?
	    if (aIt == E->arg_begin()) continue;
	    Expr* arg = *aIt;
	    argExprs.push_back(arg);
	}
#endif
	for (std::vector<ExprDetails*>::iterator aIt =
		scanfDetails->callArgExprs.begin(); aIt !=
		scanfDetails->callArgExprs.end(); aIt++) {
	    if (!((*aIt)->origExpr)) {
		llvm::outs() << "ERROR: NULL arg for scanf at line "
			     << callLine << "\n";
		error = true;
		return false;
	    }
	    argExprs.push_back((*aIt)->origExpr);
	}
    } else if (fName.compare("fscanf") == 0) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found fscanf\n";
#endif
	// Sanity check: source location of scanf FunctionDecl is not in the
	// main program
	SourceLocation SL = SM->getExpansionLoc(calleeFD->getLocStart());
	if (!(SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL))) {
	    llvm::outs() << "ERROR: Function named fscanf has declaration in main "
			 << "program\n";
	    error = true;
	    return false;
	}

	isFScanf = true;
	CallExprDetails* fscanfDetails = CallExprDetails::getObject(SM, E,
	    currBlock, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot get CallExprDetails from fscanf\n";
	    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
	    error = true;
	    return false;
	}

	if (!isInputStmtRecorded(fscanfDetails)) {
	    StmtDetails* similarInputStmt =
		getSimilarInputStmt(fscanfDetails, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While obtaining similar "
			     << "InputStmt\n";
		error = true;
		return false;
	    }
	    if (!similarInputStmt) 
		inputStmts.push_back(fscanfDetails);
	    else {
		if (!isa<CallExprDetails>(similarInputStmt)) {
		    llvm::outs() << "ERROR: Similar InputStmt is not "
				 << "CallExprDetails\n";
		    error = true;
		    return false;
		}
		CallExprDetails* similarCallExpr =
		    dyn_cast<CallExprDetails>(similarInputStmt);
		unsigned numArgs =
		    similarCallExpr->callArgExprs.size();
		for (unsigned i = numArgs; i <
			fscanfDetails->callArgExprs.size(); i++) {
		    similarCallExpr->callArgExprs.push_back(
			fscanfDetails->callArgExprs[i]);
		}
	    }
	}

	if (std::find(inputLinesFound.begin(), inputLinesFound.end(), callLine)
		== inputLinesFound.end())
	    inputLinesFound.push_back(callLine);
#if 0
	for (CallExpr::arg_iterator aIt = E->arg_begin(); aIt != E->arg_end();
		aIt = aIt + 1) {
	    // Skip the first argument of scanf, it is the format string.
	    // TODO: Should we skip or should we record it?
	    if (aIt == E->arg_begin()) continue;
	    Expr* arg = *aIt;
	    argExprs.push_back(arg);
	}
#endif
	for (std::vector<ExprDetails*>::iterator aIt =
		fscanfDetails->callArgExprs.begin(); aIt !=
		fscanfDetails->callArgExprs.end(); aIt++) {
	    if (!((*aIt)->origExpr)) {
		llvm::outs() << "ERROR: NULL arg for fscanf at line "
			     << callLine << "\n";
		error = true;
		return false;
	    }
	    argExprs.push_back((*aIt)->origExpr);
	}
    } else if (fName.compare("printf") == 0) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found printf\n";
#endif
	// Sanity check: source location of printf FunctionDecl is not in the
	// main program
	SourceLocation SL = SM->getExpansionLoc(calleeFD->getLocStart());
	if (!(SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL))) {
	    llvm::outs() << "ERROR: Function named printf has declaration in main "
			 << "program\n";
	    error = true;
	    return false;
	}

	//isPrintf = true;
	CallExprDetails* printfDetails = CallExprDetails::getObject(SM, E,
	    currBlock, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot get CallExprDetails from printf\n";
	    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
	    error = true;
	    return false;
	}

	bool isWhitespaceOutput = true;
	for (std::vector<ExprDetails*>::iterator argIt =
		printfDetails->callArgExprs.begin(); argIt != 
		printfDetails->callArgExprs.end(); argIt++) {
	    std::string outputString =
		Helper::prettyPrintExpr((*argIt)->origExpr);
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Original output string: " << outputString 
			 << " end: " << outputString.size() << "\n";
#endif
	    if (isa<StringLiteralDetails>(*argIt)) {
		StringLiteralDetails* str =
		    dyn_cast<StringLiteralDetails>(*argIt);
		StringRef outputStringRef = str->val.trim();
		outputString = outputStringRef.str();
	    } else {
		outputString = trim(outputString);
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Trimmed output string: " << outputString
			 << " end: " << outputString.size() << "\n";
#endif
	    if (outputString.size() != 0) {
		isWhitespaceOutput = false;
		break;
	    }
	}

	if (!isWhitespaceOutput) {
	  if (!isOutputStmtRecorded(printfDetails)) {
	    StmtDetails* similarOutputStmt =
		getSimilarOutputStmt(printfDetails, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While obtaining similar "
			     << "OutputStmt\n";
		error = true;
		return false;
	    }
	    if (!similarOutputStmt) 
		outputStmts.push_back(printfDetails);
	    else {
		if (!isa<CallExprDetails>(similarOutputStmt)) {
		    llvm::outs() << "ERROR: Similar OutputStmt is not "
				 << "CallExprDetails\n";
		    error = true;
		    return false;
		}
		CallExprDetails* similarCallExpr =
		    dyn_cast<CallExprDetails>(similarOutputStmt);
		unsigned numArgs =
		    similarCallExpr->callArgExprs.size();
		for (unsigned i = numArgs; i <
			printfDetails->callArgExprs.size(); i++) {
		    similarCallExpr->callArgExprs.push_back(
			printfDetails->callArgExprs[i]);
		}
	    }
	  }

	  if (std::find(outputLinesFound.begin(), outputLinesFound.end(), callLine)
		== outputLinesFound.end())
	    outputLinesFound.push_back(callLine);

	  for (std::vector<ExprDetails*>::iterator aIt = 
		printfDetails->callArgExprs.begin(); aIt != 
		printfDetails->callArgExprs.end(); aIt++) {
	    if (!((*aIt)->origExpr)) {
		llvm::outs() << "ERROR: NULL arg for printf at line "
			     << callLine << "\n";
		error = true;
		return false;
	    }
	    argExprs.push_back((*aIt)->origExpr);
	  }
	}
    } else if (fName.compare("putchar") == 0) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found putchar\n";
#endif
	// Sanity check: source location of putchar FunctionDecl is not in the
	// main program
	SourceLocation SL = SM->getExpansionLoc(calleeFD->getLocStart());
	if (!(SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL))) {
	    llvm::outs() << "ERROR: Function named putchar has declaration in main "
			 << "program\n";
	    error = true;
	    return false;
	}

	//isPrintf = true;
	CallExprDetails* putcharDetails = CallExprDetails::getObject(SM, E,
	    currBlock, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot get CallExprDetails from putchar\n";
	    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
	    error = true;
	    return false;
	}

	bool isWhitespaceOutput = true;
	for (std::vector<ExprDetails*>::iterator argIt =
		putcharDetails->callArgExprs.begin(); argIt != 
		putcharDetails->callArgExprs.end(); argIt++) {
	    std::string outputString =
		Helper::prettyPrintExpr((*argIt)->origExpr);
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Original output string: " << outputString 
			 << " end: " << outputString.size() << "\n";
#endif
	    if (isa<StringLiteralDetails>(*argIt)) {
		StringLiteralDetails* str =
		    dyn_cast<StringLiteralDetails>(*argIt);
		StringRef outputStringRef = str->val.trim();
		outputString = outputStringRef.str();
	    } else {
		outputString = trim(outputString);
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Trimmed output string: " << outputString
			 << " end: " << outputString.size() << "\n";
#endif
	    if (outputString.size() != 0) {
		isWhitespaceOutput = false;
		break;
	    }
	}
	if (!isWhitespaceOutput) {
	  if (!isOutputStmtRecorded(putcharDetails)) {
	    StmtDetails* similarOutputStmt =
		getSimilarOutputStmt(putcharDetails, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While obtaining similar "
			     << "OutputStmt\n";
		error = true;
		return false;
	    }
	    if (!similarOutputStmt) 
		outputStmts.push_back(putcharDetails);
	    else {
		if (!isa<CallExprDetails>(similarOutputStmt)) {
		    llvm::outs() << "ERROR: Similar OutputStmt is not "
				 << "CallExprDetails\n";
		    error = true;
		    return false;
		}
		CallExprDetails* similarCallExpr =
		    dyn_cast<CallExprDetails>(similarOutputStmt);
		unsigned numArgs =
		    similarCallExpr->callArgExprs.size();
		for (unsigned i = numArgs; i <
			putcharDetails->callArgExprs.size(); i++) {
		    similarCallExpr->callArgExprs.push_back(
			putcharDetails->callArgExprs[i]);
		}
	    }
	  }

	  if (std::find(outputLinesFound.begin(), outputLinesFound.end(), callLine)
		== outputLinesFound.end())
	    outputLinesFound.push_back(callLine);

	  for (std::vector<ExprDetails*>::iterator aIt = 
		putcharDetails->callArgExprs.begin(); aIt != 
		putcharDetails->callArgExprs.end(); aIt++) {
	    if (!((*aIt)->origExpr)) {
		llvm::outs() << "ERROR: NULL arg for putchar at line "
			     << callLine << "\n";
		error = true;
		return false;
	    }
	    argExprs.push_back((*aIt)->origExpr);
	  }
	}
    } else if (fName.compare("puts") == 0) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found puts\n";
#endif
	// Sanity check: source location of putchar FunctionDecl is not in the
	// main program
	SourceLocation SL = SM->getExpansionLoc(calleeFD->getLocStart());
	if (!(SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL))) {
	    llvm::outs() << "ERROR: Function named putchar has declaration in main "
			 << "program\n";
	    error = true;
	    return false;
	}

	//isPrintf = true;
	CallExprDetails* putcharDetails = CallExprDetails::getObject(SM, E,
	    currBlock, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot get CallExprDetails from putchar\n";
	    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
	    error = true;
	    return false;
	}

	bool isWhitespaceOutput = true;
	for (std::vector<ExprDetails*>::iterator argIt =
		putcharDetails->callArgExprs.begin(); argIt != 
		putcharDetails->callArgExprs.end(); argIt++) {
	    std::string outputString =
		Helper::prettyPrintExpr((*argIt)->origExpr);
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Original output string: " << outputString 
			 << " end: " << outputString.size() << "\n";
#endif
	    if (isa<StringLiteralDetails>(*argIt)) {
		StringLiteralDetails* str =
		    dyn_cast<StringLiteralDetails>(*argIt);
		StringRef outputStringRef = str->val.trim();
		outputString = outputStringRef.str();
	    } else {
		outputString = trim(outputString);
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Trimmed output string: " << outputString
			 << " end: " << outputString.size() << "\n";
#endif
	    if (outputString.size() != 0) {
		isWhitespaceOutput = false;
		break;
	    }
	}

	if (!isWhitespaceOutput) {
	  if (!isOutputStmtRecorded(putcharDetails)) {
	    StmtDetails* similarOutputStmt =
		getSimilarOutputStmt(putcharDetails, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While obtaining similar "
			     << "OutputStmt\n";
		error = true;
		return false;
	    }
	    if (!similarOutputStmt) 
		outputStmts.push_back(putcharDetails);
	    else {
		if (!isa<CallExprDetails>(similarOutputStmt)) {
		    llvm::outs() << "ERROR: Similar OutputStmt is not "
				 << "CallExprDetails\n";
		    error = true;
		    return false;
		}
		CallExprDetails* similarCallExpr =
		    dyn_cast<CallExprDetails>(similarOutputStmt);
		unsigned numArgs =
		    similarCallExpr->callArgExprs.size();
		for (unsigned i = numArgs; i <
			putcharDetails->callArgExprs.size(); i++) {
		    similarCallExpr->callArgExprs.push_back(
			putcharDetails->callArgExprs[i]);
		}
	    }
	  }

	  if (std::find(outputLinesFound.begin(), outputLinesFound.end(), callLine)
		== outputLinesFound.end())
	    outputLinesFound.push_back(callLine);

	  for (std::vector<ExprDetails*>::iterator aIt = 
		putcharDetails->callArgExprs.begin(); aIt != 
		putcharDetails->callArgExprs.end(); aIt++) {
	    if (!((*aIt)->origExpr)) {
		llvm::outs() << "ERROR: NULL arg for putchar at line "
			     << callLine << "\n";
		error = true;
		return false;
	    }
	    argExprs.push_back((*aIt)->origExpr);
	  }
	}
    } else if (std::find(customInputFuncs.begin(), customInputFuncs.end(), fName) != 
	    customInputFuncs.end()) {
	// Sanity check: source location of custominput FunctionDecl is in the
	// main program
	SourceLocation SL = SM->getExpansionLoc(calleeFD->getLocStart());
	if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	    llvm::outs() << "ERROR: Custom input function named " << fName 
			 << " has declaration outside main program\n";
	    error = true;
	    return false;
	}
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found custom input function\n";
#endif
	isCustomInput = true;

	// If callexpr does not have any arguments, then the input function
	// returns the input value and we need to match the assignment statement
	// pattern
	if (E->getNumArgs() == 0)
	    return true;

	CallExprDetails* customCallDetails = CallExprDetails::getObject(SM, E,
	    currBlock, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot get CallExprDetails of custom input "
			 << "function " << Helper::prettyPrintExpr(E) << "\n";
	    error = true;
	    return false;
	}

	if (!isInputStmtRecorded(customCallDetails)) {
	    //inputStmts.push_back(customCallDetails);
	    StmtDetails* similarInputStmt =
		getSimilarInputStmt(customCallDetails, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While obtaining similar "
			     << "InputStmt\n";
		error = true;
		return false;
	    }
	    if (!similarInputStmt) 
		inputStmts.push_back(customCallDetails);
	    else {
		if (!isa<CallExprDetails>(similarInputStmt)) {
		    llvm::outs() << "ERROR: Similar InputStmt is not "
				 << "CallExprDetails\n";
		    error = true;
		    return false;
		}
		CallExprDetails* similarCallExpr =
		    dyn_cast<CallExprDetails>(similarInputStmt);
		unsigned numArgs =
		    similarCallExpr->callArgExprs.size();
		for (unsigned i = numArgs; i <
			customCallDetails->callArgExprs.size(); i++) {
		    similarCallExpr->callArgExprs.push_back(
			customCallDetails->callArgExprs[i]);
		}
	    }
	}

	if (std::find(inputLinesFound.begin(), inputLinesFound.end(), callLine)
		== inputLinesFound.end())
	    inputLinesFound.push_back(callLine);
#if 0
	for (CallExpr::arg_iterator aIt = E->arg_begin(); aIt != E->arg_end();
		aIt = aIt + 1) {
	    Expr* arg = *aIt;
	    argExprs.push_back(arg);
	}
#endif
	for (std::vector<ExprDetails*>::iterator aIt =
		customCallDetails->callArgExprs.begin(); aIt != 
		customCallDetails->callArgExprs.end(); aIt++) {
	    if (!((*aIt)->origExpr)) {
		llvm::outs() << "ERROR: NULL arg for custom input function at "
			     << "line " << callLine << "\n";
		error = true;
		return false;
	    }
	    argExprs.push_back((*aIt)->origExpr);
	}
    } else if (std::find(customOutputFuncs.begin(), customOutputFuncs.end(), fName) != 
	    customOutputFuncs.end()) {
	// Sanity check: source location of customoutput FunctionDecl is in the
	// main program
	SourceLocation SL = SM->getExpansionLoc(calleeFD->getLocStart());
	if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	    llvm::outs() << "ERROR: Custom output function named " << fName 
			 << " has declaration outside main program\n";
	    error = true;
	    return false;
	}
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found custom output function\n";
#endif
	//isCustomOutput = true;

	// If the call expr does not have any arguments, then there is something
	// wrong.
	if (E->getNumArgs() == 0) {
	    llvm::outs() << "ERROR: No arguments for custom output function\n";
	    error = true;
	    return false;
	}

	CallExprDetails* customCallDetails = CallExprDetails::getObject(SM, E,
	    currBlock, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot get CallExprDetails of custom output "
			 << "function " << Helper::prettyPrintExpr(E) << "\n";
	    error = true;
	    return false;
	}
	bool isWhitespaceOutput = true;
	for (std::vector<ExprDetails*>::iterator argIt =
		customCallDetails->callArgExprs.begin(); argIt != 
		customCallDetails->callArgExprs.end(); argIt++) {
	    std::string outputString =
		Helper::prettyPrintExpr((*argIt)->origExpr);
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Original output string: " << outputString 
			 << " end: " << outputString.size() << "\n";
#endif
	    if (isa<StringLiteralDetails>(*argIt)) {
		StringLiteralDetails* str =
		    dyn_cast<StringLiteralDetails>(*argIt);
		StringRef outputStringRef = str->val.trim();
		outputString = outputStringRef.str();
	    } else {
		outputString = trim(outputString);
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Trimmed output string: " << outputString
			 << " end: " << outputString.size() << "\n";
#endif
	    if (outputString.size() != 0) {
		isWhitespaceOutput = false;
		break;
	    }
	}

	if (!isWhitespaceOutput) {
	  if (!isOutputStmtRecorded(customCallDetails)) {
	    StmtDetails* similarOutputStmt =
		getSimilarOutputStmt(customCallDetails, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While obtaining similar "
			     << "OutputStmt\n";
		error = true;
		return false;
	    }
	    if (!similarOutputStmt) 
		outputStmts.push_back(customCallDetails);
	    else {
		if (!isa<CallExprDetails>(similarOutputStmt)) {
		    llvm::outs() << "ERROR: Similar OutputStmt is not "
				 << "CallExprDetails\n";
		    error = true;
		    return false;
		}
		CallExprDetails* similarCallExpr =
		    dyn_cast<CallExprDetails>(similarOutputStmt);
		unsigned numArgs =
		    similarCallExpr->callArgExprs.size();
		for (unsigned i = numArgs; i <
			customCallDetails->callArgExprs.size(); i++) {
		    similarCallExpr->callArgExprs.push_back(
			customCallDetails->callArgExprs[i]);
		}
	    }
	  }

	  if (std::find(outputLinesFound.begin(), outputLinesFound.end(), callLine)
		== outputLinesFound.end())
	    outputLinesFound.push_back(callLine);

#if 0
	for (CallExpr::arg_iterator aIt = E->arg_begin(); aIt != E->arg_end();
		aIt = aIt + 1) {
	    Expr* arg = *aIt;
	    argExprs.push_back(arg);
	}
#endif
	  for (std::vector<ExprDetails*>::iterator aIt =
		customCallDetails->callArgExprs.begin(); aIt != 
		customCallDetails->callArgExprs.end(); aIt++) {
	    if (!((*aIt)->origExpr)) {
		llvm::outs() << "ERROR: NULL arg for custom output function at "
			     << "line " << callLine << "\n";
		error = true;
		return false;
	    }
	    argExprs.push_back((*aIt)->origExpr);
	  }
	}
    } else {
#ifdef DEBUG
	llvm::outs() << "DEBUG: catchall branch\n";
#endif
	// The call Expr is none of the input/output functions. We need to start
	// exploring this function
	// Sanity check: source location of FunctionDecl is in the
	// main program
	SourceLocation SL = SM->getExpansionLoc(calleeFD->getLocStart());
	if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	    // If this call expr has decl outside main program, it might be one
	    // of the library functions. Skip this
	    return true;
	}

	// Get Function Details of this function
	FunctionDetails* calleeFunction = Helper::getFunctionDetailsFromDecl(SM,
	    calleeFD, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot get FunctionDetails of " << fName << "\n";
	    error = true;
	    return false;
	}
#if 0
	calleeFunction->funcName = fName;
	calleeFunction->funcStartLine =
	    SM->getExpansionLineNumber(calleeFD->getLocStart());
	calleeFunction->funcEndLine =
	    SM->getExpansionLineNumber(calleeFD->getLocEnd());
	calleeFunction->fd = calleeFD;
#endif
#ifdef DEBUG
	llvm::outs() << "DEBUG: mObj->functions: " << mObj->functions.size() << "\n";
#endif
	bool foundFunc = false;
	FunctionDetails* calleeFuncObj = NULL;
	for (std::vector<FunctionDetails*>::iterator fIt =
		mObj->functions.begin(); fIt != mObj->functions.end(); fIt++) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Looking at function: ";
	    (*fIt)->print();
	    llvm::outs() << "\n";
#endif
	    if ((*fIt)->equals(calleeFunction)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Found entry for function "
			     << calleeFunction->funcName << " in mObj\n";
#endif
		foundFunc = true;
		calleeFuncObj = (*fIt);
		break;
	    }
	}

	if (!foundFunc) {
	    llvm::outs() << "ERROR: Cannot get FunctionDetails of "
			 << calleeFunction->funcName << " from mObj\n";
	    error = true;
	    return false;
	}

	// Recursively call InputOutputStmtVisitor on this function
	InputOutputStmtVisitor ioVisitor(this);
	ioVisitor.setCurrFunction(calleeFuncObj);

	MainFunction::runVisitorOnInlinedProgram(calleeFuncObj->cfg, &ioVisitor,
	    rv);
	if (ioVisitor.error) {
	    llvm::outs() << "ERROR: While running InputOutputStmtVisitor on "
			 << "inlined function " << calleeFunction->funcName << "\n";
	    error = true;
	    return false;
	}

	inputLinesFound.insert(inputLinesFound.end(),
	    ioVisitor.inputLinesFound.begin(), ioVisitor.inputLinesFound.end());
	outputLinesFound.insert(outputLinesFound.end(),
	    ioVisitor.outputLinesFound.begin(), 
	    ioVisitor.outputLinesFound.end());
	inputVarsFound.insert(inputVarsFound.end(),
	    ioVisitor.inputVarsFound.begin(), ioVisitor.inputVarsFound.end());
#ifdef DEBUG
    llvm::outs() << "DEBUG: inputStmts before visit:\n";
    for (std::vector<StmtDetails*>::iterator it = inputStmts.begin(); it != 
	    inputStmts.end(); it++) {
	if ((*it)->origStmt)
	    llvm::outs() << Helper::prettyPrintStmt((*it)->origStmt) << "\n";
    }
#endif
	inputStmts.insert(inputStmts.end(),
	    ioVisitor.inputStmts.begin(), ioVisitor.inputStmts.end());
#ifdef DEBUG
    llvm::outs() << "DEBUG: inputStmts after visit:\n";
    for (std::vector<StmtDetails*>::iterator it = inputStmts.begin(); it != 
	    inputStmts.end(); it++) {
	if ((*it)->origStmt)
	    llvm::outs() << Helper::prettyPrintStmt((*it)->origStmt) << "\n";
    }
#endif
	outputStmts.insert(outputStmts.end(),
	    ioVisitor.outputStmts.begin(), ioVisitor.outputStmts.end());

	return true;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Reached here\n";
#endif
    if (isCin || isScanf || isFScanf || isCustomInput) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: isCin || isScanf || isCustomInput\n";
#endif
	// Find details of input vars
	std::vector<Expr*>::iterator it;
	// It this is a scanf call, then ignore the first argument (format string).
	if (isScanf)
	    it = argExprs.begin() + 1;
	else if (isFScanf)
	    it = argExprs.begin() + 2;
	else
	    it = argExprs.begin();
	for (; it != argExprs.end(); it++) {
	    InputVar v = getInputVarFromExpr(SM, *it, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot get InputVar from Expr: "
			     << Helper::prettyPrintExpr(*it) << "\n";
		error = true;
		return false;
	    }
	    v.varExpr = *it;
	    v.inputCallLine = callLine;
	    v.func = currFunction->cloneAsPtr();
	    if (isCin)
		v.inputFunction = InputVar::InputFunc::CIN;
	    else if (isScanf)
		v.inputFunction = InputVar::InputFunc::SCANF;
	    else if (isFScanf)
		v.inputFunction = InputVar::InputFunc::FSCANF;
	    else if (isCustomInput)
		v.inputFunction = InputVar::InputFunc::CUSTOM;
	    else {
		llvm::outs() << "ERROR: Cannot resolve input function: not CIN, "
			     << "SCANF, FSCANF or CUSTOM\n";
		error = true;
		return false;
	    }

	    std::stringstream s;
	    // Set function ID
	    if (inputVarsFound.size() == 0) {
		s << currFunction->funcName << "_1";
#ifdef DEBUG
		llvm::outs() << "DEBUG: Var ID: " << s.str() << "\n";
#endif
	    } else {
		InputVar lastVar = *(inputVarsFound.rbegin());
#ifdef DEBUG
		llvm::outs() << "DEBUG: last inputvar\n";
		lastVar.print();
#endif
		int idPos = lastVar.funcID.find_last_of('_');
		std::string lastIDStr = lastVar.funcID.substr(idPos+1);
#ifdef DEBUG
		llvm::outs() << "DEBUG: lastIDStr: " << lastIDStr << "\n";
#endif
		int lastID = std::stoi(lastIDStr);
		s << currFunction->funcName << "_" << lastID+1;
#ifdef DEBUG
		llvm::outs() << "DEBUG: Var ID: " << s.str() << "\n";
#endif
	    }
	    v.funcID = s.str();
#ifdef DEBUG
	    llvm::outs() << "DEBUG: CallExprsVisited: " << callExprsVisited.size() << "\n";
	    for (std::vector<CallExprDetails*>::iterator cIt =
		    callExprsVisited.begin(); cIt != callExprsVisited.end();
		    cIt++) {
		(*cIt)->print();
	    }
	    llvm::outs() << "DEBUG: Current call:\n";
	    cDetails->print();
	    llvm::outs() << "DEBUG: inputVarsFound:\n";
	    for (std::vector<InputVar>::iterator iIt = inputVarsFound.begin();
		    iIt != inputVarsFound.end(); iIt++) {
		iIt->print();
		llvm::outs() << "\n";
	    }
	    llvm::outs() << "DEBUG: Current Var: \n";
	    v.print();
	    llvm::outs() << "\n";
#endif
	    // Check if the same var is read at multiple locations.
	    // TODO: What do we do in this case, instead of failing?
	    bool duplicate = false;
	    for (std::vector<InputVar>::iterator iIt = inputVarsFound.begin();
		    iIt != inputVarsFound.end(); iIt++) {
		if (iIt->equals(v) && iIt->inputCallLine != v.inputCallLine) {
		    
		    llvm::outs() << "ERROR: Found same var being read at "
				 << "multiple locations1:\n";
		    llvm::outs() << "1: ";
		    iIt->print();
		    llvm::outs() << "\n2: ";
		    v.print();
		    llvm::outs() << "\n";
		    error = true;
		    return false;
		} else if (iIt->equals(v) && iIt->inputCallLine ==
		    v.inputCallLine) {
		    duplicate = true;
		    break;
		}
	    }
	    if (!duplicate) {
		inputVarsFound.push_back(v);
#ifdef DEBUG
		llvm::outs() << "DEBUG: InputVar: ";
		v.print();
		llvm::outs() << "\n";
#endif
	    }
	}
    }

    return true;
}

bool InputOutputStmtVisitor::VisitBinaryOperator(BinaryOperator* B) {
    SourceLocation SL = SM->getExpansionLoc(B->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: BinaryOperator not in main program\n";
	error = true;
	return false;
    }

    int callLine = SM->getExpansionLineNumber(B->getLocStart());

    // Skip if this is not an assignment op.
    if (!B->isAssignmentOp()) return true;

    // Check if the rhs is a call expr.
    bool isGetChar = false;
    Expr* rhsExpr = B->getRHS()->IgnoreParenCasts();
    if (!isa<CallExpr>(rhsExpr)) {
	// Check if rhs is getchar() - '0'
	if (!isa<BinaryOperator>(rhsExpr)) return true;

	BinaryOperator* rhsBO = dyn_cast<BinaryOperator>(rhsExpr);
	if (rhsBO->getOpcode() != BinaryOperatorKind::BO_Sub) return true;

	Expr* lhs = rhsBO->getLHS()->IgnoreParenCasts();
	Expr* rhs = rhsBO->getRHS()->IgnoreParenCasts();
	if (isa<CallExpr>(lhs) && isa<CharacterLiteral>(rhs)) {
	    CallExpr* lhsCall = dyn_cast<CallExpr>(lhs);
	    if (lhsCall->getDirectCallee()->getQualifiedNameAsString().compare("getchar")
		    != 0)
		return true;
	    CharacterLiteral* rhsLit = dyn_cast<CharacterLiteral>(rhs);
	    if (rhsLit->getValue() != '0') return true;
	    isGetChar = true;
	} else  {
	    return true;
	}
    } else {

	CallExpr* rhsCallExpr = dyn_cast<CallExpr>(rhsExpr);

	FunctionDecl* calleeFD = rhsCallExpr->getDirectCallee();
	if (!calleeFD) {
	    llvm::outs() << "ERROR: Cannot get callee FD from callExpr\n";
	    error = true;
	    return false;
	}

	DeclarationNameInfo fInfo = calleeFD->getNameInfo();
	std::string fName = fInfo.getAsString();

#ifdef DEBUG
	llvm::outs() << "DEBUG: Visiting assignment with " << fName << " at line " 
		     << callLine << "\n";
#endif

	// If this call is not to one of the custom input functions, skip.
	if (std::find(customInputFuncs.begin(), customInputFuncs.end(), fName) == 
		customInputFuncs.end() && fName.compare("getchar") != 0)
	    return true;

	if (fName.compare("getchar") == 0)
	    isGetChar = true;
    
	// This pattern is supposed to match those custom input functions that do
	// not take any arguments, but returns the input value read. So the input
	// var in this pattern is the lhs of the assignment. So check if this
	// function takes any argument.
	if (rhsCallExpr->getNumArgs() > 0 || calleeFD->param_size() > 0) {
	    llvm::outs() << "ERROR: Custom input function call at line " << callLine 
			 << " both takes an argument and returns a value\n";
	    error = true;
	    return false;
	}
    }

    bool rv;
    BinaryOperatorDetails* customInputDetails =
	BinaryOperatorDetails::getObject(SM, B, currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain BinaryOperatorDetails of custom "
		     << "input function " << Helper::prettyPrintExpr(B) << "\n";
	error = true;
	return false;
    }

    if (!isInputStmtRecorded(customInputDetails)) {
	inputStmts.push_back(customInputDetails);
    }

    // Return LHS as the input var.
    Expr* lhsExpr = B->getLHS();
    InputVar v = getInputVarFromExpr(SM, lhsExpr, rv);
    if (!rv) {
	error = true;
	return false;
    }

    v.varExpr = lhsExpr;
    v.inputCallLine = callLine;
    if (isGetChar)
	v.inputFunction = InputVar::InputFunc::GETCHAR;
    else
	v.inputFunction = InputVar::InputFunc::CUSTOM;
    v.func = currFunction->cloneAsPtr();

    std::stringstream s;
    // Set function ID
    if (inputVarsFound.size() == 0) {
	s << currFunction->funcName << "_1";
#ifdef DEBUG
	llvm::outs() << "DEBUG: Var ID: " << s.str() << "\n";
#endif
    } else {
	InputVar lastVar = *(inputVarsFound.rbegin());
#ifdef DEBUG
	llvm::outs() << "DEBUG: last inputvar\n";
	lastVar.print();
#endif
	int idPos = lastVar.funcID.find_last_of('_');
	std::string lastIDStr = lastVar.funcID.substr(idPos+1);
#ifdef DEBUG
	llvm::outs() << "DEBUG: lastIDStr: " << lastIDStr << "\n";
#endif
	int lastID = std::stoi(lastIDStr);
	s << currFunction->funcName << "_" << lastID+1;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Var ID: " << s.str() << "\n";
#endif
    }
    v.funcID = s.str();
    // Check if the same var is read at multiple locations.
    // TODO: What do we do in this case, instead of failing?
    bool duplicate = false;
    for (std::vector<InputVar>::iterator iIt = inputVarsFound.begin();
	    iIt != inputVarsFound.end(); iIt++) {
	if (iIt->equals(v) && iIt->inputCallLine != v.inputCallLine) {
	    llvm::outs() << "ERROR: Found same var being read at "
			 << "multiple locations2:\n";
	    llvm::outs() << "1: ";
	    iIt->print();
	    llvm::outs() << "\n2: ";
	    v.print();
	    llvm::outs() << "\n";
	    error = true;
	    return false;
	} else if (iIt->equals(v) && iIt->inputCallLine == v.inputCallLine) {
	    duplicate = true;
	    break;
	}
    }
    if (!duplicate) {
	inputVarsFound.push_back(v);
	if (std::find(inputLinesFound.begin(), inputLinesFound.end(), callLine)
		== inputLinesFound.end()) 
	    inputLinesFound.push_back(callLine);
    }

    return true;
}

bool InputOutputStmtVisitor::VisitDeclStmt(DeclStmt* D) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: InputOutputStmtVisitor::VisitDeclStmt()\n";
#endif
    SourceLocation SL = SM->getExpansionLoc(D->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: DeclStmt not in main program\n";
	error = true;
	return false;
    }

    int callLine = SM->getExpansionLineNumber(D->getLocStart());

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting DeclStmt at " << callLine << "\n";
#endif

    bool rv;
    DeclStmtDetails* dsd = DeclStmtDetails::getObject(SM, D, currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get DeclStmtDetails\n";
	error = true;
	return false;
    }

    if (!dsd->vdd) {
	llvm::outs() << "ERROR: No VarDecl in Declstmt\n";
	error = true;
	return false;
    }

    // Skip if this is not an assignment op.
    if (!(dsd->vdd->initExpr)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: VarDecl has no init\n";
#endif
	return true;
    }

    // Check if the rhs is a call expr.
    //Expr* initExpr = V->getInit();
    if (!isa<CallExpr>(dsd->vdd->initExpr->origExpr)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: VarDecl init is not CallExpr\n";
#endif
	return true;
    }

    CallExpr* initCallExpr = dyn_cast<CallExpr>(dsd->vdd->initExpr->origExpr);

    FunctionDecl* calleeFD = initCallExpr->getDirectCallee();
    if (!calleeFD) {
	llvm::outs() << "ERROR: Cannot get callee FD from callExpr\n";
	error = true;
	return false;
    }

    DeclarationNameInfo fInfo = calleeFD->getNameInfo();
    std::string fName = fInfo.getAsString();

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting assignment with " << fName << " at line " 
		 << callLine << "\n";
#endif

    // If this call is not to one of the custom input functions, skip.
    if (std::find(customInputFuncs.begin(), customInputFuncs.end(), fName) == 
	    customInputFuncs.end()) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: VarDecl init does not have custom input func\n";
#endif
	return true;
    }
    
    // This pattern is supposed to match those custom input functions that do
    // not take any arguments, but returns the input value read. So the input
    // var in this pattern is the lhs of the assignment. So check if this
    // function takes any argument.
    if (initCallExpr->getNumArgs() > 0 || calleeFD->param_size() > 0) {
	llvm::outs() << "ERROR: Custom input function call at line " << callLine 
		     << " both takes an argument and returns a value\n";
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: vardecldetails\n";
    dsd->vdd->print();
#endif

    if (!isInputStmtRecorded(dsd)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Inserted inputstmt\n";
#endif
	inputStmts.push_back(dsd);
#ifdef DEBUG
    llvm::outs() << "DEBUG: after inserting, inputStmts:\n";
    for (std::vector<StmtDetails*>::iterator it = inputStmts.begin(); it != 
	    inputStmts.end(); it++) {
	llvm::outs() << "Line: " << (*it)->lineNum << ":\n";
	if ((*it)->origStmt)
	    llvm::outs() << Helper::prettyPrintStmt((*it)->origStmt) << "\n";
    }
#endif

    }

    InputVar v;
    v.var = dsd->vdd->var;
    v.varExpr = NULL;
    v.inputCallLine = callLine;
    v.inputFunction = InputVar::InputFunc::CUSTOM;
    v.func = currFunction->cloneAsPtr();

    std::stringstream s;
    // Set function ID
    if (inputVarsFound.size() == 0) {
	s << currFunction->funcName << "_1";
#ifdef DEBUG
	llvm::outs() << "DEBUG: Var ID: " << s.str() << "\n";
#endif
    } else {
	InputVar lastVar = *(inputVarsFound.rbegin());
#ifdef DEBUG
	llvm::outs() << "DEBUG: last inputvar\n";
	lastVar.print();
#endif
	int idPos = lastVar.funcID.find_last_of('_');
	std::string lastIDStr = lastVar.funcID.substr(idPos+1);
#ifdef DEBUG
	llvm::outs() << "DEBUG: lastIDStr: " << lastIDStr << "\n";
#endif
	int lastID = std::stoi(lastIDStr);
	s << currFunction->funcName << "_" << lastID+1;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Var ID: " << s.str() << "\n";
#endif
    }
    v.funcID = s.str();
    // Check if the same var is read at multiple locations.
    // TODO: What do we do in this case, instead of failing?
    bool duplicate = false;
    for (std::vector<InputVar>::iterator iIt = inputVarsFound.begin();
	    iIt != inputVarsFound.end(); iIt++) {
	if (iIt->equals(v) && iIt->inputCallLine != v.inputCallLine) {
	    llvm::outs() << "ERROR: Found same var being read at "
			 << "multiple locations3:\n";
	    llvm::outs() << "1: ";
	    iIt->print();
	    llvm::outs() << "\n2: ";
	    v.print();
	    llvm::outs() << "\n";
	    error = true;
	    return false;
	} else if (iIt->equals(v) && iIt->inputCallLine == v.inputCallLine) {
	    duplicate = true;
	    break;
	}
    }
    if (!duplicate) {
	inputVarsFound.push_back(v);
	if (std::find(inputLinesFound.begin(), inputLinesFound.end(), callLine)
		== inputLinesFound.end()) 
	    inputLinesFound.push_back(callLine);
    }

    return true;
}

bool InitUpdateStmtVisitor::VisitCallExpr(CallExpr* C) {
    SourceLocation SL = SM->getExpansionLoc(C->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: CallExpr is not in main program\n";
	error = true;
	return false;
    }

    bool rv;

    FunctionDecl* calleeFD = C->getDirectCallee();
    if (!calleeFD) {
	llvm::outs() << "ERROR: Cannot get FunctionDecl of callee from "
		     << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }
    calleeFD = calleeFD->getMostRecentDecl();
    if (!calleeFD) {
	llvm::outs() << "ERROR: Cannot get most recent decl of callee: "
		     << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }

    // If it is memset, add it to initStmts
    std::string calleeName = calleeFD->getNameInfo().getAsString();
    if (calleeName.compare("memset") == 0) {
	CallExprDetails* ced = CallExprDetails::getObject(SM, C, currBlock, rv,
	    currFunction);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot get CallExprDetails from CallExpr\n";
	    error = true;
	    return false;
	}

	if (!(symVisitor->isResolved(ced))) {
	    llvm::outs() << "ERROR: memset call is not resolved\n";
	    error = true;
	    return false;
	}

	initLinesFound.push_back(SM->getExpansionLineNumber(C->getLocStart()));
	initStmts.push_back(ced);

	return false;
    }

    // Sanity check: source location of FunctionDecl is in the
    // main program
    SL = SM->getExpansionLoc(calleeFD->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	// If this call expr has decl outside main program, it might be one
	// of the library functions. Skip this
	return true;
    }

    // Sanity check: This is not a custom input function or a custom output
    // function
    std::string fName = calleeFD->getQualifiedNameAsString();
    if (std::find(customInputFuncs.begin(), customInputFuncs.end(), fName) != 
	    customInputFuncs.end()) {
	// This is a custom input function. Skip
	return true;
    }
    if (std::find(customOutputFuncs.begin(), customOutputFuncs.end(), fName) != 
	    customOutputFuncs.end()) {
	// This is a custom output function. Skip
	return true;
    }

    // Get Function Details of this function
    FunctionDetails* calleeFunction = Helper::getFunctionDetailsFromDecl(SM,
	calleeFD, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get FunctionDetails of " << fName << "\n";
	error = true;
	return false;
    }
    for (std::vector<FunctionDetails*>::iterator fIt =
	    mObj->functions.begin(); fIt != mObj->functions.end(); fIt++) {
	if ((*fIt)->equals(calleeFunction)) {
	    calleeFunction = *fIt;
	    break;
	}
    }

    GetSymbolicExprVisitor* funcSym = NULL;
    for (std::vector<FunctionAnalysis*>::iterator aIt = 
	    mObj->analysisInfo.begin(); aIt != mObj->analysisInfo.end(); aIt++) {
	if ((*aIt)->function->equals(calleeFunction)) {
	    funcSym = (*aIt)->symVisitor;
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Found analysis object of function: " 
			 << (*aIt)->function->funcName << "\n";
#endif
	    break;
	}
    }

    if (!funcSym) {
	llvm::outs() << "ERROR: NULL GetSymbolicExprVisitor for callee\n";
	error = true;
	return false;
    }

    // Recursively call InitUpdateStmtVisitor on this function
    InitUpdateStmtVisitor iuVisitor(this, funcSym);
    iuVisitor.setCurrFunction(calleeFunction);

#if 0
    std::unique_ptr<CFG> calleeCFG = calleeFunction.constructCFG(rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot construct CFG of function:\n";
	calleeFunction.print();
	error = true;
	return false;
    }
#endif
    MainFunction::runVisitorOnInlinedProgram(calleeFunction->cfg, &iuVisitor,
	rv);
    if (iuVisitor.error) {
	llvm::outs() << "ERROR: While running InitUpdateStmtVisitor on "
		     << "inlined function " << calleeFunction->funcName << "\n";
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Returning from InitUpdateStmtVisitor on inlined function "
		 << calleeFunction->funcName << "\n";
    llvm::outs() << "DEBUG: Init Stmts found in function: " 
		 << iuVisitor.initStmts.size() << "\n";
    llvm::outs() << "DEBUG: Updt Stmts found in function: " 
		 << iuVisitor.updateStmts.size() << "\n";
#endif

    CallExprDetails* ced = CallExprDetails::getObject(SM, C, currBlock, rv,
	currFunction);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get CallExprDetails from CallExpr\n";
	error = true;
	return false;
    }
    for (std::vector<int>::iterator iIt = iuVisitor.initLinesFound.begin(); iIt != 
	    iuVisitor.initLinesFound.end(); iIt++) {
	if (std::find(initLinesFound.begin(), initLinesFound.end(), *iIt) ==
		initLinesFound.end())
	    initLinesFound.push_back(*iIt);
    }
    for (std::vector<int>::iterator iIt = iuVisitor.updateLinesFound.begin(); iIt != 
	    iuVisitor.updateLinesFound.end(); iIt++) {
	if (std::find(updateLinesFound.begin(), updateLinesFound.end(), *iIt) ==
		updateLinesFound.end())
	    updateLinesFound.push_back(*iIt);
    }

#if 0
    for (std::vector<StmtDetails*>::iterator iIt = iuVisitor.initStmts.begin(); iIt != 
	    iuVisitor.initStmts.end(); iIt++) {
	bool initExists = false;
	for (std::vector<StmtDetails*>::iterator inIt =
		initStmts.begin(); inIt != initStmts.end();
		inIt++) {
	    if ((*inIt)->equals(*iIt)) {
		initExists = true;
		break;
	    }
	}
	if (!initExists) initStmts.push_back(*iIt);

    }
#endif
    bool foundCallExpr = false;
    for (std::vector<std::pair<CallExprDetails*, std::vector<StmtDetails*> >
	    >::iterator cIt = callExprToInitStmts.begin(); cIt != 
	    callExprToInitStmts.end(); cIt++) {
	if (cIt->first->equals(ced)) {
	    foundCallExpr = true;
	    cIt->second.insert(cIt->second.end(), iuVisitor.initStmts.begin(),
		iuVisitor.initStmts.end());
	    break;
	}
    }
    if (!foundCallExpr) {
	std::pair<CallExprDetails*, std::vector<StmtDetails*> > p;
	p.first = ced;
	p.second = iuVisitor.initStmts;
	callExprToInitStmts.push_back(p);
    }

#if 0
    for (std::vector<StmtDetails*>::iterator iIt = iuVisitor.updateStmts.begin(); iIt != 
	    iuVisitor.updateStmts.end(); iIt++) {
	bool updtExists = false;
	for (std::vector<StmtDetails*>::iterator uIt = updateStmts.begin(); uIt
		!= updateStmts.end(); uIt++) {
	    if ((*uIt)->equals(*iIt)) {
		updtExists = true;
		break;
	    }
	}
	if (!updtExists) updateStmts.push_back(*iIt);
    }
#endif

    foundCallExpr = false;
    for (std::vector<std::pair<CallExprDetails*, std::vector<StmtDetails*> >
	    >::iterator cIt = callExprToUpdateStmts.begin(); cIt != 
	    callExprToUpdateStmts.end(); cIt++) {
	if (cIt->first->equals(ced)) {
	    foundCallExpr = true;
	    cIt->second.insert(cIt->second.end(), iuVisitor.initStmts.begin(),
		iuVisitor.updateStmts.end());
	    break;
	}
    }
    if (!foundCallExpr) {
	std::pair<CallExprDetails*, std::vector<StmtDetails*> > p;
	p.first = ced;
	p.second = iuVisitor.updateStmts;
	callExprToUpdateStmts.push_back(p);
    }
#ifdef DEBUG
    llvm::outs() << "DEBUG: Current total of initStmts: " << initStmts.size() 
		 << "\n";
    llvm::outs() << "DEBUG: Current total of updateStmts: " << updateStmts.size() 
		 << "\n";
#endif

    return true;
}

bool InitUpdateStmtVisitor::VisitBinaryOperator(BinaryOperator* B) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering InitUpdateStmtVisitor::VisitBinaryOperator()\n";
#endif
    SourceLocation SL = SM->getExpansionLoc(B->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: BinaryOperator is not in main program\n";
	error = true;
	return false;
    }

    if (!(B->isAssignmentOp())) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: BinaryOperator is not an assignment op. "
		     << "Skipping..\n";
#endif
	return true;
    }

    bool rv;
    BinaryOperatorDetails* currDetails = BinaryOperatorDetails::getObject(SM, B,
	currBlock, rv, currFunction);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get BinaryOperatorDetails from "
		     << "BinaryOperator: " << Helper::prettyPrintExpr(B) << "\n";
	error = true;
	return false;
    }

    if (!currDetails) {
	llvm::outs() << "ERROR: NULL BinaryOperatorDetails from "
		     << Helper::prettyPrintExpr(B) << "\n";
	error = true;
	return false;
    }

    if (!(currDetails->lhs)) {
	llvm::outs() << "ERROR: NULL lhs in BinaryOperatorDetails of "
		     << Helper::prettyPrintExpr(B) << "\n";
	error = true;
	return false;
    }
    if (!isa<ArraySubscriptExprDetails>(currDetails->lhs)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: LHS of BinaryOperator is not an array.Skipping "
		     << Helper::prettyPrintExpr(B) << "\n";
#endif
	return true;
    }

    // Check if this is an input line, then skip.
    for (std::vector<StmtDetails*>::iterator iIt = mObj->inputStmts.begin(); 
	    iIt != mObj->inputStmts.end(); iIt++) {
	if ((*iIt)->equals(currDetails)) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: BinaryOperator is an input stmt. Skipping "
			 << Helper::prettyPrintExpr(B) << "\n";
#endif
	    return false;
	}
    }


    ArraySubscriptExprDetails* lhsArrayDetails =
	dyn_cast<ArraySubscriptExprDetails>(currDetails->lhs);

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting assignment:\n";
    //currDetails->print();
    llvm::outs() << Helper::prettyPrintExpr(B) << " [" 
		 << SM->getExpansionLineNumber(B->getLocStart()) << "]\n";
#endif
    
    if (!(currDetails->rhs)) {
	llvm::outs() << "ERROR: NULL rhs in BinaryOperatorDetails of "
		     << Helper::prettyPrintExpr(B) << "\n";
	error = true;
	return false;
    }
    ExprDetails* rhsExpr = currDetails->rhs;
#ifdef DEBUG
    llvm::outs() << "DEBUG: Looking for symExprs of rhsExpr: ";
    rhsExpr->prettyPrint();
    llvm::outs() << "\n";
    symVisitor->printSymExprMap(true);
#endif

    // Check if the statement is an update statement. Get the symbolic
    // expressions for rhs. Check the varsReferenced in each SymExpr for the lhs
    // array.
    std::vector<SymbolicStmt*> rhsSymExprs =
	symVisitor->getSymbolicExprs(rhsExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: No symbolicExprs for rhsExpr:\n";
	rhsExpr->print();
	error = true;
	return false;
    }
#ifdef DEBUG
    llvm::outs() << "DEBUG: rhsSymExprs:\n";
    for (std::vector<SymbolicStmt*>::iterator rsIt = rhsSymExprs.begin();
	    rsIt != rhsSymExprs.end(); rsIt++) {
	if (isa<SymbolicExpr>(*rsIt))
	    dyn_cast<SymbolicExpr>(*rsIt)->prettyPrint(true);
	else
	    (*rsIt)->print();
    }
#endif

    // A rewrite required if the rhs is a call expr. We need to look at the
    // returnExpr instead of the actual call expr
    std::vector<SymbolicStmt*> tempRHSSymExprs;
    for (std::vector<SymbolicStmt*>::iterator rsIt = rhsSymExprs.begin(); rsIt
	    != rhsSymExprs.end(); rsIt++) {
	if (isa<SymbolicCallExpr>(*rsIt)) {
	    SymbolicCallExpr* sce = dyn_cast<SymbolicCallExpr>(*rsIt);
	    if (sce->returnExpr)
		tempRHSSymExprs.push_back(sce->returnExpr);
	    else
		tempRHSSymExprs.push_back(*rsIt);
	} else {
	    tempRHSSymExprs.push_back(*rsIt);
	}
    }

    rhsSymExprs = tempRHSSymExprs;

#ifdef DEBUG
    llvm::outs() << "DEBUG: rhsSymExprs after rewriting callExprs:\n";
    for (std::vector<SymbolicStmt*>::iterator rsIt = rhsSymExprs.begin();
	    rsIt != rhsSymExprs.end(); rsIt++) {
	if (isa<SymbolicExpr>(*rsIt))
	    dyn_cast<SymbolicExpr>(*rsIt)->prettyPrint(true);
	else
	    (*rsIt)->print();
    }
#endif

    int i = 0;
    bool foundInit = false;
    bool foundUpdate = false;
    bool foundOther = false;
    enum labelKind { INIT, UPDATE, OTHER};
    labelKind prevLabel, currLabel;
    for (std::vector<SymbolicStmt*>::iterator sIt = rhsSymExprs.begin();
	    sIt != rhsSymExprs.end(); sIt++) {
	if (sIt != rhsSymExprs.begin() && sIt != rhsSymExprs.begin()+1) {
	    if (prevLabel != currLabel) {
		llvm::outs() << "ERROR: Multiple labels for the same "
			     << "statement " 
			     << Helper::prettyPrintExpr(B) << "\n";
		llvm::outs() << "DEBUG: prevLabel: " << prevLabel 
			     << "currLabel: " << currLabel << "\n";
		error = true;
		return false;
	    }
	}
	if (sIt != rhsSymExprs.begin())
	    prevLabel = currLabel;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Looking at symexpr:\n";
	(*sIt)->prettyPrint(true);
#endif
	if (!isa<SymbolicExpr>(*sIt)) {
	    llvm::outs() << "ERROR: Symbolic expr for rhsExpr is not "
			 << "<SymbolicExpr>\n";
	    error = true;
	    return false;
	}

	if ((*sIt)->isConstantLiteral()) { 
#if 0
	    // If we already found another label, then stop
	    if (foundUpdate || foundOther) {
		llvm::outs() << "ERROR: Multiple labels for the same statement "
			     << Helper::prettyPrintExpr(B) << "\n";
		llvm::outs() << "foundUpdate || foundOther at init\n";
		error = true;
		return false;
	    }
#endif
	    foundInit = true;
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Found INIT\n";
#endif
	    currLabel = labelKind::INIT;
	} else if (isa<SymbolicDeclRefExpr>(*sIt) ||
		    isa<SymbolicArraySubscriptExpr>(*sIt)) {
	    // If there is substituteExpr, then check that instead of the
	    // DeclRefExpr
	    SymbolicDeclRefExpr* sdre;
	    if (isa<SymbolicDeclRefExpr>(*sIt))
		sdre = dyn_cast<SymbolicDeclRefExpr>(*sIt);
	    else {
		SymbolicArraySubscriptExpr* sase =
		    dyn_cast<SymbolicArraySubscriptExpr>(*sIt);
		sdre = sase->baseArray;
	    }
	    bool labelSet = false;
	    while (sdre->substituteExpr) {
		if (!isa<SymbolicDeclRefExpr>(sdre->substituteExpr) && 
		    !isa<SymbolicArraySubscriptExpr>(sdre->substituteExpr)) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: substitute expr of "
				 << "SymbolicDeclRefExpr is not "
				 << "<SymbolicDeclRefExpr> or "
				 << "<SymbolicArraySubscriptexpr>\n";
#endif
#if 0
		    if (foundInit || foundUpdate) {
			llvm::outs() << "ERROR: Multiple labels for the same "
				     << "statement " 
				     << Helper::prettyPrintExpr(B) << "\n";
			llvm::outs() << "foundUpdate || foundInit \n";
			llvm::outs() << "SymExpr: ";
			(*sIt)->prettyPrint(true);
			error = true;
			return false;
		    }
#endif
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Found OTHER\n";
#endif
		    currLabel = labelKind::OTHER;
		    foundOther = true;
		    labelSet = true;
		    break;
		}
		if (isa<SymbolicDeclRefExpr>(sdre->substituteExpr)) {
		    sdre = dyn_cast<SymbolicDeclRefExpr>(sdre->substituteExpr);
		    continue;
		}
		if (isa<SymbolicArraySubscriptExpr>(sdre->substituteExpr)) {
		    SymbolicArraySubscriptExpr* sase =
			dyn_cast<SymbolicArraySubscriptExpr>(sdre->substituteExpr);
		    sdre = sase->baseArray;
		    continue;
		}
	    }
	    if (labelSet) continue;

	    if (sdre->var.equals(lhsArrayDetails->baseArray->var)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Found UPDT\n";
#endif
		currLabel = labelKind::UPDATE;
		foundUpdate = true;
		labelSet = true;
		continue;
	    } else if (sdre->vKind == SymbolicDeclRefExpr::VarKind::INPUTVAR) {
#if 0
		if (foundUpdate || foundOther) {
		    llvm::outs() << "ERROR: Multiple labels for the same statement "
				 << Helper::prettyPrintExpr(B) << "\n";
		    llvm::outs() << "foundUpdate || foundOther at input\n";
		    error = true;
		    return false;
		}
#endif
#ifdef DEBUG
		llvm::outs() << "DEBUG: Found INIT\n";
#endif
		currLabel = labelKind::INIT;
		foundInit = true;
	    } else {
		labelSet = false;
#if 0
		// Check if this is the update. Check it is same as the LHS
		// array. TODO:Might not be correct in all cases.
		if (sdre->var.equals(lhsArrayDetails->baseArray->var)) {
		    currLabel = labelKind::UPDT;
		    foundUpdate = true;
		    labelSet = true;
		    continue;
		}
#endif

		// Check if this var is one of the input vars
		for (std::vector<InputVar>::iterator iIt = mObj->inputs.begin();
			iIt != mObj->inputs.end(); iIt++) {
		    if (iIt->var.equals(sdre->var)) {
#if 0
			if (foundUpdate || foundOther) {
			    llvm::outs() << "ERROR: Multiple labels for the same "
					 << "statement "
					 << Helper::prettyPrintExpr(B) << "\n";
			    llvm::outs() << "foundUpdate || foundOther at input\n";
			    error = true;
			    return false;
			}
#endif
#ifdef DEBUG
			llvm::outs() << "DEBUG: Found INIT\n";
#endif
			currLabel = labelKind::INIT;
			foundInit = true;
			labelSet = true;
			break;
		    }
		}
		if (labelSet) continue;
#ifdef DEBUG
		llvm::outs() << "DEBUG: SymbolicDeclRefExpr is not inputvar\n";
		//sdre->print();
		sdre->prettyPrint(true);
#endif

#if 0
		if (foundInit || foundUpdate) {
		    llvm::outs() << "ERROR: Multiple labels for the same statement "
				 << Helper::prettyPrintExpr(B) << "\n";
		    llvm::outs() << "foundUpdate || foundInit at other\n";
		    error = true;
		    return false;
		}
#endif
#ifdef DEBUG
		llvm::outs() << "DEBUG: Found OTHER\n";
#endif
		currLabel = labelKind::OTHER;
		foundOther = true;
	    }
	} else  {
	    bool labelSet = false;
#ifdef DEBUG
	    llvm::outs() << "DEBUG: SymExpr:\n";
	    if (isa<SymbolicExpr>(*sIt))
		dyn_cast<SymbolicExpr>(*sIt)->prettyPrint(true);
	    else
		(*sIt)->print();
	    llvm::outs() << "DEBUG: Vars referenced:\n";
	    (*sIt)->printVarRefs();
#endif
	    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
		    (*sIt)->varsReferenced.begin(); vrIt != 
		    (*sIt)->varsReferenced.end(); vrIt++) {
#ifdef DEBUG
		llvm::outs() << "*vrIt: " << (*vrIt)->var.varName << "\n";
		llvm::outs() << "lhs: " << lhsArrayDetails->baseArray->var.varName << "\n";
#endif
		if ((*vrIt)->var.equals(lhsArrayDetails->baseArray->var)) {
#if 0
		    if (foundInit || foundOther) {
			llvm::outs() << "ERROR: Multiple labels for the same statement "
				     << Helper::prettyPrintExpr(B) << "\n";
			error = true;
			return false;
		    }
#endif
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Equal\n";
#endif
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Found UPDT\n";
#endif
		    currLabel = labelKind::UPDATE;
		    foundUpdate = true;
		    labelSet = true;
		    break;
		} else {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Not equal\n";
#endif
		}
	    }
	    if (labelSet) continue;
#if 0
	    bool containsLHS = dyn_cast<SymbolicExpr>((*sIt))->containsVar(lhsArrayDetails->baseArray->var, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While containsVar()\n";
		error = true;
		return false;
	    }
	    if (containsLHS) {
		currLabel = labelKind::UPDATE;
		foundUpdate = true;
		labelSet = true;
		continue;
	    }
#endif

	    // Check if RHS is INT_MIN
	    if (isa<SymbolicBinaryOperator>(*sIt)) {
		SymbolicBinaryOperator* sbo =
		    dyn_cast<SymbolicBinaryOperator>(*sIt);
		SymbolicExpr* lhs = sbo->lhs;
		SymbolicExpr* rhs = sbo->rhs;
		if (lhs->isConstantLiteral() && rhs->isConstantLiteral()) {
#if 0
		    if (foundUpdate || foundOther) {
			llvm::outs() << "ERROR: Multiple labels for the same statement "
				     << Helper::prettyPrintExpr(B) << "\n";
			error = true;
			return false;
		    }
#endif
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Found INIT\n";
#endif
		    currLabel = labelKind::INIT;
		    foundInit = true;
		    labelSet = true;
		}
	    }

#if 0
	    if (!labelSet && (foundInit || foundUpdate)) {
		llvm::outs() << "ERROR: Multiple labels for the same statement "
			     << Helper::prettyPrintExpr(B) << "\n";
		error = true;
		return false;
	    }
#endif
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Found OTHER\n";
#endif
	    currLabel = labelKind::OTHER;
	    foundOther = true;
	}
	i++;
    }
    
    if (foundInit) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Assignment is INIT\n";
#endif
	if (std::find(initLinesFound.begin(), initLinesFound.end(),
		currDetails->lineNum) == initLinesFound.end())
	    initLinesFound.push_back(currDetails->lineNum);
	bool initExists = false;
	for (std::vector<StmtDetails*>::iterator iIt = initStmts.begin(); iIt
		!= initStmts.end(); iIt++) {
	    if ((*iIt)->equals(currDetails)) {
		initExists = true;
		break;
	    }
	}
	if (!initExists) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Inserting initstmt\n";
#endif
	    initStmts.push_back(currDetails);
	}
    } else if (foundUpdate) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Assignment is UPDATE\n";
#endif
	if (std::find(updateLinesFound.begin(), updateLinesFound.end(),
		currDetails->lineNum) == updateLinesFound.end())
	    updateLinesFound.push_back(currDetails->lineNum);
#ifdef DEBUG
	llvm::outs() << "DEBUG: Existing updateStmts:\n";
	for (std::vector<StmtDetails*>::iterator sIt = updateStmts.begin(); sIt
		!= updateStmts.end(); sIt++) {
	    if ((*sIt)->origStmt) 
		llvm::outs() << Helper::prettyPrintStmt((*sIt)->origStmt) 
			     << " [" << (*sIt)->lineNum << "]\n";
	    else
		(*sIt)->print();
	}
#endif
	bool updtExists = false;
	for (std::vector<StmtDetails*>::iterator uIt = updateStmts.begin(); uIt
		!= updateStmts.end(); uIt++) {
	    if ((*uIt)->equals(currDetails)) {
		updtExists = true;
		break;
	    }
	}
	if (!updtExists) {
	    updateStmts.push_back(currDetails);
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Inserted currDetails to updateStmts\n";
	    if (currDetails->origStmt)
		llvm::outs() << Helper::prettyPrintStmt(currDetails->origStmt)
			     << " [" << currDetails->lineNum << "]\n";
	    else
		currDetails->print();
#endif
	}
#ifdef DEBUG
	llvm::outs() << "DEBUG: Num updates so far: " << updateStmts.size() << "\n";
	llvm::outs() << "DEBUG: Num inits so far: " << initStmts.size() << "\n";
#endif
    } else {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Assignment is OTHER\n";
#endif
    }
    return true;
}

bool InitUpdateStmtVisitor::VisitVarDecl(VarDecl* VD) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering InitUpdateStmtVisitor::VisitVarDecl()\n";
#endif
    SourceLocation SL = SM->getExpansionLoc(VD->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: VarDecl is not in main program\n";
	error = true;
	return false;
    }

    if (!(VD->hasInit())) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: VarDecl has no init. "
		     << "Skipping..\n";
#endif
	return true;
    }

    bool rv;
    VarDeclDetails* currDetails = VarDeclDetails::getObject(SM, VD,
	currBlock, rv, currFunction);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get VarDeclDetails\n";
	error = true;
	return false;
    }

    if (!currDetails) {
	llvm::outs() << "ERROR: NULL VarDeclDetails\n";
	error = true;
	return false;
    }

    if (currDetails->var.arraySizeInfo.size() == 0) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: LHS of VarDecl is not an array.Skipping\n";
#endif
	return true;
    }
    
    if (!(currDetails->initExpr)) {
	llvm::outs() << "ERROR: NULL init in VarDeclDetails\n";
	error = true;
	return false;
    }
    ExprDetails* rhsExpr = currDetails->initExpr;
    if (!isa<InitListExprDetails>(rhsExpr)) {
	llvm::outs() << "ERROR: Init of VarDecl is not InitListExpr\n";
	error = true;
	return false;
    }

    bool foundInit = true;

    if (foundInit) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Assignment is INIT\n";
#endif
	if (std::find(initLinesFound.begin(), initLinesFound.end(),
		currDetails->lineNum) == initLinesFound.end())
	    initLinesFound.push_back(currDetails->lineNum);
	bool initExists = false;
	for (std::vector<StmtDetails*>::iterator iIt = initStmts.begin(); iIt
		!= initStmts.end(); iIt++) {
	    if ((*iIt)->equals(currDetails)) {
		initExists = true;
		break;
	    }
	}
	if (!initExists) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Inserting initstmt\n";
#endif
	    initStmts.push_back(currDetails);
	}
    }
    return true;
}

bool BlockStmtVisitor::VisitCallExpr(CallExpr* C) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering BlockStmtVisitor::VisitCallExpr() on "
		 << Helper::prettyPrintExpr(C) << "\n";
    //return false;
#endif
    SourceLocation SL = SM->getExpansionLoc(C->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: CallExpr is not in main program\n";
	error = true;
	return false;
    }

    bool rv;

    FunctionDecl* calleeFD = C->getDirectCallee();
    if (!calleeFD) {
	llvm::outs() << "ERROR: Cannot get FunctionDecl of callee from "
		     << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }
    calleeFD = calleeFD->getMostRecentDecl();
    if (!calleeFD) {
	llvm::outs() << "ERROR: Cannot get most recent decl of callee: "
		     << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }

    // Sanity check: source location of FunctionDecl is in the
    // main program
    SL = SM->getExpansionLoc(calleeFD->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	// If this call expr has decl outside main program, it might be one
	// of the library functions. Skip this
	//return false;
	return true;
    }

    // Sanity check: This is not a custom input function or a custom output
    // function
    std::string fName = calleeFD->getQualifiedNameAsString();
    if (std::find(customInputFuncs.begin(), customInputFuncs.end(), fName) != 
	    customInputFuncs.end()) {
	// This is a custom input function. Skip
	return true;
    }
    if (std::find(customOutputFuncs.begin(), customOutputFuncs.end(), fName) != 
	    customOutputFuncs.end()) {
	// This is a custom output function. Skip
	return true;
    }

    // Get CallExprDetails of this call
    CallExprDetails* callExpr = CallExprDetails::getObject(SM, C, currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain CallExprDetails of callExpr: "
		     << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }

    // If this call was already visited, skip
    for (std::vector<CallExprDetails*>::iterator cIt =
	    callsVisitedAlready.begin(); cIt != callsVisitedAlready.end();
	    cIt++) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: calls visited already:\n";
	(*cIt)->prettyPrint();
#endif
	if (callExpr->equals(*cIt)) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: This call was already visited. Skipping..\n";
#endif
	    return true;
	}
    }

    std::vector<SymbolicStmt*> callSymExprs =
	symVisitor->getSymbolicExprs(callExpr, rv);
    if (!rv) {
	llvm::outs() << "WARNING: Cannot obtain symbolic exprs of CallExprDetails\n";
	//error = true;
	//return true;
    }

    // Get Function Details of this function
    FunctionDetails* calleeFunction = Helper::getFunctionDetailsFromDecl(SM,
	calleeFD, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get FunctionDetails of " << fName << "\n";
	error = true;
	return false;
    }
    for (std::vector<FunctionDetails*>::iterator fIt =
	    mObj->functions.begin(); fIt != mObj->functions.end(); fIt++) {
	if ((*fIt)->equals(calleeFunction)) {
	    calleeFunction = *fIt;
	    break;
	}
    }

    GetSymbolicExprVisitor* funcSym = NULL;
    FunctionAnalysis* funcAnalysis = NULL;
    for (std::vector<FunctionAnalysis*>::iterator aIt = 
	    mObj->analysisInfo.begin(); aIt != mObj->analysisInfo.end(); aIt++) {
	if ((*aIt)->function->equals(calleeFunction)) {
	    funcSym = (*aIt)->symVisitor;
	    funcAnalysis = *aIt;
	    break;
	}
    }

    if (!funcSym) {
	llvm::outs() << "ERROR: NULL GetSymbolicExprVisitor for callee\n";
	error = true;
	return false;
    }
    if (!funcAnalysis) {
	llvm::outs() << "ERROR: NULL FunctionAnalysis for callee\n";
	error = true;
	return false;
    }

    // Recursively call BlockStmtVisitor on this function
    BlockStmtVisitor bVisitor(this, funcSym);
    bVisitor.setCurrFunction(calleeFunction);

#ifdef DEBUG
    llvm::outs() << "DEBUG: Calling BlockStmtVisitor on function " 
		 << calleeFunction->funcName << "\n";
#endif
    MainFunction::runVisitorOnInlinedProgram(calleeFunction->cfg, funcAnalysis, 
	&bVisitor, rv);
    if (bVisitor.error) {
	llvm::outs() << "ERROR: While running BlockStmtVisitor on "
		     << "inlined function " << calleeFunction->funcName << "\n";
	error = true;
	return false;
    }
#ifdef DEBUG
    llvm::outs() << "DEBUG: Returning BlockStmtVisitor on function " 
		 << calleeFunction->funcName << "\n";
    llvm::outs() << "DEBUG: Blocks from function:\n";
    for (std::vector<BlockDetails*>::iterator it =
	    bVisitor.blocksIdentified.begin(); it !=
	    bVisitor.blocksIdentified.end(); it++) {
	(*it)->prettyPrintHere();
    }
#endif

    callsVisitedAlready.push_back(callExpr);

    residualStmts.insert(residualStmts.end(), bVisitor.residualStmts.begin(),
	bVisitor.residualStmts.end());

    bool foundCallExpr = false;
    std::vector<std::pair<CallExprDetails*, std::vector<BlockDetails*> >
	    >::iterator cIt = callExprToBlocksIdentified.begin();
    for (; cIt != callExprToBlocksIdentified.end(); cIt++) {
	if (cIt->first->equals(callExpr)) {
	    foundCallExpr = true;
	    break;
	}
    }
    if (!foundCallExpr) {
	cIt = callExprToBlocksIdentified.begin();
    }
    if (callExprToBlocksIdentified.size() == 0) {
	callExprToBlocksIdentified.push_back(std::make_pair(callExpr,
	    std::vector<BlockDetails*>()));
	cIt = callExprToBlocksIdentified.begin();
    }

    if (callSymExprs.size() == 0) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: callSymExprs 0\n";
	callExpr->prettyPrint();
	llvm::outs() << "\n";
#endif
	for (std::vector<BlockDetails*>::iterator it =
		bVisitor.blocksIdentified.begin(); it != 
		bVisitor.blocksIdentified.end(); it++) {
	    BlockDetails* newBlock = (*it)->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot clone BlockDetails\n";
		error = true;
		return false;
	    }
	    //blocksIdentified.push_back(newBlock);
#if 0
	    if (callExprToBlocksIdentified.size() == 0) {
		std::vector<BlockDetails*> bVector;
#ifdef DEBUG
		llvm::outs() << "DEBUG: Before pushing into bVector\n";
#endif
		bVector.push_back(newBlock);
#ifdef DEBUG
		llvm::outs() << "DEBUG: After pushing into bVector\n";
#endif
		//callExprToBlocksIdentified.push_back(std::make_pair(callExpr,
		    //bVector));
		std::pair<CallExprDetails*, std::vector<BlockDetails*> > p;
		p.first = callExpr;
		p.second = bVector;
#ifdef DEBUG
		llvm::outs() << "DEBUG: I created the pair\n";
#endif
		callExprToBlocksIdentified.push_back(p);
#ifdef DEBUG
		llvm::outs() << "DEBUG: I created the pair\n";
#endif
	    } else 
#endif
		cIt->second.push_back(newBlock);
	}
	return true;
    }

    // I generate one symExpr for each return stmt in the callee function. So
    // the following loop creates some duplicate blocks.

    for (std::vector<BlockDetails*>::iterator it =
	    bVisitor.blocksIdentified.begin(); it != 
	    bVisitor.blocksIdentified.end(); it++) {
	std::vector<std::vector<SymbolicExpr*> > argSeen;
	for (std::vector<SymbolicStmt*>::iterator sIt = callSymExprs.begin();
		sIt != callSymExprs.end(); sIt++) {
	    if (!isa<SymbolicCallExpr>(*sIt)) {
		llvm::outs() << "ERROR: Symexpr of CallExpr is not "
			     << "SymbolicCallExpr\n";
		error = true;
		return false;
	    }
	    SymbolicCallExpr* sce = dyn_cast<SymbolicCallExpr>(*sIt);
	    bool foundArg = false;
	    for (std::vector<std::vector<SymbolicExpr*> >::iterator aIt =
		    argSeen.begin(); aIt != argSeen.end(); aIt++) {
		if (sce->callArgExprs.size() != aIt->size()) {
		    llvm::outs() << "ERROR: Arg size not same\n";
		    error = true;
		    return false;
		}
		std::vector<SymbolicExpr*> currArgs = *aIt;
		bool foundDiff = false;
		for (unsigned i = 0; i < aIt->size(); i++) {
		    if (currArgs[i] && sce->callArgExprs[i]) {
			if (!(currArgs[i]->equals(sce->callArgExprs[i]))) {
			    foundDiff = true;
			    break;
			}
		    }
		}
		if (!foundDiff) {
		    foundArg = true;
		    break;
		}
	    }
	    if (foundArg) continue;

	    BlockDetails* newBlock = (*it)->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot clone BlockDetails\n";
		error = true;
		return false;
	    }
#ifdef DEBUG
	    newBlock->prettyPrintHere();
	    llvm::outs() << "About to replace args\n";
#endif
	    newBlock->replaceVarsWithExprs(calleeFunction->parameters,
		sce->callArgExprs, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in block\n";
		error = true;
		return false;
	    }
#ifdef DEBUG
	    newBlock->prettyPrintHere();
	    llvm::outs() << "DEBUG: Inserting newBlock to blocksIdentified\n";
#endif
	    //blocksIdentified.push_back(newBlock);
#if 0
	    if (callExprToBlocksIdentified.size() == 0)  { 
		std::vector<BlockDetails*> bVector;
		bVector.push_back(newBlock);
		callExprToBlocksIdentified.push_back(std::make_pair(callExpr,
		    bVector));
	    } else
#endif
		cIt->second.push_back(newBlock);
	    argSeen.push_back(sce->callArgExprs);
	}
    }

    return true;
}

bool ReturnStmtVisitor::VisitReturnStmt(ReturnStmt* R) {
    SourceLocation SL = SM->getExpansionLoc(R->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: Return Stmt is not in main program\n";
	error = true;
	return false;
    }

    bool rv;
    ReturnStmtDetails* rds = ReturnStmtDetails::getObject(SM, R, currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get ReturnStmtDetails from ReturnStmt\n";
	error = true;
	return false;
    }

    returnStmtsFound.push_back(rds);
    return true;
}

bool DeclStmtVisitor::VisitCallExpr(CallExpr* C) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering DeclStmtVisitor::VisitCallExpr() on "
		 << Helper::prettyPrintExpr(C) << "\n";
    //return false;
#endif
    SourceLocation SL = SM->getExpansionLoc(C->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: CallExpr is not in main program\n";
	error = true;
	return false;
    }

    bool rv;

    FunctionDecl* calleeFD = C->getDirectCallee();
    if (!calleeFD) {
	llvm::outs() << "ERROR: Cannot get FunctionDecl of callee from "
		     << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }
    calleeFD = calleeFD->getMostRecentDecl();
    if (!calleeFD) {
	llvm::outs() << "ERROR: Cannot get most recent decl of callee: "
		     << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }

    // Sanity check: source location of FunctionDecl is in the
    // main program
    SL = SM->getExpansionLoc(calleeFD->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	// If this call expr has decl outside main program, it might be one
	// of the library functions. Skip this
	//return false;
	return true;
    }

    // Sanity check: This is not a custom input function or a custom output
    // function
    std::string fName = calleeFD->getQualifiedNameAsString();
    if (std::find(customInputFuncs.begin(), customInputFuncs.end(), fName) != 
	    customInputFuncs.end()) {
	// This is a custom input function. Skip
	return true;
    }
    if (std::find(customOutputFuncs.begin(), customOutputFuncs.end(), fName) != 
	    customOutputFuncs.end()) {
	// This is a custom output function. Skip
	return true;
    }

    // Get CallExprDetails of this call
    CallExprDetails* callExpr = CallExprDetails::getObject(SM, C, currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain CallExprDetails of callExpr: "
		     << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }

    // If this call was already visited, skip
    for (std::vector<CallExprDetails*>::iterator cIt =
	    callsVisitedAlready.begin(); cIt != callsVisitedAlready.end();
	    cIt++) {
	if (callExpr->equals(*cIt)) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: This call was already visited. Skipping..\n";
#endif
	    return true;
	}
    }

    std::vector<SymbolicStmt*> callSymExprs =
	symVisitor->getSymbolicExprs(callExpr, rv);
    if (!rv) {
	llvm::outs() << "WARNING: Cannot obtain symbolic exprs of CallExprDetails\n";
	//error = true;
	//return true;
    }

    // Get Function Details of this function
    FunctionDetails* calleeFunction = Helper::getFunctionDetailsFromDecl(SM,
	calleeFD, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get FunctionDetails of " << fName << "\n";
	error = true;
	return false;
    }
    for (std::vector<FunctionDetails*>::iterator fIt =
	    mObj->functions.begin(); fIt != mObj->functions.end(); fIt++) {
	if ((*fIt)->equals(calleeFunction)) {
	    calleeFunction = *fIt;
	    break;
	}
    }

    GetSymbolicExprVisitor* funcSym = NULL;
    FunctionAnalysis* funcAnalysis = NULL;
    for (std::vector<FunctionAnalysis*>::iterator aIt = 
	    mObj->analysisInfo.begin(); aIt != mObj->analysisInfo.end(); aIt++) {
	if ((*aIt)->function->equals(calleeFunction)) {
	    funcSym = (*aIt)->symVisitor;
	    funcAnalysis = *aIt;
	    break;
	}
    }

    if (!funcSym) {
	llvm::outs() << "ERROR: NULL GetSymbolicExprVisitor for callee\n";
	error = true;
	return false;
    }
    if (!funcAnalysis) {
	llvm::outs() << "ERROR: NULL FunctionAnalysis for callee\n";
	error = true;
	return false;
    }

    // Recursively call DeclStmtVisitor on this function
    DeclStmtVisitor dVisitor(this, funcSym);
    dVisitor.setCurrFunction(calleeFunction);

#if 0
    std::unique_ptr<CFG> calleeCFG = calleeFunction.constructCFG(rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot construct CFG of function:\n";
	calleeFunction.print();
	error = true;
	return false;
    }
#endif
#ifdef DEBUG
    llvm::outs() << "DEBUG: Calling DeclStmtVisitor on function " 
		 << calleeFunction->funcName << "\n";
#endif
    MainFunction::runVisitorOnInlinedProgram(calleeFunction->cfg, funcAnalysis, 
	&dVisitor, rv);
    if (dVisitor.error) {
	llvm::outs() << "ERROR: While running DeclStmtVisitor on "
		     << "inlined function " << calleeFunction->funcName << "\n";
	error = true;
	return false;
    }
#ifdef DEBUG
    llvm::outs() << "DEBUG: Returning DeclStmtVisitor on function " 
		 << calleeFunction->funcName << "\n";
#endif

    callsVisitedAlready.push_back(callExpr);

    if (callSymExprs.size() == 0) {
	for (std::vector<InputVar>::iterator it =
		dVisitor.inputVarsUpdated.begin(); it != 
		dVisitor.inputVarsUpdated.end(); it++) {
	    InputVar iv = it->clone();
	    inputVarsUpdated.push_back(iv);
	}
	for (std::vector<DPVar>::iterator it = 
		dVisitor.dpVarsUpdated.begin(); it != 
		dVisitor.dpVarsUpdated.end(); it++) {
	    DPVar dv = it->clone(rv);
	    if (!rv) {
		error = true;
		return false;
	    }
	    dpVarsUpdated.push_back(dv);
	}
	return true;
    }

    std::vector<std::vector<SymbolicExpr*> > argSeen;

    for (std::vector<InputVar>::iterator it =
	    dVisitor.inputVarsUpdated.begin(); it != 
	    dVisitor.inputVarsUpdated.end(); it++) {
	for (std::vector<SymbolicStmt*>::iterator sIt = callSymExprs.begin();
		sIt != callSymExprs.end(); sIt++) {
	    if (!isa<SymbolicCallExpr>(*sIt)) {
		llvm::outs() << "ERROR: Symexpr of CallExpr is not "
			     << "SymbolicCallExpr\n";
		error = true;
		return false;
	    }
	    SymbolicCallExpr* sce = dyn_cast<SymbolicCallExpr>(*sIt);
	    bool foundArg = false;
	    for (std::vector<std::vector<SymbolicExpr*> >::iterator aIt =
		    argSeen.begin(); aIt != argSeen.end(); aIt++) {
		if (sce->callArgExprs.size() != aIt->size()) {
		    llvm::outs() << "ERROR: Arg size not same\n";
		    error = true;
		    return false;
		}
		std::vector<SymbolicExpr*> currArgs = *aIt;
		bool foundDiff = false;
		for (unsigned i = 0; i < aIt->size(); i++) {
		    if (currArgs[i] && sce->callArgExprs[i]) {
			if (!(currArgs[i]->equals(sce->callArgExprs[i]))) {
			    foundDiff = true;
			    break;
			}
		    }
		}
		if (!foundDiff) {
		    foundArg = true;
		    break;
		}
	    }
	    if (foundArg) continue;

	    InputVar newIV = it->clone();
	    for (std::vector<SymbolicExpr*>::iterator sIt =
		    newIV.sizeExprs.begin(); sIt != newIV.sizeExprs.end();
		    sIt++) {
		(*sIt)->replaceVarsWithExprs(calleeFunction->parameters,
		    sce->callArgExprs, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While replacing vars in InputVar\n";
		    error = true;
		    return false;
		}
	    }
	    inputVarsUpdated.push_back(newIV);
	    argSeen.push_back(sce->callArgExprs);
	}
    }

    argSeen.clear();
    for (std::vector<DPVar>::iterator it =
	    dVisitor.dpVarsUpdated.begin(); it != 
	    dVisitor.dpVarsUpdated.end(); it++) {
	for (std::vector<SymbolicStmt*>::iterator sIt = callSymExprs.begin();
		sIt != callSymExprs.end(); sIt++) {
	    if (!isa<SymbolicCallExpr>(*sIt)) {
		llvm::outs() << "ERROR: Symexpr of CallExpr is not "
			     << "SymbolicCallExpr\n";
		error = true;
		return false;
	    }
	    SymbolicCallExpr* sce = dyn_cast<SymbolicCallExpr>(*sIt);
	    bool foundArg = false;
	    for (std::vector<std::vector<SymbolicExpr*> >::iterator aIt =
		    argSeen.begin(); aIt != argSeen.end(); aIt++) {
		if (sce->callArgExprs.size() != aIt->size()) {
		    llvm::outs() << "ERROR: Arg size not same\n";
		    error = true;
		    return false;
		}
		std::vector<SymbolicExpr*> currArgs = *aIt;
		bool foundDiff = false;
		for (unsigned i = 0; i < aIt->size(); i++) {
		    if (currArgs[i] && sce->callArgExprs[i]) {
			if (!(currArgs[i]->equals(sce->callArgExprs[i]))) {
			    foundDiff = true;
			    break;
			}
		    }
		}
		if (!foundDiff) {
		    foundArg = true;
		    break;
		}
	    }
	    if (foundArg) continue;

	    DPVar newDV = it->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While cloning DPVar\n";
		error = true;
		return false;
	    }
	    for (std::vector<SymbolicExpr*>::iterator sIt =
		    newDV.sizeExprs.begin(); sIt != newDV.sizeExprs.end();
		    sIt++) {
		(*sIt)->replaceVarsWithExprs(calleeFunction->parameters,
		    sce->callArgExprs, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While replacing vars in DPVar\n";
		    error = true;
		    return false;
		}
	    }
	    dpVarsUpdated.push_back(newDV);
	    argSeen.push_back(sce->callArgExprs);
	}
    }
    return true;
}

bool DeclStmtVisitor::VisitVarDecl(VarDecl* VD) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering DeclStmtVisitor::VisitVarDecl()\n";
#endif

    SourceLocation SL = SM->getExpansionLoc(VD->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL))
	return true;

    bool rv;
    VarDeclDetails* currDetails = VarDeclDetails::getObject(SM, VD, currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get VarDeclDetails from VarDecl\n";
	error = true;
	return false;
    }

    if (currDetails->var.arraySizeInfo.size() == 0) return true;

    bool isRelevant = false;
    for (std::vector<InputVar>::iterator inpIt = mObj->inputs.begin();
	    inpIt != mObj->inputs.end(); inpIt++) {
	if (inpIt->var.equals(currDetails->var)) {
	    isRelevant = true;
	    break;
	}
    }
    for (std::vector<DPVar>::iterator dpIt = mObj->allDPVars.begin();
	    dpIt != mObj->allDPVars.end(); dpIt++) {
	if (dpIt->dpArray.equals(currDetails->var)) {
	    isRelevant = true;
	    break;
	}
    }
    if (!isRelevant) return true;

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting VarDecl\n";
    if (currDetails->origStmt) {
	llvm::outs() << Helper::prettyPrintStmt(currDetails->origStmt) << "\n";
    } else {
	currDetails->print();
    }
#endif

    std::vector<SymbolicExpr*> sizeSymExprs;
    for (std::vector<const ArrayType*>::iterator tIt =
	    currDetails->var.arraySizeInfo.begin(); tIt !=
	    currDetails->var.arraySizeInfo.end(); tIt++) {
	if (isa<ConstantArrayType>(*tIt)) {
            const llvm::APInt size =
                dyn_cast<ConstantArrayType>(*tIt)->getSize();
	    SymbolicIntegerLiteral* sil = new SymbolicIntegerLiteral;
	    sil->val = size;
	    sizeSymExprs.push_back(sil);
        } else if (isa<VariableArrayType>(*tIt)) {
            Expr* sizeExpr =
                dyn_cast<VariableArrayType>(*tIt)->getSizeExpr();
	    ExprDetails* sizeDetails = ExprDetails::getObject(SM, sizeExpr, 
		currBlock, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot get ExprDetails of "
			     << Helper::prettyPrintExpr(sizeExpr) << "\n";
		error = true;
		return false;
	    }

	    if (!(symVisitor->isResolved(sizeDetails))) {
		llvm::outs() << "ERROR: Size of var " 
			     << currDetails->var.varName << " is not resolved\n";
		error = true;
		return false;
	    }
	    std::vector<SymbolicStmt*> symExprs =
		symVisitor->getSymbolicExprs(sizeDetails, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot get symexprs for "
			     << currDetails->var.varName << "\n";
		error = true;
		return false;
	    }
	    if (symExprs.size() != 1) {
		llvm::outs() << "ERROR: " << currDetails->var.varName
			     << " has more than one symexprs\n";
		error = true;
		return false;
	    }
	    if (!isa<SymbolicExpr>(symExprs[0])) {
		llvm::outs() << "ERROR: Symexpr of <Expr> is not <SymbolicExpr>\n";
		error = true;
		return false;
	    }
	    sizeSymExprs.push_back(dyn_cast<SymbolicExpr>(symExprs[0]));
        } else {
            llvm::outs() << "ERROR: Unsupported array type\n";
            error = true;
            return false;
        }
    }

    for (std::vector<InputVar>::iterator inpIt = mObj->inputs.begin();
	    inpIt != mObj->inputs.end(); inpIt++) {
	if (inpIt->var.equals(currDetails->var)) {
	    //inpIt->sizeExprs.insert(inpIt->sizeExprs.end(),
		//sizeSymExprs.begin(), sizeSymExprs.end());
	    // Check if this inputVar was already visited, then update that
	    // entry.
	    bool found = false;
	    for (std::vector<InputVar>::iterator iIt = inputVarsUpdated.begin();
		    iIt != inputVarsUpdated.end(); iIt++) {
		if (iIt->var.equals(inpIt->var)) {
		    iIt->sizeExprs.insert(iIt->sizeExprs.end(),
			sizeSymExprs.begin(), sizeSymExprs.end());
		    found = true;
		    break;
		}
	    }
	    if (!found) {
		InputVar cloneIV = inpIt->clone();
		cloneIV.sizeExprs.insert(cloneIV.sizeExprs.end(),
		    sizeSymExprs.begin(), sizeSymExprs.end());
		inputVarsUpdated.push_back(cloneIV);
	    }
	    break;
	}
    }
    for (std::vector<DPVar>::iterator dpIt = mObj->allDPVars.begin();
	    dpIt != mObj->allDPVars.end(); dpIt++) {
	if (dpIt->dpArray.equals(currDetails->var)) {
	    //dpIt->sizeExprs.insert(dpIt->sizeExprs.end(),
		//sizeSymExprs.begin(), sizeSymExprs.end());
	    bool found = false;
	    for (std::vector<DPVar>::iterator dIt = dpVarsUpdated.begin();
		    dIt != dpVarsUpdated.end(); dIt++) {
		if (dIt->dpArray.equals(dpIt->dpArray)) {
		    dIt->sizeExprs.insert(dIt->sizeExprs.end(),
			sizeSymExprs.begin(), sizeSymExprs.end());
		    found = true;
		    break;
		}
	    }
	    if (!found) {
		DPVar cloneDV = dpIt->clone(rv);
		if (!rv) {
		    error = true;
		    return false;
		}
		cloneDV.sizeExprs.insert(cloneDV.sizeExprs.end(),
		    sizeSymExprs.begin(), sizeSymExprs.end());
		dpVarsUpdated.push_back(cloneDV);
	    }
	    break;
	}
    }
    return true;
}

bool SubExprVisitor::VisitIfStmt(IfStmt* I) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering SubExprVisitor::VisitIfStmt(): "
		 << Helper::prettyPrintStmt(I) << "\n";
#endif
    if (Stmt* C = I->getCond()) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Condition of IfStmt: "
		     << Helper::prettyPrintStmt(C) << "\n";
#endif
	TraverseStmt(C);
#ifdef DEBUG
	llvm::outs() << "DEBUG: After visiting condition of IfStmt\n";
#endif
	if (error)
	    llvm::outs() << "ERROR: While visiting condition of IfStmt\n";
	else {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: No error while visiting condition of IfStmt\n";
#endif
	}
	return false;
    } else {
	llvm::outs() << "ERROR: Cannot obtain condition of if stmt "
		     << Helper::prettyPrintStmt(I) << "\n";
	error = true;
	return false;
    }
}

bool SubExprVisitor::VisitForStmt(ForStmt* F) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering SubExprVisitor::VisitForStmt(): "
		 << Helper::prettyPrintStmt(F) << "\n";
#endif
    if (Stmt* C = F->getCond()) {
	VisitStmt(C);
	if (error) return false;
	return false;
    } else {
	llvm::outs() << "ERROR: Cannot obtain condition of for stmt "
		     << Helper::prettyPrintStmt(F) << "\n";
	error = true;
	return false;
    }
}

bool SubExprVisitor::VisitWhileStmt(WhileStmt* W) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering SubExprVisitor::VisitWhileStmt(): "
		 << Helper::prettyPrintStmt(W) << "\n";
#endif
    if (Stmt* C = W->getCond()) {
	VisitStmt(C);
	if (error) return false;
	return false;
    } else {
	llvm::outs() << "ERROR: Cannot obtain condition of while stmt "
		     << Helper::prettyPrintStmt(W) << "\n";
	error = true;
	return false;
    }
}

bool SubExprVisitor::VisitDoStmt(DoStmt* D) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering SubExprVisitor::VisitDoStmt(): "
		 << Helper::prettyPrintStmt(D) << "\n";
#endif
    if (Stmt* C = D->getCond()) {
	VisitStmt(C);
	if (error) return false;
	return false;
    } else {
	llvm::outs() << "ERROR: Cannot obtain condition of do stmt "
		     << Helper::prettyPrintStmt(D) << "\n";
	error = true;
	return false;
    }
}

bool SubExprVisitor::VisitBinaryOperator(BinaryOperator* B) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering SubExprVisitor::VisitBinaryOperator(): "
		 << Helper::prettyPrintExpr(B) << "\n";
#endif
    if (!B->isLogicalOp()) {
	//VisitExpr(B);
	//if (error) return false;
	//return true;
	StmtMapTy::iterator I = analysisObj->StmtMap.find(B);
	if (I == analysisObj->StmtMap.end()) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Cannot find entry for expression in StmtMap\n";
#endif
	    return false;
	}

	std::pair<unsigned, unsigned> p = std::make_pair(I->second.first,
	    I->second.second);
	for (std::vector<std::pair<unsigned, unsigned> >::iterator pIt =
		subExprs.begin(); pIt != subExprs.end(); pIt++) {
	    if (pIt->first == p.first && pIt->second == p.second)
		return false;
	}

#ifdef DEBUG
	llvm::outs() << "DEBUG: Inserted subExpr: B" << p.first << "." 
		     << p.second << "\n";
#endif
	subExprs.push_back(p);
	return false;
    }

    if (B->getLHS()) {
	TraverseStmt(B->getLHS());
	if (error) {
	    llvm::outs() << "ERROR: While visiting lhs of binaryoperator "
			 << Helper::prettyPrintExpr(B) << "\n";
	    return false;
	} else {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: No error while visiting lhs of binaryoperator "
			 << Helper::prettyPrintExpr(B) << "\n";
#endif
	}
    } else {
	llvm::outs() << "ERROR: Cannot obtain lhs of binaryoperator "
		     << Helper::prettyPrintExpr(B) << "\n";
	error = true;
	return false;
    }

    if (B->getRHS()) {
	TraverseStmt(B->getRHS());
	if (error) {
	    llvm::outs() << "ERROR: While visiting rhs of binaryoperator "
			 << Helper::prettyPrintExpr(B) << "\n";
	    return false;
	} else {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: No error while visiting rhs of binaryoperator "
			 << Helper::prettyPrintExpr(B) << "\n";
#endif
	    return true;
	}
    } else {
	llvm::outs() << "ERROR: Cannot obtain rhs of binaryoperator "
		     << Helper::prettyPrintExpr(B) << "\n";
	error = true;
	return false;
    }
}

bool SubExprVisitor::VisitExpr(Expr* E) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering SubExprVisitor::VisitExpr(): "
		 << Helper::prettyPrintExpr(E) << "\n";
#endif
    E = E->IgnoreParenCasts();
    if (isa<BinaryOperator>(E)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Expression is a <BinaryOperator>\n";
#endif
	VisitBinaryOperator(dyn_cast<BinaryOperator>(E));
	return false;
    } else {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Expression is not a <BinaryOperator>\n";
#endif
    }

    StmtMapTy::iterator I = analysisObj->StmtMap.find(E);
    if (I == analysisObj->StmtMap.end()) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Cannot find entry for expression in StmtMap\n";
#endif
	return false;
    }

    std::pair<unsigned, unsigned> p = std::make_pair(I->second.first,
	I->second.second);
    for (std::vector<std::pair<unsigned, unsigned> >::iterator pIt =
	    subExprs.begin(); pIt != subExprs.end(); pIt++) {
	if (pIt->first == p.first && pIt->second == p.second)
	    return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Inserted subExpr: B" << p.first << "." 
		 << p.second << "\n";
#endif
    subExprs.push_back(p);
    return false;
}

bool ReadArrayAsScalarVisitor::VisitCallExpr(CallExpr* C) {
    SourceLocation SL = SM->getExpansionLoc(C->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: CallExpr is not in main program\n";
	error = true;
	return false;
    }

    bool rv;

    FunctionDecl* calleeFD = C->getDirectCallee();
    if (!calleeFD) {
	llvm::outs() << "ERROR: Cannot get FunctionDecl of callee from "
		     << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }
    calleeFD = calleeFD->getMostRecentDecl();
    if (!calleeFD) {
	llvm::outs() << "ERROR: Cannot get most recent decl of callee: "
		     << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }

    std::string calleeName = calleeFD->getNameInfo().getAsString();

    // Sanity check: source location of FunctionDecl is in the
    // main program
    SL = SM->getExpansionLoc(calleeFD->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	// If this call expr has decl outside main program, it might be one
	// of the library functions. Skip this
	return true;
    }

    // Sanity check: This is not a custom input function or a custom output
    // function
    std::string fName = calleeFD->getQualifiedNameAsString();
    if (std::find(customInputFuncs.begin(), customInputFuncs.end(), fName) != 
	    customInputFuncs.end()) {
	// This is a custom input function. Skip
	return true;
    }
    if (std::find(customOutputFuncs.begin(), customOutputFuncs.end(), fName) != 
	    customOutputFuncs.end()) {
	// This is a custom output function. Skip
	return true;
    }

    // Get Function Details of this function
    FunctionDetails* calleeFunction = Helper::getFunctionDetailsFromDecl(SM,
	calleeFD, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get FunctionDetails of " << fName << "\n";
	error = true;
	return false;
    }
    for (std::vector<FunctionDetails*>::iterator fIt =
	    mObj->functions.begin(); fIt != mObj->functions.end(); fIt++) {
	if ((*fIt)->equals(calleeFunction)) {
	    calleeFunction = *fIt;
	    break;
	}
    }

    GetSymbolicExprVisitor* funcSym = NULL;
    for (std::vector<FunctionAnalysis*>::iterator aIt = 
	    mObj->analysisInfo.begin(); aIt != mObj->analysisInfo.end(); aIt++) {
	if ((*aIt)->function->equals(calleeFunction))
	    funcSym = (*aIt)->symVisitor;
    }

    if (!funcSym) {
	llvm::outs() << "ERROR: NULL GetSymbolicExprVisitor for callee\n";
	error = true;
	return false;
    }

    // Recursively call ReadArrayAsScalarVisitor on this function
    ReadArrayAsScalarVisitor rasVisitor(this, funcSym);
    rasVisitor.setCurrFunction(calleeFunction);

    MainFunction::runVisitorOnInlinedProgram(calleeFunction->cfg, &rasVisitor,
	rv);
    if (rasVisitor.error) {
	llvm::outs() << "ERROR: While running ReadArrayAsScalarVisitor on "
		     << "inlined function " << calleeFunction->funcName << "\n";
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Returning from ReadArrayAsScalarVisitor on inlined function "
		 << calleeFunction->funcName << "\n";
#endif

    return true;
}

bool ReadArrayAsScalarVisitor::VisitBinaryOperator(BinaryOperator* B) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering "
		 << "ReadArrayAsScalarVisitor::VisitBinaryOperator("
		 << Helper::prettyPrintExpr(B) << ")\n";
#endif
    bool rv;
    int line;

    if (!isa<ArraySubscriptExpr>(B->getLHS()))
	return true;
    if (!isa<DeclRefExpr>(B->getRHS()->IgnoreParenCasts()))
	return true;

#ifdef DEBUG
    llvm::outs() << "DEBUG: Found assignment array = scalar\n";
#endif

    DeclRefExpr* rhsVar = dyn_cast<DeclRefExpr>(B->getRHS()->IgnoreParenCasts());
    line = SM->getExpansionLineNumber(rhsVar->getLocStart());

    DeclRefExprDetails* rhsVarDetails = DeclRefExprDetails::getObject(SM, rhsVar,
	currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get DeclRefExprDetails of rhsVar\n";
	error = true;
	return false;
    }
    std::vector<SymbolicStmt*> rhsSymExprs =
	symVisitor->getSymbolicExprs(rhsVarDetails, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get SymbolicExprs of rhsVar\n";
	error = true;
	return false;
    }

    if (rhsSymExprs.size() > 1) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: More than one symbolicExprs for rhsVar. "
		     << "Skipping this assignment..\n";
#endif
	return true;
    }
    if (!isa<SymbolicDeclRefExpr>(rhsSymExprs[0])) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: SymbolicExpr of rhsVar is not "
		     << "SymbolicDeclRefExpr. Skipping this assignment..\n";
#endif
	return true;
    }

    SymbolicDeclRefExpr* rhsDeclRefExpr =
	dyn_cast<SymbolicDeclRefExpr>(rhsSymExprs[0]);
    if (rhsDeclRefExpr->vKind != SymbolicDeclRefExpr::VarKind::INPUTVAR) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: SymbolicExpr of rhsVar is not INPUTVAR.Skipping "
		     << "this assignment..\n";
#endif
	return true;
    }

    if (!currAnalysisObj) {
	llvm::outs() << "ERROR: NULL currAnalysisObj!\n";
	error = true;
	return false;
    }

    std::vector<LoopDetails*> loopsOfStmt =
	currAnalysisObj->getLoopsOfBlock(currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get loops of block " << currBlock 
		     << "\n";
	error = true;
	return false;
    }

    if (loopsOfStmt.size() == 0) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Statement is not within any loop. Skipping this"
		     << " assignment..\n";
#endif
	return true;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: loopsOfStmt:\n";
    for (std::vector<LoopDetails*>::iterator lIt = loopsOfStmt.begin(); lIt != 
	    loopsOfStmt.end(); lIt++)
	(*lIt)->prettyPrintHere();
#endif

    // Find the definition that actually reads the input
    bool runIteration = true;
    Definition d;
    do {
	std::vector<Definition> reachDefsOfRHSVarAtLine =
	    currAnalysisObj->getReachDefsOfVarAtLine(rhsVarDetails->var, line,
	    currBlock, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While obtaining reachDefs of rhsVar at line "
			 << line << "\n";
	    error = true;
	    return false;
	}

	if (reachDefsOfRHSVarAtLine.size() > 1) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: RHS has more than one reachDefs. Skipping "
			 << "this assignment...\n";
#endif
	    return true;
	}
	if (reachDefsOfRHSVarAtLine[0].expressionStr.compare("DP_INPUT") == 0) {
	    runIteration = false;
	    d = reachDefsOfRHSVarAtLine[0];
	} else
	    runIteration = true;
    } while (runIteration);

    if (currAnalysisObj->lineToBlock.find(d.lineNumber) ==
	    currAnalysisObj->lineToBlock.end()) {
	llvm::outs() << "ERROR: Cannot find blockID for line " << d.lineNumber << "\n";
	error = true;
	return false;
    }

    if (currAnalysisObj->lineToBlock.find(d.lineNumber)->second.size() > 1) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: The line " << d.lineNumber << " has more than one blockIDs\n";
#endif
	return true;
    }

    unsigned blockID = *(currAnalysisObj->lineToBlock.find(d.lineNumber)->second.begin());
    std::vector<LoopDetails*> loopsOfReadStmt =
	currAnalysisObj->getLoopsOfBlock(blockID, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get loops of block " << blockID 
		     << "\n";
	error = true;
	return false;
    }

    if (loopsOfReadStmt.size() == 0) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Statement is not within any loop. Skipping this"
		     << " assignment..\n";
#endif
	return true;
    }

    if (loopsOfStmt.size() != loopsOfReadStmt.size()) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Assignment Statement has different loops than "
		     << "ReadStmt. Skipping this assignment..\n";
#endif
	return true;
    }

    for (unsigned i = 0; i < loopsOfStmt.size(); i++) {
	if (!(loopsOfStmt[i]->equals(loopsOfReadStmt[i]))) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Assignment Statement has different loops than "
			 << "ReadStmt. Skipping this assignment..\n";
#endif
	    return true;
	}
    }

    InputVar v = getInputVarFromExpr(SM, B->getLHS()->IgnoreParenCasts(), rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get InputVar from " 
		     << Helper::prettyPrintExpr(B->getLHS()) << "\n";
	error = true;
	return false;
    }
    v.inputCallLine = line;
    v.varExpr = B->getLHS()->IgnoreParenCasts();

    BinaryOperatorDetails* assignStmtDetails =
	BinaryOperatorDetails::getObject(SM, B, currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get StmtDetails from assignment: "
		     << Helper::prettyPrintExpr(B) << "\n";
	error = true;
	return false;
    }

    // Check if the lhsarray's index expressions are the same as loop indices
    if (!isa<ArraySubscriptExprDetails>(assignStmtDetails->lhs)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: lhs of assignment is not ArraySubscriptExprDetails\n";
#endif
	return true;
    }
    ArraySubscriptExprDetails* lhsArray =
	dyn_cast<ArraySubscriptExprDetails>(assignStmtDetails->lhs);
    // Test case loop is also part of loopsOfStmt.
    if (loopsOfStmt.size()-1 != lhsArray->indexExprs.size()) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Mismatch between loops and lhsArray->indexExprs\n";
#endif
	return true;
    }
    for (unsigned i = 1; i < loopsOfStmt.size(); i++) {
	if (!isa<DeclRefExprDetails>(lhsArray->indexExprs[i-1])) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: indexExpr is not DeclRefExpr\n";
#endif
	    return true;
	}
	DeclRefExprDetails* indexI =
	    dyn_cast<DeclRefExprDetails>(lhsArray->indexExprs[i-1]);
	std::string indexIStr = indexI->var.varName;
	std::string loopIndexStr = loopsOfStmt[i]->loopIndexVar.varName;
	if (indexIStr.compare(loopIndexStr) != 0)
	    return true;
    }

    if (inputLineToAssignmentLine.find(d.lineNumber) ==
	    inputLineToAssignmentLine.end()) {
	std::pair<InputVar, StmtDetails*> p;
	p.first = v;
	p.second = assignStmtDetails;
	inputLineToAssignmentLine[d.lineNumber] = p;
    }

    
#ifdef DEBUG
    llvm::outs() << "DEBUG: Potential assignment\n";
#endif
    return true;
}

bool BreakStmtVisitor::VisitBreakStmt(BreakStmt* B) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering BreakStmtVisitor::VisitBreakStmt() at line "
		 << SM->getExpansionLineNumber(B->getLocStart()) << "\n";
#endif
    breakStmtFound = true;
    return false;
}

bool BreakStmtVisitor::VisitReturnStmt(ReturnStmt* R) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering BreakStmtVisitor::VisitReturnStmt() at line "
		 << SM->getExpansionLineNumber(R->getLocStart()) << "\n";
#endif
    breakStmtFound = true;
    return false;
}

bool GlobalVarSymExprVisitor::VisitCallExpr(CallExpr* C) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering GlobalVarSymExprVisitor::VisitCallExpr("
		 << Helper::prettyPrintExpr(C) << ")\n";
    llvm::outs() << "DEBUG: calleeLine: " 
		 << SM->getExpansionLineNumber(C->getLocStart())
		 << "\n";
#endif
    SourceLocation SL = SM->getExpansionLoc(C->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: CallExpr is not in main program\n";
	error = true;
	return false;
    }

    bool rv;

    FunctionDecl* calleeFD = C->getDirectCallee();
    if (!calleeFD) {
	llvm::outs() << "ERROR: Cannot get FunctionDecl of callee from "
		     << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }
    calleeFD = calleeFD->getMostRecentDecl();
    if (!calleeFD) {
	llvm::outs() << "ERROR: Cannot get most recent decl of callee: "
		     << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }

    std::string calleeName = calleeFD->getNameInfo().getAsString();

    // Sanity check: source location of FunctionDecl is in the
    // main program
    SL = SM->getExpansionLoc(calleeFD->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	// If this call expr has decl outside main program, it might be one
	// of the library functions. Skip this
	return true;
    }

    // Sanity check: This is not a custom input function or a custom output
    // function
    std::string fName = calleeFD->getQualifiedNameAsString();
    if (std::find(customInputFuncs.begin(), customInputFuncs.end(), fName) != 
	    customInputFuncs.end()) {
	// This is a custom input function. Skip
	return true;
    }
    if (std::find(customOutputFuncs.begin(), customOutputFuncs.end(), fName) != 
	    customOutputFuncs.end()) {
	// This is a custom output function. Skip
	return true;
    }

    // Get Function Details of this function
    FunctionDetails* calleeFunction = Helper::getFunctionDetailsFromDecl(SM,
	calleeFD, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get FunctionDetails of " << fName << "\n";
	error = true;
	return false;
    }
    for (std::vector<FunctionDetails*>::iterator fIt =
	    mObj->functions.begin(); fIt != mObj->functions.end(); fIt++) {
	if ((*fIt)->equals(calleeFunction)) {
	    calleeFunction = *fIt;
	    break;
	}
    }

    GetSymbolicExprVisitor* funcSym = NULL;
    FunctionAnalysis* funcFA = NULL;
    for (std::vector<FunctionAnalysis*>::iterator aIt = 
	    mObj->analysisInfo.begin(); aIt != mObj->analysisInfo.end(); aIt++) {
	if ((*aIt)->function->equals(calleeFunction)) {
	    funcSym = (*aIt)->symVisitor;
	    funcFA = *aIt;
	}
    }

    if (!funcSym) {
	llvm::outs() << "ERROR: NULL GetSymbolicExprVisitor for callee\n";
	error = true;
	return false;
    }

    int calleeLine = SM->getExpansionLineNumber(C->getLocStart());
#ifdef DEBUG
    llvm::outs() << "DEBUG: calleeLine: " << calleeLine << "\n";
#endif
    StmtMapTy::iterator I = currAnalysisObj->StmtMap.find(C);
    if (I == currAnalysisObj->StmtMap.end()) {
	llvm::outs() << "ERROR: Cannot find entry for expression in StmtMap\n";
	error = true;
	return false;
    }
    std::pair<unsigned, unsigned> offset = 
	std::make_pair(I->second.first, I->second.second);
    // Get the symexprs for global vars at call location
    std::vector<std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> > > 
    globalVarSymExprsAtCallSite;
    for (std::vector<std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> >
	    >::iterator vIt = globalVarSymExprs.begin(); vIt !=
	    globalVarSymExprs.end(); vIt++) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: globalVar:\n";
	vIt->first->prettyPrint();
	llvm::outs() << "\ncalleeLine: " << calleeLine << "\n";
#endif
	VarDetails currVar = vIt->first->var;
	std::vector<SymbolicStmt*> symExprs =
	    getSymExprsOfVarInCurrentBlock(currVar, calleeLine, offset, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While getSymExprsOfVarInCurrentBlock()\n";
	    currVar.print();
	    error = true;
	    return false;
	}
#ifdef DEBUG
	llvm::outs() << "DEBUG: Num of symExprs: " << symExprs.size() << "\n";
	for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); sIt != 
		symExprs.end(); sIt++) {
	    (*sIt)->prettyPrint(true);
	    llvm::outs() << "\n";
	}
#endif
	globalVarSymExprsAtCallSite.push_back(std::make_pair(vIt->first, symExprs));
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: globalVarSymExprsAtCallSite:\n";
    for (std::vector<std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> >
	    >::iterator gvIt = globalVarSymExprsAtCallSite.begin(); gvIt != 
	    globalVarSymExprsAtCallSite.end();  gvIt++) {
	llvm::outs() << "DEBUG: Global Var:\n";
	gvIt->first->prettyPrint();
	llvm::outs() << "\n";
	llvm::outs() << "DEBUG: Num symbolic exprs: " << gvIt->second.size() << "\n";
	for (std::vector<SymbolicStmt*>::iterator sIt = gvIt->second.begin();
		sIt != gvIt->second.end(); sIt++) {
	    (*sIt)->prettyPrint(true);
	    llvm::outs() << "\n";
	}
    }
#endif

    // Replace the dummy global var sym expr in the callee function with the
    // symexprs at the call site
    for (std::vector<std::pair<StmtDetails*, std::vector<SymbolicStmt*> > >::iterator 
	    symIt = funcSym->symExprMap.begin(); symIt != funcSym->symExprMap.end(); symIt++) {
	std::vector<SymbolicStmt*> newSymExprsAfterReplacingGlobalVars;
	for (std::vector<SymbolicStmt*>::iterator stIt = symIt->second.begin(); 
		stIt != symIt->second.end(); stIt++) {
	    std::vector<SymbolicStmt*> tempSymExprs =
		replaceGlobalVarsWithSymExprs(globalVarSymExprsAtCallSite,
		*stIt, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing global vars\n";
		error = true;
		return false;
	    }
	    newSymExprsAfterReplacingGlobalVars.insert(
		newSymExprsAfterReplacingGlobalVars.end(), tempSymExprs.begin(),
		tempSymExprs.end());
	}
	symIt->second = newSymExprsAfterReplacingGlobalVars;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: After replacing global vars in function: " 
		 << currFunction->funcName << "\n";
    funcSym->prettyPrintSymExprMap(true);
#endif

    // Recursively call GlobalVarSymExprVisitor on this function
    GlobalVarSymExprVisitor gvVisitor(this, funcSym, funcFA);
    gvVisitor.setCurrFunction(calleeFunction);
    gvVisitor.setSymExprsForGlobalVars(globalVarSymExprsAtCallSite);

    MainFunction::runVisitorOnInlinedProgram(calleeFunction->cfg, &gvVisitor,
	rv);
    if (gvVisitor.error) {
	llvm::outs() << "ERROR: While running GlobalVarSymExprVisitor on "
		     << "inlined function " << calleeFunction->funcName << "\n";
	error = true;
	return false;
    }

    // TODO:I need to set the sym exprs of global vars after the call to the ones
    // at the exit of the callee function.
    for (std::vector<std::pair<StmtDetails*, std::vector<SymbolicStmt*> > >::iterator 
	    symIt = funcSym->symExprMap.begin(); symIt != funcSym->symExprMap.end(); symIt++) {
	std::vector<SymbolicStmt*> newSymExprsAfterReplacingGlobalVars;
	for (std::vector<SymbolicStmt*>::iterator stIt = symIt->second.begin(); 
		stIt != symIt->second.end(); stIt++) {
	    std::vector<SymbolicStmt*> tempSymExprs =
		replaceGlobalVarsWithSymExprs(gvVisitor.globalVarSymExprsAtExit,
		*stIt, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing global vars\n";
		error = true;
		return false;
	    }
	    newSymExprsAfterReplacingGlobalVars.insert(
		newSymExprsAfterReplacingGlobalVars.end(), tempSymExprs.begin(),
		tempSymExprs.end());
	}
	symIt->second = newSymExprsAfterReplacingGlobalVars;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: After replacing global vars in function: " 
		 << currFunction->funcName << "\n";
    funcSym->prettyPrintSymExprMap(true);
#endif

#ifdef DEBUG
    llvm::outs() << "DEBUG: Returning from GlobalVarSymExprVisitor on inlined function "
		 << calleeFunction->funcName << "\n";
#endif

    return true;
}

std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
GlobalVarSymExprVisitor::getGuardExprsOfCurrentBlock(bool &rv) {
    rv = true;
    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > guardExprs =
	getGuardExprsOfBlock(currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While getGuardExprsOfBlock(" << currBlock << "\n";
	return guardExprs;
    }
    return guardExprs;
}

std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
GlobalVarSymExprVisitor::getGuardExprsOfBlock(unsigned block, bool &rv) {
    rv = true;

    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > guardSymExprs;
    std::vector<std::vector<std::pair<ExprDetails*, bool> > > guardExprs =
	currAnalysisObj->getGuardExprsOfBlock(block, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While getGuardExprsOfBlock(" << block << ")\n";
	return guardSymExprs;
    }

    std::vector<std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > > 
    allGuardExprs;
    for (std::vector<std::vector<std::pair<ExprDetails*, bool> > >::iterator gIt
	    = guardExprs.begin(); gIt != guardExprs.end(); gIt++) {
	std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > allVecs;
	for (std::vector<std::pair<ExprDetails*, bool> >::iterator vIt = 
		gIt->begin(); vIt != gIt->end(); vIt++) {
	    bool resolved = vIt->first->isSymExprCompletelyResolved(symVisitor, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While isSymExprCompletelyResolved on guardExpr\n";
		vIt->first->prettyPrint();
		return guardSymExprs;
	    }
	    if (!resolved) {
		llvm::outs() << "ERROR: guardExpr is not resolved\n";
		vIt->first->prettyPrint();
		rv = false;
		return guardSymExprs;
	    }
	    std::vector<SymbolicStmt*> symExprs =
		symVisitor->getSymbolicExprs(vIt->first, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot obtain symexprs for guardExpr\n";
		vIt->first->prettyPrint();
		return guardSymExprs;
	    }

	    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
	    currGuardExprs;
	    bool empty = (allVecs.size() == 0);
	    for (std::vector<SymbolicStmt*>::iterator gsIt = 
		    symExprs.begin(); gsIt != symExprs.end(); gsIt++) {
		if (!(*gsIt)) {
		    llvm::outs() << "ERROR: NULL SymbolicExpr for guard: ";
		    vIt->first->prettyPrint();
		    llvm::outs() << "\n";
		    rv = false;
		    return guardSymExprs;
		}
		if (!isa<SymbolicExpr>(*gsIt)) {
		    llvm::outs() << "ERROR: Symexpr for guard expr is not <SymbolicExpr>\n";
		    rv = false;
		    return guardSymExprs;
		}
		if (empty) {
		    std::vector<std::pair<SymbolicExpr*, bool> > currVec;
		    currVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(*gsIt),
			vIt->second));
		    currGuardExprs.push_back(currVec);
		} else {
		    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool>
			    > >::iterator allIt = allVecs.begin(); allIt !=
			    allVecs.end(); allIt++) {
			std::vector<std::pair<SymbolicExpr*, bool> > currVec;
			currVec.insert(currVec.end(), allIt->begin(),
			    allIt->end());
			currVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(*gsIt),
			    vIt->second));
			currGuardExprs.push_back(currVec);
		    }
		}
	    }
	    allVecs.clear();
	    allVecs.insert(allVecs.end(), currGuardExprs.begin(),
		currGuardExprs.end());
	}

	bool empty = (allGuardExprs.size() == 0);
	std::vector<std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > >
	tempAllGuardExprs;
	if (empty) {
	    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
		    >::iterator aIt = allVecs.begin(); aIt != allVecs.end();
		    aIt++) {
		std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > 
		currVec;
		currVec.push_back(*aIt);
		tempAllGuardExprs.push_back(currVec);
	    }
	} else {
	    for (std::vector<std::vector<std::vector<std::pair<SymbolicExpr*,
		    bool> > > >::iterator allIt = allGuardExprs.begin(); allIt
		    != allGuardExprs.end(); allIt++) {
		for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
			>::iterator aIt = allVecs.begin(); aIt != allVecs.end();
			aIt++) {
		    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
		    currVec;
		    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool>
			    > >::iterator tIt = allIt->begin();
			    tIt != allIt->end(); tIt++) {
			std::vector<std::pair<SymbolicExpr*, bool> > vec1;
			for (std::vector<std::pair<SymbolicExpr*, bool>
				>::iterator pIt = tIt->begin(); pIt !=
				tIt->end(); pIt++) {
			    vec1.push_back(*pIt);
			}
			currVec.push_back(vec1);
		    }
		    currVec.push_back(*aIt);
		    tempAllGuardExprs.push_back(currVec);
		}
	    }
	}
	allGuardExprs.clear();
	allGuardExprs.insert(allGuardExprs.end(),
	    tempAllGuardExprs.begin(), tempAllGuardExprs.end());
    }

    for (std::vector<std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
	    >::iterator allIt = allGuardExprs.begin(); allIt !=
	    allGuardExprs.end(); allIt++) {
	std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
	appendedGuardExprs = appendGuards(guardSymExprs, *allIt, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While appending guardExprs\n";
	    return guardSymExprs;
	}
    }

    return guardSymExprs;
}

std::vector<SymbolicStmt*>
GlobalVarSymExprVisitor::getSymExprsOfVarInCurrentBlock(VarDetails gv, 
int lineNum, std::pair<unsigned, unsigned> offset, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering GlobalVarSymExprVisitor::getSymExprsOfVarInCurrentBlock()"
		 << " of var ";
    gv.print();
    llvm::outs() << " at line " << lineNum << " and block " << currBlock << "\n";
#endif
    rv = true;
    std::vector<SymbolicStmt*> symExprsOfVar;

    bool isLoopIndex = currAnalysisObj->isLoopIndexVar(gv, currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isLoopIndexVar()";
	gv.print();
	llvm::outs() << "\n";
	return symExprsOfVar;
    }

    if (isLoopIndex) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: The var is a loopIndexVar. Skipping..\n";
#endif
	SymbolicDeclRefExpr* symExpr = new SymbolicDeclRefExpr;
	symExpr->vKind = SymbolicDeclRefExpr::VarKind::LOOPINDEX;
	symExpr->var = gv;
	symExpr->resolved = true;
	symExpr->guards = getGuardExprsOfCurrentBlock(rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While getGuardExprsOfCurrentBlock()\n";
	    return symExprsOfVar;
	}
	SymbolicStmt* cloneVR = symExpr->clone(rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While cloning SymbolicDeclRefExpr: "
			 << "LOOPINDEX to insert in varsReferenced\n";
	    return symExprsOfVar;
	}
	if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
	    llvm::outs() << "ERROR: Clone of SymbolicDeclRefExpr is not "
			 << "SymbolicDeclRefExpr\n";
	    return symExprsOfVar;
	}
	symExpr->varsReferenced.push_back(dyn_cast<SymbolicDeclRefExpr>(cloneVR));
#ifdef DEBUG
	symExpr->prettyPrint(true);
	llvm::outs() << "\n";
#endif
	symExprsOfVar.push_back(symExpr);
	return symExprsOfVar;
    }

    std::vector<Definition> reachDefsOfVar =
	currAnalysisObj->getReachDefsOfVarAtLine(gv, lineNum, currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While getReachDefsOfVarAtLine()\n";
	gv.print();
	llvm::outs() << "\n at line " << lineNum << "\n";
	return symExprsOfVar;
    }

    std::vector<SpecialExpr> spExprDefs;
    for (std::vector<Definition>::iterator dIt = reachDefsOfVar.begin(); dIt != 
	    reachDefsOfVar.end(); dIt++) {
	std::vector<SpecialExpr*>::iterator sIt;
	bool foundSPExpr = false;
	for (sIt = currAnalysisObj->spExprs.begin(); sIt !=
		currAnalysisObj->spExprs.end(); sIt++) {
	    if (dIt->lineNumber == (*sIt)->assignmentLine) {
		foundSPExpr = true;
		break;
	    }
	}
	if (foundSPExpr) {
	    dIt->toDelete = true;
	    spExprDefs.push_back((*sIt)->clone());
	}
    }
    reachDefsOfVar.erase(std::remove_if(reachDefsOfVar.begin(),
	reachDefsOfVar.end(), [](Definition d) { return d.toDelete; }),
	reachDefsOfVar.end());

    for (std::vector<Definition>::iterator dIt = reachDefsOfVar.begin(); dIt !=
	    reachDefsOfVar.end(); dIt++) {
	bool foundSPInit = false;
	std::vector<SpecialExpr>::iterator sIt;
	for (sIt = spExprDefs.begin(); sIt != spExprDefs.end(); sIt++) {
	    if (sIt->initialVal.lineNumber == dIt->lineNumber) {
		foundSPInit = true;
		break;
	    }
	}
	if (foundSPInit) {
	    dIt->toDelete = true;
	}
    }
    reachDefsOfVar.erase(std::remove_if(reachDefsOfVar.begin(),
	reachDefsOfVar.end(), [](Definition d) { return d.toDelete; }),
	reachDefsOfVar.end());

    std::vector<LoopDetails*> loopIndexDefs;
    for (std::vector<Definition>::iterator dIt = reachDefsOfVar.begin(); dIt !=
	    reachDefsOfVar.end(); dIt++) {
	std::string prefix("DP_GLOBAL_VAR");
	auto res = std::mismatch(prefix.begin(), prefix.end(),
	    dIt->expressionStr.begin());
	if (dIt->throughBackEdge && res.first != prefix.end()) {
	    bool isLoopIndex = (currAnalysisObj->isLoopIndexVar(gv,
		dIt->blockID, rv));
	    if (!rv) {
		llvm::outs() << "ERROR: While isLoopIndexVar()\n";
		gv.print();
		llvm::outs() << "\n";
		return symExprsOfVar;
	    }
	    if (isLoopIndex) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: global var is loopindex\n";
#endif
		std::vector<LoopDetails*> loops =
		    currAnalysisObj->getLoopsOfBlock(dIt->blockID, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While getLoopsOfBlock()\n";
		    gv.print();
		    llvm::outs() << "\n";
		    return symExprsOfVar;
		}
		for (std::vector<LoopDetails*>::iterator lIt = loops.begin();
			lIt != loops.end(); lIt++) {
		    if ((*lIt)->loopIndexVar.equals(gv)) {
			if ((*lIt)->hasBreak) {
			    llvm::outs() << "ERROR: Var " << gv.varName 
					 << " at line " << lineNum 
					 << " has definition reaching through a back edge\n";
			    dIt->print();
			    rv = false;
			    return symExprsOfVar;
			}
			dIt->toDelete = true;
			loopIndexDefs.push_back(*lIt);
			break;
		    }
		}
	    } else {
		llvm::outs() << "ERROR: Var " << gv.varName << " at line " 
			     << lineNum << " has defintion reaching through a back edge\n";
		dIt->print();
		rv = false;
		return symExprsOfVar;
	    }
	}
    }

    for (std::vector<Definition>::iterator dIt = reachDefsOfVar.begin(); dIt != 
	    reachDefsOfVar.end(); dIt++) {
	if (dIt->toDelete) continue;
	for (std::vector<LoopDetails*>::iterator lIt = loopIndexDefs.begin();
		lIt != loopIndexDefs.end(); lIt++) {
	    for (std::vector<std::pair<Definition, SymbolicExpr*> >::iterator
		    idIt = (*lIt)->setOfLoopIndexInitValDefs.begin(); idIt != 
		    (*lIt)->setOfLoopIndexInitValDefs.end(); idIt++) {
		if (dIt->equals(idIt->first)) {
		    dIt->toDelete = true;
		    break;
		}
	    }
	    if (dIt->toDelete) break;
	}
    }
    reachDefsOfVar.erase(std::remove_if(reachDefsOfVar.begin(),
	reachDefsOfVar.end(), [](Definition d) { return d.toDelete; }),
	reachDefsOfVar.end());

    reachDefsOfVar.insert(reachDefsOfVar.end(), spExprDefs.begin(), spExprDefs.end());

    for (std::vector<Definition>::iterator dIt = reachDefsOfVar.begin(); 
	    dIt != reachDefsOfVar.end(); dIt++) {
	std::string prefix("DP_GLOBAL_VAR");
	auto res = std::mismatch(prefix.begin(), prefix.end(),
	    dIt->expressionStr.begin());
#ifdef DEBUG
	llvm::outs() << "DEBUG: dIt->expressionStr: " << dIt->expressionStr << "\n";
#endif
	if (!dIt->expression && dIt->expressionStr.compare("DP_INPUT") != 0 &&
		dIt->expressionStr.compare("DP_FUNC_PARAM") != 0 &&
		dIt->expressionStr.compare("DP_UNDEFINED") != 0 &&
		//dIt->expressionStr.compare("DP_GLOBAL_VAR") != 0 &&
		res.first != prefix.end() &&
		!dIt->isSpecialExpr) {
	    llvm::outs() << "ERROR: NULL expression for def:\n";
	    dIt->print();
	    rv = false;
	    return symExprsOfVar;
	}

	if (dIt->expressionStr.compare("DP_INPUT") == 0) {
	    // Def is the input read line
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Definition is the input read line\n";
#endif
	    SymbolicDeclRefExpr* sdre = new SymbolicDeclRefExpr;
	    sdre->var = gv;
	    sdre->vKind = SymbolicDeclRefExpr::VarKind::INPUTVAR;
	    sdre->resolved = true;
	    sdre->guards = getGuardExprsOfBlock(dIt->blockID, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While getGuardExprsOfBlock(" 
			     << dIt->blockID << ")\n";
		return symExprsOfVar;
	    }
	    SymbolicStmt* cloneVR = sdre->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While cloning SymbolicDeclRefExpr: "
			     << "INPUTVAR for varsReferenced\n";
		rv = false;
		return symExprsOfVar;
	    }
	    if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
		llvm::outs() << "ERROR: Clone of SymbolicDeclRefExpr is not "
			     << "SymbolicDeclRefExpr\n";
		rv = false;
		return symExprsOfVar;
	    }
	    sdre->varsReferenced.push_back(dyn_cast<SymbolicDeclRefExpr>(cloneVR));
	    symExprsOfVar.push_back(sdre);
#ifdef DEBUG
	    sdre->prettyPrint(true);
	    llvm::outs() << "\n";
#endif
	} else if (dIt->expressionStr.compare("DP_FUNC_PARAM") == 0) {
	    // Def is the function parameter
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Definition is function parameter\n";
#endif
	    SymbolicDeclRefExpr* sdre = new SymbolicDeclRefExpr;
	    sdre->var = gv;
	    sdre->vKind = SymbolicDeclRefExpr::VarKind::FUNCPARAM;
	    sdre->resolved = true;
	    sdre->guards = getGuardExprsOfBlock(dIt->blockID, rv);
	    SymbolicStmt* cloneVR = sdre->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While cloning SymbolicDeclRefExpr: "
			     << "FUNCPARAM for varsReferenced\n";
		rv = false;
		return symExprsOfVar;
	    }
	    if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
		llvm::outs() << "ERROR: Clone of SymbolicDeclRefExpr is not "
			     << "SymbolicDeclRefExpr\n";
		rv = false;
		return symExprsOfVar;
	    }
	    sdre->varsReferenced.push_back(
		dyn_cast<SymbolicDeclRefExpr>(cloneVR));
	    symExprsOfVar.push_back(sdre);
#ifdef DEBUG
	    sdre->prettyPrint(true);
	    llvm::outs() << "\n";
#endif
	} else if (dIt->expressionStr.compare("DP_GLOBAL_VAR") == 0) {
	    // Def is a global var
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Definition is global var\n";
#endif
	    for (std::vector<
		    std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> > 
		    >::iterator gIt = globalVarSymExprs.begin(); gIt != 
		    globalVarSymExprs.end(); gIt++) {
		if (gIt->first->var.equals(gv)) {
		    symExprsOfVar.insert(symExprsOfVar.end(),
			gIt->second.begin(), gIt->second.end());
		    break;
		}
	    }
	} else if (res.second != dIt->expressionStr.end() && res.second !=
		dIt->expressionStr.begin()) {
	    // Def is a global var after call expr
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Definition is a global var after call expr\n";
#endif
	    // Skip dummy global var def that is from the same location we are
	    // looking at
	    std::stringstream ss;
	    ss << "_B" << offset.first << "." << offset.second;
	    std::string suffix(ss.str());
	    std::string prefix;
#ifdef DEBUG
	    llvm::outs() << "DEBUG: comparing: dIt->expressionStr: " 
			 << dIt->expressionStr << ", suffix: " << suffix << "\n";
#endif
	    if (dIt->expressionStr.length() < suffix.length()) {
		llvm::outs() << "ERROR: Something wrong here!\n";
		rv = false;
		return symExprsOfVar;
	    }
	    if (dIt->expressionStr.compare(
		    dIt->expressionStr.length() - suffix.length(),
		    suffix.length(), suffix) == 0)
		continue;

	    if (globalVarSymExprsAtExit.size() != 0) {
		for (std::vector<std::pair<VarDeclDetails*,
			std::vector<SymbolicStmt*> > >::iterator gIt = 
			globalVarSymExprsAtExit.begin(); gIt != 
			globalVarSymExprsAtExit.end(); gIt++) {
		    if (gIt->first->var.equals(gv)) {
			symExprsOfVar.insert(symExprsOfVar.end(),
			    gIt->second.begin(), gIt->second.end());
			break;
		    }
		}
	    } else {
		SymbolicDeclRefExpr* sdre = new SymbolicDeclRefExpr;
		sdre->var = gv;
		sdre->vKind = SymbolicDeclRefExpr::VarKind::GLOBALVAR;
		sdre->resolved = true;
		if (res.second != dIt->expressionStr.end()) {
		    std::string id(res.second, dIt->expressionStr.end());
		    sdre->callExprID = id;
		}
		sdre->guards = getGuardExprsOfBlock(dIt->blockID, rv);
		SymbolicStmt* cloneVR = sdre->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While cloning SymbolicDeclRefExpr: "
				 << "FUNCPARAM for varsReferenced\n";
		    rv = false;
		    return symExprsOfVar;
		}
		if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
		    llvm::outs() << "ERROR: Clone of SymbolicDeclRefExpr is not "
				 << "SymbolicDeclRefExpr\n";
		    rv = false;
		    return symExprsOfVar;
		}
		sdre->varsReferenced.push_back(
		    dyn_cast<SymbolicDeclRefExpr>(cloneVR));
		symExprsOfVar.push_back(sdre);
#ifdef DEBUG
		sdre->prettyPrint(true);
		llvm::outs() << "\n";
#endif
	    }
	} else if (dIt->expressionStr.compare("DP_UNDEFINED") == 0) {
	    // Def is the uninitialized value of a variable
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Definition is uninitialized\n";
#endif
	    SymbolicDeclRefExpr* sdre = new SymbolicDeclRefExpr;
	    sdre->vKind = SymbolicDeclRefExpr::VarKind::GARBAGE;
	    sdre->resolved = true;
	    sdre->guards = getGuardExprsOfBlock(dIt->blockID, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While getGuardExprsOfBlock(" 
			     << dIt->blockID << ")\n";
		return symExprsOfVar;
	    }
	    symExprsOfVar.push_back(sdre);

#ifdef DEBUG
	    sdre->prettyPrint(true);
	    llvm::outs() << "\n";
#endif
	} else if (dIt->isSpecialExpr) {
	    // Def is a special expr
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Definition is a special expr\n";
#endif
	    SpecialExpr se;
	    bool foundSE = false;
	    for (std::vector<SpecialExpr>::iterator seIt =
		    spExprDefs.begin(); seIt != spExprDefs.end(); seIt++) {
		if (dIt->equals(*seIt)) {
		    se = *seIt;
		    foundSE = true;
		    break;
		}
	    }
	    if (!foundSE) {
		llvm::outs() << "ERROR: Cannot find SpecialExpr "
			     << "corresponding to the Definiton\n";
		rv = false;
		return symExprsOfVar;
	    }

#ifdef DEBUG
	    llvm::outs() << "DEBUG: Special Expression:\n";
	    se.print();
#endif

	    std::vector<VarDetails> loopIndices;
	    std::vector<SymbolicExpr*> initialExprs, finalExprs;
	    std::vector<bool> strictBounds;
	    for (std::vector<LoopDetails*>::iterator lIt = se.loops.begin(); 
		    lIt != se.loops.end(); lIt++) {
		loopIndices.push_back((*lIt)->loopIndexVar);
		(*lIt)->getSymbolicValuesForLoopHeader(SM, symVisitor, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While getSymbolicValuesForLoopHeader()\n";
		    (*lIt)->prettyPrintHere();
		    rv = false;
		    return symExprsOfVar;
		}

		initialExprs.push_back((*lIt)->loopIndexInitValSymExpr);
		if ((*lIt)->strictBound) {
		    SymbolicBinaryOperator* sbo = new SymbolicBinaryOperator;
		    sbo->guards.insert(sbo->guards.end(), 
			(*lIt)->loopIndexFinalValSymExpr->guards.begin(),
			(*lIt)->loopIndexFinalValSymExpr->guards.end());
		    sbo->varsReferenced.insert(sbo->varsReferenced.end(),
			(*lIt)->loopIndexFinalValSymExpr->varsReferenced.begin(),
			(*lIt)->loopIndexFinalValSymExpr->varsReferenced.end());
		    SymbolicIntegerLiteral* sil = new SymbolicIntegerLiteral;
		    llvm::APInt val(32,1);
		    sil->val = val;
		    sbo->lhs = (*lIt)->loopIndexFinalValSymExpr;
		    if ((*lIt)->loopStep == 1)
			sbo->opKind = BinaryOperatorKind::BO_Sub;
		    else if ((*lIt)->loopStep == -1)
			sbo->opKind = BinaryOperatorKind::BO_Add;
		    sbo->rhs = sil;
		    finalExprs.push_back(sbo);
		} else
		    finalExprs.push_back((*lIt)->loopIndexFinalValSymExpr);
		strictBounds.push_back((*lIt)->strictBound);
	    }

	    // Get the init expression of SpecialExpr
	    const Expr* initialE = se.initialVal.expression;
	    if (!initialE) {
		llvm::outs() << "ERROR: NULL initial expression for special expression\n";
		se.print();
	    }

	    Expr* initialExpr = const_cast<Expr*>(initialE);
	    ExprDetails* initialExprDetails = ExprDetails::getObject(SM,
		initialExpr, se.initialVal.blockID, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot obtain ExprDetails of initial "
			     << "expr of special expression\n";
		se.print();
		rv = false;
		return symExprsOfVar;
	    }

	    bool resolved = initialExprDetails->isSymExprCompletelyResolved(symVisitor, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While isSymExprCompletelyResolved() "
			     << "on initialExpr\n";
		rv = false;
		return symExprsOfVar;
	    }
	    if (!resolved)
	    {
		llvm::outs() << "ERROR: Initial expr is not resolved\n";
		llvm::outs() << "Special expr:\n";
		se.print();
		rv = false;
		return symExprsOfVar;
	    }
	    std::vector<SymbolicStmt*> initialSymExprs =
		symVisitor->getSymbolicExprs(initialExprDetails, rv);
		
	    for (std::vector<SymbolicStmt*>::iterator iIt =
		    initialSymExprs.begin(); iIt != initialSymExprs.end();
		    iIt++) {
		if (!isa<SymbolicExpr>(*iIt)) {
		    llvm::outs() << "ERROR: Symbolic expr for initial expr "
				 << "is not <SymbolicExpr>\n";
		    rv = false;
		    return symExprsOfVar;
		}
		std::vector<std::vector<SymbolicExpr*> > allIndexSymExprs;
		for (std::vector<Expr*>::iterator eIt =
			se.arrayIndexExprsAtAssignLine.begin(); 
			eIt != se.arrayIndexExprsAtAssignLine.end(); eIt++) {
		    ExprDetails* indexExprDetails =
			ExprDetails::getObject(SM, *eIt, se.assignmentBlock, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: Cannot obtain ExprDetails "
				     << "from indexExpr: " 
				     << Helper::prettyPrintExpr(*eIt) << "\n";
			rv = false;
			return symExprsOfVar;
		    }

		    resolved = indexExprDetails->isSymExprCompletelyResolved(symVisitor, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While isSymExprCompletelyResolved() "
				     << "on indexExpr\n";
			rv = false;
			return symExprsOfVar;
		    }
		    if (!resolved)
		    {
			llvm::outs() << "ERROR: Index expr is not resolved: "
				     << Helper::prettyPrintExpr(*eIt) << "\n";
			indexExprDetails->prettyPrint();
			rv = false;
			return symExprsOfVar;
		    }

		    std::vector<SymbolicStmt*> indexSymExprs =
			symVisitor->getSymbolicExprs(indexExprDetails, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: Cannot obtain symexprs for "
				     << Helper::prettyPrintExpr(*eIt) << "\n";
			return symExprsOfVar;
		    }

		    bool empty = (allIndexSymExprs.size() == 0);
		    std::vector<std::vector<SymbolicExpr*> >
			currIndexSymExprs;
		    for (std::vector<SymbolicStmt*>::iterator sIt =
			    indexSymExprs.begin(); sIt !=
			    indexSymExprs.end(); sIt++) {
			if (!isa<SymbolicExpr>(*sIt)) {
			    llvm::outs() << "ERROR: Symexpr for index expr "
					 << "is not <SymbolicExpr>\n";
			    rv = false;
			    return symExprsOfVar;
			}

			if (empty) {
			    std::vector<SymbolicExpr*> currVec;
			    currVec.push_back(dyn_cast<SymbolicExpr>(*sIt));
			    currIndexSymExprs.push_back(currVec);
			} else {
			    for (std::vector<std::vector<SymbolicExpr*>
				    >::iterator aIt =
				    allIndexSymExprs.begin(); aIt != 
				    allIndexSymExprs.end(); aIt++) {
				std::vector<SymbolicExpr*> currVec;
				currVec.insert(currVec.end(), aIt->begin(),
				    aIt->end());
				currVec.push_back(dyn_cast<SymbolicExpr>(*sIt));
				currIndexSymExprs.push_back(currVec);
			    }
			}
		    }
		    allIndexSymExprs.clear();
		    allIndexSymExprs.insert(allIndexSymExprs.end(),
			currIndexSymExprs.begin(), currIndexSymExprs.end());
		}

		for (std::vector<std::vector<SymbolicExpr*> >::iterator aIt =
			allIndexSymExprs.begin(); aIt != allIndexSymExprs.end();
			aIt++) {
		    SymbolicSpecialExpr* sse = new SymbolicSpecialExpr;
		    sse->sKind = se.kind;
		    sse->resolved = true;
		    sse->arrayVar = new SymbolicDeclRefExpr(se.arrayVar,
			SymbolicDeclRefExpr::VarKind::ARRAY, true);
		    sse->varsReferenced.push_back(sse->arrayVar);
		    sse->guards = getGuardExprsOfBlock(dIt->blockID, rv);

		    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendedGuardExprs =
			appendGuards(sse->guards, (*iIt)->guards, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While appending guardExprs\n";
			rv = false;
			return symExprsOfVar;
		    }
		    sse->guards.clear();
		    sse->guards.insert(sse->guards.end(),
			appendedGuardExprs.begin(), appendedGuardExprs.end());
		    sse->initExpr = dyn_cast<SymbolicExpr>(*iIt);
		    for (std::vector<SymbolicDeclRefExpr*>::iterator initvrIt =
			    (*iIt)->varsReferenced.begin(); initvrIt != 
			    (*iIt)->varsReferenced.end(); initvrIt++) {
			bool foundVR = false;
			for (std::vector<SymbolicDeclRefExpr*>::iterator
				vrIt = sse->varsReferenced.begin(); vrIt !=
				sse->varsReferenced.end(); vrIt++) {
			    if ((*vrIt)->var.equals((*initvrIt)->var)) {
				foundVR = true;
				break;
			    }
			}
			if (!foundVR)
			    sse->varsReferenced.push_back(*initvrIt);
		    }

		    std::vector<std::pair<std::pair<std::vector<SymbolicExpr*>,
			std::vector<SymbolicExpr*> >, bool> > indexRanges;
		    for (std::vector<SymbolicExpr*>::iterator sIt =
			    aIt->begin(); sIt != aIt->end(); sIt++) {
			std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendedGuardExprs =
			    appendGuards(sse->guards, (*sIt)->guards, rv);
			if (!rv) {
			    llvm::outs() << "ERROR: While appending guardExprs\n";
			    rv = false;
			    return symExprsOfVar;
			}
			sse->guards.clear();
			sse->guards.insert(sse->guards.end(),
			    appendedGuardExprs.begin(), appendedGuardExprs.end());
			sse->indexExprsAtAssignLine.push_back(*sIt);

			if (isa<SymbolicDeclRefExpr>(*sIt)) {
			    SymbolicDeclRefExpr* indexDRE =
				dyn_cast<SymbolicDeclRefExpr>(*sIt);
			    if (indexDRE->vKind ==
				SymbolicDeclRefExpr::VarKind::LOOPINDEX) {
#ifdef DEBUG
				llvm::outs() << "DEBUG: Index is LOOPINDEX\n";
				indexDRE->var.print();
#endif
				// Check if this is a loop index var for the
				// special expr
				std::vector<LoopDetails*>::iterator lIt;
				for (lIt = se.loops.begin(); lIt != 
					se.loops.end(); lIt++) {
#ifdef DEBUG
				    llvm::outs() << "DEBUG: loop index: ";
				    (*lIt)->loopIndexVar.print();
				    llvm::outs() << "\n";
#endif
				    if ((*lIt)->loopIndexVar.equals(indexDRE->var)) {
#ifdef DEBUG
					llvm::outs() << "DEBUG: Index equal to LOOPINDEX\n";
#endif
					std::vector<SymbolicExpr*> initSymExprs, finalSymExprs;
					// Get initial val of index var
					const Expr* constInitialExpression = 
					    (*lIt)->loopIndexInitValDef.expression;
					if (!constInitialExpression) {
					    llvm::outs() << "ERROR: Cannot get initial expression of loop index var ";
					    indexDRE->var.print();	
					    llvm::outs() << "\n";
					    rv = false;
					    return symExprsOfVar;
					}

					Expr* initialExpression = const_cast<Expr*>(constInitialExpression);
					ExprDetails* initialDetails =
					    ExprDetails::getObject(SM,
					    initialExpression,
					    (*lIt)->loopIndexInitValDef.blockID,
					    rv);
					if (!rv) {
					    llvm::outs() << "ERROR: Cannot get ExprDetails from initial expression of loop index var\n";
					    indexDRE->var.print();
					    rv = false;
					    return symExprsOfVar;
					}

					resolved = initialDetails->isSymExprCompletelyResolved(symVisitor, rv);
					if (!rv) {
					    llvm::outs() << "ERROR: While isSymExprCompletelyResolved() "
							 << "on initialExpr\n";
					    rv = false;
					    return symExprsOfVar;
					}
					if (!resolved)
					{
					    llvm::outs() << "ERROR: Initial expression is not resolved\n";
					    initialDetails->prettyPrint();
					    rv = false;
					    return symExprsOfVar;
					}
					std::vector<SymbolicStmt*> isymExprs = symVisitor->getSymbolicExprs(initialDetails, rv);
					if (!rv) {
					    llvm::outs() << "ERROR: Cannot obtain symbolic exprs for intial expression\n";
					    initialDetails->prettyPrint();
					    rv = false;
					    return symExprsOfVar;
					}
					for (std::vector<SymbolicStmt*>::iterator it = isymExprs.begin();
						it != isymExprs.end(); it++) {
					    if (!isa<SymbolicExpr>(*it)) {
						llvm::outs() << "ERROR: Symexpr for initial expression is not <SymbolicExpr>\n";
						rv = false;
						return symExprsOfVar;
					    }
					    initSymExprs.push_back(dyn_cast<SymbolicExpr>(*it));
					}

					// Get final val of index var
					bool strict = (*lIt)->strictBound;
					const Expr* constFinalExpression = 
					    (*lIt)->loopIndexFinalValDef.expression;
					if (!constFinalExpression) {
					    llvm::outs() << "ERROR: Cannot get final expression of loop index var ";
					    indexDRE->var.print();	
					    llvm::outs() << "\n";
					    rv = false;
					    return symExprsOfVar;
					}

					Expr* finalExpression = const_cast<Expr*>(constFinalExpression);
					ExprDetails* finalDetails =
					    ExprDetails::getObject(SM,
					    finalExpression,
					    (*lIt)->loopIndexFinalValDef.blockID,
					    rv);
					if (!rv) {
					    llvm::outs() << "ERROR: Cannot get ExprDetails from final expression of loop index var\n";
					    indexDRE->var.print();
					    rv = false;
					    return symExprsOfVar;
					}

					resolved = finalDetails->isSymExprCompletelyResolved(symVisitor, rv);
					if (!rv) {
					    llvm::outs() << "ERROR: While isSymExprCompletelyResolved() "
							 << "on finalExpr\n";
					    rv = false;
					    return symExprsOfVar;
					}
					if (!resolved)
					{
					    llvm::outs() << "ERROR: Final expression is not resolved\n";
					    finalDetails->prettyPrint();
					    rv = false;
					    return symExprsOfVar;
					}
					std::vector<SymbolicStmt*> fsymExprs = symVisitor->getSymbolicExprs(finalDetails, rv);
					if (!rv) {
					    llvm::outs() << "ERROR: Cannot obtain symbolic exprs for final expression\n";
					    finalDetails->prettyPrint();
					    rv = false;
					    return symExprsOfVar;
					}
					for (std::vector<SymbolicStmt*>::iterator it = fsymExprs.begin();
						it != fsymExprs.end(); it++) {
					    if (!isa<SymbolicExpr>(*it)) {
						llvm::outs() << "ERROR: Symexpr for final expression is not <SymbolicExpr>\n";
						rv = false;
						return symExprsOfVar;
					    }
					    finalSymExprs.push_back(dyn_cast<SymbolicExpr>(*it));
					}
					indexRanges.push_back(std::make_pair(std::make_pair(initSymExprs,
					    finalSymExprs), strict));
#ifdef DEBUG
					llvm::outs() << "Index: ";
					(*sIt)->print();
					llvm::outs() << "Init val: ";
					for (std::vector<SymbolicExpr*>::iterator it = initSymExprs.begin();
						it != initSymExprs.end(); it++)
					    (*it)->prettyPrint(true);
					llvm::outs() << "Final val: ";
					for (std::vector<SymbolicExpr*>::iterator it = finalSymExprs.begin();
						it != finalSymExprs.end(); it++)
					    (*it)->prettyPrint(true);
					llvm::outs() << (strict? "Strict": "Not strict")
						     << "\n";
#endif
					break;
				    }
				}
			    } else {
				std::vector<SymbolicExpr*> initVec, finalVec;
				SymbolicStmt* cloneIndex = (*sIt)->clone(rv);
				if (!rv) {
				    llvm::outs() << "ERROR: While cloning indexExpr\n";
				    rv = false;
				    return symExprsOfVar;
				}
				if (!isa<SymbolicExpr>(cloneIndex)) {
				    llvm::outs() << "ERROR: Clone of SymbolicExpr is not SymbolicExpr\n";
				    rv = false;
				    return symExprsOfVar;
				}
				SymbolicExpr* cloneIndexExpr =
				    dyn_cast<SymbolicExpr>(cloneIndex);
				cloneIndexExpr->replaceVarsWithExprs(loopIndices, initialExprs, rv);
				if (!rv) {
				    llvm::outs() << "ERROR: While replacing vars in indexexpr\n";
				    rv = false;
				    return symExprsOfVar;
				}
				initVec.push_back(cloneIndexExpr);
				cloneIndex = (*sIt)->clone(rv);
				if (!rv) {
				    rv = false;
				    return symExprsOfVar;
				}
				cloneIndexExpr =
				    dyn_cast<SymbolicExpr>(cloneIndex);
				cloneIndexExpr->replaceVarsWithExprs(loopIndices, finalExprs, rv);
				if (!rv) {
				    llvm::outs() << "ERROR: While replacing vars in indexexpr\n";
				    rv = false;
				    return symExprsOfVar;
				}
				finalVec.push_back(cloneIndexExpr);
				// TODO: How do I know if the range is strict or
				// not.
				indexRanges.push_back(std::make_pair(std::make_pair(initVec,
				    finalVec), false));
			    }
			} else {
			    std::vector<SymbolicExpr*> initVec, finalVec;
			    SymbolicStmt* cloneIndex = (*sIt)->clone(rv);
			    if (!rv) {
				llvm::outs() << "ERROR: While cloning indexExpr\n";
				rv = false;
				return symExprsOfVar;
			    }
			    if (!isa<SymbolicExpr>(cloneIndex)) {
				llvm::outs() << "ERROR: Clone of SymbolicExpr is not SymbolicExpr\n";
				rv = false;
				return symExprsOfVar;
			    }
			    SymbolicExpr* cloneIndexExpr =
				dyn_cast<SymbolicExpr>(cloneIndex);
			    cloneIndexExpr->replaceVarsWithExprs(loopIndices, initialExprs, rv);
			    if (!rv) {
				llvm::outs() << "ERROR: While replacing vars in indexexpr\n";
				rv = false;
				return symExprsOfVar;
			    }
			    initVec.push_back(cloneIndexExpr);
			    cloneIndex = (*sIt)->clone(rv);
			    if (!rv) {
				rv = false;
				return symExprsOfVar;
			    }
			    cloneIndexExpr =
				dyn_cast<SymbolicExpr>(cloneIndex);
			    cloneIndexExpr->replaceVarsWithExprs(loopIndices, finalExprs, rv);
			    if (!rv) {
				llvm::outs() << "ERROR: While replacing vars in indexexpr\n";
				rv = false;
				return symExprsOfVar;
			    }
			    finalVec.push_back(cloneIndexExpr);
			    // TODO: How do I know if the range is strict or
			    // not.
			    indexRanges.push_back(std::make_pair(std::make_pair(initVec,
				finalVec), false));
			}
		    }

		    std::vector<std::vector<std::pair<std::pair<SymbolicExpr*,
			SymbolicExpr*>, bool> > > allIndexRanges;
		    SymbolicExpr* NULLExpr;
		    for (std::vector<std::pair<std::pair<std::vector<SymbolicExpr*>,
			    std::vector<SymbolicExpr*> >, bool> >::iterator vIt =
			    indexRanges.begin(); vIt != indexRanges.end();
			    vIt++) {
			std::vector<std::vector<std::pair<std::pair<SymbolicExpr*,
			    SymbolicExpr*>, bool> > > currIndexRanges;
			std::vector<std::vector<std::pair<std::pair<SymbolicExpr*,
			    SymbolicExpr*>, bool> > > currInitAndFinalIndexRanges;
			bool empty = (allIndexRanges.size() == 0);
			for (std::vector<SymbolicExpr*>::iterator initIt =
				vIt->first.first.begin(); initIt !=
				vIt->first.first.end(); initIt++) {
			    if (empty) {
				std::vector<std::pair<std::pair<SymbolicExpr*,
				    SymbolicExpr*>, bool> > currVec;
				currVec.push_back(std::make_pair(std::make_pair(*initIt,
				    NULLExpr), vIt->second));
				currIndexRanges.push_back(currVec);
			    } else {
				for (std::vector<std::vector<std::pair<std::pair<SymbolicExpr*, SymbolicExpr*>,
					bool> > >::iterator aIt = allIndexRanges.begin(); aIt !=
					allIndexRanges.end(); aIt++) {
				    std::vector<std::pair<std::pair<SymbolicExpr*,
					SymbolicExpr*>, bool> > currVec;
				    currVec.insert(currVec.end(),
					aIt->begin(), aIt->end());
				    currVec.push_back(std::make_pair(std::make_pair(*initIt,
					NULLExpr), vIt->second));
				    currIndexRanges.push_back(currVec);
				}
			    }

			    for (std::vector<std::vector<std::pair<std::pair<SymbolicExpr*, SymbolicExpr*>,
				    bool> > >::iterator cIt = currIndexRanges.begin(); cIt !=
				    currIndexRanges.end(); cIt++) {
				for (std::vector<SymbolicExpr*>::iterator
					finalIt = vIt->first.second.begin();
					finalIt != vIt->first.second.end();
					finalIt++) {
				    cIt->rbegin()->first.second = *finalIt;
				    currInitAndFinalIndexRanges.push_back(*cIt);
				}
			    }
			    currIndexRanges.clear();
			}
			allIndexRanges.clear();
			allIndexRanges.insert(allIndexRanges.end(),
			    currInitAndFinalIndexRanges.begin(),
			    currInitAndFinalIndexRanges.end());
		    }

		    for (std::vector<std::vector<std::pair<std::pair<SymbolicExpr*,
			    SymbolicExpr*>, bool> > >::iterator indexIt =
			    allIndexRanges.begin(); indexIt != 
			    allIndexRanges.end(); indexIt++) {

			SymbolicStmt* cloneStmt = sse->clone(rv);
			if (!rv) {
			    llvm::outs() << "ERROR: Cannot obtain clone of "
					 << "SymbolicSpecialExpr\n";
			    rv = false;
			    return symExprsOfVar;
			}
			if (!isa<SymbolicSpecialExpr>(cloneStmt)) {
			    llvm::outs() << "ERROR: Clone of SpecialExpr is "
					 << "not SpecialExpr\n";
			    rv = false;
			    return symExprsOfVar;
			}
			SymbolicSpecialExpr* cloneSSE =
			    dyn_cast<SymbolicSpecialExpr>(cloneStmt);
			for (std::vector<std::pair<std::pair<SymbolicExpr*, SymbolicExpr*>, bool> >::iterator
				rIt = indexIt->begin(); rIt !=
				indexIt->end(); rIt++) {
			    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
				appendGuards(cloneSSE->guardBlocks, rIt->first.first->guardBlocks);
			    cloneSSE->guardBlocks.clear();
			    cloneSSE->guardBlocks.insert(cloneSSE->guardBlocks.end(),
				appendedGuards.begin(), appendedGuards.end());
			    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendedGuardExprs =
				appendGuards(cloneSSE->guards, rIt->first.first->guards, rv);
			    if (!rv) {
				llvm::outs() << "ERROR: While appending guardExprs\n";
				rv = false;
				return symExprsOfVar;
			    }
			    cloneSSE->guards.clear();
			    cloneSSE->guards.insert(cloneSSE->guards.end(),
				appendedGuardExprs.begin(), appendedGuardExprs.end());
			    appendedGuardExprs =
				appendGuards(cloneSSE->guards, rIt->first.second->guards, rv);
			    if (!rv) {
				llvm::outs() << "ERROR: While appending guardExprs\n";
				rv = false;
				return symExprsOfVar;
			    }
			    cloneSSE->guards.clear();
			    cloneSSE->guards.insert(cloneSSE->guards.end(),
				appendedGuardExprs.begin(), appendedGuardExprs.end());
			    cloneSSE->indexRangesAtAssignLine.push_back(*rIt);

			    for (std::vector<SymbolicDeclRefExpr*>::iterator
				    indexVRIt = rIt->first.first->varsReferenced.begin();
				    indexVRIt != rIt->first.first->varsReferenced.end();
				    indexVRIt++) {
				bool foundVR = false;
				for (std::vector<SymbolicDeclRefExpr*>::iterator
					vrIt = cloneSSE->varsReferenced.begin();
					vrIt != cloneSSE->varsReferenced.end();
					vrIt++) {
				    if ((*indexVRIt)->var.equals((*vrIt)->var)) {
					foundVR = true;
					break;
				    }
				}
				if (!foundVR)
				    cloneSSE->varsReferenced.push_back((*indexVRIt));
			    }
			}

			symExprsOfVar.push_back(cloneSSE);
		    }
		}
	    }
	} else {
#ifdef DEBUG
	    dIt->print();
	    llvm::outs() << "DEBUG: Definition is neither the input read "
			 << "line nor a special expr\n";
#endif
	    // Get ExprDetails from clang::Expr
	    ExprDetails* ed = ExprDetails::getObject(SM,
		const_cast<Expr*>(dIt->expression), dIt->blockID, rv);
	    if (!rv) {
		rv = false;
		return symExprsOfVar;
	    }

	    bool resolved = ed->isSymExprCompletelyResolved(symVisitor, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While isSymExprCompletelyResolved() "
			     << "on Expr\n";
		rv = false;
		return symExprsOfVar;
	    }
	    if (!resolved) {
		llvm::outs() << "ERROR: The definition is not resolved\n";
		ed->prettyPrint();
		rv = false;
		return symExprsOfVar;
	    }

	    std::vector<SymbolicStmt*> symExprs = symVisitor->getSymbolicExprs(ed, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot get symbolic exprs of ";
		ed->prettyPrint();
		rv = false;
		return symExprsOfVar;
	    }
	    for (std::vector<SymbolicStmt*>::iterator sIt =
		    symExprs.begin(); sIt != symExprs.end(); sIt++) {
		if (!isa<SymbolicExpr>(*sIt)) {
		    llvm::outs() << "ERROR: Symbolic expr for definition is "
				 << "not <SymbolicExpr>\n";
		    ed->prettyPrint();
		    rv = false;
		    return symExprsOfVar; 
		} 
		(*sIt)->guards = getGuardExprsOfBlock(dIt->blockID, rv);
		symExprsOfVar.push_back(*sIt);
	    }
	}
    }
    return symExprsOfVar;
}

std::vector<SymbolicStmt*>
GlobalVarSymExprVisitor::replaceGlobalVarsWithSymExprs(
std::vector<std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> > > gvSymExprs,
SymbolicStmt* baseStmt, bool &rv) {
    rv = true;
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering GlobalVarSymExprVisitor::replaceGlobalVarsWithSymExprs()\n";
#endif

    std::vector<SymbolicStmt*> replacedStmts;
    if (!baseStmt) {
	llvm::outs() << "ERROR: NULL baseStmt\n";
	return replacedStmts;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Trying to replace global vars in baseStmt: ";
    baseStmt->prettyPrint(true);
    llvm::outs() << "\n";
#endif
    if (gvSymExprs.size() == 0) {
	// If there are no global vars to be replaced, return the original stmt
	replacedStmts.push_back(baseStmt);
	return replacedStmts;
    }

    for (std::vector<std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> >
	    >::iterator gvIt = gvSymExprs.begin(); gvIt != gvSymExprs.end();
	    gvIt++) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Currently looking at global var: ";
	gvIt->first->prettyPrint();
	llvm::outs() << "\nNum symexprs: " << gvIt->second.size() << "\n";
#endif
	// Get SymbolicDeclRefExpr representing globalvar
	SymbolicDeclRefExpr* globalVarExpr = new SymbolicDeclRefExpr;
	globalVarExpr->var = gvIt->first->var;
	globalVarExpr->vKind = SymbolicDeclRefExpr::VarKind::GLOBALVAR;
	globalVarExpr->resolved = true;

	std::vector<SymbolicStmt*> tempReplacedStmts;
	bool empty = (replacedStmts.size() == 0);
	for (std::vector<SymbolicStmt*>::iterator symIt = gvIt->second.begin(); 
		symIt != gvIt->second.end(); symIt++) {
	    if (!isa<SymbolicExpr>(*symIt)) {
		llvm::outs() << "ERROR: SymExpr of global var is not "
			     << "<SymbolicExpr>\n";
		rv = false;
		return replacedStmts;
	    }

	    if (empty) {
		SymbolicStmt* cloneBaseStmt = baseStmt->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While cloning SymbolicStmt: ";
		    baseStmt->prettyPrint(true);
		    llvm::outs() << "\n";
		    return replacedStmts;
		}
		cloneBaseStmt->replaceGlobalVarExprWithExpr(globalVarExpr,
		    dyn_cast<SymbolicExpr>(*symIt), rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While replacing global var expr\n";
		    return replacedStmts;
		}
		std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
		newGuards = appendGuards(cloneBaseStmt->guards,
		    (*symIt)->guards, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While appending guards\n";
		    return replacedStmts;
		}
		cloneBaseStmt->guards.clear();
		cloneBaseStmt->guards.insert(cloneBaseStmt->guards.end(),
		    newGuards.begin(), newGuards.end());
		tempReplacedStmts.push_back(cloneBaseStmt);
	    } else {
		for (std::vector<SymbolicStmt*>::iterator stIt =
			replacedStmts.begin(); stIt != replacedStmts.end();
			stIt++) {
		    SymbolicStmt* cloneRStmt = (*stIt)->clone(rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While cloning SymbolicStmt: ";
			(*stIt)->prettyPrint(true);
			llvm::outs() << "\n";
			return replacedStmts;
		    }
		    cloneRStmt->replaceGlobalVarExprWithExpr(globalVarExpr,
			dyn_cast<SymbolicExpr>(*symIt), rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While replacing global var expr\n";
			return replacedStmts;
		    }
		    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
		    newGuards = appendGuards(cloneRStmt->guards,
			(*symIt)->guards, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While appending guards\n";
			return replacedStmts;
		    }
		    cloneRStmt->guards.clear();
		    cloneRStmt->guards.insert(cloneRStmt->guards.end(),
			newGuards.begin(), newGuards.end());
		    tempReplacedStmts.push_back(cloneRStmt);
		}
	    }
	}
#ifdef DEBUG
	llvm::outs() << "DEBUG: tempReplacedStmts:\n";
	for (std::vector<SymbolicStmt*>::iterator tIt =
		tempReplacedStmts.begin(); tIt != tempReplacedStmts.end();
		tIt++) {
	    (*tIt)->prettyPrint(true);
	    llvm::outs() << "\n";
	}
#endif
	replacedStmts.clear();
	replacedStmts.insert(replacedStmts.end(), tempReplacedStmts.begin(),
	    tempReplacedStmts.end());
#ifdef DEBUG
	llvm::outs() << "DEBUG: replacedStmts:\n";
	for (std::vector<SymbolicStmt*>::iterator tIt =
		replacedStmts.begin(); tIt != replacedStmts.end();
		tIt++) {
	    (*tIt)->prettyPrint(true);
	    llvm::outs() << "\n";
	}
#endif
    }

    return replacedStmts;
}
