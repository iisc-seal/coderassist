#include "StmtDetails.h"
#include "GetSymbolicExprVisitor.h"
#include "clang/AST/StmtObjC.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/AST/Expr.h"

void StmtDetails::callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering StmtDetails::callVisitor()\n";
#endif
    rv = true;
    if (!origStmt) {
	llvm::outs() << "ERROR: NULL origStmt\n";
	rv = false;
	return;
    }
    symVisitor->TraverseStmt(origStmt);
    if (symVisitor->error) {
	llvm::outs() << "ERROR: While TraverseStmt(" 
		     << Helper::prettyPrintStmt(origStmt) << "\n";
	rv = false;
	return;
    }
}

void DeclStmtDetails::callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering DeclStmtDetails::callVisitor()\n";
#endif
    rv = true;
    vdd->callVisitor(symVisitor, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While callVisitor() on VarDecl\n";
	return;
    }
    if (!origStmt) {
	llvm::outs() << "ERROR: NULL origStmt\n";
	rv = false;
	return;
    }
    symVisitor->TraverseStmt(origStmt);
    if (symVisitor->error) {
	llvm::outs() << "ERROR: While TraverseStmt("
		     << Helper::prettyPrintStmt(origStmt) << "\n";
	rv = false;
	return;
    }
}

void VarDeclDetails::callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering VarDeclDetails::callVisitor()\n";
#endif
    rv = true;

    if (hasInit()) {
	initExpr->callVisitor(symVisitor, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While callVisitor() on initExpr\n";
	    return;
	}
    }
}

void ReturnStmtDetails::callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering ReturnStmtDetails::callVisitor()\n";
#endif
    rv = true;
    if (retExpr) {
	retExpr->callVisitor(symVisitor, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While callVisitor() on retExpr\n";
	    return;
	}
    }
    if (!origStmt) {
	llvm::outs() << "ERROR: NULL origStmt\n";
	rv = false;
	return;
    }
    symVisitor->TraverseStmt(origStmt);
    if (symVisitor->error) {
	llvm::outs() << "ERROR; While TraverseStmt("
		     << Helper::prettyPrintStmt(origStmt) << "\n";
	rv = false;
	return;
    }
}

void CallExprDetails::callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering CallExprDetails::callVisitor()\n";
#endif
    rv = true;
    for (std::vector<ExprDetails*>::iterator argIt = callArgExprs.begin(); 
	    argIt != callArgExprs.end(); argIt++) {
	if (!*argIt) {
	    llvm::outs() << "ERROR: NULL argExpr in callExpr\n";
	    rv = false;
	    return;
	}
	(*argIt)->callVisitor(symVisitor, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While callVisitor() on argExpr\n";
	    return;
	}
    }
    if (!callee) {
	llvm::outs() << "ERROR: NULL callee\n";
	rv = false;
	return;
    }
    callee->callVisitor(symVisitor, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While callVisitor() on callee\n";
	return;
    }
    if (!origExpr) {
	llvm::outs() << "ERROR: NULL origExpr\n";
	rv = false;
	return;
    }
    symVisitor->VisitCallExpr(dyn_cast<CallExpr>(origExpr));
    if (symVisitor->error) {
	llvm::outs() << "ERROR: While VisitCallExpr()\n";
	rv = false;
	return;
    }
}

void CXXOperatorCallExprDetails::callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering CXXOperatorCallExprDetails::callVisitor()\n";
#endif
    rv = true;
    CallExprDetails::callVisitor(symVisitor, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While callVisitor() \n";
	return;
    }
}

void InitListExprDetails::callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering InitListExprDetails::callVisitor()\n";
#endif
    rv = true;
    if (!origExpr) {
	llvm::outs() << "ERROR: NULL origExpr\n";
	rv = false;
	return;
    }
    symVisitor->VisitInitListExpr(dyn_cast<InitListExpr>(origExpr));
    if (symVisitor->error) {
	llvm::outs() << "ERROR: While VisitInitListExpr()\n";
	rv = false;
	return;
    }
}

void DeclRefExprDetails::callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering DeclRefExprDetails::callVisitor()\n";
#endif
    rv = true;
    if (!origExpr) {
	llvm::outs() << "ERROR: NULL origExpr\n";
	rv = false;
	return;
    }
    symVisitor->VisitDeclRefExpr(dyn_cast<DeclRefExpr>(origExpr));
    if (symVisitor->error) {
	llvm::outs() << "ERROR: While VisitDeclRefExpr()\n";
	rv = false;
	return;
    }
}

void UnaryExprOrTypeTraitExprDetails::callVisitor(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering UnaryExprOrTypeTraitExprDetails::callVisitor()\n";
#endif
    rv = true;
    if (!origExpr) {
	llvm::outs() << "ERROR: NULL origExpr\n";
	rv = false;
	return;
    }
    symVisitor->VisitUnaryExprOrTypeTraitExpr(
	dyn_cast<UnaryExprOrTypeTraitExpr>(origExpr));
    if (symVisitor->error) {
	llvm::outs() << "ERROR: While VisitUnaryExprOrTypeTraitExpr()\n";
	rv = false;
	return;
    }
}

void ArraySubscriptExprDetails::callVisitor(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering ArraySubscriptExprDetails::callVisitor()\n";
#endif
    rv = true;
    for (std::vector<ExprDetails*>::iterator indexIt = indexExprs.begin();
	    indexIt != indexExprs.end(); indexIt++) {
	if (!*indexIt) {
	    llvm::outs() << "ERROR: NULL indexExpr\n";
	    rv = false;
	    return;
	}
	(*indexIt)->callVisitor(symVisitor, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While callVisitor() on indexExpr\n";
	    return;
	}
    }
    if (!baseArray) {
	llvm::outs() << "ERROR: NULL baseArray\n";
	rv = false;
	return;
    }
    baseArray->callVisitor(symVisitor, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While callVisitor on baseArray\n";
	return;
    }
    if (!origExpr) {
	llvm::outs() << "ERROR: NULL origExpr\n";
	rv = false;
	return;
    }
    symVisitor->VisitArraySubscriptExpr(dyn_cast<ArraySubscriptExpr>(origExpr));
    if (symVisitor->error) {
	llvm::outs() << "ERROR: While VisitArraySubscriptExpr()\n";
	rv = false;
	return;
    }
}

void UnaryOperatorDetails::callVisitor(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering UnaryOperatorDetails::callVisitor()\n";
#endif
    rv = true;
    if (!opExpr) {
	llvm::outs() << "ERROR: NULL opExpr\n";
	rv = false;
	return;
    }
    opExpr->callVisitor(symVisitor, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While callVisitor() on opExpr\n";
	return;
    }
    if (!origExpr) {
	llvm::outs() << "ERROR: NULL origExpr\n";
	rv = false;
	return;
    }
    symVisitor->VisitUnaryOperator(dyn_cast<UnaryOperator>(origExpr));
    if (symVisitor->error) {
	llvm::outs() << "ERROR: While VisitUnaryOperator()\n";
	rv = false;
	return;
    }
}

void IntegerLiteralDetails::callVisitor(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering IntegerLiteralDetails::callVisitor()\n";
#endif
    rv = true;
    if (!origExpr) {
	llvm::outs() << "ERROR: NULL origExpr\n";
	rv = false;
	return;
    }
    symVisitor->VisitIntegerLiteral(dyn_cast<IntegerLiteral>(origExpr));
    if (symVisitor->error) {
	llvm::outs() << "ERROR: While VisitIntegerLiteral()\n";
	rv = false;
	return;
    }
}

void ImaginaryLiteralDetails::callVisitor(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering ImaginaryLiteralDetails::callVisitor()\n";
#endif
    rv = true;
    if (!origExpr) {
	llvm::outs() << "ERROR: NULL origExpr\n";
	rv = false;
	return;
    }
    symVisitor->VisitImaginaryLiteral(dyn_cast<ImaginaryLiteral>(origExpr));
    if (symVisitor->error) {
	llvm::outs() << "ERROR: While VisitImaginaryLiteral()\n";
	rv = false;
	return;
    }
}

