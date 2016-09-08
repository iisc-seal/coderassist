#include "Helper.h"
#include "BlockDetails.h"
#include "MainFunction.h"
#include "StmtDetails.h"

void BlockScalarInputStmt::prettyPrint(std::ofstream &logFile, const
MainFunction* mObj, bool &rv, bool inResidual,
std::vector<LoopDetails> surroundingLoops) {
    rv = true;
    prettyPrintGuards(logFile, mObj, rv, true, inResidual, surroundingLoops);
    if (!rv) {
	llvm::outs() << "ERROR: While prettyPrinting guards\n";
	return;
    }
    logFile << ": ";
    if (substituteExpr) {
	substituteExpr->prettyPrintSummary(logFile, mObj, false, rv, true, inResidual,
	    surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyPrinting inputVar\n";
	    return;
	}
    } else {
	for (std::vector<InputVar>::const_iterator iIt = mObj->inputs.begin(); 
		iIt != mObj->inputs.end(); iIt++) {
	    if (inputVar.equals(iIt->var)) {
		if (iIt->substituteArray.array) {
		    // Check if the surrounding loops for this symexpr
		    // matches the substitute's loops
		    if (surroundingLoops.size() >=
			    iIt->substituteArray.loops.size()) {
			for (std::vector<LoopDetails>::iterator slIt =
				surroundingLoops.begin(); slIt != 
				surroundingLoops.end(); slIt++) {
			    unsigned d = std::distance(slIt, surroundingLoops.end());
			    if (d != iIt->substituteArray.loops.size())
				continue;
			    unsigned startingIndex = std::distance(surroundingLoops.begin(), slIt);
			    bool match = true;
			    for (unsigned i = 0; i <
				    iIt->substituteArray.loops.size();
				    i++) {
				if (!(surroundingLoops[startingIndex+i].equals(iIt->substituteArray.loops[i]))) {
				    match = false; break;
				}
			    }
			    if (match) {
				logFile << INPUTNAME << "_" << iIt->progID;
				for (std::vector<SymbolicExpr*>::iterator 
					inIt = iIt->substituteArray.array->indexExprs.begin();
					inIt != iIt->substituteArray.array->indexExprs.end();
					inIt++) {
				    logFile << "[";
				    (*inIt)->prettyPrintSummary(logFile,
					mObj, false, rv, true, 
					inResidual, surroundingLoops);
				    logFile << "]";
				}
				break;
			    }
			}
		    }
		} else {
		    logFile << INPUTNAME << "_" << iIt->progID;
		}
		break;
	    }
	}
    }
    logFile << " = any;\n";
}

std::vector<BlockInputStmt*> BlockInputStmt::getBlockStmt(StmtDetails* st, 
GetSymbolicExprVisitor* visitor, FunctionDetails* f, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering BlockInputStmt::getBlockStmt()\n";
#endif
    rv = true;
    std::vector<BlockInputStmt*> inpStmts;

    std::vector<SymbolicStmt*> symExprs = visitor->getSymbolicExprs(st, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain symbolic exprs of stmt:\n";
	st->print();
	return inpStmts;
    }

    if (symExprs.size() > 1) {
	llvm::outs() << "WARNING: InputStmt has more than one symExpr\n";
    }

    for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); 
	    sIt != symExprs.end(); sIt++) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: SymExpr for inputStmt:\n";
	if (isa<SymbolicExpr>(*sIt))
	    dyn_cast<SymbolicExpr>(*sIt)->prettyPrint(true);
	else
	    (*sIt)->print();
