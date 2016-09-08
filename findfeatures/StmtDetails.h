#ifndef STMTDETAILS_H
#define STMTDETAILS_H

#include "includes.h"
#include "Helper.h"
#include "llvm/Support/Casting.h"

class ExprDetails;
class VarDeclDetails;
class GetSymbolicExprVisitor;

// Class to store Stmt details
class StmtDetails {
private:
    const StmtKind kind;

public:
    unsigned blockID;
    int lineNum;
    Stmt* origStmt;
    bool isDummy;
    FunctionDetails* func;

    StmtDetails(StmtKind k): kind(k) {
	blockID = -1;
	lineNum = -1;
	origStmt = NULL;
	isDummy = false;
	func = NULL;
    }

    virtual bool equals(StmtDetails* sd) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: In StmtDetails::equals()\n";
#endif
	if (sd->getKind() != kind) return false;
	if (sd->blockID != blockID) return false;
	if (sd->lineNum != lineNum) return false;
	return true;
    }

    virtual void print();
    virtual void prettyPrint() {}
    virtual ~StmtDetails() {}
    virtual bool isSymExprCompletelyResolved(
    GetSymbolicExprVisitor* symVisitor, bool &rv) {
	return false;
    }
    virtual void callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv);

    StmtKind getKind() const {return kind;}

    // Given a stmt and its blockID, return an StmtDetails object
    static StmtDetails* getStmtDetails(const SourceManager* SM, Stmt* S, 
    unsigned blockID, bool &rv, FunctionDetails* f=NULL);
};

// Class to store DeclStmt details
class DeclStmtDetails: public StmtDetails {
public:
    VarDetails var; // var declared
    VarDeclDetails* vdd;

    DeclStmtDetails():StmtDetails(DECL) {
	vdd = NULL;
    }

    DeclStmtDetails(StmtKind k): StmtDetails(k) {
	vdd = NULL;
    }

    bool equals(StmtDetails* sd);
    void print();
    void prettyPrint();
    bool isSymExprCompletelyResolved(GetSymbolicExprVisitor* symVisitor, bool &rv);
    void callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv);

    static DeclStmtDetails* getObject(const SourceManager* SM, DeclStmt* D,
    unsigned blockID, bool &rv, FunctionDetails* f = NULL);

    static bool classof (const StmtDetails* S) {
	return S->getKind() >= DECL && S->getKind() <= DECLEND;
    }
};

class VarDeclDetails : public DeclStmtDetails {
public:
    VarDetails var;
    ExprDetails* initExpr;

    VarDeclDetails(): DeclStmtDetails(VARDECL) {
	initExpr = NULL;
    }

    bool hasInit() {
	if (initExpr != NULL)
	    return true;
	else
	    return false;
    }

    void print();
    void prettyPrint();
    bool isSymExprCompletelyResolved(GetSymbolicExprVisitor* symVisitor, bool &rv);
    void callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv);

    bool equals(StmtDetails* sd) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: In VarDeclDetails::equals()\n";
#endif
	if (!(DeclStmtDetails::equals(sd))) return false;
	if (!isa<VarDeclDetails>(sd)) return false;
	VarDeclDetails* vdd = dyn_cast<VarDeclDetails>(sd);
	if (!(var.equals(vdd->var))) return false;
	return true;
    }

    static VarDeclDetails* getObject(const SourceManager* SM, VarDecl* VD,
    unsigned blockId, bool &rv, FunctionDetails* f = NULL);

    static bool classof (const StmtDetails* S) {
	return S->getKind() == VARDECL;
    }
};

class ReturnStmtDetails: public StmtDetails {
public:
    ExprDetails* retExpr;

    ReturnStmtDetails(): StmtDetails(RETURN) {
	retExpr = NULL;
    }

    bool equals(StmtDetails* S);
    void print();
    void prettyPrint();
    bool isSymExprCompletelyResolved(GetSymbolicExprVisitor* symVisitor, bool &rv);
    void callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv);

    static ReturnStmtDetails* getObject(const SourceManager* SM, ReturnStmt* R, 
    unsigned blockID, bool &rv, FunctionDetails* f = NULL);

    static bool classof (const StmtDetails* S) {
	return S->getKind() == RETURN;
    }
};

// Class to store Expr details
class ExprDetails: public StmtDetails {
public:
    //bool isVar; // true if this expr is a var
    //VarDetails var; // populated if isVar is true
    //std::string expr;
    Expr* origExpr;

    ExprDetails(): StmtDetails(EXPR) {
	//isVar = false;
	//expr = "";
	origExpr = NULL;
    }

    ExprDetails(StmtKind k): StmtDetails(k) {
	origExpr = NULL;
    }

#if 0
    ExprDetails(VarDetails vd, int line, unsigned block): StmtDetails(EXPR) {
	blockID = block;
	lineNum = line;
	isVar = true;
	var = vd;
	expr = "";
	origExpr = NULL;
    }