void FloatingLiteralDetails::callVisitor(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering FloatingLiteralDetails::callVisitor()\n";
#endif
    rv = true;
    if (!origExpr) {
	llvm::outs() << "ERROR: NULL origExpr\n";
	rv = false;
	return;
    }
    symVisitor->VisitFloatingLiteral(dyn_cast<FloatingLiteral>(origExpr));
    if (symVisitor->error) {
	llvm::outs() << "ERROR: While VisitFloatingLiteral()\n";
	rv = false;
	return;
    }
}

void CXXBoolLiteralExprDetails::callVisitor(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering CXXBoolLiteralExprDetails::callVisitor()\n";
#endif
    rv = true;
    if (!origExpr) {
	llvm::outs() << "ERROR: NULL origExpr\n";
	rv = false;
	return;
    }
    symVisitor->VisitCXXBoolLiteralExpr(dyn_cast<CXXBoolLiteralExpr>(origExpr));
    if (symVisitor->error) {
	llvm::outs() << "ERROR: While VisitCXXBoolLiteralExpr()\n";
	rv = false;
	return;
    }
}

void CharacterLiteralDetails::callVisitor(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering CharacterLiteralDetails::callVisitor()\n";
#endif
    rv = true;
    if (!origExpr) {
	llvm::outs() << "ERROR: NULL origExpr\n";
	rv = false;
	return;
    }
    symVisitor->VisitCharacterLiteral(dyn_cast<CharacterLiteral>(origExpr));
    if (symVisitor->error) {
	llvm::outs() << "ERROR: While VisitCharacterLiteral()\n";
	rv = false;
	return;
    }
}

void StringLiteralDetails::callVisitor(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering StringLiteralDetails::callVisitor()\n";
#endif
    rv = true;
    if (!origExpr) {
	llvm::outs() << "ERROR: NULL origExpr\n";
	rv = false;
	return;
    }
    symVisitor->VisitStringLiteral(dyn_cast<StringLiteral>(origExpr));
    if (symVisitor->error) {
	llvm::outs() << "ERROR: While VisitStringLiteral()\n";
	rv = false;
	return;
    }
}

void BinaryOperatorDetails::callVisitor(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering BinaryOperatorDetails::callVisitor()\n";
#endif
    rv = true;
    if (!lhs || !rhs) {
	llvm::outs() << "ERROR: NULL lhs/rhs\n";
	rv = false;
	return;
    }
    lhs->callVisitor(symVisitor, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While callVisitor() on lhs\n";
	return;
    }
    rhs->callVisitor(symVisitor, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While callVisitor() on rhs\n";
	return;
    }
    if (!origExpr) {
	llvm::outs() << "ERROR: NULL origExpr\n";
	rv = false;
	return;
    }
    symVisitor->VisitBinaryOperator(dyn_cast<BinaryOperator>(origExpr));
    if (symVisitor->error) {
	llvm::outs() << "ERROR: While VisitBinaryOperator()\n";
	rv = false;
	return;
    }
}

void ConditionalOperatorDetails::callVisitor(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering ConditionalOperatorDetails::callVisitor()\n";
#endif
    rv = true;
    if (!condition || !trueExpr || !falseExpr) {
	llvm::outs() << "ERROR: NULL condition/trueExpr/falseExpr\n";
	rv = false;
	return;
    }
    condition->callVisitor(symVisitor, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While callVisitor() on condition\n";
	return;
    }
    trueExpr->callVisitor(symVisitor, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While callVisitor() on trueExpr\n";
	return;
    }
    falseExpr->callVisitor(symVisitor, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While callVisitor() on falseExpr\n";
	return;
    }
    if (!origExpr) {
	llvm::outs() << "ERROR: NULL origExpr\n";
	rv = false;
	return;
    }
    symVisitor->VisitConditionalOperator(dyn_cast<ConditionalOperator>(origExpr));
    if (symVisitor->error) {
	llvm::outs() << "ERROR: While VisitConditionalOperator()\n";
	rv = false;
	return;
    }
}

bool DeclStmtDetails::isSymExprCompletelyResolved(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering DeclStmtDetails::isSymExprCompletelyResolved()\n";
#endif
    rv = true;
    if (var.isArray())
	if (!symVisitor->isSizeResolved(var)) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Issue in size in DeclStmtDetails\n";
#endif
	    return false;
	}
    if (!vdd) {
	llvm::outs() << "ERROR: NULL VarDeclDetails in DeclStmtDetails\n";
	rv = false;
	return false;
    }
    if (!vdd->isSymExprCompletelyResolved(symVisitor, rv) || !rv) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Issue in VarDeclDetails in DeclStmtDetails\n";
#endif
	return false;
    }
    return symVisitor->isResolved(this);
}

bool VarDeclDetails::isSymExprCompletelyResolved(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering VarDeclDetails::isSymExprCompletelyResolved()\n";
#endif
    rv = true;
    if (var.isArray())
	if (!symVisitor->isSizeResolved(var)) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Issue in size of VarDeclDetails\n";
#endif
	    return false;
	}
    if (initExpr && (!initExpr->isSymExprCompletelyResolved(symVisitor, rv) ||
	    !rv)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Issue in initExpr of VarDeclDetails\n";
#endif
	return false;
    }
    return symVisitor->isResolved(this);
}

bool ReturnStmtDetails::isSymExprCompletelyResolved(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
    if (retExpr && (!retExpr->isSymExprCompletelyResolved(symVisitor, rv) || !rv))
	return false;
    return symVisitor->isResolved(this);
}

bool CallExprDetails::isSymExprCompletelyResolved(
GetSymbolicExprVisitor* symVisitor, bool &rv)  {
    if (!callee) {
	rv = false;
	return false;
    }
    if (!callee->isSymExprCompletelyResolved(symVisitor, rv) || !rv) 
	return false;
    for (std::vector<ExprDetails*>::iterator cIt = callArgExprs.begin(); cIt != 
	    callArgExprs.end(); cIt++) {
	if (!(*cIt)) {
	    rv = false;
	    return false;
	}
	if (!(*cIt)->isSymExprCompletelyResolved(symVisitor, rv) || !rv) 
	    return false;
    }
    return symVisitor->isResolved(this);
}

bool CXXOperatorCallExprDetails::isSymExprCompletelyResolved(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
    rv = true;
    return CallExprDetails::isSymExprCompletelyResolved(symVisitor, rv);
}

bool InitListExprDetails::isSymExprCompletelyResolved(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
    rv = true;
    if (arrayFiller && 
	(!arrayFiller->isSymExprCompletelyResolved(symVisitor, rv) || !rv))
	return false;
    for (std::vector<ExprDetails*>::iterator iIt = inits.begin(); iIt !=
	    inits.end(); iIt++) {
	if (!(*iIt)) {
	    rv = false;
	    return false;
	}
	if (!(*iIt)->isSymExprCompletelyResolved(symVisitor, rv) || !rv)
	    return false;
    }
    return symVisitor->isResolved(this);
}