#endif
	std::vector<SymbolicExpr*> argExprs;
	if (isa<SymbolicCXXOperatorCallExpr>(*sIt)) {
	    SymbolicCXXOperatorCallExpr* callExpr =
		dyn_cast<SymbolicCXXOperatorCallExpr>(*sIt);
	    if (callExpr->opKind != CXXOperatorKind::CIN) {
		llvm::outs() << "ERROR: OperatorKind of CXXOperatorCallExpr is "
			     << "not CIN\n";
		rv = false;
		return inpStmts;
	    }
	    argExprs.insert(argExprs.end(), callExpr->callArgExprs.begin(),
		callExpr->callArgExprs.end());
	} else if (isa<SymbolicCallExpr>(*sIt)) {
	    SymbolicCallExpr* callExpr = dyn_cast<SymbolicCallExpr>(*sIt);
	    argExprs.insert(argExprs.end(), callExpr->callArgExprs.begin(),
		callExpr->callArgExprs.end());
	} else if (isa<SymbolicBinaryOperator>(*sIt)) {
	    SymbolicBinaryOperator* inpStmt =
		dyn_cast<SymbolicBinaryOperator>(*sIt);
	    if (inpStmt->opKind != BinaryOperatorKind::BO_Assign) {
		llvm::outs() << "ERROR: BinaryOperator in input stmt is not an "
			     << "assignment statement\n";
		rv = false;
		return inpStmts;
	    }
	    argExprs.push_back(inpStmt->lhs);
	} else if (isa<SymbolicDeclStmt>(*sIt)) {
	    SymbolicDeclStmt* svd = dyn_cast<SymbolicDeclStmt>(*sIt);
	    SymbolicDeclRefExpr* sdre = new SymbolicDeclRefExpr;
	    sdre->var = svd->var->varD.clone();
	    argExprs.push_back(sdre);
	} else {
	    llvm::outs() << "ERROR: Input Stmt is not a CallExpr or a "
			 << "BinaryOperator\n";
	    rv = false;
	    return inpStmts;
	}

	for (std::vector<SymbolicExpr*>::iterator aIt = argExprs.begin();
		aIt != argExprs.end(); aIt++) {
	    if (isa<SymbolicDeclRefExpr>(*aIt) ||
		isa<SymbolicArraySubscriptExpr>(*aIt) ||
		isa<SymbolicUnaryOperator>(*aIt) ||
		isa<SymbolicBinaryOperator>(*aIt)) {
		std::vector<SymbolicExpr*> indexExprs;
		SymbolicDeclRefExpr* sdre = NULL;
		if (isa<SymbolicDeclRefExpr>(*aIt)) {
		    sdre = dyn_cast<SymbolicDeclRefExpr>(*aIt);
		} else if (isa<SymbolicArraySubscriptExpr>(*aIt)) {
		    SymbolicArraySubscriptExpr* sase =
			dyn_cast<SymbolicArraySubscriptExpr>(*aIt);
		    sdre = sase->baseArray;
		    for (std::vector<SymbolicExpr*>::reverse_iterator iIt = 
			    sase->indexExprs.rbegin(); iIt !=
			    sase->indexExprs.rend(); iIt++)
			indexExprs.insert(indexExprs.begin(), *iIt);
		} else if (isa<SymbolicUnaryOperator>(*aIt)) {
		    // Argument to scanf with & operator.
		    SymbolicUnaryOperator* suo =
			dyn_cast<SymbolicUnaryOperator>(*aIt);
		    if (suo->opKind != UnaryOperatorKind::UO_AddrOf) {
			llvm::outs() << "ERROR: Argument to inputStmt is "
				     << "UnaryOperator, but not UO_AddrOf\n";
			rv = false;
			return inpStmts;
		    }
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Argument to scanf with & operator\n";
		    suo->print();
#endif
		    if (isa<SymbolicDeclRefExpr>(suo->opExpr)) {
			sdre = dyn_cast<SymbolicDeclRefExpr>(suo->opExpr);
		    } else if (isa<SymbolicArraySubscriptExpr>(suo->opExpr)) {
			SymbolicArraySubscriptExpr* sase =
			    dyn_cast<SymbolicArraySubscriptExpr>(suo->opExpr);
			sdre = sase->baseArray;
			for (std::vector<SymbolicExpr*>::reverse_iterator iIt = 
				sase->indexExprs.rbegin(); iIt !=
				sase->indexExprs.rend(); iIt++)
			    indexExprs.insert(indexExprs.begin(), *iIt);
		    } else {
			llvm::outs() << "ERROR: Argument to inputStmt is "
				     << "UnaryOperator, but not DeclRefExpr or "
				     << "ArraySubscriptExpr\n";
			rv = false;
			return inpStmts;
		    }
		} else if (isa<SymbolicBinaryOperator>(*aIt)) {
		    // Argument that reads array as "arr + i/ arr[i] + j"
		    SymbolicBinaryOperator* sbo =
			dyn_cast<SymbolicBinaryOperator>(*aIt);
		    if (isa<SymbolicDeclRefExpr>(sbo->lhs) &&
			    isa<SymbolicDeclRefExpr>(sbo->rhs)) {
			SymbolicDeclRefExpr* lhsDeclRefExpr =
			    dyn_cast<SymbolicDeclRefExpr>(sbo->lhs);
			SymbolicDeclRefExpr* rhsDeclRefExpr =
			    dyn_cast<SymbolicDeclRefExpr>(sbo->rhs);
			if (lhsDeclRefExpr->var.arraySizeInfo.size() != 0 && 
			    rhsDeclRefExpr->var.arraySizeInfo.size() == 0) {
			    sdre = lhsDeclRefExpr;
			    indexExprs.push_back(rhsDeclRefExpr);
			} else if (lhsDeclRefExpr->var.arraySizeInfo.size() == 0 && 
			    rhsDeclRefExpr->var.arraySizeInfo.size() != 0) {
			    sdre = rhsDeclRefExpr;
			    indexExprs.push_back(lhsDeclRefExpr);
			}
		    } else if (isa<SymbolicArraySubscriptExpr>(sbo->lhs) &&
			    isa<SymbolicDeclRefExpr>(sbo->rhs)) {
			SymbolicArraySubscriptExpr* lhsArray =
			    dyn_cast<SymbolicArraySubscriptExpr>(sbo->lhs);
			SymbolicDeclRefExpr* rhsDeclRefExpr = 
			    dyn_cast<SymbolicDeclRefExpr>(sbo->rhs);
			sdre = lhsArray->baseArray;
			indexExprs.insert(indexExprs.end(),
			    lhsArray->indexExprs.begin(),
			    lhsArray->indexExprs.end());
			indexExprs.push_back(rhsDeclRefExpr);
		    } else if (isa<SymbolicDeclRefExpr>(sbo->lhs) &&
			    isa<SymbolicArraySubscriptExpr>(sbo->rhs)) {
			SymbolicDeclRefExpr* lhsDeclRefExpr = 
			    dyn_cast<SymbolicDeclRefExpr>(sbo->lhs);
			SymbolicArraySubscriptExpr* rhsArray = 
			    dyn_cast<SymbolicArraySubscriptExpr>(sbo->rhs);
			sdre = rhsArray->baseArray;
			indexExprs.insert(indexExprs.end(),
			    rhsArray->indexExprs.begin(),
			    rhsArray->indexExprs.end());
			indexExprs.push_back(lhsDeclRefExpr);
		    } else {
			llvm::outs() << "ERROR: Argument to inputStmt is "
				     << "BinaryOperator, but not an array read\n";
			rv = false;
			return inpStmts;
		    }
		}

		if (!sdre) {
		    llvm::outs() << "ERROR: sdre not initialized\n";
		    rv = false;
		    return inpStmts;
		}

		while (sdre->substituteExpr) {
		    if (isa<SymbolicDeclRefExpr>(sdre->substituteExpr))
			sdre = dyn_cast<SymbolicDeclRefExpr>(sdre->substituteExpr);
		    else if (isa<SymbolicArraySubscriptExpr>(sdre->substituteExpr)) {
			SymbolicArraySubscriptExpr* sase =
			    dyn_cast<SymbolicArraySubscriptExpr>(sdre->substituteExpr);
			sdre = sase->baseArray;
			for (std::vector<SymbolicExpr*>::reverse_iterator iIt =
				sase->indexExprs.rbegin(); iIt != 
				sase->indexExprs.rend(); iIt++)
			    indexExprs.insert(indexExprs.begin(), *iIt);
		    } else {
			llvm::outs() << "ERROR: SubstituteExpr of "
				     << "SymbolicDeclRefExpr is not SymbolicDeclRefExpr "
				     << "or SymbolicArraySubscriptExpr\n";
			rv = false;
			return inpStmts;
		    }
		}
		if (indexExprs.size() == 0) {
		    BlockScalarInputStmt* bStmt = new BlockScalarInputStmt;
		    bStmt->guards.insert(bStmt->guards.end(),
			(*sIt)->guards.begin(), (*sIt)->guards.end());
		    bStmt->inputVar = sdre->var;
		    bStmt->func = f;
		    bStmt->stmt = st;
		    inpStmts.push_back(bStmt);
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Found input stmt:\n";
		    bStmt->prettyPrintHere();
#endif
		} else {
		    SymbolicArraySubscriptExpr* sase = new SymbolicArraySubscriptExpr;
		    sase->baseArray = sdre;
		    sase->indexExprs.insert(sase->indexExprs.end(),
			indexExprs.begin(), indexExprs.end());
		    BlockArrayInputStmt* bStmt = new BlockArrayInputStmt;
		    bStmt->guards.insert(bStmt->guards.end(),
			(*sIt)->guards.begin(), (*sIt)->guards.end());
		    bStmt->inputArray = sase;
		    bStmt->func = f;
		    bStmt->stmt = st;
		    inpStmts.push_back(bStmt);
		}
	    } else if (isa<SymbolicStringLiteral>(*aIt)) {
		// Found format string argument of scanf. Do nothing.
#ifdef DEBUG
		llvm::outs() << "DEBUG: Found format string:\n";
		(*aIt)->print();
#endif
	    } else {
		llvm::outs() << "ERROR: Argument expr of input stmt is not "
			     << "DeclRefExpr or ArraySubscriptExpr\n";
		(*aIt)->print();
		rv = false;
		return inpStmts;
	    }
	}
    }

    return inpStmts;
}

