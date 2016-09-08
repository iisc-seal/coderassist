#include "Helper.h"
#include "StmtDetails.h"
#include "GetSymbolicExprVisitor.h"
#include <cctype>

CFGBlock* getBlockFromID(std::unique_ptr<CFG> &cfg, unsigned blockID) {
    for (CFG::iterator bIt = cfg->begin(); bIt !=
	    cfg->end(); bIt++) {
	if ((*bIt)->getBlockID() == blockID)
	    return *bIt;
    }
    return NULL;
}

void Helper::printVarType(Helper::VarType t) {
    switch(t) {
	case BOOLVAR:
	    llvm::outs() << "BOOLVAR";
	    break;
	case INTVAR:
	    llvm::outs() << "INTVAR";
	    break;
	case FLOATVAR:
	    llvm::outs() << "FLOATVAR";
	    break;
	case BOOLARR:
	    llvm::outs() << "BOOLARR";
	    break;
	case INTARR:
	    llvm::outs() << "INTARR";
	    break;
	case FLOATARR:
	    llvm::outs() << "FLOATARR";
	    break;
	case UNKNOWNVAR:
	    llvm::outs() << "UNKNOWNVAR";
	    break;
	default:
	    llvm::outs() << "Unknown VarType";
    }
}

void Helper::printValueType(Helper::ValueType t) {
    switch(t) {
	case BOOL:
	    llvm::outs() << "BOOL";
	    break;
	case INT:
	    llvm::outs() << "INT";
	    break;
	case VAR:
	    llvm::outs() << "VAR";
	    break;
	case UNKNOWNVAL:
	    llvm::outs() << "UNKNOWNVAL";
	    break;
    }
}

Helper::ValueType Helper::getValueTypeFromInt(int x) {
    switch(x) {
	case 0: return BOOL;
	case 1: return INT;
	case 2: return VAR;
	default: return UNKNOWNVAL;
    }
}

Helper::VarType Helper::getVarType(const Type* ExprType, std::string &size1,
    std::string &size2) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering Helper::getVarType()\n";
    //ExprType->dump();
#endif
    VarType retValue;
    if (ExprType->isBooleanType()) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Expr type: Boolean\n";
#endif
	retValue = VarType::BOOLVAR;
    } else if (ExprType->isIntegerType()) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Expr type: Integer\n";
#endif
	retValue = VarType::INTVAR;
    } else if (ExprType->isArrayType()) {

#ifdef DEBUG
		llvm::outs() << "DEBUG: Expr type: Array Type\n";
#endif
                const ArrayType* ExprArrayType =
                    ExprType->getAsArrayTypeUnsafe();

		// Get dimensions.
		if (ExprArrayType->isConstantArrayType()) {
		    const ConstantArrayType* constArrayType =
			dyn_cast<ConstantArrayType>(ExprArrayType);
		    const uint64_t * size1D =
			constArrayType->getSize().getRawData();
		    if (!size1D) {
#ifdef ERRORMSG
			llvm::outs() << "ERROR: Cannot get size from ConstArrayType\n";
#endif
			return UNKNOWNVAR;
		    }
		    size1 = std::to_string(*size1D);
		} else if (ExprArrayType->isVariableArrayType()) {
		    const VariableArrayType* varArrayType =
			dyn_cast<VariableArrayType>(ExprArrayType);
		    const Expr* size1D =
			varArrayType->getSizeExpr();
		    if (!size1D) {
#ifdef ERRORMSG
			llvm::outs() << "ERROR: Cannot get size from VarArrayType\n";
#endif
			return UNKNOWNVAR;
		    }
		    size1 = Helper::prettyPrintExpr(size1D);
		}
                const Type* ExprArrayElementType =
                    ExprArrayType->getElementType().split().Ty;

		if (ExprArrayElementType->isIntegerType())
		    retValue = VarType::INTARR;
		else if (ExprArrayElementType->isBooleanType())
		    retValue = VarType::BOOLARR;
		//else if (ExprArrayElementType->isCharType())
		    //retValue = VarType::CHARARR;
		else if (ExprArrayElementType->isFloatingType())
		    retValue = VarType::FLOATARR;
		else if (ExprArrayElementType->isArrayType()) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Expr type: 2D Array Type\n";
#endif
		    const ArrayType* Expr2DArrayType =
			ExprArrayElementType->getAsArrayTypeUnsafe();

		    // Get dimensions.
		    if (Expr2DArrayType->isConstantArrayType()) {
			const ConstantArrayType* constArrayType =
			    dyn_cast<ConstantArrayType>(Expr2DArrayType);
			const uint64_t * size2D =
			    constArrayType->getSize().getRawData();
			if (!size2D) {
#ifdef ERRORMSG
			    llvm::outs() << "ERROR: Cannot get size from ConstArrayType\n";
#endif
			    return UNKNOWNVAR;
			}
			size2 = std::to_string(*size2D);
		    } else if (Expr2DArrayType->isVariableArrayType()) {
			const VariableArrayType* varArrayType =
			    dyn_cast<VariableArrayType>(Expr2DArrayType);
			const Expr* size2D =
			    varArrayType->getSizeExpr();
			if (!size2D) {
#ifdef ERRORMSG
			    llvm::outs() << "ERROR: Cannot get size from VarArrayType\n";
#endif
			    return UNKNOWNVAR;
			}
			size2 = Helper::prettyPrintExpr(size2D);
		    }

		    const Type* Expr2DArrayElementType =
			Expr2DArrayType->getElementType().split().Ty;

		    if (Expr2DArrayElementType->isIntegerType()) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: 2D Array Element Integer\n";
#endif
			retValue = VarType::INTARR;
		    } else if (Expr2DArrayElementType->isBooleanType()) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: 2D Array Element Boolean\n";
#endif
			retValue = VarType::BOOLARR;
		    //} else if (Expr2DArrayElementType->isCharType()) {
#ifdef DEBUG
			//llvm::outs() << "DEBUG: 2D Array Element Character\n";
#endif
			//retValue = VarType::CHARARR;
		    } else if (Expr2DArrayElementType->isFloatingType()) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: 2D Array Element Float\n";
#endif
			retValue = VarType::FLOATARR;
		    } else {
#ifdef DEBUG
			llvm::outs() << "DEBUG: 2D Array Element Unknown\n";
#endif
			retValue = VarType::UNKNOWNVAR;
		    }
		} else
		    retValue = VarType::UNKNOWNVAR;

#if 0
	const Type* tempType = ExprType;
	while (tempType->isArrayType()) {
	    const ArrayType* ExprArrayType =
		tempType->getAsArrayTypeUnsafe();
	    tempType = ExprArrayType->getElementType().split().Ty;
	}

	if (tempType->isBooleanType()) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Expr type: Boolean Array\n";
#endif
	    retValue = VarType::BOOLARR;
	} else if (tempType->isIntegerType()) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Expr type: Integer Array\n";
#endif
	    retValue = VarType::INTARR;
	} else {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Expr type: Unknown array type\n";
#endif
	    retValue = UNKNOWNVAR;
	}
#endif
    } else {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Expr type: Unknown\n";
#endif
	retValue = UNKNOWNVAR;
    }
#ifdef DEBUG
    llvm::outs() << "DEBUG: Finishing Helper::getVarType()\n";
#endif
    return retValue;
}

Helper::VarType Helper::getArrayVarType(Helper::VarType t, bool &rv) {
    rv = true;
    switch (t) {
    case BOOLVAR:  return BOOLARR;
    case CHARVAR:  return CHARARR;
    case INTVAR:   return INTARR;
    case FLOATVAR: return FLOATARR;
    default:
	rv = false;
	return UNKNOWNVAR;
    }
}