    bool operator<(const ExprDetails& rhs) const {
	return (blockID < rhs.blockID || 
	    (blockID == rhs.blockID && lineNum < rhs.lineNum) || 
	    (blockID == rhs.blockID && lineNum == rhs.lineNum && isVar &&
	    rhs.isVar && var.varDeclLine < rhs.var.varDeclLine) || 
	    (blockID == rhs.blockID && lineNum == rhs.lineNum && isVar &&
	    rhs.isVar && var.varDeclLine == rhs.var.varDeclLine &&
	    var.varName.compare(rhs.var.varName) < 0) || 
	    (blockID == rhs.blockID && lineNum == rhs.lineNum && isVar && 
	    !rhs.isVar) ||
	    (blockID == rhs.blockID && lineNum == rhs.lineNum && !isVar && 
	    !rhs.isVar && expr.compare(rhs.expr) < 0));
    }

    bool equals(ExprDetails* e) {
	if (e->blockID != blockID) return false;
	if (e->lineNum != lineNum) return false;
	if (e->getKind() != getKind()) return false;
	if (e->isVar != isVar) return false;
	if (isVar && !(e->var.equals(var))) return false;
	if (!isVar && e->expr.compare(expr) != 0) return false;
	return true;
    }
#endif

#if 0
    bool equals(StmtDetails s) {
	if (s.kind == StmtDetails::StmtKind::EXPR) {
	    ExprDetails sED = static_cast<ExprDetails>(s);
	    return this->equals(sED);
	} else 
	    return false;
    }
#endif

    ExprDetails* clone() {
	ExprDetails* cloneObj = new ExprDetails;
	cloneObj->blockID = blockID;
	cloneObj->lineNum = lineNum;
	//cloneObj->isVar = isVar;
	//cloneObj->var = var.clone();
	//cloneObj->expr = expr;
	cloneObj->origExpr = origExpr;
	return cloneObj;
    }

    bool equals(StmtDetails* sd) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: In ExprDetails::equals()\n";
#endif
	if (!(StmtDetails::equals(sd))) return false;
	if (!isa<ExprDetails>(sd)) return false;
	ExprDetails* ed = dyn_cast<ExprDetails>(sd);
	std::string expr = Helper::prettyPrintExpr(origExpr);
	std::string sdExpr = Helper::prettyPrintExpr(ed->origExpr);
	if (expr.compare(sdExpr) != 0) return false;
	return true;
    }

    void print() {
	llvm::outs() << "Expr at line " << lineNum << " and block " << blockID;
	if (origExpr)
	    llvm::outs() << ": " << Helper::prettyPrintExpr(origExpr);
	else
	    llvm::outs() << ": NULL";
	llvm::outs() << "\n";
	StmtDetails::print();
    }

    virtual void prettyPrint() {
    }

#if 0
    // Given an expr and its blockID, return an ExprDetails object
    static ExprDetails* getExprDetails(const SourceManager* SM, Expr* E, 
    unsigned blockID, bool &rv);

    // Given a decl and its blockID, return an ExprDetails object
    static ExprDetails* getExprDetails(const SourceManager* SM, Decl* D, 
    unsigned blockID, bool &rv);
#endif

    static ExprDetails* getObject(const SourceManager* SM, Expr* E,
    unsigned blockID, bool &rv, FunctionDetails* f = NULL);

    static bool classof (const StmtDetails* S) {
	return S->getKind() >= EXPR && S->getKind() <= EXPREND;
    }
};

class CallExprDetails: public ExprDetails {
public:
    ExprDetails* callee;
    std::vector<ExprDetails*> callArgExprs;

    CallExprDetails(): ExprDetails(CALLEXPR) {
	callee = NULL;
    }

    CallExprDetails(StmtKind k): ExprDetails(k) {
	callee = NULL;
    }

    void print() {
	llvm::outs() << "Callee:\n";
	if (callee)
	    callee->print();
	else 
	    llvm::outs() << "NULL";
	llvm::outs() << "\n";
	llvm::outs() << "ArgExprs:\n";
	for (std::vector<ExprDetails*>::iterator it = callArgExprs.begin(); 
		it != callArgExprs.end(); it++) {
	    if (*it)
		(*it)->print();
	}
	ExprDetails::print();
    }

    void prettyPrint() {
	if (callee) {
	    callee->prettyPrint();
	    llvm::outs() << "(";
	}
	for (std::vector<ExprDetails*>::iterator it = callArgExprs.begin();
		it != callArgExprs.end(); it++) {
	    if (*it) {
		(*it)->prettyPrint();
		if (it+1 != callArgExprs.end())
		    llvm::outs() << ", ";
	    }
	}
	if (callee) llvm::outs() << ")";
    }
    
    bool equals(StmtDetails* sd) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: In CallExprDetails::equals()\n";
#endif
	if (!isa<ExprDetails>(sd)) return false;
	if (!(ExprDetails::equals(sd))) return false;
	if (!isa<CallExprDetails>(sd)) return false;
	CallExprDetails* ced = dyn_cast<CallExprDetails>(sd);
	if (callee && !ced->callee) return false;
	if (!callee && ced->callee) return false;
	if (callee && ced->callee) {
	    if (!(callee->equals(ced->callee))) return false;
	}
	if (callArgExprs.size() != ced->callArgExprs.size()) return false;
	for (unsigned i = 0; i < callArgExprs.size(); i++) {
	    if (!callArgExprs[i] && ced->callArgExprs[i]) return false;
	    if (callArgExprs[i] && !ced->callArgExprs[i]) return false;
	    if (callArgExprs[i] && ced->callArgExprs[i]) {
		if (!(callArgExprs[i]->equals(ced->callArgExprs[i]))) return false;
	    }
	}
	return true;
    }

    bool isSymExprCompletelyResolved(GetSymbolicExprVisitor* symVisitor, bool &rv);
    void callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv);

    static CallExprDetails* getObject(const SourceManager* SM, CallExpr* E,
    unsigned blockID, bool &rv, FunctionDetails* f = NULL);

    static bool classof (const StmtDetails* S) {
	return S->getKind() >= CALLEXPR && S->getKind() <= CALLEXPREND;
    }
};

