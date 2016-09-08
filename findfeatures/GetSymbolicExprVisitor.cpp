#include "GetSymbolicExprVisitor.h"
#include "MainFunction.h"
//#include <z3++.h>

// Destructors of all classes

SymbolicStmt::~SymbolicStmt() {
#if 0
    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator sIt;
    for (sIt = guards.begin(); sIt != guards.end(); sIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt =
		sIt->begin(); gIt != sIt->end(); gIt++) {
	    if (gIt->first)
		delete gIt->first;
	}
    }
#endif
}

#if 0
SymbolicDeclStmt::~SymbolicDeclStmt() {
    if (var)
	delete var;
}

SymbolicReturnStmt::~SymbolicReturnStmt() {
    if (retExpr)
	delete retExpr;
}

SymbolicCallExpr::~SymbolicCallExpr() {
    if (callee) delete callee;

    for (std::vector<SymbolicExpr*>::iterator aIt = callArgExprs.begin(); aIt != 
	    callArgExprs.end(); aIt++) {
	if (*aIt)
	    delete *aIt;
    }
}

SymbolicInitListExpr::~SymbolicInitListExpr() {
    if (arrayFiller) delete arrayFiller;
    for (std::vector<SymbolicExpr*>::iterator iIt = inits.begin(); iIt != 
	    inits.end(); iIt++) {
	if (*iIt) delete *iIt;
    }
}

SymbolicArraySubscriptExpr::~SymbolicArraySubscriptExpr() {
    if (baseArray)
	delete baseArray;
    for (std::vector<SymbolicExpr*>::iterator eIt = indexExprs.begin(); eIt != 
	    indexExprs.end(); eIt++) {
	if (*eIt)
	    delete *eIt;
    }
}

SymbolicArrayRangeExpr::~SymbolicArrayRangeExpr() {
    if (baseArray)
	delete baseArray;
    for (std::vector<std::pair<SymbolicExpr*, SymbolicExpr*> >::iterator eIt = 
	    indexExprs.begin(); eIt != indexExprs.end(); eIt++) {
	if (eIt->first)
	    delete eIt->first;
	if (eIt->second)
	    delete eIt->second;
    }
}

SymbolicUnaryOperator::~SymbolicUnaryOperator() {
    if (opExpr)
	delete opExpr;
}

SymbolicBinaryOperator::~SymbolicBinaryOperator() {
    if (lhs)
	delete lhs;
    if (rhs)
	delete rhs;
}

SymbolicConditionalOperator::~SymbolicConditionalOperator() {
    if (condition)
	delete condition;
    if (trueExpr)
	delete trueExpr;
    if (falseExpr)
	delete falseExpr;
}

SymbolicSpecialExpr::~SymbolicSpecialExpr() {
    if (arrayVar)
	delete arrayVar;
    if (initExpr)
	delete initExpr;
    for (std::vector<SymbolicExpr*>::iterator iIt =
	    indexExprsAtAssignLine.begin(); iIt != 
	    indexExprsAtAssignLine.end(); iIt++) {
	if (*iIt)
	    delete *iIt;
    }
    for (std::vector<std::pair<std::pair<SymbolicExpr*, SymbolicExpr*>, bool> >::iterator iIt = 
	    indexRangesAtAssignLine.begin(); iIt !=
	    indexRangesAtAssignLine.end(); iIt++) {
	if (iIt->first.first)
	    delete iIt->first.first;
	if (iIt->first.second)
	    delete iIt->first.second;
    }
}
#endif

// getZ3Expr() of all classes
z3::expr SymbolicDeclRefExpr::getZ3Expr(z3::context* c, const MainFunction* mObj,
std::vector<LoopDetails> surroundingLoops, bool inputFirst, bool &rv) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicDeclRefExpr::getZ3Expr()\n";
#endif
    rv = true;
    z3::expr retExpr(*c);
    if (vKind == VarKind::FUNC) {
	llvm::outs() << "ERROR: Trying to get z3 expr of func\n";
	rv = false;
	return retExpr;
    } else if (vKind == VarKind::OTHER) {
	llvm::outs() << "ERROR: Trying to get z3 expr of OTHER\n";
	rv = false;
	return retExpr;
    } else if (vKind == VarKind::GARBAGE) {
	llvm::outs() << "ERROR: Trying to get z3 expr of GARBAGE\n";
	rv = false;
	return retExpr;
    }

    if (substituteExpr) {
	retExpr = substituteExpr->getZ3Expr(c, mObj, surroundingLoops, inputFirst, rv);
	return retExpr;
    } else {
	std::stringstream ss;
	toString(ss, mObj, false, rv, inputFirst, false, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While SymbolicDeclRefExpr::toString()\n";
	    return retExpr;
	}
	std::string varName = ss.str();
	z3::sort s = getSortFromType(c, var.type, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot get sort for var: ";
	    var.print();
	    llvm::outs() << "\n";
	    return retExpr;
	}
	if (vKind == VarKind::ARRAY) {
	    for (unsigned i = 0; i < var.arraySizeInfo.size(); i++) {
#if 0
		if (i == 0)
		    s = c->array_sort(c->int_sort(), s);
		else
#endif
		    s = c->array_sort(c->int_sort(), s);
	    }
	}
	return c->constant(varName.c_str(), s);
    }
}

z3::expr SymbolicCallExpr::getZ3Expr(z3::context* c, const MainFunction* mObj, 
std::vector<LoopDetails> surroundingLoops, bool inputFirst, bool &rv) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicCallExpr::getZ3Expr()\n";
#endif
    z3::expr retExpr(*c);
    if (returnExpr) {
	retExpr = returnExpr->getZ3Expr(c, mObj, surroundingLoops, inputFirst, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While getZ3Expr() on returnExpr\n";
	}
    } else {
	rv = false;
	llvm::outs() << "ERROR: Calling getZ3Expr on SymbolicCallExpr\n";
    }
    return retExpr;
}

z3::expr SymbolicArraySubscriptExpr::getZ3Expr(z3::context* c, const MainFunction* mObj, 
std::vector<LoopDetails> surroundingLoops, bool inputFirst, bool &rv) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicArraySubscriptExpr::getZ3Expr()\n";
#endif
    rv = true;
    z3::expr retExpr(*c);
    if (substituteExpr) {
	retExpr = substituteExpr->getZ3Expr(c, mObj, surroundingLoops,
inputFirst, rv);
	return retExpr;
    }
    std::stringstream ss;
    toString(ss, mObj, false, rv, inputFirst, false, surroundingLoops);
    if (!rv) {
	llvm::outs() << "ERROR: SymbolicArraySubscriptExpr::toString()\n";
	return retExpr;
    }
    std::string arrayexprID = ss.str();
    ss.clear();
    baseArray->toString(ss, mObj, false, rv, inputFirst, false, surroundingLoops);
    if (!rv) {
	llvm::outs() << "ERROR: SymbolicDeclRefExpr::toString()\n";
	return retExpr;
    }
    std::string basearrayID = ss.str();
    z3::sort basearraysort(*c);
    z3::sort basetype = getSortFromType(c, baseArray->var.type, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get sort of basetype\n";
	return retExpr;
    }
    for (unsigned i = 0; i < indexExprs.size(); i++) {
	if (i == 0)
	    basearraysort = c->array_sort(c->int_sort(), basetype);
	else
	    basearraysort = c->array_sort(c->int_sort(), basearraysort);
    }
    z3::expr basearrayexpr = c->constant(basearrayID.c_str(), basearraysort);
    ss.clear();
    z3::expr arrayexpr(*c);
    for (std::vector<SymbolicExpr*>::iterator iIt = indexExprs.begin(); iIt != 
	    indexExprs.end(); iIt++) {
	ss.clear();
	z3::expr index = (*iIt)->getZ3Expr(c, mObj, surroundingLoops,
inputFirst, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While getZ3Expr() on indexExpr\n";
	    return retExpr;
	}
	if (iIt == indexExprs.begin())
	    arrayexpr = select(basearrayexpr, index);
	else
	    arrayexpr = select(arrayexpr, index);
    }
    return arrayexpr;
}

z3::expr SymbolicArrayRangeExpr::getZ3Expr(z3::context* c, const MainFunction* mObj,
std::vector<LoopDetails> surroundingLoops, bool inputFirst, bool &rv) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicArrayRangeExpr::getZ3Expr()\n";
#endif
    rv = false;
    llvm::outs() << "ERROR: Calling getZ3Expr on SymbolicArrayRangeExpr\n";
    z3::expr retExpr(*c);
    return retExpr;
}

z3::expr SymbolicUnaryOperator::getZ3Expr(z3::context* c, const MainFunction* mObj,
std::vector<LoopDetails> surroundingLoops, bool inputFirst, bool &rv) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicUnaryOperator::getZ3Expr()\n";
#endif
    rv = true;
    z3::expr retExpr(*c);
    std::stringstream ss;
    toString(ss, mObj, false, rv, inputFirst, false, surroundingLoops);
    if (!rv) {
	llvm::outs() << "ERROR: While SymbolicUnaryOperator::toString()\n";
	return retExpr;
    }

    z3::expr opexpr = opExpr->getZ3Expr(c, mObj, surroundingLoops, inputFirst, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While getZ3Expr on opExpr\n";
	return retExpr;
    }

    if (opKind == UnaryOperatorKind::UO_Minus)
	return (-opexpr);
    else if (opKind == UnaryOperatorKind::UO_Plus)
	return opexpr;
    else if (opKind == UnaryOperatorKind::UO_LNot)
	return (opexpr == 0);
    else {
	rv = false;
	llvm::outs() << "ERROR: Unsupported unary operator in getZ3Expr()\n";
	return retExpr;
    }
}

z3::expr SymbolicFloatingLiteral::getZ3Expr(z3::context* c, const MainFunction* mObj,
std::vector<LoopDetails> surroundingLoops, bool inputFirst, bool &rv) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicFloatingLiteral::getZ3Expr()\n";
#endif
    rv = true;
    z3::expr retExpr(*c);
    std::stringstream ss;
    toString(ss, mObj, false, rv, inputFirst, false, surroundingLoops);
    if (!rv) {
	llvm::outs() << "ERROR: While SymbolicFloatingLiteral::toString()\n";
	return retExpr;
    }
    return c->real_val(ss.str().c_str());
}

z3::expr SymbolicIntegerLiteral::getZ3Expr(z3::context* c, const MainFunction* mObj,
std::vector<LoopDetails> surroundingLoops, bool inputFirst, bool &rv) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicIntegerLiteral::getZ3Expr()\n";
#endif
    rv = true;
    int v = (int)val.getSExtValue();
#ifdef DEBUG
    llvm::outs() << "Val: " << v << "\n";
    prettyPrint(false);
#endif
    //return c->int_val(v);
    return c->num_val(v, c->int_sort());
}

z3::expr SymbolicImaginaryLiteral::getZ3Expr(z3::context* c, const MainFunction* mObj,
std::vector<LoopDetails> surroundingLoops, bool inputFirst, bool &rv) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicImaginaryLiteral::getZ3Expr()\n";
#endif
    rv = true;
    return c->int_val((int)val.getSExtValue());
}

z3::expr SymbolicCXXBoolLiteralExpr::getZ3Expr(z3::context* c, const MainFunction* mObj,
std::vector<LoopDetails> surroundingLoops, bool inputFirst, bool &rv) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicCXXBoolLiteral::getZ3Expr()\n";
#endif
    rv = true;
    if (val)
	return c->int_val(1);
    else
	return c->int_val(0);
}

z3::expr SymbolicCharacterLiteral::getZ3Expr(z3::context* c, const MainFunction* mObj,
std::vector<LoopDetails> surroundingLoops, bool inputFirst, bool &rv) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicCharacterLiteral::getZ3Expr()\n";
#endif
    rv = true;
    return c->int_val(val);
}

z3::expr SymbolicBinaryOperator::getZ3Expr(z3::context* c, const MainFunction* mObj,
std::vector<LoopDetails> surroundingLoops, bool inputFirst, bool &rv) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicBinaryOperator::getZ3Expr()\n";
#endif
    rv = true;
    z3::expr retExpr(*c);
    z3::expr lhsexpr = lhs->getZ3Expr(c, mObj, surroundingLoops, inputFirst, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While getZ3Expr() on lhs\n";
	return retExpr;
    }
    z3::expr rhsexpr = rhs->getZ3Expr(c, mObj, surroundingLoops, inputFirst, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While getZ3Expr() on rhs\n";
	return retExpr;
    }

    if (opKind == BinaryOperatorKind::BO_Mul)
	return lhsexpr * rhsexpr;
    else if (opKind == BinaryOperatorKind::BO_Div)
	return lhsexpr / rhsexpr;
    //else if (opKind == BinaryOperatorKind::BO_Rem)
	//return lhsexpr % rhsexpr;
    else if (opKind == BinaryOperatorKind::BO_Add)
	return lhsexpr + rhsexpr;
    else if (opKind == BinaryOperatorKind::BO_Sub)
	return lhsexpr - rhsexpr;
    else if (opKind == BinaryOperatorKind::BO_LT)
	return lhsexpr < rhsexpr;
    else if (opKind == BinaryOperatorKind::BO_GT)
	return lhsexpr > rhsexpr;
    else if (opKind == BinaryOperatorKind::BO_LE)
	return lhsexpr <= rhsexpr;
    else if (opKind == BinaryOperatorKind::BO_GE)
	return lhsexpr >= rhsexpr;
    else if (opKind == BinaryOperatorKind::BO_EQ)
	return lhsexpr == rhsexpr;
    else if (opKind == BinaryOperatorKind::BO_NE)
	return lhsexpr != rhsexpr;
    else if (opKind == BinaryOperatorKind::BO_LAnd || 
		opKind == BinaryOperatorKind::BO_And) {
	if (lhsexpr.is_bool() && rhsexpr.is_bool())
	    return lhsexpr && rhsexpr;
	else if (lhsexpr.is_bool() && rhsexpr.is_arith())
	    return lhsexpr && (rhsexpr != 0);
	else if (lhsexpr.is_arith() && rhsexpr.is_bool())
	    return (lhsexpr != 0) && rhsexpr;
	else if (lhsexpr.is_arith() && rhsexpr.is_arith())
	    return (lhsexpr != 0 && rhsexpr != 0);
	else {
	    rv = false;
	    llvm::outs() << "ERROR: Unsupported sorts while getZ3Expr() on &&\n";
	    return retExpr;
	}
    } else if (opKind == BinaryOperatorKind::BO_LOr || 
		opKind == BinaryOperatorKind::BO_Or){
	if (lhsexpr.is_bool() && rhsexpr.is_bool())
	    return lhsexpr || rhsexpr;
	else if (lhsexpr.is_bool() && rhsexpr.is_arith())
	    return lhsexpr || (rhsexpr != 0);
	else if (lhsexpr.is_arith() && rhsexpr.is_bool())
	    return (lhsexpr != 0) || rhsexpr;
	else if (lhsexpr.is_arith() && rhsexpr.is_arith())
	    return (lhsexpr != 0 || rhsexpr != 0);
	else {
	    rv = false;
	    llvm::outs() << "ERROR: Unsupported sorts while getZ3Expr() on &&\n";
	    return retExpr;
	}
    } else {
	rv = false;
	llvm::outs() << "ERROR: Unsupported BinaryOperator\n";
	return retExpr;
    }

    rv = false;
    return retExpr;
}

z3::expr SymbolicConditionalOperator::getZ3Expr(z3::context* c, 
const MainFunction* mObj, std::vector<LoopDetails> surroundingLoops, 
bool inputFirst, bool &rv) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicConditionalOperator::getZ3Expr()\n";
#endif
    rv = true;
    z3::expr retExpr(*c);
    z3::expr condexpr = condition->getZ3Expr(c, mObj, surroundingLoops,
	inputFirst, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While getZ3Expr() on condition\n";
	return retExpr;
    }
    z3::expr trueexpr = trueExpr->getZ3Expr(c, mObj, surroundingLoops,
	inputFirst, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While getZ3Expr() on trueExpr\n";
	return retExpr;
    }
    z3::expr falseexpr = falseExpr->getZ3Expr(c, mObj, surroundingLoops,
	inputFirst, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While getZ3Expr() on falseExpr\n";
	return retExpr;
    }
    return ite(condexpr, trueexpr, falseexpr);
}

z3::expr SymbolicSpecialExpr::getZ3Expr(z3::context* c, const MainFunction* mObj,
std::vector<LoopDetails> surroundingLoops, bool inputFirst, bool &rv) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicSpecialExpr::getZ3Expr()\n";
#endif
    rv = true;
    z3::expr retExpr(*c);
    if (!sane) {
	llvm::outs() << "ERROR: Calling getZ3Expr() on SymbolicSpecialExpr\n";
	rv = false;
	return retExpr;
    }

    std::string specialexprvarname = "";
    for (std::vector<std::pair<SymbolicSpecialExpr*, std::string> >::const_iterator
	    sIt = mObj->z3varsForSpecialExprs.begin(); sIt !=
	    mObj->z3varsForSpecialExprs.end(); sIt++) {
	if (this->equals(sIt->first)) {
	    specialexprvarname = sIt->second;
	    break;
	}
    }
    if (specialexprvarname.compare("") == 0) {
	std::stringstream ss;
	toString(ss, mObj, false, rv, inputFirst, false, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While toString() in SpecialExpr\n";
	    return retExpr;
	}
	specialexprvarname = ss.str();
	const_cast<MainFunction*>(mObj)->z3varsForSpecialExprs.push_back(std::make_pair(this,
	    specialexprvarname));
    }
    z3::sort s = getSortFromType(c, arrayVar->var.type, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While getSortFromType()\n";
	return retExpr;
    }
    retExpr = c->constant(specialexprvarname.c_str(), s);
    return retExpr;
}

// clone method of all classes

SymbolicStmt* SymbolicStmt::clone(bool &rv) {
    rv = true;
    SymbolicStmt* cloneObj = new SymbolicStmt(getKind());
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;
    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicStmt::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }
    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
	    varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	if (!(*vrIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicStmt::varsReferenced\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneVR = (*vrIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
	    llvm::outs() << "ERROR: Clone of VarRef is not "
			 << "SymbolicDeclRefExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->varsReferenced.push_back(
	    dyn_cast<SymbolicDeclRefExpr>(cloneVR));
    }
    return cloneObj;
}

SymbolicStmt* SymbolicDeclStmt::clone(bool &rv) {
    rv = true;
    SymbolicDeclStmt* cloneObj = new SymbolicDeclStmt(getKind());
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicDeclStmt::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    // SymbolicDeclStmt details
    if (!var) {
	llvm::outs() << "ERROR: NULL SymbolicDeclStmt::var\n";
	rv = false;
	return cloneObj;
    }
    SymbolicStmt* cloneVar = var->clone(rv);
    if (!rv) return cloneObj;
    if (!isa<SymbolicVarDecl>(cloneVar)) {
	llvm::outs() << "ERROR: Clone of declared var is not SymbolicVarDecl\n";
	rv = false;
	return cloneObj;
    }
    cloneObj->var = dyn_cast<SymbolicVarDecl>(var);
    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
	    varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	if (!(*vrIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicStmt::varsReferenced\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneVR = (*vrIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
	    llvm::outs() << "ERROR: Clone of VarRef is not "
			 << "SymbolicDeclRefExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->varsReferenced.push_back(
	    dyn_cast<SymbolicDeclRefExpr>(cloneVR));
    }
    return cloneObj;
}

SymbolicStmt* SymbolicVarDecl::clone(bool &rv) {
    rv = true;
    SymbolicVarDecl* cloneObj = new SymbolicVarDecl;
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicVarDecl::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

#if 0
    // SymbolicDeclStmt details
    if (!var) {
	llvm::outs() << "ERROR: NULL SymbolicVarDecl::var\n";
	print();
	rv = false;
	return cloneObj;
    }
    SymbolicStmt* cloneVar = var->clone(rv);
    if (!rv) return cloneObj;
    if (!isa<SymbolicVarDecl>(cloneVar)) {
	llvm::outs() << "ERROR: Clone of declared var is not SymbolicVarDecl\n";
	rv = false;
	return cloneObj;
    }
    cloneObj->var = dyn_cast<SymbolicVarDecl>(var);
#endif
    // SymbolicVarDecl details
    cloneObj->varD = varD.clone();
    if (hasInit()) {
	SymbolicStmt* cloneInitExpr = initExpr->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicExpr>(cloneInitExpr)) {
	    llvm::outs() << "ERROR: Clone of initExpr is not SymbolicExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->initExpr = dyn_cast<SymbolicExpr>(cloneInitExpr);
    }
    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
	    varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	if (!(*vrIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicStmt::varsReferenced\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneVR = (*vrIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
	    llvm::outs() << "ERROR: Clone of VarRef is not "
			 << "SymbolicDeclRefExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->varsReferenced.push_back(
	    dyn_cast<SymbolicDeclRefExpr>(cloneVR));
    }
    return cloneObj;
}

SymbolicStmt* SymbolicReturnStmt::clone(bool &rv) {
    rv = true;
    SymbolicReturnStmt* cloneObj = new SymbolicReturnStmt;
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicReturnStmt::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    // SymbolicReturnStmt details
    if (retExpr) {
	SymbolicStmt* cloneRetExpr = retExpr->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicExpr>(cloneRetExpr)) {
	    llvm::outs() << "ERROR: Clone of retExpr is not SymbolicExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->retExpr = dyn_cast<SymbolicExpr>(cloneRetExpr);
    }
    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
	    varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	if (!(*vrIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicStmt::varsReferenced\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneVR = (*vrIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
	    llvm::outs() << "ERROR: Clone of VarRef is not "
			 << "SymbolicDeclRefExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->varsReferenced.push_back(
	    dyn_cast<SymbolicDeclRefExpr>(cloneVR));
    }
    return cloneObj;
}

SymbolicStmt* SymbolicExpr::clone(bool &rv) {
    rv = true;
    SymbolicExpr* cloneObj = new SymbolicExpr(getKind());
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicExpr::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
	    varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	if (!(*vrIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicStmt::varsReferenced\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneVR = (*vrIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
	    llvm::outs() << "ERROR: Clone of VarRef is not "
			 << "SymbolicDeclRefExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->varsReferenced.push_back(
	    dyn_cast<SymbolicDeclRefExpr>(cloneVR));
    }
    return cloneObj;
}

SymbolicStmt* SymbolicCallExpr::clone(bool &rv) {
    rv = true;
    SymbolicCallExpr* cloneObj = new SymbolicCallExpr(getKind());
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicCallExpr::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    // SymbolicCallExpr details
    if (!callee) {
	llvm::outs() << "ERROR: NULL SymbolicCallExpr::callee\n";
	rv = false;
	return cloneObj;
    }
    SymbolicStmt* cloneCallee = callee->clone(rv);
    if (!rv) return cloneObj;
    if (!isa<SymbolicExpr>(cloneCallee)) {
	llvm::outs() << "ERROR: Clone of callee is not SymbolicExpr\n";
	rv = false;
	return cloneObj;
    }
    cloneObj->callee = dyn_cast<SymbolicExpr>(cloneCallee);
    for (std::vector<SymbolicExpr*>::iterator aIt = callArgExprs.begin();
	    aIt != callArgExprs.end(); aIt++) {
	if (!(*aIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicCallExpr::callArgExpr\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneArgExpr = (*aIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicExpr>(cloneArgExpr)) {
	    llvm::outs() << "ERROR: Clone of argExpr is not SymbolicExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->callArgExprs.push_back(dyn_cast<SymbolicExpr>(cloneArgExpr));
    }
#if 0
    for (std::vector<SymbolicExpr*>::iterator rIt = returnExprs.begin();
	    rIt != returnExprs.end(); rIt++) {
	if (!(*rIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicCallExpr::returnExpr\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneRetExpr = (*rIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicExpr>(cloneRetExpr)) {
	    llvm::outs() << "ERROR: Clone of retExpr is not SymbolicExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->returnExprs.push_back(dyn_cast<SymbolicExpr>(cloneRetExpr));
    }
#endif
    if (returnExpr) {
	SymbolicStmt* cloneRetExpr = returnExpr->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicExpr>(cloneRetExpr)) {
	    llvm::outs() << "ERROR: Clone of retExpr is not SymbolicExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->returnExpr = dyn_cast<SymbolicExpr>(cloneRetExpr);
    }

    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
	    varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	if (!(*vrIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicStmt::varsReferenced\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneVR = (*vrIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
	    llvm::outs() << "ERROR: Clone of VarRef is not "
			 << "SymbolicDeclRefExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->varsReferenced.push_back(
	    dyn_cast<SymbolicDeclRefExpr>(cloneVR));
    }
    return cloneObj;
}

SymbolicStmt* SymbolicCXXOperatorCallExpr::clone(bool &rv) {
    rv = true;
    SymbolicCXXOperatorCallExpr* cloneObj = new
	SymbolicCXXOperatorCallExpr;
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicCXXOperatorCallExpr::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    // SymbolicCallExpr details
    for (std::vector<SymbolicExpr*>::iterator aIt = callArgExprs.begin();
	    aIt != callArgExprs.end(); aIt++) {
	if (!(*aIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicCXXOperatorCallExpr::callArgExpr\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneArgExpr = (*aIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicExpr>(cloneArgExpr)) {
	    llvm::outs() << "ERROR: Clone of argExpr is not SymbolicExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->callArgExprs.push_back(dyn_cast<SymbolicExpr>(cloneArgExpr));
    }
    // SymbolicCXXOperatorCallExpr details
    cloneObj->opKind = opKind;
    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
	    varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	if (!(*vrIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicStmt::varsReferenced\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneVR = (*vrIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
	    llvm::outs() << "ERROR: Clone of VarRef is not "
			 << "SymbolicDeclRefExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->varsReferenced.push_back(
	    dyn_cast<SymbolicDeclRefExpr>(cloneVR));
    }
    return cloneObj;
}

SymbolicStmt* SymbolicInitListExpr::clone(bool &rv) {
    rv = true;
    SymbolicInitListExpr* cloneObj = new
	SymbolicInitListExpr;
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicCXXOperatorCallExpr::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    // SymbolicInitListExpr details
    cloneObj->hasArrayFiller = hasArrayFiller;
    if (arrayFiller) {
	SymbolicStmt* cloneFiller = arrayFiller->clone(rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While cloning arrayFiller\n";
	    return cloneObj;
	}
	if (!isa<SymbolicExpr>(cloneFiller)) {
	    llvm::outs() << "ERROR: Clone of arrayFiller is not SymbolicExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->arrayFiller = dyn_cast<SymbolicExpr>(cloneFiller);
    }
    for (std::vector<SymbolicExpr*>::iterator iIt = inits.begin(); iIt != 
	    inits.end(); iIt++) {
	if (*iIt) {
	    SymbolicStmt* cloneInit = (*iIt)->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While cloning init\n";
		return cloneObj;
	    }
	    if (!isa<SymbolicExpr>(cloneInit)) {
		llvm::outs() << "ERROR: Clone of init is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    cloneObj->inits.push_back(dyn_cast<SymbolicExpr>(cloneInit));
	}
    }
    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
	    varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	if (!(*vrIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicStmt::varsReferenced\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneVR = (*vrIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
	    llvm::outs() << "ERROR: Clone of VarRef is not "
			 << "SymbolicDeclRefExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->varsReferenced.push_back(
	    dyn_cast<SymbolicDeclRefExpr>(cloneVR));
    }
    return cloneObj;
}

SymbolicStmt* SymbolicUnaryExprOrTypeTraitExpr::clone(bool &rv) {
    rv = true;
    SymbolicUnaryExprOrTypeTraitExpr* cloneObj = new SymbolicUnaryExprOrTypeTraitExpr;
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicUnaryExprOrTypeTraitExpr::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    // SymbolicUnaryExprOrTypeTraitExpr details
    cloneObj->kind = kind;
    if (!array && !Ty) {
	llvm::outs() << "ERROR: NULL array && Type in UETT\n";
	rv = false;
	return cloneObj;
    } else if (!array) {
	cloneObj->Ty = Ty;
    }
    SymbolicStmt* cloneArray = array->clone(rv);
    if (!rv) return cloneObj;
    if (!isa<SymbolicDeclRefExpr>(cloneArray)) {
	llvm::outs() << "ERROR: Clone of SymbolicDeclRefExpr is not "
		     << "<SymbolicDeclRefExpr>\n";
	rv = false;
	return cloneObj;
    }
    cloneObj->array = dyn_cast<SymbolicDeclRefExpr>(cloneArray);
    for (std::vector<SymbolicExpr*>::iterator sIt = sizeExprs.begin(); sIt != 
	    sizeExprs.end(); sIt++) {
	if (!(*sIt)) {
	    llvm::outs() << "ERROR: NULL sizeExpr\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneExpr = (*sIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicExpr>(cloneExpr)) {
	    llvm::outs() << "ERROR: Clone of SymbolicExpr is not "
			 << "<SymbolicExpr>\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->sizeExprs.push_back(dyn_cast<SymbolicExpr>(cloneExpr));
    }

    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
	    varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	if (!(*vrIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicStmt::varsReferenced\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneVR = (*vrIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
	    llvm::outs() << "ERROR: Clone of VarRef is not "
			 << "SymbolicDeclRefExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->varsReferenced.push_back(
	    dyn_cast<SymbolicDeclRefExpr>(cloneVR));
    }
    return cloneObj;
}

SymbolicStmt* SymbolicDeclRefExpr::clone(bool &rv) {
    rv = true;
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: SymbolicDeclRefExpr::clone(): ";
    prettyPrint(false);
    llvm::outs() << "\n";
#endif
    SymbolicDeclRefExpr* cloneObj = new SymbolicDeclRefExpr;
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicDeclRefExpr::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    // SymbolicDeclRefExpr details
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: cloning var\n";
#endif
    cloneObj->var = var.clone();
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: after cloning var\n";
    prettyPrint(false);
    llvm::outs() << "\n";
#endif
    cloneObj->fd = fd;
    cloneObj->vKind = vKind;
    if (substituteExpr) {
	SymbolicStmt* cloneExpr = substituteExpr->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicExpr>(cloneExpr)) {
	    llvm::outs() << "ERROR: Clone of substituteExpr is not "
			 << "<SymbolicExpr>\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->substituteExpr = dyn_cast<SymbolicExpr>(cloneExpr);
    }
    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
	    varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	if (!(*vrIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicStmt::varsReferenced\n";
	    rv = false;
	    return cloneObj;
	}
	if (this->equals(*vrIt)) {
	    cloneObj->varsReferenced.push_back(cloneObj);
	} else {
	    SymbolicStmt* cloneVR = (*vrIt)->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
		llvm::outs() << "ERROR: Clone of VarRef is not "
			     << "SymbolicDeclRefExpr\n";
		rv = false;
		return cloneObj;
	    }
	    cloneObj->varsReferenced.push_back(
		dyn_cast<SymbolicDeclRefExpr>(cloneVR));
	}
    }
    return cloneObj;
}

SymbolicStmt* SymbolicArraySubscriptExpr::clone(bool &rv) {
    rv = true;
    SymbolicArraySubscriptExpr* cloneObj = new SymbolicArraySubscriptExpr;
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicArraySubscriptExpr::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    // SymbolicArraySubscriptExpr details
    if (!baseArray) {
	llvm::outs() << "ERROR: NULL SymbolicArraySubscriptExpr::baseArray\n";
	rv = false;
	return cloneObj;
    }
    SymbolicStmt* cloneBase = baseArray->clone(rv);
    if (!rv) return cloneObj;
    if (!isa<SymbolicDeclRefExpr>(cloneBase)) {
	llvm::outs() << "ERROR: Clone of base array is not "
		     << "SymbolicDeclRefExpr\n";
	rv = false;
	return cloneObj;
    }
    cloneObj->baseArray = dyn_cast<SymbolicDeclRefExpr>(cloneBase);
    for (std::vector<SymbolicExpr*>::iterator iIt = indexExprs.begin();
	    iIt != indexExprs.end(); iIt++) {
	if (!(*iIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicArraySubscriptExpr::indexExpr\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneIndex = (*iIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicExpr>(cloneIndex)) {
	    llvm::outs() << "ERROR: Clone of index expr is not SymbolicExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->indexExprs.push_back(dyn_cast<SymbolicExpr>(cloneIndex));
    }
    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
	    varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	if (!(*vrIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicStmt::varsReferenced\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneVR = (*vrIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
	    llvm::outs() << "ERROR: Clone of VarRef is not "
			 << "SymbolicDeclRefExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->varsReferenced.push_back(
	    dyn_cast<SymbolicDeclRefExpr>(cloneVR));
    }
    if (substituteExpr) {
	SymbolicStmt* cloneSubstituteExpr = substituteExpr->clone(rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While cloning substituteExpr\n";
	    return cloneObj;
	}
	if (!isa<SymbolicExpr>(cloneSubstituteExpr)) {
	    llvm::outs() << "ERROR: clone of substituteExpr is not <SymbolicExpr>\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->substituteExpr = dyn_cast<SymbolicExpr>(cloneSubstituteExpr);
    }
    return cloneObj;
}

SymbolicStmt* SymbolicArrayRangeExpr::clone(bool &rv) {
    rv = true;
    SymbolicArrayRangeExpr* cloneObj = new SymbolicArrayRangeExpr;
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicArrayRangeExpr::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    // SymbolicArrayRangeExpr details
    if (!baseArray) {
	llvm::outs() << "ERROR: NULL SymbolicArrayRangeExpr::baseArray\n";
	rv = false;
	return cloneObj;
    }
    SymbolicStmt* cloneBase = baseArray->clone(rv);
    if (!rv) return cloneObj;
    if (!isa<SymbolicDeclRefExpr>(cloneBase)) {
	llvm::outs() << "ERROR: Clone of base array is not "
		     << "SymbolicDeclRefExpr\n";
	rv = false;
	return cloneObj;
    }
    cloneObj->baseArray = dyn_cast<SymbolicDeclRefExpr>(cloneBase);
    for (std::vector<std::pair<SymbolicExpr*, SymbolicExpr*> >::iterator iIt = 
	    indexRangeExprs.begin(); iIt != indexRangeExprs.end(); iIt++) {
	if (!(iIt->first)) {
	    llvm::outs() << "ERROR: NULL SymbolicArrayRangeExpr::indexExpr1\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneIndex1 = iIt->first->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicExpr>(cloneIndex1)) {
	    llvm::outs() << "ERROR: Clone of index expr is not SymbolicExpr\n";
	    rv = false;
	    return cloneObj;
	}
	if (!(iIt->second)) {
	    llvm::outs() << "ERROR: NULL SymbolicArrayRangeExpr::indexExpr2\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneIndex2 = iIt->second->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicExpr>(cloneIndex2)) {
	    llvm::outs() << "ERROR: Clone of index expr is not SymbolicExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->indexRangeExprs.push_back(
	    std::make_pair(dyn_cast<SymbolicExpr>(cloneIndex1),
	    dyn_cast<SymbolicExpr>(cloneIndex2)));
    }
    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
	    varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	if (!(*vrIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicStmt::varsReferenced\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneVR = (*vrIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
	    llvm::outs() << "ERROR: Clone of VarRef is not "
			 << "SymbolicDeclRefExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->varsReferenced.push_back(
	    dyn_cast<SymbolicDeclRefExpr>(cloneVR));
    }
    return cloneObj;
}

SymbolicStmt* SymbolicUnaryOperator::clone(bool &rv) {
    rv = true;
    SymbolicUnaryOperator* cloneObj = new SymbolicUnaryOperator;
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicUnaryOperator::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    // SymbolicUnaryOperator details
    cloneObj->opKind = opKind;
    if (!opExpr) {
	llvm::outs() << "ERROR: NULL SymbolicUnaryOperator::opExpr\n";
	rv = false;
	return cloneObj;
    }
    SymbolicStmt* cloneOpExpr = opExpr->clone(rv);
    if (!rv) return cloneObj;
    if (!isa<SymbolicExpr>(cloneOpExpr)) {
	llvm::outs() << "ERROR: Clone of opExpr is not SymbolicExpr\n";
	rv = false;
	return cloneObj;
    }
    cloneObj->opExpr = dyn_cast<SymbolicExpr>(cloneOpExpr);
    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
	    varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	if (!(*vrIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicStmt::varsReferenced\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneVR = (*vrIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
	    llvm::outs() << "ERROR: Clone of VarRef is not "
			 << "SymbolicDeclRefExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->varsReferenced.push_back(
	    dyn_cast<SymbolicDeclRefExpr>(cloneVR));
    }
    return cloneObj;
}

SymbolicStmt* SymbolicIntegerLiteral::clone(bool &rv) {
    rv = true;
    SymbolicIntegerLiteral* cloneObj = new SymbolicIntegerLiteral;
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicIntegerLiteral::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    // SymbolicIntegerLiteral details
    cloneObj->val = val;
    if (varsReferenced.size() != 0) {
	llvm::outs() << "ERROR: SymbolicIntegerLiteral has non-zero "
		     << "varsReferenced:\n";
	for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
		varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	    (*vrIt)->print();
	    llvm::outs() << "\n";
	}
	rv = false;
	return cloneObj;
    }
    return cloneObj;
}

SymbolicStmt* SymbolicImaginaryLiteral::clone(bool &rv) {
    rv = true;
    SymbolicImaginaryLiteral* cloneObj = new SymbolicImaginaryLiteral;
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicImaginaryLiteral::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    // SymbolicImaginaryLiteral details
    cloneObj->val = val;
    if (varsReferenced.size() != 0) {
	llvm::outs() << "ERROR: SymbolicImaginaryLiteral has non-zero "
		     << "varsReferenced:\n";
	for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
		varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	    (*vrIt)->print();
	    llvm::outs() << "\n";
	}
	rv = false;
	return cloneObj;
    }
    return cloneObj;
}

SymbolicStmt* SymbolicFloatingLiteral::clone(bool &rv) {
    rv = true;
    SymbolicFloatingLiteral* cloneObj = new SymbolicFloatingLiteral;
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicFloatingLiteral::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    // SymbolicFloatingLiteral details
    cloneObj->val = val;
    if (varsReferenced.size() != 0) {
	llvm::outs() << "ERROR: SymbolicFloatingLiteral has non-zero "
		     << "varsReferenced:\n";
	for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
		varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	    (*vrIt)->print();
	    llvm::outs() << "\n";
	}
	rv = false;
	return cloneObj;
    }
    return cloneObj;
}
SymbolicStmt* SymbolicCXXBoolLiteralExpr::clone(bool &rv) {
    rv = true;
    SymbolicCXXBoolLiteralExpr* cloneObj = new SymbolicCXXBoolLiteralExpr;
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicCXXBoolLiteralExpr::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    // SymbolicCXXBoolLiteralExpr details
    cloneObj->val = val;
    if (varsReferenced.size() != 0) {
	llvm::outs() << "ERROR: SymbolicCXXBoolLiteralExpr has non-zero "
		     << "varsReferenced:\n";
	for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
		varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	    (*vrIt)->print();
	    llvm::outs() << "\n";
	}
	rv = false;
	return cloneObj;
    }
    return cloneObj;
}

SymbolicStmt* SymbolicCharacterLiteral::clone(bool &rv) {
    rv = true;
    SymbolicCharacterLiteral* cloneObj = new SymbolicCharacterLiteral;
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicCharacterLiteral::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    // SymbolicCharacterLiteral details
    cloneObj->val = val;
    if (varsReferenced.size() != 0) {
	llvm::outs() << "ERROR: SymbolicCharacterLiteral has non-zero "
		     << "varsReferenced:\n";
	for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
		varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	    (*vrIt)->print();
	    llvm::outs() << "\n";
	}
	rv = false;
	return cloneObj;
    }
    return cloneObj;
}

SymbolicStmt* SymbolicStringLiteral::clone(bool &rv) {
    rv = true;
    SymbolicStringLiteral* cloneObj = new SymbolicStringLiteral;
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicStringLiteral::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    // SymbolicStringLiteral details
    cloneObj->val = val;
    if (varsReferenced.size() != 0) {
	llvm::outs() << "ERROR: SymbolicStringLiteral has non-zero "
		     << "varsReferenced:\n";
	for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
		varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	    (*vrIt)->print();
	    llvm::outs() << "\n";
	}
	rv = false;
	return cloneObj;
    }
    return cloneObj;
}

SymbolicStmt* SymbolicBinaryOperator::clone(bool &rv) {
    rv = true;
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: SymbolicBinaryOperator::clone(): ";
    prettyPrint(false);
    llvm::outs() << "\n";
#endif
    SymbolicBinaryOperator* cloneObj = new SymbolicBinaryOperator;
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicBinaryOperator::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    // SymbolicBinaryOperator details
    cloneObj->opKind = opKind;
    if (!lhs) {
	llvm::outs() << "ERROR: NULL SymbolicBinaryOperator::lhs\n";
	rv = false;
	return cloneObj;
    }
    SymbolicStmt* cloneLHS = lhs->clone(rv);
    if (!rv) return cloneObj;
    if (!isa<SymbolicExpr>(cloneLHS)) {
	llvm::outs() << "ERROR: Clone of lhsExpr is not SymbolicExpr\n";
	rv = false;
	return cloneObj;
    }
    cloneObj->lhs = dyn_cast<SymbolicExpr>(cloneLHS);
    if (!rhs) {
	llvm::outs() << "ERROR: NULL SymbolicBinaryOperator::rhs\n";
	rv = false;
	return cloneObj;
    }
    SymbolicStmt* cloneRHS = rhs->clone(rv);
    if (!rv) return cloneObj;
    if (!isa<SymbolicExpr>(cloneRHS)) {
	llvm::outs() << "ERROR: Clone of rhsExpr is not SymbolicExpr\n";
	rv = false;
	return cloneObj;
    }
    cloneObj->rhs = dyn_cast<SymbolicExpr>(cloneRHS);
    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
	    varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	if (!(*vrIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicStmt::varsReferenced\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneVR = (*vrIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
	    llvm::outs() << "ERROR: Clone of VarRef is not "
			 << "SymbolicDeclRefExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->varsReferenced.push_back(
	    dyn_cast<SymbolicDeclRefExpr>(cloneVR));
    }
    return cloneObj;
}

SymbolicStmt* SymbolicConditionalOperator::clone(bool &rv) {
    rv = true;
    SymbolicConditionalOperator* cloneObj = new SymbolicConditionalOperator;
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicConditionalOperator::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    // SymbolicConditionalOperator details
    if (!condition) {
	llvm::outs() << "ERROR: NULL SymbolicConditionalOperator::condition\n";
	rv = false;
	return cloneObj;
    }
    SymbolicStmt* cloneCondition = condition->clone(rv);
    if (!rv) return cloneObj;
    if (!isa<SymbolicExpr>(cloneCondition)) {
	llvm::outs() << "ERROR: Clone of conditionExpr is not SymbolicExpr\n";
	rv = false;
	return cloneObj;
    }
    cloneObj->condition = dyn_cast<SymbolicExpr>(cloneCondition);
    if (!trueExpr) {
	llvm::outs() << "ERROR: NULL SymbolicConditionalOperator::trueExpr\n";
	rv = false;
	return cloneObj;
    }
    SymbolicStmt* cloneTrueExpr = trueExpr->clone(rv);
    if (!rv) return cloneObj;
    if (!isa<SymbolicExpr>(cloneTrueExpr)) {
	llvm::outs() << "ERROR: Clone of trueExprExpr is not SymbolicExpr\n";
	rv = false;
	return cloneObj;
    }
    cloneObj->trueExpr = dyn_cast<SymbolicExpr>(cloneTrueExpr);
    if (!falseExpr) {
	llvm::outs() << "ERROR: NULL SymbolicConditionalOperator::falseExpr\n";
	rv = false;
	return cloneObj;
    }
    SymbolicStmt* cloneFalseExpr = falseExpr->clone(rv);
    if (!rv) return cloneObj;
    if (!isa<SymbolicExpr>(cloneFalseExpr)) {
	llvm::outs() << "ERROR: Clone of falseExprExpr is not SymbolicExpr\n";
	rv = false;
	return cloneObj;
    }
    cloneObj->falseExpr = dyn_cast<SymbolicExpr>(cloneFalseExpr);
    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
	    varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	if (!(*vrIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicStmt::varsReferenced\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneVR = (*vrIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
	    llvm::outs() << "ERROR: Clone of VarRef is not "
			 << "SymbolicDeclRefExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->varsReferenced.push_back(
	    dyn_cast<SymbolicDeclRefExpr>(cloneVR));
    }
    return cloneObj;
}

SymbolicStmt* SymbolicSpecialExpr::clone(bool &rv) {
    rv = true;
    SymbolicSpecialExpr* cloneObj = new SymbolicSpecialExpr;
    // SymbolicStmt details
    cloneObj->resolved = resolved;
    cloneObj->isDummy = isDummy;

    cloneObj->guardBlocks.insert(cloneObj->guardBlocks.end(), guardBlocks.begin(), guardBlocks.end());
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > guardVec;
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!(sIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicSpecialExpr::guard\n";
		rv = false;
		return cloneObj;
	    }
	    SymbolicStmt* cloneExpr = sIt->first->clone(rv);
	    if (!rv) return cloneObj;
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of guardExpr is not SymbolicExpr\n";
		rv = false;
		return cloneObj;
	    }
	    guardVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(cloneExpr),
		sIt->second));
	}
	cloneObj->guards.push_back(guardVec);
    }

    // SymbolicSpecialExpr details
    cloneObj->sKind = sKind;
    if (!arrayVar) {
	llvm::outs() << "ERROR: NULL SymbolicSpecialExpr::arrayVar\n";
	rv = false;
	return cloneObj;
    }
    SymbolicStmt* cloneArray = arrayVar->clone(rv);
    if (!rv) return cloneObj;
    if (!isa<SymbolicDeclRefExpr>(cloneArray)) {
	llvm::outs() << "ERROR: Clone of arrayVar is not SymbolicDeclRefExpr\n";
	rv = false;
	return cloneObj;
    }
    cloneObj->arrayVar = dyn_cast<SymbolicDeclRefExpr>(cloneArray);
    if (!initExpr) {
	llvm::outs() << "ERROR: NULL SymbolicSpecialExpr::initExpr\n";
	rv = false;
	return cloneObj;
    }
    SymbolicStmt* cloneInitExpr = initExpr->clone(rv);
    if (!rv) return cloneObj;
    if (!isa<SymbolicExpr>(cloneInitExpr)) {
	llvm::outs() << "ERROR: Clone of initExpr is not SymbolicExpr\n";
	rv = false;
	return cloneObj;
    }
    cloneObj->initExpr = dyn_cast<SymbolicExpr>(cloneInitExpr);
    for (std::vector<SymbolicExpr*>::iterator iIt =
	    indexExprsAtAssignLine.begin(); iIt != indexExprsAtAssignLine.end();
	    iIt++) {
	if (!(*iIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicSpecialExpr::indexExprsAtAssignLine\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneIndex = (*iIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicExpr>(cloneIndex)) {
	    llvm::outs() << "ERROR: Clone of indexexpr is not SymbolicExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->indexExprsAtAssignLine.push_back(
	    dyn_cast<SymbolicExpr>(cloneIndex));
    }
    for (std::vector<std::pair<RANGE, bool> >::iterator irIt = 
	    indexRangesAtAssignLine.begin(); irIt != 
	    indexRangesAtAssignLine.end(); irIt++) {
	if (!(irIt->first.first)) {
	    llvm::outs() << "ERROR: NULL SymbolicSpecialExpr::indexRange1\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneRange1 = irIt->first.first->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicExpr>(cloneRange1)) {
	    llvm::outs() << "ERROR: Clone of range expr1 is not SymbolicExpr\n";
	    rv = false;
	    return cloneObj;
	}
	if (!(irIt->first.second)) {
	    llvm::outs() << "ERROR: NULL SymbolicSpecialExpr::indexRange2\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneRange2 = irIt->first.second->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicExpr>(cloneRange2)) {
	    llvm::outs() << "ERROR: Clone of range expr2 is not SymbolicExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->indexRangesAtAssignLine.push_back(
	    std::make_pair(
		std::make_pair(dyn_cast<SymbolicExpr>(cloneRange1),
		    dyn_cast<SymbolicExpr>(cloneRange2)),
		irIt->second));
    }
    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
	    varsReferenced.begin(); vrIt != varsReferenced.end(); vrIt++) {
	if (!(*vrIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicStmt::varsReferenced\n";
	    rv = false;
	    return cloneObj;
	}
	SymbolicStmt* cloneVR = (*vrIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
	    llvm::outs() << "ERROR: Clone of VarRef is not "
			 << "SymbolicDeclRefExpr\n";
	    rv = false;
	    return cloneObj;
	}
	cloneObj->varsReferenced.push_back(
	    dyn_cast<SymbolicDeclRefExpr>(cloneVR));
    }
    cloneObj->sane = sane;
    cloneObj->originalSpecialExpr = originalSpecialExpr;
    return cloneObj;
}

// replaceArrayExpr of all classes
void SymbolicCallExpr::replaceArrayExprWithExpr(
std::vector<SymbolicArraySubscriptExpr*> origExpr, 
std::vector<SymbolicExpr*> substituteExpr, bool &rv) {
    rv = true;
    if (!callee) {
	llvm::outs() << "ERROR: NULL callee\n";
	rv = false;
	return;
    }
    callee->replaceArrayExprWithExpr(origExpr, substituteExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replaceArrayExprWithExpr on callee\n";
	return;
    }
    for (std::vector<SymbolicExpr*>::iterator iIt = callArgExprs.begin(); iIt != 
	    callArgExprs.end(); iIt++) {
	(*iIt)->replaceArrayExprWithExpr(origExpr, substituteExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replaceArrayExprWithExpr on arg\n";
	    return;
	}
    }
    if (returnExpr) {
	returnExpr->replaceArrayExprWithExpr(origExpr, substituteExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replaceArrayExprWithExpr on returnExpr\n";
	    return;
	}
    }
}

void SymbolicCXXOperatorCallExpr::replaceArrayExprWithExpr(
std::vector<SymbolicArraySubscriptExpr*> origExpr, 
std::vector<SymbolicExpr*> substituteExpr, bool &rv) {
    rv = true;
    SymbolicCallExpr::replaceArrayExprWithExpr(origExpr, substituteExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replaceArrayExprWithExpr on CXXOperator\n";
	return;
    }
}

void SymbolicArraySubscriptExpr::replaceArrayExprWithExpr(
std::vector<SymbolicArraySubscriptExpr*> origExpr, 
std::vector<SymbolicExpr*> subExpr, bool &rv) {
    rv = true;
    if (substituteExpr) {
	substituteExpr->replaceArrayExprWithExpr(origExpr, subExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replaceArrayExprWithExpr() on substituteExpr\n";
	    return;
	}
	return;
    }

    for (unsigned i = 0; i < origExpr.size(); i++) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: this: ";
	prettyPrint(false);
	llvm::outs() << "\nDEBUG: origExpr[i]: ";
	origExpr[i]->prettyPrint(false);
#endif
	if (this->equals(origExpr[i])) 
	{
#ifdef DEBUG
	    llvm::outs() << "EQUAL\n";
#endif
	    substituteExpr = subExpr[i];
	    break;
	} else {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Not equal\n";
#endif
	}
    }
}

void SymbolicArrayRangeExpr::replaceArrayExprWithExpr(
std::vector<SymbolicArraySubscriptExpr*> origExpr, 
std::vector<SymbolicExpr*> subExpr, bool &rv) {
    rv = true;
    for (std::vector<std::pair<SymbolicExpr*, SymbolicExpr*> >::iterator iIt = 
	    indexRangeExprs.begin(); iIt != indexRangeExprs.end(); iIt++) {
	iIt->first->replaceArrayExprWithExpr(origExpr, subExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replaceArrayExprWithExpr in indexExpr\n";
	    return;
	}
	iIt->second->replaceArrayExprWithExpr(origExpr, subExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replaceArrayExprWithExpr in indexExpr\n";
	    return;
	}
    }
}

void SymbolicUnaryOperator::replaceArrayExprWithExpr(
std::vector<SymbolicArraySubscriptExpr*> origExpr, 
std::vector<SymbolicExpr*> subExpr, bool &rv) {
    rv = true;
    opExpr->replaceArrayExprWithExpr(origExpr, subExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replaceArrayExprWithExpr in opExpr\n";
    }
}

void SymbolicBinaryOperator::replaceArrayExprWithExpr(
std::vector<SymbolicArraySubscriptExpr*> origExpr, 
std::vector<SymbolicExpr*> subExpr, bool &rv) {
    rv = true;
    lhs->replaceArrayExprWithExpr(origExpr, subExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replaceArrayExprWithExpr in lhs\n";
	return;
    }
    rhs->replaceArrayExprWithExpr(origExpr, subExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replaceArrayExprWithExpr in rhs\n";
	return;
    }
}

void SymbolicConditionalOperator::replaceArrayExprWithExpr(
std::vector<SymbolicArraySubscriptExpr*> origExpr, 
std::vector<SymbolicExpr*> subExpr, bool &rv) {
    rv = true;
    condition->replaceArrayExprWithExpr(origExpr, subExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replaceArrayExprWithExpr in condition\n";
	return;
    }
    trueExpr->replaceArrayExprWithExpr(origExpr, subExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replaceArrayExprWithExpr in trueExpr\n";
	return;
    }
    falseExpr->replaceArrayExprWithExpr(origExpr, subExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replaceArrayExprWithExpr in falseExpr\n";
	return;
    }
}

void SymbolicSpecialExpr::replaceArrayExprWithExpr(
std::vector<SymbolicArraySubscriptExpr*> origExpr, 
std::vector<SymbolicExpr*> subExpr, bool &rv) {
    rv = true;
    initExpr->replaceArrayExprWithExpr(origExpr, subExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replaceArrayExprWithExpr in initExpr\n";
	return;
    }

    for (std::vector<SymbolicExpr*>::iterator iIt =
	    indexExprsAtAssignLine.begin(); iIt != 
	    indexExprsAtAssignLine.end(); iIt++) {
	(*iIt)->replaceArrayExprWithExpr(origExpr, subExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replaceArrayExprWithExpr in indexExpr\n";
	    return;
	}
    }

    for (std::vector<std::pair<RANGE, bool> >::iterator iIt =
	    indexRangesAtAssignLine.begin(); iIt !=
	    indexRangesAtAssignLine.end(); iIt++) {
	iIt->first.first->replaceArrayExprWithExpr(origExpr, subExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replaceArrayExprWithExpr in range\n";
	    return;
	}
	iIt->first.second->replaceArrayExprWithExpr(origExpr, subExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replaceArrayExprWithExpr in range\n";
	    return;
	}
    }
}

// replaceVars/ExprsWithExprs() of all classes
void SymbolicStmt::replaceVarsWithExprs(std::vector<VarDetails> origVars, 
std::vector<SymbolicExpr*> substituteExprs, bool &rv) {
    rv = true;

    // Replace each of the guardExprs
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicStmt::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceVarsWithExprs(origVars, substituteExprs, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    return;
}

void SymbolicStmt::replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr, 
SymbolicExpr* substituteExpr, bool &rv) {
    rv = true;
    // Replace each of the guardExprs
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicStmt::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    return;
}

void SymbolicDeclStmt::replaceVarsWithExprs(std::vector<VarDetails> origVars,
std::vector<SymbolicExpr*> substituteExprs, bool &rv) {
    rv = true;
    // Replace each of the guardExprs

    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicDeclStmt::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceVarsWithExprs(origVars, substituteExprs, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (!var) {
	llvm::outs() << "ERROR: NULL SymbolicDeclStmt::var\n";
	rv = false;
	return;
    }
    var->replaceVarsWithExprs(origVars, substituteExprs, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in "
		     << "SymbolicDeclStmt::var\n";
	return;
    }
}

void SymbolicDeclStmt::replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr,
SymbolicExpr* substituteExpr, bool &rv) {
    rv = true;
    // Replace each of the guardExprs

    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicDeclStmt::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing exprs in guardExpr\n";
		return;
	    }
	}
    }

    if (!var) {
	llvm::outs() << "ERROR: NULL SymbolicDeclStmt::var\n";
	rv = false;
	return;
    }
    var->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing exprs in "
		     << "SymbolicDeclStmt::var\n";
	return;
    }
}

void SymbolicVarDecl::replaceVarsWithExprs(std::vector<VarDetails> origVars, 
std::vector<SymbolicExpr*> substituteExprs, bool &rv) {
    rv = true;
    // Replace each of the guardExprs

    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicVarDecl::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceVarsWithExprs(origVars, substituteExprs, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }


    if (hasInit()) {
	if (!initExpr) {
	    llvm::outs() << "ERROR: NULL initExpr\n";
	    rv = false;
	    return;
	}
	initExpr->replaceVarsWithExprs(origVars, substituteExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in "
			 << "SymbolicVarDecl::initExpr\n";
	    return;
	}
    }
}

void SymbolicVarDecl::replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr, 
SymbolicExpr* substituteExpr, bool &rv) {
    rv = true;
    // Replace each of the guardExprs

    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicVarDecl::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing exprs in guardExpr\n";
		return;
	    }
	}
    }


    if (hasInit()) {
	if (!initExpr) {
	    llvm::outs() << "ERROR: NULL initExpr\n";
	    rv = false;
	    return;
	}
	initExpr->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in "
			 << "SymbolicVarDecl::initExpr\n";
	    return;
	}
    }
}

void SymbolicReturnStmt::replaceVarsWithExprs(std::vector<VarDetails> origVars,
std::vector<SymbolicExpr*> substituteExprs, bool &rv) {
    rv = true;
    // Replace each of the guardExprs

    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicReturnStmt::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceVarsWithExprs(origVars, substituteExprs, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (retExpr) {
	retExpr->replaceVarsWithExprs(origVars, substituteExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in "
			 << "SymbolicReturnStmt::retExpr\n";
	    return;
	}
    }
}

void SymbolicReturnStmt::replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr,
SymbolicExpr* substituteExpr, bool &rv) {
    rv = true;
    // Replace each of the guardExprs

    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicReturnStmt::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (retExpr) {
	retExpr->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in "
			 << "SymbolicReturnStmt::retExpr\n";
	    return;
	}
    }
}

void SymbolicCallExpr::replaceVarsWithExprs(std::vector<VarDetails> origVars,
std::vector<SymbolicExpr*> substituteExprs, bool &rv) {
    rv = true;
    // Replace each of the guardExprs

    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicCallExpr::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceVarsWithExprs(origVars, substituteExprs, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (!callee) {
	llvm::outs() << "ERROR: NULL SymbolicCallExpr::callee\n";
	rv = false;
	return;
    }
    callee->replaceVarsWithExprs(origVars, substituteExprs, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in "
		     << "SymbolicCallExpr::callee\n";
	return;
    }
    for (std::vector<SymbolicExpr*>::iterator aIt = callArgExprs.begin();
	    aIt != callArgExprs.end(); aIt++) {
	if (!(*aIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicCallExpr::callArgExpr\n";
	    rv = false;
	    return;
	}
	(*aIt)->replaceVarsWithExprs(origVars, substituteExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in "
			 << "SymbolicCallExpr::callArgExpr\n";
	    return;
	}
    }
#if 0
    for (std::vector<SymbolicExpr*>::iterator rIt = returnExprs.begin();
	    rIt != returnExprs.end(); rIt++) {
	if (!(*rIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicCallExpr::returnExpr\n";
	    rv = false;
	    return;
	}
	(*rIt)->replaceVarsWithExprs(origVars, substituteExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in "
			 << "SymbolicCallExpr::returnExpr\n";
	    return;
	}
    }
#endif
    if (returnExpr) {
	returnExpr->replaceVarsWithExprs(origVars, substituteExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in "
			 << "SymbolicCallExpr::returnExpr\n";
	}
    }
}

void SymbolicCallExpr::replaceGlobalVarExprWithExpr(SymbolicDeclRefExpr* origExpr,
SymbolicExpr* substituteExpr, bool &rv) {
    rv = true;
    // Replace each of the guardExprs

    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicCallExpr::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (!callee) {
	llvm::outs() << "ERROR: NULL SymbolicCallExpr::callee\n";
	rv = false;
	return;
    }
    callee->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in "
		     << "SymbolicCallExpr::callee\n";
	return;
    }
    for (std::vector<SymbolicExpr*>::iterator aIt = callArgExprs.begin();
	    aIt != callArgExprs.end(); aIt++) {
	if (!(*aIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicCallExpr::callArgExpr\n";
	    rv = false;
	    return;
	}
	(*aIt)->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in "
			 << "SymbolicCallExpr::callArgExpr\n";
	    return;
	}
    }
    if (returnExpr) {
	returnExpr->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in "
			 << "SymbolicCallExpr::returnExpr\n";
	}
    }
}
void SymbolicCXXOperatorCallExpr::replaceVarsWithExprs(
std::vector<VarDetails> origVars, std::vector<SymbolicExpr*> substituteExprs, 
bool &rv) {
    rv = true;
    // Replace each of the guardExprs

    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicCXXOperatorCallExpr::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceVarsWithExprs(origVars, substituteExprs, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    for (std::vector<SymbolicExpr*>::iterator aIt = callArgExprs.begin();
	    aIt != callArgExprs.end(); aIt++) {
	if (!(*aIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicCallExpr::callArgExpr\n";
	    rv = false;
	    return;
	}
	(*aIt)->replaceVarsWithExprs(origVars, substituteExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in "
			 << "SymbolicCallExpr::callArgExpr\n";
	    return;
	}
    }
}

void SymbolicCXXOperatorCallExpr::replaceGlobalVarExprWithExpr(
SymbolicDeclRefExpr* origExpr, SymbolicExpr* substituteExpr, 
bool &rv) {
    rv = true;
    // Replace each of the guardExprs

    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicCXXOperatorCallExpr::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    for (std::vector<SymbolicExpr*>::iterator aIt = callArgExprs.begin();
	    aIt != callArgExprs.end(); aIt++) {
	if (!(*aIt)) {
	    llvm::outs() << "ERROR: NULL SymbolicCallExpr::callArgExpr\n";
	    rv = false;
	    return;
	}
	(*aIt)->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in "
			 << "SymbolicCallExpr::callArgExpr\n";
	    return;
	}
    }
}

void SymbolicInitListExpr::replaceVarsWithExprs(
std::vector<VarDetails> origVars, std::vector<SymbolicExpr*> substituteExprs, 
bool &rv) {
    rv = true;
    // Replace each of the guardExprs

    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicCXXOperatorCallExpr::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceVarsWithExprs(origVars, substituteExprs, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (arrayFiller) {
	arrayFiller->replaceVarsWithExprs(origVars, substituteExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in arrayFiller\n";
	    return;
	}
    }

    for (std::vector<SymbolicExpr*>::iterator iIt = inits.begin(); iIt != 
	    inits.end(); iIt++) {
	if (*iIt) {
	    (*iIt)->replaceVarsWithExprs(origVars, substituteExprs, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in init\n";
		return;
	    }
	}
    }
}

void SymbolicInitListExpr::replaceGlobalVarExprWithExpr(
SymbolicDeclRefExpr* origExpr, SymbolicExpr* substituteExpr, 
bool &rv) {
    rv = true;
    // Replace each of the guardExprs

    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicCXXOperatorCallExpr::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (arrayFiller) {
	arrayFiller->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in arrayFiller\n";
	    return;
	}
    }

    for (std::vector<SymbolicExpr*>::iterator iIt = inits.begin(); iIt != 
	    inits.end(); iIt++) {
	if (*iIt) {
	    (*iIt)->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in init\n";
		return;
	    }
	}
    }
}

void SymbolicUnaryExprOrTypeTraitExpr::replaceVarsWithExprs(std::vector<VarDetails> origVars,
std::vector<SymbolicExpr*> substituteExprs, bool &rv) {
    rv = true;
    // Replace each of the guardExprs

    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
#ifdef DEBUG
	llvm::outs() << "Replacing guardExprs\n";
#endif
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicUnaryExprOrTypeTraitExpr::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceVarsWithExprs(origVars, substituteExprs, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (!array) {
	llvm::outs() << "ERROR: NULL array in UETT\n";
	rv = false;
	return;
    }
    array->replaceVarsWithExprs(origVars, substituteExprs, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in array of UETT\n";
	return;
    }

    for (std::vector<SymbolicExpr*>::iterator sIt = sizeExprs.begin(); sIt != 
	    sizeExprs.end(); sIt++) {
	if (!(*sIt)) {
	    llvm::outs() << "ERROR: NULL sizeExpr in UETT\n";
	    rv = false;
	    return;
	}
	(*sIt)->replaceVarsWithExprs(origVars, substituteExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in sizeExpr of UETT\n";
	    return;
	}
    }
}


void SymbolicUnaryExprOrTypeTraitExpr::replaceGlobalVarExprWithExpr(
SymbolicDeclRefExpr* origExpr, SymbolicExpr* substituteExpr, bool &rv) {
    rv = true;
    // Replace each of the guardExprs

    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
#ifdef DEBUG
	llvm::outs() << "Replacing guardExprs\n";
#endif
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicUnaryExprOrTypeTraitExpr::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (!array) {
	llvm::outs() << "ERROR: NULL array in UETT\n";
	rv = false;
	return;
    }
    array->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in array of UETT\n";
	return;
    }

    for (std::vector<SymbolicExpr*>::iterator sIt = sizeExprs.begin(); sIt != 
	    sizeExprs.end(); sIt++) {
	if (!(*sIt)) {
	    llvm::outs() << "ERROR: NULL sizeExpr in UETT\n";
	    rv = false;
	    return;
	}
	(*sIt)->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in sizeExpr of UETT\n";
	    return;
	}
    }
}

void SymbolicDeclRefExpr::replaceVarsWithExprs(std::vector<VarDetails> origVars,
std::vector<SymbolicExpr*> substituteExprs, bool &rv) {
    rv = true;
#ifdef DEBUG
    llvm::outs() << "DEBUG: Calling replaceVars on ";
    prettyPrint(true);
    llvm::outs() << "\n";
#endif
    // Replace each of the guardExprs

    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
#ifdef DEBUG
	llvm::outs() << "Replacing guardExprs\n";
#endif
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicDeclRefExpr::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceVarsWithExprs(origVars, substituteExprs, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }


    // If there is a non-null substitute expr, it means that it is part of
    // another function. So we replace the substitute expr instead.
    if (substituteExpr) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Replacing substituteExpr:\n";
	substituteExpr->prettyPrint(true);
	llvm::outs() << "\n";
#endif
	substituteExpr->replaceVarsWithExprs(origVars, substituteExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in "
			 << "SymbolicDeclRefExpr::substituteExpr\n";
	    return;
	}
	// We return here since, the DeclRefExpr is no longer relevant (once
	// there is a substitute expr already)
#ifdef DEBUG
	llvm::outs() << "DEBUG: Replaced substituteExpr\n";
#endif
	return;
    }

    if (vKind == VarKind::FUNC) return;
    //if (vKind == VarKind::LOOPINDEX) return;
    if (vKind == VarKind::SPECIALSCALAR) return;
    if (vKind == VarKind::DEFVAR) return;
    if (vKind == VarKind::OTHER) {
	llvm::outs() << "ERROR: Trying to replace SymbolicDeclRefExpr of kind "
		     << "OTHER\n";
	rv = false;
	return;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Before iterating through origVars: currVar: ";
    var.print();
    llvm::outs() << "\norigVars: " << origVars.size() << "\n";
#endif
    for (unsigned i = 0; i < origVars.size(); i++) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Looking at origVar: ";
	origVars[i].print();
	llvm::outs() << "\n";
#endif
	if (origVars[i].equals(var)) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: origVar equal to currVar\n";
	    llvm::outs() << "origVar: ";
	    origVars[i].print();
	    llvm::outs() << "\nvar: ";
	    var.print();
	    llvm::outs() << "\n";
#endif
	    // TODO: Replace only if the var and substituteExpr are
	    // type-compatible
	    // If the var represented by SymbolicDeclRefExpr is an array, check
	    // that the expression being substituted is also an array.
	    if (var.arraySizeInfo.size() != 0) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Var is array\n";
#endif
		if (!isa<SymbolicDeclRefExpr>(substituteExprs[i]) && 
		    !isa<SymbolicArraySubscriptExpr>(substituteExprs[i])) { 
		    llvm::outs() << "ERROR: Trying to replace array with an "
				 << "expression that is not SymbolicDeclRefExpr "
				 << "or SymbolicArraySubscriptExpr\n";
		    rv = false;
		    return;
		}
		if (isa<SymbolicDeclRefExpr>(substituteExprs[i])) {
		    SymbolicDeclRefExpr* sdre =
			dyn_cast<SymbolicDeclRefExpr>(substituteExprs[i]);
		    if (var.arraySizeInfo.size() !=
			    sdre->var.arraySizeInfo.size()) {
			llvm::outs() << "ERROR: Array size info of var and "
				     << "substituteExpr do not match\n";
			rv = false;
			return;
		    }
		    if (var.type != sdre->var.type) {
			llvm::outs() << "ERROR: Type mismatch between var and "
				     << "substituteExpr\n";
			rv = false;
			return;
		    }
		} else {
		    // it is SymbolicArraySubscriptExpr. Catches the case:
		    // f(arr[i]) where f(x) x is declared to be 1D array and
		    // arr is declared to be 2D array
		    SymbolicArraySubscriptExpr* sase = 
			dyn_cast<SymbolicArraySubscriptExpr>(substituteExprs[i]);
		    if (var.arraySizeInfo.size() != (sase->indexExprs.size() -
1)) {
			llvm::outs() << "ERROR: Array size info of var and "
				     << "substituteExpr do not match\n";
			rv = false;
			return;
		    }
		    if (var.type != sase->baseArray->var.type) {
			llvm::outs() << "ERROR: Type mismatch between var and "
				     << "substituteExpr\n";
			rv = false;
			return;
		    }
		}
	    } else {
		// Var is not an array. Then it can be replaced by any expr
#ifdef DEBUG
		llvm::outs() << "DEBUG: Var is not array\n";
#endif
	    }
	    SymbolicStmt* cloneSubExpr = substituteExprs[i]->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While cloning substituteExpr\n";
		rv = false;
		return;
	    }
	    if (!isa<SymbolicExpr>(cloneSubExpr)) {
		llvm::outs() << "ERROR: Clone of substituteExpr is not "
			     << "SymbolicExpr\n";
		rv = false;
		return;
	    }
	    substituteExpr = dyn_cast<SymbolicExpr>(cloneSubExpr);
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Populated substituteExpr\n";
	    substituteExpr->prettyPrint(true);
	    llvm::outs() << "\n";
#endif
	    // Populate varsReferenced. Remove the ref corresponding to current
	    // SymbolicDeclRefExpr
	    std::vector<SymbolicDeclRefExpr*>::iterator vrIt;
	    bool foundVR = false;
	    for (vrIt = varsReferenced.begin(); vrIt != varsReferenced.end();
		    vrIt++) {
		if (this->equals(*vrIt)) {
		    foundVR = true;
		    break;
		}
	    }
	    if (foundVR) varsReferenced.erase(vrIt);
	    else {
		//llvm::outs() << "ERROR: Cannot find current SymbolicDeclRefExpr "
			     //<< "in the varsReferenced\n";
		//rv = false;
		//return;
	    }
	    varsReferenced.insert(varsReferenced.end(),
		substituteExpr->varsReferenced.begin(), 
		substituteExpr->varsReferenced.end());
	    return;
	}
    }
}

void SymbolicDeclRefExpr::replaceGlobalVarExprWithExpr(
SymbolicDeclRefExpr* origExpr, SymbolicExpr* subExpr, bool &rv) {
    rv = true;
#ifdef DEBUG
    llvm::outs() << "DEBUG: Calling replaceVars on ";
    prettyPrint(true);
    llvm::outs() << "\n";
#endif
    // Replace each of the guardExprs

    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
#ifdef DEBUG
	llvm::outs() << "Replacing guardExprs\n";
#endif
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicDeclRefExpr::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceGlobalVarExprWithExpr(origExpr, subExpr, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }


    // If there is a non-null substitute expr, it means that it is part of
    // another function. So we replace the substitute expr instead.
    if (substituteExpr) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Replacing substituteExpr:\n";
	substituteExpr->prettyPrint(true);
	llvm::outs() << "\n";
#endif
	substituteExpr->replaceGlobalVarExprWithExpr(origExpr, subExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in "
			 << "SymbolicDeclRefExpr::substituteExpr\n";
	    return;
	}
	// We return here since, the DeclRefExpr is no longer relevant (once
	// there is a substitute expr already)
#ifdef DEBUG
	llvm::outs() << "DEBUG: Replaced substituteExpr\n";
#endif
	return;
    }

    if (vKind == VarKind::OTHER) {
	llvm::outs() << "ERROR: Trying to replace SymbolicDeclRefExpr of kind "
		     << "OTHER\n";
	rv = false;
	return;
    } else if (vKind != VarKind::GLOBALVAR) {
	return;
    }

    if (var.equals(origExpr->var) && 
	    callExprID.compare(origExpr->callExprID) == 0) {
	std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
	appendedGuards = appendGuards(guards, subExpr->guards, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While appending guards\n";
	    return;
	}
	guards.clear();
	guards.insert(guards.end(), appendedGuards.begin(), appendedGuards.end());

	substituteExpr = subExpr;
    }
}

void SymbolicArraySubscriptExpr::replaceVarsWithExprs(
std::vector<VarDetails> origVars, std::vector<SymbolicExpr*> substituteExprs, 
bool &rv) {
    rv = true;
    // Replace each of the guardExprs
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicArraySubscriptExpr::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceVarsWithExprs(origVars, substituteExprs, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (!baseArray) {
	llvm::outs() << "ERROR: NULL baseArray\n";
	rv = false;
	return;
    }

    baseArray->replaceVarsWithExprs(origVars, substituteExprs, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in baseArray\n";
	return;
    }

    for (std::vector<SymbolicExpr*>::iterator iIt = indexExprs.begin(); iIt != 
	    indexExprs.end(); iIt++) {
	if (!(*iIt)) {
	    llvm::outs() << "ERROR: NULL indexExpr\n";
	    rv = false;
	    return;
	}
	(*iIt)->replaceVarsWithExprs(origVars, substituteExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in indexExpr\n";
	    return;
	}
    }
}

void SymbolicArraySubscriptExpr::replaceGlobalVarExprWithExpr(
SymbolicDeclRefExpr* origExpr, SymbolicExpr* substituteExpr, 
bool &rv) {
    rv = true;
    // Replace each of the guardExprs
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicArraySubscriptExpr::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (!baseArray) {
	llvm::outs() << "ERROR: NULL baseArray\n";
	rv = false;
	return;
    }

    baseArray->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in baseArray\n";
	return;
    }

    for (std::vector<SymbolicExpr*>::iterator iIt = indexExprs.begin(); iIt != 
	    indexExprs.end(); iIt++) {
	if (!(*iIt)) {
	    llvm::outs() << "ERROR: NULL indexExpr\n";
	    rv = false;
	    return;
	}
	(*iIt)->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in indexExpr\n";
	    return;
	}
    }
}

void SymbolicArrayRangeExpr::replaceVarsWithExprs(
std::vector<VarDetails> origVars, std::vector<SymbolicExpr*> substituteExprs, 
bool &rv) {
    rv = true;
    // Replace each of the guardExprs
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicArrayRangeExpr::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceVarsWithExprs(origVars, substituteExprs, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (!baseArray) {
	llvm::outs() << "ERROR: NULL baseArray\n";
	rv = false;
	return;
    }
    baseArray->replaceVarsWithExprs(origVars, substituteExprs, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in baseArray\n";
	return;
    }

    for (std::vector<std::pair<SymbolicExpr*, SymbolicExpr*> >::iterator iIt = 
	    indexRangeExprs.begin(); iIt != indexRangeExprs.end(); iIt++) {
	if (!(iIt->first)) {
	    llvm::outs() << "ERROR: NULL indexExpr\n";
	    rv = false;
	    return;
	}
	iIt->first->replaceVarsWithExprs(origVars, substituteExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in indexExpr\n";
	    return;
	}
	iIt->second->replaceVarsWithExprs(origVars, substituteExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in indexExpr\n";
	    return;
	}
    }
}

void SymbolicArrayRangeExpr::replaceGlobalVarExprWithExpr(
SymbolicDeclRefExpr* origExpr, SymbolicExpr* substituteExpr, 
bool &rv) {
    rv = true;
    // Replace each of the guardExprs
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicArrayRangeExpr::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (!baseArray) {
	llvm::outs() << "ERROR: NULL baseArray\n";
	rv = false;
	return;
    }
    baseArray->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in baseArray\n";
	return;
    }

    for (std::vector<std::pair<SymbolicExpr*, SymbolicExpr*> >::iterator iIt = 
	    indexRangeExprs.begin(); iIt != indexRangeExprs.end(); iIt++) {
	if (!(iIt->first)) {
	    llvm::outs() << "ERROR: NULL indexExpr\n";
	    rv = false;
	    return;
	}
	iIt->first->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in indexExpr\n";
	    return;
	}
	iIt->second->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in indexExpr\n";
	    return;
	}
    }
}

void SymbolicUnaryOperator::replaceVarsWithExprs(
std::vector<VarDetails> origVars, std::vector<SymbolicExpr*> substituteExprs, 
bool &rv) {
    rv = true;
    // Replace each of the guardExprs
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicUnaryOperator::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceVarsWithExprs(origVars, substituteExprs, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (!opExpr) {
	llvm::outs() << "ERROR: NULL opExpr\n";
	rv = false;
	return;
    }
    opExpr->replaceVarsWithExprs(origVars, substituteExprs, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in opExpr\n";
	return;
    }
}

void SymbolicUnaryOperator::replaceGlobalVarExprWithExpr(
SymbolicDeclRefExpr* origExpr, SymbolicExpr* substituteExpr, 
bool &rv) {
    rv = true;
    // Replace each of the guardExprs
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicUnaryOperator::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (!opExpr) {
	llvm::outs() << "ERROR: NULL opExpr\n";
	rv = false;
	return;
    }
    opExpr->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in opExpr\n";
	return;
    }
}

void SymbolicBinaryOperator::replaceVarsWithExprs(
std::vector<VarDetails> origVars, std::vector<SymbolicExpr*> substituteExprs, 
bool &rv) {
    rv = true;
    // Replace each of the guardExprs
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicBinaryOperator::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceVarsWithExprs(origVars, substituteExprs, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (!lhs) {
	llvm::outs() << "ERROR: NULL lhs\n";
	rv = false;
	return;
    }
    lhs->replaceVarsWithExprs(origVars, substituteExprs, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in lhs\n";
	return;
    }

    if (!rhs) {
	llvm::outs() << "ERROR: NULL rhs\n";
	rv = false;
	return;
    }
    rhs->replaceVarsWithExprs(origVars, substituteExprs, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in rhs\n";
	return;
    }
}

void SymbolicBinaryOperator::replaceGlobalVarExprWithExpr(
SymbolicDeclRefExpr* origExpr, SymbolicExpr* substituteExpr, 
bool &rv) {
    rv = true;
    // Replace each of the guardExprs
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicBinaryOperator::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (!lhs) {
	llvm::outs() << "ERROR: NULL lhs\n";
	rv = false;
	return;
    }
    lhs->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in lhs\n";
	return;
    }

    if (!rhs) {
	llvm::outs() << "ERROR: NULL rhs\n";
	rv = false;
	return;
    }
    rhs->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in rhs\n";
	return;
    }
}

void SymbolicConditionalOperator::replaceVarsWithExprs(
std::vector<VarDetails> origVars, std::vector<SymbolicExpr*> substituteExprs, 
bool &rv) {
    rv = true;
    // Replace each of the guardExprs
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicConditionalOperator::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceVarsWithExprs(origVars, substituteExprs, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (!condition) {
	llvm::outs() << "ERROR: NULL condition\n";
	rv = false;
	return;
    }
    condition->replaceVarsWithExprs(origVars, substituteExprs, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in condition\n";
	return;
    }

    if (!trueExpr) {
	llvm::outs() << "ERROR: NULL trueExpr\n";
	rv = false;
	return;
    }
    trueExpr->replaceVarsWithExprs(origVars, substituteExprs, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in trueExpr\n";
	return;
    }

    if (!falseExpr) {
	llvm::outs() << "ERROR: NULL falseExpr\n";
	rv = false;
	return;
    }
    falseExpr->replaceVarsWithExprs(origVars, substituteExprs, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in falseExpr\n";
	return;
    }
}

void SymbolicConditionalOperator::replaceGlobalVarExprWithExpr(
SymbolicDeclRefExpr* origExpr, SymbolicExpr* substituteExpr, 
bool &rv) {
    rv = true;
    // Replace each of the guardExprs
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicConditionalOperator::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (!condition) {
	llvm::outs() << "ERROR: NULL condition\n";
	rv = false;
	return;
    }
    condition->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in condition\n";
	return;
    }

    if (!trueExpr) {
	llvm::outs() << "ERROR: NULL trueExpr\n";
	rv = false;
	return;
    }
    trueExpr->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in trueExpr\n";
	return;
    }

    if (!falseExpr) {
	llvm::outs() << "ERROR: NULL falseExpr\n";
	rv = false;
	return;
    }
    falseExpr->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in falseExpr\n";
	return;
    }
}

void SymbolicSpecialExpr::replaceVarsWithExprs(
std::vector<VarDetails> origVars, std::vector<SymbolicExpr*> substituteExprs, 
bool &rv) {
    rv = true;
    // Replace each of the guardExprs
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicSpecialExpr::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceVarsWithExprs(origVars, substituteExprs, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (!arrayVar) {
	llvm::outs() << "ERROR: NULL arrayVar\n";
	rv = false;
	return;
    }
    arrayVar->replaceVarsWithExprs(origVars, substituteExprs, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in arrayVar\n";
	return;
    }

    if (!initExpr) {
	llvm::outs() << "ERROR: NULL initExpr\n";
	rv = false;
	return;
    }
    initExpr->replaceVarsWithExprs(origVars, substituteExprs, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in initExpr\n";
	return;
    }

    for (std::vector<SymbolicExpr*>::iterator iIt =
	    indexExprsAtAssignLine.begin(); iIt != 
	    indexExprsAtAssignLine.end(); iIt++) {
	if (!(*iIt)) {
	    llvm::outs() << "ERROR: NULL indexExprAtAssignLine\n";
	    rv = false;
	    return;
	}
	(*iIt)->replaceVarsWithExprs(origVars, substituteExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in indexExpr\n";
	    return;
	}
    }

    for (std::vector<std::pair<RANGE, bool> >::iterator iIt = 
	    indexRangesAtAssignLine.begin(); iIt !=
	    indexRangesAtAssignLine.end(); iIt++) {
	if (!(iIt->first.first)) {
	    llvm::outs() << "ERROR: NULL indexRangeAtAssignLine\n";
	    rv = false;
	    return;
	}
	iIt->first.first->replaceVarsWithExprs(origVars, substituteExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in indexRangeExpr1\n";
	    return;
	}
	if (!(iIt->first.second)) {
	    llvm::outs() << "ERROR: NULL indexRangeAtAssignLine\n";
	    rv = false;
	    return;
	}
	iIt->first.second->replaceVarsWithExprs(origVars, substituteExprs, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in indexRangeExpr2\n";
	    return;
	}
    }
}

void SymbolicSpecialExpr::replaceGlobalVarExprWithExpr(
SymbolicDeclRefExpr* origExpr, SymbolicExpr* substituteExpr, 
bool &rv) {
    rv = true;
    // Replace each of the guardExprs
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    vIt = guards.begin(); vIt != guards.end(); vIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt = 
		vIt->begin(); gIt != vIt->end(); gIt++) {
	    if (!(gIt->first)) {
		llvm::outs() << "ERROR: NULL SymbolicSpecialExpr::guard\n";
		rv = false;
		return;
	    }
	    gIt->first->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in guardExpr\n";
		return;
	    }
	}
    }

    if (!arrayVar) {
	llvm::outs() << "ERROR: NULL arrayVar\n";
	rv = false;
	return;
    }
    arrayVar->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in arrayVar\n";
	return;
    }

    if (!initExpr) {
	llvm::outs() << "ERROR: NULL initExpr\n";
	rv = false;
	return;
    }
    initExpr->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While replacing vars in initExpr\n";
	return;
    }

    for (std::vector<SymbolicExpr*>::iterator iIt =
	    indexExprsAtAssignLine.begin(); iIt != 
	    indexExprsAtAssignLine.end(); iIt++) {
	if (!(*iIt)) {
	    llvm::outs() << "ERROR: NULL indexExprAtAssignLine\n";
	    rv = false;
	    return;
	}
	(*iIt)->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in indexExpr\n";
	    return;
	}
    }

    for (std::vector<std::pair<RANGE, bool> >::iterator iIt = 
	    indexRangesAtAssignLine.begin(); iIt !=
	    indexRangesAtAssignLine.end(); iIt++) {
	if (!(iIt->first.first)) {
	    llvm::outs() << "ERROR: NULL indexRangeAtAssignLine\n";
	    rv = false;
	    return;
	}
	iIt->first.first->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in indexRangeExpr1\n";
	    return;
	}
	if (!(iIt->first.second)) {
	    llvm::outs() << "ERROR: NULL indexRangeAtAssignLine\n";
	    rv = false;
	    return;
	}
	iIt->first.second->replaceGlobalVarExprWithExpr(origExpr, substituteExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While replacing vars in indexRangeExpr2\n";
	    return;
	}
    }
}
bool SymbolicStmt::isConstantLiteral() {
    if (isa<SymbolicIntegerLiteral>(this) ||
	isa<SymbolicImaginaryLiteral>(this) ||
	isa<SymbolicFloatingLiteral>(this) ||
	isa<SymbolicCXXBoolLiteralExpr>(this) || 
	isa<SymbolicStringLiteral>(this) || 
	isa<SymbolicCharacterLiteral>(this))
	return true;
    else if (isa<SymbolicUnaryOperator>(this)) {
	SymbolicUnaryOperator* suo = dyn_cast<SymbolicUnaryOperator>(this);
	if (suo->opKind == UnaryOperatorKind::UO_Minus || 
	    suo->opKind == UnaryOperatorKind::UO_LNot || 
	    suo->opKind == UnaryOperatorKind::UO_Plus) {
	    if (suo->opExpr->isConstantLiteral()) return true;
	    else return false;
	} else {
	    return false;
	}
    } else {
	return false;
    }
}

// print method of all classes

void SymbolicDeclStmt::print() {
    llvm::outs() << "In SymbolicDeclStmt::print()\n";
    printGuards();
    if (var)
	var->print();
}

void SymbolicDeclStmt::prettyPrint(bool g) {
    if (g) {
	printGuards();
	llvm::outs() << "\n";
	prettyPrintGuardExprs();
    }
    llvm::outs() << "Decl Stmt: ";
    if (var)
	var->prettyPrint(false);
#if 0
    if (resolved) llvm::outs() << " resolved";
    else    llvm::outs() << " not resolved";
#endif
    llvm::outs() << "\n";
}

void SymbolicStmt::printGuardExprs() {
    llvm::outs() << "GuardExprs: ";
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt =
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    llvm::outs() << "("; 
	    sIt->first->print();
	    llvm::outs() << ", " 
			 << (sIt->second? "true": "false") << ")";
	    if (sIt+1 != gIt->end()) llvm::outs() << " && ";
	}
	if (gIt+1 != guards.end()) llvm::outs() << "\n||";
	llvm::outs() << "\n";
    }
    llvm::outs() << "\n";
}

void SymbolicStmt::prettyPrintGuardExprs() {
    bool emptyguard = true;
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    gIt = guards.begin(); gIt != guards.end(); gIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    emptyguard = false;
	    if (!(sIt->second)) {
		llvm::outs() << "!";
	    }
	    llvm::outs() << "(";
	    sIt->first->prettyPrint(false);
	    llvm::outs() << ")";
	    if (sIt+1 != gIt->end()) llvm::outs() << " && ";
	}
	if (gIt+1 != guards.end()) llvm::outs() << " || ";
    }
    if (emptyguard) llvm::outs() << "true";
    llvm::outs() << ": ";
}

void prettyPrintTheseGuardExprs(
std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > guards) {
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator gIt =
	    guards.begin(); gIt != guards.end(); gIt++) {
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt =
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    if (!sIt->second) llvm::outs() << "!";
	    llvm::outs() << "(";
	    sIt->first->prettyPrint(false);
	    llvm::outs() << ")";
	    if (sIt+1 != gIt->end()) llvm::outs() << " && ";
	}
	if (gIt+1 != guards.end()) llvm::outs() << " || ";
    }
}

void SymbolicStmt::printVarRefs() {
    llvm::outs() << "Vars Referenced: " << varsReferenced.size() << "\n";
    for (std::vector<SymbolicDeclRefExpr*>::iterator vIt =
	    varsReferenced.begin(); vIt != varsReferenced.end(); vIt++) {
	(*vIt)->print();
    }
}

void SymbolicVarDecl::print() {
    llvm::outs() << "In SymbolicVarDecl::print()\n";
    printGuards();
    varD.print();
    if (hasInit()) {
	llvm::outs() << "InitExpr:\n";
	initExpr->print();
    }
    llvm::outs() << "\n";
    printVarRefs();
}

void SymbolicVarDecl::prettyPrint(bool g) {
    if (g) {
	printGuards();
	llvm::outs() << "\n";
	prettyPrintGuardExprs();
    }
    llvm::outs() << "Var: " << varD.varName;
    for (std::vector<SymbolicExpr*>::iterator it =
	    varD.arraySizeSymExprs.begin(); it != 
	    varD.arraySizeSymExprs.end(); it++) {
	llvm::outs() << "[";
	(*it)->prettyPrint(false);
	llvm::outs() << "]  ";
    }
    if (hasInit()) {
	llvm::outs() << " = ";
	initExpr->prettyPrint(false);
    }
#if 0
    if (resolved) llvm::outs() << " resolved";
    else    llvm::outs() << " not resolved";
#endif
    llvm::outs() << "\n";
}

void SymbolicReturnStmt::print() {
    printGuards();
    if (retExpr)
	retExpr->print();
    else 
	llvm::outs() << "NULL";
    llvm::outs() << "\n";
    printVarRefs();
}

void SymbolicReturnStmt::prettyPrint(bool g) {
    if (g) {
	printGuards();
	llvm::outs() << "\n";
	prettyPrintGuardExprs();
    }
    llvm::outs() << " return ";
    if (retExpr)
	retExpr->prettyPrint(false);
#if 0
    if (resolved) llvm::outs() << " resolved";
    else    llvm::outs() << " not resolved";
#endif
    llvm::outs() << "\n";
}

bool SymbolicStmt::equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicStmt::equals()\n";
#endif
    if (st->getKind() != kind) return false;
    if (st->resolved != resolved) return false;
    if (st->isDummy != isDummy) return false;
    if (st->guardBlocks.size() != guardBlocks.size()) return false;

    for (unsigned i = 0; i < guardBlocks.size(); i++) {
	if (st->guardBlocks[i].size() != guardBlocks[i].size()) return false;
	for (unsigned j = 0; j < guardBlocks[i].size(); j++) {
	    if (st->guardBlocks[i][j].first != guardBlocks[i][j].first)
		return false;
	    if (st->guardBlocks[i][j].second != guardBlocks[i][j].second)
		return false;
	}
    }

    if (st->guards.size() != guards.size()) return false;
    for (unsigned i = 0; i < guards.size(); i++) {
	if (st->guards[i].size() != guards[i].size()) return false;
	for (unsigned j = 0; j < guards[i].size(); j++) {
	    if (!(st->guards[i][j].first) && guards[i][j].first) return false;
	    if (st->guards[i][j].first && !(guards[i][j].first)) return false;
	    if (st->guards[i][j].first && guards[i][j].first) {
		if (!(st->guards[i][j].first->equals(guards[i][j].first)))
		    return false;
	    }
	    if (st->guards[i][j].second != guards[i][j].second) return false;
	}
    }

#if 0
    if (st->varsReferenced.size() != varsReferenced.size()) return false;
    for (std::vector<SymbolicDeclRefExpr*>::iterator vIt = 
	    st->varsReferenced.begin(); vIt != st->varsReferenced.end(); vIt++) {
	if (!(*vIt)) {
	    llvm::outs() << "ERROR: Null VarRef in SymbolicStmt:\n";
	    st->print();
	    continue;
	}
	bool foundVarRef = false;
	for (std::vector<SymbolicDeclRefExpr*>::iterator it = 
		varsReferenced.begin(); it != varsReferenced.end(); it++) {
	    if (!(*it)) {
		llvm::outs() << "ERROR: Null VarRef in SymbolicStmt:\n";
		this->print();
		continue;
	    }
	    if ((*vIt)->equals(*it)) {
		foundVarRef = true;
		break;
	    }
	}
	if (!foundVarRef) return false;
    }
#endif
    
    return true;
}

bool SymbolicDeclStmt::equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicDeclStmt::equals()\n";
#endif
    //if (!(SymbolicStmt::equals(st))) return false;
    if (!isa<SymbolicDeclStmt>(st)) return false;
    SymbolicDeclStmt* sdst = dyn_cast<SymbolicDeclStmt>(st);
    if (!var && sdst->var) return false;
    if (var && !(sdst->var)) return false;
    if (var && sdst->var && !(var->equals(sdst->var))) return false;
    return true;
}

bool SymbolicVarDecl::equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicVarDecl::equals()\n";
#endif
    //if (!(SymbolicStmt::equals(st))) return false;
    if (!isa<SymbolicDeclStmt>(st)) return false;
    if (!(SymbolicDeclStmt::equals(st))) return false;
    if (!isa<SymbolicVarDecl>(st)) return false;
    SymbolicVarDecl* svd = dyn_cast<SymbolicVarDecl>(st);
    if (!(varD.equals(svd->varD))) return false;
    if (initExpr && !(svd->initExpr)) return false;
    if (!initExpr && svd->initExpr) return false;
    if (initExpr && svd->initExpr && !(initExpr->equals(svd->initExpr))) 
	return false;
    return true;
}

bool SymbolicReturnStmt::equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicReturnStmt::equals()\n";
#endif
    //if (!(SymbolicStmt::equals(st))) return false;
    if (!isa<SymbolicReturnStmt>(st)) return false;
    SymbolicReturnStmt* srs = dyn_cast<SymbolicReturnStmt>(st);
    if (retExpr && !(srs->retExpr)) return false;
    if (!retExpr && srs->retExpr) return false;
    if (retExpr && srs->retExpr && !(retExpr->equals(srs->retExpr))) 
	return false;
    return true;
}

bool SymbolicCallExpr::equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicCallExpr::equals()\n";
#endif
    //if (!(SymbolicStmt::equals(st))) return false;
    if (!isa<SymbolicCallExpr>(st)) return false;
    SymbolicCallExpr* sce = dyn_cast<SymbolicCallExpr>(st);
    if (callee && !(sce->callee)) return false;
    if (!callee && sce->callee) return false;
    if (callee && sce->callee && !(callee->equals(sce->callee))) return false;
    if (callArgExprs.size() != sce->callArgExprs.size()) return false;
#if 0
    for (std::vector<SymbolicExpr*>::iterator aIt = callArgExprs.begin();
	    aIt != callArgExprs.end(); aIt++) {
	if (!(*aIt)) {
	    llvm::outs() << "ERROR: NULL arg expression for call expr:\n";
	    print();
	    continue;
	}
	bool foundArgExpr = false;
	for (std::vector<SymbolicExpr*>::iterator it =
		sce->callArgExprs.begin(); it != sce->callArgExprs.end(); it++) {
	    if (!(*it)) {
		llvm::outs() << "ERROR: NULL arg expression for call expr:\n";
		sce->print();
		continue;
	    }
	    if ((*aIt)->equals(*it)) {
		foundArgExpr = true;
		break;
	    }
	}
	if (!foundArgExpr) return false;
    }
#endif
    for (unsigned i = 0; i < callArgExprs.size(); i++) {
	if (!(callArgExprs[i]) || !(sce->callArgExprs[i])) {
	    llvm::outs() << "ERROR: NULL arg expression in callExpr\n";
	    return false;
	}
	if (!(callArgExprs[i]->equals(sce->callArgExprs[i]))) return false;
    }
#if 0
    if (returnExprs.size() != sce->returnExprs.size()) return false;
    for (std::vector<SymbolicExpr*>::iterator aIt = returnExprs.begin();
	    aIt != returnExprs.end(); aIt++) {
	if (!(*aIt)) {
	    llvm::outs() << "ERROR: NULL ret expression for call expr:\n";
	    print();
	    continue;
	}
	bool foundRetExpr = false;
	for (std::vector<SymbolicExpr*>::iterator it =
		sce->returnExprs.begin(); it != sce->returnExprs.end(); it++) {
	    if (!(*it)) {
		llvm::outs() << "ERROR: NULL ret expression for call expr:\n";
		sce->print();
		continue;
	    }
	    if ((*aIt)->equals(*it)) {
		foundRetExpr = true;
		break;
	    }
	}
	if (!foundRetExpr) return false;
    }
#endif
    if (returnExpr && !(sce->returnExpr)) return false;
    if (!returnExpr && sce->returnExpr) return false;
    // If both are NULL, return true
    if (!returnExpr && !(sce->returnExpr)) return true;
    if (!(returnExpr->equals(sce->returnExpr))) return false;

    return true;
}

bool SymbolicCXXOperatorCallExpr::equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicCXXOperatorCallExpr::equals()\n";
#endif
    //if (!SymbolicStmt::equals(st)) return false;
    if (!isa<SymbolicCallExpr>(st)) return false;
    if (!SymbolicCallExpr::equals(st)) return false;
    if (!isa<SymbolicCXXOperatorCallExpr>(st)) return false;
    SymbolicCXXOperatorCallExpr* scce = dyn_cast<SymbolicCXXOperatorCallExpr>(st);
    if (opKind != scce->opKind) return false;
    return true;
}

bool SymbolicInitListExpr::equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicInitListExpr::equals()\n";
#endif
    //if (!SymbolicStmt::equals(st)) return false;
    if (!isa<SymbolicInitListExpr>(st)) return false;
    SymbolicInitListExpr* sile = dyn_cast<SymbolicInitListExpr>(st);
    if (hasArrayFiller != sile->hasArrayFiller) return false;
    if (arrayFiller && !sile->arrayFiller) return false;
    if (!arrayFiller && sile->arrayFiller) return false;
    if (arrayFiller && sile->arrayFiller && !(arrayFiller->equals(sile->arrayFiller))) return false;
    if (inits.size() != sile->inits.size()) return false;
    for (unsigned i = 0; i < inits.size(); i++) {
	if (inits[i] && !sile->inits[i]) return false;
	if (!inits[i] && sile->inits[i]) return false;
	if (inits[i] && sile->inits[i] && !(inits[i]->equals(sile->inits[i]))) return false;
    }
    return true;
}

bool SymbolicUnaryExprOrTypeTraitExpr::equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicUnaryExprOrTypeTraitExpr::equals()\n";
#endif
    if (!isa<SymbolicExpr>(st)) return false;
    if (!SymbolicExpr::equals(st)) return false;
    if (!isa<SymbolicUnaryExprOrTypeTraitExpr>(st)) return false;
    SymbolicUnaryExprOrTypeTraitExpr* suett =
	dyn_cast<SymbolicUnaryExprOrTypeTraitExpr>(st);
    if (suett->kind != kind) return false;
    if (!(suett->array) || !array) return false;
    if (!(suett->array->equals(array))) return false;
    if (suett->sizeExprs.size() != sizeExprs.size()) return false;
    for (unsigned i = 0; i < sizeExprs.size(); i++) {
	if (!(suett->sizeExprs[i]) || !(sizeExprs[i])) return false;
	if (!(suett->sizeExprs[i]->equals(sizeExprs[i]))) return false;
    }
    return true;
}

bool SymbolicDeclRefExpr::equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicDeclRefExpr::equals()\n";
#endif
    //if (!SymbolicStmt::equals(st)) return false;
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Calling equals on substituteExpr\n";
    prettyPrint(false);
    llvm::outs() << "\n";
#endif
    if (substituteExpr) {
	if (!(st->equals(substituteExpr))) return false;
	else return true;
    }
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: After Calling equals on substituteExpr\n";
#endif
    if (!isa<SymbolicDeclRefExpr>(st)) return false;
    SymbolicDeclRefExpr* sdre = dyn_cast<SymbolicDeclRefExpr>(st);
    if (vKind != sdre->vKind) return false;
    if (vKind == VarKind::FUNC) {
	if (!(fd->equals(sdre->fd))) return false;
    } else {
	//if (!substituteExpr && sdre->substituteExpr &&
		//!(sdre->substituteExpr->equals(this))) 
	    //return false;
	if (/*!substituteExpr && !sdre->substituteExpr && */
		!(var.equals(sdre->var))) 
	    return false;
    }
#ifdef DEBUGFULL
    llvm::outs() << "this: ";
    prettyPrint(false);
    llvm::outs() << "\nst: ";
    st->prettyPrint(false);
    llvm::outs() << "\n";
#endif
    return true;
}

bool SymbolicArraySubscriptExpr::equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicArraySubscriptExpr::equals()\n";
#endif
    //if (!SymbolicStmt::equals(st)) return false;
    if (substituteExpr) { 
	if (!(st->equals(substituteExpr))) return false;
	else return true;
    }
    if (!isa<SymbolicArraySubscriptExpr>(st)) return false;
    SymbolicArraySubscriptExpr* sase = dyn_cast<SymbolicArraySubscriptExpr>(st);
    //if (!substituteExpr && sase->substituteExpr &&
	    //!(sase->substituteExpr->equals(this)))
	//return false;
    //if (!substituteExpr && !sase->substituteExpr) {
	if (baseArray && !(sase->baseArray)) return false;
	if (!baseArray && sase->baseArray) return false;
	if (baseArray && sase->baseArray && !(baseArray->equals(sase->baseArray))) 
	    return false;
	if (indexExprs.size() != sase->indexExprs.size()) return false;
	for (unsigned i = 0; i < indexExprs.size(); i++) {
	    if (!(indexExprs[i]) && sase->indexExprs[i]) return false;
	    if (indexExprs[i] && !(sase->indexExprs[i])) return false;
	    if (indexExprs[i] && sase->indexExprs[i] &&
		    !(indexExprs[i]->equals(sase->indexExprs[i]))) return false;
	}
    //}
    return true;
}

bool SymbolicUnaryOperator::equals(SymbolicStmt* st) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicUnaryOperator::equals()\n";
#endif
    //if (!SymbolicStmt::equals(st)) return false;
    if (!isa<SymbolicUnaryOperator>(st)) return false;
    SymbolicUnaryOperator* suo = dyn_cast<SymbolicUnaryOperator>(st);
    if (opKind != suo->opKind) return false;
    if (opExpr && !(suo->opExpr)) return false;
    if (!opExpr && suo->opExpr) return false;
    if (opExpr && suo->opExpr && !(opExpr->equals(suo->opExpr))) return false;
    return true;
}

bool GetSymbolicExprVisitor::VisitDeclRefExpr(DeclRefExpr* E) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering GetSymbolicExprVisitor::VisitDeclRefExpr()\n";
    llvm::outs() << Helper::prettyPrintExpr(E) << "\n";
#endif
    // Check whether this is already resolved
    bool resolved;
    bool rv;
    DeclRefExprDetails* dred = DeclRefExprDetails::getObject(SM, E, currBlock,
	rv);
    if (!rv) {
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: DeclRefExprDetails:\n";
    dred->prettyPrint();
#endif

    //if (isResolved(dred)) 
    resolved = dred->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While DeclRefExprDetails::isSymExprCompletelyResolved()\n";
	error = true;
	return false;
    }
    if (resolved)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: DeclRefExpr " << Helper::prettyPrintExpr(E) 
		     << " is already resolved. Skipping..\n";
	dred->prettyPrint();
	llvm::outs() << "DEBUG: SymExprs:\n";
#if 0
	std::vector<SymbolicStmt*> symExprs = getSymbolicExprs(dred, rv);
	if (rv) {
	    llvm::outs() << "DEBUG: Num of symbolicExprs: " << symExprs.size()
			 << "\n";
	    for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin();
		    sIt != symExprs.end(); sIt++) {
		(*sIt)->print();
	    }
	}
#endif
	printSymExprs(dred);
#endif
	return true;
    }

    // If we obtained a dummy object for DeclRefExpr (may be the expr is for a
    // library function like operator>>)
    if (dred->isDummy) {
	// Then create a dummy symbolic expr and mark it as resolved
	SymbolicDeclRefExpr* sdre = new SymbolicDeclRefExpr;
	sdre->isDummy = true;
	if (dred->rKind == DeclRefKind::FUNC || dred->rKind == DeclRefKind::LIB) {
	    sdre->vKind = SymbolicDeclRefExpr::VarKind::FUNC;
	    sdre->var = dred->var.clone();
	    sdre->fd = dred->fd;
	}
	//sdre->guardBlocks.insert(sdre->guardBlocks.end(), currGuards.begin(), 
	    //currGuards.end());
#ifdef DEBUG
	llvm::outs() << "DEBUG: currGuards:\n";
	SymbolicStmt::prettyPrintGuards(currGuards);
#endif
	std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
	    appendGuards(sdre->guardBlocks, currGuards);
#ifdef DEBUG
	llvm::outs() << "DEBUG: Appended guards:\n";
	SymbolicStmt::prettyPrintGuards(appendedGuards);
#endif
	sdre->guardBlocks.clear();
	sdre->guardBlocks.insert(sdre->guardBlocks.end(),
	    appendedGuards.begin(), appendedGuards.end());
#ifdef DEBUG
	llvm::outs() << "DEBUG: After copying guards:\n";
	sdre->printGuards();
#endif

	//sdre->vKind = SymbolicDeclRefExpr::VarKind::OTHER;
	sdre->resolved = true;
	// Replace any existing entry in symExprMap
	replaceExprInSymMap(dred, sdre);

#ifdef DEBUG
	llvm::outs() << "DEBUG: DeclRefExpr " << Helper::prettyPrintExpr(E)
		     << " is a dummy expr. Inserting dummy symbolic expr\n";
#endif
	return true;
    }

    // If the DeclRefExpr is a function that is user defined, resolve it here
    if (dred->rKind == DeclRefKind::FUNC) {
	SymbolicDeclRefExpr* sdre = new SymbolicDeclRefExpr(dred->fd, true);
	//sdre->guardBlocks.insert(sdre->guardBlocks.end(), currGuards.begin(), 
	    //currGuards.end());
	std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
	    appendGuards(sdre->guardBlocks, currGuards);
	sdre->guardBlocks.clear();
	sdre->guardBlocks.insert(sdre->guardBlocks.end(),
	    appendedGuards.begin(), appendedGuards.end());
	replaceExprInSymMap(dred, sdre);
#ifdef DEBUG
	llvm::outs() << "DEBUG: DeclRefExpr " << Helper::prettyPrintExpr(E)
		     << " is a function. Inserting symExpr\n";
#endif
	return true;
    }

    if (E->isLValue()) {
	// Check if this is a VarDecl
	ValueDecl* d = E->getDecl();
	if (!(isa<DeclaratorDecl>(d))) {
	    // If the DeclRefExpr is not VarDecl, then resolve it (otherwise the
	    // back substitution loop will not terminate)
	    SymbolicDeclRefExpr* symdre = new SymbolicDeclRefExpr;
	    symdre->isDummy = true;
	    symdre->resolved = true;
	    //symdre->guardBlocks.insert(symdre->guardBlocks.end(), 
		//currGuards.begin(), currGuards.end());
	    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
		appendGuards(symdre->guardBlocks, currGuards);
	    symdre->guardBlocks.clear();
	    symdre->guardBlocks.insert(symdre->guardBlocks.end(),
		appendedGuards.begin(), appendedGuards.end());

	    if (isExprPresentInSymMap(dred)) {
		llvm::outs() << "DEBUG: Existing symbolic exprs:\n";
#if 0
		std::vector<SymbolicStmt*> symExprs = getSymbolicExprs(dred, rv);
		std::vector<SymbolicStmt*>::iterator sIt;
		for (sIt = symExprs.begin(); sIt != symExprs.end(); sIt++) {
		    (*sIt)->print();
		}
#endif
		printSymExprs(dred);
		llvm::outs() << "WARNING: Replacing this entry with dummy\n";
	    }

	    replaceExprInSymMap(dred, symdre);

	    return true;
	}
	DeclaratorDecl* dd = dyn_cast<DeclaratorDecl>(d);
	if (!(isa<VarDecl>(dd))) { 
	    // If the DeclRefExpr is not VarDecl, then resolve it (otherwise the
	    // back substitution loop will not terminate)
	    SymbolicDeclRefExpr* symdre = new SymbolicDeclRefExpr;
	    symdre->isDummy = true;
	    symdre->resolved = true;
	    //symdre->guardBlocks.insert(symdre->guardBlocks.end(), currGuards.begin(), 
		//currGuards.end());
	    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
		appendGuards(symdre->guardBlocks, currGuards);
	    symdre->guardBlocks.clear();
	    symdre->guardBlocks.insert(symdre->guardBlocks.end(),
		appendedGuards.begin(), appendedGuards.end());

	    if (isExprPresentInSymMap(dred)) {
		llvm::outs() << "DEBUG: Existing symbolic exprs:\n";
#if 0
		std::vector<SymbolicStmt*> symExprs = getSymbolicExprs(dred, rv);
		std::vector<SymbolicStmt*>::iterator sIt;
		for (sIt = symExprs.begin(); sIt != symExprs.end(); sIt++) {
		    (*sIt)->print();
		}
#endif
		printSymExprs(dred);
		llvm::outs() << "WARNING: Replacing this entry with dummy\n";
	    }

	    replaceExprInSymMap(dred, symdre);

	    return true;
	}
	VarDecl* vd = dyn_cast<VarDecl>(dd);

#ifdef DEBUG
	llvm::outs() << "DEBUG: Decided DeclRefExpr is var\n";
	dred->print();
#endif

	// Check if the VarDecl is in the main file of the program.
	SourceLocation SL = SM->getExpansionLoc(vd->getLocStart());
	if (SM->isInSystemHeader(SL) || !(SM->isWrittenInMainFile(SL))) {
	    llvm::outs() << "ERROR: DeclRefExpr is not in main file\n";
	    error = true;
	    return false;
	}

	const int lineNum = SM->getExpansionLineNumber(E->getLocStart(), 0);

	// If this line is not part of the current basic block under
	// consideration, we do not want to visit it now.
	if (FA->blockToLines.find(currBlock) == FA->blockToLines.end()) {
	    llvm::outs() << "ERROR: Cannot find entry for block " << currBlock 
			 << " in blockToLines\n";
	    error = true;
	    return false;
	}

	std::pair<int, int> lines = FA->blockToLines[currBlock];
	// If current line not within the lines constituting the current block
	if (!(lines.first <= lineNum && lineNum <= lines.second)) {
	    llvm::outs() << "DEBUG: Current line " << lineNum << " is not within block "
			 << currBlock << "\n";
	    llvm::outs() << currBlock << ": " << lines.first << " - " << lines.second << "\n";
	    return true;
	}


#ifdef DEBUG
	llvm::outs() << "Visiting DeclRefExpr: ";
	//currVar->print();
	dred->prettyPrint();
	llvm::outs() << "\n";
#endif

	// First, check whether this var's symexprs are already recorded or not.
	// This needs to be done, because the visitor might be called
	// redundantly on the same statement/expression multiple times. The
	// reason why the visitor is called on the same expression is due to the way
	// CFGBlock is structured. The implementation calls the visitor on each
	// statement or CFGElement in the block and there is redundancy there. We can
	// avoid unnecessary computation if we do this check first.
#if 0
	if (isExprPresentInSymMap(dred)) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Already visited this var. Skipping...\n";
#endif
	    return true;
	}
#endif

	// If the current var is a loop index or array var, we do not have to
	// compute the reaching def. Although we do not have to compute the
	// reaching def, record it in the symExprMap so that we do not have to
	// check this again.
	if (Helper::isVarDeclAnArray(vd)) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: The var is an array. Skipping..\n";
#endif
	    // Record it in the symExprMap as resolved.
	    SymbolicDeclRefExpr* symExpr = new SymbolicDeclRefExpr;
	    symExpr->var = dred->var;
	    symExpr->vKind = SymbolicDeclRefExpr::VarKind::ARRAY;
	    symExpr->resolved = true;
	    //symExpr->guardBlocks.insert(symExpr->guardBlocks.end(), currGuards.begin(), 
		//currGuards.end());
	    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
		appendGuards(symExpr->guardBlocks, currGuards);
	    symExpr->guardBlocks.clear();
	    symExpr->guardBlocks.insert(symExpr->guardBlocks.end(),
		appendedGuards.begin(), appendedGuards.end());
	    SymbolicStmt* cloneVR = symExpr->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While cloning SymbolicDeclRefExpr: "
			     << "ARRAY to insert in varsReferenced\n";
		error = true;
		return false;
	    }
	    if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
		llvm::outs() << "ERROR: Clone of SymbolicDeclRefExpr is not "
			     << "SymbolicDeclRefExpr\n";
		error = true;
		return false;
	    }
	    symExpr->varsReferenced.push_back(
		dyn_cast<SymbolicDeclRefExpr>(cloneVR));
	    if (!insertExprInSymMap(dred, symExpr)) {
		llvm::outs() << "ERROR: While inserting symexpr for:\n";
		dred->print();
		error = true;
		return false;
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Inserted symExpr for DeclRefExpr:\n";
	    symExpr->prettyPrint(true);
#endif
	    return true;
	}

	bool isLoopIndex = FA->isLoopIndexVar(dred->var, currBlock, rv);
	if (!rv) {
	    error = true;
	    return false;
	}
	if (isLoopIndex) {


#ifdef DEBUG
	    llvm::outs() << "DEBUG: The var is a loopIndexVar. Skipping..\n";
#endif

	    // Record it in the symExprMap as resolved.

	    SymbolicDeclRefExpr* symExpr = new SymbolicDeclRefExpr;
	    symExpr->vKind = SymbolicDeclRefExpr::VarKind::LOOPINDEX;
	    symExpr->var = dred->var;
	    symExpr->resolved = true;
	    //symExpr->guardBlocks.insert(symExpr->guardBlocks.end(), 
		//currGuards.begin(), currGuards.end());
	    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
		appendGuards(symExpr->guardBlocks, currGuards);
	    symExpr->guardBlocks.clear();
	    symExpr->guardBlocks.insert(symExpr->guardBlocks.end(),
		appendedGuards.begin(), appendedGuards.end());
	    SymbolicStmt* cloneVR = symExpr->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While cloning SymbolicDeclRefExpr: "
			     << "LOOPINDEX to insert in varsReferenced\n";
		error = true;
		return false;
	    }
	    if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
		llvm::outs() << "ERROR: Clone of SymbolicDeclRefExpr is not "
			     << "SymbolicDeclRefExpr\n";
		error = true;
		return false;
	    }
	    symExpr->varsReferenced.push_back(dyn_cast<SymbolicDeclRefExpr>(cloneVR));
	    if (!insertExprInSymMap(dred, symExpr)) {
		llvm::outs() << "ERROR: While inserting symexpr for:\n";
		dred->prettyPrint();
		error = true;
		return false;
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Inserted symExpr for DeclRefExpr:\n";
	    symExpr->prettyPrint(true);
#endif
	    return true;
	}

	// If the current var is the scalarvar of a special expr and we are
	// looking for its reaching definitions at the guard stmt of the special
	// expr, then stop. If the current statement is after the assignment
	// statement of the special expr, then the later code records the
	// special expr as the symbolic expr. If the current statement is before
	// the assignment statement of the special expr, then the var cannot be
	// considered as the scalar var of special expr (unless the current
	// statement is the guard of the special expr).
	// TODO: The following code only checks if the special expr is a max or
	// a min. We need to implement sum as well.
	for (std::vector<SpecialExpr*>::iterator sIt = FA->spExprs.begin(); 
		sIt != FA->spExprs.end(); sIt++) {
	    if ((*sIt)->scalarVar.equals(dred->var) && 
		((((*sIt)->kind == SpecialExprKind::MAX || 
		    (*sIt)->kind == SpecialExprKind::MIN) && 
		    (*sIt)->guardLine == lineNum) || 
		((*sIt)->kind == SpecialExprKind::SUM && 
		    (*sIt)->assignmentLine == lineNum))) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: The var is part of a special MAX/MIN expr "
			     << "identified and we should not be rewriting this "
			     << "var at line " << lineNum << "\n";
#endif

		// TODO:This statement needs to be ignored during back
		// substitution

		SymbolicDeclRefExpr* symExpr = new SymbolicDeclRefExpr;
		symExpr->vKind = SymbolicDeclRefExpr::VarKind::SPECIALSCALAR;
		symExpr->resolved = true;
		//symExpr->guardBlocks.insert(symExpr->guardBlocks.end(),
		    //currGuards.begin(), currGuards.end());
		std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
		    appendGuards(symExpr->guardBlocks, currGuards);
		symExpr->guardBlocks.clear();
		symExpr->guardBlocks.insert(symExpr->guardBlocks.end(),
		    appendedGuards.begin(), appendedGuards.end());
		SymbolicStmt* cloneVR = symExpr->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While cloning SymbolicDeclRefExpr: "
				 << "SPECIALSCALAR to insert in varsReferenced\n";
		    error = true;
		    return false;
		}
		if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
		    llvm::outs() << "ERROR: Clone of SymbolicDeclRefExpr is not "
				 << "SymbolicDeclRefExpr\n";
		    error = true;
		    return false;
		}
		symExpr->varsReferenced.push_back(dyn_cast<SymbolicDeclRefExpr>(cloneVR));
		if (!insertExprInSymMap(dred, symExpr)) {
		    llvm::outs() << "ERROR: While inserting symexpr for:\n";
		    dred->prettyPrint();
		    error = true;
		    return false;
		}
#ifdef DEBUG
		llvm::outs() << "DEBUG: Inserted symExpr for DeclRefExpr:\n";
		symExpr->prettyPrint(true);
#endif
		return true;
	    }
	}

	FunctionAnalysis::ReachDef rdObj = FA->rd;
	// Check if this var is defined at this line in this block. This
	// is a strong check. The actual check should be that the var is only
	// defined at this line. There can be cases when the same var is both
	// defined and used in the line. For example, x = x/2; x is defined and
	// used in this stmt. In other words, the check should be that the var
	// is not used, but defined in this line.
	bool isVarUsed = false;
	if (rdObj.blockLineToVarsUsedInDef.find(currBlock) !=
		rdObj.blockLineToVarsUsedInDef.end()) {
	    if (rdObj.blockLineToVarsUsedInDef[currBlock].find(lineNum) != 
		    rdObj.blockLineToVarsUsedInDef[currBlock].end()) {
		std::vector<VarDetails> varsUsed =
		    rdObj.blockLineToVarsUsedInDef[currBlock][lineNum];
		for (std::vector<VarDetails>::iterator vIt = varsUsed.begin();
			vIt != varsUsed.end(); vIt++) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: varsused: ";
		    vIt->print();
		    llvm::outs() << "\n";
#endif
		    if (vIt->equals(dred->var)) {
			isVarUsed = true;
			break;
		    }
		}
	    } else {
#ifdef DEBUG
		llvm::outs() << "DEBUG: No entry for line " << lineNum 
			     << " in blockLineToVarsUsedInDef[" << currBlock 
			     << "]\n";
#endif
	    }
	} else {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: No entry for block " << currBlock 
			 << " in blockLineToVarsUsedInDef\n";
#endif
	}
	if (rdObj.blockToVarsDefined.find(currBlock) !=
		rdObj.blockToVarsDefined.end()) {
	    std::vector<std::pair<VarDetails, int> > varsDefined = 
		rdObj.blockToVarsDefined[currBlock];
	    bool isVarDefined = false;
	    for (std::vector<std::pair<VarDetails, int> >::iterator vIt = 
		    varsDefined.begin(); vIt != varsDefined.end(); vIt++) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: varsdefined at line " << vIt->second 
			     << ": ";
		vIt->first.print();
		llvm::outs() << "\n";
#endif
		if (vIt->second != lineNum) continue;
		if (vIt->first.varName.compare(dred->var.varName) == 0 && 
			vIt->first.varDeclLine == dred->var.varDeclLine) {
		    isVarDefined = true;
		    break;
		}
	    }
#ifdef DEBUG
	    if (isVarDefined) llvm::outs() << "Var defined\n";
	    else llvm::outs() << "Var not defined\n";
	    if (isVarUsed) llvm::outs() << "Var used\n";
	    else llvm::outs() << "Var not used\n";
#endif

	    if (isVarDefined && !isVarUsed) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: The var is defined in this line. "
			     << "Skipping..\n";
#endif
		// Record this in symExprMap as resolved.
#if 0
		SymbolicExprs symExprs;
		symExprs.kind = SymbolicExprs::SymExprKind::DEFVAR;
		symExprs.resolved = true;
		bool ret = insertExprInSymMap(currVar, symExprs);
		if (!ret) {
		    error = true;
		    return false;
		}
#endif
		SymbolicDeclRefExpr* symExpr = new SymbolicDeclRefExpr;
		symExpr->vKind = SymbolicDeclRefExpr::VarKind::DEFVAR;
		symExpr->var = dred->var;
		symExpr->resolved = true;
		//symExpr->guardBlocks.insert(symExpr->guardBlocks.end(),
		    //currGuards.begin(), currGuards.end());
		std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
		    appendGuards(symExpr->guardBlocks, currGuards);
		symExpr->guardBlocks.clear();
		symExpr->guardBlocks.insert(symExpr->guardBlocks.end(),
		    appendedGuards.begin(), appendedGuards.end());
		SymbolicStmt* cloneVR = symExpr->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While cloning SymbolicDeclRefExpr: "
				 << "DEFVAR to insert in varsReferenced\n";
		    error = true;
		    return false;
		}
		if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
		    llvm::outs() << "ERROR: Clone of SymbolicDeclRefExpr is not "
				 << "SymbolicDeclRefExpr\n";
		    error = true;
		    return false;
		}
		symExpr->varsReferenced.push_back(dyn_cast<SymbolicDeclRefExpr>(cloneVR));
		if (!insertExprInSymMap(dred, symExpr)) {
		    llvm::outs() << "ERROR: While inserting symexpr for:\n";
		    dred->prettyPrint();
		    error = true;
		    return false;
		}
#ifdef DEBUG
		llvm::outs() << "DEBUG: Inserted symExpr for DeclRefExpr:\n";
		symExpr->prettyPrint(true);
#endif
		return true;
	    }
	}

	// Get the reaching definitions for this var at this block and this line
	if (rdObj.reachInSet.find(currBlock) == rdObj.reachInSet.end()) {
	    llvm::outs() << "ERROR: Cannot find entry for block " << currBlock
			 << " in reachInSet\n";
	    error = true;
	    return false;
	}

	bool defsPresentInBlock = false;
	if (rdObj.allDefsInBlock.find(currBlock) != rdObj.allDefsInBlock.end()) {
	    if (rdObj.allDefsInBlock[currBlock].find(dred->var.varID) !=
		    rdObj.allDefsInBlock[currBlock].end())
		defsPresentInBlock = true;
	}

	std::vector<Definition> defs;
	Definition currDef;
	bool foundDefInBlock = false;
	std::vector<Definition> blockDefs;
	if (defsPresentInBlock) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: defsPresentInBlock\n";
#endif
	    blockDefs = rdObj.allDefsInBlock[currBlock][dred->var.varID];
	    for (std::vector<Definition>::iterator bIt = blockDefs.begin();
		    bIt != blockDefs.end(); bIt++) {
		// If the current stmt is an input statement and we are looking to
		// find the reaching definition of an input var, skip
		if (bIt->lineNumber == lineNum) {
		    if (bIt->expressionStr.compare("DP_INPUT") == 0) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: Trying to find reaching def of input"
				     << " var at the read location. Skipping..\n";
#endif
			// Record this in symExprMap as resolved.
			SymbolicDeclRefExpr* symExpr = new SymbolicDeclRefExpr;
			symExpr->var = dred->var;
			symExpr->vKind =
			    SymbolicDeclRefExpr::VarKind::INPUTVAR;
			symExpr->resolved = true;
			SymbolicStmt* cloneVR = symExpr->clone(rv);
			if (!rv) {
			    llvm::outs() << "ERROR: While cloning SymbolicDeclRefExpr: "
					 << "INPUTVAR to insert in varsReferenced\n";
			    error = true;
			    return false;
			}
			if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
			    llvm::outs() << "ERROR: Clone of SymbolicDeclRefExpr "
					 << "is not SymbolicDeclRefExpr\n";
			    error = true;
			    return false;
			}
			symExpr->varsReferenced.push_back(dyn_cast<SymbolicDeclRefExpr>(cloneVR));
			if (!insertExprInSymMap(dred, symExpr)) {
			    llvm::outs() << "ERROR: While inserting symexpr for:\n";
			    dred->prettyPrint();
			    error = true;
			    return false;
			}
#ifdef DEBUG
			llvm::outs() << "DEBUG: Inserted symExpr for DeclRefExpr:\n";
			symExpr->prettyPrint(true);
#endif
			return true;
		    }
		}
		// Don't look at defs that are after the current line we are
		// looking at. TODO: There is some issue here. Should it be >=
		// or >
		if (bIt->lineNumber >= lineNum) continue;

		if (currDef.lineNumber == -1) {
		    currDef = *bIt;
		    foundDefInBlock = true;
		} else if (currDef.lineNumber < bIt->lineNumber) {
		    currDef = *bIt;
		    foundDefInBlock = true;
		}
	    }

	    if (foundDefInBlock)
		defs.push_back(currDef);

	}
	if (defs.size() == 0) {
	    if (rdObj.reachInSet[currBlock].find(dred->var.varID) ==
		    rdObj.reachInSet[currBlock].end()) {

		// TODO: Ideally, we should not be terminating here, but rather
		// give a warning or something that the var was used before
		// defining it.
		llvm::outs() << "ERROR: Var " << dred->var.varName << " does not have any "
			     << "reaching definitions at entry of block "
			     << currBlock << " and also no definitions inside the block\n";
		error = true;
		return false;

	    } else {
		// Get defs from reachInSet of the current block
		defs.insert(defs.end(), rdObj.reachInSet[currBlock][dred->var.varID].begin(),
		    rdObj.reachInSet[currBlock][dred->var.varID].end());
	    }
	}

#ifdef DEBUG
	llvm::outs() << "DEBUG: All defs identified for var: "; 
	dred->var.print();
	for (std::vector<Definition>::iterator dIt = defs.begin(); dIt != 
		defs.end(); dIt++) {
	    dIt->print();
	}
	llvm::outs() << "\n";
#endif

	std::vector<SpecialExpr*> spExprDefs;
	// Remove any defs that is part of special exprs and record the
	// corresponding special exprs.
	for (std::vector<Definition>::iterator dIt = defs.begin(); dIt != 
		defs.end(); dIt++) {
	    std::vector<SpecialExpr*>::iterator sIt;
	    bool foundSPExpr = false;
	    for (sIt = FA->spExprs.begin(); sIt != FA->spExprs.end(); sIt++) {
		if (dIt->lineNumber == (*sIt)->assignmentLine) {
		    foundSPExpr = true;
		    break;
		}
	    }
	    if (foundSPExpr) {
		//defs.erase(dIt);
		dIt->toDelete = true;
		SpecialExpr* clone = new SpecialExpr;
		(*sIt)->clone(clone);
		//spExprDefs.push_back((*sIt)->clone());
		spExprDefs.push_back(clone);
	    }
	}

	defs.erase(std::remove_if(defs.begin(), defs.end(), 
	    [](Definition d) { return d.toDelete; }), defs.end());

	// Remove any defs that is the initial val of special exprs identified
	// above
	for (std::vector<Definition>::iterator dIt = defs.begin(); dIt != 
		defs.end(); dIt++) {
	    bool foundSPInit = false;
	    std::vector<SpecialExpr*>::iterator sIt;
	    for (sIt = spExprDefs.begin(); sIt != spExprDefs.end(); sIt++) {
		if ((*sIt)->initialVal.lineNumber == dIt->lineNumber) {
		    foundSPInit = true;
		    break;
		}
	    }
	    if (foundSPInit) {
		dIt->toDelete = true;
	    }
	}

	defs.erase(std::remove_if(defs.begin(), defs.end(), 
	    [](Definition d) { return d.toDelete; }), defs.end());

	std::vector<LoopDetails*> loopIndexDefs;
	for (std::vector<Definition>::iterator dIt = defs.begin(); dIt != 
		defs.end(); dIt++) {
	    if (dIt->throughBackEdge) {
		bool isLoopIndex = (FA->isLoopIndexVar(dred->var, dIt->blockID, rv));
		if (!rv) {
		    error = true;
		    return false;
		}
		if (isLoopIndex) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Def is from a loop index\n";
#endif
		    std::vector<LoopDetails*> loops =
			FA->getLoopsOfBlock(dIt->blockID, rv);
		    if (!rv) {
			error = true;
			return false;
		    }
		    for (std::vector<LoopDetails*>::iterator lIt = loops.begin(); 
			    lIt != loops.end(); lIt++) {
			if ((*lIt)->loopIndexVar.equals(dred->var)) {
			    // check if this loop has break statements. If so,
			    // we cannot ignore this definition
			    if ((*lIt)->hasBreak) {
				llvm::outs() << "ERROR: Var " << dred->var.varName  
					     << " at line " << dred->lineNum << " has definition "
					     << "reaching through a back edge\n";
				dIt->print();
				error = true;
				return false;
			    }
			    dIt->toDelete = true;
			    loopIndexDefs.push_back(*lIt);
			    break;
			}
		    }
		} else {
		    llvm::outs() << "ERROR: Var " << dred->var.varName << " at line "
				 << dred->lineNum << " has definition "
				 << "reaching through a back edge\n";
		    dIt->print();
		    error = true;
		    return false;
		}
	    } 
	}

	for (std::vector<Definition>::iterator dIt = defs.begin(); dIt != 
		defs.end(); dIt++) {
	    if (dIt->toDelete) continue;
	    for (std::vector<LoopDetails*>::iterator lIt = loopIndexDefs.begin();
		    lIt != loopIndexDefs.end(); lIt++) {
		for (std::vector<std::pair<Definition, SymbolicExpr*> >::iterator idIt =
			(*lIt)->setOfLoopIndexInitValDefs.begin(); idIt !=
			(*lIt)->setOfLoopIndexInitValDefs.end(); idIt++) {
		    if (dIt->equals(idIt->first)) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: Removing loop index init val def from reaching defs\n";
#endif
			dIt->toDelete = true;
			break;
		    }
		}
		if (dIt->toDelete) break;
	    }
	}
	defs.erase(std::remove_if(defs.begin(), defs.end(), 
	    [](Definition d) { return d.toDelete; }), defs.end());

#ifdef DEBUG
	llvm::outs() << "============================\n";
	llvm::outs() << "DEBUG: Definitions for var: ";
	dred->var.print();
	llvm::outs() << "\nAt line: " << lineNum << "\n";
	int i = 0;
	for (std::vector<Definition>::iterator sIt = defs.begin(); sIt !=
		defs.end(); sIt++) {
	    llvm::outs() << "*********Expr " << ++i << "*******\n";
	    sIt->print();
	}
	for (std::vector<SpecialExpr*>::iterator sIt = spExprDefs.begin(); sIt != 
		spExprDefs.end(); sIt++) {
	    llvm::outs() << "*********Expr " << ++i << "*******\n";
	    (*sIt)->print();
	}
	llvm::outs() << "DEBUG: LoopIndexDefs:\n";
	for (std::vector<LoopDetails*>::iterator lIt = loopIndexDefs.begin(); lIt
		!= loopIndexDefs.end(); lIt++) {
	    llvm::outs() << "*********Expr " << ++i << "*******\n";
	    (*lIt)->print();
	}
	llvm::outs() << "============================\n";
#endif

	//defs.insert(defs.end(), spExprDefs.begin(), spExprDefs.end());
	for (std::vector<SpecialExpr*>::iterator sIt = spExprDefs.begin(); sIt
		!= spExprDefs.end(); sIt++) {
	    defs.push_back(**sIt);
	}

	std::vector<std::pair<GuardedDefinition, bool> > defsForThisVar;
	//bool varDefsResolved = true;
	// Get guarded definitions
	for (std::vector<Definition>::iterator dIt = defs.begin(); dIt !=
		defs.end(); dIt++) {
	    GuardedDefinition gDef;
	    // Find the guards of this def
	    std::vector<GUARD> guards;
	    std::vector<std::vector<std::pair<unsigned, bool> > > gB;
	    if (dIt->isSpecialExpr) {
		// If this is a special expr, then we need to find the guard of
		// the block containing the initialVal.
		Definition defObj = *dIt;
		SpecialExpr* spExprObj = static_cast<SpecialExpr*> (&defObj);
#if 0
		guards = FA->getGuardExprsOfBlock(spExprObj->initialVal.blockID, rv);
		if (!rv) {
		    error = true;
		    return false;
		}
#endif
		gB = FA->getGuardsOfBlock(spExprObj->initialVal.blockID);
	    } else {
#if 0
		guards = FA->getGuardExprsOfBlock(dIt->blockID, rv);
		if (!rv) {
		    error = true;
		    return false;
		}
#endif
		gB = FA->getGuardsOfBlock(dIt->blockID);
	    }

	    // Check if all the guard expressions are resolved. If not, we have
	    // a problem here. Ideally, we should have rewritten all the guards
	    // and definitions reaching here before visiting this var.
	    //std::vector<std::pair<SymbolicExpr*, bool> > rewrittenGuards;
#if 0
	    for (std::vector<GUARD>::iterator gIt = guards.begin(); gIt != 
		    guards.end(); gIt++) {
		if (!isResolved(gIt->first)) {
		    llvm::outs() << "ERROR: Guard is not resolved\n";
		    gIt->first->print();
		    error = true;
		    return false;
		}

#if 0
		std::vector<SymbolicStmt*> symExprs =
		    getSymbolicExprs(gIt->first, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: Cannot find symbolic exprs for guard:\n";
		    gIt->first->print();
		    error = true;
		    return false;
		}

		for (std::vector<SymbolicStmt*>::iterator gseIt =
			symExprs.begin(); gseIt != symExprs.end(); gseIt++) {
		}
#endif
	    }
#endif

	    //gDef.guards.insert(gDef.guards.end(), guards.begin(), guards.end());
	    gDef.def = dIt->clone();
#ifdef DEBUGFULL
	    llvm::outs() << "============================\n";
	    llvm::outs() << "DEBUG: Guarded Definition:\n";
	    gDef.print();
	    llvm::outs() << "============================\n";
#endif
	    std::string prefix("DP_GLOBAL_VAR");
	    auto res = std::mismatch(prefix.begin(), prefix.end(),
		dIt->expressionStr.begin());

	    if (!dIt->expression && dIt->expressionStr.compare("DP_INPUT") != 0 
		    && dIt->expressionStr.compare("DP_FUNC_PARAM") != 0 
		    && dIt->expressionStr.compare("DP_UNDEFINED") != 0
		    //&& dIt->expressionStr.compare("DP_GLOBAL_VAR") != 0
		    && res.first != prefix.end()    // check if DP_GLOBAL_VAR is a prefix
		    && !dIt->isSpecialExpr) {
		llvm::outs() << "ERROR: NULL expression for def:\n";
		dIt->print();
		error = true;
		return false;
	    }

	    if (dIt->expressionStr.compare("DP_INPUT") == 0) {
		// Def is the input read line
#ifdef DEBUG
		llvm::outs() << "DEBUG: Definition is the input read line\n";
#endif
		//defsForThisVar.push_back(std::pair<GuardedDefinition,
		    //bool>(gDef, true));
		SymbolicDeclRefExpr* sdre = new SymbolicDeclRefExpr;
		sdre->var = dred->var;
		sdre->vKind = SymbolicDeclRefExpr::VarKind::INPUTVAR;
		sdre->resolved = true;
		//sdre->guardBlocks.insert(sdre->guardBlocks.end(), currGuards.begin(), 
		    //currGuards.end());
		std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
		    appendGuards(sdre->guardBlocks, currGuards);
		sdre->guardBlocks.clear();
		sdre->guardBlocks.insert(sdre->guardBlocks.end(),
		    appendedGuards.begin(), appendedGuards.end());
		//sdre->guardBlocks.insert(sdre->guardBlocks.end(), gB.begin(), gB.end());
		appendedGuards = appendGuards(sdre->guardBlocks, gB);
		sdre->guardBlocks.clear();
		sdre->guardBlocks.insert(sdre->guardBlocks.end(),
		    appendedGuards.begin(), appendedGuards.end());
		//sdre->guards.insert(sdre->guards.end(), rewrittenGuards.begin(),
		    //rewrittenGuards.end());
		SymbolicStmt* cloneVR = sdre->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While cloning SymbolicDeclRefExpr: "
				 << "INPUTVAR for varsReferenced\n";
		    error = true;
		    return false;
		}
		if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
		    llvm::outs() << "ERROR: Clone of SymbolicDeclRefExpr is not "
				 << "SymbolicDeclRefExpr\n";
		    error = true;
		    return false;
		}
		sdre->varsReferenced.push_back(dyn_cast<SymbolicDeclRefExpr>(cloneVR));
		if (!insertExprInSymMap(dred, sdre)) {
		    llvm::outs() << "ERROR: While inserting symexpr for:\n";
		    dred->prettyPrint();
		    error = true;
		    return false;
		}

#ifdef DEBUG
		llvm::outs() << "DEBUG: Inserted symExpr for DeclRefExpr:\n";
		sdre->prettyPrint(true);
#endif
	    } else if (dIt->expressionStr.compare("DP_FUNC_PARAM") == 0) {
		// Def is the function parameter
#ifdef DEBUG
		llvm::outs() << "DEBUG: Definition is function parameter\n";
#endif
		//defsForThisVar.push_back(std::pair<GuardedDefinition,
		    //bool>(gDef, true));
		SymbolicDeclRefExpr* sdre = new SymbolicDeclRefExpr;
		sdre->var = dred->var;
		sdre->vKind = SymbolicDeclRefExpr::VarKind::FUNCPARAM;
		sdre->resolved = true;
		//sdre->guardBlocks.insert(sdre->guardBlocks.end(), currGuards.begin(), 
		    //currGuards.end());
		std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
		    appendGuards(sdre->guardBlocks, currGuards);
		sdre->guardBlocks.clear();
		sdre->guardBlocks.insert(sdre->guardBlocks.end(),
		    appendedGuards.begin(), appendedGuards.end());
		//sdre->guardBlocks.insert(sdre->guardBlocks.end(), gB.begin(), gB.end());
		appendedGuards = appendGuards(sdre->guardBlocks, gB);
		sdre->guardBlocks.clear();
		sdre->guardBlocks.insert(sdre->guardBlocks.end(),
		    appendedGuards.begin(), appendedGuards.end());
		//sdre->guards.insert(sdre->guards.end(), rewrittenGuards.begin(),
		    //rewrittenGuards.end());
		SymbolicStmt* cloneVR = sdre->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While cloning SymbolicDeclRefExpr: "
				 << "FUNCPARAM for varsReferenced\n";
		    error = true;
		    return false;
		}
		if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
		    llvm::outs() << "ERROR: Clone of SymbolicDeclRefExpr is not "
				 << "SymbolicDeclRefExpr\n";
		    error = true;
		    return false;
		}
		sdre->varsReferenced.push_back(
		    dyn_cast<SymbolicDeclRefExpr>(cloneVR));
		if (!insertExprInSymMap(dred, sdre)) {
		    llvm::outs() << "ERROR: While inserting symexpr for:\n";
		    dred->prettyPrint();
		    error = true;
		    return false;
		}

#ifdef DEBUG
		llvm::outs() << "DEBUG: Inserted symExpr for DeclRefExpr:\n";
		sdre->prettyPrint(true);
#endif
	    } else if 
	    //(dIt->expressionStr.compare("DP_GLOBAL_VAR") == 0) 
	    (res.first == prefix.end())
	    {
		// Def is a global var
#ifdef DEBUG
		llvm::outs() << "DEBUG: Definition is global var\n";
#endif
		//defsForThisVar.push_back(std::pair<GuardedDefinition,
		    //bool>(gDef, true));
		SymbolicDeclRefExpr* sdre = new SymbolicDeclRefExpr;
		sdre->var = dred->var;
		sdre->vKind = SymbolicDeclRefExpr::VarKind::GLOBALVAR;
		sdre->resolved = true;
		if (res.second != dIt->expressionStr.end()) {
		    std::string id(res.second, dIt->expressionStr.end());
		    sdre->callExprID = id;
		}
		//sdre->guardBlocks.insert(sdre->guardBlocks.end(), currGuards.begin(), 
		    //currGuards.end());
		std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
		    appendGuards(sdre->guardBlocks, currGuards);
		sdre->guardBlocks.clear();
		sdre->guardBlocks.insert(sdre->guardBlocks.end(),
		    appendedGuards.begin(), appendedGuards.end());
		//sdre->guardBlocks.insert(sdre->guardBlocks.end(), gB.begin(), gB.end());
		appendedGuards = appendGuards(sdre->guardBlocks, gB);
		sdre->guardBlocks.clear();
		sdre->guardBlocks.insert(sdre->guardBlocks.end(),
		    appendedGuards.begin(), appendedGuards.end());
		//sdre->guards.insert(sdre->guards.end(), rewrittenGuards.begin(),
		    //rewrittenGuards.end());
		SymbolicStmt* cloneVR = sdre->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While cloning SymbolicDeclRefExpr: "
				 << "GLOBALVAR for varsReferenced\n";
		    error = true;
		    return false;
		}
		if (!isa<SymbolicDeclRefExpr>(cloneVR)) {
		    llvm::outs() << "ERROR: Clone of SymbolicDeclRefExpr is not "
				 << "SymbolicDeclRefExpr\n";
		    error = true;
		    return false;
		}
		sdre->varsReferenced.push_back(
		    dyn_cast<SymbolicDeclRefExpr>(cloneVR));
		if (!insertExprInSymMap(dred, sdre)) {
		    llvm::outs() << "ERROR: While inserting symexpr for:\n";
		    dred->prettyPrint();
		    error = true;
		    return false;
		}

#ifdef DEBUG
		llvm::outs() << "DEBUG: Inserted symExpr for DeclRefExpr:\n";
		sdre->prettyPrint(true);
#endif
	    } else if (dIt->expressionStr.compare("DP_UNDEFINED") == 0) {
		// Def is the uninitialized value of a variable
#ifdef DEBUG
		llvm::outs() << "DEBUG: Definition is uninitialized\n";
#endif
		SymbolicDeclRefExpr* sdre = new SymbolicDeclRefExpr;
		sdre->vKind = SymbolicDeclRefExpr::VarKind::GARBAGE;
		sdre->resolved = true;
		sdre->guardBlocks.insert(sdre->guardBlocks.end(), currGuards.begin(), 
		    currGuards.end());
		std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
		    appendGuards(sdre->guardBlocks, gB);
		sdre->guardBlocks.clear();
		sdre->guardBlocks.insert(sdre->guardBlocks.end(),
		    appendedGuards.begin(), appendedGuards.end());
		//sdre->guards.insert(sdre->guards.end(), rewrittenGuards.begin(),
		    //rewrittenGuards.end());
		if (!insertExprInSymMap(dred, sdre)) {
		    llvm::outs() << "ERROR: While inserting symexpr for:\n";
		    dred->prettyPrint();
		    error = true;
		    return false;
		}

#ifdef DEBUG
		llvm::outs() << "DEBUG: Inserted symExpr for DeclRefExpr:\n";
		sdre->prettyPrint(true);
#endif
	    } else if (dIt->isSpecialExpr) {
		// Def is a special expr
#ifdef DEBUG
		llvm::outs() << "DEBUG: Definition is a special expr\n";
#endif
		//defsForThisVar.push_back(std::pair<GuardedDefinition,
		    //bool>(gDef, true));
		
		SpecialExpr* se;
		bool foundSE = false;
		for (std::vector<SpecialExpr*>::iterator seIt =
			spExprDefs.begin(); seIt != spExprDefs.end(); seIt++) {
		    if (dIt->equals(**seIt)) {
			se = *seIt;
			foundSE = true;
			break;
		    }
		}
		if (!foundSE) {
		    llvm::outs() << "ERROR: Cannot find SpecialExpr "
				 << "corresponding to the Definiton\n";
		    error = true;
		    return false;
		}

#ifdef DEBUG
		llvm::outs() << "DEBUG: Special Expression:\n";
		se->print();
#endif

		// Record guardBlock of special expr only if it is MAX or MIN
		unsigned guardBlock = -1;
		std::set<unsigned> blocks;
		if (se->kind != SpecialExprKind::SUM && !se->fromTernaryOperator) {
		    if (FA->lineToBlock.find(se->guardLine) == FA->lineToBlock.end()) {
			llvm::outs() << "ERROR: Cannot find block for line " 
				     << se->guardLine << "\n";
			error = true;
			return false;
		    }

		    blocks = FA->lineToBlock[se->guardLine];
		    if (blocks.size() != 1) {
			llvm::outs() << "ERROR: blocks of line " << se->guardLine
				     << " != 1\n";
			error = true;
			return false;
		    }

		    guardBlock = *(blocks.begin());
		}

		std::vector<VarDetails> loopIndices;
		std::vector<SymbolicExpr*> initialExprs, finalExprs;
		std::vector<bool> strictBounds;
		for (std::vector<LoopDetails*>::iterator lIt = se->loops.begin(); 
			lIt != se->loops.end(); lIt++) {
		    loopIndices.push_back((*lIt)->loopIndexVar);
		    (*lIt)->getSymbolicValuesForLoopHeader(SM, this, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While getSymbolicValuesForLoopHeader()\n";
			(*lIt)->prettyPrintHere();
			error = true;
			return false;
		    }

		    initialExprs.push_back((*lIt)->loopIndexInitValSymExpr);
		    if ((*lIt)->strictBound) {
			SymbolicBinaryOperator* sbo = new SymbolicBinaryOperator;
			sbo->guardBlocks.insert(sbo->guardBlocks.end(),
			    (*lIt)->loopIndexFinalValSymExpr->guardBlocks.begin(),
			    (*lIt)->loopIndexFinalValSymExpr->guardBlocks.end());
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
		const Expr* initialE = se->initialVal.expression;
		if (!initialE) {
		    llvm::outs() << "ERROR: NULL initial expression for special expression\n";
		    se->print();
		}

		Expr* initialExpr = const_cast<Expr*>(initialE);
		ExprDetails* initialExprDetails = ExprDetails::getObject(SM,
		    initialExpr, se->initialVal.blockID, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: Cannot obtain ExprDetails of initial "
				 << "expr of special expression\n";
		    se->print();
		    error = true;
		    return false;
		}

#ifdef DEBUG
		llvm::outs() << "DEBUG: Initial expression: " 
			     << Helper::prettyPrintExpr(initialExpr) << "\n";
		llvm::outs() << "DEBUG: Initial ExprDetails:\n";
		initialExprDetails->print();
		std::vector<SymbolicStmt*> symbolicExprs =
		    getSymbolicExprs(initialExprDetails, rv);
		if (!rv) {
		    llvm::outs() << "No Symexprs for initialexpression\n";
		} else {
		    for (std::vector<SymbolicStmt*>::iterator sIt =
			    symbolicExprs.begin(); sIt != symbolicExprs.end();
			    sIt++) {
			(*sIt)->prettyPrint(true);
		    }
		}
#endif

		//if (!isResolved(initialExprDetails)) 
		resolved = initialExprDetails->isSymExprCompletelyResolved(this, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While isSymExprCompletelyResolved() "
				 << "on initialExpr\n";
		    error = true;
		    return false;
		}
		if (!resolved)
		{
#if 0
		    llvm::outs() << "ERROR: Initial expr is not resolved\n";
		    llvm::outs() << "Special expr:\n";
		    se.print();
		    error = true;
		    return false;
#endif
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Initial expr is not resolved\n";
		    llvm::outs() << "Special expr:\n";
		    se->print();
#endif
		    //removeSymExprs(dred, rv);
		    //waitingSet = true;
		    //waitingForBlock = se.initialVal.blockID;
		    return true;
		    
		}
		std::vector<SymbolicStmt*> initialSymExprs =
		    getSymbolicExprs(initialExprDetails, rv);
		
		for (std::vector<SymbolicStmt*>::iterator iIt =
			initialSymExprs.begin(); iIt != initialSymExprs.end();
			iIt++) {
		    if (!isa<SymbolicExpr>(*iIt)) {
			llvm::outs() << "ERROR: Symbolic expr for initial expr "
				     << "is not <SymbolicExpr>\n";
			error = true;
			return false;
		    }
		    std::vector<std::vector<SymbolicExpr*> > allIndexSymExprs;
		    for (std::vector<Expr*>::iterator eIt =
			    se->arrayIndexExprsAtAssignLine.begin(); 
			    eIt != se->arrayIndexExprsAtAssignLine.end(); eIt++) {
			ExprDetails* indexExprDetails =
			    ExprDetails::getObject(SM, *eIt, se->assignmentBlock, rv);
			if (!rv) {
			    llvm::outs() << "ERROR: Cannot obtain ExprDetails "
					 << "from indexExpr: " 
					 << Helper::prettyPrintExpr(*eIt) << "\n";
			    error = true;
			    return false;
			}

			//if (!isResolved(indexExprDetails)) 
			resolved = indexExprDetails->isSymExprCompletelyResolved(this, rv);
			if (!rv) {
			    llvm::outs() << "ERROR: While isSymExprCompletelyResolved() "
					 << "on indexExpr\n";
			    error = true;
			    return false;
			}
			if (!resolved)
			{
#if 0
			    llvm::outs() << "ERROR: Index expr is not resolved: "
			                 << Helper::prettyPrintExpr(*eIt) << "\n";
			    indexExprDetails->print();
			    error = true;
			    return false;
#endif
#ifdef DEBUG
			    llvm::outs() << "DEBUG: Index expr is not resolved: "
					 << Helper::prettyPrintExpr(*eIt) << "\n";
			    indexExprDetails->prettyPrint();
			    llvm::outs() << "DEBUG: Call SymExprVisitor on index expr\n";
#endif
			    // Save state before calling symVisitor
			    unsigned processingBlock = currBlock;
			    std::vector<std::vector<std::pair<unsigned, bool> > > processingBlockGuards;
			    processingBlockGuards.insert(processingBlockGuards.end(),
				currGuards.begin(), currGuards.end());
			    setCurrBlock(indexExprDetails->blockID);
			    std::vector<std::vector<std::pair<unsigned, bool> >
			    > guards = FA->getGuardsOfBlock(indexExprDetails->blockID);
			    setCurrGuards(guards);
			    //VisitExpr(*eIt);
			    indexExprDetails->callVisitor(this, rv);
			    if (!rv) {
				llvm::outs() << "ERROR: While callVisitor on indexExpr\n";
				error = true;	
				return false;
			    }
			    // Reset state
			    setCurrBlock(processingBlock);
			    setCurrGuards(processingBlockGuards);
			    resolved = indexExprDetails->isSymExprCompletelyResolved(this, rv);
			    if (!rv) {
				llvm::outs() << "ERROR: While isSymExprCompletelyResolved()\n";
				error = true;
				return false;
			    }
			    if (!resolved) {
#ifdef DEBUG
				llvm::outs() << "DEBUG: Index expr still not resolved\n";
				indexExprDetails->prettyPrint();
#endif
				return true;
			    }
			    //removeSymExprs(dred, rv);
			    //waitingSet = true;
			    //waitingForBlock = se.assignmentBlock;
			    //return true;
			}

			std::vector<SymbolicStmt*> indexSymExprs =
			    getSymbolicExprs(indexExprDetails, rv);
			if (!rv) {
			    llvm::outs() << "ERROR: Cannot obtain symexprs for "
					 << Helper::prettyPrintExpr(*eIt) << "\n";
			    error = true;
			    return false;
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
				error = true;
				return false;
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
			sse->sKind = se->kind;
			sse->resolved = true;
			sse->arrayVar = new SymbolicDeclRefExpr(se->arrayVar,
			    SymbolicDeclRefExpr::VarKind::ARRAY, true);
			sse->varsReferenced.push_back(sse->arrayVar);

			//sse->guards.insert(sse->guards.end(), rewrittenGuards.begin(),
			    //rewrittenGuards.end());
			//sse->guardBlocks.insert(sse->guardBlocks.end(),
			    //currGuards.begin(), currGuards.end());
			std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
			    appendGuards(sse->guardBlocks, currGuards);
			sse->guardBlocks.clear();
			sse->guardBlocks.insert(sse->guardBlocks.end(),
			    appendedGuards.begin(), appendedGuards.end());
			//sse->guardBlocks.insert(sse->guardBlocks.end(), gB.begin(), gB.end());
			appendedGuards = appendGuards(sse->guardBlocks, gB);
			sse->guardBlocks.clear();
			sse->guardBlocks.insert(sse->guardBlocks.end(),
			    appendedGuards.begin(), appendedGuards.end());
			//sse->guardBlocks.insert(sse->guardBlocks.end(),
			    //(*iIt)->guardBlocks.begin(), (*iIt)->guardBlocks.end());
			appendedGuards = appendGuards(sse->guardBlocks, (*iIt)->guardBlocks);
			sse->guardBlocks.clear();
			sse->guardBlocks.insert(sse->guardBlocks.end(),
			    appendedGuards.begin(), appendedGuards.end());
			std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendedGuardExprs =
			    appendGuards(sse->guards, (*iIt)->guards, rv);
			if (!rv) {
			    llvm::outs() << "ERROR: While appending guardExprs\n";
			    error = true;
			    return false;
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
			    //sse->guardBlocks.insert(sse->guardBlocks.end(), 
				//(*sIt)->guardBlocks.begin(), (*sIt)->guardBlocks.end());
			    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
				appendGuards(sse->guardBlocks, (*sIt)->guardBlocks);
			    sse->guardBlocks.clear();
			    sse->guardBlocks.insert(sse->guardBlocks.end(),
				appendedGuards.begin(), appendedGuards.end());
			    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendedGuardExprs =
				appendGuards(sse->guards, (*sIt)->guards, rv);
			    if (!rv) {
				llvm::outs() << "ERROR: While appending guardExprs\n";
				error = true;
				return false;
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
				    for (lIt = se->loops.begin(); lIt != 
					    se->loops.end(); lIt++) {
#ifdef DEBUG
					llvm::outs() << "DEBUG: loop index: ";
					(*lIt)->loopIndexVar.print();
					llvm::outs() << "\n";
#endif
					if ((*lIt)->loopIndexVar.equals(indexDRE->var)) {
					//if (lIt->loopIndexVar.varName.compare(indexDRE->var.varName) == 0 
						//&& lIt->loopIndexVar.varDeclLine == indexDRE->var.varDeclLine)
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
						error = true;
						return false;
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
						error = true;
						return false;
					    }

					    //if (!isResolved(initialDetails)) 
					    resolved = initialDetails->isSymExprCompletelyResolved(this, rv);
					    if (!rv) {
						llvm::outs() << "ERROR: While isSymExprCompletelyResolved() "
							     << "on initialExpr\n";
						error = true;
						return false;
					    }
					    if (!resolved)
					    {
#if 0
						llvm::outs() << "ERROR: Initial expression is not resolved\n";
						initialDetails->print();
						error = true;
						return false;
#endif
#ifdef DEBUG
						llvm::outs() << "DEBUG: Initial expression is not resolved\n";
						initialDetails->prettyPrint();
#endif
						//removeSymExprs(dred, rv);
						//waitingSet = true;
						//waitingForBlock = lIt->loopIndexInitValDef.blockID;
						return true;
					    }
					    std::vector<SymbolicStmt*> isymExprs = getSymbolicExprs(initialDetails, rv);
					    if (!rv) {
						llvm::outs() << "ERROR: Cannot obtain symbolic exprs for intial expression\n";
						initialDetails->prettyPrint();
						error = true;
						return false;
					    }
					    for (std::vector<SymbolicStmt*>::iterator it = isymExprs.begin();
						    it != isymExprs.end(); it++) {
						if (!isa<SymbolicExpr>(*it)) {
						    llvm::outs() << "ERROR: Symexpr for initial expression is not <SymbolicExpr>\n";
						    error = true;
						    return false;
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
						error = true;
						return false;
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
						error = true;
						return false;
					    }

					    //if (!isResolved(finalDetails)) 
					    resolved = finalDetails->isSymExprCompletelyResolved(this, rv);
					    if (!rv) {
						llvm::outs() << "ERROR: While isSymExprCompletelyResolved() "
							     << "on finalExpr\n";
						error = true;
						return false;
					    }
					    if (!resolved)
					    {
#if 0
						llvm::outs() << "ERROR: Final expression is not resolved\n";
						finalDetails->print();
						error = true;
						return false;
#endif
#ifdef DEBUG
						llvm::outs() << "DEBUG: Final expression is not resolved\n";
						finalDetails->prettyPrint();
#endif
						//removeSymExprs(dred, rv);
						//waitingSet = true;
						//waitingForBlock = lIt->loopIndexFinalValDef.blockID;
						return true;
					    }
					    std::vector<SymbolicStmt*> fsymExprs = getSymbolicExprs(finalDetails, rv);
					    if (!rv) {
						llvm::outs() << "ERROR: Cannot obtain symbolic exprs for final expression\n";
						finalDetails->prettyPrint();
						error = true;
						return false;
					    }
					    for (std::vector<SymbolicStmt*>::iterator it = fsymExprs.begin();
						    it != fsymExprs.end(); it++) {
						if (!isa<SymbolicExpr>(*it)) {
						    llvm::outs() << "ERROR: Symexpr for final expression is not <SymbolicExpr>\n";
						    error = true;
						    return false;
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
					error = true;
					return false;
				    }
				    if (!isa<SymbolicExpr>(cloneIndex)) {
					llvm::outs() << "ERROR: Clone of SymbolicExpr is not SymbolicExpr\n";
					error = true;
					return false;
				    }
				    SymbolicExpr* cloneIndexExpr =
					dyn_cast<SymbolicExpr>(cloneIndex);
				    cloneIndexExpr->replaceVarsWithExprs(loopIndices, initialExprs, rv);
				    if (!rv) {
					llvm::outs() << "ERROR: While replacing vars in indexexpr\n";
					error = true;
					return false;
				    }
				    initVec.push_back(cloneIndexExpr);
				    cloneIndex = (*sIt)->clone(rv);
				    if (!rv) {
					error = true;
					return false;
				    }
				    cloneIndexExpr =
					dyn_cast<SymbolicExpr>(cloneIndex);
				    cloneIndexExpr->replaceVarsWithExprs(loopIndices, finalExprs, rv);
				    if (!rv) {
					llvm::outs() << "ERROR: While replacing vars in indexexpr\n";
					error = true;
					return false;
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
				    error = true;
				    return false;
				}
				if (!isa<SymbolicExpr>(cloneIndex)) {
				    llvm::outs() << "ERROR: Clone of SymbolicExpr is not SymbolicExpr\n";
				    error = true;
				    return false;
				}
				SymbolicExpr* cloneIndexExpr =
				    dyn_cast<SymbolicExpr>(cloneIndex);
				cloneIndexExpr->replaceVarsWithExprs(loopIndices, initialExprs, rv);
				if (!rv) {
				    llvm::outs() << "ERROR: While replacing vars in indexexpr\n";
				    error = true;
				    return false;
				}
				initVec.push_back(cloneIndexExpr);
				cloneIndex = (*sIt)->clone(rv);
				if (!rv) {
				    error = true;
				    return false;
				}
				cloneIndexExpr =
				    dyn_cast<SymbolicExpr>(cloneIndex);
				cloneIndexExpr->replaceVarsWithExprs(loopIndices, finalExprs, rv);
				if (!rv) {
				    llvm::outs() << "ERROR: While replacing vars in indexexpr\n";
				    error = true;
				    return false;
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
				error = true;
				return false;
			    }
			    if (!isa<SymbolicSpecialExpr>(cloneStmt)) {
				llvm::outs() << "ERROR: Clone of SpecialExpr is "
					     << "not SpecialExpr\n";
				error = true;
				return false;
			    }
			    SymbolicSpecialExpr* cloneSSE =
				dyn_cast<SymbolicSpecialExpr>(cloneStmt);
			    for (std::vector<std::pair<std::pair<SymbolicExpr*, SymbolicExpr*>, bool> >::iterator
				    rIt = indexIt->begin(); rIt !=
				    indexIt->end(); rIt++) {
				//cloneSSE->guardBlocks.insert(cloneSSE->guardBlocks.end(),
				    //rIt->first.first->guardBlocks.begin(), 
				    //rIt->first.first->guardBlocks.end());
				std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
				    appendGuards(cloneSSE->guardBlocks, rIt->first.first->guardBlocks);
				cloneSSE->guardBlocks.clear();
				cloneSSE->guardBlocks.insert(cloneSSE->guardBlocks.end(),
				    appendedGuards.begin(), appendedGuards.end());
				std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendedGuardExprs =
				    appendGuards(cloneSSE->guards, rIt->first.first->guards, rv);
				if (!rv) {
				    llvm::outs() << "ERROR: While appending guardExprs\n";
				    error = true;
				    return false;
				}
				cloneSSE->guards.clear();
				cloneSSE->guards.insert(cloneSSE->guards.end(),
				    appendedGuardExprs.begin(), appendedGuardExprs.end());
				//cloneSSE->guardBlocks.insert(cloneSSE->guardBlocks.end(),
				    //rIt->first.second->guardBlocks.begin(), 
				    //rIt->first.second->guardBlocks.end());
				appendedGuards = appendGuards(cloneSSE->guardBlocks, rIt->first.second->guardBlocks);
				cloneSSE->guardBlocks.clear();
				cloneSSE->guardBlocks.insert(cloneSSE->guardBlocks.end(),
				    appendedGuards.begin(), appendedGuards.end());
				appendedGuardExprs =
				    appendGuards(cloneSSE->guards, rIt->first.second->guards, rv);
				if (!rv) {
				    llvm::outs() << "ERROR: While appending guardExprs\n";
				    error = true;
				    return false;
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
			    // Remove guard corresponding to guardBlock of
			    // SpecialExpr
			    std::vector<std::vector<std::pair<unsigned, bool> > >::iterator it;
			    for (it = cloneSSE->guardBlocks.begin(); it !=
				    cloneSSE->guardBlocks.end(); it++) {
				std::vector<std::pair<unsigned, bool>>::iterator vIt;
				for (vIt = it->begin(); vIt != it->end(); vIt++) {
				    if (vIt->first == guardBlock) break;
				}
				if (vIt != it->end()) {
				    it->erase(vIt);
				}
			    }
			    cloneSSE->sane = se->sane;
			    cloneSSE->originalSpecialExpr = se;

			    if (!insertExprInSymMap(dred, cloneSSE)) {
				llvm::outs() << "ERROR: While inserting symexpr for:\n";
				dred->prettyPrint();
				error = true;
				return false;
			    } else {
#ifdef DEBUG
				llvm::outs() << "DEBUG: SSE inserted\n";
				cloneSSE->prettyPrint(true);
				llvm::outs() << "\n";
				llvm::outs() << (cloneSSE->sane? "sane": "not sane") << "\n";
				cloneSSE->originalSpecialExpr->print();
#endif
			    }
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
		    error = true;
		    return false;
		}

#if 0
		std::pair<std::map<ExprDetails, SymbolicExprs>::iterator, bool> ret;
		SymbolicExprs s = getSymbolicExprs(ed, rv);
		if (!rv) {
		    // Could not find symbolic expr of this expr. So we conclude
		    // that this def/expr is not completely resolved.
		    llvm::outs() << "ERROR: The definition is not resolved\n";
		    error = true;
		    return false;
#if 0
		    std::pair<GuardedDefinition, bool> sExpr;
		    sExpr.first = gDef;
		    sExpr.second = false;
		    varDefsResolved = false;
		    defsForThisVar.push_back(sExpr);
#endif
		} else {
		    for (std::vector<std::pair<GuardedDefinition, bool> >::iterator 
			    gIt = s.defs.begin(); gIt != s.defs.end(); gIt++) {
			gIt->first.guards.insert(gIt->first.guards.end(),
			    guards.begin(), guards.end());
			if (!gIt->second) varDefsResolved = false;
		    }

		    defsForThisVar.insert(defsForThisVar.end(), s.defs.begin(),
			s.defs.end());
		}
#endif
		//if (!isResolved(ed)) 
		resolved = ed->isSymExprCompletelyResolved(this, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While isSymExprCompletelyResolved() "
				 << "on Expr\n";
		    error = true;
		    return false;
		}
		if (!resolved)
		{
#if 0
		    llvm::outs() << "ERROR: The definition is not resolved\n";
		    ed->print();
		    error = true;
		    return false;
#endif
#ifdef DEBUG
		    llvm::outs() << "DEBUG: The definition is not resolved\n";
		    ed->prettyPrint();
#endif
		    //removeSymExprs(dred, rv);
		    //waitingSet = true;
		    //waitingForBlock = dIt->blockID;

		    // Visit definition here. Before that save symVisitor state
		    unsigned processingBlock = currBlock;
		    std::vector<std::vector<std::pair<unsigned, bool> > >
		    processingBlockGuards;
		    processingBlockGuards.insert(processingBlockGuards.end(), 
			currGuards.begin(), currGuards.end());
		    setCurrBlock(dIt->blockID);
		    std::vector<std::vector<std::pair<unsigned, bool> > > guards
			= FA->getGuardsOfBlock(dIt->blockID);
		    setCurrGuards(guards);
#ifdef DEBUG
		    llvm::outs() << "DEBUG: About to visit expr: "
				 << Helper::prettyPrintExpr(dIt->expression) << "\n";
#endif
		    //VisitExpr(const_cast<Expr*>(dIt->expression));
		    ed->callVisitor(this, rv);
		    if (!rv) {
			setCurrBlock(processingBlock);
			setCurrGuards(processingBlockGuards);
			return false;
		    }
		    resolved = ed->isSymExprCompletelyResolved(this, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While isSymExprCompletelyResolved()\n";
			error = true;
			setCurrBlock(processingBlock);
			setCurrGuards(processingBlockGuards);
			return false;
		    }
		    
		    if (!resolved) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: Not resolved a second time\n";
#endif
			setCurrBlock(processingBlock);
			setCurrGuards(processingBlockGuards);
			return true;
		    }

		    // Reset the symVisitor state
		    setCurrBlock(processingBlock);
		    setCurrGuards(processingBlockGuards);
		}

		std::vector<SymbolicStmt*> symExprs = getSymbolicExprs(ed, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: Cannot get symbolic exprs of ";
		    ed->prettyPrint();
		    error = true;
		    return false;
		}
		for (std::vector<SymbolicStmt*>::iterator sIt =
			symExprs.begin(); sIt != symExprs.end(); sIt++) {
		    if (!isa<SymbolicExpr>(*sIt)) {
			llvm::outs() << "ERROR: Symbolic expr for definition is "
				     << "not <SymbolicExpr>\n";
			ed->prettyPrint();
			error = true;
			return false; 
		    } 
#if 0 
		    SymbolicExpr* se = new SymbolicExpr; 
		    se->guardBlocks.insert(currGuards.begin(), currGuards.end()); 
		    se->guardBlocks.insert((*sIt)->guardBlocks.begin(), (*sIt)->guardBlocks.end()); 
#endif 
		    // TODO: Ideally, the current guards need to be added to the 
		    // symbolic expr corresponding to current DeclRefExpr. If I 
		    // do it here, the guards will be added to the def's 
		    // symbolic expr as well.  
		    if (!insertExprInSymMap(dred, *sIt)) { 
			llvm::outs() << "ERROR: While inserting symexpr for:\n"; 
			dred->prettyPrint(); 
			error = true; 
			return false; 
		    } 
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Inserted symExpr for DeclRefExpr:\n";
		    dred->prettyPrint();
		    (*sIt)->prettyPrint(true);
#endif
		}
	    }

	    defsForThisVar.push_back(std::pair<GuardedDefinition,
		bool>(gDef, true));
	}

#ifdef DEBUG
	llvm::outs() << "DEBUG: About to look through loopIndexDefs: " 
		     << loopIndexDefs.size() << "\n";
#endif
	for (std::vector<LoopDetails*>::iterator lIt = loopIndexDefs.begin(); 
		lIt != loopIndexDefs.end(); lIt++) {
		ExprDetails* ed = ExprDetails::getObject(SM,
		    const_cast<Expr*>((*lIt)->loopIndexFinalValDef.expression),
		    (*lIt)->loopIndexFinalValDef.blockID, rv);
		if (!rv) {
		    error = true;
		    return false;
		}

		//if (!isResolved(ed)) 
		resolved = ed->isSymExprCompletelyResolved(this, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While isSymExprCompletelyResolved() "
				 << "on loopIndexFinalVal\n";
		    error = true;
		    return false;
		}
		if (!resolved)
		{
#ifdef DEBUG
		    llvm::outs() << "DEBUG: The definition is not resolved\n";
		    ed->prettyPrint();
#endif
		    //removeSymExprs(dred, rv);
		    //waitingSet = true;
		    //waitingForBlock = lIt->loopIndexFinalValDef.blockID;
		    return true;
		}

		std::vector<SymbolicStmt*> symExprs = getSymbolicExprs(ed, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: Cannot get symbolic exprs of ";
		    ed->prettyPrint();
		    error = true;
		    return false;
		}
#ifdef DEBUG
		llvm::outs() << "DEBUG: loopIndexFinalValDef.expression is resolved: "
			     << symExprs.size() << "\n";
#endif

		for (std::vector<SymbolicStmt*>::iterator sIt =
			symExprs.begin(); sIt != symExprs.end(); sIt++) {
		    if (!isa<SymbolicExpr>(*sIt)) {
			llvm::outs() << "ERROR: Symbolic expr for definition is "
				     << "not <SymbolicExpr>\n";
			ed->prettyPrint();
			error = true;
			return false;
		    }

		    if (!(*lIt)->strictBound) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: Loop does not have strictBound\n";
#endif
			SymbolicBinaryOperator* sbo = new SymbolicBinaryOperator;
			sbo->resolved = true;
			sbo->guardBlocks.insert(sbo->guardBlocks.end(),
			    (*sIt)->guardBlocks.begin(),
			    (*sIt)->guardBlocks.end());
			sbo->guards.insert(sbo->guards.end(),
			    (*sIt)->guards.begin(),
			    (*sIt)->guards.end());
			sbo->lhs = dyn_cast<SymbolicExpr>(*sIt);
			if ((*lIt)->loopStep == 1)
			    sbo->opKind = BinaryOperatorKind::BO_Add;
			else if ((*lIt)->loopStep == -1)
			    sbo->opKind = BinaryOperatorKind::BO_Sub;
			SymbolicIntegerLiteral* sil = new SymbolicIntegerLiteral;
			llvm::APInt val(32,1);
			sil->val = val;
			sbo->rhs = sil;
			if (!insertExprInSymMap(dred, sbo)) {
			    llvm::outs() << "ERROR: While inserting symexpr for:\n";
			    dred->prettyPrint();
			    error = true;
			    return false;
			}
#ifdef DEBUG
			llvm::outs() << "DEBUG: Inserted symExpr for DeclRefExpr:\n";
			sbo->prettyPrint(true);
#endif

		    } else {
#ifdef DEBUG
			llvm::outs() << "DEBUG: Loop has strictBound\n";
#endif
			if (!insertExprInSymMap(dred, *sIt)) {
			    llvm::outs() << "ERROR: While inserting symexpr for:\n";
			    dred->prettyPrint();
			    error = true;
			    return false;
			}

#ifdef DEBUG
			llvm::outs() << "DEBUG: Inserted symExpr for DeclRefExpr:\n";
			(*sIt)->prettyPrint(true);
#endif
		    }
		}
	}

#ifdef DEBUG
	llvm::outs() << "============================\n";
	llvm::outs() << "DEBUG: defsForThisVar:\n";
	for (std::vector<std::pair<GuardedDefinition, bool> >::iterator gIt =
		defsForThisVar.begin(); gIt != defsForThisVar.end(); gIt++) {
	    gIt->first.print();
	    llvm::outs() << (gIt->second? "Resolved": "Not resolved") << "\n";
	}
	llvm::outs() << "============================\n";
#endif
	std::vector<SymbolicStmt*> symExprs = getSymbolicExprs(dred, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Could not find symbolic expr for the dred\n";
	    error = true;
	    return false;
	}

#ifdef DEBUG
	llvm::outs() << "----------\n";
	llvm::outs() << "DEBUG: Symbolic exprs for expr:\n";
	//currVar->print();
	dred->prettyPrint();
	llvm::outs() << "\n --> \n";
	for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); sIt != 
		symExprs.end(); sIt++) {
	    (*sIt)->prettyPrint(true);
	    llvm::outs() << "\n";
	}
	llvm::outs() << "----------\n";
#endif

	// Check the symExprs added. If there is at least one symexpr with true
	// guard and at least one with another guard, we need to rewrite the
	// true guard with the negation of the other guard.
	// int x = 0; if (g) x = y; The symexprs should be {g: y, !g: 0} and not
	// {true: 0, g: y}
	std::vector<std::vector<std::pair<unsigned, bool> > > negatedGuardBlocks;
	std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > negatedGuards;
	bool foundTrueGuard = false;
	for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); sIt != 
		symExprs.end(); sIt++) {
	    if ((*sIt)->guardBlocks.size() == 0 && (*sIt)->guards.size() == 0) {
		if (foundTrueGuard) {
		    llvm::outs() << "ERROR: Multiple symexprs with true guard. "
				 << "Something's wrong!\n";
		    error = true;
		    return false;
		}
		foundTrueGuard = true;
	    } else {
		for (std::vector<std::vector<std::pair<unsigned, bool> >
			>::iterator gIt = (*sIt)->guardBlocks.begin(); gIt != 
			(*sIt)->guardBlocks.end(); gIt++) {
		    std::vector<std::vector<std::pair<unsigned, bool> > > negatedVec;
		    for (std::vector<std::pair<unsigned, bool> >::iterator vIt = 
			    gIt->begin(); vIt != gIt->end(); vIt++) {
			std::vector<std::pair<unsigned, bool> > v;
			v.push_back(std::make_pair(vIt->first, !(vIt->second)));
			negatedVec.push_back(v);
		    }

		    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
			appendGuards(negatedGuardBlocks, negatedVec);
		    negatedGuardBlocks.clear();
		    negatedGuardBlocks.insert(negatedGuardBlocks.end(),
			appendedGuards.begin(), appendedGuards.end());
		}
		for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
			>::iterator gIt = (*sIt)->guards.begin(); gIt != 
			(*sIt)->guards.end(); gIt++) {
		    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > negatedVec;
		    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator vIt = 
			    gIt->begin(); vIt != gIt->end(); vIt++) {
			std::vector<std::pair<SymbolicExpr*, bool> > v;
			v.push_back(std::make_pair(vIt->first, !(vIt->second)));
			negatedVec.push_back(v);
		    }

		    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendedGuards =
			appendGuards(negatedGuards, negatedVec, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While appending guards\n";
			error = true;
			return false;
		    }
		    negatedGuards.clear();
		    negatedGuards.insert(negatedGuards.end(),
			appendedGuards.begin(), appendedGuards.end());
		}
	    }
	}

	if (!foundTrueGuard) return false; // No issues

	for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); sIt != 
		symExprs.end(); sIt++) {
	    if ((*sIt)->guardBlocks.size() == 0 && (*sIt)->guards.size() == 0) {
		(*sIt)->guardBlocks.insert((*sIt)->guardBlocks.end(),
		    negatedGuardBlocks.begin(), negatedGuardBlocks.end());
		(*sIt)->guards.insert((*sIt)->guards.end(),
		    negatedGuards.begin(), negatedGuards.end());
		return false;
	    }
	}
    }

    return true;
}

bool GetSymbolicExprVisitor::VisitArraySubscriptExpr(ArraySubscriptExpr* A) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering GetSymbolicExprVisitor::VisitArraySubscriptExpr()\n";
#endif
    if (!A->isLValue()) return true;

    SourceLocation SL = SM->getExpansionLoc(A->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: ArraySubscriptExpr is not in main program\n";
	error = true;
	return false;
    }

    bool resolved;
    bool rv;
    ArraySubscriptExprDetails* currDetails =
	ArraySubscriptExprDetails::getObject(SM, A, currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Could not obtain ArraySubscriptExprDetails from "
		     << "ArraySubscriptExpr\n";
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting ArraySubscriptExpr at line " 
		 << currDetails->lineNum << "\n";
    currDetails->prettyPrint();
    llvm::outs() << "\n";
#endif

    //if (isResolved(currDetails)) 
    resolved = currDetails->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While ArraySubscriptExprDetails::isSymExprCompletelyResolved()\n";
	error = true;
	return false;
    }
    if (resolved)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: ArraySubscriptExpr is already resolved. "
		     << "Skipping..\n";
#endif
	return true;
    }

    // Check if base array expr is resolved
    //if (!isResolved(currDetails->baseArray)) 
    resolved = currDetails->baseArray->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on baseArray\n";
	error = true;
	return false;
    }
    if (!resolved) return true;

    // Check if index exprs are resolved
    for (std::vector<ExprDetails*>::iterator eIt =
	    currDetails->indexExprs.begin(); eIt != 
	    currDetails->indexExprs.end(); eIt++) {
	//if (!isResolved(*eIt)) 
	resolved = (*eIt)->isSymExprCompletelyResolved(this, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on indexExpr\n";
	    error = true;
	    return false;
	}
	if (!resolved) return true;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Base and indices of ArraySubscriptExpr are "
		 << "resolved\n";
#endif
    // Construct symbolic exprs of ArraySubscriptExpr from those of the base
    // array and the index exprs.
    std::vector<std::vector<SymbolicExpr*> > allIndexExprs;
    for (std::vector<ExprDetails*>::iterator iIt =
	    currDetails->indexExprs.begin(); iIt != 
	    currDetails->indexExprs.end(); iIt++) {
	std::vector<SymbolicStmt*> currIndexSymExprs = getSymbolicExprs(*iIt, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Could not obtain symbolic exprs for index\n";
	    error = true;
	    return false;
	}

	bool emptyVec = false;
	if (allIndexExprs.size() == 0) emptyVec = true;

	std::vector<std::vector<SymbolicExpr*> > currIndexExprs;
	for (std::vector<SymbolicStmt*>::iterator sIt =
		currIndexSymExprs.begin(); sIt != currIndexSymExprs.end();
		sIt++) {
	    if (!isa<SymbolicExpr>(*sIt)) {
		llvm::outs() << "ERROR: Symbolic expr for index is not "
			     << "<SymbolicExpr>\n";
		error = true;
		return false;
	    }

	    if (emptyVec) {
		std::vector<SymbolicExpr*> currVec;
		currVec.push_back(dyn_cast<SymbolicExpr>(*sIt));
		currIndexExprs.push_back(currVec);
	    } else {
		for (std::vector<std::vector<SymbolicExpr*> >::iterator vIt = 
			allIndexExprs.begin(); vIt != allIndexExprs.end();
			vIt++) {
		    std::vector<SymbolicExpr*> currVec;
		    currVec.insert(currVec.end(), vIt->begin(), vIt->end());
		    currVec.push_back(dyn_cast<SymbolicExpr>(*sIt));
		    currIndexExprs.push_back(currVec);
		}
	    }
	}

	allIndexExprs.clear();
	allIndexExprs.insert(allIndexExprs.end(), currIndexExprs.begin(),
	    currIndexExprs.end());
	currIndexExprs.clear();
    }

    std::vector<SymbolicStmt*> baseSymExprs =
	getSymbolicExprs(currDetails->baseArray, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Could not obtain symbolic exprs for base array\n";
	error = true;
	return false;
    }

    for (std::vector<SymbolicStmt*>::iterator bIt = baseSymExprs.begin(); 
	    bIt != baseSymExprs.end(); bIt++) {
	if (!isa<SymbolicDeclRefExpr>(*bIt)) {
	    llvm::outs() << "ERROR: Symbolic expr for array base is not "
			 << "<SymbolicDeclRefExpr>\n";
	    error = true;
	    return false;
	}

	for (std::vector<std::vector<SymbolicExpr*> >::iterator iIt =
		allIndexExprs.begin(); iIt != allIndexExprs.end(); iIt++) {
	    SymbolicArraySubscriptExpr* symase = new SymbolicArraySubscriptExpr;
	    symase->resolved = true;
	    //symase->guardBlocks.insert(symase->guardBlocks.end(), currGuards.begin(), 
		//currGuards.end());
	    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
		appendGuards(symase->guardBlocks, currGuards);
	    symase->guardBlocks.clear();
	    symase->guardBlocks.insert(symase->guardBlocks.end(),
		appendedGuards.begin(), appendedGuards.end());
	    //symase->guardBlocks.insert(symase->guardBlocks.end(), 
		//(*bIt)->guardBlocks.begin(), ((*bIt)->guardBlocks.end()));
	    appendedGuards = appendGuards(symase->guardBlocks, (*bIt)->guardBlocks);
	    symase->guardBlocks.clear();
	    symase->guardBlocks.insert(symase->guardBlocks.end(),
		appendedGuards.begin(), appendedGuards.end());
	    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendedGuardExprs =
		appendGuards(symase->guards, (*bIt)->guards, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While appending guardExprs\n";
		error = true;
		return false;
	    }
	    symase->guards.clear();
	    symase->guards.insert(symase->guards.end(),
		appendedGuardExprs.begin(), appendedGuardExprs.end());
	    symase->baseArray = dyn_cast<SymbolicDeclRefExpr>(*bIt);
	    for (std::vector<SymbolicDeclRefExpr*>::iterator baseArrayVRIt = 
		    (*bIt)->varsReferenced.begin(); baseArrayVRIt != 
		    (*bIt)->varsReferenced.end(); baseArrayVRIt++) {
		symase->varsReferenced.push_back(*baseArrayVRIt);
	    }
	    for (std::vector<SymbolicExpr*>::iterator indexIt = iIt->begin();
		    indexIt != iIt->end(); indexIt++) {
		//symase->guardBlocks.insert(symase->guardBlocks.end(), 
		    //(*indexIt)->guardBlocks.begin(), (*indexIt)->guardBlocks.end());
		std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
		    appendGuards(symase->guardBlocks, currGuards);
		symase->guardBlocks.clear();
		symase->guardBlocks.insert(symase->guardBlocks.end(),
		    appendedGuards.begin(), appendedGuards.end());
		symase->indexExprs.push_back(*indexIt);
		for (std::vector<SymbolicDeclRefExpr*>::iterator indexVRIt = 
			(*indexIt)->varsReferenced.begin(); indexVRIt != 
			(*indexIt)->varsReferenced.end(); indexVRIt++) {
		    bool foundVR = false;
		    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt = 
			    symase->varsReferenced.begin(); vrIt != 
			    symase->varsReferenced.end(); vrIt++) {
			if ((*indexVRIt)->var.equals((*vrIt)->var)) {
			    foundVR = true;
			    break;
			}
		    }
		    if (!foundVR)
			symase->varsReferenced.push_back(*indexVRIt);
		}
	    }
	    if(!insertExprInSymMap(currDetails, symase)) {
		llvm::outs() << "ERROR: While inserting symexpr for:\n";
		currDetails->prettyPrint();
		error = true;
		return false;
	    }
	}
    }

    return true;
}

bool GetSymbolicExprVisitor::VisitDeclStmt(DeclStmt* D) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering GetSymbolicExprVisitor::VisitDeclStmt()\n";
#endif
    bool resolved;
    bool rv;
    SourceLocation SL = SM->getExpansionLoc(D->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL))
	return true;

    int lineNum = SM->getExpansionLineNumber(D->getLocStart());
    if (!D->isSingleDecl()) {
#ifdef DEBUG
	llvm::outs() << "ERROR: DeclStmt at line " << lineNum 
		     << " has more than one Decl\n";
#endif
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting DeclStmt at line " << lineNum << "\n";
#endif

    StmtDetails* sd = StmtDetails::getStmtDetails(SM, D, currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain StmtDetails from "
		     << Helper::prettyPrintStmt(D) << "\n";
	error = true;
	return false;
    }
#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting DeclStmt at line " << lineNum << "\n";
    //sd->print();
    sd->prettyPrint();
#endif

    // Check if this stmt is already resolved
#if 0
    SymbolicExprs sSE = getSymbolicExprs(sd, rv);
    if (rv) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: DeclStmt already resolved. Skipping..\n";
#endif
	return true;
    }

    // Find the var declared and check if it is resolved.
    if (isa<DeclStmtDetails>(sd)) {
	DeclStmtDetails* dsd = dyn_cast<DeclStmtDetails>(sd);
	ExprDetails* ed = dsd->getAsExprDetails();
	SymbolicExprs sED = getSymbolicExprs(ed, rv);
	if (rv && sED.resolved) {
	    bool ret = insertExprInSymMap(dsd, sED);
	    if (!ret) {
		error = true;
		return false;
	    }
	}
    } else {
	llvm::outs() << "ERROR: Did not get DeclStmtDetails\n";
	error = true;
	return false;
    }
#endif

    //if (isResolved(sd)) 
    resolved = sd->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on DeclStmt\n";
	error = true;
	return false;
    }
    if (resolved)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: DeclStmt already resolved. Skipping..\n";
	//sd->print();
	sd->prettyPrint();
	llvm::outs() << "DEBUG: SymExprs:\n";
	std::vector<SymbolicStmt*> symExprs = getSymbolicExprs(sd, rv);
	if (rv) {
	    for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin();
		    sIt != symExprs.end(); sIt++) {
		(*sIt)->prettyPrint(true);
	    }
	}
#endif
	return true;
    } else {
#ifdef DEBUG
	llvm::outs() << "DEBUG: DeclStmt not resolved\n";
#endif
    }

    // Find the var declared and check if it is resolved.
    if (isa<DeclStmtDetails>(sd)) {
	DeclStmtDetails* currDeclStmtDetails = dyn_cast<DeclStmtDetails>(sd);
	Decl* singleDecl = D->getSingleDecl();
	if (isa<VarDecl>(singleDecl)) {
	    VarDecl* vd = dyn_cast<VarDecl>(singleDecl);
	    VarDeclDetails* vdd = VarDeclDetails::getObject(SM, vd, currBlock,
				    rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot get VarDeclDetails from Decl in DeclStmt\n";
		error = true;
		return false;
	    }

	    //if (isResolved(vdd)) 
	    resolved = vdd->isSymExprCompletelyResolved(this, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on "
			     << "VarDecl\n";
		error = true;
		return false;
	    }
	    if (resolved)
	    {
#ifdef DEBUG
		llvm::outs() << "DEBUG: VarDecl in DeclStmt is resolved\n";
#endif
		std::vector<SymbolicStmt*> symExprs = getSymbolicExprs(vdd, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: Could not obtain symbolic exprs of VarDecl:\n";
		    vdd->prettyPrint();
		    error = true;
		    return false;
		}

		for (std::vector<SymbolicStmt*>::iterator sIt =
			symExprs.begin(); sIt != symExprs.end(); sIt++) {
		    if (!(*sIt)) {
			llvm::outs() << "ERROR: NULL SymExpr for DeclStmt\n";
			sd->prettyPrint();
			error = true;
			return false;
		    }
		    SymbolicDeclStmt* sds = new SymbolicDeclStmt;
		    //sds->guardBlocks.insert(sds->guardBlocks.end(), currGuards.begin(), 
			//currGuards.end());
		    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
			appendGuards(sds->guardBlocks, currGuards);
		    sds->guardBlocks.clear();
		    sds->guardBlocks.insert(sds->guardBlocks.end(),
			appendedGuards.begin(), appendedGuards.end());
		    //sds->guardBlocks.insert(sds->guardBlocks.end(), 
			//(*sIt)->guardBlocks.begin(), (*sIt)->guardBlocks.end());
		    appendedGuards = appendGuards(sds->guardBlocks, (*sIt)->guardBlocks);
		    sds->guardBlocks.clear();
		    sds->guardBlocks.insert(sds->guardBlocks.end(),
			appendedGuards.begin(), appendedGuards.end());
		    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendedGuardExprs =
			appendGuards(sds->guards, (*sIt)->guards, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While appending guardExprs\n";
			error = true;
			return false;
		    }
		    sds->guards.clear();
		    sds->guards.insert(sds->guards.end(),
			appendedGuardExprs.begin(), appendedGuardExprs.end());
		    if (isa<SymbolicVarDecl>(*sIt)) {
			sds->var = dyn_cast<SymbolicVarDecl>(*sIt);
			for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt = 
				(*sIt)->varsReferenced.begin(); vrIt != 
				(*sIt)->varsReferenced.end(); vrIt++) {
			    sds->varsReferenced.push_back(*vrIt);
			}
		    } else {
			llvm::outs() << "ERROR: Symbolic Expr for VarDecl:";
			vdd->prettyPrint();
			llvm::outs() << " is not SymbolicVarDecl\n";
			error = true;
			return false;
		    }
		    sds->resolved = true;
		    if (!insertExprInSymMap(currDeclStmtDetails, sds)) {
			llvm::outs() << "ERROR: While inserting symexpr for:\n";
			currDeclStmtDetails->prettyPrint();
			error = true;
			return false;
		    }
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Inserted symExpr for DeclStmt:\n";
		    sds->prettyPrint(true);
#endif
		}
		return true;
	    } else {
#ifdef DEBUG
		llvm::outs() << "DEBUG: VarDecl is not resolved\n";
#endif
	    }
	} else {
	    llvm::outs() << "ERROR: Declaration in DeclStmt at line " << lineNum 
			 << " and block " << currBlock << " is not VarDecl\n";
	    error = true;
	    return false;
	}
    } else {
	llvm::outs() << "ERROR: Did not get DeclStmtDetails\n";
	error = true;
	return false;
    }

    return true;
}

bool GetSymbolicExprVisitor::VisitVarDecl(VarDecl* V) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering GetSymbolicExprVisitor::VisitVarDecl()\n";
#endif
    bool resolved;
    bool rv;
    SourceLocation SL = SM->getExpansionLoc(V->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL))
	return true;

    VarDeclDetails* currVarDeclDetails = VarDeclDetails::getObject(SM, V,
	currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain VarDeclDetails\n";
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting VarDecl: ";
    currVarDeclDetails->var.print();
    llvm::outs() << "\n";
#endif

    //if (isResolved(currVarDeclDetails)) 
    resolved = currVarDeclDetails->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on VarDecl\n";
	error = true;
	return false;
    }
    if (resolved)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: VarDecl already resolved. Skipping..\n";
	currVarDeclDetails->prettyPrint();
	llvm::outs() << "DEBUG: SymExprs:\n";
#if 0
	std::vector<SymbolicStmt*> symExprs =
	    getSymbolicExprs(currVarDeclDetails, rv);
	if (rv) {
	    for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin();
		    sIt != symExprs.end(); sIt++) {
		(*sIt)->print();
	    }
	}
#endif
	printSymExprs(currVarDeclDetails);
#endif
	return true;
    }

    if (currVarDeclDetails->var.isArray()) {
	std::vector<SymbolicExpr*> sizeSymExprs;
	for (std::vector<const ArrayType*>::iterator tIt =
		currVarDeclDetails->var.arraySizeInfo.begin(); tIt != 
		currVarDeclDetails->var.arraySizeInfo.end(); tIt++) {
	    SymbolicExpr* se = getSymbolicExprForType(*tIt, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Something wrong while getting symExpr for size of "
			     << currVarDeclDetails->var.varName << "\n";
		error = true;
		return false;
	    } else {
		if (!se) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: SizeExpr is not resolved yet for var: ";
		    currVarDeclDetails->var.print();
#endif
		    return true;
		}
	    }
	    sizeSymExprs.push_back(se);
	}
	currVarDeclDetails->var.setArraySizeSymExprs(sizeSymExprs);

	bool foundVar = false;
	for (std::vector<std::pair<VarDetails, std::vector<SymbolicExpr*> >
		>::iterator vIt = varSizeSymExprMap.begin(); vIt != 
		varSizeSymExprMap.end(); vIt++) {
	    if (vIt->first.equals(currVarDeclDetails->var)) {
		if (sizeSymExprs.size() != vIt->second.size()) {
		    llvm::outs() << "ERROR: Duplicate entry found in "
				 << "varSizeSymExprMap for var " 
				 << currVarDeclDetails->var.varName
				 << "\n";
		    error = true;
		    return false;
		}
		for (unsigned i = 0; i < sizeSymExprs.size(); i++) {
		    if (!sizeSymExprs[i]->equals(vIt->second[i])) {
			llvm::outs() << "ERROR: Duplicate entry found in "
				     << "varSizeSymExprMap for var " 
				     << currVarDeclDetails->var.varName << "\n";
			error = true;
			return false;
		    }
		}
#ifdef DEBUG
		llvm::outs() << "DEBUG: Found duplicate entry in varSizeSymExprMap\n";
#endif
		foundVar = true;
		break;
	    } 
	}
	if (!foundVar) {
	    std::pair<VarDetails, std::vector<SymbolicExpr*> > p;
	    p.first = currVarDeclDetails->var;
	    p.second.insert(p.second.end(), sizeSymExprs.begin(),
		sizeSymExprs.end());
	    varSizeSymExprMap.push_back(p);
	}
    }

    if (V->hasInit()) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: VarDecl has Init\n";
#endif
	ExprDetails* initDetails = currVarDeclDetails->initExpr;
	//if (isResolved(initDetails)) 
	resolved = initDetails->isSymExprCompletelyResolved(this, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on initExpr\n";
	    error = true;
	    return false;
	}
	if (resolved)
	{
#ifdef DEBUG
	    llvm::outs() << "DEBUG: VarDecl init is resolved\n";
#endif
	    std::vector<SymbolicStmt*> initSymExprs =
		getSymbolicExprs(initDetails, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot find symbolic exprs of "
			     << "initExpr:\n";
		initDetails->prettyPrint();
		error = true;
		return false;
	    }

	    for (std::vector<SymbolicStmt*>::iterator iIt =
		    initSymExprs.begin(); iIt != initSymExprs.end(); 
		    iIt++) {
		SymbolicVarDecl* svd = new SymbolicVarDecl;
		//svd->guardBlocks.insert(svd->guardBlocks.end(), currGuards.begin(), 
		    //currGuards.end());
		std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
		    appendGuards(svd->guardBlocks, currGuards);
		svd->guardBlocks.clear();
		svd->guardBlocks.insert(svd->guardBlocks.end(),
		    appendedGuards.begin(), appendedGuards.end());
		//svd->guardBlocks.insert(svd->guardBlocks.end(), 
		    //(*iIt)->guardBlocks.begin(), (*iIt)->guardBlocks.end());
		appendedGuards = appendGuards(svd->guardBlocks, (*iIt)->guardBlocks);
		svd->guardBlocks.clear();
		svd->guardBlocks.insert(svd->guardBlocks.end(),
		    appendedGuards.begin(), appendedGuards.end());
		std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendedGuardExprs =
		    appendGuards(svd->guards, (*iIt)->guards, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While appending guardExprs\n";
		    error = true;
		    return false;
		}
		svd->guards.clear();
		svd->guards.insert(svd->guards.end(),
		    appendedGuardExprs.begin(), appendedGuardExprs.end());
		svd->varD = currVarDeclDetails->var;
		if (isa<SymbolicExpr>(*iIt)) {
		    svd->initExpr = dyn_cast<SymbolicExpr>(*iIt);
		    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt = 
			    (*iIt)->varsReferenced.begin(); vrIt != 
			    (*iIt)->varsReferenced.end(); vrIt++)
			svd->varsReferenced.push_back(*vrIt);
		} else {
		    llvm::outs() << "ERROR: Symbolic expr for init expr: ";
		    initDetails->prettyPrint();
		    llvm::outs() << " is not SymbolicExpr\n";
		    error = true;
		    return false;
		}
		svd->resolved = true;
		if (!insertExprInSymMap(currVarDeclDetails, svd)) {
		    llvm::outs() << "ERROR: While inserting symexpr for:\n";
		    currVarDeclDetails->prettyPrint();
		    error = true;
		    return false;
		}
	    }
	    return true;
	} else {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Init Expression is not yet resolved\n";
#endif
	    return true;
	}
    } else {
#ifdef DEBUG
	llvm::outs() << "DEBUG: VarDecl has no Init\n";
#endif
	SymbolicVarDecl* svd = new SymbolicVarDecl;
	//svd->guardBlocks.insert(svd->guardBlocks.end(), currGuards.begin(), 
	    //currGuards.end());
	std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
	    appendGuards(svd->guardBlocks, currGuards);
	svd->guardBlocks.clear();
	svd->guardBlocks.insert(svd->guardBlocks.end(),
	    appendedGuards.begin(), appendedGuards.end());
	svd->varD = currVarDeclDetails->var;
	svd->initExpr = NULL;
	svd->resolved = true;
	if (!insertExprInSymMap(currVarDeclDetails, svd)) {
	    llvm::outs() << "ERROR: While inserting symexpr for:\n";
	    currVarDeclDetails->prettyPrint();
	    error = true;
	    return false;
	}
#ifdef DEBUG
	llvm::outs() << "DEBUG: Inserted symexpr for vardecl without init\n";
	svd->prettyPrint(true);
	llvm::outs() << "\n";
#endif
	return true;
    }

    return true;
}

bool GetSymbolicExprVisitor::VisitInitListExpr(InitListExpr* I) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering GetSymbolicExprVisitor::VisitInitListExpr()\n";
#endif

    bool resolved;
    bool rv;
    SourceLocation SL = SM->getExpansionLoc(I->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: InitListExpr is not in main program\n";
	error = true;
	return false;
    }

    InitListExprDetails* currDetails = 
	InitListExprDetails::getObject(SM, I, currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain InitListExprDetails from "
		     << "InitListExpr\n";
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting InitListExpr:\n";
    currDetails->prettyPrint();
#endif

    //if (isResolved(currDetails)) 
    resolved = currDetails->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on InitListExpr\n";
	error = true;
	return false;
    }
    if (resolved)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: InitListExpr is already resolved. "
		     << "Skipping...\n";
#endif
	return true;
    }


    std::vector<std::vector<SymbolicExpr*> > allInitSymExprs;
    for (std::vector<ExprDetails*>::iterator aIt =
	    currDetails->inits.begin(); aIt != 
	    currDetails->inits.end(); aIt++) {
	ExprDetails* initExpr = *aIt;
	//if (!isResolved(initExpr)) return true;
	resolved = initExpr->isSymExprCompletelyResolved(this, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on initExpr\n";
	    error = true;
	    return false;
	}
	if (!resolved) return true;

	// Get symbolic expr of initExpr
	std::vector<SymbolicStmt*> symExprs = getSymbolicExprs(initExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Could not obtain symbolic exprs for initExpr:\n";
	    initExpr->prettyPrint();
	    error = true;
	    return false;
	}
    
	std::vector<std::vector<SymbolicExpr*> > currInitSymExprs;
	bool empty = (allInitSymExprs.size() == 0);
	for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); 
		sIt != symExprs.end(); sIt++) {
	    if (!isa<SymbolicExpr>(*sIt)) {
		llvm::outs() << "ERROR: Symbolic expr for argExpr is not <SymbolicExpr>\n";
		error = true;
		return false;
	    }

	    if (empty) {
		std::vector<SymbolicExpr*> currVec;
		currVec.push_back(dyn_cast<SymbolicExpr>(*sIt));
		currInitSymExprs.push_back(currVec);
	    } else {
		for (std::vector<std::vector<SymbolicExpr*> >::iterator aIt = 
			allInitSymExprs.begin(); aIt != allInitSymExprs.end();
			aIt++) {
		    std::vector<SymbolicExpr*> currVec;
		    currVec.insert(currVec.end(), aIt->begin(), aIt->end());
		    currVec.push_back(dyn_cast<SymbolicExpr>(*sIt));
		    currInitSymExprs.push_back(currVec);
		}
	    }
	}

	allInitSymExprs.clear();
	allInitSymExprs.insert(allInitSymExprs.end(), currInitSymExprs.begin(),
	    currInitSymExprs.end());
    }

    llvm::outs() << "DEBUG: allInitSymExprs: " << allInitSymExprs.size() << "\n";

    for (std::vector<std::vector<SymbolicExpr*> >::iterator aIt =
	    allInitSymExprs.begin(); aIt != allInitSymExprs.end(); aIt++) {
	SymbolicInitListExpr* sile = new SymbolicInitListExpr;
	sile->resolved = true;
	sile->hasArrayFiller = currDetails->hasArrayFiller;
	std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
	    appendGuards(sile->guardBlocks, currGuards);
	sile->guardBlocks.clear();
	sile->guardBlocks.insert(sile->guardBlocks.end(),
	    appendedGuards.begin(), appendedGuards.end());
	for (std::vector<SymbolicExpr*>::iterator it = aIt->begin(); it != 
		aIt->end(); it++) {
	    appendedGuards = appendGuards(sile->guardBlocks, (*it)->guardBlocks);
	    sile->guardBlocks.clear();
	    sile->guardBlocks.insert(sile->guardBlocks.end(),
		appendedGuards.begin(), appendedGuards.end());
	    sile->inits.push_back(*it);
	    for (std::vector<SymbolicDeclRefExpr*>::iterator initVRIt = 
		    (*it)->varsReferenced.begin(); initVRIt != 
		    (*it)->varsReferenced.end(); initVRIt++) {
		bool foundVR = false;
		for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt = 
			sile->varsReferenced.begin(); vrIt != 
			sile->varsReferenced.end(); vrIt++) {
		    if ((*initVRIt)->var.equals((*vrIt)->var)) {
			foundVR = true;
			break;
		    }
		}
		if (!foundVR)
		    sile->varsReferenced.push_back(*initVRIt);
	    }
	}
	if (currDetails->arrayFiller) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Has arrayFiller\n";
#endif
	    std::vector<SymbolicStmt*> afSymExprs;
	    //if (!isResolved(currDetails->arrayFiller)) return true;
	    resolved = currDetails->arrayFiller->isSymExprCompletelyResolved(this, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on arrayFiller\n";
		error = true;
		return false;
	    }
	    if (!resolved) return true;
	    afSymExprs = getSymbolicExprs(currDetails->arrayFiller, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot obtain symexprs for arrayFiller\n";
		error = true;
		return false;
	    }
	    for (std::vector<SymbolicStmt*>::iterator afIt =
		    afSymExprs.begin(); afIt != afSymExprs.end(); afIt++) {
		SymbolicStmt* cloneExpr = sile->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While cloning SymbolicInitListExpr\n";
		    error = true;
		    return false;
		}
		SymbolicInitListExpr* newSile =
		    dyn_cast<SymbolicInitListExpr>(cloneExpr);
		appendedGuards = appendGuards(newSile->guardBlocks,
		    (*afIt)->guardBlocks);
		newSile->guardBlocks.clear();
		newSile->guardBlocks.insert(newSile->guardBlocks.end(),
		    appendedGuards.begin(), appendedGuards.end());
		if (!isa<SymbolicExpr>(*afIt)) {
		    llvm::outs() << "ERROR: Symexpr of arrayFiller is not <SymbolicExpr>\n";
		    error = true;
		    return false;
		}
		newSile->arrayFiller = dyn_cast<SymbolicExpr>(*afIt);
		for (std::vector<SymbolicDeclRefExpr*>::iterator afVRIt = 
			(*afIt)->varsReferenced.begin(); afVRIt != 
			(*afIt)->varsReferenced.end(); afVRIt++) {
		    bool foundVR = false;
		    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt = 
			    newSile->varsReferenced.begin(); vrIt != 
			    newSile->varsReferenced.end(); vrIt++) {
			if ((*afVRIt)->var.equals((*vrIt)->var)) {
			    foundVR = true;
			    break;
			}
		    }
		    if (!foundVR)
			newSile->varsReferenced.push_back(*afVRIt);
		}
		if (!insertExprInSymMap(currDetails, newSile)) {
		    llvm::outs() << "ERROR: While inserting symexpr for:\n";
		    currDetails->prettyPrint();
		    error = true;
		    return false;
		}
#ifdef DEBUG
		llvm::outs() << "Inserted symexpr:\n";
		newSile->prettyPrint(true);
#endif
	    }
	} else {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: About to insert symexpr for:\n";
	    currDetails->prettyPrint();
	    sile->print();
#endif
	    if (!insertExprInSymMap(currDetails, sile)) {
		llvm::outs() << "ERROR: While inserting symexpr for:\n";
		currDetails->prettyPrint();
		error = true;
		return false;
	    }
#ifdef DEBUG
	    llvm::outs() << "Inserted symexpr:\n";
	    sile->prettyPrint(true);
#endif
	}
    }

    return true;
}

bool GetSymbolicExprVisitor::VisitCXXOperatorCallExpr(CXXOperatorCallExpr* C) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering "
		 << "GetSymbolicExprVisitor::VisitCXXOperatorCallExpr()\n";
#endif

    bool resolved;
    bool rv;
    SourceLocation SL = SM->getExpansionLoc(C->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: CXXOperatorCallExpr is not in main program\n";
	error = true;
	return false;
    }

    CXXOperatorCallExprDetails* currDetails =
	CXXOperatorCallExprDetails::getObject(SM, C, currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain CXXOperatorCallExprDetails from "
		     << "CXXOperatorCallExpr\n";
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting CXXOperatorCallExpr:\n";
    currDetails->prettyPrint();
#endif

    //if (isResolved(currDetails)) 
    resolved = currDetails->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on CXXOperatorCallExpr\n";
	error = true;
	return false;
    }
    if (resolved)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: CXXOperatorCallExpr is already resolved. "
		     << "Skipping...\n";
#endif
	return true;
    }

    // Get callee symexpr
    //if (!isResolved(currDetails->callee)) 
    resolved = currDetails->callee->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on callee\n";
	error = true;
	return false;
    }
    if (!resolved)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: callee not resolved\n";
#endif
	return true;
    }
    std::vector<SymbolicStmt*> calleeSymExprs =
	getSymbolicExprs(currDetails->callee, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain symExprs for callee: "
		     << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }
    if (calleeSymExprs.size() != 1) {
	llvm::outs() << "ERROR: size of symexprs for callee != 1\n";
	llvm::outs() << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }
    if (!isa<SymbolicExpr>(*(calleeSymExprs.begin()))) {
	llvm::outs() << "ERROR: symExpr for callee is not <SymbolicExpr>\n";
	llvm::outs() << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }
    SymbolicExpr* calleeSymExpr =
	dyn_cast<SymbolicExpr>(*(calleeSymExprs.begin()));

    std::vector<std::vector<SymbolicExpr*> > allArgSymExprs;
    for (std::vector<ExprDetails*>::iterator aIt =
	    currDetails->callArgExprs.begin(); aIt != 
	    currDetails->callArgExprs.end(); aIt++) {
	ExprDetails* argExpr = *aIt;
	//if (!isResolved(argExpr)) 
	resolved = argExpr->isSymExprCompletelyResolved(this, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on argExpr\n";
	    error = true;
	    return false;
	}
	if (!resolved)
	{
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Argexpr not resolved\n";
#endif
	    return true;
	}

	// Get symbolic expr of argExpr
	std::vector<SymbolicStmt*> symExprs = getSymbolicExprs(argExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Could not obtain symbolic exprs for argExpr:\n";
	    argExpr->prettyPrint();
	    error = true;
	    return false;
	}
    
	std::vector<std::vector<SymbolicExpr*> > currArgSymExprs;
	bool empty = (allArgSymExprs.size() == 0);
	for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); 
		sIt != symExprs.end(); sIt++) {
	    if (!isa<SymbolicExpr>(*sIt)) {
		llvm::outs() << "ERROR: Symbolic expr for argExpr is not <SymbolicExpr>\n";
		error = true;
		return false;
	    }

	    if (empty) {
		std::vector<SymbolicExpr*> currVec;
		currVec.push_back(dyn_cast<SymbolicExpr>(*sIt));
		currArgSymExprs.push_back(currVec);
	    } else {
		for (std::vector<std::vector<SymbolicExpr*> >::iterator aIt = 
			allArgSymExprs.begin(); aIt != allArgSymExprs.end();
			aIt++) {
		    std::vector<SymbolicExpr*> currVec;
		    currVec.insert(currVec.end(), aIt->begin(), aIt->end());
		    currVec.push_back(dyn_cast<SymbolicExpr>(*sIt));
		    currArgSymExprs.push_back(currVec);
		}
	    }
	}

	allArgSymExprs.clear();
	allArgSymExprs.insert(allArgSymExprs.end(), currArgSymExprs.begin(),
	    currArgSymExprs.end());
    }

    for (std::vector<std::vector<SymbolicExpr*> >::iterator aIt =
	    allArgSymExprs.begin(); aIt != allArgSymExprs.end(); aIt++) {
	SymbolicCXXOperatorCallExpr* sce = new SymbolicCXXOperatorCallExpr;
	sce->callee = calleeSymExpr;
	//sce->guardBlocks.insert(sce->guardBlocks.end(), currGuards.begin(), 
	    //currGuards.end());
	std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
	    appendGuards(sce->guardBlocks, currGuards);
	sce->guardBlocks.clear();
	sce->guardBlocks.insert(sce->guardBlocks.end(),
	    appendedGuards.begin(), appendedGuards.end());
	sce->opKind = currDetails->opKind;
	sce->resolved = true;
	for (std::vector<SymbolicExpr*>::iterator argIt = aIt->begin(); 
		argIt != aIt->end(); argIt++) {
	    //sce->guardBlocks.insert(sce->guardBlocks.end(), 
		//(*argIt)->guardBlocks.begin(), (*argIt)->guardBlocks.end());
	    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
		appendGuards(sce->guardBlocks, (*argIt)->guardBlocks);
	    sce->guardBlocks.clear();
	    sce->guardBlocks.insert(sce->guardBlocks.end(),
		appendedGuards.begin(), appendedGuards.end());
	    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendedGuardExprs =
		appendGuards(sce->guards, (*argIt)->guards, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While appending guardExprs\n";
		error = true;
		return false;
	    }
	    sce->guards.clear();
	    sce->guards.insert(sce->guards.end(),
		appendedGuardExprs.begin(), appendedGuardExprs.end());
	    sce->callArgExprs.push_back(*argIt);
	    for (std::vector<SymbolicDeclRefExpr*>::iterator argVRIt = 
		    (*argIt)->varsReferenced.begin(); argVRIt != 
		    (*argIt)->varsReferenced.end(); argVRIt++) {
		bool foundVR = false;
		for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt = 
			sce->varsReferenced.begin(); vrIt != 
			sce->varsReferenced.end(); vrIt++) {
		    if ((*argVRIt)->var.equals((*vrIt)->var)) {
			foundVR = true;
			break;
		    }
		}
		if (!foundVR)
		    sce->varsReferenced.push_back(*argVRIt);
	    }
	}

	if (!insertExprInSymMap(currDetails, sce)) {
	    llvm::outs() << "ERROR: While inserting symexpr for:\n";
	    currDetails->prettyPrint();
	    error = true;
	    return false;
	}
#ifdef DEBUG
	llvm::outs() << "DEBUG: Inserting symexpr for CXXOperator\n";
#endif
    }

    return true;
}

bool GetSymbolicExprVisitor::VisitUnaryOperator(UnaryOperator* U) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering GetSymbolicExprVisitor::VisitUnaryOperator()\n";
#endif

    bool resolved;
    bool rv;
    SourceLocation SL = SM->getExpansionLoc(U->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: UnaryOperator is not in main program\n";
	error = true;
	return false;
    }

    int lineNum = SM->getExpansionLineNumber(U->getLocStart());

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting UnaryOperator at line " << lineNum 
		 << " and block " << currBlock << ": "
		 << Helper::prettyPrintExpr(U) << "\n";
#endif

    UnaryOperatorDetails* currDetails = UnaryOperatorDetails::getObject(SM, U,
	currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get UnaryOperatorDetails of UnaryOperator\n";
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: UnaryOperatorDetails:\n";
    currDetails->prettyPrint();
#endif

    //if (isResolved(currDetails)) 
    resolved = currDetails->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on UnaryOperator\n";
	error = true;
	return false;
    }
    if (resolved)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: UnaryOperator is already resolved. Skipping..\n";
#endif
	return true;
    }

    // Check if the subExpr is resolved
    //if (isResolved(currDetails->opExpr)) 
    resolved = currDetails->opExpr->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved on opExpr\n";
	error = true;
	return false;
    }
    if (resolved)
    {
	std::vector<SymbolicStmt*> symExprs =
	    getSymbolicExprs(currDetails->opExpr, rv);
	if (!rv) {
	    // Not resolved
	    return true;
	}
	for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); 
		sIt != symExprs.end(); sIt++) {
	    if (!isa<SymbolicExpr>(*sIt)) {
		llvm::outs() << "ERROR: The symbolic expr for opExpr of "
			     << "UnaryOperator is not <SymbolicExpr>\n";
		error = true;
		return false;
	    }

	    bool updated = false;
	    if (currDetails->opKind == UnaryOperatorKind::UO_PreInc || 
		    currDetails->opKind == UnaryOperatorKind::UO_PostInc || 
		    currDetails->opKind == UnaryOperatorKind::UO_PreDec || 
		    currDetails->opKind == UnaryOperatorKind::UO_PostDec) {
		// Check if it is a loop index var
		bool isLoopIndex = false;
		if (isa<SymbolicDeclRefExpr>(*sIt)) {
		    SymbolicDeclRefExpr* sdre =
			dyn_cast<SymbolicDeclRefExpr>(*sIt);
		    if (sdre->vKind == SymbolicDeclRefExpr::LOOPINDEX)
			isLoopIndex = true;
		}

		if (!isLoopIndex) {
		    updated = true;
		    SymbolicBinaryOperator* sbo = new SymbolicBinaryOperator;
		    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
			appendGuards(sbo->guardBlocks, currGuards);
		    sbo->guardBlocks.clear();
		    sbo->guardBlocks.insert(sbo->guardBlocks.end(),
			appendedGuards.begin(), appendedGuards.end());
		    appendedGuards = appendGuards(sbo->guardBlocks, (*sIt)->guardBlocks);
		    sbo->guardBlocks.clear();
		    sbo->guardBlocks.insert(sbo->guardBlocks.end(),
			appendedGuards.begin(), appendedGuards.end());
		    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendedGuardExprs =
			appendGuards(sbo->guards, (*sIt)->guards, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While appending guardExprs\n";
			error = true;
			return false;
		    }
		    sbo->guards.clear();
		    sbo->guards.insert(sbo->guards.end(),
			appendedGuardExprs.begin(), appendedGuardExprs.end());
		    sbo->resolved = true;
		    if (currDetails->opKind == UnaryOperatorKind::UO_PostInc || 
			currDetails->opKind == UnaryOperatorKind::UO_PreInc)
			sbo->opKind = BinaryOperatorKind::BO_Add;
		    else if (currDetails->opKind == UnaryOperatorKind::UO_PostDec ||
			currDetails->opKind == UnaryOperatorKind::UO_PreDec)
			sbo->opKind = BinaryOperatorKind::BO_Sub;
		    sbo->lhs = dyn_cast<SymbolicExpr>(*sIt);

		    SymbolicIntegerLiteral* sil = new SymbolicIntegerLiteral;
		    llvm::APInt val(32, 1);
		    sil->val = val;
		    sbo->rhs = sil;

		    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
			    (*sIt)->varsReferenced.begin(); vrIt != 
			    (*sIt)->varsReferenced.end(); vrIt++)
			sbo->varsReferenced.push_back(*vrIt);
		    if (!insertExprInSymMap(currDetails, sbo)) {
			llvm::outs() << "ERROR: While inserting symexpr for:\n";
			currDetails->print();
			error = true;
			return false;
		    }
		}
	    }
	    if (!updated) {
		SymbolicUnaryOperator* sue = new SymbolicUnaryOperator;
		//sue->guardBlocks.insert(sue->guardBlocks.end(), currGuards.begin(), 
		    //currGuards.end());
		std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
		    appendGuards(sue->guardBlocks, currGuards);
		sue->guardBlocks.clear();
		sue->guardBlocks.insert(sue->guardBlocks.end(),
		    appendedGuards.begin(), appendedGuards.end());
		//sue->guardBlocks.insert(sue->guardBlocks.end(), 
		    //(*sIt)->guardBlocks.begin(), (*sIt)->guardBlocks.end());
		appendedGuards = appendGuards(sue->guardBlocks, (*sIt)->guardBlocks);
		sue->guardBlocks.clear();
		sue->guardBlocks.insert(sue->guardBlocks.end(),
		    appendedGuards.begin(), appendedGuards.end());
		std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendedGuardExprs =
		    appendGuards(sue->guards, (*sIt)->guards, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While appending guardExprs\n";
		    error = true;
		    return false;
		}
		sue->guards.clear();
		sue->guards.insert(sue->guards.end(),
		    appendedGuardExprs.begin(), appendedGuardExprs.end());
		sue->resolved = true;
		sue->opKind = currDetails->opKind;
		sue->opExpr = dyn_cast<SymbolicExpr>(*sIt);
		for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
			(*sIt)->varsReferenced.begin(); vrIt != 
			(*sIt)->varsReferenced.end(); vrIt++)
		    sue->varsReferenced.push_back(*vrIt);
		if (!insertExprInSymMap(currDetails, sue)) {
		    llvm::outs() << "ERROR: While inserting symexpr for:\n";
		    currDetails->prettyPrint();
		    error = true;
		    return false;
		}
	    }
	}
    }

    return true;
}

bool GetSymbolicExprVisitor::VisitIntegerLiteral(IntegerLiteral* I) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering GetSymbolicExprVisitor::VisitIntegerLiteral()\n";
#endif

    bool resolved;
    bool rv;
    SourceLocation SL = SM->getExpansionLoc(I->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: IntegerLiteral is not in main program\n";
	error = true;
	return false;
    }

    int lineNum = SM->getExpansionLineNumber(I->getLocStart());

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting IntegerLiteral at line " << lineNum 
		 << " and block " << currBlock << ": "
		 << Helper::prettyPrintExpr(I) << "\n";
#endif

    IntegerLiteralDetails* currDetails = IntegerLiteralDetails::getObject(SM, I,
	currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get IntegerLiteralDetails of IntegerLiteral\n";
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: IntegerLiteralDetails:\n";
    currDetails->prettyPrint();
#endif

    //if (isResolved(currDetails)) 
    resolved = currDetails->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on IntegerLiteral\n";
	error = true;
	return false;
    }
    if (resolved)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: IntegerLiteral is already resolved. Skipping..\n";
#endif
	return true;
    }

    SymbolicIntegerLiteral* sil = new SymbolicIntegerLiteral;
    //sil->guardBlocks.insert(sil->guardBlocks.end(), currGuards.begin(), 
	//currGuards.end());
    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
	appendGuards(sil->guardBlocks, currGuards);
    sil->guardBlocks.clear();
    sil->guardBlocks.insert(sil->guardBlocks.end(),
	appendedGuards.begin(), appendedGuards.end());
    sil->resolved = true;
    sil->val = I->getValue();
    if (!insertExprInSymMap(currDetails, sil)) {
	llvm::outs() << "ERROR: While inserting symexpr for:\n";
	currDetails->prettyPrint();
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Inserted symExpr for IntegerLiteral\n";
    currDetails->prettyPrint();
    llvm::outs() << "\n";
    sil->prettyPrint(true);
    llvm::outs() << "\n";
    std::vector<SymbolicStmt*> symExprs = getSymbolicExprs(currDetails, rv);
    if (!rv) {
	llvm::outs() << "DEBUG: Cannot find symbolicexprs for IntegerLiteral\n";
	error = true;
	return false;
    }
    llvm::outs() << "DEBUG: Total symExprs: " << symExprs.size() << "\n";
    for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); sIt != 
	    symExprs.end(); sIt++) {
	(*sIt)->prettyPrint(true);
	llvm::outs() << "\n";
    }
    llvm::outs() << "DEBUG: End SymExprs\n";
#endif
    return true;
}

bool GetSymbolicExprVisitor::VisitImaginaryLiteral(ImaginaryLiteral* I) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering GetSymbolicExprVisitor::VisitImaginaryLiteral()\n";
#endif

    bool resolved;
    bool rv;
    SourceLocation SL = SM->getExpansionLoc(I->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: ImaginaryLiteral is not in main program\n";
	error = true;
	return false;
    }

    int lineNum = SM->getExpansionLineNumber(I->getLocStart());

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting ImaginaryLiteral at line " << lineNum 
		 << " and block " << currBlock << ": "
		 << Helper::prettyPrintExpr(I) << "\n";
#endif

    ImaginaryLiteralDetails* currDetails = ImaginaryLiteralDetails::getObject(SM, I,
	currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get ImaginaryLiteralDetails of ImaginaryLiteral\n";
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: ImaginaryLiteralDetails:\n";
    currDetails->prettyPrint();
#endif

    //if (isResolved(currDetails)) 
    resolved = currDetails->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on ImaginaryLiteral\n";
	error = true;
	return false;
    }
    if (resolved)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: ImaginaryLiteral is already resolved. Skipping..\n";
#endif
	return true;
    }

    SymbolicIntegerLiteral* sil = new SymbolicIntegerLiteral;
    //sil->guardBlocks.insert(sil->guardBlocks.end(), currGuards.begin(), 
	//currGuards.end());
    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
	appendGuards(sil->guardBlocks, currGuards);
    sil->guardBlocks.clear();
    sil->guardBlocks.insert(sil->guardBlocks.end(),
	appendedGuards.begin(), appendedGuards.end());
    sil->resolved = true;
    sil->val = currDetails->val;
    if (!insertExprInSymMap(currDetails, sil)) {
	llvm::outs() << "ERROR: While inserting symexpr for:\n";
	currDetails->prettyPrint();
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Inserted symExpr for ImaginaryLiteral\n";
    currDetails->prettyPrint();
    llvm::outs() << "\n";
    sil->prettyPrint(true);
    llvm::outs() << "\n";
    std::vector<SymbolicStmt*> symExprs = getSymbolicExprs(currDetails, rv);
    if (!rv) {
	llvm::outs() << "DEBUG: Cannot find symbolicexprs for ImaginaryLiteral\n";
	error = true;
	return false;
    }
    llvm::outs() << "DEBUG: Total symExprs: " << symExprs.size() << "\n";
    for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); sIt != 
	    symExprs.end(); sIt++) {
	(*sIt)->prettyPrint(true);
	llvm::outs() << "\n";
    }
    llvm::outs() << "DEBUG: End SymExprs\n";
#endif
    return true;
}

bool GetSymbolicExprVisitor::VisitFloatingLiteral(FloatingLiteral* F) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering GetSymbolicExprVisitor::VisitFloatingLiteral()\n";
#endif

    bool resolved;
    bool rv;
    SourceLocation SL = SM->getExpansionLoc(F->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: FloatingLiteral is not in main program\n";
	error = true;
	return false;
    }

    int lineNum = SM->getExpansionLineNumber(F->getLocStart());

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting FloatingLiteral at line " << lineNum 
		 << " and block " << currBlock << ": "
		 << Helper::prettyPrintExpr(F) << "\n";
#endif

    FloatingLiteralDetails* currDetails = FloatingLiteralDetails::getObject(SM, F,
	currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get FloatingLiteralDetails of FloatingLiteral\n";
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: FloatingLiteralDetails:\n";
    currDetails->prettyPrint();
#endif

    //if (isResolved(currDetails)) 
    resolved = currDetails->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on FloatingLiteral\n";
	error = true;
	return false;
    }
    if (resolved)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: FloatingLiteral is already resolved. Skipping..\n";
#endif
	return true;
    }

    SymbolicFloatingLiteral* sil = new SymbolicFloatingLiteral;
    //sil->guardBlocks.insert(sil->guardBlocks.end(), currGuards.begin(), 
	//currGuards.end());
    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
	appendGuards(sil->guardBlocks, currGuards);
    sil->guardBlocks.clear();
    sil->guardBlocks.insert(sil->guardBlocks.end(),
	appendedGuards.begin(), appendedGuards.end());
    sil->resolved = true;
    sil->val = F->getValue();
    if (!insertExprInSymMap(currDetails, sil)) {
	llvm::outs() << "ERROR: While inserting symexpr for:\n";
	currDetails->prettyPrint();
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Inserted symExpr for FloatingLiteral\n";
    currDetails->prettyPrint();
    sil->prettyPrint(true);
    std::vector<SymbolicStmt*> symExprs = getSymbolicExprs(currDetails, rv);
    if (!rv) {
	llvm::outs() << "DEBUG: Cannot find symbolicexprs for FloatingLiteral\n";
	error = true;
	return false;
    }
    for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); sIt != 
	    symExprs.end(); sIt++)
	(*sIt)->prettyPrint(true);
    llvm::outs() << "DEBUG: End SymExprs\n";
#endif
    return true;
}

bool GetSymbolicExprVisitor::VisitCXXBoolLiteralExpr(CXXBoolLiteralExpr* B) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering GetSymbolicExprVisitor::VisitCXXBoolLiteralExpr()\n";
#endif

    bool resolved;
    bool rv;
    SourceLocation SL = SM->getExpansionLoc(B->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: CXXBoolLiteralExpr is not in main program\n";
	error = true;
	return false;
    }

    int lineNum = SM->getExpansionLineNumber(B->getLocStart());

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting CXXBoolLiteralExpr at line " << lineNum 
		 << " and block " << currBlock << ": "
		 << Helper::prettyPrintExpr(B) << "\n";
#endif

    CXXBoolLiteralExprDetails* currDetails = CXXBoolLiteralExprDetails::getObject(
	SM, B, currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get CXXBoolLiteralExprDetails of CXXBoolLiteralExpr\n";
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: CXXBoolLiteralExprDetails:\n";
    currDetails->prettyPrint();
#endif

    //if (isResolved(currDetails)) 
    resolved = currDetails->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on BoolLiteral\n";
	error = true;
	return false;
    }
    if (resolved)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: CXXBoolLiteralExpr is already resolved. Skipping..\n";
#endif
	return true;
    }

    SymbolicCXXBoolLiteralExpr* sbl = new SymbolicCXXBoolLiteralExpr;
    //sbl->guardBlocks.insert(sbl->guardBlocks.end(), currGuards.begin(), 
	//currGuards.end());
    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
	appendGuards(sbl->guardBlocks, currGuards);
    sbl->guardBlocks.clear();
    sbl->guardBlocks.insert(sbl->guardBlocks.end(),
	appendedGuards.begin(), appendedGuards.end());
    sbl->resolved = true;
    sbl->val = B->getValue();
    if (!insertExprInSymMap(currDetails, sbl)) {
	llvm::outs() << "ERROR: While inserting symexpr for:\n";
	currDetails->prettyPrint();
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Inserted symExpr for CXXBoolLiteralExpr\n";
    currDetails->prettyPrint();
    sbl->prettyPrint(true);
    std::vector<SymbolicStmt*> symExprs = getSymbolicExprs(currDetails, rv);
    if (!rv) {
	llvm::outs() << "DEBUG: Cannot find symbolicexprs for CXXBoolLiteralExpr\n";
	error = true;
	return false;
    }
    for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); sIt != 
	    symExprs.end(); sIt++)
	(*sIt)->prettyPrint(true);
    llvm::outs() << "DEBUG: End SymExprs\n";
#endif
    return true;
}

bool GetSymbolicExprVisitor::VisitCharacterLiteral(CharacterLiteral* S) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering GetSymbolicExprVisitor::VisitCharacterLiteral()\n";
#endif

    bool resolved;
    bool rv;
    SourceLocation SL = SM->getExpansionLoc(S->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: CharacterLiteral is not in main program\n";
	error = true;
	return false;
    }

    int lineNum = SM->getExpansionLineNumber(S->getLocStart());

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting CharacterLiteral at line " << lineNum 
		 << " and block " << currBlock << ": "
		 << Helper::prettyPrintExpr(S) << "\n";
#endif

    CharacterLiteralDetails* currDetails = CharacterLiteralDetails::getObject(SM, S,
	currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get CharacterLiteralDetails of CharacterLiteral\n";
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: CharacterLiteralDetails:\n";
    currDetails->prettyPrint();
#endif

    //if (isResolved(currDetails)) 
    resolved = currDetails->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletleyResolved() on CharLiteral\n";
	error = true;
	return false;
    }
    if (resolved)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: CharacterLiteral is already resolved. Skipping..\n";
#endif
	return true;
    }

    SymbolicCharacterLiteral* ssl = new SymbolicCharacterLiteral;
    //ssl->guardBlocks.insert(ssl->guardBlocks.end(), currGuards.begin(), 
	//currGuards.end());
    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
	appendGuards(ssl->guardBlocks, currGuards);
    ssl->guardBlocks.clear();
    ssl->guardBlocks.insert(ssl->guardBlocks.end(),
	appendedGuards.begin(), appendedGuards.end());
    ssl->resolved = true;
    ssl->val = S->getValue();
    if (!insertExprInSymMap(currDetails, ssl)) {
	llvm::outs() << "ERROR: While inserting symexpr for:\n";
	currDetails->prettyPrint();
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Inserted symExpr for CharacterLiteral\n";
    currDetails->prettyPrint();
    ssl->prettyPrint(true);
    std::vector<SymbolicStmt*> symExprs = getSymbolicExprs(currDetails, rv);
    if (!rv) {
	llvm::outs() << "DEBUG: Cannot find symbolicexprs for CharacterLiteral\n";
	error = true;
	return false;
    }
    for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); sIt != 
	    symExprs.end(); sIt++)
	(*sIt)->prettyPrint(true);
    llvm::outs() << "DEBUG: End SymExprs\n";
#endif
    return true;
}

bool GetSymbolicExprVisitor::VisitStringLiteral(StringLiteral* S) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering GetSymbolicExprVisitor::VisitStringLiteral()\n";
#endif

    bool resolved;
    bool rv;
    SourceLocation SL = SM->getExpansionLoc(S->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: StringLiteral is not in main program\n";
	error = true;
	return false;
    }

    int lineNum = SM->getExpansionLineNumber(S->getLocStart());

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting StringLiteral at line " << lineNum 
		 << " and block " << currBlock << ": "
		 << Helper::prettyPrintExpr(S) << "\n";
#endif

    StringLiteralDetails* currDetails = StringLiteralDetails::getObject(SM, S,
	currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get StringLiteralDetails of StringLiteral\n";
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: StringLiteralDetails:\n";
    currDetails->prettyPrint();
#endif

    //if (isResolved(currDetails)) 
    resolved = currDetails->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on StringLiteral\n";
	error = true;
	return false;
    }
    if (resolved)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: StringLiteral is already resolved. Skipping..\n";
#endif
	return true;
    }

    SymbolicStringLiteral* ssl = new SymbolicStringLiteral;
    //ssl->guardBlocks.insert(ssl->guardBlocks.end(), currGuards.begin(), 
	//currGuards.end());
    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
	appendGuards(ssl->guardBlocks, currGuards);
    ssl->guardBlocks.clear();
    ssl->guardBlocks.insert(ssl->guardBlocks.end(),
	appendedGuards.begin(), appendedGuards.end());
    ssl->resolved = true;
    ssl->val = S->getString();
    if (!insertExprInSymMap(currDetails, ssl)) {
	llvm::outs() << "ERROR: While inserting symexpr for:\n";
	currDetails->prettyPrint();
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Inserted symExpr for StringLiteral\n";
    currDetails->prettyPrint();
    ssl->prettyPrint(true);
    std::vector<SymbolicStmt*> symExprs = getSymbolicExprs(currDetails, rv);
    if (!rv) {
	llvm::outs() << "DEBUG: Cannot find symbolicexprs for StringLiteral\n";
	error = true;
	return false;
    }
    for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); sIt != 
	    symExprs.end(); sIt++)
	(*sIt)->prettyPrint(true);
    llvm::outs() << "DEBUG: End SymExprs\n";
#endif
    return true;
}

bool GetSymbolicExprVisitor::VisitBinaryOperator(BinaryOperator* B) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering "
		 << "GetSymbolicExprVisitor::VisitBinaryOperator()\n";
#endif

    bool resolved;
    bool rv;
    SourceLocation SL = SM->getExpansionLoc(B->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: BinaryOperator is not in main program\n";
	error = true;
	return false;
    }

    BinaryOperatorDetails* currDetails = BinaryOperatorDetails::getObject(SM, B,
	currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain BinaryOperatorDetails from "
		     << "BinaryOperator\n";
	llvm::outs() << Helper::prettyPrintExpr(B) << "\n";
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting BinaryOperator:\n";
    currDetails->prettyPrint();
#endif

    //if (isResolved(currDetails)) 
    resolved = currDetails->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on BinaryOperator\n";
	error = true;
	return false;
    }
    if (resolved)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: BinaryOperator is already resolved. "
		     << "Skipping..\n";
#endif
	return true;
    }

    if (B->isCompoundAssignmentOp()) {
	llvm::outs() << "ERROR: BinaryOperator is compound assignment op. We "
		     << "are not handling it now\n";
	llvm::outs() << Helper::prettyPrintExpr(B) << "\n";
	error = true;
	return false;
    }

    // Check if the lhs and rhs are resolved
    //if (!isResolved(currDetails->lhs) || !isResolved(currDetails->rhs)) 
    resolved = currDetails->lhs->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved on lhs\n";
	error = true;
	return false;
    }
    bool resolved1 = currDetails->rhs->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved on rhs\n";
	error = true;
	return false;
    }
    if (!resolved || !resolved1)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: lhs or rhs of BinaryOperator is not resolved\n";
#endif
	return true;
    }

    std::vector<SymbolicStmt*> lhsSymExprs = getSymbolicExprs(currDetails->lhs,
	rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain symbolic exprs for lhs of "
		     << "BinaryOperator\n";
	error = true;
	return false;
    }

    std::vector<SymbolicStmt*> rhsSymExprs = getSymbolicExprs(currDetails->rhs,
	rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain symbolic exprs for rhs of "
		     << "BinaryOperator\n";
	error = true;
	return false;
    }

    for (std::vector<SymbolicStmt*>::iterator lIt = lhsSymExprs.begin(); lIt != 
	    lhsSymExprs.end(); lIt++) {
	if (!isa<SymbolicExpr>(*lIt)) {
	    llvm::outs() << "ERROR: Symbolic expr for lhs of BinaryOperator is "
			 << "not <SymbolicExpr>\n";
	    error = true;
	    return false;
	}
	for (std::vector<SymbolicStmt*>::iterator rIt = rhsSymExprs.begin(); 
		rIt != rhsSymExprs.end(); rIt++) {
	    if (!isa<SymbolicExpr>(*rIt)) {
		llvm::outs() << "ERROR: Symbolic expr for rhs of BinaryOperator "
			     << "is not <SymbolicExpr>\n";
		error = true;
		return false;
	    }
	    SymbolicBinaryOperator* sbo = new SymbolicBinaryOperator;
	    sbo->resolved = true;
	    //sbo->guardBlocks.insert(sbo->guardBlocks.end(), currGuards.begin(), 
		//currGuards.end());
	    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
		appendGuards(sbo->guardBlocks, currGuards);
	    sbo->guardBlocks.clear();
	    sbo->guardBlocks.insert(sbo->guardBlocks.end(),
		appendedGuards.begin(), appendedGuards.end());
	    //sbo->guardBlocks.insert(sbo->guardBlocks.end(), 
		//(*lIt)->guardBlocks.begin(), (*lIt)->guardBlocks.end());
	    appendedGuards = appendGuards(sbo->guardBlocks, (*lIt)->guardBlocks);
	    sbo->guardBlocks.clear();
	    sbo->guardBlocks.insert(sbo->guardBlocks.end(),
		appendedGuards.begin(), appendedGuards.end());
	    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendedGuardExprs =
		appendGuards(sbo->guards, (*lIt)->guards, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While appending guardExprs\n";
		error = true;
		return false;
	    }
	    sbo->guards.clear();
	    sbo->guards.insert(sbo->guards.end(),
		appendedGuardExprs.begin(), appendedGuardExprs.end());
	    //sbo->guardBlocks.insert(sbo->guardBlocks.end(), 
		//(*rIt)->guardBlocks.begin(), (*rIt)->guardBlocks.end());
	    appendedGuards = appendGuards(sbo->guardBlocks, (*rIt)->guardBlocks);
	    sbo->guardBlocks.clear();
	    sbo->guardBlocks.insert(sbo->guardBlocks.end(),
		appendedGuards.begin(), appendedGuards.end());
	    appendedGuardExprs =
		appendGuards(sbo->guards, (*rIt)->guards, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While appending guardExprs\n";
		error = true;
		return false;
	    }
	    sbo->guards.clear();
	    sbo->guards.insert(sbo->guards.end(),
		appendedGuardExprs.begin(), appendedGuardExprs.end());
	    sbo->opKind = currDetails->opKind;
	    sbo->lhs = dyn_cast<SymbolicExpr>(*lIt);
	    sbo->rhs = dyn_cast<SymbolicExpr>(*rIt);
	    for (std::vector<SymbolicDeclRefExpr*>::iterator lhsVRIt = 
		    (*lIt)->varsReferenced.begin(); lhsVRIt != 
		    (*lIt)->varsReferenced.end(); lhsVRIt++) {
		bool foundVR = false;
		for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
			sbo->varsReferenced.begin(); vrIt != 
			sbo->varsReferenced.end(); vrIt++) {
		    if ((*lhsVRIt)->var.equals((*vrIt)->var)) {
			foundVR = true;
			break;
		    }
		}
		if (!foundVR)
		    sbo->varsReferenced.push_back(*lhsVRIt);
	    }
	    for (std::vector<SymbolicDeclRefExpr*>::iterator rhsVRIt = 
		    (*rIt)->varsReferenced.begin(); rhsVRIt != 
		    (*rIt)->varsReferenced.end(); rhsVRIt++) {
		bool foundVR = false;
		for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
			sbo->varsReferenced.begin(); vrIt != 
			sbo->varsReferenced.end(); vrIt++) {
		    if ((*rhsVRIt)->var.equals((*vrIt)->var)) {
			foundVR = true;
			break;
		    }
		}
		if (!foundVR)
		    sbo->varsReferenced.push_back(*rhsVRIt);
	    }
	    if (!insertExprInSymMap(currDetails, sbo)) {
		llvm::outs() << "ERROR: While inserting symexpr for:\n";
		currDetails->prettyPrint();
		error = true;
		return false;
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Inserted symExpr for binaryOperator\n";
#endif
	}
    }
    return true;
}

bool GetSymbolicExprVisitor::VisitConditionalOperator(ConditionalOperator* B) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering "
		 << "GetSymbolicExprVisitor::VisitConditionalOperator()\n";
#endif

    bool resolved1, resolved2, resolved3;
    bool rv;
    SourceLocation SL = SM->getExpansionLoc(B->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: ConditionalOperator is not in main program\n";
	error = true;
	return false;
    }

    ConditionalOperatorDetails* currDetails = ConditionalOperatorDetails::getObject(SM, B,
	currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain ConditionalOperatorDetails from "
		     << "ConditionalOperator\n";
	llvm::outs() << Helper::prettyPrintExpr(B) << "\n";
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting ConditionalOperator:\n";
    currDetails->prettyPrint();
#endif

    //if (isResolved(currDetails)) 
    resolved1 = currDetails->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on ConditionalOperator\n";
	error = true;
	return false;
    }
    if (resolved1)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: ConditionalOperator is already resolved. "
		     << "Skipping..\n";
#endif
	return true;
    }

    // Check if the condition, trueExpr and falseExpr are resolved
#if 0
    if (!isResolved(currDetails->condition) || 
	!isResolved(currDetails->trueExpr)  ||
	!isResolved(currDetails->falseExpr))
#endif
    resolved1 = currDetails->condition->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on condition\n";
	error = true;
	return false;
    }
    resolved2 = currDetails->trueExpr->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on trueExpr\n";
	error = true;
	return false;
    }
    resolved3 = currDetails->falseExpr->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on falseExpr\n";
	error = true;
	return false;
    }
    if (!resolved1 || !resolved2 || !resolved3)
	return true;

    std::vector<SymbolicStmt*> conditionSymExprs = getSymbolicExprs(currDetails->condition,
	rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain symbolic exprs for condition of "
		     << "ConditionalOperator\n";
	error = true;
	return false;
    }

    std::vector<SymbolicStmt*> trueExprSymExprs = getSymbolicExprs(currDetails->trueExpr,
	rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain symbolic exprs for trueExpr of "
		     << "ConditionalOperator\n";
	error = true;
	return false;
    }

    std::vector<SymbolicStmt*> falseExprSymExprs = getSymbolicExprs(currDetails->falseExpr,
	rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain symbolic exprs for falseExpr of "
		     << "ConditionalOperator\n";
	error = true;
	return false;
    }

    for (std::vector<SymbolicStmt*>::iterator cIt = conditionSymExprs.begin(); cIt != 
	    conditionSymExprs.end(); cIt++) {
	if (!isa<SymbolicExpr>(*cIt)) {
	    llvm::outs() << "ERROR: Symbolic expr for condition of ConditionalOperator is "
			 << "not <SymbolicExpr>\n";
	    error = true;
	    return false;
	}
	for (std::vector<SymbolicStmt*>::iterator tIt = trueExprSymExprs.begin(); 
		tIt != trueExprSymExprs.end(); tIt++) {
	    if (!isa<SymbolicExpr>(*tIt)) {
		llvm::outs() << "ERROR: Symbolic expr for rhs of ConditionalOperator "
			     << "is not <SymbolicExpr>\n";
		error = true;
		return false;
	    }
	    for (std::vector<SymbolicStmt*>::iterator fIt = falseExprSymExprs.begin();
	            fIt != falseExprSymExprs.end(); fIt++) {
	    	SymbolicConditionalOperator* sbo = new SymbolicConditionalOperator;
	    	sbo->resolved = true;
	    	//sbo->guardBlocks.insert(sbo->guardBlocks.end(), currGuards.begin(), 
		    //currGuards.end());
		std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
		    appendGuards(sbo->guardBlocks, currGuards);
		sbo->guardBlocks.clear();
		sbo->guardBlocks.insert(sbo->guardBlocks.end(),
		    appendedGuards.begin(), appendedGuards.end());
	    	//sbo->guardBlocks.insert(sbo->guardBlocks.end(), 
		    //(*cIt)->guardBlocks.begin(), (*cIt)->guardBlocks.end());
		appendedGuards = appendGuards(sbo->guardBlocks, (*cIt)->guardBlocks);
		sbo->guardBlocks.clear();
		sbo->guardBlocks.insert(sbo->guardBlocks.end(),
		    appendedGuards.begin(), appendedGuards.end());
		std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendedGuardExprs =
		    appendGuards(sbo->guards, (*cIt)->guards, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While appending guardExprs\n";
		    error = true;
		    return false;
		}
		sbo->guards.clear();
		sbo->guards.insert(sbo->guards.end(),
		    appendedGuardExprs.begin(), appendedGuardExprs.end());
	    	//sbo->guardBlocks.insert(sbo->guardBlocks.end(), 
		    //(*tIt)->guardBlocks.begin(), (*tIt)->guardBlocks.end());
		appendedGuards = appendGuards(sbo->guardBlocks, (*tIt)->guardBlocks);
		sbo->guardBlocks.clear();
		sbo->guardBlocks.insert(sbo->guardBlocks.end(),
		    appendedGuards.begin(), appendedGuards.end());
		appendedGuardExprs =
		    appendGuards(sbo->guards, (*tIt)->guards, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While appending guardExprs\n";
		    error = true;
		    return false;
		}
		sbo->guards.clear();
		sbo->guards.insert(sbo->guards.end(),
		    appendedGuardExprs.begin(), appendedGuardExprs.end());
		appendedGuards =
		appendGuards(sbo->guardBlocks, (*fIt)->guardBlocks);
		sbo->guardBlocks.clear();
		sbo->guardBlocks.insert(sbo->guardBlocks.end(),
		    appendedGuards.begin(), appendedGuards.end());
		appendedGuardExprs =
		    appendGuards(sbo->guards, (*fIt)->guards, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While appending guardExprs\n";
		    error = true;
		    return false;
		}
		sbo->guards.clear();
		sbo->guards.insert(sbo->guards.end(),
		    appendedGuardExprs.begin(), appendedGuardExprs.end());

	    	sbo->condition = dyn_cast<SymbolicExpr>(*cIt);
	    	sbo->trueExpr = dyn_cast<SymbolicExpr>(*tIt);
	    	sbo->falseExpr = dyn_cast<SymbolicExpr>(*fIt);
	    	for (std::vector<SymbolicDeclRefExpr*>::iterator conditionVRIt = 
		    	(*cIt)->varsReferenced.begin(); conditionVRIt != 
		    	(*cIt)->varsReferenced.end(); conditionVRIt++) {
		    bool foundVR = false;
		    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
			    sbo->varsReferenced.begin(); vrIt != 
			    sbo->varsReferenced.end(); vrIt++) {
		        if ((*conditionVRIt)->var.equals((*vrIt)->var)) {
			    foundVR = true;
			    break;
		        }
		    }
		    if (!foundVR)
		        sbo->varsReferenced.push_back(*conditionVRIt);
	        }
	        for (std::vector<SymbolicDeclRefExpr*>::iterator trueVRIt = 
		        (*tIt)->varsReferenced.begin(); trueVRIt != 
		        (*tIt)->varsReferenced.end(); trueVRIt++) {
		    bool foundVR = false;
		    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
			    sbo->varsReferenced.begin(); vrIt != 
			    sbo->varsReferenced.end(); vrIt++) {
		        if ((*trueVRIt)->var.equals((*vrIt)->var)) {
			    foundVR = true;
			    break;
		        }
		    }
		    if (!foundVR)
		        sbo->varsReferenced.push_back(*trueVRIt);
	        }
	        for (std::vector<SymbolicDeclRefExpr*>::iterator falseVRIt = 
		        (*fIt)->varsReferenced.begin(); falseVRIt != 
		        (*fIt)->varsReferenced.end(); falseVRIt++) {
		    bool foundVR = false;
		    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt =
			    sbo->varsReferenced.begin(); vrIt != 
			    sbo->varsReferenced.end(); vrIt++) {
		        if ((*falseVRIt)->var.equals((*vrIt)->var)) {
			    foundVR = true;
			    break;
		        }
		    }
		    if (!foundVR)
		        sbo->varsReferenced.push_back(*falseVRIt);
	        }
	        if (!insertExprInSymMap(currDetails, sbo)) {
		    llvm::outs() << "ERROR: While inserting symexpr for:\n";
		    currDetails->prettyPrint();
		    error = true;
		    return false;
	        }
	    }
	}
    }
    return true;
}

bool GetSymbolicExprVisitor::VisitReturnStmt(ReturnStmt* R) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering "
		 << "GetSymbolicExprVisitor::VisitReturnStmt()\n";
#endif

    bool resolved;
    bool rv;
    SourceLocation SL = SM->getExpansionLoc(R->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: ReturnStmt is not in main program\n";
	error = true;
	return false;
    }

    ReturnStmtDetails* currDetails = ReturnStmtDetails::getObject(SM, R,
    currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain ReturnStmtDetails from "
		     << "ReturnStmt\n";
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting ReturnStmt:\n";
    currDetails->prettyPrint();
#endif

    //if (isResolved(currDetails)) 
    resolved = currDetails->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on returnstmt\n";
	error = true;
	return false;
    }
    if (resolved)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: The return stmt is already resolved. "
		     << "Skipping..\n";
#endif
	return true;
    }

    // Check if return expression exists. If not, the expression is resolved.
    if (!currDetails->retExpr) {
	SymbolicReturnStmt* rd = new SymbolicReturnStmt;
	rd->resolved = true;
	if (!insertExprInSymMap(currDetails, rd)) {
	    llvm::outs() << "ERROR: While inserting symexpr for:\n";
	    currDetails->prettyPrint();
	    error = true;
	    return false;
	}
	return true;
    } else {
	// Check if return expression is resolved
	//if (!isResolved(currDetails->retExpr)) return true;
	resolved = currDetails->retExpr->isSymExprCompletelyResolved(this, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on retExpr\n";
	    error = true;
	    return false;
	}
	if (!resolved) return true;

	std::vector<SymbolicStmt*> symExprs =
	    getSymbolicExprs(currDetails->retExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot obtain symexprs for:\n";
	    currDetails->retExpr->prettyPrint();
	    error = true;
	    return false;
	}

	for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); 
		sIt != symExprs.end(); sIt++) {
	    if (!isa<SymbolicExpr>(*sIt)) {
		llvm::outs() << "ERROR: Symexpr for return expression is not "
			     << "<SymbolicExpr>\n";
		error = true;
		return false;
	    }

	    SymbolicReturnStmt* rd = new SymbolicReturnStmt;
	    rd->resolved = true;
	    //rd->guardBlocks.insert(rd->guardBlocks.end(), currGuards.begin(), 
		//currGuards.end());
	    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
		appendGuards(rd->guardBlocks, currGuards);
	    rd->guardBlocks.clear();
	    rd->guardBlocks.insert(rd->guardBlocks.end(),
		appendedGuards.begin(), appendedGuards.end());
	    //rd->guardBlocks.insert(rd->guardBlocks.end(), (*sIt)->guardBlocks.begin(),
		//(*sIt)->guardBlocks.end());
	    appendedGuards = appendGuards(rd->guardBlocks, (*sIt)->guardBlocks);
	    rd->guardBlocks.clear();
	    rd->guardBlocks.insert(rd->guardBlocks.end(),
		appendedGuards.begin(), appendedGuards.end());
	    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendedGuardExprs =
		appendGuards(rd->guards, (*sIt)->guards, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While appending guardExprs\n";
		error = true;
		return false;
	    }
	    rd->guards.clear();
	    rd->guards.insert(rd->guards.end(),
		appendedGuardExprs.begin(), appendedGuardExprs.end());
	    rd->retExpr = dyn_cast<SymbolicExpr>(*sIt);
	    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt = 
		    (*sIt)->varsReferenced.begin(); vrIt != 
		    (*sIt)->varsReferenced.end(); vrIt++)
		rd->varsReferenced.push_back(*vrIt);
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Found retExpr:\n";
	    rd->retExpr->prettyPrint(true);
#endif
	    if (!insertExprInSymMap(currDetails, rd)) {
		llvm::outs() << "ERROR: While inserting symexpr for:\n";
		currDetails->prettyPrint();
		error = true;
		return false;
	    }
	}

	return true;
    }

    return true;
}

bool GetSymbolicExprVisitor::VisitCallExpr(CallExpr* C) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering GetSymbolicExprVisitor::VisitCallExpr(): "
		 << Helper::prettyPrintExpr(C) << "\n";
#endif

    if (isa<CXXOperatorCallExpr>(C)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: CallExpr is CXXOperatorCallExpr. Not handling "
		     << "here..\n";
#endif
	return true;
    }

    bool resolved;
    bool rv;
    
    SourceLocation SL = SM->getExpansionLoc(C->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: CallExpr is not in main program\n";
	error = true;
	return false;
    }

    FunctionDecl* calleeFD = C->getDirectCallee();
    if (!calleeFD) {
	llvm::outs() << "ERROR: Cannot obtain direct callee of callexpr\n";
	error = true;
	return false;
    }

    calleeFD = calleeFD->getMostRecentDecl();
    if (!calleeFD) {
	llvm::outs() << "ERROR: Cannot obtain most recent decl of callee\n";
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "calleeFD:\n";
    calleeFD->print(llvm::outs());
#endif
    
    // Check if the callee is in main program. If not, fail.
    // TODO: We can model some library functions like std::max(), rather than 
    // failing here.
    std::string calleeName = calleeFD->getNameInfo().getAsString();
    if (calleeName.compare("exit") == 0 && calleeFD->isExternC()) return true;
    SL = SM->getExpansionLoc(calleeFD->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: callee not in main prog\n";
#endif
	if (calleeName.compare("scanf") != 0 && calleeName.compare("printf") != 0 &&
	    calleeName.compare("max") != 0 && calleeName.compare("std::max") != 0 &&
	    calleeName.compare("min") != 0 && calleeName.compare("std::min") != 0 &&
	    calleeName.compare("memset") != 0 && calleeName.compare("putchar") != 0 &&
	    calleeName.compare("getchar") != 0 && calleeName.compare("exit") != 0 && 
	    calleeName.compare("puts") != 0 && calleeName.compare("fmax") != 0) {
	    //llvm::outs() << "ERROR: Callee of callExpr is not in main program: ";
	    //llvm::outs() << Helper::prettyPrintExpr(C) << "\n";
	    //error = true;
	    //return false;
	    llvm::outs() << "WARNING: Callee of callExpr is not in main program: ";
	    llvm::outs() << Helper::prettyPrintExpr(C) << "\n";
	    llvm::outs() << "Adding stmt to residual\n";
	    CallExprDetails* currDetails = CallExprDetails::getObject(SM, C, currBlock, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot get CallExprDetails of "
			     << Helper::prettyPrintExpr(C) << "\n";
		error = true;
		return false;
	    }
	    bool stmtExists = false;
	    for (std::vector<StmtDetails*>::iterator sIt = residualStmts.begin();
		    sIt != residualStmts.end(); sIt++) {
		if (currDetails->equals(*sIt)) {
		    stmtExists = true;
		    break;
		}
	    }
	    if (!stmtExists) {
		residualStmts.push_back(currDetails);
#ifdef DEBUG
		llvm::outs() << "DEBUG: Added stmt to residual:\n";
		currDetails->print();
#endif
	    }
	    return true;
	}
    }

    CallExprDetails* currDetails = CallExprDetails::getObject(SM, C, currBlock,
	rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get CallExprDetails of "
		     << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }
    //if (isResolved(currDetails)) 
    resolved = currDetails->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on callExpr\n";
	error = true;
	return false;
    }
    if (resolved)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: CallExpr is already resolved. Skipping..\n";
#endif
	return true;
    }

    // Get callee symexpr
    //if (!isResolved(currDetails->callee)) return true;
    resolved = currDetails->callee->isSymExprCompletelyResolved(this, rv);
    if (!resolved) return true;
    std::vector<SymbolicStmt*> calleeSymExprs =
	getSymbolicExprs(currDetails->callee, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain symExprs for callee: "
		     << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }
    if (calleeSymExprs.size() != 1) {
	llvm::outs() << "ERROR: size of symexprs for callee != 1\n";
	llvm::outs() << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }
    if (!isa<SymbolicExpr>(*(calleeSymExprs.begin()))) {
	llvm::outs() << "ERROR: symExpr for callee is not <SymbolicExpr>\n";
	llvm::outs() << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }

    SymbolicExpr* calleeSymExpr =
	dyn_cast<SymbolicExpr>(*(calleeSymExprs.begin()));

    std::vector<std::vector<SymbolicExpr*> > allArgSymExprs;
    for (std::vector<ExprDetails*>::iterator aIt =
	    currDetails->callArgExprs.begin(); aIt != 
	    currDetails->callArgExprs.end(); aIt++) {
	ExprDetails* argExpr = *aIt;
	//if (!isResolved(argExpr)) return true;
	resolved = argExpr->isSymExprCompletelyResolved(this, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on argExpr\n";
	    error = true;
	    return false;
	}
	if (!resolved) return true;

	// Get symbolic expr of argExpr
	std::vector<SymbolicStmt*> symExprs = getSymbolicExprs(argExpr, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Could not obtain symbolic exprs for argExpr:\n";
	    //argExpr->print();
	    error = true;
	    return false;
	}
    
	std::vector<std::vector<SymbolicExpr*> > currArgSymExprs;
	bool empty = (allArgSymExprs.size() == 0);
	for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); 
		sIt != symExprs.end(); sIt++) {
	    if (!isa<SymbolicExpr>(*sIt)) {
		llvm::outs() << "ERROR: Symbolic expr for argExpr is not <SymbolicExpr>\n";
		error = true;
		return false;
	    }

	    if (empty) {
		std::vector<SymbolicExpr*> currVec;
		currVec.push_back(dyn_cast<SymbolicExpr>(*sIt));
		currArgSymExprs.push_back(currVec);
	    } else {
		for (std::vector<std::vector<SymbolicExpr*> >::iterator aIt = 
			allArgSymExprs.begin(); aIt != allArgSymExprs.end();
			aIt++) {
		    std::vector<SymbolicExpr*> currVec;
		    currVec.insert(currVec.end(), aIt->begin(), aIt->end());
		    currVec.push_back(dyn_cast<SymbolicExpr>(*sIt));
		    currArgSymExprs.push_back(currVec);
		}
	    }
	}

	allArgSymExprs.clear();
	allArgSymExprs.insert(allArgSymExprs.end(), currArgSymExprs.begin(),
	    currArgSymExprs.end());
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: allArgSymExprs of " << Helper::prettyPrintExpr(C)
		 << " :\n";
    for (std::vector<std::vector<SymbolicExpr*> >::iterator aIt =
	    allArgSymExprs.begin(); aIt != allArgSymExprs.end(); aIt++) {
	for (std::vector<SymbolicExpr*>::iterator argIt = aIt->begin();
		argIt != aIt->end(); argIt++) {
	    //(*argIt)->print();
	    (*argIt)->prettyPrint(true);
	    llvm::outs() << ", ";
	}

	llvm::outs() << "\n=========\n";
    }
#endif

    FunctionDetails* calleeDetails = Helper::getFunctionDetailsFromDecl(SM,
	calleeFD, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain FunctionDetails of function: "
		     << Helper::prettyPrintExpr(C) << "\n";
	error = true;
	return false;
    }

    // Get the full FunctionDetails object so as to obtain the set of return
    // stmts in the function
    for (std::vector<FunctionDetails*>::iterator fIt = FA->allFunctions.begin();
	    fIt != FA->allFunctions.end(); fIt++) {
	if ((*fIt)->equals(calleeDetails)) {
	    calleeDetails = *fIt;
	    break;
	}
    }

    FunctionAnalysis *calleeAnalysisObj;
    // Get FunctionAnalysis object for the callee function
#ifdef DEBUG
    llvm::outs() << "DEBUG: callee: ";
    calleeDetails->print();
    llvm::outs() << "\n";
#endif
    bool foundAnalysisObj = false;
    for (std::vector<FunctionAnalysis*>::iterator aIt =
	    mainObj->analysisInfo.begin(); aIt != 
	    mainObj->analysisInfo.end(); aIt++) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: analysisInfo: ";
	(*aIt)->function->print();
	llvm::outs() << "\n";
#endif
	if ((*aIt)->function->equals(calleeDetails)) {
	    calleeAnalysisObj = *aIt;
	    foundAnalysisObj = true;
	    break;
	}
    }

    if (!foundAnalysisObj && !(calleeDetails->isLibraryFunction) &&
	    !(calleeDetails->isCustomInputFunction) &&
	    !(calleeDetails->isCustomOutputFunction)) {
	llvm::outs() << "ERROR: Could not obtain the FuntionAnalysis object "
		     << "for function: " << calleeDetails->funcName << "\n";
	error = true;
	return false;
    }

    if (allArgSymExprs.size() == 0) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: CallExpr has no arguments.\n";
#endif
	SymbolicCallExpr* sce = new SymbolicCallExpr;
	sce->callee = calleeSymExpr;
	//sce->guardBlocks.insert(sce->guardBlocks.end(), currGuards.begin(), 
	    //currGuards.end());
	std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
	    appendGuards(sce->guardBlocks, currGuards);
	sce->guardBlocks.clear();
	sce->guardBlocks.insert(sce->guardBlocks.end(),
	    appendedGuards.begin(), appendedGuards.end());
	sce->resolved = true;
	if (!(calleeDetails->isLibraryFunction) &&
	    !(calleeDetails->isCustomInputFunction) && 
	    !(calleeDetails->isCustomOutputFunction)) {
	  for (std::vector<SymbolicExpr*>::iterator retIt =
		calleeAnalysisObj->retSymExprs.begin(); retIt != 
		calleeAnalysisObj->retSymExprs.end(); retIt++) {
	    // I cannot add the guardBlocks of callee function here, since
	    // callee function has a distinct CFG and the blockIDs do not make
	    // sense in the current function. Rather, I should add the
	    // guardExprs here and call replaceVarsWithExprs to replace formal
	    // args with actual args.
	    //sce->guardBlocks.insert((*retIt)->guardBlocks.begin(),
		//(*retIt)->guardBlocks.end());
	    SymbolicStmt* cloneExpr = (*retIt)->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot obtain clone of "
			     << "retExpr:\n";
		(*retIt)->prettyPrint(true);
		error = true;
		return false;
	    }
	    if (!isa<SymbolicExpr>(cloneExpr)) {
		llvm::outs() << "ERROR: Clone of SymbolicExpr is not "
			     << "<SymbolicExpr>\n";
		error = true;
		return false;
	    }
	    SymbolicExpr* retExpr = dyn_cast<SymbolicExpr>(cloneExpr);
	    // Erase the guard blocks in the return Expr
	    retExpr->guardBlocks.clear();
	    retExpr->replaceVarsWithExprs(calleeDetails->parameters, 
		sce->callArgExprs, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While replacing vars in returnExpr\n";
		retExpr->prettyPrint(true);
		error = true;
		return false;
	    }

	    SymbolicStmt* cloneCallStmt = sce->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While cloning SymbolicCallExpr\n";
		sce->prettyPrint(true);
		error = true;
		return false;
	    }
	    if (!isa<SymbolicCallExpr>(cloneCallStmt)) {
		llvm::outs() << "ERROR: Clone of SymbolicCallExpr is not "
			     << "<SymbolicCallExpr>\n";
		error = true;
		return false;
	    }
	    SymbolicCallExpr* cloneSCE =
		dyn_cast<SymbolicCallExpr>(cloneCallStmt);
	    cloneSCE->returnExpr = retExpr;
	    // Copy guards of retExpr to the callExpr.
	    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
		    >::iterator gIt = retExpr->guards.begin(); gIt !=
		    retExpr->guards.end(); gIt++) {
		std::vector<std::pair<SymbolicExpr*, bool> > gVec;
		for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator vIt
			= gIt->begin(); vIt != gIt->end(); vIt++) {
		    if (!(vIt->first)) {
			llvm::outs() << "ERROR: NULL guardExpr\n";
			error = true;
			return false;
		    }
		    SymbolicStmt* cloneGuardExpr = vIt->first->clone(rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While cloning guardExpr\n";
			vIt->first->prettyPrint(true);
			error = true;
			return false;
		    }
		    if (!isa<SymbolicExpr>(cloneGuardExpr)) {
			llvm::outs() << "ERROR: Clone of guardExpr is not "
				     << "<SymbolicExpr>\n";
			error = true;
			return false;
		    }
		    cloneGuardExpr->replaceVarsWithExprs(calleeDetails->parameters,
			cloneSCE->callArgExprs, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While replacing vars in "
				     << "guardExpr\n";
			error = true;
			return false;
		    }
		    gVec.push_back(std::make_pair(
			dyn_cast<SymbolicExpr>(cloneGuardExpr), vIt->second));
		}
		cloneSCE->guards.push_back(gVec);
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: currDetails:\n";
	    currDetails->prettyPrint();
	    llvm::outs() << "\n";
	    llvm::outs() << "DEBUG: sce:\n";
	    cloneSCE->prettyPrint(true);
	    llvm::outs() << "\n";
#endif
	    if (!insertExprInSymMap(currDetails, cloneSCE)) {
		llvm::outs() << "ERROR: While inserting symexpr for:\n";
		currDetails->prettyPrint();
		error = true;
		return false;
	    }
	  }
	  return true;
	}
	if (!insertExprInSymMap(currDetails, sce)) {
	    llvm::outs() << "ERROR: While inserting symexpr for:\n";
	    currDetails->prettyPrint();
	    error = true;
	    return false;
	}
	return true;
    }

    for (std::vector<std::vector<SymbolicExpr*> >::iterator aIt =
	    allArgSymExprs.begin(); aIt != allArgSymExprs.end(); aIt++) {
	SymbolicCallExpr* sce = new SymbolicCallExpr;
	sce->callee = calleeSymExpr;
	//sce->guardBlocks.insert(sce->guardBlocks.end(), currGuards.begin(), 
	    //currGuards.end());
	std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
	    appendGuards(sce->guardBlocks, currGuards);
	sce->guardBlocks.clear();
	sce->guardBlocks.insert(sce->guardBlocks.end(),
	    appendedGuards.begin(), appendedGuards.end());
	sce->resolved = true;
	for (std::vector<SymbolicExpr*>::iterator argIt = aIt->begin(); 
		argIt != aIt->end(); argIt++) {
	    //sce->guardBlocks.insert(sce->guardBlocks.end(), 
		//(*argIt)->guardBlocks.begin(),(*argIt)->guardBlocks.end());
	    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
		appendGuards(sce->guardBlocks, (*argIt)->guardBlocks);
	    sce->guardBlocks.clear();
	    sce->guardBlocks.insert(sce->guardBlocks.end(),
		appendedGuards.begin(), appendedGuards.end());
	    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendedGuardExprs =
		appendGuards(sce->guards, (*argIt)->guards, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While appending guardExprs\n";
		error = true;
		return false;
	    }
	    sce->guards.clear();
	    sce->guards.insert(sce->guards.end(),
		appendedGuardExprs.begin(), appendedGuardExprs.end());
	    sce->callArgExprs.push_back(*argIt);

	    // Update varsReferenced
	    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt = 
		    (*argIt)->varsReferenced.begin(); vrIt != 
		    (*argIt)->varsReferenced.end(); vrIt++)
		sce->varsReferenced.push_back(*vrIt);
	}
#ifdef DEBUG
	llvm::outs() << "DEBUG: sce: ";
	sce->prettyPrint(true);
	llvm::outs() << "\n";
#endif

	if (!(calleeDetails->isLibraryFunction) &&
	    !(calleeDetails->isCustomInputFunction) && 
	    !(calleeDetails->isCustomOutputFunction)) {
	  if (calleeAnalysisObj->retSymExprs.size() == 0) {
	    if (!insertExprInSymMap(currDetails, sce)) {
		llvm::outs() << "ERROR: While inserting symexpr for:\n";
		error = true;
		return false;
	    }
	    //return true;
	  } else {
	    for (std::vector<SymbolicExpr*>::iterator retIt =
		    calleeAnalysisObj->retSymExprs.begin(); retIt != 
		    calleeAnalysisObj->retSymExprs.end(); retIt++) {
		// I cannot add the guardBlocks of callee function here, since
		// callee function has a distinct CFG and the blockIDs do not make
		// sense in the current function. Rather, I should add the
		// guardExprs here and call replaceVarsWithExprs to replace formal
		// args with actual args.
		//sce->guardBlocks.insert((*retIt)->guardBlocks.begin(),
		    //(*retIt)->guardBlocks.end());
#ifdef DEBUG
		llvm::outs() << "DEBUG: Looking at retExpr\n";
		(*retIt)->prettyPrint(true);
		llvm::outs() << "\n";
#endif
		SymbolicStmt* cloneExpr = (*retIt)->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: Cannot obtain clone of "
				 << "retExpr:\n";
		    (*retIt)->prettyPrint(true);
		    error = true;
		    return false;
		}
		if (!isa<SymbolicExpr>(cloneExpr)) {
		    llvm::outs() << "ERROR: Clone of SymbolicExpr is not "
				 << "<SymbolicExpr>\n";
		    error = true;
		    return false;
		}
		SymbolicExpr* retExpr = dyn_cast<SymbolicExpr>(cloneExpr);
		// Erase the guard blocks in the return Expr
		retExpr->guardBlocks.clear();
		// I do not have to replace guards. replaceVarsWithExprs() will do
		// that.
#ifdef DEBUG
		llvm::outs() << "DEBUG: Before replacing vars in retExpr:\n";
		retExpr->prettyPrint(true);
		llvm::outs() << "\n";
		llvm::outs() << "Parameters:\n";
		for (std::vector<VarDetails>::iterator pIt =
			calleeDetails->parameters.begin(); pIt != 
			calleeDetails->parameters.end(); pIt++) {
		    pIt->print(); llvm::outs() << "\n";
		}
		llvm::outs() << "ArgExprs:\n";
		for (std::vector<SymbolicExpr*>::iterator argIt =
			sce->callArgExprs.begin(); argIt != 
			sce->callArgExprs.end(); argIt++) {
		    (*argIt)->prettyPrint(true); llvm::outs() << "\n";
		}
#endif
		retExpr->replaceVarsWithExprs(calleeDetails->parameters, 
		    sce->callArgExprs, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While replacing vars in returnExpr\n";
		    retExpr->prettyPrint(true);
		    error = true;
		    return false;
		}
#ifdef DEBUG
		llvm::outs() << "DEBUG: After replacing vars in retExpr:\n";
		retExpr->prettyPrint(true);
		llvm::outs() << "\n";
#endif

		//sce->returnExprs.push_back(retExpr);
		SymbolicStmt* cloneCallStmt = sce->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While cloning SymbolicCallExpr\n";
		    sce->prettyPrint(true);
		    error = true;
		    return false;
		}
		if (!isa<SymbolicCallExpr>(cloneCallStmt)) {
		    llvm::outs() << "ERROR: Clone of SymbolicCallExpr is not "
				 << "<SymbolicCallExpr>\n";
		    error = true;
		    return false;
		}
		SymbolicCallExpr* cloneSCE =
		    dyn_cast<SymbolicCallExpr>(cloneCallStmt);
		cloneSCE->returnExpr = retExpr;
		// Copy guards of retExpr to the callExpr.
		for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
			>::iterator gIt = retExpr->guards.begin(); gIt !=
			retExpr->guards.end(); gIt++) {
		    std::vector<std::pair<SymbolicExpr*, bool> > gVec;
		    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator vIt
			    = gIt->begin(); vIt != gIt->end(); vIt++) {
			if (!(vIt->first)) {
			    llvm::outs() << "ERROR: NULL guardExpr\n";
			    error = true;
			    return false;
			}
			SymbolicStmt* cloneGuardExpr = vIt->first->clone(rv);
			if (!rv) {
			    llvm::outs() << "ERROR: While cloning guardExpr\n";
			    vIt->first->prettyPrint(true);
			    error = true;
			    return false;
			}
			if (!isa<SymbolicExpr>(cloneGuardExpr)) {
			    llvm::outs() << "ERROR: Clone of guardExpr is not "
					 << "<SymbolicExpr>\n";
			    error = true;
			    return false;
			}
			cloneGuardExpr->replaceVarsWithExprs(calleeDetails->parameters,
			    sce->callArgExprs, rv);
			if (!rv) {
			    llvm::outs() << "ERROR: While replacing vars in "
					 << "guardExpr\n";
			    error = true;
			    return false;
			}
			gVec.push_back(std::make_pair(
			    dyn_cast<SymbolicExpr>(cloneGuardExpr), vIt->second));
		    }
		    cloneSCE->guards.push_back(gVec);
		}

		// Update varsReferenced
		for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt = 
			retExpr->varsReferenced.begin(); vrIt != 
			retExpr->varsReferenced.end(); vrIt++)
		    cloneSCE->varsReferenced.push_back(*vrIt);
#ifdef DEBUG
		llvm::outs() << "DEBUG: currDetails:\n";
		currDetails->prettyPrint();
		llvm::outs() << "\n";
		llvm::outs() << "DEBUG: cloneSCE:\n";
		cloneSCE->prettyPrint(true);
		llvm::outs() << "\n!!\n";
		sce->printVarRefs();
#endif
		if (!insertExprInSymMap(currDetails, cloneSCE)) {
		    llvm::outs() << "ERROR: While inserting symexpr for:\n";
		    error = true;
		    return false;
		}
	    }
	    //return true;
	  }
	} else {

	    if (!insertExprInSymMap(currDetails, sce)) {
		llvm::outs() << "ERROR: While inserting symexpr for:\n";
		currDetails->prettyPrint();
		error = true;
		return false;
	    }
	}
#ifdef DEBUG
	llvm::outs() << "DEBUG: currDetails:\n";
	currDetails->prettyPrint();
	llvm::outs() << "DEBUG: sce:\n";
	sce->prettyPrint(true);
#endif
    }
    
    return true;
}

bool
GetSymbolicExprVisitor::VisitUnaryExprOrTypeTraitExpr(
UnaryExprOrTypeTraitExpr* U) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering "
		 << "GetSymbolicExprVisitor::VisitUnaryExprOrTypeTraitExpr()\n";
#endif

    bool resolved;
    bool rv;
    SourceLocation SL = SM->getExpansionLoc(U->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	llvm::outs() << "ERROR: UnaryExprOrTypeTraitExpr is not in main program\n";
	error = true;
	return false;
    }

    UnaryExprOrTypeTraitExprDetails* currDetails = UnaryExprOrTypeTraitExprDetails::getObject(SM, U,
    currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain UnaryExprOrTypeTraitExprDetails from "
		     << "UnaryExprOrTypeTraitExpr\n";
	error = true;
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Visiting UnaryExprOrTypeTraitExpr:\n";
    currDetails->prettyPrint();
#endif

    //if (isResolved(currDetails)) 
    resolved = currDetails->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on UnaryExprOrTypeTraitExpr\n";
	error = true;
	return false;
    }
    if (resolved)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: The UETT is already resolved. "
		     << "Skipping..\n";
#endif
	return true;
    }

    //if (!isResolved(currDetails->array)) 
    resolved = currDetails->array->isSymExprCompletelyResolved(this, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on array\n";
	error = true;
	return false;
    }
    if (!resolved)
    {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Array of UETT is not resolved..\n";
#endif
	return true;
    }

    if (currDetails->array->rKind != VAR) {
	llvm::outs() << "ERROR: Array of UETT is not VAR kind\n";
	error = true;
	return false;
    }

    if (currDetails->array->var.arraySizeInfo.size() == 0) {
	llvm::outs() << "ERROR: Array of UETT does not have sizeInfo\n";
	error = true;
	return false;
    }

    SymbolicUnaryExprOrTypeTraitExpr* suett = new SymbolicUnaryExprOrTypeTraitExpr;
    suett->resolved = true;
    std::vector<std::vector<std::pair<unsigned, bool> > > appendedGuards =
	appendGuards(suett->guardBlocks, currGuards);
    suett->guardBlocks.clear();
    suett->guardBlocks.insert(suett->guardBlocks.end(),
	appendedGuards.begin(), appendedGuards.end());
    suett->kind = currDetails->kind;
    std::vector<SymbolicStmt*> arrSymExprs = getSymbolicExprs(currDetails->array, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot obtain symexprs of array\n";
	error = true;
	return false;
    }
    if (arrSymExprs.size() > 1) {
	llvm::outs() << "ERROR: array of UETT has more than one symexprs\n";
	error = true;
	return false;
    }
    if (!isa<SymbolicDeclRefExpr>(*(arrSymExprs.begin()))) {
	llvm::outs() << "ERROR: Symexpr for array of UETT is not <SymbolicDeclRefExpr>\n";
	error = true;
	return false;
    }
    SymbolicDeclRefExpr* arr =
	dyn_cast<SymbolicDeclRefExpr>(*(arrSymExprs.begin()));
    if (arr->guardBlocks.size() != 0 || arr->guards.size() != 0) {
	llvm::outs() << "ERROR: Symexpr for array of UETT has guards\n";
	error = true;
	return false;
    }
    suett->array = arr;
    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt = 
	    arr->varsReferenced.begin(); vrIt != 
	    arr->varsReferenced.end(); vrIt++)
	suett->varsReferenced.push_back(*vrIt);

    for (std::vector<const ArrayType*>::iterator tIt =
	    currDetails->array->var.arraySizeInfo.begin(); tIt != 
	    currDetails->array->var.arraySizeInfo.end(); tIt++) {
	if (isa<ConstantArrayType>(*tIt)) {
	    const llvm::APInt size =
		dyn_cast<ConstantArrayType>(*tIt)->getSize();
	    SymbolicIntegerLiteral* sil = new SymbolicIntegerLiteral;
	    sil->val = size;
	    suett->sizeExprs.push_back(sil);
	} else if (isa<VariableArrayType>(*tIt)) {
	    Expr* sizeExpr = dyn_cast<VariableArrayType>(*tIt)->getSizeExpr();
	    if (!sizeExpr) {
		llvm::outs() << "ERROR: NULL sizeExpr\n";
		error = true;
		return false;
	    }
	    ExprDetails* sizeExprDetails = ExprDetails::getObject(SM, sizeExpr,
		currBlock, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot obtain ExprDetails of sizeExpr\n";
		error = true;
		return false;
	    }
	    //if (!isResolved(sizeExprDetails)) 
	    resolved = sizeExprDetails->isSymExprCompletelyResolved(this, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on sizeExpr\n";
		error = true;
		return false;
	    }
	    if (!resolved)
	    {
#ifdef DEBUG
		llvm::outs() << "DEBUG: sizeExpr is not resolved\n";
#endif
		return true;
	    }
	    std::vector<SymbolicStmt*> symExprs = getSymbolicExprs(sizeExprDetails, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot get symbolicExprs for sizeExpr\n";
		error = true;
		return false;
	    }
	    if (symExprs.size() > 1) {
		llvm::outs() << "ERROR: SizeExpr has more than one symExpr\n";
		error = true;
		return false;
	    }
	    if (!isa<SymbolicExpr>(*(symExprs.begin()))) {
		llvm::outs() << "ERROR: SymExpr of sizeExpr is not <SymbolicExpr>\n";
		error = true;
		return false;
	    }
	    SymbolicExpr* se = dyn_cast<SymbolicExpr>(*(symExprs.begin()));
	    // It should not have any guards either
	    if (se->guardBlocks.size() != 0 || se->guards.size() != 0) {
		llvm::outs() << "ERROR: SymExpr of sizeExpr has guards\n";
		error = true;
		return false;
	    }
	    suett->sizeExprs.push_back(se);
	    for (std::vector<SymbolicDeclRefExpr*>::iterator vrIt = 
		    se->varsReferenced.begin(); vrIt != 
		    se->varsReferenced.end(); vrIt++)
		suett->varsReferenced.push_back(*vrIt);
	} else {
	    llvm::outs() << "ERROR: Unsupported array type\n";
	    error = true;
	    return false;
	}
    }

    if (!insertExprInSymMap(currDetails, suett)) {
	llvm::outs() << "ERROR: While inserting symexpr for uett\n";
	error = true;
	return false;
    }
#ifdef DEBUG
    llvm::outs() << "DEBUG: Inserted symexpr for uett\n";
#endif

    return true;
}

void GetSymbolicExprVisitor::getGuardExprs(bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering GetSymbolicExprVisitor::getGuardExprs()\n";
#endif
    bool resolved;
    rv = true;
    for (std::vector<std::pair<StmtDetails*, std::vector<SymbolicStmt*> >
	    >::iterator mIt = symExprMap.begin(); mIt != symExprMap.end();
	    mIt++) {
	std::vector<SymbolicStmt*> currSymExprs = mIt->second;
	std::vector<std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > >
	    allGuardExprs;
	std::vector<SymbolicStmt*> newSymExprs;
	for (std::vector<SymbolicStmt*>::iterator sIt = currSymExprs.begin(); 
		sIt != currSymExprs.end(); sIt++) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Looking at SymbolicStmt:\n";
	    if (*sIt) {
		if (isa<SymbolicExpr>(*sIt)) {
		    dyn_cast<SymbolicExpr>((*sIt))->prettyPrint(true);
		    llvm::outs() << "\n";
		} else {
		    (*sIt)->prettyPrint(true);
		}
	    } else {
		llvm::outs() << "NULL\n";
	    }
#endif
	    allGuardExprs.clear();
	    for (std::vector<std::vector<std::pair<unsigned, bool> > >::iterator gIt =
		    (*sIt)->guardBlocks.begin(); gIt !=
		    (*sIt)->guardBlocks.end(); gIt++) {

		//std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > allVecs;
		std::vector<std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > >
		holdGuards;
		
		for (std::vector<std::pair<unsigned, bool> >::iterator vIt = 
			gIt->begin(); vIt != gIt->end(); vIt++) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Finding terminator expression of block "
				 << vIt->first << "\n";
#endif
		    ExprDetails* guardExpr =
			FA->getTerminatorExpressionOfBlock(vIt->first, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: Cannot obtain terminator expression "
				     << "of block " << vIt->first << "\n";
			return;
		    }
		    if (!guardExpr) {
			llvm::outs() << "ERROR: NULL guardExpr obtained for "
				     << "terminator expression of block " 
				     << vIt->first << "\n";
			rv = false;
			return;
		    }

		    resolved = guardExpr->isSymExprCompletelyResolved(this, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on guardExpr\n";
			return;
		    }
		    if (!resolved)
		    {
#ifdef DEBUG
			llvm::outs() << "DEBUG: Guard Expr is not resolved.\n";
#endif
			if (!guardExpr->origExpr) {
			    llvm::outs() << "ERROR: guardExpr has NULL origExpr\n";
			    rv = false;
			    return;
			}
			int counter=0;
			unsigned oldBlock = currBlock;
			setCurrBlock(vIt->first);
			clearCurrGuards();
			while (true) {
			    //if (counter >= 25 || isResolved(guardExpr))
			    resolved = guardExpr->isSymExprCompletelyResolved(this, rv);
			    if (!rv) {
				llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on guardExpr\n";
				return;
			    }
			    if (counter >= 25 || resolved) {
				if (!resolved) {
				    llvm::outs() << "WARNING: Breaking AST traversal of guardExpr, counter >= 25\n";
				}
				break;
			    }
			    counter++;
			    TraverseStmt(guardExpr->origExpr);
			    if (error) {
				llvm::outs() << "ERROR: While calling GetSymbolicExprVisitor on "
					    << Helper::prettyPrintExpr(guardExpr->origExpr)
					    << "\n";
				rv = false;
				return;
			    }
			}
			//if (!isResolved(guardExpr)) 
			resolved = guardExpr->isSymExprCompletelyResolved(this, rv);
			if (!rv) {
			    llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on guardExpr\n";
			    return;
			}
			if (!resolved)
			{
			    llvm::outs() << "ERROR: Cannot resolve guardExpr: "
					 << Helper::prettyPrintExpr(guardExpr->origExpr) 
					 << "\n";
			}
			setCurrBlock(oldBlock);
		    } else {
#ifdef DEBUG
			llvm::outs() << "DEBUG: guardExpr is resolved\n";
#endif
		    }

		    std::vector<SymbolicStmt*> symExprs =
			getSymbolicExprs(guardExpr, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: Cannot obtain symexprs for guardExpr\n";
			guardExpr->prettyPrint();
			return;
		    }

#ifdef DEBUG
		    llvm::outs() << "DEBUG: Symbolic exprs of guardExpr:\n";
		    printSymExprs(guardExpr);
#endif

		    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
		    currGuardExprs;
		    //bool empty = (allVecs.size() == 0);
		    bool emptyHoldGuards = (holdGuards.size() == 0);
		    std::vector<std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > >
		    currHoldGuards;

		    for (std::vector<SymbolicStmt*>::iterator gsIt =
			    symExprs.begin(); gsIt != symExprs.end(); gsIt++) {
			if (!(*gsIt)) {
			    llvm::outs() << "ERROR: NULL SymbolicExpr for guard: ";
			    guardExpr->prettyPrint();
			    llvm::outs() << "\n";
			    rv = false;
			    return;
			}
			if (!isa<SymbolicExpr>(*gsIt)) {
			    llvm::outs() << "ERROR: Symexpr for guard expr is not "
					 << "<SymbolicExpr>\n";
			    rv = false;
			    return;
			}

#if 0
			if (empty) {
			    std::vector<std::pair<SymbolicExpr*, bool> > currVec;
			    currVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(*gsIt),
				vIt->second));
			    currGuardExprs.push_back(currVec);
			} else {
			    for (std::vector<std::vector<std::pair<SymbolicExpr*,
				    bool> > >::iterator allIt = allVecs.begin();
				    allIt != allVecs.end(); allIt++) {
				std::vector<std::pair<SymbolicExpr*, bool> > currVec;
				currVec.insert(currVec.end(), allIt->begin(),
				    allIt->end());
				currVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(*gsIt), 
				    vIt->second));
				currGuardExprs.push_back(currVec);
			    }
			}
#if 0
			// Append guards of the guardExpr to currGuardExprs
			currGuardExprs = appendGuards(currGuardExprs,
			    (*gsIt)->guards, rv);
			if (!rv) {
			    llvm::outs() << "ERROR: While appending guards\n";
			    return;
			}
#endif
		    }

		    allVecs.clear();
		    allVecs.insert(allVecs.end(),
			currGuardExprs.begin(), currGuardExprs.end());
#endif
			std::vector<std::pair<SymbolicExpr*, bool> > currVec;
			currVec.push_back(std::make_pair(dyn_cast<SymbolicExpr>(*gsIt),
			    vIt->second));
			std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
			currVecVec;
			currVecVec.push_back(currVec);
			std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
			currGuardExprs = appendGuards((*gsIt)->guards, currVecVec, rv);
			if (!rv) {
			    llvm::outs() << "ERROR: While appending guards\n";
			    return;
			}
			if (emptyHoldGuards) {
			    currHoldGuards.push_back(currGuardExprs);
			} else {
			    for (std::vector<std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > 
				    >::iterator hIt = holdGuards.begin(); hIt !=
				    holdGuards.end(); hIt++) {
				std::vector<std::vector<std::pair<SymbolicExpr*,
				    bool> > > temp = appendGuards(currGuardExprs, *hIt, rv);
				if (!rv) {
				    llvm::outs() << "ERROR: While appending guards\n";
				    return;
				}
				currHoldGuards.push_back(temp);
			    }
			}
		    }
		    holdGuards.clear();
		    holdGuards.insert(holdGuards.end(), currHoldGuards.begin(),
			currHoldGuards.end());
		}
#if 0
#ifdef DEBUG
		llvm::outs() << "DEBUG: allVecs:\n";
		prettyPrintTheseGuardExprs(allVecs);
		llvm::outs() << "\n";
#endif

		bool empty = (allGuardExprs.size() == 0);
		std::vector<std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > >
		tempAllGuardExprs;
		if (empty) {
#ifdef DEBUG
		    llvm::outs() << "empty case\n";
#endif
		    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool>
			    > >::iterator aIt = allVecs.begin(); aIt !=
			    allVecs.end(); aIt++) {
			std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > currVec;
			currVec.push_back(*aIt);
			tempAllGuardExprs.push_back(currVec);
		    }
		} else {
#ifdef DEBUG
		    llvm::outs() << "non empty case\n";
#endif
		    for (std::vector<std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
			    >::iterator allIt = allGuardExprs.begin(); allIt !=
			    allGuardExprs.end(); allIt++) {
			for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
				aIt = allVecs.begin(); aIt != allVecs.end();
				aIt++) {
			    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > currVec;
			    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator 
				    tIt = allIt->begin(); tIt != allIt->end(); tIt++) {
				//currVec.insert(currVec.end(), tIt->begin(), tIt->end());
				std::vector<std::pair<SymbolicExpr*, bool> > vec1;
				for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator
					pIt = tIt->begin(); pIt != tIt->end();
					pIt++) {
				    vec1.push_back(*pIt);
				}
				currVec.push_back(vec1);
			    }
			    currVec.push_back(*aIt);
			    tempAllGuardExprs.push_back(currVec);
			}
		    }
		}
#ifdef DEBUG
		llvm::outs() << "DEBUG: tempAllGuardExprs:\n";
		for (std::vector<std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > 
			>::iterator tIt = tempAllGuardExprs.begin(); tIt !=
			tempAllGuardExprs.end(); tIt++) {
		    llvm::outs() << "guard::\n";
		    prettyPrintTheseGuardExprs(*tIt);
		    llvm::outs() << "\n";
		}
#endif

		allGuardExprs.clear();
		allGuardExprs.insert(allGuardExprs.end(),
		    tempAllGuardExprs.begin(), tempAllGuardExprs.end());
#endif
		allGuardExprs.insert(allGuardExprs.end(), holdGuards.begin(),
		    holdGuards.end());
	    }

	    for (std::vector<std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
		    >::iterator allIt = allGuardExprs.begin(); allIt != 
		    allGuardExprs.end(); allIt++) {
		SymbolicStmt* newStmt = (*sIt)->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: Cannot obtain clone of SymbolicStmt\n";
		    rv = false;
		    return;
		}
#ifdef DEBUG
		llvm::outs() << "DEBUG: Appending guards:\n";
		llvm::outs() << "DEBUG: newStmt->guards:\n";
		prettyPrintTheseGuardExprs(newStmt->guards);
		llvm::outs() << "\nDEBUG: *allIt:\n";
		prettyPrintTheseGuardExprs(*allIt);
		llvm::outs() << "\n";
#endif
		std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendedGuardExprs =
		    appendGuards(newStmt->guards, *allIt, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While appending guardExprs\n";
		    rv = false;
		    return;
		}
#ifdef DEBUG
		llvm::outs() << "DEBUG: appendedGuardExprs:\n";
		prettyPrintTheseGuardExprs(appendedGuardExprs);
		llvm::outs() << "\n";
#endif
		newStmt->guards.clear();
		newStmt->guards.insert(newStmt->guards.end(),
		    appendedGuardExprs.begin(), appendedGuardExprs.end());
#ifdef DEBUG
		llvm::outs() << "Orig guards:\n";
		(*sIt)->prettyPrintGuardExprs();
		llvm::outs() << "\n";
		llvm::outs() << "New guards:\n";
		newStmt->prettyPrintGuardExprs();
		llvm::outs() << "\n";
#endif
		newSymExprs.push_back(newStmt);
	    }
	}

	if (newSymExprs.size() != 0)
	    mIt->second = newSymExprs;

    }
}

void SymbolicDeclRefExpr::prettyPrintSummary(std::ofstream &logFile, 
const MainFunction* mObj, bool printAsGuard, bool &rv, bool inputFirst, 
bool inResidual, std::vector<LoopDetails> surroundingLoops) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering SymbolicDeclRefExpr::prettyPrintSummary()\n";
    prettyPrint(true);
#endif
    rv = true;
#if 0
    if (printAsGuard) {
	logFile << "(";
	prettyPrintGuardExprs(logFile, mObj, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyprinting guardExpr\n";
	    return;
	}
	logFile << ") && (";
    }
#endif
    if (vKind == VarKind::FUNC) {
	if (fd->funcName.compare("std::max") == 0 ||
		fd->funcName.compare("fmax") == 0)
	    logFile << "stdmax";
	else
	    logFile << fd->funcName;
    } else if (vKind == VarKind::LOOPINDEX) {
	if (substituteExpr) {
	    substituteExpr->prettyPrintSummary(logFile, mObj, false, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting substituteExpr\n";
		return;
	    }
	} else
	    logFile << var.varName;
    } else if (vKind == VarKind::GARBAGE) {
	logFile << "GARBAGE";
    } else {
	if (substituteExpr) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Printing substituteExpr in SymbolicDeclRefExpr\n";
	    substituteExpr->prettyPrint(false);
#endif
	    substituteExpr->prettyPrintSummary(logFile, mObj, false, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting substituteExpr\n";
		return;
	    }
	} else {
	    // TODO: Do a typecheck here
	    if (inputFirst) {
		for (std::vector<InputVar>::const_iterator ivIt = mObj->inputs.begin(); ivIt != 
			mObj->inputs.end(); ivIt++) {
		    if (var.equals(ivIt->var)) {
			std::stringstream ss;
			ss << INPUTNAME << "_" << ivIt->progID;
			// Check if there is any substituteArray for this input
			// var
			if (ivIt->substituteArray.array) {
			    // Check if the surrounding loops for this symexpr
			    // matches the substitute's loops
			    if (surroundingLoops.size() >=
				    ivIt->substituteArray.loops.size()) {
				for (std::vector<LoopDetails>::iterator slIt =
					surroundingLoops.begin(); slIt != 
					surroundingLoops.end(); slIt++) {
				    unsigned d = std::distance(slIt, surroundingLoops.end());
				    if (d != ivIt->substituteArray.loops.size())
					continue;
				    unsigned startingIndex = std::distance(surroundingLoops.begin(), slIt);
				    bool match = true;
				    for (unsigned i = 0; i <
					    ivIt->substituteArray.loops.size();
					    i++) {
					if (!(surroundingLoops[startingIndex+i].equals(ivIt->substituteArray.loops[i]))) {
					    match = false; break;
					}
				    }
				    if (match) {
					logFile << ss.str();
					for (std::vector<SymbolicExpr*>::iterator 
						inIt = ivIt->substituteArray.array->indexExprs.begin();
						inIt != ivIt->substituteArray.array->indexExprs.end();
						inIt++) {
					    logFile << "[";
					    (*inIt)->prettyPrintSummary(logFile,
						mObj, false, rv, inputFirst, 
						inResidual, surroundingLoops);
					    logFile << "]";
					}
					break;
				    }
				}
			    }
			} else {
			    logFile << ss.str();
			}
			//if (printAsGuard) logFile << ")";
			return;
		    }
		}
		for (std::vector<DPVar>::const_iterator dpIt = mObj->allDPVars.begin(); dpIt != 
			mObj->allDPVars.end(); dpIt++) {
		    if (var.equals(dpIt->dpArray)) {
			std::stringstream ss;
			ss << DPARRNAME << "_" << dpIt->id;
			logFile << ss.str();
			//if (printAsGuard) logFile << ")";
			return;
		    }
		}
	    } else {
		for (std::vector<DPVar>::const_iterator dpIt = mObj->allDPVars.begin(); dpIt != 
			mObj->allDPVars.end(); dpIt++) {
		    if (var.equals(dpIt->dpArray)) {
			std::stringstream ss;
			ss << DPARRNAME << "_" << dpIt->id;
			logFile << ss.str();
			//if (printAsGuard) logFile << ")";
			return;
		    }
		}
		for (std::vector<InputVar>::const_iterator ivIt = mObj->inputs.begin(); ivIt != 
			mObj->inputs.end(); ivIt++) {
		    if (var.equals(ivIt->var)) {
			std::stringstream ss;
			ss << INPUTNAME << "_" << ivIt->progID;
			if (ivIt->substituteArray.array) {
			    // Check if the surrounding loops for this symexpr
			    // matches the substitute's loops
			    if (surroundingLoops.size() >=
				    ivIt->substituteArray.loops.size()) {
				for (std::vector<LoopDetails>::iterator slIt =
					surroundingLoops.begin(); slIt != 
					surroundingLoops.end(); slIt++) {
				    unsigned d = std::distance(slIt, surroundingLoops.end());
				    if (d != ivIt->substituteArray.loops.size())
					continue;
				    unsigned startingIndex = std::distance(surroundingLoops.begin(), slIt);
				    bool match = true;
				    for (unsigned i = 0; i <
					    ivIt->substituteArray.loops.size();
					    i++) {
					if (!(surroundingLoops[startingIndex+i].equals(ivIt->substituteArray.loops[i]))) {
					    match = false; break;
					}
				    }
				    if (match) {
					logFile << ss.str();
					for (std::vector<SymbolicExpr*>::iterator 
						inIt = ivIt->substituteArray.array->indexExprs.begin();
						inIt != ivIt->substituteArray.array->indexExprs.end();
						inIt++) {
					    logFile << "[";
					    (*inIt)->prettyPrintSummary(logFile,
						mObj, false, rv, inputFirst, 
						inResidual, surroundingLoops);
					    logFile << "]";
					}
					break;
				    }
				}
			    }
			} else {
			    logFile << ss.str();
			}
			//if (printAsGuard) logFile << ")";
			return;
		    }
		}
	    }

#ifdef DEBUG
	    llvm::outs() << (inResidual? "residual": "not residual");
#endif
	    if (!inResidual) {
		llvm::outs() << "ERROR: Var " << var.varName << " is not a summary variable\n";
		rv = false;
		return;
	    } else
		logFile << var.varName;
	}
    }
    //if (printAsGuard) logFile << ")";
}

void SymbolicDeclRefExpr::toString(std::stringstream &logStr, 
const MainFunction* mObj, bool printAsGuard, bool &rv, bool inputFirst, 
bool inResidual, std::vector<LoopDetails> surroundingLoops) {
#ifdef DEBUGFULL
    llvm::outs() << "DEBUG: Entering SymbolicDeclRefExpr::toString()\n";
#endif
    rv = true;
    if (vKind == VarKind::FUNC) {
	if (fd->funcName.compare("std::max") == 0 ||
		fd->funcName.compare("fmax") == 0)
	    logStr << "stdmax";
	else
	    logStr << fd->funcName;
    } else if (vKind == VarKind::LOOPINDEX) {
	if (substituteExpr) {
	    substituteExpr->toString(logStr, mObj, false, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting substituteExpr\n";
		return;
	    }
	} else
	    logStr << var.varName;
    } else if (vKind == VarKind::GARBAGE) {
	logStr << "GARBAGE";
    } else {
	if (substituteExpr) {
	    substituteExpr->toString(logStr, mObj, false, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting substituteExpr\n";
		return;
	    }
	} else {
	    // TODO: Do a typecheck here
	    if (inputFirst) {
		for (std::vector<InputVar>::const_iterator ivIt = mObj->inputs.begin(); ivIt != 
			mObj->inputs.end(); ivIt++) {
		    if (var.equals(ivIt->var)) {
			std::stringstream ss;
			ss << INPUTNAME << "_" << ivIt->progID;
			// Check if there is any substituteArray for this input
			// var
			if (ivIt->substituteArray.array) {
			    // Check if the surrounding loops for this symexpr
			    // matches the substitute's loops
			    if (surroundingLoops.size() >=
				    ivIt->substituteArray.loops.size()) {
				for (std::vector<LoopDetails>::iterator slIt =
					surroundingLoops.begin(); slIt != 
					surroundingLoops.end(); slIt++) {
				    unsigned d = std::distance(slIt, surroundingLoops.end());
				    if (d != ivIt->substituteArray.loops.size())
					continue;
				    unsigned startingIndex = std::distance(surroundingLoops.begin(), slIt);
				    bool match = true;
				    for (unsigned i = 0; i <
					    ivIt->substituteArray.loops.size();
					    i++) {
					if (!(surroundingLoops[startingIndex+i].equals(ivIt->substituteArray.loops[i]))) {
					    match = false; break;
					}
				    }
				    if (match) {
					logStr << ss.str();
					for (std::vector<SymbolicExpr*>::iterator 
						inIt = ivIt->substituteArray.array->indexExprs.begin();
						inIt != ivIt->substituteArray.array->indexExprs.end();
						inIt++) {
					    logStr << "[";
					    (*inIt)->toString(logStr,
						mObj, false, rv, inputFirst, 
						inResidual, surroundingLoops);
					    logStr << "]";
					}
					break;
				    }
				}
			    }
			} else {
			    logStr << ss.str();
			}
			return;
		    }
		}
		for (std::vector<DPVar>::const_iterator dpIt = mObj->allDPVars.begin(); dpIt != 
			mObj->allDPVars.end(); dpIt++) {
		    if (var.equals(dpIt->dpArray)) {
			std::stringstream ss;
			ss << DPARRNAME << "_" << dpIt->id;
			logStr << ss.str();
			return;
		    }
		}
	    } else {
		for (std::vector<DPVar>::const_iterator dpIt = mObj->allDPVars.begin(); dpIt != 
			mObj->allDPVars.end(); dpIt++) {
		    if (var.equals(dpIt->dpArray)) {
			std::stringstream ss;
			ss << DPARRNAME << "_" << dpIt->id;
			logStr << ss.str();
			return;
		    }
		}
		for (std::vector<InputVar>::const_iterator ivIt = mObj->inputs.begin(); ivIt != 
			mObj->inputs.end(); ivIt++) {
		    if (var.equals(ivIt->var)) {
			std::stringstream ss;
			ss << INPUTNAME << "_" << ivIt->progID;
			if (ivIt->substituteArray.array) {
#ifdef DEBUG
			    llvm::outs() << "DEBUG: Checking if the surrounding "
					 << "loops for this symexpr matches the "
					 << "substitute's loops\n";
			    llvm::outs() << "DEBUG: surroundingLoops:\n";
			    for (std::vector<LoopDetails>::iterator l1It =
				    surroundingLoops.begin(); l1It != 
				    surroundingLoops.end(); l1It++) {
				l1It->prettyPrintHere();
			    }
			    llvm::outs() << "DEBUG: substitute's loops:\n";
			    for (std::vector<LoopDetails*>::const_iterator l1It =
				    ivIt->substituteArray.loops.begin(); l1It != 
				    ivIt->substituteArray.loops.end(); l1It++) {
				(*l1It)->prettyPrintHere();
			    }
#endif
			    // Check if the surrounding loops for this symexpr
			    // matches the substitute's loops
			    if (surroundingLoops.size() >=
				    ivIt->substituteArray.loops.size()) {
				for (std::vector<LoopDetails>::iterator slIt =
					surroundingLoops.begin(); slIt != 
					surroundingLoops.end(); slIt++) {
				    unsigned d = std::distance(slIt, surroundingLoops.end());
				    if (d != ivIt->substituteArray.loops.size())
					continue;
				    unsigned startingIndex = std::distance(surroundingLoops.begin(), slIt);
				    bool match = true;
				    for (unsigned i = 0; i <
					    ivIt->substituteArray.loops.size();
					    i++) {
					if (!(surroundingLoops[startingIndex+i].equals(ivIt->substituteArray.loops[i]))) {
					    match = false; break;
					}
				    }
				    if (match) {
					logStr << ss.str();
					for (std::vector<SymbolicExpr*>::iterator 
						inIt = ivIt->substituteArray.array->indexExprs.begin();
						inIt != ivIt->substituteArray.array->indexExprs.end();
						inIt++) {
					    logStr << "[";
					    (*inIt)->toString(logStr,
						mObj, false, rv, inputFirst, 
						inResidual, surroundingLoops);
					    logStr << "]";
					}
					break;
				    }
				}
			    }
			} else {
			    logStr << ss.str();
			}
			return;
		    } else {
#ifdef DEBUG
			llvm::outs() << "DEBUG: Var " << var.varName << " is not "
				     << "inputvar " << ivIt->var.varName << "\n";
#endif
		    }
		}
	    }

	    if (!inResidual) {
		llvm::outs() << "ERROR: Var " << var.varName << " is not a summary variable\n";
		rv = false;
		return;
	    } else
		logStr << var.varName;
	}
    }
}

bool SymbolicStmt::prettyPrintGuardExprs(std::ofstream &logFile,
const MainFunction* mObj, bool &rv, bool inputFirst, bool inResidual, 
std::vector<LoopDetails> surroundingLoops) {
    rv = true;
    bool printed = false;
    if (guards.size() == 0) {
	logFile << "true";
	return true;
    }
    bool foundTrue = false;
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
	    >::iterator gIt = guards.begin(); gIt != guards.end(); gIt++) {
	if (gIt->size() == 0) {
	    foundTrue = true; break;
	}
    }
    if (foundTrue) {
	logFile << "true";
	return true;
    }
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
	    >::iterator gIt = guards.begin(); gIt != guards.end(); gIt++) {
	if (gIt->size() == 0) {
	    printed = printed || true;
	    logFile << "true";
	}
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    printed = printed || sIt->first->prettyPrintGuardExprs(logFile,
		mObj, rv, inputFirst, inResidual, surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting guards "
			     << " of returnExpr\n";
		return printed;
	    }
	    if (sIt+1 != gIt->end() && printed) logFile << " && ";
	}
	if (gIt+1 != guards.end() && printed) logFile << " || ";
    }
    if (printed) logFile << " && ";
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
	    >::iterator gIt = guards.begin(); gIt != guards.end(); gIt++) {
	if (gIt->size() == 0) logFile << "true";
	for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator sIt = 
		gIt->begin(); sIt != gIt->end(); sIt++) {
	    printed = printed || true;
	    if (!(sIt->second)) logFile << "!";
	    logFile << "(";
	    sIt->first->prettyPrintSummary(logFile, mObj, false, rv, inputFirst, inResidual,
		surroundingLoops);
	    if (!rv) {
		llvm::outs() << "ERROR: While prettyprinting guard\n";
		return printed;
	    }
	    logFile << ")";
	    if (sIt+1 != gIt->end()) logFile << " && ";
	}
	if (gIt+1 != guards.end()) logFile << " || ";
    }
    return printed;
}

SymbolicExpr* GetSymbolicExprVisitor::getSymbolicExprForType(
const ArrayType* T, bool &rv) {
    rv = true;
    bool resolved;
    if (isa<ConstantArrayType>(T)) {
	const llvm::APInt size =
            dyn_cast<ConstantArrayType>(T)->getSize();
	SymbolicIntegerLiteral* sil = new SymbolicIntegerLiteral;
	sil->resolved = true;
	sil->val = size;
	return sil;
    } else if (isa<VariableArrayType>(T)) {
        Expr* sizeExpr =
            dyn_cast<VariableArrayType>(T)->getSizeExpr();
	ExprDetails* sizeDetails = ExprDetails::getObject(SM, sizeExpr, 
	    currBlock, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot get ExprDetails of "
			 << Helper::prettyPrintExpr(sizeExpr) << "\n";
	    rv = false;
	    return NULL;
	}

	//if (!isResolved(sizeDetails)) 
	resolved = sizeDetails->isSymExprCompletelyResolved(this, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While isSymExprCompletelyResolved() on size\n";
	    return NULL;
	}
	if (!resolved)
	{
	    llvm::outs() << "WARNING: " << Helper::prettyPrintExpr(sizeExpr) 
			 << " is not resolved\n";
	    //rv = false;
	    return NULL;
	}
	std::vector<SymbolicStmt*> symExprs =
	    getSymbolicExprs(sizeDetails, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot get symexprs for "
			 << Helper::prettyPrintExpr(sizeExpr) << "\n";
	    rv = false;
	    return NULL;
	}
	if (symExprs.size() != 1) {
	    llvm::outs() << "ERROR: " << Helper::prettyPrintExpr(sizeExpr)
			 << " has more than one symexprs\n";
	    rv = false;
	    return NULL;
	}
	if (!isa<SymbolicExpr>(symExprs[0])) {
	    llvm::outs() << "ERROR: Symexpr of <Expr> is not <SymbolicExpr>\n";
	    error = true;
	    return NULL;
	}
	return dyn_cast<SymbolicExpr>(symExprs[0]);
    } else {
        llvm::outs() << "ERROR: Unsupported array type\n";
	rv = false;
        return NULL;
    }
}

void GetSymbolicExprVisitor::printSymExprMap(bool printGuardExprs) {
    llvm::outs() << "symExprMap:\n";
    for (std::vector<std::pair<StmtDetails*, std::vector<SymbolicStmt*> >
	    >::iterator sIt = symExprMap.begin(); sIt != symExprMap.end();
	    sIt++) {
	llvm::outs() << "StmtDetails:\n";
	sIt->first->print();
	llvm::outs() << "\n";
	printSymExprs(sIt->first);
#if 0
	llvm::outs() << "\nSymbolicStmt:\n";
	for (std::vector<SymbolicStmt*>::iterator ssIt =
		sIt->second.begin(); ssIt != sIt->second.end(); ssIt++) {
	    if (printGuardExprs) {
		(*ssIt)->printGuardExprs();
	    }
	    (*ssIt)->print();
	}
#endif
    }
}

void GetSymbolicExprVisitor::prettyPrintSymExprMap(bool printGuardExprs) {
    llvm::outs() << "symExprMap:\n";
    for (std::vector<std::pair<StmtDetails*, std::vector<SymbolicStmt*> >
	    >::iterator sIt = symExprMap.begin(); sIt != symExprMap.end();
	    sIt++) {
	llvm::outs() << "StmtDetails:\n";
	sIt->first->prettyPrint();
	printSymExprs(sIt->first);
#if 0
	llvm::outs() << "\nSymbolicStmt:\n";
	for (std::vector<SymbolicStmt*>::iterator ssIt =
		sIt->second.begin(); ssIt != sIt->second.end(); ssIt++) {
	    if (printGuardExprs) {
		(*ssIt)->printGuardExprs();
	    }
	    (*ssIt)->print();
	}
#endif
    }
}

bool GetSymbolicExprVisitor::isExprPresentInSymMap(StmtDetails* ed) {
    for (std::vector<std::pair<StmtDetails*, std::vector<SymbolicStmt*> >
	    >::iterator sIt = symExprMap.begin(); sIt != symExprMap.end();
	    sIt++) {
	if (ed->equals(sIt->first))
	    return true;
    }
    return false;
}

bool GetSymbolicExprVisitor::insertExprInSymMap(StmtDetails* sd, SymbolicStmt* sst) {
    bool foundStmtDetails = false, foundSymbolicStmt = false;
    std::vector<std::pair<StmtDetails*, std::vector<SymbolicStmt*>
	> >::iterator sIt;
    std::vector<SymbolicStmt*>::iterator ssIt;
    for (sIt = symExprMap.begin(); sIt != symExprMap.end(); sIt++) {
	if (sIt->first->equals(sd)) {
	    foundStmtDetails = true;
	    for (ssIt = sIt->second.begin(); ssIt != sIt->second.end();
		    ssIt++) {
		if ((*ssIt)->equals(sst)) {
		    // Also check if the guards are equal
		    if ((*ssIt)->SymbolicStmt::equals(sst)) {
			foundSymbolicStmt = true;
			break;
		    }
		}
	    }
	    break;
	}
    }

    if (!foundStmtDetails) {
	std::vector<SymbolicStmt*> svec;
	svec.push_back(sst);
	std::pair<StmtDetails*, std::vector<SymbolicStmt*>> p =
	    std::make_pair(sd, svec);
	symExprMap.push_back(p);
    } else {
	if (!foundSymbolicStmt) {
	    sIt->second.push_back(sst);
	} else {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Found a duplicate entry\n\n";
	    //printSymExprMap(true);
#endif
	    //return false;
	}
    }

//#ifdef DEBUG
#if 0
    if (sd->lineNum == 29) {
	bool rv;
	llvm::outs() << "DEBUG: SymbolicStmt inserted:\n";
	sst->print();
	llvm::outs() << "DEBUG: Current symExprs:\n";
	std::vector<SymbolicStmt*> symExprs = 
	    getSymbolicExprs(sd, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot get symExprs for:\n";
	    sd->print();
	    return true;
	}
	llvm::outs() << "DEBUG: Size: " << symExprs.size() << "\n";
	for (std::vector<SymbolicStmt*>::iterator it = symExprs.begin(); 
		it != symExprs.end(); it++) {
	    (*it)->print();
	}
    }
#endif
    return true;
}

bool GetSymbolicExprVisitor::replaceExprInSymMap(StmtDetails* sd, SymbolicStmt* sst) {
    bool foundStmtDetails = false;
    std::vector<std::pair<StmtDetails*, std::vector<SymbolicStmt*>
	> >::iterator sIt;
    for (sIt = symExprMap.begin(); sIt != symExprMap.end(); sIt++) {
	if (sIt->first->equals(sd)) {
	    foundStmtDetails = true;
	    break;
	}
    }

    std::vector<SymbolicStmt*> svec;
    svec.push_back(sst);

    if (!foundStmtDetails) {
	std::pair<StmtDetails*, std::vector<SymbolicStmt*>> p =
	    std::make_pair(sd, svec);
	symExprMap.push_back(p);
    } else {
	sIt->second = svec;
    }

    return true;
}

std::vector<SymbolicStmt*> GetSymbolicExprVisitor::getSymbolicExprs(StmtDetails* sd, bool &rv) {
    rv = true;

#ifdef DEBUGFULL
    if (sd->blockID == 8) {
	llvm::outs() << "DEBUG: Looking for symbolic exprs of\n";
	sd->print();
    }
#endif
    std::vector<SymbolicStmt*> svec;
    for (std::vector<std::pair<StmtDetails*, std::vector<SymbolicStmt*> >
	    >::iterator sIt = symExprMap.begin(); sIt != symExprMap.end();
	    sIt++) {
#ifdef DEBUGFULL
	if (sd->blockID == 8) {
	    llvm::outs() << "DEBUG: Looking at\n";
	    sIt->first->print();
	}
#endif
	if (sd->equals(sIt->first)) {
#ifdef DEBUGFULL
	    if (sd->blockID == 8) {
		llvm::outs() << "DEBUG: Equal\n";
		if (sIt->second.size() == 0)
		    llvm::outs() << "DEBUG: No symbolic expression\n";
	    }
#endif
	    return sIt->second;
	}
    }
    rv = false;
    return svec;
}

void GetSymbolicExprVisitor::removeSymExprs(StmtDetails* sd, bool &rv) {
    rv = true;
    std::vector<std::pair<StmtDetails*, std::vector<SymbolicStmt*> > >::iterator sIt;
    for (sIt = symExprMap.begin(); sIt != symExprMap.end(); sIt++) {
	if (sd->equals(sIt->first)) break;
    }

    if (sIt != symExprMap.end()) symExprMap.erase(sIt);
}

void GetSymbolicExprVisitor::printSymExprs(StmtDetails* ed) {
    bool rv;
    std::vector<SymbolicStmt*> symExprs = getSymbolicExprs(ed, rv);
    if (!rv) {
	llvm::outs() << "No Symbolic Exprs\n";
    } else {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Num of symbolic exprs: " << symExprs.size()
		     << "\n";
#endif
	for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin();
		sIt != symExprs.end(); sIt++) {
	    (*sIt)->prettyPrint(true);
	    llvm::outs() << "\n";
	}
    }
    if (isa<VarDeclDetails>(ed)) {
	VarDeclDetails* vdd = dyn_cast<VarDeclDetails>(ed);
	std::vector<SymbolicExpr*> types =
	    getSymExprsForArrayType(vdd->var);
	llvm::outs() << "Size Exprs: ";
	for (std::vector<SymbolicExpr*>::iterator it = types.begin(); it != 
		types.end(); it++) {
	    llvm::outs() << "[";
	    (*it)->prettyPrint(true);
	    llvm::outs() << "]";
	}
    }
}

void SymbolicSpecialExpr::prettyPrintSummary(std::ofstream &logFile, 
const MainFunction* mObj, bool printAsGuard, bool &rv, 
bool inputFirst, bool inResidual, 
std::vector<LoopDetails> surroundingLoops) {
    rv = true;
#if 0
    if (printAsGuard) {
	logFile << "(";
	prettyPrintGuardExprs(logFile, mObj, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyprinting guardExpr\n";
	    return;
	}
	logFile << ") && (";
    }
#endif
    if (sKind == SpecialExprKind::SUM) {
	std::stringstream logStr;
	toString(logStr, mObj, printAsGuard, rv, inputFirst, inResidual,
	    surroundingLoops);
	std::string sumStr = logStr.str();
	for (unsigned i = 0; i < sumStr.length(); i++) {
	    if (sumStr[i] == ',')
		sumStr[i] = '_';
	    else if (sumStr[i] == '+')
		sumStr[i] = 'p';
	    else if (sumStr[i] == '-')
		sumStr[i] = 'm';
	}
	char chars[] = "()[]. ";
	for (unsigned i = 0; i < strlen(chars); i++) {
	    sumStr.erase(std::remove(sumStr.begin(), sumStr.end(), chars[i]),
		sumStr.end());
	}
	logFile << sumStr;
	return;
    }
    if (sane) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: PrettyPrinting sane specialExpr\n";
#endif
	if (sKind == SpecialExprKind::MAX) logFile << "symex_max";
	else if (sKind == SpecialExprKind::MIN) logFile << "symex_min";
	else if (sKind == SpecialExprKind::SUM) logFile << "symex_sum";
	else {
	    llvm::outs() << "ERROR: Unresolved SpecialExprKind\n";
	    rv = false;
	    return;
	}
	logFile << "(";
	initExpr->prettyPrintSummary(logFile, mObj, printAsGuard, rv,
	    inputFirst, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyprinting initExpr\n";
	    return;
	}
	logFile << ", ";
	arrayVar->prettyPrintSummary(logFile, mObj, printAsGuard, rv,
	    inputFirst, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: while prettyprinting arrayVar\n";
	    return;
	}
	for (std::vector<std::pair<RANGE, bool> >::iterator iIt =
		indexRangesAtAssignLine.begin(); iIt != 
		indexRangesAtAssignLine.end(); iIt++) {
	    logFile << "[";
	    if (iIt->first.first->equals(iIt->first.second)) {
		iIt->first.first->prettyPrintSummary(logFile, mObj,
		    printAsGuard, rv, inputFirst, inResidual, surroundingLoops);
		if (!rv) {
		    llvm::outs() << "ERROR: While prettyprinting indexRange\n";
		    return;
		}
	    } else {
		iIt->first.first->prettyPrintSummary(logFile, mObj,
		    printAsGuard, rv, inputFirst, inResidual, surroundingLoops);
		if (!rv) {
		    llvm::outs() << "ERROR: While prettyprinting indexRange1\n";
		    return;
		}
		logFile << " ... ";
		iIt->first.second->prettyPrintSummary(logFile, mObj,
		    printAsGuard, rv, inputFirst, inResidual, surroundingLoops);
		if (!rv) {
		    llvm::outs() << "ERROR: While prettyprinting indexRange2\n";
		    return;
		}
		if (iIt->second) {
		    //Strict bound
		    logFile << " - 1";
		}
	    }
	    logFile << "]";
	}
	logFile << ")";
	//if (printAsGuard) logFile << ")";
    } else {
#ifdef DEBUG
	llvm::outs() << "DEBUG: PrettyPrinting not sane specialExpr\n";
#endif
	if (originalSpecialExpr) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: originalSpecialExpr not NULL\n";
#endif
	    unsigned numLoops = 0;
	    logFile << "out = ";
	    initExpr->prettyPrintSummary(logFile, mObj, printAsGuard, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) return;
	    logFile << ";\n";
	    for (std::vector<LoopDetails*>::iterator lIt =
		    originalSpecialExpr->loops.begin(); lIt != 
		    originalSpecialExpr->loops.end(); lIt++) {
		bool isTestCaseLoop =
		    ((*lIt)->isTestCaseLoop(const_cast<InputVar*>(&(mObj->testCaseVar)), rv));
		if (!rv) return;
		if (isTestCaseLoop) continue;
		numLoops++;
		(*lIt)->prettyPrint(logFile, mObj, rv, inResidual);
	    }
	    logFile << "(out ";
	    if (sKind == SpecialExprKind::MIN) logFile << "> ";
	    else if (sKind == SpecialExprKind::MAX) logFile << "< ";
	    else {
		llvm::outs() << "ERROR: Unresolved special expr\n";
		rv = false;
		return;
	    }
	    arrayVar->prettyPrintSummary(logFile, mObj, printAsGuard, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) return;
	    for (std::vector<SymbolicExpr*>::iterator iIt =
		    indexExprsAtAssignLine.begin(); iIt != 
		    indexExprsAtAssignLine.end(); iIt++) {
		logFile << "[";
		(*iIt)->prettyPrintSummary(logFile, mObj, printAsGuard, rv,
		    inputFirst, inResidual, surroundingLoops);
		if (!rv) return;
		logFile << "]";
	    }
	    logFile << "): out = ";
	    arrayVar->prettyPrintSummary(logFile, mObj, printAsGuard, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) return;
	    for (std::vector<SymbolicExpr*>::iterator iIt =
		    indexExprsAtAssignLine.begin(); iIt != 
		    indexExprsAtAssignLine.end(); iIt++) {
		logFile << "[";
		(*iIt)->prettyPrintSummary(logFile, mObj, printAsGuard, rv,
		    inputFirst, inResidual, surroundingLoops);
		if (!rv) return;
		logFile << "]";
	    }
	    logFile << ";\n";
	    for (unsigned i = 0; i < numLoops; i++)
		logFile << "}\n";
	}
    }
}

void SymbolicSpecialExpr::toString(std::stringstream &logStr, 
const MainFunction* mObj, bool printAsGuard, bool &rv, 
bool inputFirst, bool inResidual, 
std::vector<LoopDetails> surroundingLoops) {
    rv = true;
    if (sane) {
	if (sKind == SpecialExprKind::MAX) logStr << "symex_max";
	else if (sKind == SpecialExprKind::MIN) logStr << "symex_min";
	else if (sKind == SpecialExprKind::SUM) logStr << "symex_sum";
	else {
	    llvm::outs() << "ERROR: Unresolved SpecialExprKind\n";
	    rv = false;
	    return;
	}
	logStr << "(";
	initExpr->toString(logStr, mObj, printAsGuard, rv,
	    inputFirst, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: While prettyprinting initExpr\n";
	    return;
	}
	logStr << ", ";
	arrayVar->toString(logStr, mObj, printAsGuard, rv,
	    inputFirst, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: while prettyprinting arrayVar\n";
	    return;
	}
	for (std::vector<std::pair<RANGE, bool> >::iterator iIt =
		indexRangesAtAssignLine.begin(); iIt != 
		indexRangesAtAssignLine.end(); iIt++) {
	    logStr << "[";
	    if (iIt->first.first->equals(iIt->first.second)) {
		iIt->first.first->toString(logStr, mObj,
		    printAsGuard, rv, inputFirst, inResidual, surroundingLoops);
		if (!rv) {
		    llvm::outs() << "ERROR: While prettyprinting indexRange\n";
		    return;
		}
	    } else {
		iIt->first.first->toString(logStr, mObj,
		    printAsGuard, rv, inputFirst, inResidual, surroundingLoops);
		if (!rv) {
		    llvm::outs() << "ERROR: While prettyprinting indexRange1\n";
		    return;
		}
		logStr << " ... ";
		iIt->first.second->toString(logStr, mObj,
		    printAsGuard, rv, inputFirst, inResidual, surroundingLoops);
		if (!rv) {
		    llvm::outs() << "ERROR: While prettyprinting indexRange2\n";
		    return;
		}
		if (iIt->second) {
		    //Strict bound
		    logStr << " - 1";
		}
	    }
	    logStr << "]";
	}
	logStr << ")";
    } else {
	llvm::outs() << "ERROR: Calling toString() on special expr that is not sane\n";
	rv = false;
	return;
#if 0
	if (originalSpecialExpr) {
	    unsigned numLoops = 0;
	    logStr << "out = ";
	    initExpr->toString(logStr, mObj, printAsGuard, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) return;
	    logStr << ";\n";
	    for (std::vector<LoopDetails*>::iterator lIt =
		    originalSpecialExpr->loops.begin(); lIt != 
		    originalSpecialExpr->loops.end(); lIt++) {
		bool isTestCaseLoop =
		    ((*lIt)->isTestCaseLoop(const_cast<InputVar*>(&(mObj->testCaseVar)), rv));
		if (!rv) return;
		if (isTestCaseLoop) continue;
		numLoops++;
		(*lIt)->prettyPrint(logStr, mObj, rv, inResidual);
	    }
	    logStr << "(out ";
	    if (sKind == SpecialExprKind::MIN) logStr << "> ";
	    else if (sKind == SpecialExprKind::MAX) logStr << "< ";
	    else {
		llvm::outs() << "ERROR: Unresolved special expr\n";
		rv = false;
		return;
	    }
	    arrayVar->toString(logStr, mObj, printAsGuard, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) return;
	    for (std::vector<SymbolicExpr*>::iterator iIt =
		    indexExprsAtAssignLine.begin(); iIt != 
		    indexExprsAtAssignLine.end(); iIt++) {
		logStr << "[";
		(*iIt)->toString(logStr, mObj, printAsGuard, rv,
		    inputFirst, inResidual, surroundingLoops);
		if (!rv) return;
		logStr << "]";
	    }
	    logStr << "): out = ";
	    arrayVar->toString(logStr, mObj, printAsGuard, rv,
		inputFirst, inResidual, surroundingLoops);
	    if (!rv) return;
	    for (std::vector<SymbolicExpr*>::iterator iIt =
		    indexExprsAtAssignLine.begin(); iIt != 
		    indexExprsAtAssignLine.end(); iIt++) {
		logStr << "[";
		(*iIt)->toString(logStr, mObj, printAsGuard, rv,
		    inputFirst, inResidual, surroundingLoops);
		if (!rv) return;
		logStr << "]";
	    }
	    logStr << ";\n";
	    for (unsigned i = 0; i < numLoops; i++)
		logStr << "}\n";
	}
#endif
    }
}