Helper::ValueType Helper::getValueType(const Type* type) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering Helper::getValueType()\n";
#endif

    Helper::ValueType retValue;

    if (type->isBooleanType()) {
#ifdef DEBUG
	llvm::outs() << "Boolean Type\n";
#endif
	retValue = Helper::ValueType::BOOL;
    } else if (type->isIntegerType()) {
#ifdef DEBUG
	llvm::outs() << "Integer Type\n";
#endif
	retValue = Helper::ValueType::INT;
    } else if (type->isArrayType() || type->isScalarType()) {
#ifdef DEBUG
	llvm::outs() << "Array or Scalar Var\n";
#endif
	retValue = Helper::ValueType::VAR;
    } else {
#ifdef DEBUG
	llvm::outs() << "Unknown Type\n";
#endif
	retValue = Helper::ValueType::UNKNOWNVAL;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Finishing Helper::getValueType()\n";
#endif

    return retValue;
}

bool Helper::indexPresentInString(std::string Index, std::string String) {
#if 0
    size_t pos = String.find(Index);
    int size = Index.length();
    if (pos == std::string::npos)
	return false;
    else {
	char prevChar, nextChar;
	if (pos != 0)
	    prevChar = String.at(pos-1);
	else
	    prevChar = -1;
	if (pos+size < String.length()-1)
	    nextChar = String.at(pos+size);
	else
	    nextChar = -1;

	bool prevCharFlag = true;
	bool nextCharFlag = true;
	if (prevChar != -1)
	    prevCharFlag = 
		(!(('a' <= prevChar && prevChar <= 'z') || 
		   ('A' <= prevChar && prevChar <= 'Z') ||
		   ('0' <= prevChar && prevChar <= '9') ||
		   (prevChar == '_')));
	if (nextChar != -1)
	    nextCharFlag = 
		(!(('a' <= nextChar && nextChar <= 'z') ||
		   ('A' <= nextChar && nextChar <= 'Z') ||
		   ('0' <= nextChar && nextChar <= '9') ||
		   (nextChar == '_')));
	if (prevCharFlag && nextCharFlag)
	    return true;
	else
	    return false;
    }
#endif

    std::set<std::string> idSet;
    Helper::getIdentifiers(String, idSet);
    for (std::set<std::string>::iterator it = idSet.begin(); 
	it != idSet.end(); it++) {
	if ((*it).compare(Index) == 0)
	    return true;
    }
    return false;
}

void Helper::getIdentifiers(std::string sourceExpr, std::set<std::string> &idSet) {
    std::string temp;
    for (unsigned i = 0; i < sourceExpr.size(); ) {
	while (!(sourceExpr[i] == '_' || isalnum(sourceExpr[i])) 
		&& i < sourceExpr.size()) {
#ifdef DEBUGDUMMY
	    llvm::outs() << "DEBUG: Skipping " << sourceExpr[i] << "\n";
#endif
	    i++;
	}

#ifdef DEBUGDUMMY
	llvm::outs() << "DEBUG: Starting to match identifier\n";
#endif

	temp = "";
	while ((sourceExpr[i] == '_' || isalnum(sourceExpr[i])) 
		&& i < sourceExpr.size()) {
#ifdef DEBUGDUMMY
	    llvm::outs() << sourceExpr[i] << "\n";
#endif
	    temp.push_back(sourceExpr[i]);
	    i++;
	}

#ifdef DEBUGDUMMY
	llvm::outs() << "DEBUG: Found identifier: " << temp << "\n";
#endif
	if (temp.compare("") != 0)
	    idSet.insert(temp);
    }

#ifdef DEBUGDUMMY
    llvm::outs() << "DEBUG: Matched identifiers\n";
    for (std::set<std::string>::iterator it = idSet.begin(); 
	    it != idSet.end(); it++) {
	llvm::outs() << *it << "\n";
    }
#endif
}

void Helper::str_replace( std:: string &s, const std::string &search, 
    const std::string &replace ) {
#if 0
    for( size_t pos = 0; ; pos += replace.length() ) {
        pos = s.find( search, pos );
        if( pos == std::string::npos ) break;
   
        s.erase( pos, search.length() );
        s.insert( pos, replace );
    }
#endif
    replaceIdentifiers(s, search, replace);
}

void Helper::replaceIdentifiers(std::string &sourceString, 
    const std::string &searchString, const std::string &replaceString) {
    std::string temp;
    std::string replacedString;
    for (unsigned i = 0; i < sourceString.size(); ) {
	temp = "";
	while (!(sourceString[i] == '_' || isalnum(sourceString[i])) 
		&& i < sourceString.size()) {
#ifdef DEBUGDUMMY
	    llvm::outs() << "DEBUG: Skipping " << sourceString[i] << "\n";
#endif
	    temp.push_back(sourceString[i]);
	    i++;
	}

	replacedString = replacedString + temp;

#ifdef DEBUGDUMMY
	llvm::outs() << "DEBUG: Starting to match identifier\n";
#endif

	temp = "";
	while ((sourceString[i] == '_' || isalnum(sourceString[i])) 
		&& i < sourceString.size()) {
#ifdef DEBUGDUMMY
	    llvm::outs() << sourceString[i] << "\n";
#endif
	    temp.push_back(sourceString[i]);
	    i++;
	}

#ifdef DEBUGDUMMY
	llvm::outs() << "DEBUG: Found identifier: " << temp << "\n";
#endif
	if (temp.compare("") != 0 && temp.compare(searchString) == 0)
	    replacedString = replacedString + replaceString;
	else if (temp.compare("") != 0)
	    replacedString = replacedString + temp;
    }

    sourceString = replacedString;
}

std::string Helper::prettyPrintExpr(const clang::Expr* E) {
    clang::LangOptions LangOpts;
    LangOpts.CPlusPlus = true;
    clang::PrintingPolicy Policy(LangOpts);

    std::string TypeS;
    llvm::raw_string_ostream s(TypeS);
    E->printPretty(s, 0, Policy);
    return s.str();
}

std::string Helper::prettyPrintStmt(const clang::Stmt* S) {
    clang::LangOptions LangOpts;
    LangOpts.CPlusPlus = true;
    clang::PrintingPolicy Policy(LangOpts);

    std::string TypeS;
    llvm::raw_string_ostream s(TypeS);
    S->printPretty(s, 0, Policy);
    return s.str();
}

const Type* Helper::getTypeOfExpr(const clang::Expr* E) {
    return E->getType().split().Ty;
}

bool Helper::isLiteral(std::string str, Helper::ValueType &val) {
    bool isNumber = true;
    for (std::string::iterator it = str.begin(); it != str.end(); it++) {
	if (!isdigit(*it)) {
	    isNumber = false;
	    break;
	}
    }
    if (isNumber) {
	val = Helper::ValueType::INT;
	return true;
    }

    if (str.compare("true") == 0 || str.compare("false") == 0) {
	val = Helper::ValueType::BOOL;
	return true;
    }

    return false;
}