std::pair<std::vector<BlockInitStmt*>, std::vector<BlockUpdateStmt*> > 
BlockInitStmt::getBlockStmt(StmtDetails* st, GetSymbolicExprVisitor* visitor, 
FunctionDetails* f, bool &rv) {
    rv = true;
    std::vector<BlockInitStmt*> initStmts;
    std::vector<BlockUpdateStmt*> updtStmts;
    std::pair<std::vector<BlockInitStmt*>, std::vector<BlockUpdateStmt*> > p;

    std::vector<SymbolicStmt*> symExprs = visitor->getSymbolicExprs(st, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain symbolic exprs of stmt:\n";
	st->print();
	return p;
    }

    for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); 
	    sIt != symExprs.end(); sIt++) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Symexpr for initStmt\n";
	if (isa<SymbolicExpr>(*sIt))
	    dyn_cast<SymbolicExpr>(*sIt)->prettyPrint(true);
	else
	    (*sIt)->print();
#endif
	if (isa<SymbolicCallExpr>(*sIt)) {
	    SymbolicCallExpr* callExpr = dyn_cast<SymbolicCallExpr>(*sIt);
	    if (!isa<SymbolicDeclRefExpr>(callExpr->callee)) {
		llvm::outs() << "ERROR: callee of callExpr is not <SymbolicDeclRefExpr>\n";
		rv = false;
		return p;
	    }
	    SymbolicDeclRefExpr* callee =
		dyn_cast<SymbolicDeclRefExpr>(callExpr->callee);
	    if (callee->fd->funcName.compare("memset") != 0) {
		llvm::outs() << "ERROR: Symexpr of init stmt callee is not memset\n";
		rv = false;
		return p;
	    }
	    
	    if (callExpr->callArgExprs.size() != 3) {
		llvm::outs() << "ERROR: memset callExpr arg size != 3\n";
		rv = false;
		return p;
	    }
	    if (!isa<SymbolicDeclRefExpr>(callExpr->callArgExprs[0])) {
		llvm::outs() << "ERROR: First arg to memset is not <SymbolicDeclRefExpr>\n";
		rv = false;
		return p;
	    }
	    SymbolicArrayRangeExpr* lhs = new SymbolicArrayRangeExpr;
	    lhs->baseArray = dyn_cast<SymbolicDeclRefExpr>(callExpr->callArgExprs[0]);
	    if (isa<SymbolicUnaryExprOrTypeTraitExpr>(callExpr->callArgExprs[2])) {
		SymbolicUnaryExprOrTypeTraitExpr* size =
		    dyn_cast<SymbolicUnaryExprOrTypeTraitExpr>(callExpr->callArgExprs[2]);
		for (std::vector<SymbolicExpr*>::iterator iIt = size->sizeExprs.begin();
			iIt != size->sizeExprs.end(); iIt++) {
		    std::pair<SymbolicExpr*, SymbolicExpr*> range;
		    //Initial value of range is always 0.
		    SymbolicIntegerLiteral* sil = new SymbolicIntegerLiteral;
		    sil->val = 0;
		    range.first = sil;
		    // Final value of range is size - 1.
		    SymbolicBinaryOperator* sbo = new SymbolicBinaryOperator;
		    sbo->lhs = *iIt;
		    sbo->opKind = BinaryOperatorKind::BO_Sub;
		    sil = new SymbolicIntegerLiteral;
		    sil->val = llvm::APInt(32,1);
		    sbo->rhs = sil;
		    range.second = sbo;
		    lhs->indexRangeExprs.push_back(range);
		}
	    } else {
		// Get the solver to decide whether the third arg is a constant
		// or not
		bool canBeHandled = false;
		z3::context c;
		z3::expr sizeExpr = callExpr->callArgExprs[2]->getZ3Expr(&c,
		    visitor->mainObj, std::vector<LoopDetails>(), false, rv);
		if (!rv) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: I can't get z3 expr of: ";
		    callExpr->callArgExprs[2]->prettyPrint(true);
		    llvm::outs() << "\n";
#endif
		} else {
		    z3::solver s(c);
		    z3::expr v = c.int_const("v");
		    s.add(v == sizeExpr);
		    z3::check_result result = z3::unknown;
		    switch (s.check()) {
		    case z3::sat:
			s.add(v != s.get_model().eval(v));
			result = s.check();
			break;
		    case z3::unsat:
		    case z3::unknown:
			break;
		    }
		    if (result == z3::unsat) {
			// sizeExpr is constant
			// Check if the lhsArray is a 1D array
			if (lhs->baseArray->var.arraySizeInfo.size() == 1) {
			    std::pair<SymbolicExpr*, SymbolicExpr*> range;
			    //Initial value of range is always 0.
			    SymbolicIntegerLiteral* sil = new SymbolicIntegerLiteral;
			    sil->val = 0;
			    range.first = sil;
			    // Final value of range is size - 1.
			    SymbolicBinaryOperator* sbo = new SymbolicBinaryOperator;
			    sbo->lhs = callExpr->callArgExprs[2];
			    sbo->opKind = BinaryOperatorKind::BO_Sub;
			    sil = new SymbolicIntegerLiteral;
			    sil->val = llvm::APInt(32,1);
			    sbo->rhs = sil;
			    range.second = sbo;
			    lhs->indexRangeExprs.push_back(range);
			    canBeHandled = true;
			}
		    }
		}
		
		if (!canBeHandled) {
		    llvm::outs() << "ERROR: Third arg to memset is unsupported\n";
		    rv = false;
		    return p;
		}
	    }

	    BlockInitStmt* init = new BlockInitStmt;
	    init->lhsArray = lhs;
	    // Get initExpr from second argument to memset
	    init->initExpr = callExpr->callArgExprs[1];
	    init->func = f;
	    init->stmt = st;
	    initStmts.push_back(init);
	    continue;