bool DeclRefExprDetails::isSymExprCompletelyResolved(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
    rv = true;
    if (isDummy || rKind == DeclRefKind::FUNC || rKind == DeclRefKind::LIB) {
	return symVisitor->isResolved(this);
    }
    if (var.isArray()) {
	return symVisitor->isResolved(this);
    }

    bool isLoopIndex = symVisitor->FA->isLoopIndexVar(var, blockID, rv);
    if (!rv) return false;
    if (isLoopIndex) {
	return symVisitor->isResolved(this);
    }

    for (std::vector<SpecialExpr*>::iterator sIt =
	    symVisitor->FA->spExprs.begin(); sIt !=
	    symVisitor->FA->spExprs.end(); sIt++) {
	if ((*sIt)->scalarVar.equals(var) && (*sIt)->guardLine == lineNum) {
	    return symVisitor->isResolved(this);
	}
    }
    bool isDefined = symVisitor->FA->rd.isVarDefined(var, blockID, lineNum);
    bool isUsed = symVisitor->FA->rd.isVarUsed(var, blockID, lineNum);
    if (isDefined && !isUsed) {
	return symVisitor->isResolved(this);
    }

    std::vector<Definition> reachDefs =
	symVisitor->FA->getReachDefsOfVarAtLine(var, lineNum, blockID, rv);
    if (!rv) return false;

    for (std::vector<Definition>::iterator dIt = reachDefs.begin(); dIt !=
	    reachDefs.end(); dIt++) {
	if (dIt->throughBackEdge) continue;
	if (dIt->expressionStr.compare("DP_INPUT") == 0 ||
		dIt->expressionStr.compare("DP_FUNC_PARAM") == 0 ||
		dIt->expressionStr.compare("DP_UNDEFINED") == 0 ||
		dIt->expressionStr.compare("DP_GLOBAL_VAR") == 0)
	    continue;
	if (dIt->isSpecialExpr) {
	    llvm::outs() << "ERROR: I am not handling this now!\n";
	    rv = false;
	    return false;
	} else {
	    if (!dIt->expression) {
		llvm::outs() << "ERROR: NULL expression for def:\n";
		dIt->print();
		rv = false;
		return false;
	    }
	    ExprDetails* ed = ExprDetails::getObject(symVisitor->SM,
		const_cast<Expr*>(dIt->expression), dIt->blockID, rv);
	    if (!rv) return false;
	    if (!ed->isSymExprCompletelyResolved(symVisitor, rv) || !rv)
		return false;
	}
    }
    return symVisitor->isResolved(this);
}

bool UnaryExprOrTypeTraitExprDetails::isSymExprCompletelyResolved(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
    rv = true;
    if (array && (!array->isSymExprCompletelyResolved(symVisitor, rv) || !rv))
	return false;
    return symVisitor->isResolved(this);
}

bool ArraySubscriptExprDetails::isSymExprCompletelyResolved(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
    rv = true;
    if (!baseArray) {
	rv = false;
	return false;
    }
    if (!baseArray->isSymExprCompletelyResolved(symVisitor, rv) || !rv)
	return false;
    for (std::vector<ExprDetails*>::iterator iIt = indexExprs.begin(); iIt != 
	    indexExprs.end(); iIt++) {
	if (!(*iIt)) {
	    rv = false;
	    return false;
	}
	if (!(*iIt)->isSymExprCompletelyResolved(symVisitor, rv) || !rv)
	    return false;
    }
    return symVisitor->isResolved(this);
}

bool UnaryOperatorDetails::isSymExprCompletelyResolved(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
    rv = true;
    if (!opExpr) {
	rv = false;
	return false;
    }
    if (!opExpr->isSymExprCompletelyResolved(symVisitor, rv) || !rv)
	return false;
    return symVisitor->isResolved(this);
}

bool IntegerLiteralDetails::isSymExprCompletelyResolved(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
    rv = true;
    return symVisitor->isResolved(this);
}

bool ImaginaryLiteralDetails::isSymExprCompletelyResolved(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
    rv = true;
    return symVisitor->isResolved(this);
}

bool FloatingLiteralDetails::isSymExprCompletelyResolved(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
    rv = true;
    return symVisitor->isResolved(this);
}

bool CXXBoolLiteralExprDetails::isSymExprCompletelyResolved(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
    rv = true;
    return symVisitor->isResolved(this);
}

bool CharacterLiteralDetails::isSymExprCompletelyResolved(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
    rv = true;
    return symVisitor->isResolved(this);
}

bool StringLiteralDetails::isSymExprCompletelyResolved(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
    rv = true;
    return symVisitor->isResolved(this);
}

bool BinaryOperatorDetails::isSymExprCompletelyResolved(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
    rv = true;
    if (!lhs || !rhs) {
	rv = false;
	return false;
    }
    if (!lhs->isSymExprCompletelyResolved(symVisitor, rv) || !rv)
	return false;
    if (!rhs->isSymExprCompletelyResolved(symVisitor, rv) || !rv)
	return false;
    return symVisitor->isResolved(this);
}

bool ConditionalOperatorDetails::isSymExprCompletelyResolved(
GetSymbolicExprVisitor* symVisitor, bool &rv) {
    rv = true;
    if (!condition || !trueExpr || !falseExpr) {
	llvm::outs() << "ERROR: NULL condition/trueExpr/falseExpr\n";
	rv = false;
	return false;
    }
    if (!condition->isSymExprCompletelyResolved(symVisitor, rv) || !rv)
	return false;
    if (!trueExpr->isSymExprCompletelyResolved(symVisitor, rv) || !rv)
	return false;
    if (!falseExpr->isSymExprCompletelyResolved(symVisitor, rv) || !rv)
	return false;
    return symVisitor->isResolved(this);
}

bool ReturnStmtDetails::equals(StmtDetails* S) {
    if (!(StmtDetails::equals(S))) return false;
    if (!isa<ReturnStmtDetails>(S)) return false;
    ReturnStmtDetails* rsd = dyn_cast<ReturnStmtDetails>(S);
    if (retExpr && !rsd->retExpr) return false;
    if (!retExpr && rsd->retExpr) return false;
    if (retExpr && rsd->retExpr) {
	if (!(rsd->retExpr->equals(retExpr))) return false;
    }
    return true;
}

void ReturnStmtDetails::print() {
    llvm::outs() << "RETURN ";
    if (retExpr)
	retExpr->print();
    else 
	llvm::outs() << "NULL";
    llvm::outs() << "\n";
}

void ReturnStmtDetails::prettyPrint() {
    llvm::outs() << "return ";
    if (retExpr)
	retExpr->prettyPrint();
    llvm::outs() << "\n";
}

bool DeclStmtDetails::equals(StmtDetails* sd) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: In DeclStmtDetails::equals()\n";
#endif
    if (!(StmtDetails::equals(sd))) return false;
    if (!isa<DeclStmtDetails>(sd)) return false;
    DeclStmtDetails* dsd = dyn_cast<DeclStmtDetails>(sd);
    if (!(dsd->var.equals(var))) return false;
    if (vdd && !dsd->vdd) return false;
    if (!vdd && dsd->vdd) return false;
    if (vdd && dsd->vdd && !(vdd->equals(dsd->vdd))) return false;
    return true;
}

void DeclStmtDetails::print() {
    llvm::outs() << "DeclStmt at line " << lineNum << " and block "
		 << blockID << ": ";
    var.print();
    llvm::outs() << "\n";
    if (vdd) vdd->print();
    StmtDetails::print();
}

void DeclStmtDetails::prettyPrint() {
    llvm::outs() << "DeclStmt: ";
    if (vdd) vdd->prettyPrint();
}