bool Helper::findFunctionCallExpr(std::string &sourceStr, std::string funcName, 
	int numOfArgsToFunc, std::string returnExpr) {

#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering findFunctionCallExpr()\n";
#endif

    bool replaced = false;

    while (1) {
	std::pair<size_t, size_t> res;
#ifdef DEBUGDUMMY
	llvm::outs() << "DEBUG: Finding function " << funcName << " in string "
		     << sourceStr << "\n";
	llvm::outs() << "DEBUG: " << numOfArgsToFunc << " args to function\n";
#endif
	size_t ret = sourceStr.find(funcName);
	if (ret == std::string::npos) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Cannot find function " << funcName 
			 << " in string " << sourceStr << "\n";
#endif
	    return replaced;
	} else {
#ifdef DEBUGDUMMY
	    llvm::outs() << "DEBUG: find() returned that function is present in "
			 << "string\n";
#endif
	}

	res.first = ret;
	int sizeOfFuncName = funcName.length();
	ret = ret + sizeOfFuncName + 1;

	std::string arg = "";
	bool matchedFunctionCall = false;
	int numArgs = 0;

#ifdef DEBUGDUMMY
	llvm::outs() << "DEBUG: Entering loop\n";
#endif
	std::string stack = "";
	for (; ret < sourceStr.size(); ret++) {
#ifdef DEBUGDUMMY
	    llvm::outs() << "DEBUG: sourceStr[" << ret << "]: " << sourceStr[ret]
			 << "\n";
	    llvm::outs() << "DEBUG: stack: " << stack << "(" << stack.size()
			 << ")\n";
	    llvm::outs() << "DEBUG: numArgs: " << numArgs << "\n";
	    llvm::outs() << "DEBUG: arg: " << arg << "\n";
#endif
	    if (sourceStr[ret] != '(' && sourceStr[ret] != ',' && 
		sourceStr[ret] != ')') {
		arg.push_back(sourceStr[ret]);
		continue;
	    }

	    if (sourceStr[ret] == '(') {
		stack.push_back('(');
		continue;
	    }

	    if (sourceStr[ret] == ')') {
		if (stack.size() == 0) {
#ifdef DEBUGDUMMY
		    llvm::outs() << "DEBUG: stack size is 0, I should be "
				 << "incrementing numArgs now\n";
#endif
		    matchedFunctionCall = true;
		    numArgs++;
		    break;
		}

		if (stack[stack.size() - 1] == '(') {
		    stack.erase(stack.size() - 1);
		    continue;
		} else {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Saw ), but cannot find the matching (\n";
		    return replaced;
#endif
		}

	    }

	    if (sourceStr[ret] == ',') {
#ifdef DEBUGDUMMY
		llvm::outs() << "DEBUG: Saw comma, I should be incrementing "
			     << "numArgs now\n";
#endif
		numArgs++;
		arg = "";
	    } else {
#ifdef DEBUGDUMMY
		llvm::outs() << "DEBUG: Didn't see comma\n";
#endif
	    }
	}

	// Checking if we matched the entire function call expression We check
	// whether the number of arguments to the function call is same as the
	// number passed as argument. This might have to change since functions
	// can have default arguments that need not occur in a call. Now there
	// is a TODO!
	if (matchedFunctionCall && numArgs == numOfArgsToFunc) {
	    res.second = ret;
	    sourceStr.replace(res.first, (res.second - res.first) + 1, returnExpr);
	    replaced = true;
	} else {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Cannot find function call in " << sourceStr 
			 << "\n";
#endif
	    return replaced;
	}
    }

    return false;
}

// We need to get bound +/- amount
// addOrSub: true if add, false if sub
std::string Helper::simplifyBound(const Expr* boundExpr, bool addOrSub, int amount) {
    if (isa<BinaryOperator>(boundExpr)) {
	const BinaryOperator* boundExprBinaryOperator = 
	    dyn_cast<BinaryOperator>(boundExpr);
	if (!boundExprBinaryOperator) {
#ifdef ERROR
	    llvm::outs() << "ERROR: Cannot get BinaryOperator of boundExpr\n";
#endif
	    return "";
	}

	if (boundExprBinaryOperator->isAdditiveOp()) {
	    const Expr* LHSAdd = boundExprBinaryOperator->getLHS();
	    const Expr* RHSAdd = boundExprBinaryOperator->getRHS();
	    if (!LHSAdd || !RHSAdd) {
#ifdef ERROR
		llvm::outs() << "ERROR: Cannot get LHS/RHS expression\n";
#endif
		return "";
	    }

	    if (isa<IntegerLiteral>(LHSAdd)) {
		const IntegerLiteral* LHSAddInt = 
		    dyn_cast<IntegerLiteral>(LHSAdd);
		const uint64_t* lhsInt = LHSAddInt->getValue().getRawData();
		if (!lhsInt) {
#ifdef ERROR
		    llvm::outs() << "ERROR: Cannot get lhsInt from "
				 << "IntegerLiteral\n";
#endif
		    return "";
		}

		int lhsInteger = *lhsInt;
		if (addOrSub)
		    lhsInteger = lhsInteger + amount;
		else
		    lhsInteger = lhsInteger - amount;

		std::string boundString = "";
		if (boundExprBinaryOperator->getOpcode() ==
			clang::BinaryOperatorKind::BO_Add) {
		    if (lhsInteger == 0)
			boundString = Helper::prettyPrintExpr(RHSAdd);
		    else
			boundString = std::to_string(lhsInteger) + "+" +
			    Helper::prettyPrintExpr(RHSAdd);
		} else if (boundExprBinaryOperator->getOpcode() ==
			    clang::BinaryOperatorKind::BO_Sub) {
		    if (lhsInteger == 0)
			boundString = Helper::prettyPrintExpr(RHSAdd);
		    else
			boundString = std::to_string(lhsInteger) + "-" +
			    Helper::prettyPrintExpr(RHSAdd);
		}
		return boundString;
	    }

	    if (isa<IntegerLiteral>(RHSAdd)) {
		const IntegerLiteral* RHSAddInt = 
		    dyn_cast<IntegerLiteral>(RHSAdd);
		const uint64_t* rhsInt = RHSAddInt->getValue().getRawData();
		if (!rhsInt) {
#ifdef ERROR
		    llvm::outs() << "ERROR: Cannot get rhsInt from "
				 << "IntegerLiteral\n";
#endif
		    return "";
		}

		int rhsInteger = *rhsInt;
		if (addOrSub)
		    rhsInteger = rhsInteger + amount;
		else
		    rhsInteger = rhsInteger - amount;

		std::string boundString = "";
		if (boundExprBinaryOperator->getOpcode() ==
			clang::BinaryOperatorKind::BO_Add) {
		    if (rhsInteger == 0)
			boundString = Helper::prettyPrintExpr(LHSAdd);
		    else
			boundString = Helper::prettyPrintExpr(LHSAdd) +
			    "+" + std::to_string(rhsInteger);
		} else if (boundExprBinaryOperator->getOpcode() ==
			    clang::BinaryOperatorKind::BO_Sub) {
		    if (rhsInteger == 0)
			boundString = Helper::prettyPrintExpr(LHSAdd);
		    else
			boundString = Helper::prettyPrintExpr(LHSAdd) +
			    "-" + std::to_string(rhsInteger);
		}
		return boundString;
	    }
	}
    } else if (isa<IntegerLiteral>(boundExpr)) {
	const IntegerLiteral* boundLiteral = dyn_cast<IntegerLiteral>(boundExpr);
	if (!boundLiteral) {
#ifdef ERRORMSG
	    llvm::outs() << "ERROR: Cannot get IntegerLiteral for boundExpr\n";
#endif
	    return "";
	}

	const uint64_t* bound = boundLiteral->getValue().getRawData();
	if (!bound) {
#ifdef ERRORMSG
	    llvm::outs() << "ERROR: Cannot get bound rawdata\n";
#endif
	    return "";
	}

	uint64_t newBound;
	if (addOrSub)
	    newBound = *bound + amount;
	else
	    newBound = *bound - amount;

	return std::to_string(newBound);
    }

    std::stringstream ss;
    ss << Helper::prettyPrintExpr(boundExpr);
    if (addOrSub)
	ss << " + ";
    else
	ss << " - ";
    ss << amount;
    return ss.str();
    //return Helper::prettyPrintExpr(boundExpr);
}

std::pair<int, int> Helper::getFunctionExpr(std::string sourceExpr, 
    std::string fName) {

    unsigned i = 0;
    unsigned fNameStart = 0, fNameEnd = 0;
    bool found = false;
    while (i < sourceExpr.size()) {
	if (sourceExpr[i] != fName[0]) {
	    i++;
	    continue;
	}

	unsigned j = 0;
	fNameStart = i;
	while (i < sourceExpr.size() && j < fName.size()) { 
	    if (sourceExpr[i] != fName[j])
		break;
	    i++; j++;
	}

	if (j == fName.size()) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Matched " << fName << " at " << fNameStart 
			 << "\n";
#endif

	    while (i < sourceExpr.size()) { 
		if (sourceExpr[i] == ' ') i++;
		else break;
	    }

	    if (sourceExpr[i] != '(')
		continue;

	    found = true;
	    std::vector<char> stack;
	    while (i < sourceExpr.size()) {
		if (sourceExpr[i] == '(')
		    stack.push_back('(');
		if (sourceExpr[i] == ')') {
		    stack.pop_back();
		    if (stack.size() == 0)
			break;
		}
		i++;
	    }
	    if (i != sourceExpr.size())
		fNameEnd = i;
	}
    }

    std::pair<int, int> ret;
    if (found) {
	ret.first = fNameStart;
	ret.second = fNameEnd;
    } else {
	ret.first = -1;
	ret.second = -1;
    }
    return ret;
}