#if 0
	} else if (isa<SymbolicVarDecl>(*sIt)) {
	    SymbolicVarDecl* svd = dyn_cast<SymbolicVarDecl>(*sIt);
	    SymbolicArrayRangeExpr* lhsArray = new SymbolicArrayRangeExpr;
	    SymbolicDeclRefExpr* sdre = new SymbolicDeclRefExpr;
	    sdre->var = svd->var;
	    sdre->vKind = SymbolicDeclRefExpr::VarKind::ARRAY;
	    lhsArray->baseArray = sdre;
	    BlockInitStmt* init = new BlockInitStmt;
	    init->lhsArray = lhsArray;
#endif
	}

	if (!isa<SymbolicBinaryOperator>(*sIt)) {
	    llvm::outs() << "ERROR: Symexpr of init stmt is not BinaryOperator\n";
	    rv = false;
	    return p;
	}
	SymbolicBinaryOperator* sbo = dyn_cast<SymbolicBinaryOperator>(*sIt);
	if (sbo->opKind != BinaryOperatorKind::BO_Assign) {
	    llvm::outs() << "ERROR: Symexpr of init stmt is not assignment\n";
	    rv = false;
	    return p;
	}
	if (!isa<SymbolicArraySubscriptExpr>(sbo->lhs)) {
	    llvm::outs() << "ERROR: LHS of InitStmt is not array\n";
	    rv = false;
	    return p;
	}

	if (isa<SymbolicArraySubscriptExpr>(sbo->rhs)) {
	    // If the input array is reused as update array, then at this point we
	    // have to relabel some init stmts to update stmts
	    SymbolicArraySubscriptExpr* slhs =
		dyn_cast<SymbolicArraySubscriptExpr>(sbo->lhs);
	    SymbolicDeclRefExpr* lhsBase = slhs->baseArray;
	    while (lhsBase->substituteExpr) {
		if (!isa<SymbolicDeclRefExpr>(lhsBase->substituteExpr)) {
		    llvm::outs() << "ERROR: Substitute Expr of ArrayBase is not "
				 << "<SymbolicDeclRefExpr>\n";
		    rv = false;
		    return p;
		}
		lhsBase = dyn_cast<SymbolicDeclRefExpr>(lhsBase->substituteExpr);
	    }
	    SymbolicArraySubscriptExpr* srhs =
		dyn_cast<SymbolicArraySubscriptExpr>(sbo->rhs);
	    SymbolicDeclRefExpr* rhsBase = srhs->baseArray;
	    while (rhsBase->substituteExpr) {
		if (!isa<SymbolicDeclRefExpr>(rhsBase->substituteExpr)) {
		    llvm::outs() << "ERROR: Substitute Expr of ArrayBase is not "
				 << "<SymbolicDeclRefExpr>\n";
		    rv = false;
		    return p;
		}
		rhsBase = dyn_cast<SymbolicDeclRefExpr>(rhsBase->substituteExpr);
	    }
	    if (lhsBase->equals(rhsBase)) {
		BlockUpdateStmt* bStmt = new BlockUpdateStmt;
		bStmt->guards.insert(bStmt->guards.end(),
		    (*sIt)->guards.begin(), (*sIt)->guards.end());
		bStmt->updateStmts.push_back(std::make_pair(slhs, sbo->rhs));
		bStmt->func = f;
		bStmt->stmt = st;
		updtStmts.push_back(bStmt);
		continue;
	    }
	} else {
	    SymbolicArraySubscriptExpr* slhs =
		dyn_cast<SymbolicArraySubscriptExpr>(sbo->lhs);
	    VarDetails lhsVar = slhs->baseArray->var;
	    // Check if the init stmt has guards over dp array
	    bool containsDPVar = false;
	    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
		    >::iterator gIt = (*sIt)->guards.begin(); gIt !=
		    (*sIt)->guards.end(); gIt++) {
		for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator g2It
			= gIt->begin(); g2It != gIt->end(); g2It++) {
		    bool ret = g2It->first->containsVar(lhsVar, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While containsVar()\n";
			return p;
		    }
		    if (ret) {
			containsDPVar = true;
			break;
		    }
		}
		if (containsDPVar) break;
	    }
	    if (containsDPVar) {
		BlockUpdateStmt* bStmt = new BlockUpdateStmt;
		bStmt->guards.insert(bStmt->guards.end(),
		    (*sIt)->guards.begin(), (*sIt)->guards.end());
		bStmt->updateStmts.push_back(std::make_pair(slhs, sbo->rhs));
		bStmt->func = f;
		bStmt->stmt = st;
		updtStmts.push_back(bStmt);
		continue;
	    }
	}

	BlockInitStmt* bStmt = new BlockInitStmt;
	bStmt->guards.insert(bStmt->guards.end(), (*sIt)->guards.begin(),
	    (*sIt)->guards.end());
	bStmt->lhsArray = dyn_cast<SymbolicArraySubscriptExpr>(sbo->lhs);
	bStmt->initExpr = sbo->rhs;
	bStmt->func = f;
	bStmt->stmt = st;
	initStmts.push_back(bStmt);
    }

    p.first = initStmts;
    p.second = updtStmts;
    return p;
}

std::vector<BlockUpdateStmt*> BlockUpdateStmt::getBlockStmt(StmtDetails* st,
GetSymbolicExprVisitor* visitor, FunctionDetails* f, bool &rv) {
    rv = true;
    std::vector<BlockUpdateStmt*> updtStmts;

    std::vector<SymbolicStmt*> symExprs = visitor->getSymbolicExprs(st, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain symbolic exprs of stmt:\n";
	st->print();
	return updtStmts;
    }

    for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); 
	    sIt != symExprs.end(); sIt++) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Symexpr for updtStmt\n";
	if (isa<SymbolicExpr>(*sIt))
	    dyn_cast<SymbolicExpr>(*sIt)->prettyPrint(true);
	else
	    (*sIt)->print();
#endif
	if (!isa<SymbolicBinaryOperator>(*sIt)) {
	    llvm::outs() << "ERROR: Symexpr of update stmt is not BinaryOperator\n";
	    rv = false;
	    return updtStmts;
	}
	SymbolicBinaryOperator* sbo = dyn_cast<SymbolicBinaryOperator>(*sIt);
	if (sbo->opKind != BinaryOperatorKind::BO_Assign) {
	    llvm::outs() << "ERROR: Symexpr of update stmt is not assignment\n";
	    rv = false;
	    return updtStmts;
	}
	if (!isa<SymbolicArraySubscriptExpr>(sbo->lhs)) {
	    llvm::outs() << "ERROR: LHS of UpdateStmt is not array\n";
	    rv = false;
	    return updtStmts;
	}

	BlockUpdateStmt* bStmt = new BlockUpdateStmt;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Guards of update stmt:\n";
	(*sIt)->printGuardExprs();
#endif
	bStmt->guards.insert(bStmt->guards.end(), (*sIt)->guards.begin(),
	    (*sIt)->guards.end());
	bStmt->updateStmts.push_back(std::make_pair(
	    dyn_cast<SymbolicArraySubscriptExpr>(sbo->lhs), sbo->rhs));
	bStmt->func = f;
	bStmt->stmt = st;
	updtStmts.push_back(bStmt);
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: updtStmts identified: " << updtStmts.size() << "\n";
    for (std::vector<BlockUpdateStmt*>::iterator uIt = updtStmts.begin(); uIt != 
	    updtStmts.end(); uIt++) {
	(*uIt)->prettyPrintHere();
    }
#endif
    return updtStmts;
}