class CXXOperatorCallExprDetails: public CallExprDetails {
public:

    CXXOperatorKind opKind;

    CXXOperatorCallExprDetails(): CallExprDetails(CXXOPERATORCALLEXPR) {
	opKind = UNKNOWNOP;
    }

    void print() {
	llvm::outs() << "Operator: ";
	if (opKind == CIN) llvm::outs() << "CIN";
	else if (opKind == COUT) llvm::outs() << "COUT";
	else llvm::outs() << "UNKNOWN";
	llvm::outs() << "\n";
	CallExprDetails::print();
    }

    void prettyPrint() {
	if (opKind == CIN) llvm::outs() << "cin >> ";
	else if (opKind == COUT) llvm::outs() << "cout << ";
	else llvm::outs() << "Unknown CXXOperator ";
	llvm::outs() << "\n";
	for (std::vector<ExprDetails*>::iterator it = callArgExprs.begin(); 
		it != callArgExprs.end(); it++) {
	    if (*it) {
		(*it)->prettyPrint();
		if (it+1 != callArgExprs.end())
		    llvm::outs() << ", ";
	    }
	}
    }

    bool equals(StmtDetails* sd) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: In CXXOperatorCallExprDetails::equals()\n";
#endif
	if (!isa<CallExprDetails>(sd)) return false;
	if (!(CallExprDetails::equals(sd))) return false;
	if (!isa<CXXOperatorCallExprDetails>(sd)) return false;
	CXXOperatorCallExprDetails* ced =
	    dyn_cast<CXXOperatorCallExprDetails>(sd);
	if (opKind != ced->opKind) return false;
	return true;
    }

    bool isSymExprCompletelyResolved(GetSymbolicExprVisitor* symVisitor, bool &rv);
    void callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv);

    static CXXOperatorCallExprDetails* getObject(const SourceManager* SM,
    CXXOperatorCallExpr* E, unsigned blockID, bool &rv, FunctionDetails* f = NULL);

    static bool classof(const StmtDetails* S) {
	return S->getKind() == CXXOPERATORCALLEXPR;
    }
};

class InitListExprDetails: public ExprDetails {
public:
    bool hasArrayFiller;
    ExprDetails* arrayFiller;
    std::vector<ExprDetails*> inits;

    InitListExprDetails(): ExprDetails(INITLISTEXPR) {
	arrayFiller = nullptr;
    }

    void print() {
	if (arrayFiller) {
	    llvm::outs() << "ArrayFiller:\n";
	    arrayFiller->print();
	}
	llvm::outs() << "Inits:\n";
	for (std::vector<ExprDetails*>::iterator inIt = inits.begin(); inIt != 
		inits.end(); inIt++) {
	    if (*inIt) (*inIt)->print();
	}
	ExprDetails::print();
    }

    void prettyPrint() {
	if (arrayFiller) {
	    llvm::outs() << "ArrayFiller: ";
	    arrayFiller->prettyPrint();
	    llvm::outs() << "\n";
	}
	llvm::outs() << "Inits: ";
	for (std::vector<ExprDetails*>::iterator inIt = inits.begin(); inIt != 
		inits.end(); inIt++) {
	    if (*inIt) {
		(*inIt)->prettyPrint();
		if (inIt+1 != inits.end())
		    llvm::outs() << ", ";
	    }
	}
    }

    bool equals(StmtDetails* sd) {
	if (!isa<ExprDetails>(sd)) return false;
	if (!ExprDetails::equals(sd)) return false;
	if (!isa<InitListExprDetails>(sd)) return false;
	InitListExprDetails* ile = dyn_cast<InitListExprDetails>(sd);
	if (ile->hasArrayFiller != hasArrayFiller) return false;
	if (hasArrayFiller) {
	    if (arrayFiller && !(ile->arrayFiller)) return false;
	    if (!arrayFiller && ile->arrayFiller) return false;
	    if (ile->arrayFiller && arrayFiller && !(ile->arrayFiller->equals(arrayFiller))) return false;
	}
	if (ile->inits.size() != inits.size()) return false;
	for (unsigned i = 0; i < inits.size(); i++) {
	    if (inits[i] && !(ile->inits[i])) return false;
	    if (!inits[i] && ile->inits[i]) return false;
	    if (inits[i] && ile->inits[i] && !(ile->inits[i]->equals(inits[i]))) return false;
	}
	return true;
    }

    bool isSymExprCompletelyResolved(GetSymbolicExprVisitor* symVisitor, bool &rv);
    void callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv);

    static InitListExprDetails* getObject(const SourceManager* SM, InitListExpr* I, 
    unsigned blockID, bool &rv, FunctionDetails* f = NULL);

    static bool classof(const StmtDetails* S) {
	return S->getKind() == INITLISTEXPR;
    }
};

class DeclRefExprDetails: public ExprDetails {
public:

    DeclRefKind rKind;
    VarDetails var;
    FunctionDetails* fd;
    