std::pair<std::pair<int, int>, std::string> Helper::getArrayExpr(std::string sourceExpr, 
    std::string aName) {

    unsigned i = 0;
    unsigned aNameStart = 0, aNameEnd = 0;
    std::string temp = "";
    bool found = false;
    while (i < sourceExpr.size()) {
	if (sourceExpr[i] != aName[0]) {
	    i++;
	    continue;
	}

	unsigned j = 0;
	aNameStart = i;
	while (i < sourceExpr.size() && j < aName.size()) { 
	    if (sourceExpr[i] != aName[j])
		break;
	    i++; j++;
	}

	if (j == aName.size()) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Matched " << aName << " at " << aNameStart 
			 << "\n";
#endif

	    while (i < sourceExpr.size()) {
		if (sourceExpr[i] == ' ') {
		    i++; continue;
		} else
		    break;
	    }

	    if (sourceExpr[i] != '[')
		continue;

	    found = true;
	    std::vector<char> stack;
	    stack.push_back('[');
	    i++;
	    temp = "";
	    while (i < sourceExpr.size()) {
		if (sourceExpr[i] == '[')
		    stack.push_back('[');
		if (sourceExpr[i] == ']') {
		    stack.pop_back();
		    if (stack.size() == 0)
			break;
		}
		temp.push_back(sourceExpr[i]);
		i++;
	    }
	    if (i != sourceExpr.size())
		aNameEnd = i;
	}
    }

    std::pair<int, int> boundary;
    if (found) {
	boundary.first = aNameStart;
	boundary.second = aNameEnd;
    } else {
	boundary.first = -1;
	boundary.second = -1;
    }
    std::pair<std::pair<int, int>, std::string> ret;
    ret.first = boundary;
    ret.second = temp;
    return ret;
}

bool Helper::isNumber(std::string str) {
    for (std::string::iterator it = str.begin(); it != str.end(); it++) {
	if (!isdigit(*it))
	    return false;
    }
    return true;
}

#if 0
std::vector<std::pair<int, int> > getPositionsOfString(std::string sourceString,
    std::string searchString) {

    std::vector<std::pair<int, int> > result;
    for (unsigned i = 0; i < sourceString.size(); ) {
	temp = "";
	while (!(sourceString[i] == '_' || isalnum(sourceString[i])) 
		&& i < sourceString.size()) {
#ifdef DEBUGDUMMY
	    llvm::outs() << "DEBUG: Skipping " << sourceString[i] << "\n";
#endif
	    temp.push_back(sourceString[i]);
	    i++;
	}

	replacedString = replacedString + temp;

#ifdef DEBUGDUMMY
	llvm::outs() << "DEBUG: Starting to match identifier\n";
#endif

	temp = "";
	int idStart = i;
	while ((sourceString[i] == '_' || isalnum(sourceString[i])) 
		&& i < sourceString.size()) {
#ifdef DEBUGDUMMY
	    llvm::outs() << sourceString[i] << "\n";
#endif
	    temp.push_back(sourceString[i]);
	    i++;
	}
	int idEnd = i;

#ifdef DEBUGDUMMY
	llvm::outs() << "DEBUG: Found identifier: " << temp << "\n";
#endif
	if (temp.compare("") != 0 && temp.compare(searchString) == 0) {
	    std::pair<int, int> pos;
	    pos.first = idStart;
	    pos.end = idEnd;

	    result.push_back(pos);
	}
    }

    return result;
}
#endif

std::string Helper::replaceFormalWithActual(std::string sourceString,
    std::map<std::string, std::string> formalToActualMap) {

    std::string replacedString, temp;
    for (unsigned i = 0; i < sourceString.size(); ) {
	temp = "";
	while (!(sourceString[i] == '_' || isalnum(sourceString[i])) 
		&& i < sourceString.size()) {
#ifdef DEBUGDUMMY
	    llvm::outs() << "DEBUG: Skipping " << sourceString[i] << "\n";
#endif
	    temp.push_back(sourceString[i]);
	    i++;
	}

	replacedString = replacedString + temp;

#ifdef DEBUGDUMMY
	llvm::outs() << "DEBUG: Starting to match identifier\n";
#endif

	temp = "";
	while ((sourceString[i] == '_' || isalnum(sourceString[i])) 
		&& i < sourceString.size()) {
#ifdef DEBUGDUMMY
	    llvm::outs() << sourceString[i] << "\n";
#endif
	    temp.push_back(sourceString[i]);
	    i++;
	}

#ifdef DEBUGDUMMY
	llvm::outs() << "DEBUG: Found identifier: " << temp << "\n";
#endif
	if (temp.compare("") != 0) {
	    if (formalToActualMap.find(temp) != formalToActualMap.end()) {
		std::string actualArg;
		std::pair<std::pair<int, int>, std::string> arrayDetails =
		    Helper::getArrayExpr(sourceString, temp);
		if (arrayDetails.first.first == -1 && arrayDetails.first.second == -1)
		    actualArg = std::string("(") +
			formalToActualMap[temp] + std::string(")");
		else
		    actualArg = formalToActualMap[temp];
		replacedString = replacedString + actualArg;
	    } else
		replacedString = replacedString + temp;
	}
    }

    return replacedString;
}

bool Helper::isKeyword(std::string str) {
    if (str.compare("int") == 0 || str.compare("float") == 0 || 
	str.compare("double") == 0)
	return true;
    else
	return false;
}

bool Helper::isInputStmt(const Stmt* S) {
    if (isa<CXXOperatorCallExpr>(S)) {
	const CXXOperatorCallExpr* callExpr = dyn_cast<CXXOperatorCallExpr>(S);
	const Expr* firstArg = callExpr->getArg(0)->IgnoreParenCasts();
	if (!firstArg) {
	    llvm::outs() << "ERROR: Cannot get first argument of CXXOperatorCallExpr\n";
	    return false;
	}
	std::string operatorName = Helper::prettyPrintExpr(firstArg);
	if (operatorName.compare("cin") != 0 && 
	    operatorName.compare("std::cin") != 0)
	    return false;

	return true;
    }

    return false;
}

// Always call isInputStmt() before call getInpuVarsFromStmt()
std::vector<const Expr*> Helper::getInputVarsFromStmt(const Stmt* S) {
    std::vector<const Expr*> args;
    if (isa<CXXOperatorCallExpr>(S)) { // Found cin
	const CXXOperatorCallExpr* currStmtAsCallExpr = 
	    dyn_cast<CXXOperatorCallExpr>(S);
        unsigned numArgs = currStmtAsCallExpr->getNumArgs();
        for (unsigned i = 1; i < numArgs; i++) {
	    const Expr* cinArgExpr =
                currStmtAsCallExpr->getArg(i)->IgnoreParenCasts();
	    args.push_back(cinArgExpr);
        }
	return args;
    }

    return args;
}

const Expr* Helper::getBaseArrayExpr(const ArraySubscriptExpr* array) {
    ArraySubscriptExpr const *arrayExpr = array;
    Expr const *arrayBase = arrayExpr->getBase()->IgnoreParenCasts();
    while (isa<ArraySubscriptExpr>(arrayBase)) {
	arrayExpr = dyn_cast<ArraySubscriptExpr>(arrayBase);
	arrayBase = arrayExpr->getBase()->IgnoreParenCasts();
    }
    return arrayBase;
}

