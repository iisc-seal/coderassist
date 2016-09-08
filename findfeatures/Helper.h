#ifndef HELPER_H
#define HELPER_H

#include "includes.h"
#include "clang/Analysis/CFG.h"
#include <cctype>
#include <z3++.h>

class VarDetails;
class FunctionDetails;
class ReturnStmtDetails;
class SymbolicExpr;
class SymbolicArraySubscriptExpr;
class LoopDetails;

enum StmtKind {
    EXPR, 
	DECLREFEXPR, 
	ARRAYSUBEXPR,
	    ARRAYRANGEEXPR,
	ARRAYSUBEXPREND,
	CALLEXPR, 
	    CXXOPERATORCALLEXPR,
	CALLEXPREND,
	INITLISTEXPR,
	UNARYEXPRORTYPETRAITEXPR,
	UNARYOPEXPR,
	BINARYOPEXPR,
	CONDITIONALOPEXPR,
	INTEGERLITERAL,
	IMAGINARYLITERAL,
	FLOATINGLITERAL,
	CXXBOOLLITERALEXPR,
	CHARACTERLITERAL,
	STRINGLITERAL,
	SPECIAL,
    EXPREND,
    DECL, 
	VARDECL, 
    DECLEND,
    BREAK, 
    COMPOUND, 
    CONTINUE, 
    FOR, 
    IF, 
    NULLST, 
    RETURN,
    SWCASE, 
    SWITCH, 
    WHILE,
    UNKNOWNSTMT, 
};

enum CXXOperatorKind {
    CIN, COUT, UNKNOWNOP
};

enum DeclRefKind {
    VAR, FUNC, LIB, UNKNOWNREF
};

enum SpecialExprKind {
    MAX, MIN, SUM, UNRESOLVED
};

typedef llvm::DenseMap<const Stmt*,std::pair<unsigned,unsigned> > StmtMapTy;
typedef llvm::DenseMap<const Decl*,std::pair<unsigned,unsigned> > DeclMapTy;

CFGBlock* getBlockFromID(std::unique_ptr<CFG> &cfg, unsigned blockID);

std::vector<std::vector<std::pair<unsigned, bool> > > appendGuards(
std::vector<std::vector<std::pair<unsigned, bool> > > origGuards,
std::vector<std::vector<std::pair<unsigned, bool> > > newGuards);

std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendGuards(
std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > origGuards,
std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > newGuards, bool &rv);

std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > negateConjunction(
std::vector<std::pair<SymbolicExpr*, bool> > origGuard);

std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
negateGuard(
std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > origGuard, 
bool &rv);

class Helper {
  public:

    enum VarType {
	BOOLVAR,
	CHARVAR,
	INTVAR,
	FLOATVAR,
	BOOLARR,
	CHARARR,
	INTARR,
	FLOATARR,
	VECTOR,
	UNKNOWNVAR
    };

    enum ValueType {
	BOOL,
	INT,
	VAR,
	UNKNOWNVAL
    };

    static void printVarType(VarType t);

    static void printValueType(ValueType t);

    static ValueType getValueTypeFromInt(int x);

    static VarType getVarType(const Type* ExprType, std::string &size1,
	std::string &size2);
    static VarType getArrayVarType(VarType t, bool &rv);

    static ValueType getValueType(const Type* type);

    static bool indexPresentInString(std::string Index, std::string String);

    static void getIdentifiers(std::string sourceExpr, std::set<std::string>
&idSet);

    static void str_replace( std:: string &s, const std::string &search, 
	const std::string &replace );

    static void replaceIdentifiers(std::string &sourceString, 
	const std::string &searchString, const std::string &replaceString); 

    static std::string prettyPrintExpr(const clang::Expr* E); 

    static std::string prettyPrintStmt(const clang::Stmt* S); 

    static const Type* getTypeOfExpr(const clang::Expr* E);

    static bool isLiteral(std::string str, Helper::ValueType &val);

    static bool findFunctionCallExpr(std::string &sourceStr, 
	std::string funcName, int numOfArgsToFunc, 
	std::string returnExpr);

    static std::string simplifyBound(const Expr* boundExpr, bool addOrSub, 
	int amount);

    static std::pair<int, int> getFunctionExpr(std::string sourceExpr,
	std::string fName);

    static std::pair<std::pair<int, int>, std::string> getArrayExpr(std::string sourceExpr,
	std::string aName);

    static bool isNumber(std::string str);

    static bool isKeyword(std::string str);

    static std::string replaceFormalWithActual(std::string sourceString,
	std::map<std::string, std::string> formalToActualMap);

    static bool isInputStmt(const Stmt* S);

    static std::vector<const Expr*> getInputVarsFromStmt(const Stmt* S);