std::vector<BlockOutputStmt*> BlockOutputStmt::getBlockStmt(StmtDetails* st,
GetSymbolicExprVisitor* visitor, FunctionDetails* f, bool &rv) {
    rv = true;
    std::vector<BlockOutputStmt*> outStmts;

    bool isPrintf = false, isPutchar = false;
    if (isa<CallExprDetails>(st)) {
	CallExprDetails* call = dyn_cast<CallExprDetails>(st);
	if (call->callee) {
	    if (isa<DeclRefExprDetails>(call->callee)) {
		DeclRefExprDetails* calleedre =
		    dyn_cast<DeclRefExprDetails>(call->callee);
		if (calleedre->rKind == DeclRefKind::LIB) {
		    if (calleedre->fd->funcName.compare("printf") == 0)
			isPrintf = true;
		    else if (calleedre->fd->funcName.compare("putchar") == 0)
			isPutchar = true;
		}
	    }
	}
    }

    std::vector<SymbolicStmt*> symExprs = visitor->getSymbolicExprs(st, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain symbolic exprs of stmt:\n";
	if (st->origStmt)
	    llvm::outs() << Helper::prettyPrintStmt(st->origStmt) << "\n";
	else
	    st->print();
	return outStmts;
    }

    for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); 
	    sIt != symExprs.end(); sIt++) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Symexpr for outStmt\n";
	if (isa<SymbolicExpr>(*sIt))
	    dyn_cast<SymbolicExpr>(*sIt)->prettyPrint(true);
	else
	    (*sIt)->print();
#endif
	std::vector<SymbolicExpr*> argExprs;
	if (isa<SymbolicCXXOperatorCallExpr>(*sIt)) {
	    SymbolicCXXOperatorCallExpr* scce =
		dyn_cast<SymbolicCXXOperatorCallExpr>(*sIt);
	    argExprs.insert(argExprs.end(), scce->callArgExprs.begin(),
		scce->callArgExprs.end());
	} else if (isa<SymbolicCallExpr>(*sIt)) {
	    SymbolicCallExpr* sce = dyn_cast<SymbolicCallExpr>(*sIt);
#if 0
	    // Check if the call is to printf. If so, skip first argument
	    // (format string)
	    bool isPrintf = false;
	    if (isa<SymbolicDeclRefExpr>(sce->callee)) {
		SymbolicDeclRefExpr* callee =
		    dyn_cast<SymbolicDeclRefExpr>(sce->callee);
		if (callee->vKind != SymbolicDeclRefExpr::VarKind::FUNC) {
		    llvm::outs() << "ERROR: Callee of SymbolicCallExpr is not FUNC\n";
		    callee->print();
		    rv = false;
		    return outStmts;
		}
		if (callee->fd.funcName.compare("printf") == 0)
		    isPrintf = true;
	    } else {
		llvm::outs() << "ERROR: Callee of SymbolicCallExpr is not SymbolicDeclRefExpr\n";
		rv = false;
		return outStmts;
	    }
#endif
	    if (isPrintf && sce->callArgExprs.size() > 1) {
		// If printf has only one argument, there is no format string,
		// so do not skip
		argExprs.insert(argExprs.end(), sce->callArgExprs.begin()+1,
		    sce->callArgExprs.end());
	    } else if (isPutchar) {
		if (sce->callArgExprs.size() != 1) {
		    llvm::outs() << "ERROR: More than one args for putchar\n";
		    rv = false;
		    return outStmts;
		}
		SymbolicExpr* arg = sce->callArgExprs[0];
		if (isa<SymbolicBinaryOperator>(arg)) {
		    SymbolicBinaryOperator* abo =
			dyn_cast<SymbolicBinaryOperator>(arg);
		    if (isa<SymbolicIntegerLiteral>(abo->rhs) && abo->opKind ==
			    BinaryOperatorKind::BO_Add) {
			SymbolicIntegerLiteral* sil =
			    dyn_cast<SymbolicIntegerLiteral>(abo->rhs);
			if (sil->val == 48) argExprs.push_back(abo->lhs);
			else argExprs.push_back(arg);
		    } else {
			argExprs.push_back(arg);
		    }
		} else {
		    argExprs.push_back(arg);
		}
	    } else {
		argExprs.insert(argExprs.end(), sce->callArgExprs.begin(),
		    sce->callArgExprs.end());
	    }
	} else {
	    llvm::outs() << "ERROR: SymbolicExpr of output stmt is not a CallExpr\n";
	    rv = false;
	    return outStmts;
	}
	for (std::vector<SymbolicExpr*>::iterator aIt = argExprs.begin();
		aIt != argExprs.end(); aIt++) {
	    BlockOutputStmt* bStmt = new BlockOutputStmt;
	    bStmt->guards.insert(bStmt->guards.end(), (*sIt)->guards.begin(),
		(*sIt)->guards.end());
	    bStmt->outExpr = *aIt;
	    bStmt->func = f;
	    bStmt->stmt = st;
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Output argExpr:\n";
	    bStmt->outExpr->print();
#endif
	    outStmts.push_back(bStmt);
	}
    }

    return outStmts;
}

bool BlockDetails::convertBlockToRangeExpr(bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering BlockDetails::convertBlockToRangeExpr\n";
#endif
    rv = true;
    if (!isInit()) {
	llvm::outs() << "ERROR: Trying to convert input/update/output block to "
		     << "rangeExpr\n";
	prettyPrintHere();
	rv = false;
	return false;
    }
    if (loops.size() == 0) return false;
    std::vector<VarDetails> loopIndexVars;
    std::vector<SymbolicExpr*> initialExprs;
    std::vector<SymbolicExpr*> finalExprs;
    std::vector<bool> direction;

    for (std::vector<LoopDetails>::iterator lIt = loops.begin(); lIt != 
	    loops.end(); lIt++) {
#ifdef DEBUG
        llvm::outs() << "DEBUG: loopIndexVar: ";
	lIt->loopIndexVar.print();
	llvm::outs() << "\n";
#endif
	loopIndexVars.push_back(lIt->loopIndexVar);
	if (lIt->loopIndexInitValSymExpr) {
	    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt = 
		    lIt->loopIndexInitValSymExpr->varsReferenced.begin(); vrIt
		    != lIt->loopIndexInitValSymExpr->varsReferenced.end();
		    vrIt++) {
		for (std::vector<LoopDetails>::iterator vIt = loops.begin();
			vIt != loops.end(); vIt++) {
		    if ((*vrIt)->var.equals(vIt->loopIndexVar)) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: loop bound is a loop index var\n";
			llvm::outs() << "VarRef: ";
			(*vrIt)->var.print(); llvm::outs() << "\n";
			llvm::outs() << "loopIndexVar: ";
			lIt->loopIndexVar.print(); llvm::outs() << "\n";
#endif
			return false;
		    }
		}
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: loopIndexInitValSymExpr: ";
	    lIt->loopIndexInitValSymExpr->prettyPrint(false);
	    llvm::outs() << "\n";
#endif
	    initialExprs.push_back(lIt->loopIndexInitValSymExpr);
	} else {
	    llvm::outs() << "ERROR: NULL loopIndexInitValSymExpr\n";
	    rv = false;
	    return false;
	}
	if (lIt->loopIndexFinalValSymExpr) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: loopIndexFinalValSymExpr: ";
	    lIt->loopIndexFinalValSymExpr->prettyPrint(false);
	    llvm::outs() << "\n";
#endif
	    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt = 
		    lIt->loopIndexFinalValSymExpr->varsReferenced.begin(); vrIt
		    != lIt->loopIndexFinalValSymExpr->varsReferenced.end();
		    vrIt++) {
		for (std::vector<LoopDetails>::iterator vIt = loops.begin();
			vIt != loops.end(); vIt++) {
		    if ((*vrIt)->var.equals(vIt->loopIndexVar)) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: loop bound is a loop index var\n";
			llvm::outs() << "VarRef: ";
			(*vrIt)->var.print(); llvm::outs() << "\n";
			llvm::outs() << "loopIndexVar: ";
			lIt->loopIndexVar.print(); llvm::outs() << "\n";
#endif
		 	return false;
		    }
		}
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: loopIndexFinalValSymExpr: ";
	    lIt->loopIndexFinalValSymExpr->prettyPrint(false);
	    llvm::outs() << "\n";