    DeclRefExprDetails(): ExprDetails(DECLREFEXPR) {
	rKind = UNKNOWNREF;
	fd = NULL;
    }

    void print() {
	llvm::outs() << "DeclRefExprDetails: ";
	if (rKind == VAR) {
	    llvm::outs() << "VAR: ";
	    var.print();
	} else if (rKind == FUNC) {
	    llvm::outs() << "FUNC: ";
	    fd->print();
	} else if (rKind == LIB) {
	    llvm::outs() << "LIB: ";
	} else {
	    llvm::outs() << "UNKNOWN: ";
	}
	llvm::outs() << "\n";
	ExprDetails::print();
    }

    void prettyPrint() {
	if (rKind == VAR)
	    llvm::outs() << var.varName;
	else if (rKind == FUNC)
	    llvm::outs() << fd->funcName;
	else if (rKind == LIB) {
	    llvm::outs() << "Lib: ";
	    if (fd) llvm::outs() << fd->funcName;
	    else llvm::outs() << var.varName;
	} else 
	    llvm::outs() << "Unknown";
    }

    bool equals(StmtDetails* sd) {
#ifdef DEBUGFULL
	llvm::outs() << "DEBUG: In DeclRefExprDetails::equals()\n";
#endif
	if (!isa<ExprDetails>(sd)) return false;
	if (!(ExprDetails::equals(sd))) return false;
	if (!isa<DeclRefExprDetails>(sd)) return false;
	DeclRefExprDetails* dred = dyn_cast<DeclRefExprDetails>(sd);
	if (rKind != dred->rKind) return false;
	if (rKind == VAR && !(var.equals(dred->var))) return false;
	return true;
    }

    bool isSymExprCompletelyResolved(GetSymbolicExprVisitor* symVisitor, bool &rv);
    void callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv);

    static bool classof (const StmtDetails* S) {
	return S->getKind() == DECLREFEXPR;
    }

    // Given an expr and its blockID, return a DeclRefExprDetails object
    static DeclRefExprDetails* getObject(const SourceManager* SM, DeclRefExpr*
    DRE, unsigned blockID, bool &rv, FunctionDetails* f = NULL);
};

class UnaryExprOrTypeTraitExprDetails: public ExprDetails {
public:
    UnaryExprOrTypeTrait kind;
    DeclRefExprDetails* array;
    TypeSourceInfo* Ty;

    UnaryExprOrTypeTraitExprDetails(): ExprDetails(UNARYEXPRORTYPETRAITEXPR) {
	array = NULL;
	Ty = NULL;
    }

    void print() {
	llvm::outs() << "UnaryExprOrTypeTraitExprDetails: ";
	if (kind == UnaryExprOrTypeTrait::UETT_SizeOf) {
	    llvm::outs() << "sizeof ";
	    if (array) array->print();
	    else llvm::outs() << "array NULL\n";
	    if (Ty) Ty->getType().split().Ty->dump();
	    else llvm::outs() << "Type NULL";
	    llvm::outs() << "\n";
	} else {
	    llvm::outs() << "Unknown UnaryExprOrTypeTrait\n";
	}
    }

    void prettyPrint() {
	if (kind == UnaryExprOrTypeTrait::UETT_SizeOf) {
	    llvm::outs() << "sizeof(";
	    if (array) array->prettyPrint();
	    if (Ty) llvm::outs() << "Type";
	}
    }

    bool equals(StmtDetails* sd) {
	if (!isa<ExprDetails>(sd)) return false;
	if (!(ExprDetails::equals(sd))) return false;
	if (!isa<UnaryExprOrTypeTraitExprDetails>(sd)) return false;
	UnaryExprOrTypeTraitExprDetails* uett =
	    dyn_cast<UnaryExprOrTypeTraitExprDetails>(sd);
	if (uett->kind != kind) return false;
	if (uett->array && !array) return false;
	if (!(uett->array) && array) return false;
	if (!(uett->array) && !array) return false;
	if (!(uett->array->equals(array))) return false;
	return true;
    }

    bool isSymExprCompletelyResolved(GetSymbolicExprVisitor* symVisitor, bool &rv);
    void callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv);

    static bool classof (const StmtDetails* S) {
	return S->getKind() == UNARYEXPRORTYPETRAITEXPR;
    }

    static UnaryExprOrTypeTraitExprDetails* getObject(const SourceManager* SM, 
    UnaryExprOrTypeTraitExpr* UETT, unsigned blockID, bool &rv, FunctionDetails* f = NULL);
};

class ArraySubscriptExprDetails: public ExprDetails {
public:
    DeclRefExprDetails* baseArray;
    std::vector<ExprDetails*> indexExprs;

    ArraySubscriptExprDetails(): ExprDetails(ARRAYSUBEXPR) {
	baseArray = NULL;
	indexExprs = std::vector<ExprDetails*>();
    }

    void print() {
	llvm::outs() << "ArraySubscriptExprDetails: ";
	llvm::outs() << "Array Base: ";
	if (baseArray) {
	    baseArray->print();
	}
	llvm::outs() << "Index exprs: ";
	int i = 0;
	for (std::vector<ExprDetails*>::iterator iIt = indexExprs.begin();
		iIt != indexExprs.end(); iIt++) {
	    llvm::outs() << "Index " << ++i << ": ";
	    (*iIt)->print();
	}
    }