Expr* Helper::getBaseArrayExpr(ArraySubscriptExpr* array) {
    ArraySubscriptExpr *arrayExpr = array;
    Expr* arrayBase = arrayExpr->getBase()->IgnoreParenCasts();
    while (isa<ArraySubscriptExpr>(arrayBase)) {
	arrayExpr = dyn_cast<ArraySubscriptExpr>(arrayBase);
	arrayBase = arrayExpr->getBase()->IgnoreParenCasts();
    }
    return arrayBase;
}

#if 0
bool Helper::copyFunctionDetails(FunctionDetails source, FunctionDetails
    &destination) {
    destination.funcName = source.funcName;
    destination.funcStartLine = source.funcStartLine;
    destination.funcEndLine = source.funcEndLine;
    destination.isCustomInputFunction = source.isCustomInputFunction;
    destination.fd = source.fd;
    destination.cfg = source.cfg;
    
    return true;
}
#endif

#if 0
bool Helper::copyVarDetails(VarDetails source, VarDetails &destination) {
    destination.varID = source.varID;
    destination.varName = source.varName;
    destination.varDeclLine = source.varDeclLine;
    destination.type = source.type;
    destination.arraySizeInfo.insert(destination.arraySizeInfo.end(),
	source.arraySizeInfo.begin(), source.arraySizeInfo.end());
    return true;
}
#endif

bool Helper::isVarDeclAnArray(const VarDecl* VD) {
    const Type* varType = VD->getType().split().Ty;
    return varType->isArrayType();
}

std::pair<Helper::VarType, std::vector<const ArrayType*> > Helper::getVarType(
VarDecl* vd, bool &rv) {
    rv = true;
    const Type* t = vd->getType().split().Ty;
    VarType vt;
    std::vector<const ArrayType*> size;
    std::pair<VarType, std::vector<const ArrayType*> > p;
    if (t->isArrayType()) {
	while (t->isArrayType()) {
	    ArrayType const *at = t->getAsArrayTypeUnsafe();
	    if (isa<DependentSizedArrayType>(at)) {
		llvm::outs() << "ERROR: Found DependentSizedArrayType\n";
		rv = false;
		return p;
	    } else if (isa<IncompleteArrayType>(at)) {
		llvm::outs() << "ERROR: Found IncompleteArrayType\n";
		rv = false;
		return p;
	    }
	    size.push_back(at);
	    
	    t = at->getElementType().split().Ty;
	}
    }

    if (t->isIntegerType()) {
	if (size.size() > 0)
	    vt = INTARR;
	else
	    vt = INTVAR;
    } else if (t->isBooleanType())
	if (size.size() > 0)
	    vt = BOOLARR;
	else
	    vt = BOOLVAR;
    else if (t->isFloatingType())
	if (size.size() > 0)
	    vt = FLOATARR;
	else
	    vt = FLOATVAR;
    else if (t->isCharType())
	if (size.size() > 0)
	    vt = CHARARR;
	else
	    vt = CHARVAR;
    else if (t->isPointerType()) {
	llvm::outs() << "ERROR: Var is of Pointer type. Not handling\n";
	rv = false;
	return p;
    } else if (t->isRecordType()) {
	llvm::outs() << "ERROR: Var is of Record type. Not handling\n";
	rv = false;
	return p;
    } else {
	llvm::outs() << "ERROR: VarType not INT, BOOL, FLOAT or CHAR\n";
	rv = false;
	return p;
    }

    p.first = vt;
    p.second = size;
    return p;
}

std::pair<Helper::VarType, std::vector<const ArrayType*> > Helper::getVarType(
const Type* t, bool &rv) {
    rv = true;
    VarType vt;
    std::vector<const ArrayType*> size;
    std::pair<VarType, std::vector<const ArrayType*> > p;
    if (t->isArrayType()) {
	while (t->isArrayType()) {
	    ArrayType const *at = t->getAsArrayTypeUnsafe();
	    if (isa<DependentSizedArrayType>(at)) {
		llvm::outs() << "ERROR: Found DependentSizedArrayType\n";
		rv = false;
		return p;
	    } else if (isa<IncompleteArrayType>(at)) {
		llvm::outs() << "ERROR: Found IncompleteArrayType\n";
		rv = false;
		return p;
	    }
	    size.push_back(at);
	    
	    t = at->getElementType().split().Ty;
	}
    }

    if (t->isIntegerType()) {
	if (size.size() > 0)
	    vt = INTARR;
	else
	    vt = INTVAR;
    } else if (t->isBooleanType())
	if (size.size() > 0)
	    vt = BOOLARR;
	else
	    vt = BOOLVAR;
    else if (t->isFloatingType())
	if (size.size() > 0)
	    vt = FLOATARR;
	else
	    vt = FLOATVAR;
    else if (t->isCharType())
	if (size.size() > 0)
	    vt = CHARARR;
	else
	    vt = CHARVAR;
    else if (t->isPointerType()) {
	llvm::outs() << "ERROR: Var is of Pointer type. Not handling\n";
	rv = false;
	return p;
    } else {
	llvm::outs() << "ERROR: VarType not INT, BOOL, FLOAT or CHAR\n";
	rv = false;
	return p;
    }

    p.first = vt;
    p.second = size;
    return p;
}

#if 0
void Helper::printVarDetails(VarDetails v) {
    llvm::outs() << v.varID << ": " v.varName << " (" << v.varDeclLine << ") ";
    printVarType(v.type);
    llvm::outs() << " sizeInfo: "  << v.arraySizeInfo.size();
}
#endif

VarDetails Helper::getVarDetailsFromExpr(const SourceManager* SM, 
const Expr* E, bool &rv) {

    rv = true;
    VarDetails ret;
    Expr const* E1 = E->IgnoreParenCasts();
    if (isa<DeclRefExpr>(E1)) {
	DeclRefExpr const *ref = dyn_cast<DeclRefExpr>(E1);
	ValueDecl* d = const_cast<ValueDecl*>(ref->getDecl());
	if (!isa<VarDecl>(d)) {
	    llvm::outs() << "ERROR: Expr (DeclRefExpr) is not VarDecl\n";
	    llvm::outs() << Helper::prettyPrintExpr(E1) << "\n";
	    rv = false;
	    return ret;
	}
	VarDecl* vd = dyn_cast<VarDecl>(d);
	std::pair<Helper::VarType, std::vector<const ArrayType*> > typeInfo =
	    Helper::getVarType(vd, rv);
	if (!rv)
	    return ret;

	if (vd->hasGlobalStorage())
	    ret.kind = VarDetails::VarKind::GLOBALVAR;
	ret.varName = Helper::prettyPrintExpr(ref);
	ret.varDeclLine = SM->getExpansionLineNumber(vd->getLocStart());
	ret.type = typeInfo.first;
	ret.arraySizeInfo = typeInfo.second;
	ret.setVarID();
	return ret;
    } else if (isa<ArraySubscriptExpr>(E1)) {
	ArraySubscriptExpr const *aref = dyn_cast<ArraySubscriptExpr>(E1);
	Expr const *arrayBase =
	    Helper::getBaseArrayExpr(aref)->IgnoreParenCasts();
	//arrayBase = arrayBase->IgnoreParenCasts();
	if (!isa<DeclRefExpr>(arrayBase)) {
	    llvm::outs() << "ERROR: Array Base is not DeclRefExpr\n";
	    rv = false;
	    return ret;
	}
	ValueDecl* d =
	    const_cast<ValueDecl*>((dyn_cast<DeclRefExpr>(arrayBase))->getDecl());
	if (!isa<VarDecl>(d)) {
	    llvm::outs() << "ERROR: Cannot get VarDecl of Array Base\n";
	    rv = false;
	    return ret;
	}
	VarDecl* vd = dyn_cast<VarDecl>(d);
	std::pair<Helper::VarType, std::vector<const ArrayType*> > typeInfo = 
	    Helper::getVarType(vd, rv);
	if (!rv)
	    return ret;
	
	if (vd->hasGlobalStorage())
	    ret.kind = VarDetails::VarKind::GLOBALVAR;
	ret.varName = Helper::prettyPrintExpr(arrayBase);
	ret.varDeclLine = SM->getExpansionLineNumber(vd->getLocStart());
	ret.type = typeInfo.first;
	ret.arraySizeInfo = typeInfo.second;
	ret.setVarID();
	return ret;
    } else {
	llvm::outs() << "ERROR: Expr is neither DeclRefExpr nor "
		     << "ArraySubscriptExpr: " << Helper::prettyPrintExpr(E1) 
		     << "\n";
	rv = false;
	return ret;
    }
}