#endif
	    if (lIt->strictBound) {
		SymbolicBinaryOperator* sbo = new SymbolicBinaryOperator;
		sbo->guards.insert(sbo->guards.end(),
		    lIt->loopIndexFinalValSymExpr->guards.begin(),
		    lIt->loopIndexFinalValSymExpr->guards.end());
		sbo->varsReferenced.insert(sbo->varsReferenced.end(),
		    lIt->loopIndexFinalValSymExpr->varsReferenced.begin(),
		    lIt->loopIndexFinalValSymExpr->varsReferenced.end());
		SymbolicIntegerLiteral* sil = new SymbolicIntegerLiteral;
		llvm::APInt val(32,1);
		sil->val = val;
		sbo->lhs = lIt->loopIndexFinalValSymExpr;
		if (lIt->loopStep == 1)
		    sbo->opKind = BinaryOperatorKind::BO_Sub;
		else if (lIt->loopStep == -1)
		    sbo->opKind = BinaryOperatorKind::BO_Add;
		sbo->rhs = sil;
		finalExprs.push_back(sbo);
	    } else
		finalExprs.push_back(lIt->loopIndexFinalValSymExpr);
	} else {
	    llvm::outs() << "ERROR: NULL loopIndexFinalValSymExpr\n";
	    rv = false;
	    return false;
	}
	if (lIt->loopStep == 1)
	    direction.push_back(true);
	else if (lIt->loopStep == -1)
	    direction.push_back(false);
    }

    for (std::vector<BlockStmt*>::iterator sIt = stmts.begin(); sIt !=
	    stmts.end(); sIt++) {
	if ((*sIt)->isInput()) {
	    if ((*sIt)->guards.size() != 0) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: input stmt has guards. Not "
			     << "converting to range expr\n";
#endif
		//continue;
		return false;
	    }
	    if (isa<BlockScalarInputStmt>(*sIt)) {
		if (loops.size() != 0) {
		    llvm::outs() << "WARNING: Scalar input inside loops\n";
		}
		//continue;
		return false;
	    }
	    BlockArrayInputStmt* bi = dyn_cast<BlockArrayInputStmt>(*sIt);
	    SymbolicArrayRangeExpr* sare = new SymbolicArrayRangeExpr;
	    SymbolicStmt* arr = bi->inputArray->baseArray->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot clone SymbolicDeclRefExpr\n";
		return false;
	    }
	    if (!isa<SymbolicDeclRefExpr>(arr)) {
		llvm::outs() << "ERROR: Clone of SymbolicDeclRefExpr is not "
			     << "<SymbolicDeclRefExpr>\n";
		llvm::outs() << "OriginalDeclRefExpr: ";
		bi->inputArray->print();
		llvm::outs() << "Clone: ";
		arr->print();
		rv = false;
		return false;
	    }
	    sare->baseArray = dyn_cast<SymbolicDeclRefExpr>(arr);
	    unsigned i = 0;
	    for (std::vector<SymbolicExpr*>::iterator indexIt =
		    bi->inputArray->indexExprs.begin(); indexIt != 
		    bi->inputArray->indexExprs.end(); indexIt++) {
		std::pair<SymbolicExpr*, SymbolicExpr*> indexRange;
		SymbolicStmt* cloneIndex = (*indexIt)->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: Cannot clone SymbolicExpr\n";
		    return false;
		}
		if (!isa<SymbolicExpr>(cloneIndex)) {
		    llvm::outs() << "ERROR: Clone of SymbolicExpr is not "
				 << "<SymbolicExpr>\n";
		    rv = false;
		    return false;
		}
		SymbolicExpr* cloneIndexExpr =
		    dyn_cast<SymbolicExpr>(cloneIndex);
		cloneIndexExpr->replaceVarsWithExprs(loopIndexVars,
		    initialExprs, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While replacing vars in indexExpr\n";
		    return false;
		}
		if (direction[i])
		    indexRange.first = cloneIndexExpr;
		else
		    indexRange.second = cloneIndexExpr;
		cloneIndex = (*indexIt)->clone(rv);
		if (!rv) return false;
		cloneIndexExpr = dyn_cast<SymbolicExpr>(cloneIndex);
		cloneIndexExpr->replaceVarsWithExprs(loopIndexVars,
		    finalExprs, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While replacing vars in indexExpr\n";
		    return false;
		}
		if (direction[i])
		    indexRange.second = cloneIndexExpr;
		else
		    indexRange.first = cloneIndexExpr;
		sare->indexRangeExprs.push_back(indexRange);
		i++;
	    }
	    bi->inputArray = sare;
	} else if ((*sIt)->isInit()) {
	    if ((*sIt)->guards.size() != 0) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: init stmt has guards. Not "
			     << "converting to range expr\n";
#endif
		//continue;
		return false;
	    }
	    BlockInitStmt* bi = dyn_cast<BlockInitStmt>(*sIt);
	    if (isa<SymbolicArraySubscriptExpr>(bi->initExpr)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: RHS of init is ArraySubscriptExpr. "
			     << "Cannot convert to range expr\n";
#endif
		return false;
	    }
	    // Check if more than one array indices are both dependent on the same loop
	    // index variable
	    for (std::vector<VarDetails>::iterator lIt = loopIndexVars.begin();
		    lIt != loopIndexVars.end(); lIt++) {
		unsigned numTimesLoopIndexVarFound = 0;
		for (std::vector<SymbolicExpr*>::iterator iIt =
			bi->lhsArray->indexExprs.begin(); iIt != 
			bi->lhsArray->indexExprs.end(); iIt++) {
		    bool contains = (*iIt)->containsVar(*lIt, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While checking if loop index variable "
				     << lIt->varName << " in expr: ";
			(*iIt)->prettyPrint(true);
			return false;
		    }
		    if (contains) {
			numTimesLoopIndexVarFound++;
			if (numTimesLoopIndexVarFound > 1) {
#ifdef DEBUG
			    llvm::outs() << "DEBUG: Array Indices of LHS array "
					 << "init statement are dependent on "
					 << "loop index variables\n";
#endif
			    return false;
			}
		    }
		}
	    }

	    SymbolicArrayRangeExpr* sare = new SymbolicArrayRangeExpr;
	    SymbolicStmt* arr = bi->lhsArray->baseArray->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot clone SymbolicDeclRefExpr\n";
		return false;
	    }
	    if (!isa<SymbolicDeclRefExpr>(arr)) {
		llvm::outs() << "ERROR: Clone of SymbolicDeclRefExpr is not "
			     << "<SymbolicDeclRefExpr>\n";
		llvm::outs() << "OriginalDeclRefExpr: ";
		bi->lhsArray->print();
		llvm::outs() << "Clone: ";
		arr->print();
		rv = false;
		return false;
	    }
	    sare->baseArray = dyn_cast<SymbolicDeclRefExpr>(arr);
	    for (std::vector<SymbolicExpr*>::iterator indexIt =
		    bi->lhsArray->indexExprs.begin(); indexIt != 
		    bi->lhsArray->indexExprs.end(); indexIt++) {
		std::pair<SymbolicExpr*, SymbolicExpr*> indexRange;
		SymbolicStmt* cloneIndex = (*indexIt)->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: Cannot clone SymbolicExpr\n";
		    return false;
		}
		if (!isa<SymbolicExpr>(cloneIndex)) {
		    llvm::outs() << "ERROR: Clone of SymbolicExpr is not "
				 << "<SymbolicExpr>\n";
		    rv = false;
		    return false;
		}
		SymbolicExpr* cloneIndexExpr =
		    dyn_cast<SymbolicExpr>(cloneIndex);
		cloneIndexExpr->replaceVarsWithExprs(loopIndexVars,
		    initialExprs, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While replacing vars in indexExpr\n";
		    return false;
		}
		indexRange.first = cloneIndexExpr;
		cloneIndex = (*indexIt)->clone(rv);
		if (!rv) return false;
		cloneIndexExpr = dyn_cast<SymbolicExpr>(cloneIndex);
		cloneIndexExpr->replaceVarsWithExprs(loopIndexVars,
		    finalExprs, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While replacing vars in indexExpr\n";
		    return false;
		}
		indexRange.second = cloneIndexExpr;
		sare->indexRangeExprs.push_back(indexRange);
	    }
	    bi->lhsArray = sare;
	} else {
	    llvm::outs() << "ERROR: Stmt in block is not input or init\n";
	    (*sIt)->prettyPrintHere();
	    rv = false;
	    return false;
	}
    }

    loops.clear();
    return true;
}