    void prettyPrint() {
	if (baseArray) baseArray->prettyPrint();
	for (std::vector<ExprDetails*>::iterator iIt = indexExprs.begin();
		iIt != indexExprs.end(); iIt++) {
	    if (*iIt) {
		llvm::outs() << "[";
		(*iIt)->prettyPrint();
		llvm::outs() << "]";
	    }
	}
    }

    bool equals(StmtDetails* sd) {
	if (!isa<ExprDetails>(sd)) return false;
	if (!(ExprDetails::equals(sd))) return false;
	if (!isa<ArraySubscriptExprDetails>(sd)) return false;
	ArraySubscriptExprDetails* ad = dyn_cast<ArraySubscriptExprDetails>(sd);
	if (!(ad->baseArray->equals(baseArray))) return false;
	if (ad->indexExprs.size() != indexExprs.size()) return false;
	for (unsigned i = 0; i < ad->indexExprs.size(); i++) {
	    if (!(ad->indexExprs[i]->equals(indexExprs[i]))) return false;
	}
	return true;
    }

    bool isSymExprCompletelyResolved(GetSymbolicExprVisitor* symVisitor, bool &rv);
    void callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv);

    static ArraySubscriptExprDetails* getObject(const SourceManager* SM,
    ArraySubscriptExpr* A, unsigned blockID, bool &rv, FunctionDetails* f = NULL);

    static bool classof (const StmtDetails* S) {
	return S->getKind() == ARRAYSUBEXPR;
    }
};

class UnaryOperatorDetails: public ExprDetails {
public:
    UnaryOperatorKind opKind;
    ExprDetails* opExpr;

    UnaryOperatorDetails(): ExprDetails(UNARYOPEXPR) {
	opExpr = NULL;
    }

    void print() {
	llvm::outs() << "UnaryOperatorDetails: ";
	if (opExpr) {
	    if (opKind == UnaryOperatorKind::UO_PostInc)
		llvm::outs() << "UO_PostInc";
	    else if (opKind == UnaryOperatorKind::UO_PostDec)
		llvm::outs() << "UO_PostDec";
	    else if (opKind == UnaryOperatorKind::UO_PreInc)
		llvm::outs() << "UO_PreInc";
	    else if (opKind == UnaryOperatorKind::UO_PreDec)
		llvm::outs() << "UO_PreDec";
	    else if (opKind == UnaryOperatorKind::UO_AddrOf)
		llvm::outs() << "UO_AddrOf";
	    else if (opKind == UnaryOperatorKind::UO_Deref)
		llvm::outs() << "UO_Deref";
	    else if (opKind == UnaryOperatorKind::UO_Plus)
		llvm::outs() << "UO_Plus";
	    else if (opKind == UnaryOperatorKind::UO_Minus)
		llvm::outs() << "UO_Minus";
	    else if (opKind == UnaryOperatorKind::UO_Not)
		llvm::outs() << "UO_Not";
	    else if (opKind == UnaryOperatorKind::UO_LNot)
		llvm::outs() << "UO_LNot";
	    else
		llvm::outs() << "Unknown opKind";
	    llvm::outs() << "\n";

	    opExpr->print();
	}
    }

    void prettyPrint() {
	if (opExpr) {
	    if (opKind == UnaryOperatorKind::UO_PreInc)
		llvm::outs() << "++";
	    else if (opKind == UnaryOperatorKind::UO_PreDec)
		llvm::outs() << "--";
	    else if (opKind == UnaryOperatorKind::UO_AddrOf)
		llvm::outs() << "&";
	    else if (opKind == UnaryOperatorKind::UO_Deref)
		llvm::outs() << "*";
	    else if (opKind == UnaryOperatorKind::UO_Minus)
		llvm::outs() << "-";
	    else if (opKind == UnaryOperatorKind::UO_Not)
		llvm::outs() << "~";
	    else if (opKind == UnaryOperatorKind::UO_LNot)
		llvm::outs() << "!";
	    opExpr->prettyPrint();
	    if (opKind == UnaryOperatorKind::UO_PostInc)
		llvm::outs() << "++";
	    else if (opKind == UnaryOperatorKind::UO_PostDec)
		llvm::outs() << "--";
	}
    }

    bool equals(StmtDetails* sd) {
	if (!isa<ExprDetails>(sd)) return false;
	if (!(ExprDetails::equals(sd))) return false;
	if (!isa<UnaryOperatorDetails>(sd)) return false;
	UnaryOperatorDetails* uod = dyn_cast<UnaryOperatorDetails>(sd);
	if (uod->opKind != opKind) return false;
	if (!(uod->opExpr->equals(opExpr))) return false;
	return true;
    }

    bool isSymExprCompletelyResolved(GetSymbolicExprVisitor* symVisitor, bool &rv);
    void callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv);

    static bool classof (const StmtDetails* S) {
	return S->getKind() == UNARYOPEXPR;
    }

    static UnaryOperatorDetails* getObject(const SourceManager* SM, 
    UnaryOperator* U, unsigned blockID, bool &rv, FunctionDetails* f = NULL);
};

class IntegerLiteralDetails: public ExprDetails {
public:
    llvm::APInt val;

    IntegerLiteralDetails(): ExprDetails(INTEGERLITERAL) {
    }

    void print() {
	// print takes a bool flag for isSigned
	val.print(llvm::outs(), val.isNegative());
	ExprDetails::print();
    }