VarDetails Helper::getVarDetailsFromDecl(const SourceManager* SM, 
const Decl* D, bool &rv) {

    rv = true;
    VarDetails ret;
    if (!isa<VarDecl>(D)) {
	llvm::outs() << "ERROR: Decl is not VarDecl\n";
	rv = false;
	return ret;
    }
    VarDecl const *vd = dyn_cast<VarDecl>(D);
    std::pair<Helper::VarType, std::vector<const ArrayType*> > typeInfo =
	Helper::getVarType(const_cast<VarDecl*>(vd), rv);
    if (!rv)
	return ret;

    if (vd->hasGlobalStorage())
	ret.kind = VarDetails::VarKind::GLOBALVAR;
    ret.varName = vd->getQualifiedNameAsString();
    ret.varDeclLine = SM->getExpansionLineNumber(vd->getLocStart());
    ret.type = typeInfo.first;
    ret.arraySizeInfo = typeInfo.second;
    ret.setVarID();
    return ret;
}

FunctionDetails* Helper::getFunctionDetailsFromDecl(const SourceManager* SM,
const Decl* D, bool &rv) {
    rv = true;
    FunctionDetails* fd = new FunctionDetails;
    if (!isa<FunctionDecl>(D)) {
	llvm::outs() << "ERROR: Decl is not FunctionDecl\n";
	rv = false;
	return NULL;
    }
    FunctionDecl const *funcDecl = dyn_cast<FunctionDecl>(D);
    DeclarationNameInfo fNameInfo = funcDecl->getNameInfo();
    fd->funcName = fNameInfo.getName().getAsString();
    fd->fd = funcDecl->getMostRecentDecl();
    if (!(fd->fd)) {
	llvm::outs() << "ERROR: Cannot get most recent decl for function "
		     << fd->funcName << "\n";
	rv = false;
	return NULL;
    }
    SourceLocation SL = SM->getExpansionLoc(fd->fd->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL)) {
	fd->isLibraryFunction = true;
	return fd;
    }
    fd->funcStartLine = SM->getExpansionLineNumber(fd->fd->getLocStart());
    fd->funcEndLine = SM->getExpansionLineNumber(fd->fd->getLocEnd());

    // Get parameters. Skip if it is main()
    if (!(fd->fd->isMain())) {
	for (FunctionDecl::param_const_iterator pIt = fd->fd->param_begin(); 
		pIt != fd->fd->param_end(); pIt++) {
	    const ParmVarDecl* pvd = *pIt;
	    VarDetails vd = getVarDetailsFromDecl(SM, pvd, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot obtain VarDetails of parameter\n";
		rv = false;
		return NULL;
	    }
	    fd->parameters.push_back(vd);
	}
    }

    return fd;
}

std::vector<Expr*> Helper::getArrayIndexExprs(ArraySubscriptExpr* array) {

    std::vector<Expr*> indices;
    Expr* baseArray = array->getBase()->IgnoreParenCasts();
    Expr* index = array->getIdx()->IgnoreParenCasts();
    indices.push_back(index);
    while (isa<ArraySubscriptExpr>(baseArray)) {
	ArraySubscriptExpr* ase = dyn_cast<ArraySubscriptExpr>(baseArray);
	baseArray = ase->getBase()->IgnoreParenCasts();
	index = ase->getIdx()->IgnoreParenCasts();
	indices.push_back(index);
    }
    std::reverse(indices.begin(), indices.end());
    return indices;
}

#if 0
bool Helper::areEqual(VarDetails v1, VarDetails v2) {
    if (v1.varID.compare(v2.varID) != 0)
	return false;
    if (v1.varName.compare(v2.varName) != 0)
	return false;
    if (v1.varDeclLine != v2.varDeclLine)
	return false;
    if (v1.type != v2.type)
	return false;
    if (v1.arraySizeInfo.size() != v2.arraySizeInfo.size())
	return false;

    return true;
}
#endif

bool VarDetails::equals(VarDetails v) {
    if (varName.compare(v.varName) != 0)
	return false;
    if (varDeclLine != v.varDeclLine)
	return false;
    if (type != v.type)
	return false;
    if (kind != v.kind)
	return false;
    if (varID.compare(v.varID) != 0)
	return false;
    if (arraySizeInfo.size() != v.arraySizeInfo.size())
	return false;

    return true;
}

Expr* Helper::getStreamOfCXXOperatorCallExpr(CXXOperatorCallExpr* C, bool &rv) {
    rv = true;
    if (C->getNumArgs() != 2) {
	llvm::outs() << "ERROR: The arguments to CXXOperatorCallExpr != 2\n";
	rv = false;
	return NULL;
    }

    Expr* firstArg = C->getArg(0)->IgnoreParenCasts();
    while (isa<CXXOperatorCallExpr>(firstArg)) {
	CXXOperatorCallExpr* firstArgCallExpr =
	    dyn_cast<CXXOperatorCallExpr>(firstArg);
	firstArg = firstArgCallExpr->getArg(0)->IgnoreParenCasts();
    }

    if (isa<DeclRefExpr>(firstArg)) {
	return firstArg;
    } else {
	llvm::outs() << "ERROR: The stream to CXXOperatorCallExpr: "
		     << Helper::prettyPrintExpr(C) << " is not <DeclRefExpr>\n";
	rv = false;
	return NULL;
    }
}

std::vector<Expr*> Helper::getArgsToCXXOperatorCallExpr(CXXOperatorCallExpr* C,
bool &rv) {
    rv = true;
    std::vector<Expr*> argExprs;

    if (C->getNumArgs() != 2) {
	llvm::outs() << "ERROR: The arguments to CXXOperatorCallExpr != 2\n";
	rv = false;
	return argExprs;
    }

    Expr* firstArg = C->getArg(0)->IgnoreParenCasts();
    Expr* secondArg = C->getArg(1)->IgnoreParenCasts();
    argExprs.push_back(secondArg);

    while (isa<CXXOperatorCallExpr>(firstArg)) {
	CXXOperatorCallExpr* firstArgCallExpr = 
	    dyn_cast<CXXOperatorCallExpr>(firstArg);
	firstArg = firstArgCallExpr->getArg(0)->IgnoreParenCasts();
	secondArg = firstArgCallExpr->getArg(1)->IgnoreParenCasts();
	argExprs.push_back(secondArg);
    }

    // Reverse the argExprs since we would have obtained the last expression
    // first in the loop above
    std::reverse(argExprs.begin(), argExprs.end());
    if (isa<DeclRefExpr>(firstArg)) {
	return argExprs;
    } else {
	llvm::outs() << "ERROR: The stream to CXXOperatorCallExpr: "
		     << Helper::prettyPrintExpr(C) << " is not <DeclRefExpr>\n";
	rv = false;
	return argExprs;
    }
}

void FunctionDetails::print() {
    llvm::outs() << "Function " << funcName << " (" << funcStartLine 
		 << " - " << funcEndLine << ") " 
		 << (isCustomInputFunction? "custom-input-func": "")
		 << (isCustomOutputFunction? "custom-output-func": "")
		 << (isLibraryFunction? "library-func": "") << "\n";
    llvm::outs() << "Parameters: " << parameters.size() << "\n";
    for (std::vector<VarDetails>::iterator pIt = parameters.begin(); pIt != 
	    parameters.end(); pIt++) {
	pIt->print();
	llvm::outs() << "\n";
    }
}