// TODO: Should do soundness checks while doing this. 
std::vector<BlockDetails*> BlockDetails::getSameVarBlocks(bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering BlockDetails::getSameVarBlocks()\n";
#endif
    rv = true;
    std::vector<BlockDetails*> sameVarBlocks;
    std::vector<std::string> varsProcessed;
    std::string currVarID, varInProcess;
    BlockDetails* newBlock = NULL;
    for (std::vector<BlockStmt*>::iterator sIt = stmts.begin(); sIt != 
	    stmts.end(); sIt++) {
	varInProcess = (*sIt)->getVarIDofStmt(rv);
	if (!rv) return sameVarBlocks;

	// Skip if the curr var is already processed
	if (std::find(varsProcessed.begin(), varsProcessed.end(), varInProcess)
		!= varsProcessed.end())
	    continue;

	newBlock = clone(rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot clone BlockDetails\n";
	    return sameVarBlocks;
	}
	newBlock->stmts.clear();
#ifdef DEBUG
	llvm::outs() << "DEBUG: Skeleton newBlock:\n";
	newBlock->prettyPrintHere();
#endif

	varsProcessed.push_back(varInProcess);
#ifdef DEBUG
	llvm::outs() << "DEBUG: Looking for block with " << varInProcess << "\n";
#endif
	for (std::vector<BlockStmt*>::iterator cIt = stmts.begin(); cIt != 
		stmts.end(); cIt++) {
	    currVarID = (*cIt)->getVarIDofStmt(rv);
	    if (!rv) return sameVarBlocks;

	    if (currVarID.compare(varInProcess) != 0)
		continue;

	    BlockStmt* newStmt = (*cIt)->clone(rv);
	    if (!rv) return sameVarBlocks;
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Inserting stmt into newBlock:\n";
	    newStmt->prettyPrintHere();
#endif
	    newBlock->stmts.push_back(newStmt);
	}
	newBlock->setLabel(rv);
	if (!rv) return sameVarBlocks;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Obtained new Block:\n";
	newBlock->prettyPrintHere();
#endif

	sameVarBlocks.push_back(newBlock);
    }

    return sameVarBlocks;
}

std::string BlockStmt::getVarIDofStmt(bool &rv) {
    rv = true;
    
    if (isa<BlockScalarInputStmt>(this)) {
	return dyn_cast<BlockScalarInputStmt>(this)->inputVar.getVarID();
    } else if (isa<BlockArrayInputStmt>(this)) {
	return dyn_cast<BlockArrayInputStmt>(this)->inputArray->baseArray->var.getVarID();
    } else if (isa<BlockInitStmt>(this)) {
	return dyn_cast<BlockInitStmt>(this)->lhsArray->baseArray->var.getVarID();
    } else if (isa<BlockUpdateStmt>(this)) {
	return dyn_cast<BlockUpdateStmt>(this)->updateStmts[0].first->baseArray->var.getVarID();
    } else if (isa<BlockOutputStmt>(this)) {
	llvm::outs() << "ERROR: Cannot get varID in Output Stmt\n";
	rv = false;
	return "";
    } else {
	llvm::outs() << "ERROR: Unknown Stmt Type\n";
	rv = false;
	return "";
    }
}

void BlockStmt::print() {
    llvm::outs() << "Block Stmt:\n";
    if (!stmt) {
	llvm::outs() << "NULL stmt\n";
    } else {
	stmt->print();
    }
}

void BlockStmt::prettyPrint(std::ofstream &logFile, 
const MainFunction* mObj, bool &rv, bool inResidual,
std::vector<LoopDetails> surroundingLoops) {
    rv = true;
    llvm::outs() << "Block Stmt:\n";
    if (!stmt) {
	llvm::outs() << "NULL stmt\n";
    } else {
	stmt->print();
    }
}