StmtDetails* StmtDetails::getStmtDetails(const SourceManager* SM, Stmt* S, 
unsigned blockID, bool &rv, FunctionDetails* f) {
    rv = true;
    //StmtDetails sd(StmtDetails::StmtKind::UNKNOWN);
    StmtDetails* sd = new StmtDetails(StmtKind::UNKNOWNSTMT);
    int line = SM->getExpansionLineNumber(S->getLocStart());

    sd->lineNum = line;
    sd->blockID = blockID;
    sd->origStmt = S;
    sd->func = f;

    if (isa<Expr>(S)) {
	Expr* E = dyn_cast<Expr>(S);
	ExprDetails* ed = new ExprDetails;
	//ed = ExprDetails::getExprDetails(SM, E, blockID, rv);
	ed = ExprDetails::getObject(SM, E, blockID, rv, f);
	if (!rv)
	    return sd;
	delete sd;
	return ed;
    } else if (isa<AsmStmt>(S)) {
	llvm::outs() << "ERROR: Found AsmStmt at line " << line << "\n";
	rv = false;
    } else if (isa<AttributedStmt>(S)) {
	llvm::outs() << "ERROR: Found AttributsdStmt at line " << line << "\n";
	rv = false;
    } else if (isa<BreakStmt>(S)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found BreakStmt at line " << line << "\n";
#endif
	rv = true;
    } else if (isa<CapturedStmt>(S)) {
	llvm::outs() << "ERROR: Found CaptursdStmt at line " << line << "\n";
	rv = false;
    } else if (isa<CompoundStmt>(S)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found CompoundStmt at line " << line << "\n";
#endif
	rv = true;
    } else if (isa<ContinueStmt>(S)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found ContinueStmt at line " << line << "\n";
#endif
	rv = true;
    } else if (isa<CXXCatchStmt>(S)) {
	llvm::outs() << "ERROR: Found CXXCatchStmt at line " << line << "\n";
	rv = false;
    } else if (isa<CXXTryStmt>(S)) {
	llvm::outs() << "ERROR: Found CXXTryStmt at line " << line << "\n";
	rv = false;
    } else if (isa<DeclStmt>(S)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found DeclStmt at line " << line << "\n";
#endif
	DeclStmtDetails* dsd = new DeclStmtDetails;
	dsd = DeclStmtDetails::getObject(SM, dyn_cast<DeclStmt>(S),
		blockID, rv, f);
	if (!rv)
	    return sd;
	delete sd;
	return dsd;
    } else if (isa<DoStmt>(S)) {
	llvm::outs() << "ERROR: Found DoStmt at line " << line << "\n";
	rv = false;
    } else if (isa<ForStmt>(S)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found ForStmt at line " << line << "\n";
#endif
	rv = true;
    } else if (isa<GotoStmt>(S)) {
	llvm::outs() << "ERROR: Found GotoStmt at line " << line << "\n";
	rv = false;
    } else if (isa<IfStmt>(S)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found IfStmt at line " << line << "\n";
#endif
	rv = true;
    } else if (isa<IndirectGotoStmt>(S)) {
	llvm::outs() << "ERROR: Found IndirectGotoStmt at line " << line 
		     << "\n";
	rv = false;
    } else if (isa<MSDependentExistsStmt>(S)) {
	llvm::outs() << "ERROR: Found MSDependentExistsStmt at line " << line 
		     << "\n";
	rv = false;
    } else if (isa<NullStmt>(S)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found NullStmt at line " << line << "\n";
#endif
	rv = true;
    } else if (isa<ObjCAtCatchStmt>(S) || isa<ObjCAtFinallyStmt>(S) || 
		isa<ObjCAtSynchronizedStmt>(S) || isa<ObjCAtThrowStmt>(S) ||
		isa<ObjCAtTryStmt>(S) || isa<ObjCAutoreleasePoolStmt>(S) ||
		isa<ObjCForCollectionStmt>(S)) {
	llvm::outs() << "ERROR: Found ObjC stmt at line " << line << "\n";
	rv = false;
    } else if (isa<OMPExecutableDirective>(S)) {
	llvm::outs() << "ERROR: Found OMPExecutableDirective at line " << line
		     << "\n";
	rv = false;
    } else if (isa<ReturnStmt>(S)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found ReturnStmt at line " << line << "\n";
#endif
	rv = true;
	ReturnStmt* RS = dyn_cast<ReturnStmt>(S);
	ReturnStmtDetails* rd = ReturnStmtDetails::getObject(SM, RS, blockID,
	    rv, f);
	if (!rv)
	    return sd;
	delete sd;
	return rd;
    } else if (isa<SEHExceptStmt>(S) || isa<SEHFinallyStmt>(S) ||
		isa<SEHLeaveStmt>(S) || isa<SEHTryStmt>(S)) {
	llvm::outs() << "ERROR: Found SEH stmt at line " << line << "\n";
	rv = false;
    } else if (isa<SwitchCase>(S)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found SwitchCase stmt at line " << line << "\n";
#endif
	rv = true;
    } else if (isa<SwitchStmt>(S)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found SwitchStmt at line " << line << "\n";
#endif
	rv = true;
    } else if (isa<WhileStmt>(S)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found WhileStmt at line " << line << "\n";
#endif
	rv = true;
    } else {
	llvm::outs() << "ERROR: Unknown StmtKind\n";
	rv = false;
    }

    return sd;
}

DeclStmtDetails* DeclStmtDetails::getObject(const SourceManager* SM, 
DeclStmt* D, unsigned blockID, bool &rv, FunctionDetails* f) {
    rv = true;
    DeclStmtDetails* dsd = new DeclStmtDetails;
    dsd->blockID = blockID;
    dsd->lineNum = SM->getExpansionLineNumber(D->getLocStart());
    dsd->origStmt = D;
    dsd->func = f;
    if (D->isSingleDecl()) {
	Decl* singleDecl = D->getSingleDecl();
	dsd->var = Helper::getVarDetailsFromDecl(SM, singleDecl, rv);
	if (isa<VarDecl>(singleDecl)) {
	    VarDecl* vd = dyn_cast<VarDecl>(singleDecl);
	    dsd->vdd = VarDeclDetails::getObject(SM, vd, blockID, rv, f);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot get VarDeclDetails\n";
		return dsd;
	    }
	} else {
	    rv = false;
	    return dsd;
	}
	return dsd;
    } else {
	llvm::outs() << "ERROR: DeclStmt at line " << dsd->lineNum 
		     << " is not singleDecl\n";
	rv = false;
	return dsd;
    }
}

VarDeclDetails* VarDeclDetails::getObject(const SourceManager* SM, VarDecl* VD,
unsigned blockID, bool &rv, FunctionDetails* f) {
    rv = true;
    VarDeclDetails* vdd = new VarDeclDetails;
    vdd->blockID = blockID;
    vdd->lineNum = SM->getExpansionLineNumber(VD->getLocStart());
    vdd->var = Helper::getVarDetailsFromDecl(SM, VD, rv);
    vdd->func = f;
    if (!rv)
	return vdd;

    if (VD->hasInit()) {
	vdd->initExpr = ExprDetails::getObject(SM, VD->getInit(), blockID, rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot obtain ExprDetails of initExpr: "
			 << Helper::prettyPrintExpr(VD->getInit()) << "\n";
	    return vdd;
	}
    }

    return vdd;
}

void StmtDetails::print() {
    llvm::outs() << "Stmt at line " << lineNum << " and block " << blockID 
		 << (isDummy? " (dummy)": "") << "\n";
#if 0
    if (kind == BREAK) {
	llvm::outs() << "BREAK stmt at line " << lineNum << " and block "
		     << blockID;
    } else if (kind == COMPOUND) {
	llvm::outs() << "COMPOUND stmt at line " << lineNum << " and block "
		     << blockID;
    } else if (kind == CONTINUE) {
	llvm::outs() << "CONTINUE stmt at line " << lineNum << " and block "
		     << blockID;
    } else if (kind == DECL) {
	//llvm::outs() << "DECL stmt at line " << lineNum << " and block "
		     //<< blockID;
	//((DeclStmtDetails)this)->print();
	llvm::outs() << "DEBUG: Am in StmtDetails::print()\n";
    } else if (kind == FOR) {
	llvm::outs() << "FOR stmt at line " << lineNum << " and block "
		     << blockID;
    } else if (kind == IF) {
	llvm::outs() << "IF stmt at line " << lineNum << " and block "
		     << blockID;
    } else if (kind == NULLST) {
	llvm::outs() << "NULL stmt at line " << lineNum << " and block "
		     << blockID;
    } else if (kind == RETURN) {
	llvm::outs() << "RETURN stmt at line " << lineNum << " and block "
		     << blockID;
    } else if (kind == SWCASE) {
	llvm::outs() << "SWCASE stmt at line " << lineNum << " and block "
		     << blockID;
    } else if (kind == SWITCH) {
	llvm::outs() << "SWITCH stmt at line " << lineNum << " and block "
		     << blockID;
    } else if (kind == WHILE) {
	llvm::outs() << "WHILE stmt at line " << lineNum << " and block "
		     << blockID;
    } else {
	llvm::outs() << "Unknown stmt at line " << lineNum << " and block "
		     << blockID;
    }
#endif
}