InputVar InputVar::clone() {
    InputVar cloneObj;
    cloneObj.progID = progID;
    cloneObj.funcID = funcID;
#ifdef DEBUG
    llvm::outs() << "DEBUG: After funcID\n";
#endif
    cloneObj.var = var.clone();
    cloneObj.varExpr = varExpr;
#ifdef DEBUG
    llvm::outs() << "DEBUG: After varExpr\n";
#endif
    cloneObj.isDummyScalarInput = isDummyScalarInput;
    cloneObj.inputCallLine = inputCallLine;
    //Helper::copyFunctionDetails(func, cloneObj.func);
#ifdef DEBUG
    llvm::outs() << "DEBUG: After inputCallLine\n";
#endif
    cloneObj.func = func->cloneAsPtr();
#ifdef DEBUG
    llvm::outs() << "DEBUG: After func\n";
#endif
    cloneObj.inputFunction = inputFunction;
    for (std::vector<SymbolicExpr*>::iterator sIt = sizeExprs.begin();
	    sIt != sizeExprs.end(); sIt++) {
	bool rv;
	SymbolicStmt* size = (*sIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicExpr>(size)) {
	    llvm::outs() << "WARNING: Clone of SymbolicExpr is not "
			 << "<SymbolicExpr>\n";
	    return cloneObj;
	}
	cloneObj.sizeExprs.push_back(dyn_cast<SymbolicExpr>(size));
    }
    return cloneObj;
}

void InputVar::print() {
    llvm::outs() << "Input Var (" << progID << ", " << funcID << ", "
		 << (isDummyScalarInput? "dummy scalar": "") << "):\n";
    var.print();
    llvm::outs() << " at line " << inputCallLine;
    llvm::outs() << " by func ";
    if (inputFunction == InputFunc::CIN)
	llvm::outs() << "cin";
    else if (inputFunction == InputFunc::SCANF)
	llvm::outs() << "scanf";
    else if (inputFunction == InputFunc::FSCANF)
	llvm::outs() << "fscanf";
    else if (inputFunction == InputFunc::CUSTOM)
	llvm::outs() << "custom";
    else 
	llvm::outs() << "UNKNOWN";
    for (std::vector<SymbolicExpr*>::iterator sIt = sizeExprs.begin();
	    sIt != sizeExprs.end(); sIt++) {
	(*sIt)->prettyPrint(true);
	llvm::outs() << ", ";
    }
    llvm::outs() << "\n";
    llvm::outs() << "substituteArray: ";
    if (substituteArray.array)
	substituteArray.array->prettyPrint(true);
    else
	llvm::outs() << "NULL\n";
}

void DPVar::print() {
    llvm::outs() << "DP ID: " << id << ": ";
    dpArray.print();
    for (std::vector<SymbolicExpr*>::iterator sIt = sizeExprs.begin();
	    sIt != sizeExprs.end(); sIt++) {
	(*sIt)->print();
	(*sIt)->prettyPrint(true);
	llvm::outs() << ", ";
    }
}

DPVar DPVar::clone(bool &rv) {
    rv = true;
    DPVar cloneObj;
    cloneObj.id = id;
    cloneObj.dpArray = dpArray.clone();
    for (std::vector<SymbolicExpr*>::iterator sIt = sizeExprs.begin(); sIt 
	    != sizeExprs.end(); sIt++) {
	SymbolicStmt* cloneExpr = (*sIt)->clone(rv);
	if (!rv) return cloneObj;
	if (!isa<SymbolicExpr>(cloneExpr)) {
	    rv = false;
	    return cloneObj;
	}
	cloneObj.sizeExprs.push_back(dyn_cast<SymbolicExpr>(cloneExpr));
    }
    cloneObj.toDelete = toDelete;
    cloneObj.hasUpdate = hasUpdate;
    cloneObj.hasInit = hasInit;
    return cloneObj;
}

void VarDetails::print() {
    llvm::outs() << "ID: " << varID << ", name: " << varName << " ("
		 << varDeclLine << " ";
    if (kind == VarKind::PARAMETER) llvm::outs() << "PARAMETER";
    else if (kind == VarKind::RETURN) llvm::outs() << "RETURN";
    else if (kind == VarKind::OTHER) llvm::outs() << "OTHER";
    else if (kind == VarKind::GLOBALVAR) llvm::outs() << "GLOBALVAR";
    else llvm::outs() << "UNKNOWN VARKIND";
    llvm::outs() << ") ";
    Helper::printVarType(type);
    llvm::outs() << " sizeInfo: " << arraySizeInfo.size() << " symSize: " 
		 << arraySizeSymExprs.size();
    for (std::vector<SymbolicExpr*>::iterator it = arraySizeSymExprs.begin();
	    it != arraySizeSymExprs.end(); it++) {
	llvm::outs() << " [";
	(*it)->prettyPrint(true);
	llvm::outs() << "]";
    }
}

std::vector<std::vector<std::pair<unsigned, bool> > > appendGuards(
std::vector<std::vector<std::pair<unsigned, bool> > > origGuards,
std::vector<std::vector<std::pair<unsigned, bool> > > newGuards) {

    std::vector<std::vector<std::pair<unsigned, bool> > > finalGuards;
    if (origGuards.size() == 0) return newGuards;
    if (newGuards.size() == 0) return origGuards;
    for (std::vector<std::vector<std::pair<unsigned, bool> > >::iterator orIt = 
	    origGuards.begin(); orIt != origGuards.end(); orIt++) {
	for (std::vector<std::vector<std::pair<unsigned, bool> > >::iterator
		newIt = newGuards.begin(); newIt != newGuards.end(); newIt++) {
	    std::vector<std::pair<unsigned, bool> > gVec;
	    // Add origVec to gVec. While adding we check if we are adding
	    // duplicates or contradicting guards (i.e. same block with true and
	    // false). In case of contradicting guards we just do not add this
	    // vector to the finalGuards.
	    bool foundContradicting = false, foundDuplicate = false;
	    for (std::vector<std::pair<unsigned, bool> >::iterator vIt =
		    orIt->begin(); vIt != orIt->end(); vIt++) {
		// Traverse gVec to find duplicates or contradictions
		foundContradicting = false;
		foundDuplicate = false;
		for (std::vector<std::pair<unsigned, bool> >::iterator gIt = 
			gVec.begin(); gIt != gVec.end(); gIt++) {
		    if (gIt->first != vIt->first) continue;
		    if (gIt->second == vIt->second) {
			foundDuplicate = true;
			break;
		    } else {
			foundContradicting = true;
			break;
		    }
		}
		if (foundContradicting) break;
		if (!foundDuplicate) gVec.push_back(*vIt);
	    }
	    if (foundContradicting) continue;
	    for (std::vector<std::pair<unsigned, bool> >::iterator vIt =
		    newIt->begin(); vIt != newIt->end(); vIt++) {
		// Traverse gVec to find duplicates or contradictions
		foundContradicting = false;
		foundDuplicate = false;
		for (std::vector<std::pair<unsigned, bool> >::iterator gIt = 
			gVec.begin(); gIt != gVec.end(); gIt++) {
		    if (gIt->first != vIt->first) continue;
		    if (gIt->second == vIt->second) {
			foundDuplicate = true;
			break;
		    } else {
			foundContradicting = true;
			break;
		    }
		}
		if (foundContradicting) break;
		if (!foundDuplicate) gVec.push_back(*vIt);
	    }
	    if (foundContradicting) continue;

	    finalGuards.push_back(gVec);
	}
    }

    return finalGuards;
}

std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > appendGuards(
std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > origGuards,
std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > newGuards, bool &rv) {

    rv = true;
    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > finalGuards;
    if (origGuards.size() == 0) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: First argument to appendGuards empty\n";
#endif
	return newGuards;
    }
    if (newGuards.size() == 0) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Second argument to appendGuards empty\n";