    static const Expr* getBaseArrayExpr(const ArraySubscriptExpr* array);
    static Expr* getBaseArrayExpr(ArraySubscriptExpr* array);
    static std::vector<Expr*> getArrayIndexExprs(ArraySubscriptExpr* array);

    static Expr* getStreamOfCXXOperatorCallExpr(CXXOperatorCallExpr* C, bool &rv);
    static std::vector<Expr*> getArgsToCXXOperatorCallExpr(CXXOperatorCallExpr*
    C, bool &rv);

    static bool isVarDeclAnArray(const VarDecl* VD);

    static std::pair<VarType, std::vector<const ArrayType*> > getVarType(VarDecl* vd, 
    bool &rv);
    static std::pair<VarType, std::vector<const ArrayType*> > getVarType(const
    Type* t, bool &rv);

    //static void printVarDetails(VarDetails v);

    static VarDetails getVarDetailsFromExpr(const SourceManager* SM, const Expr* E,
    bool &rv);
    static VarDetails getVarDetailsFromDecl(const SourceManager* SM, const Decl* E,
    bool &rv);
    static FunctionDetails* getFunctionDetailsFromDecl(const SourceManager* SM,
    const Decl* D, bool &rv);
};

z3::sort getSortFromType(z3::context* c, Helper::VarType T, bool &rv);


class VarDetails {
public:
    std::string varID; // ID = varName + varDeclLine
    std::string varName;
    int varDeclLine;
    Helper::VarType type;
    std::vector<const ArrayType*> arraySizeInfo;
    std::vector<SymbolicExpr*> arraySizeSymExprs;
    enum VarKind {
	PARAMETER, RETURN, GLOBALVAR, OTHER
    };
    VarKind kind;

    VarDetails() {
	varID = "";
	varName = "";
	varDeclLine = -1;
	type = Helper::VarType::UNKNOWNVAR;
	kind = VarKind::OTHER;
    }

    bool isArray() {
	return (arraySizeInfo.size() > 0 || arraySizeSymExprs.size() > 0);
    }

    void printArrayType();

    void setVarID() {
	std::stringstream ss;
	ss << varName << varDeclLine;
	varID = ss.str();
    }

    std::string getVarID() {
	setVarID();
	return varID;
    }

    void setArraySizeSymExprs(std::vector<SymbolicExpr*> symExprs) {
	arraySizeSymExprs.clear();
	arraySizeSymExprs.insert(arraySizeSymExprs.end(), symExprs.begin(),
	    symExprs.end());
    }

    static VarDetails createReturnVar(Helper::VarType t, std::vector<const
	    ArrayType*> sizeInfo) {
	VarDetails v;
	v.varName = "DP_FUNC_RET";
	v.type = t;
	v.arraySizeInfo.insert(v.arraySizeInfo.end(), sizeInfo.begin(), 
	    sizeInfo.end());
	v.kind = VarKind::RETURN;
	v.setVarID();
	return v;
    }

    VarDetails clone() {
	VarDetails cloneObj;
	cloneObj.varID = varID;
	cloneObj.varName = varName;
	cloneObj.varDeclLine = varDeclLine;
	cloneObj.type = type;
	cloneObj.arraySizeInfo.insert(cloneObj.arraySizeInfo.end(),
	    arraySizeInfo.begin(), arraySizeInfo.end());
	cloneObj.arraySizeSymExprs.insert(cloneObj.arraySizeSymExprs.end(),
	    arraySizeSymExprs.begin(), arraySizeSymExprs.end());
	cloneObj.kind = kind;
	return cloneObj;
    }

    void print();

    bool equals(VarDetails v);
};

class FunctionDetails {
private:
    FunctionDetails(FunctionDetails const &);
    FunctionDetails& operator=(FunctionDetails const &);
public:
    std::string funcName;
    int funcStartLine;
    int funcEndLine;
    bool isCustomInputFunction;
    bool isCustomOutputFunction;
    FunctionDecl const *fd;
    std::unique_ptr<CFG> cfg;
    // Store detials to construct CFG
    Stmt* funcBody;
    ASTContext* C;

    std::vector<VarDetails> parameters;
    bool isLibraryFunction;

    FunctionDetails() {
	funcName = "";
	funcStartLine = -1;
	funcEndLine = -1;
	isCustomInputFunction = false;
	isCustomOutputFunction = false;
	fd = NULL;
	isLibraryFunction = false;
    }

    FunctionDetails& operator=(FunctionDetails &&f) {
	if (this != &f) {
	    funcName = f.funcName;
	    funcStartLine = f.funcStartLine;
	    funcEndLine = f.funcEndLine;
	    isCustomInputFunction = f.isCustomInputFunction;
	    isCustomOutputFunction = f.isCustomOutputFunction;
	    fd = f.fd;
	    cfg = std::move(f.cfg);
	    funcBody = f.funcBody;
	    C = f.C;
	}
	return *this;
    }