void VarDeclDetails::print() {
    llvm::outs() << "Var: ";
    var.print();
    llvm::outs() << "\n";
    if (hasInit()) {
	//llvm::outs() << "InitExpr: " << Helper::prettyPrintExpr(initExpr);
	llvm::outs() << "InitExpr:\n";
	initExpr->print();
    }
    DeclStmtDetails::print();
}

void VarDeclDetails::prettyPrint() {
    llvm::outs() << "Var Declared: ";
    var.print();
    llvm::outs() << "\n";
    if (hasInit()) {
	llvm::outs() << "InitExpr: ";
	initExpr->prettyPrint();
	llvm::outs() << "\n";
    }
}

#if 0
ExprDetails* DeclStmtDetails::getAsExprDetails() {
    ExprDetails* ed = new ExprDetails;
    ed->isVar = true;
    ed->var = var;
    ed->blockID = blockID;
    ed->lineNum = lineNum;
    return ed;
}
#endif

ExprDetails* ExprDetails::getObject(const SourceManager* SM, Expr* E, 
unsigned blockID, bool &rv, FunctionDetails* f) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering ExprDetails::getObject("
		 << Helper::prettyPrintExpr(E) << ")\n";
#endif
    rv = true;
    E = E->IgnoreParenCasts();
    ExprDetails* ed = nullptr;
    if (isa<DeclRefExpr>(E)) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: Expr is DeclRefExpr\n";
#endif
	DeclRefExpr* dRE = dyn_cast<DeclRefExpr>(E);
	DeclRefExprDetails* dred = DeclRefExprDetails::getObject(SM, dRE, blockID,
	    rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Could not obtain DeclRefExprDetails from "
			 << "DeclRefExpr\n";
	    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
	    return ed;
	}
	return dred;
    } else if (isa<ArraySubscriptExpr>(E)) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: Expr is ArraySubscriptExpr\n";
#endif
	ArraySubscriptExpr* arr = dyn_cast<ArraySubscriptExpr>(E);
	ArraySubscriptExprDetails* arrDetails =
	    ArraySubscriptExprDetails::getObject(SM, arr, blockID, rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Could not obtain ArraySubscriptExprDetails "
			 << "from ArraySubscriptExpr\n";
	    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
	    return ed;
	}
	return arrDetails;
    } else if (isa<CallExpr>(E)) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: Expr is CallExpr\n";
#endif
	CallExpr* cE = dyn_cast<CallExpr>(E);
	CallExprDetails* cd =
	    CallExprDetails::getObject(SM, cE, blockID, rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Could not obtain CallExprDetails "
			 << "from CallExpr\n";
	    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
	    return ed;
	}
	return cd;
    } else if (isa<UnaryOperator>(E)) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: Expr is UnaryOperator\n";
#endif
	UnaryOperator* U = dyn_cast<UnaryOperator>(E);
	UnaryOperatorDetails* ud = UnaryOperatorDetails::getObject(SM, U,
	    blockID, rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Could not obtain UnaryOperatorDetails "
			 << "from UnaryOperator\n";
	    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
	    return ud;
	}
	return ud;
    } else if (isa<IntegerLiteral>(E)) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: Expr is IntegerLiteral\n";
#endif
	IntegerLiteral* I = dyn_cast<IntegerLiteral>(E);
	IntegerLiteralDetails* id = IntegerLiteralDetails::getObject(SM, I,
	    blockID, rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Could not obtain IntegerLiteralDetails from "
			 << "IntegerLiteral\n";
	    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
	    return id;
	}
	return id;
    } else if (isa<ImaginaryLiteral>(E)) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: Expr is ImaginaryLiteral\n";
#endif
	ImaginaryLiteral* I = dyn_cast<ImaginaryLiteral>(E);
	ImaginaryLiteralDetails* id = ImaginaryLiteralDetails::getObject(SM, I,
	    blockID, rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Could not obtain ImaginaryLiteralDetails from "
			 << "ImaginaryLiteral\n";
	    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
	    return id;
	}
	return id;
    } else if (isa<FloatingLiteral>(E)) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: Expr is FloatingLiteral\n";
#endif
	FloatingLiteral* I = dyn_cast<FloatingLiteral>(E);
	FloatingLiteralDetails* id = FloatingLiteralDetails::getObject(SM, I,
	    blockID, rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Could not obtain FloatingLiteralDetails from "
			 << "FloatingLiteral\n";
	    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
	    return id;
	}
	return id;
    } else if (isa<CXXBoolLiteralExpr>(E)) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: Expr is CXXBoolLiteralExpr\n";
#endif
	CXXBoolLiteralExpr* B = dyn_cast<CXXBoolLiteralExpr>(E);
	CXXBoolLiteralExprDetails* bd = CXXBoolLiteralExprDetails::getObject(SM,
	    B, blockID, rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Could not obtain CXXBoolLiteralExprDetails "
			 << "from CXXBoolLiteralExpr\n";
	    llvm::outs() << Helper::prettyPrintExpr(B) << "\n";
	    return bd;
	}
	return bd;
    } else if (isa<CharacterLiteral>(E)) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: Expr is CharacterLiteral\n";
#endif
	CharacterLiteral* S = dyn_cast<CharacterLiteral>(E);
	CharacterLiteralDetails* sd = CharacterLiteralDetails::getObject(SM, S, 
		blockID, rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Coud not obtain CharacterLiteralDetails "
			 << "from CharacterLiteral\n";
	    llvm::outs() << Helper::prettyPrintExpr(S) << "\n";
	    return sd;
	}
	return sd;
    } else if (isa<StringLiteral>(E)) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: Expr is StringLiteral\n";
#endif
	StringLiteral* S = dyn_cast<StringLiteral>(E);
	StringLiteralDetails* sd = StringLiteralDetails::getObject(SM, S, 
		blockID, rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Coud not obtain StringLiteralDetails "
			 << "from StringLiteral\n";
	    llvm::outs() << Helper::prettyPrintExpr(S) << "\n";
	    return sd;
	}
	return sd;
    } else if (isa<BinaryOperator>(E)) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: Expr is BinaryOperator\n";
#endif
	BinaryOperator* B = dyn_cast<BinaryOperator>(E);
	BinaryOperatorDetails* bd = BinaryOperatorDetails::getObject(SM, B,
	    blockID, rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Could not obtain BinaryOperatorDetails "
			 << "from BinaryOperator\n";
	    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
	    return bd;
	}

	return bd;
    } else if (isa<ConditionalOperator>(E)) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: Expr is ConditionalOperator\n";
#endif
	ConditionalOperator* C = dyn_cast<ConditionalOperator>(E);
	ConditionalOperatorDetails* cd = ConditionalOperatorDetails::getObject(
	    SM, C, blockID, rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Could not obtain ConditionalOperatorDetails "
			 << "from ConditonalOperator\n";
	    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
	    return cd;
	}
        return cd;
    } else if (isa<InitListExpr>(E)) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: Expr is InitListExpr\n";
#endif
	InitListExpr* I = dyn_cast<InitListExpr>(E);
	InitListExprDetails* id = InitListExprDetails::getObject(SM, I, blockID,
	    rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Could not obtain InitListExprDetails "
			 << "from InitListExpr\n";
	    llvm::outs() << Helper::prettyPrintExpr(I) << "\n";
	    return id;
	}
	return id;
    } else if (isa<UnaryExprOrTypeTraitExpr>(E)) {
	UnaryExprOrTypeTraitExpr* U = dyn_cast<UnaryExprOrTypeTraitExpr>(E);
	UnaryExprOrTypeTraitExprDetails* ud =
	    UnaryExprOrTypeTraitExprDetails::getObject(SM, U, blockID, rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Could not obtain UnaryExprOrTypeTraitExprDetails\n";
	    llvm::outs() << Helper::prettyPrintExpr(U) << "\n";
	    return ud;
	}
	return ud;
    }

#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Expr is of unknown type\n";
#endif

    llvm::outs() << "ERROR: Could not obtain ExprDetails from Expr\n";
    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
    rv = false;
    return ed;
}