    void prettyPrint() {
	val.print(llvm::outs(), val.isNegative());
    }

    bool equals(StmtDetails* sd) {
	if (!isa<ExprDetails>(sd)) return false;
	if (!(ExprDetails::equals(sd))) return false;
	if (!isa<IntegerLiteralDetails>(sd)) return false;
	IntegerLiteralDetails* id = dyn_cast<IntegerLiteralDetails>(sd);
	if (id->val != val) return false;
	return true;
    }

    bool isSymExprCompletelyResolved(GetSymbolicExprVisitor* symVisitor, bool &rv);
    void callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv);

    static IntegerLiteralDetails* getObject(const SourceManager* SM,
    IntegerLiteral* I, unsigned blockID, bool &rv, FunctionDetails* f = NULL);

    static bool classof(const StmtDetails* S) {
	return S->getKind() == INTEGERLITERAL;
    }
};

// Handling only 0i as an imaginary literal
class ImaginaryLiteralDetails: public ExprDetails {
public:
    llvm::APInt val;

    ImaginaryLiteralDetails(): ExprDetails(IMAGINARYLITERAL) {
    }

    void print() {
	// print takes a bool flag for isSigned
	val.print(llvm::outs(), val.isNegative());
	ExprDetails::print();
    }

    void prettyPrint() {
	val.print(llvm::outs(), val.isNegative());
    }

    bool equals(StmtDetails* sd) {
	if (!isa<ExprDetails>(sd)) return false;
	if (!(ExprDetails::equals(sd))) return false;
	if (!isa<ImaginaryLiteralDetails>(sd)) return false;
	ImaginaryLiteralDetails* id = dyn_cast<ImaginaryLiteralDetails>(sd);
	if (id->val != val) return false;
	return true;
    }

    bool isSymExprCompletelyResolved(GetSymbolicExprVisitor* symVisitor, bool &rv);
    void callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv);

    static ImaginaryLiteralDetails* getObject(const SourceManager* SM,
    ImaginaryLiteral* I, unsigned blockID, bool &rv, FunctionDetails* f = NULL);

    static bool classof(const StmtDetails* S) {
	return S->getKind() == IMAGINARYLITERAL;
    }
};

class FloatingLiteralDetails: public ExprDetails {
public:
    llvm::APFloat val;

    FloatingLiteralDetails(): ExprDetails(FLOATINGLITERAL),
    val(0.0) {
    }

    void print() {
	// print takes a bool flag for isSigned
	llvm::outs() << val.convertToDouble();
	ExprDetails::print();
    }

    void prettyPrint() {
	llvm::outs() << val.convertToDouble();
    }

    bool equals(StmtDetails* sd) {
	if (!isa<ExprDetails>(sd)) return false;
	if (!(ExprDetails::equals(sd))) return false;
	if (!isa<FloatingLiteralDetails>(sd)) return false;
	FloatingLiteralDetails* id = dyn_cast<FloatingLiteralDetails>(sd);
	if (!(val.bitwiseIsEqual(id->val))) return false;
	return true;
    }

    bool isSymExprCompletelyResolved(GetSymbolicExprVisitor* symVisitor, bool &rv);
    void callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv);

    static FloatingLiteralDetails* getObject(const SourceManager* SM,
    FloatingLiteral* I, unsigned blockID, bool &rv, FunctionDetails* f = NULL);

    static bool classof(const StmtDetails* S) {
	return S->getKind() == FLOATINGLITERAL;
    }
};

class CXXBoolLiteralExprDetails: public ExprDetails {
public:
    bool val;

    CXXBoolLiteralExprDetails(): ExprDetails(CXXBOOLLITERALEXPR) {
    }

    void print() {
	llvm::outs() << (val? "true": "false") << "\n";
	ExprDetails::print();
    }

    void prettyPrint() {
	llvm::outs() << (val? "true": "false");
    }

    bool equals(StmtDetails* sd) {
	if (!isa<ExprDetails>(sd)) return false;
	if (!(ExprDetails::equals(sd))) return false;
	if (!isa<CXXBoolLiteralExprDetails>(sd)) return false;
	CXXBoolLiteralExprDetails* id = dyn_cast<CXXBoolLiteralExprDetails>(sd);
	if (id->val != val) return false;
	return true;
    }

    bool isSymExprCompletelyResolved(GetSymbolicExprVisitor* symVisitor, bool &rv);
    void callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv);

    static CXXBoolLiteralExprDetails* getObject(const SourceManager* SM,
    CXXBoolLiteralExpr* I, unsigned blockID, bool &rv, FunctionDetails* f = NULL);

    static bool classof(const StmtDetails* S) {
	return S->getKind() == CXXBOOLLITERALEXPR;
    }
};

class CharacterLiteralDetails: public ExprDetails {
public:
    unsigned val;

    CharacterLiteralDetails(): ExprDetails(CHARACTERLITERAL) {
    }

    void print() {
	llvm::outs() << val << "\n";
	ExprDetails::print();
    }

    void prettyPrint() {
	llvm::outs() << "'" << val << "'";
    }

    bool equals(StmtDetails* sd) {
	if (!isa<ExprDetails>(sd)) return false;
	if (!(ExprDetails::equals(sd))) return false;
	if (!isa<CharacterLiteralDetails>(sd)) return false;
	CharacterLiteralDetails* id = dyn_cast<CharacterLiteralDetails>(sd);
	if (id->val != val) return false;
	return true;
    }