void BlockUpdateStmt::prettyPrintHere() {
    if (func)
	llvm::outs() << func->funcName << "()\n";
    else
	llvm::outs() << "NULL func\n";
    prettyPrintGuardsHere();
    for (std::vector<std::pair<SymbolicArraySubscriptExpr*, SymbolicExpr*>
	    >::iterator sIt = updateStmts.begin(); sIt != updateStmts.end();
	    sIt++) {
	sIt->first->prettyPrint(false);
	llvm::outs() << " = ";
	sIt->second->prettyPrint(false);
	if (stmt)
	    llvm::outs() << "; [" << stmt->lineNum << "]\n";
	else
	    llvm::outs() << ";\n";
    }
}

z3::expr BlockStmt::getZ3ExprForGuard(z3::context* c, const MainFunction* mObj,
std::vector<LoopDetails> surroundingLoops, bool inputFirst, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering BlockStmt::getZ3ExprForGuard()\n";
#endif
    rv = true;
    z3::expr guardExpr(*c);
    if (guards.size() == 0) {
	return c->bool_val(true);
    }
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator 
	    gIt = guards.begin(); gIt != guards.end(); gIt++) {
	z3::expr conjunct(*c);
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gcIt
		= gIt->begin(); gcIt != gIt->end(); gcIt++) {
	    z3::expr gExpr = gcIt->first->getZ3Expr(c, mObj, surroundingLoops,
		inputFirst, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While getZ3Expr() on ";
		gcIt->first->prettyPrint(false);
		return guardExpr;
	    }
	    if (!gcIt->second) {
		if (gExpr.is_bool())
		    gExpr = !gExpr;
		else if (gExpr.is_arith())
		    gExpr = (gExpr == 0);
		else {
		    llvm::outs() << "ERROR: Unsupported sort in guard\n";
		    gcIt->first->prettyPrint(false);
		    rv = false;
		    return guardExpr;
		}
	    }
	    if (gcIt == gIt->begin()) {
		if (gExpr.is_bool())
		    conjunct = gExpr;
		else if (gExpr.is_arith())
		    conjunct = (gExpr != 0);
		else {
		    llvm::outs() << "ERROR: Unsupported sort in guard\n";
		    gcIt->first->prettyPrint(false);
		    rv = false;
		    return guardExpr;
		}
	    } else {
		if (gExpr.is_bool())
		    conjunct = conjunct && gExpr;
		else if (gExpr.is_arith())
		    conjunct = conjunct && (gExpr != 0);
		else {
		    llvm::outs() << "ERROR: Unsupported sort in guard\n";
		    gcIt->first->prettyPrint(false);
		    rv = false;
		    return guardExpr;
		}
	    }
	}
	if (gIt == guards.begin()) {
	    if (conjunct.is_arith())
		guardExpr = (conjunct != 0);
	    else if (conjunct.is_bool())
		guardExpr = conjunct;
	    else {
		llvm::outs() << "ERROR: Unsupported sort in guard\n";
		rv = false;
		return guardExpr;
	    }
	} else {
	    if (conjunct.is_arith())
		guardExpr = guardExpr || (conjunct != 0);
	    else if (conjunct.is_bool())
		guardExpr = guardExpr || conjunct;
	    else {
		llvm::outs() << "ERROR: Unsupported sort in guard\n";
		rv = false;
		return guardExpr;
	    }
	}
    }
    return guardExpr;
}

z3::expr BlockDetails::getLoopBoundsAsZ3Expr(z3::context* c, const MainFunction* mObj,
std::vector<LoopDetails> surroundingLoops, bool inputFirst, bool &rv) {
    rv = true;
    z3::expr boundExpr(*c);
    for (std::vector<LoopDetails>::iterator lIt = loops.begin(); lIt !=
	    loops.end(); lIt++) {
	z3::expr index = c->int_const(lIt->loopIndexVar.varName.c_str());
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: Calling getZ3Expr() of ";
	lIt->loopIndexInitValSymExpr->prettyPrint(false);
	llvm::outs() << "\n";
#endif
	z3::expr lower = lIt->loopIndexInitValSymExpr->getZ3Expr(c, mObj,
	    std::vector<LoopDetails>(loops.begin(), lIt), inputFirst, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While getZ3Expr on loop indx init val\n";
	    return boundExpr;
	}
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: Calling getZ3Expr() of ";
	lIt->loopIndexFinalValSymExpr->prettyPrint(false);
	llvm::outs() << "\n";
#endif
	z3::expr upper = lIt->loopIndexFinalValSymExpr->getZ3Expr(c, mObj,
	    std::vector<LoopDetails>(loops.begin(), lIt), inputFirst, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While getZ3Expr on loop indx init val\n";
	    return boundExpr;
	}
	if (lIt == loops.begin()) {
	    if (lIt->upperBound) {
		boundExpr = index;
#ifdef DEBUGFULL
		llvm::outs() << "DEBUG: Before lowerbound: lower <= index";
#endif
		boundExpr = (lower <= index);
#ifdef DEBUGFULL
		llvm::outs() << "DEBUG: lowerbound: lower <= index";
#endif
	    } else {
		boundExpr = lower >= index;
#ifdef DEBUGFULL
		llvm::outs() << "DEBUG: lowerbound: lower >= index";
#endif
	    }
	} else {
	    if (lIt->upperBound) {
		boundExpr = boundExpr && (lower <= index);
#ifdef DEBUGFULL
		llvm::outs() << "DEBUG: lowerbound: boundExpr && lower <= index";
#endif
	    } else {
		boundExpr = boundExpr && (lower >= index);
#ifdef DEBUGFULL
		llvm::outs() << "DEBUG: lowerbound: boundExpr && lower >= index";
#endif
	    }
	}

	if (lIt->upperBound) {
	    if (lIt->strictBound) {
		boundExpr = boundExpr && (index < upper);
#ifdef DEBUGFULL
		llvm::outs() << "DEBUG: uppderbound: boundExpr && index < upper";
#endif
	    } else {
		boundExpr = boundExpr && (index <= upper);
#ifdef DEBUGFULL
		llvm::outs() << "DEBUG: uppderbound: boundExpr && index <= upper";
#endif
	    }
	} else {
	    if (lIt->strictBound) {
		boundExpr = boundExpr && (index > upper);
#ifdef DEBUGFULL
		llvm::outs() << "DEBUG: uppderbound: boundExpr && index > upper";
#endif
	    } else {
		boundExpr = boundExpr && (index >= upper);
#ifdef DEBUGFULL
		llvm::outs() << "DEBUG: uppderbound: boundExpr && index >= upper";
#endif
	    }
	}
    }
    return boundExpr;
}