CallExprDetails* CallExprDetails::getObject(const SourceManager* SM, CallExpr* E, 
unsigned blockID, bool &rv, FunctionDetails* f) {
    rv = true;
    CallExprDetails* ced = new CallExprDetails;
    if (isa<CXXOperatorCallExpr>(E)) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: Expr is CXXOperatorCallExpr\n";
#endif
	CXXOperatorCallExpr* cxxE = dyn_cast<CXXOperatorCallExpr>(E);
	CXXOperatorCallExprDetails* cd =
	    CXXOperatorCallExprDetails::getObject(SM, cxxE, blockID, rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Could not obtain CXXOperatorCallExprDetails "
			 << "from CXXOperatorCallExpr\n";
	    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
	    return ced;
	}
	return cd;
    }

    ced->lineNum = SM->getExpansionLineNumber(E->getLocStart());
    ced->blockID = blockID;
    ced->origExpr = E;
    ced->origStmt = E;
    ced->func = f;
    // Get callee
    Expr* calleeExpr = E->getCallee();
    if (!calleeExpr) {
	llvm::outs() << "ERROR: Cannot get callee expr of CallExpr\n";
	llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
	rv = false;
	return ced;
    }

    ced->callee = ExprDetails::getObject(SM, calleeExpr, blockID, rv, f);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get ExprDetails of callee expr: "
		     << Helper::prettyPrintExpr(calleeExpr) << "\n";
	rv = false;
	return ced;
    }

    for (unsigned i = 0; i < E->getNumArgs(); i++) {
	if (!E->getArg(i)) {
	    llvm::outs() << "ERROR: Cannot get argument to callExpr\n";
	    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
	    rv = false;
	    return ced;
	}

	ExprDetails* argExprDetails = ExprDetails::getObject(SM, E->getArg(i),
	    blockID, rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot get ExprDetails from argExpr\n";
	    llvm::outs() << Helper::prettyPrintExpr(E->getArg(i)) << "\n";
	    rv = false;
	    return ced;
	}

	ced->callArgExprs.push_back(argExprDetails);
    }

    return ced;
}

CXXOperatorCallExprDetails* CXXOperatorCallExprDetails::getObject(const
SourceManager* SM, CXXOperatorCallExpr* E, unsigned blockID, bool &rv, FunctionDetails* f) {
    rv = true;
    CXXOperatorCallExprDetails* cd = new CXXOperatorCallExprDetails;
    cd->origExpr = E;
    cd->origStmt = E;
    cd->blockID = blockID;
    cd->lineNum = SM->getExpansionLineNumber(E->getLocStart());
    cd->func = f;
    // Get callee
    FunctionDecl* calleeDecl = E->getDirectCallee();
    if (!calleeDecl) {
	llvm::outs() << "ERROR: Cannot get callee of "
		     << Helper::prettyPrintExpr(E) << "\n";
	rv = false;
	return cd;
    }

    // Check if the callee is operator<< or operator>>
    DeclarationNameInfo calleeNameInfo = calleeDecl->getNameInfo();
    std::string calleeName = calleeNameInfo.getAsString();

    Expr* streamArg = Helper::getStreamOfCXXOperatorCallExpr(E, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get stream argument of "
		     << "CXXOperatorCallExpr: " << Helper::prettyPrintExpr(E)
		     << "\n";
	rv = false;
	return cd;
    }
    
    std::string streamArgName;
    if (isa<DeclRefExpr>(streamArg)) {
	streamArgName = Helper::prettyPrintExpr(streamArg);
	ExprDetails* calleeD = ExprDetails::getObject(SM, streamArg, blockID,
	    rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot get ExprDetails of stream object\n";
	    rv = false;
	    return cd;
	}
	cd->callee = calleeD;
    } else {
	llvm::outs() << "ERROR: Stream argument of CXXOperatorCallExpr is not "
		     << "DeclRefExpr\n";
	llvm::outs() << Helper::prettyPrintExpr(streamArg) << "\n";
	rv = false;
	return cd;
    }

    if (calleeName.compare("operator>>") == 0) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: Callee is operator>>\n";
#endif
	if (streamArgName.compare("cin") == 0 ||
	    streamArgName.compare("std::cin") == 0) {
	    cd->opKind = CXXOperatorKind::CIN;
	} else {
	    llvm::outs() << "ERROR: First argument to operator>> is not cin\n";
	    rv = false;
	    return cd;
	}
    } else if (calleeName.compare("operator<<") == 0) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: Callee is operator<<\n";
#endif
	if (streamArgName.compare("cout") == 0 ||
	    streamArgName.compare("std::cout") == 0) {
	    cd->opKind = CXXOperatorKind::COUT;
	} else {
	    llvm::outs() << "ERROR: First argument to operator<< is not cout\n";
	    rv = false;
	    return cd;
	}
    } else {
	llvm::outs() << "ERROR: Callee of " << Helper::prettyPrintExpr(E)
		     << " is not operator>> or operator<<\n";
	rv = false;
	return cd;
    }

    if (E->getNumArgs() != 2) {
	llvm::outs() << "ERROR: cin/cout arg size != 2\n";
	rv = false;
	return cd;
    }

    // Get the all arguments to cin/cout.
    std::vector<Expr*> argExprs = Helper::getArgsToCXXOperatorCallExpr(E, rv);
    for (std::vector<Expr*>::iterator aIt = argExprs.begin(); aIt !=
	    argExprs.end(); aIt++) {
	ExprDetails* argDetails = ExprDetails::getObject(SM, *aIt, blockID, rv,
	    f);
	if (!rv) {
	    llvm::outs() << "ERROR: Could not obtain ExprDetails of argExpr to "
			 << "cin/cout\n";
	    rv = false;
	    return cd;
	}
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: argDetails:\n";
	if (argDetails)
	    argDetails->print();
#endif

	cd->callArgExprs.push_back(argDetails);
    }
    return cd;
}

DeclRefExprDetails* DeclRefExprDetails::getObject(const SourceManager* SM,
DeclRefExpr* E, unsigned blockID, bool &rv, FunctionDetails* f) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: getObject(" << Helper::prettyPrintExpr(E) << ")\n";
#endif

    rv = true;
    DeclRefExprDetails* ed = new DeclRefExprDetails;

    ed->blockID = blockID;
    ed->lineNum = SM->getExpansionLineNumber(E->getLocStart());
    ed->origExpr = E;
    ed->origStmt = E;
    ed->func = f;

    // Check if this is a VarDecl
    ValueDecl* d = E->getDecl();
    if (!(isa<DeclaratorDecl>(d))) {
#ifdef DEBUG
	llvm::outs() << "ERROR: DeclRefExpr " << Helper::prettyPrintExpr(E) 
		     << " is not DeclaratorDecl\n";
#endif
	rv = false;
	return ed;
    }
    DeclaratorDecl* dd = dyn_cast<DeclaratorDecl>(d);
#if 0
    if (!(isa<VarDecl>(dd))) {
#ifdef DEBUG
	llvm::outs() << "ERROR: DeclRefExpr " << Helper::prettyPrintExpr(E)
		     << " is not VarDecl\n";
#endif
	rv = false;
	return ed;
    }
#endif

    if (isa<VarDecl>(dd)) {
	// If the var is from outside the program (i.e. library), then return
	VarDecl* vd = dyn_cast<VarDecl>(dd);
	SourceLocation SL = SM->getExpansionLoc(vd->getLocStart());
	if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Found DeclRefKind::LIB "
			 << Helper::prettyPrintExpr(E) << "\n";
#endif
	    ed->rKind = DeclRefKind::LIB;
	    ed->var.varName = Helper::prettyPrintExpr(E);
	    ed->isDummy = true;
	    return ed;
	}

#ifdef DEBUG
	llvm::outs() << "DEBUG: Not DeclRefKind::LIB\n";