    bool isSymExprCompletelyResolved(GetSymbolicExprVisitor* symVisitor, bool &rv);
    void callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv);

    static CharacterLiteralDetails* getObject(const SourceManager* SM,
    CharacterLiteral* S, unsigned blockID, bool &rv, FunctionDetails* f = NULL);

    static bool classof(const StmtDetails* S) {
	return S->getKind() == CHARACTERLITERAL;
    }
};

class StringLiteralDetails: public ExprDetails {
public:
    StringRef val;

    StringLiteralDetails(): ExprDetails(STRINGLITERAL) {
    }

    void print() {
	llvm::outs() << val.str() << "\n";
	ExprDetails::print();
    }

    void prettyPrint() {
	llvm::outs() << val.str();
    }

    bool equals(StmtDetails* sd) {
	if (!isa<ExprDetails>(sd)) return false;
	if (!(ExprDetails::equals(sd))) return false;
	if (!isa<StringLiteralDetails>(sd)) return false;
	StringLiteralDetails* id = dyn_cast<StringLiteralDetails>(sd);
	if (id->val != val) return false;
	return true;
    }

    bool isSymExprCompletelyResolved(GetSymbolicExprVisitor* symVisitor, bool &rv);
    void callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv);

    static StringLiteralDetails* getObject(const SourceManager* SM,
    StringLiteral* S, unsigned blockID, bool &rv, FunctionDetails* f = NULL);

    static bool classof(const StmtDetails* S) {
	return S->getKind() == STRINGLITERAL;
    }
};

class BinaryOperatorDetails: public ExprDetails {
public:
    BinaryOperatorKind opKind;
    ExprDetails* lhs;
    ExprDetails* rhs;

    BinaryOperatorDetails(): ExprDetails(BINARYOPEXPR) {
    }

    void print() {
	if (opKind == BO_PtrMemD)
	    llvm::outs() << "BO_PtrMemD";
	else if (opKind == BO_PtrMemI)
	    llvm::outs() << "BO_PtrMemI";
	else if (opKind == BO_Mul)
	    llvm::outs() << "BO_Mul";
	else if (opKind == BO_Div)
	    llvm::outs() << "BO_Div";
	else if (opKind == BO_Rem)
	    llvm::outs() << "BO_Rem";
	else if (opKind == BO_Add)
	    llvm::outs() << "BO_Add";
	else if (opKind == BO_Sub)
	    llvm::outs() << "BO_Sub";
	else if (opKind == BO_Shl)
	    llvm::outs() << "BO_Shl";
	else if (opKind == BO_Shr)
	    llvm::outs() << "BO_Shr";
	else if (opKind == BO_LT)
	    llvm::outs() << "BO_LT";
	else if (opKind == BO_GT)
	    llvm::outs() << "BO_GT";
	else if (opKind == BO_LE)
	    llvm::outs() << "BO_LE";
	else if (opKind == BO_GE)
	    llvm::outs() << "BO_GE";
	else if (opKind == BO_EQ)
	    llvm::outs() << "BO_EQ";
	else if (opKind == BO_NE)
	    llvm::outs() << "BO_NE";
	else if (opKind == BO_And)
	    llvm::outs() << "BO_And";
	else if (opKind == BO_Xor)
	    llvm::outs() << "BO_Xor";
	else if (opKind == BO_Or)
	    llvm::outs() << "BO_Or";
	else if (opKind == BO_LAnd)
	    llvm::outs() << "BO_LAnd";
	else if (opKind == BO_LOr)
	    llvm::outs() << "BO_LOr";
	else if (opKind == BO_Assign)
	    llvm::outs() << "BO_Assign";
	else if (opKind == BO_MulAssign)
	    llvm::outs() << "BO_MulAssign";
	else if (opKind == BO_DivAssign)
	    llvm::outs() << "BO_DivAssign";
	else if (opKind == BO_RemAssign)
	    llvm::outs() << "BO_RemAssign";
	else if (opKind == BO_AddAssign)
	    llvm::outs() << "BO_AddAssign";
	else if (opKind == BO_SubAssign)
	    llvm::outs() << "BO_SubAssign";
	else if (opKind == BO_ShlAssign)
	    llvm::outs() << "BO_ShlAssign";
	else if (opKind == BO_ShrAssign)
	    llvm::outs() << "BO_ShrAssign";
	else if (opKind == BO_AndAssign)
	    llvm::outs() << "BO_AndAssign";
	else if (opKind == BO_XorAssign)
	    llvm::outs() << "BO_XorAssign";
	else if (opKind == BO_OrAssign)
	    llvm::outs() << "BO_OrAssign";
	else if (opKind == BO_Comma)
	    llvm::outs() << "BO_Comma";
	else
	    llvm::outs() << "Unknown operator";
	llvm::outs() << "\n";
	llvm::outs() << "LHS: ";
	lhs->print();
	llvm::outs() << "RHS: ";
	rhs->print();
	llvm::outs() << "\n";
    }