#endif
	return origGuards;
    }
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator orIt = 
	    origGuards.begin(); orIt != origGuards.end(); orIt++) {
	for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
		newIt = newGuards.begin(); newIt != newGuards.end(); newIt++) {
	    std::vector<std::pair<SymbolicExpr*, bool> > gVec;
	    // Add origVec to gVec. 
	    bool foundDuplicate = false, foundContradicting = false;
	    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator vIt =
		    orIt->begin(); vIt != orIt->end(); vIt++) {
		if (!(vIt->first)) {
		    llvm::outs() << "ERROR: NULL guard\n";
		    rv = false;
		    return finalGuards;
		}
		SymbolicStmt* cloneExpr = vIt->first->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While cloning guard\n";
		    return finalGuards;
		}
		if (!isa<SymbolicExpr>(cloneExpr)) {
		    llvm::outs() << "ERROR: Clone of SymbolicExpr is not "
				 << "<SymbolicExpr>\n";
		    rv = false;
		    return finalGuards;
		}
		// Check for duplicates.
		foundDuplicate = false; foundContradicting = false;
		for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gvIt
			= gVec.begin(); gvIt != gVec.end(); gvIt++) {
		    if (!(gvIt->first)) {
			llvm::outs() << "ERROR: NULL guard\n";
			rv = false;
			return finalGuards;
		    }
		    if (!(cloneExpr->equals(gvIt->first))) continue;
		    if (vIt->second == gvIt->second) {
			foundDuplicate = true;
			break;
		    } else {
			foundContradicting = true;
#ifdef DEBUG
			llvm::outs() << "DEBUG: found contradicting:";
			if (isa<SymbolicExpr>(cloneExpr))
			    dyn_cast<SymbolicExpr>(cloneExpr)->prettyPrint(false);
			gvIt->first->prettyPrint(false);
#endif
			break;
		    }
		}
		if (foundContradicting) break;
		if (!foundDuplicate)
		    gVec.push_back(std::make_pair(
			dyn_cast<SymbolicExpr>(cloneExpr), vIt->second));
	    }
	    if (foundContradicting) continue;
#ifdef DEBUG
	    llvm::outs() << "gVec:\n";
	    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator itt = 
		    gVec.begin(); itt != gVec.end(); itt++) {
		itt->first->prettyPrint(false);
	    }
#endif
	    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator vIt =
		    newIt->begin(); vIt != newIt->end(); vIt++) {
		if (!(vIt->first)) {
		    llvm::outs() << "ERROR: NULL guard\n";
		    rv = false;
		    return finalGuards;
		}
		SymbolicStmt* cloneExpr = vIt->first->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While cloning guard\n";
		    return finalGuards;
		}
		if (!isa<SymbolicExpr>(cloneExpr)) {
		    llvm::outs() << "ERROR: Clone of SymbolicExpr is not "
				 << "<SymbolicExpr>\n";
		    rv = false;
		    return finalGuards;
		}
#ifdef DEBUG
		llvm::outs() << "DEBUG: Check if expr is present in gVec:\n";
		cloneExpr->prettyPrint(false);
#endif
		foundDuplicate = false; foundContradicting = false;
		for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gvIt
			= gVec.begin(); gvIt != gVec.end(); gvIt++) {
		    if (!(gvIt->first)) {
			llvm::outs() << "ERROR: NULL guard\n";
			rv = false;
			return finalGuards;
		    }
		    if (!(cloneExpr->equals(gvIt->first))) continue;
#ifdef DEBUG
		    llvm::outs() << "DEBUG: cloneExpr is equal to gVec element:\n";
		    gvIt->first->prettyPrint(false);
#endif
		    if (vIt->second == gvIt->second) {
			foundDuplicate = true;
			break;
		    } else {
			foundContradicting = true;
			break;
		    }
		}
#ifdef DEBUG
		llvm::outs() << "DEBUG: foundContradicting: " << foundContradicting
			     << " foundDuplicate: " << foundDuplicate << "\n";
#endif
		if (foundContradicting) break;
		if (!foundDuplicate) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: About to add new guard: ";
		    cloneExpr->prettyPrint(false);
		    llvm::outs() << "\n";
#endif
		    gVec.push_back(std::make_pair(
			dyn_cast<SymbolicExpr>(cloneExpr), vIt->second));
		}
	    }
	    if (foundContradicting) continue;

#ifdef DEBUG
	    llvm::outs() << "After adding newguards, gVec:\n";
	    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator itt = 
		    gVec.begin(); itt != gVec.end(); itt++) {
		itt->first->prettyPrint(false);
	    }
#endif
#if 0
	    foundDuplicate = true;
	    if (finalGuards.size() == 0) foundDuplicate = false;
	    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
		    >::iterator fIt = finalGuards.begin(); fIt !=
		    finalGuards.end(); fIt++) {
		if (fIt->size() != gVec.size()) continue;
		foundDuplicate = true;
		std::vector<std::pair<SymbolicExpr*, bool> > fVec = *fIt;
		for (unsigned i = 0; i < gVec.size(); i++) {
		    if (!fVec[i].first || !gVec[i].first) {
			llvm::outs() << "ERROR: NULL guard\n";
			rv = false;
			return finalGuards;
		    }
		    if (!(fVec[i].first->equals(gVec[i].first)) || 
			fVec[i].second != gVec[i].second) {
			foundDuplicate = false;
			break;
		    }
		}
		if (foundDuplicate) break;
	    }
	    if (!foundDuplicate)
		finalGuards.push_back(gVec);
#endif
	    foundDuplicate = false;
	    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
		    ::iterator fIt = finalGuards.begin(); fIt !=
		    finalGuards.end(); fIt++) {
		if (fIt->size() != gVec.size()) continue;
		std::vector<std::pair<SymbolicExpr*, bool> > fVec = *fIt;
		unsigned i;
		for (i = 0; i < gVec.size(); i++) {
		    if (!(fVec[i].first->equals(gVec[i].first)) ||
			fVec[i].second != gVec[i].second)
			break;
		}

		if (i == gVec.size()) {
		    foundDuplicate = true;
		    break;
		}
	    }

	    if (!foundDuplicate)
		finalGuards.push_back(gVec);
	}
    }

    return finalGuards;
}

std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > negateConjunction(
std::vector<std::pair<SymbolicExpr*, bool> > origGuard) {
    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > negatedGuard;
    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator gIt =
	    origGuard.begin(); gIt != origGuard.end(); gIt++) {
	std::vector<std::pair<SymbolicExpr*, bool> > neg;
	neg.push_back(std::make_pair(gIt->first, !gIt->second));
	negatedGuard.push_back(neg);
    }
    return negatedGuard;
}

std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > negateGuard(
std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > origGuard, bool &rv) {
    rv = true;
    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > > negatedGuard;
    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >::iterator
	    gIt = origGuard.begin(); gIt != origGuard.end(); gIt++) {
	std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
	negationOfConjunction = negateConjunction(*gIt);
	negatedGuard = appendGuards(negatedGuard, negationOfConjunction, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While appending guards\n";
	    return negatedGuard;
	}
    }
    return negatedGuard;
}

z3::sort getSortFromType(z3::context* c, Helper::VarType T, bool &rv) {
    rv = true;
    switch(T) {
    // I am allowing only int sort for now
    case Helper::VarType::FLOATARR:
    case Helper::VarType::FLOATVAR: return c->real_sort();
    case Helper::VarType::BOOLARR:
    case Helper::VarType::CHARARR:
    case Helper::VarType::INTARR:
    case Helper::VarType::BOOLVAR: //return c->bool_sort(); 
    case Helper::VarType::CHARVAR: 
    case Helper::VarType::INTVAR: return c->int_sort();
    default: rv = false; return c->int_sort();
    }
}