#endif
	//ed->isVar = true;
	ed->rKind = DeclRefKind::VAR;
	ed->var = Helper::getVarDetailsFromExpr(SM, E, rv);
	if (!rv)
	    return ed;
	return ed;
    } else if (isa<FunctionDecl>(dd)) {
	// If the function is from outside the program (i.e. library), then
	// return
	FunctionDecl* funcDecl = dyn_cast<FunctionDecl>(dd);
	SourceLocation SL = SM->getExpansionLoc(funcDecl->getLocStart());
	if (funcDecl->isImplicit() || SM->isInSystemHeader(SL) || 
		!SM->isWrittenInMainFile(SL)) {
	    ed->rKind = DeclRefKind::LIB;
	    ed->fd = new FunctionDetails;
	    ed->fd->funcName = funcDecl->getQualifiedNameAsString();
	    ed->isDummy = true;
	    return ed;
	}

	ed->rKind = DeclRefKind::FUNC;
	ed->fd = Helper::getFunctionDetailsFromDecl(SM, funcDecl, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot get FunctionDetails from FunctionDecl\n";
	    return ed;
	}
	return ed;
    } else {
	// DeclRefExpr is not var. If it is from header, then return
	SourceLocation SL = SM->getExpansionLoc(dd->getLocStart());
	if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	    ed->rKind = DeclRefKind::LIB;
	    ed->isDummy = true;
	    return ed;
	}
    }

    return ed;
}

UnaryOperatorDetails* UnaryOperatorDetails::getObject(const SourceManager* SM,
UnaryOperator* U, unsigned blockID, bool &rv, FunctionDetails* f) {
    rv = true;

    UnaryOperatorDetails* ud = new UnaryOperatorDetails;
    ud->lineNum = SM->getExpansionLineNumber(U->getLocStart());
    ud->blockID = blockID;
    ud->origExpr = U;
    ud->origStmt = U;
    ud->func = f;

    ud->opKind = U->getOpcode();
    Expr* subExpr = U->getSubExpr();
    if (!subExpr) {
	llvm::outs() << "ERROR: Cannot obtain subExpr of UnaryOperator: "
		     << Helper::prettyPrintExpr(U) << "\n";
	rv = false;
	return ud;
    }
    ud->opExpr = ExprDetails::getObject(SM, subExpr, blockID, rv, f);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain ExprDetails of subExpr of UnaryOperator "
		     << Helper::prettyPrintExpr(U) << "\n";
	rv = false;
	return ud;
    }

    return ud;
}

IntegerLiteralDetails* IntegerLiteralDetails::getObject(const SourceManager* SM,
IntegerLiteral* I, unsigned blockID, bool &rv, FunctionDetails* f) {
    rv = true;

    IntegerLiteralDetails* id = new IntegerLiteralDetails;
    id->lineNum = SM->getExpansionLineNumber(I->getLocStart());
    id->blockID = blockID;
    id->origExpr = I;
    id->origStmt = I;
    id->func = f;

    id->val = I->getValue();

    return id;
}

ImaginaryLiteralDetails* ImaginaryLiteralDetails::getObject(const SourceManager* SM,
ImaginaryLiteral* I, unsigned blockID, bool &rv, FunctionDetails* f) {
    rv = true;

    ImaginaryLiteralDetails* id = new ImaginaryLiteralDetails;
    id->lineNum = SM->getExpansionLineNumber(I->getLocStart());
    id->blockID = blockID;
    id->origExpr = I;
    id->origStmt = I;
    id->func = f;

    Expr* subExpr = I->getSubExpr();
    if (!subExpr) {
	llvm::outs() << "ERROR: NULL subExpr for ImaginaryLiteral\n";
	rv = false;
	return id;
    }

    if (!isa<IntegerLiteral>(subExpr)) {
	llvm::outs() << "ERROR: subExpr of ImaginaryLiteral is not IntegerLiteral\n";
	rv = false;
	return id;
    }

    IntegerLiteral* subExprIL = dyn_cast<IntegerLiteral>(subExpr);
    if (subExprIL->getValue() == 0)
	id->val = subExprIL->getValue();
    else {
	llvm::outs() << "ERROR: subExpr of ImaginaryLiteral is not 0\n";
	rv = false;
	return id;
    }

    return id;
}

FloatingLiteralDetails* FloatingLiteralDetails::getObject(const SourceManager* SM,
FloatingLiteral* I, unsigned blockID, bool &rv, FunctionDetails* f) {
    rv = true;

    FloatingLiteralDetails* id = new FloatingLiteralDetails;
    id->lineNum = SM->getExpansionLineNumber(I->getLocStart());
    id->blockID = blockID;
    id->origExpr = I;
    id->origStmt = I;
    id->func = f;

    id->val = I->getValue();

    return id;
}

CXXBoolLiteralExprDetails* CXXBoolLiteralExprDetails::getObject(const
SourceManager* SM, CXXBoolLiteralExpr* B, unsigned blockID, bool &rv, FunctionDetails* f) {
    rv = true;

    CXXBoolLiteralExprDetails* bd = new CXXBoolLiteralExprDetails;
    bd->lineNum = SM->getExpansionLineNumber(B->getLocStart());
    bd->blockID = blockID;
    bd->origExpr = B;
    bd->origStmt = B;
    bd->func = f;

    bd->val = B->getValue();

    return bd;
}

CharacterLiteralDetails* CharacterLiteralDetails::getObject(const SourceManager* SM,
CharacterLiteral* S, unsigned blockID, bool &rv, FunctionDetails* f) {
    rv = true;

    CharacterLiteralDetails* sd = new CharacterLiteralDetails;
    sd->lineNum = SM->getExpansionLineNumber(S->getLocStart());
    sd->blockID = blockID;
    sd->origExpr = S;
    sd->origStmt = S;
    sd->func = f;

    sd->val = S->getValue();

    return sd;
}

StringLiteralDetails* StringLiteralDetails::getObject(const SourceManager* SM,
StringLiteral* S, unsigned blockID, bool &rv, FunctionDetails* f) {
    rv = true;

    StringLiteralDetails* sd = new StringLiteralDetails;
    sd->lineNum = SM->getExpansionLineNumber(S->getLocStart());
    sd->blockID = blockID;
    sd->origExpr = S;
    sd->origStmt = S;
    sd->func = f;

    sd->val = S->getString();

    return sd;
}

BinaryOperatorDetails* BinaryOperatorDetails::getObject(const SourceManager* SM,
BinaryOperator* B, unsigned blockID, bool &rv, FunctionDetails* f) {
    rv = true;

    BinaryOperatorDetails* bd = new BinaryOperatorDetails;
    bd->lineNum = SM->getExpansionLineNumber(B->getLocStart());
    bd->blockID = blockID;
    bd->origExpr = B;
    bd->origStmt = B;
    bd->func = f;

    bd->opKind = B->getOpcode();
    Expr* lhsExpr = B->getLHS();
    if (!lhsExpr) {
	llvm::outs() << "ERROR: NULL lhsExpr in BinaryOperator: "
		     << Helper::prettyPrintExpr(B) << "\n";
	rv = false;
	return bd;
    }
    bd->lhs = ExprDetails::getObject(SM, lhsExpr->IgnoreParenCasts(), blockID,
	rv, f);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain ExprDetails for lhsexpr\n";
	return bd;
    }
    Expr* rhsExpr = B->getRHS();
    if (!rhsExpr) {
	llvm::outs() << "ERROR: NULL rhsExpr in BinaryOperator: "
		     << Helper::prettyPrintExpr(B) << "\n";
	rv = false;
	return bd;
    }
    bd->rhs = ExprDetails::getObject(SM, rhsExpr->IgnoreParenCasts(), blockID,
	rv, f);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain ExprDetails for rhsexpr\n";
	return bd;
    }

    return bd;
}