    void prettyPrint() {
	lhs->prettyPrint();
	if (opKind == BO_PtrMemD)
	    llvm::outs() << "BO_PtrMemD";
	else if (opKind == BO_PtrMemI)
	    llvm::outs() << "BO_PtrMemI";
	else if (opKind == BO_Mul)
	    llvm::outs() << " * ";
	else if (opKind == BO_Div)
	    llvm::outs() << " / ";
	else if (opKind == BO_Rem)
	    llvm::outs() << " % ";
	else if (opKind == BO_Add)
	    llvm::outs() << " + ";
	else if (opKind == BO_Sub)
	    llvm::outs() << " - ";
	else if (opKind == BO_Shl)
	    llvm::outs() << "BO_Shl";
	else if (opKind == BO_Shr)
	    llvm::outs() << "BO_Shr";
	else if (opKind == BO_LT)
	    llvm::outs() << " < ";
	else if (opKind == BO_GT)
	    llvm::outs() << " > ";
	else if (opKind == BO_LE)
	    llvm::outs() << " <= ";
	else if (opKind == BO_GE)
	    llvm::outs() << " >= ";
	else if (opKind == BO_EQ)
	    llvm::outs() << " == ";
	else if (opKind == BO_NE)
	    llvm::outs() << " != ";
	else if (opKind == BO_And)
	    llvm::outs() << " & ";
	else if (opKind == BO_Xor)
	    llvm::outs() << "BO_Xor";
	else if (opKind == BO_Or)
	    llvm::outs() << " | ";
	else if (opKind == BO_LAnd)
	    llvm::outs() << " && ";
	else if (opKind == BO_LOr)
	    llvm::outs() << " || ";
	else if (opKind == BO_Assign)
	    llvm::outs() << " = ";
	else if (opKind == BO_MulAssign)
	    llvm::outs() << " *= ";
	else if (opKind == BO_DivAssign)
	    llvm::outs() << " /= ";
	else if (opKind == BO_RemAssign)
	    llvm::outs() << " %= ";
	else if (opKind == BO_AddAssign)
	    llvm::outs() << " += ";
	else if (opKind == BO_SubAssign)
	    llvm::outs() << " -= ";
	else if (opKind == BO_ShlAssign)
	    llvm::outs() << "BO_ShlAssign";
	else if (opKind == BO_ShrAssign)
	    llvm::outs() << "BO_ShrAssign";
	else if (opKind == BO_AndAssign)
	    llvm::outs() << "BO_AndAssign";
	else if (opKind == BO_XorAssign)
	    llvm::outs() << "BO_XorAssign";
	else if (opKind == BO_OrAssign)
	    llvm::outs() << "BO_OrAssign";
	else if (opKind == BO_Comma)
	    llvm::outs() << " , ";
	else
	    llvm::outs() << "Unknown operator";
	rhs->prettyPrint();
    }

    bool equals(StmtDetails* sd) {
	if (!isa<ExprDetails>(sd)) return false;
	if (!(ExprDetails::equals(sd))) return false;
	if (!isa<BinaryOperatorDetails>(sd)) return false;
	BinaryOperatorDetails* bo = dyn_cast<BinaryOperatorDetails>(sd);
	if (bo->opKind != opKind) return false;
	if (!(bo->lhs->equals(lhs))) return false;
	if (!(bo->rhs->equals(rhs))) return false;
	return true;
    }

    bool isSymExprCompletelyResolved(GetSymbolicExprVisitor* symVisitor, bool &rv);
    void callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv);

    static BinaryOperatorDetails* getObject(const SourceManager* SM,
    BinaryOperator* B, unsigned blockID, bool &rv, FunctionDetails* f = NULL);

    static bool classof (const StmtDetails* S) {
	return S->getKind() == BINARYOPEXPR;
    }
};

class ConditionalOperatorDetails: public ExprDetails {
public:
    ExprDetails* condition;
    ExprDetails* trueExpr;
    ExprDetails* falseExpr;

    ConditionalOperatorDetails(): ExprDetails(CONDITIONALOPEXPR) {
    }

    void print() {
	llvm::outs() << "Condition:";
	condition->print();
	llvm::outs() << "trueExpr: ";
	trueExpr->print();
	llvm::outs() << "falseExpr: ";
	falseExpr->print();
	llvm::outs() << "\n";
    }

    void prettyPrint() {
	llvm::outs() << "(";
	condition->prettyPrint();
	llvm::outs() << " ? ";
	trueExpr->prettyPrint();
	llvm::outs() << " : ";
	falseExpr->prettyPrint();
    }

    bool equals(StmtDetails* sd) {
	if (!isa<ExprDetails>(sd)) return false;
	if (!(ExprDetails::equals(sd))) return false;
	if (!isa<ConditionalOperatorDetails>(sd)) return false;
	ConditionalOperatorDetails* bo = dyn_cast<ConditionalOperatorDetails>(sd);
	if (!(bo->condition->equals(condition))) return false;
	if (!(bo->trueExpr->equals(trueExpr))) return false;
	if (!(bo->falseExpr->equals(falseExpr))) return false;
	return true;
    }

    bool isSymExprCompletelyResolved(GetSymbolicExprVisitor* symVisitor, bool &rv);
    void callVisitor(GetSymbolicExprVisitor* symVisitor, bool &rv);

    static ConditionalOperatorDetails* getObject(const SourceManager* SM,
    ConditionalOperator* C, unsigned blockID, bool &rv, FunctionDetails* f = NULL);

    static bool classof (const StmtDetails* S) {
	return S->getKind() == CONDITIONALOPEXPR;
    }
};
#endif /* STMTDETAILS_H */