    std::unique_ptr<CFG> constructCFG(bool &rv) {
	rv = true;
	if (!fd) {
	    llvm::outs() << "ERROR: FunctionDecl is NULL\n";
	    rv = false;
	    return NULL;
	}
	if (!funcBody) {
	    llvm::outs() << "ERROR: FunctionBody is NULL\n";
	    rv = false;
	    return NULL;
	}
	if (!C) {
	    llvm::outs() << "ERROR: ASTContext is NULL\n";
	    rv = false;
	    return NULL;
	}
	CFG::BuildOptions bo;
	CFG::BuildOptions fbo = bo.setAllAlwaysAdd();
	return CFG::buildCFG(fd, funcBody, C, fbo);
    }

    FunctionDetails clone() {
	FunctionDetails cloneObj;
	cloneObj.funcName = funcName;
	cloneObj.funcStartLine = funcStartLine;
	cloneObj.funcEndLine = funcEndLine;
	cloneObj.isCustomInputFunction = isCustomInputFunction;
	cloneObj.isCustomOutputFunction = isCustomOutputFunction;
	cloneObj.fd = fd;
	cloneObj.funcBody = funcBody;
	cloneObj.C = C;
	for (std::vector<VarDetails>::iterator pIt = parameters.begin();
		pIt != parameters.end(); pIt++)
	    cloneObj.parameters.push_back(pIt->clone());
	cloneObj.isLibraryFunction = isLibraryFunction;
	return cloneObj;
    }

    FunctionDetails* cloneAsPtr() {
	FunctionDetails* cloneObj = new FunctionDetails;
	cloneObj->funcName = funcName;
	cloneObj->funcStartLine = funcStartLine;
	cloneObj->funcEndLine = funcEndLine;
	cloneObj->isCustomInputFunction = isCustomInputFunction;
	cloneObj->isCustomOutputFunction = isCustomOutputFunction;
	cloneObj->fd = fd;
	cloneObj->funcBody = funcBody;
	cloneObj->C = C;
	for (std::vector<VarDetails>::iterator pIt = parameters.begin();
		pIt != parameters.end(); pIt++)
	    cloneObj->parameters.push_back(pIt->clone());
	cloneObj->isLibraryFunction = isLibraryFunction;
	return cloneObj;
    }

    bool equals(FunctionDetails* f) {
	if (funcName.compare(f->funcName) != 0) return false;
	if (funcStartLine != f->funcStartLine) return false;
	if (funcEndLine != f->funcEndLine) return false;
	if (isLibraryFunction != f->isLibraryFunction) return false;
	return true;
    }
    void print();
};

class InputVar {
public:
    int progID;	// Program-level ID representing global order
    std::string funcID; // Function-level ID: funcName + ID

    VarDetails var;
    Expr* varExpr;
    int inputCallLine;	// linenumber in which this var is read.
    FunctionDetails* func; // function in which this var is read.
    bool isDummyScalarInput; // true if this input var is the dummy scalar used
			     // to assign to an input array.
    enum InputFunc {
	CIN, SCANF, FSCANF, GETCHAR, CUSTOM
    };
    InputFunc inputFunction;
    std::vector<SymbolicExpr*> sizeExprs;

    // Store details for substitute array corresponding to this var
    struct subArrayType {
	SymbolicArraySubscriptExpr* array;
	std::vector<LoopDetails*> loops;
    };

    subArrayType substituteArray;

    InputVar() {
	progID = -1;
	funcID = "";
	varExpr = NULL;
	isDummyScalarInput = false;
	substituteArray.array = NULL;
    }

    InputVar clone();

    bool equals(InputVar v) {
	if (!(var.equals(v.var))) return false;
	if (inputCallLine != v.inputCallLine) return false;
	if (!(func->equals(v.func))) return false;
	if (inputFunction != v.inputFunction) return false;
	if (isDummyScalarInput != v.isDummyScalarInput) return false;
	return true;
    }

    void print();

    bool operator< (InputVar v) const {
	return inputCallLine < v.inputCallLine;
    }
};

class DPVar {
public:
    static int counter;
    int id;
    VarDetails dpArray;
    std::vector<SymbolicExpr*> sizeExprs;
    bool toDelete;
    bool hasUpdate;
    bool hasInit;

    DPVar(VarDetails v) {
	counter++;
	dpArray = v.clone();
	id = counter;
	toDelete = false;
	hasUpdate = false;
	hasInit = false;
    }

    DPVar() {
    }

    void print();

    DPVar clone(bool &rv);

};

// trim from start
static inline std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(),
std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int,
int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
}
#endif /* HELPER_H */