ConditionalOperatorDetails* ConditionalOperatorDetails::getObject(const SourceManager* SM,
ConditionalOperator* C, unsigned blockID, bool &rv, FunctionDetails* f) {
    rv = true;

    ConditionalOperatorDetails* cd = new ConditionalOperatorDetails;
    cd->lineNum = SM->getExpansionLineNumber(C->getLocStart());
    cd->blockID = blockID;
    cd->origExpr = C;
    cd->origStmt = C;
    cd->func = f;

    cd->condition = ExprDetails::getObject(SM, C->getCond(), blockID, rv, f);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain ExprDetails for condition "
		     << "of ConditionalExpr\n";
	llvm::outs() << Helper::prettyPrintExpr(C) << "\n";
	return cd;
    }
    cd->trueExpr = ExprDetails::getObject(SM, C->getTrueExpr(), blockID, rv, f);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain ExprDetails for trueExpr "
		     << "of ConditionalExpr\n";
	llvm::outs() << Helper::prettyPrintExpr(C) << "\n";
	return cd;
    }
    cd->falseExpr = ExprDetails::getObject(SM, C->getFalseExpr(), blockID, rv, f);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain ExprDetails for falseExpr "
		     << "of ConditionalExpr\n";
	llvm::outs() << Helper::prettyPrintExpr(C) << "\n";
	return cd;
    }

    return cd;
}

ArraySubscriptExprDetails* ArraySubscriptExprDetails::getObject(const
SourceManager* SM, ArraySubscriptExpr* A, unsigned blockID, bool &rv, FunctionDetails* f) {
    rv = true;

    ArraySubscriptExprDetails* ad = new ArraySubscriptExprDetails;
    ad->lineNum = SM->getExpansionLineNumber(A->getLocStart());
    ad->blockID = blockID;
    ad->origExpr = A;
    ad->origStmt = A;
    ad->func = f;

    Expr* base = Helper::getBaseArrayExpr(A);
    std::vector<Expr*> indices = Helper::getArrayIndexExprs(A);

    if (!base) {
	llvm::outs() << "ERROR: Cannot obtain base expr of "
		     << Helper::prettyPrintExpr(A) << "\n";
	rv = false;
	return ad;
    }

    ExprDetails* baseDetails = ExprDetails::getObject(SM, base, blockID, rv, f);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain ExprDetails of base array "
		     << Helper::prettyPrintExpr(base) << "\n";
	rv = false;
	return ad;
    }

    if (!isa<DeclRefExprDetails>(baseDetails)) {
	llvm::outs() << "ERROR: ExprDetails of base array " 
		     << Helper::prettyPrintExpr(base) 
		     << " is not DeclRefExprDetails\n";
	rv = false;
	return ad;
    }

    ad->baseArray = dyn_cast<DeclRefExprDetails>(baseDetails);
    
    for (std::vector<Expr*>::iterator iIt = indices.begin(); iIt !=
	    indices.end(); iIt++) {
	if (!(*iIt)) {
	    llvm::outs() << "ERROR: Cannot obtain index expr of "
			 << Helper::prettyPrintExpr(A) << "\n";
	    rv = false;
	    return ad;
	}

	ExprDetails* indexDetails = ExprDetails::getObject(SM, *iIt, blockID,
	    rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot obtain ExprDetails of index expr "
			 << Helper::prettyPrintExpr(*iIt) << "\n";
	    rv = false;
	    return ad;
	}

	ad->indexExprs.push_back(indexDetails);
    }

    return ad;
}

ReturnStmtDetails* ReturnStmtDetails::getObject(const SourceManager* SM,
ReturnStmt* R, unsigned blockID, bool &rv, FunctionDetails* f) {
    rv = true;
    ReturnStmtDetails* rd = new ReturnStmtDetails;
    rd->blockID = blockID;
    rd->lineNum = SM->getExpansionLineNumber(R->getLocStart());
    rd->origStmt = R;
    rd->func = f;

    Expr* returnExpression = R->getRetValue();
    if (!returnExpression) {
	llvm::outs() << "WARNING: No return expression obtained for the ReturnStmt\n";
	//rv = false;
	return rd;
    }

    rd->retExpr = ExprDetails::getObject(SM, returnExpression, blockID, rv, f);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain ExprDetails for return expression\n";
	rv = false;
	return rd;
    }

    return rd;
}

UnaryExprOrTypeTraitExprDetails*
UnaryExprOrTypeTraitExprDetails::getObject(const SourceManager* SM, 
UnaryExprOrTypeTraitExpr* U, unsigned blockID, bool &rv, FunctionDetails* f) {
    rv = true;
    UnaryExprOrTypeTraitExprDetails* ud = new UnaryExprOrTypeTraitExprDetails;
    ud->blockID = blockID;
    ud->lineNum = SM->getExpansionLineNumber(U->getLocStart());
    ud->origStmt = U;
    ud->origExpr = U;
    ud->func = f;

    ud->kind = U->getKind();
    if (ud->kind != UnaryExprOrTypeTrait::UETT_SizeOf) {
	llvm::outs() << "ERROR: Unsupported UETT_kind "
		     << Helper::prettyPrintExpr(U) << "\n";
	rv = false;
	return ud;
    }

    if (U->isArgumentType()) {
	ud->Ty = U->getArgumentTypeInfo();
#if 0
	llvm::outs() << "ERROR: Argument to UETT is type. Not handling\n";
	llvm::outs() << Helper::prettyPrintExpr(U) << "\n";
	rv = false;
	return ud;
#endif
#ifdef DEBUG
	llvm::outs() << "DEBUG: Argument to UETT is type.\n";
#endif
	return ud;
    } else {
	Expr* argExpr = U->getArgumentExpr()->IgnoreParenCasts();
	if (!argExpr) {
	    llvm::outs() << "ERROR: NULL argExpr in UETT: "
			 << Helper::prettyPrintExpr(U) << "\n";
	    rv = false;
	    return ud;
	}
	ExprDetails* argExprDetails = ExprDetails::getObject(SM, argExpr,
	    blockID, rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot obtain ExprDetails of argExpr of UETT: "
			 << Helper::prettyPrintExpr(U) << "\n";
	    return ud;
	}

	if (!isa<DeclRefExprDetails>(argExprDetails)) {
	    llvm::outs() << "ERROR: argExpr of UETT is not DeclRefExpr: "
			 << Helper::prettyPrintExpr(U) << "\n";
	    rv = false;
	    return ud;
	}

	ud->array = dyn_cast<DeclRefExprDetails>(argExprDetails);
	return ud;
    }
}

InitListExprDetails* InitListExprDetails::getObject(const SourceManager* SM, 
InitListExpr* I, unsigned blockID, bool &rv, FunctionDetails* f) {
    rv = true;
    InitListExprDetails* id = new InitListExprDetails;
    id->blockID = blockID;
    id->lineNum = SM->getExpansionLineNumber(I->getLocStart());
    id->origStmt = I;
    id->origExpr = I;
    id->func = f;

    id->hasArrayFiller = I->hasArrayFiller();
    Expr* arrayFiller = I->getArrayFiller();
    if (arrayFiller) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: ArrayFiller: " 
		     << Helper::prettyPrintExpr(arrayFiller) << "\n";
#endif
	if (!isa<ImplicitValueInitExpr>(arrayFiller)) {
	    id->arrayFiller = ExprDetails::getObject(SM, arrayFiller,
				blockID, rv, f);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot obtain ExprDetails of arrayFiller\n";
		return id;
	    }
	}
    }
    for (unsigned i = 0; i < I->getNumInits(); i++) {
	Expr* initExpr = I->getInit(i);
	if (!initExpr) {
	    llvm::outs() << "ERROR: NULL Init in InitListExpr: "
			 << Helper::prettyPrintExpr(I) << "\n";
	    rv = false;
	    return id;
	}
#ifdef DEBUG
	if (initExpr)
	    llvm::outs() << "DEBUG: Init " << i << ": "
		         << Helper::prettyPrintExpr(initExpr) << "\n";
	else
	    llvm::outs() << "DEBUG: Init " << i << ": NULL\n";
#endif
	ExprDetails* init = ExprDetails::getObject(SM, initExpr, blockID, rv, f);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot obtain ExprDetails of init\n";
	    return id;
	}
	id->inits.push_back(init);
    }

    return id;
}
