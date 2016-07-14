//------------------------------------------------------------------------------
// Using AST matchers with RefactoringTool. Demonstrates:
//
// * How to use Replacements to collect replacements in a matcher instead of
//   directly applying fixes to a Rewriter.
//
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include <string>
#include <map>
#include <algorithm>

#include "clang/AST/AST.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Refactoring.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/Lex/Lexer.h"
#include "Utils.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

static llvm::cl::OptionCategory ToolingSampleCategory("Matcher Sample");

std::string print(const clang::Stmt* S) {
    clang::LangOptions LangOpts;
    LangOpts.CPlusPlus = true;
    clang::PrintingPolicy Policy(LangOpts);

    std::string TypeS;
    llvm::raw_string_ostream s(TypeS);
    S->printPretty(s, 0, Policy);
    return s.str();
}

class ContStmtHandler : public MatchFinder::MatchCallback {
public:
  
  ContStmtHandler(Replacements *Replace) : Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
    // The matched 'if' statement was bound to 'ifStmt'.
    if(const Stmt *loopStmt = Result.Nodes.getNodeAs<clang::Stmt>("stmt")) {
      if (Result.SourceManager->isInMainFile(loopStmt->getLocStart()) == false)
	return;
      if(const IfStmt *IfS = Result.Nodes.getNodeAs<clang::IfStmt>("ifStmt")) {
	llvm::outs() << "Triggered continue rewrite \n";
	//const Stmt *Then = IfS->getThen();
	const Stmt *Else = IfS->getElse();
	
	if (Else == nullptr) {
	  SourceLocation END = Result.SourceManager->getExpansionLoc(IfS->getLocEnd());
	  const char *endChar = Result.SourceManager->getCharacterData(END);

	  if((*endChar) == '}'){
	    END = END.getLocWithOffset(1);
	  }
	  else{
	    END = Lexer::findLocationAfterToken(END, tok::semi, *(Result.SourceManager), Result.Context->getLangOpts(), true);
	    if(END.isInvalid()){
	      llvm::outs() << "Rewrite failed to find a valid location -- quitting!" << "\n";
	      return;
	    }
	  } 

	  Replacement Rep(*(Result.SourceManager), END, 0, "\nelse{\n");
	  Replacement Rep1(*(Result.SourceManager), loopStmt->getLocEnd(), 0,
			   "}\n");
	  Replace->insert(Rep);
	  Replace->insert(Rep1);
	  
	}
      }
    }
  }

private:
  Replacements *Replace;
};

class CondStmtHandler : public MatchFinder::MatchCallback {

public:
  
  CondStmtHandler(Replacements *Replace) : Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
    
    if(const ConditionalOperator *CO = Result.Nodes.getNodeAs<clang::ConditionalOperator>("condOp")) {
      if (Result.SourceManager->isInMainFile(CO->getExprLoc()) == false)
	return;

      std::string cond = getStringFor(CO->getCond());
      std::string texpr = getStringFor(CO->getTrueExpr());
      std::string fexpr = getStringFor(CO->getFalseExpr());
      
      llvm::outs() << "Triggered conditional stmt rewrite \n";

      const Expr* trueExpr = CO->getTrueExpr()->IgnoreParenCasts();
      const Expr* falseExpr = CO->getFalseExpr()->IgnoreParenCasts();

      // hack: only expand printf or assignment statement
      if (texpr.find("printf") == std::string::npos && 
	    fexpr.find("printf") == std::string::npos) {
	if (!isa<BinaryOperator>(trueExpr) || !isa<BinaryOperator>(falseExpr))
	    return;
	const BinaryOperator* trueExprBO = dyn_cast<BinaryOperator>(trueExpr);
	const BinaryOperator* falseExprBO = dyn_cast<BinaryOperator>(falseExpr);
	if (!trueExprBO->isAssignmentOp() || !falseExprBO->isAssignmentOp())
	    return;
      }

      std::string rewrite = "if(" + cond + "){\n" + texpr + "; \n} else{\n " + fexpr +"; \n}";
      SourceLocation END = CO->getLocEnd();
      int offset = clang::Lexer::MeasureTokenLength(END,
      						    *(Result.SourceManager),
      							  Result.Context->getLangOpts()) + 1;
      SourceLocation END1 = END.getLocWithOffset(offset);

      Replacement Rep(*(Result.SourceManager), clang::CharSourceRange(clang::SourceRange(CO->getLocStart(), END1), true)
	, rewrite);
      Replace->insert(Rep); 		
    }
  }
private:
  Replacements *Replace;
};

class StaticCastHandler : public MatchFinder::MatchCallback {
public: 
  StaticCastHandler(Replacements *Replace) : Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
    if(const CXXStaticCastExpr* sCast = Result.Nodes.getNodeAs<clang::CXXStaticCastExpr>("sCast")) {
      if (Result.SourceManager->isInMainFile(sCast->getExprLoc()) == false)
	return;

      std::string type = sCast->getTypeAsWritten().getAsString();
      std::string cexpr = getStringFor(sCast->getSubExprAsWritten());
      std::string rewrite = "((" + type + ")(" + cexpr + "))";
      llvm::outs() << "Triggered static cast rewrite \n";
      Replacement Rep(*(Result.SourceManager), clang::CharSourceRange(clang::SourceRange(sCast->getLocStart(), sCast->getLocEnd()), true)
	, rewrite);
      Replace->insert(Rep); 		
    }
  }
private:
  Replacements *Replace;
};


class FCastHandler : public MatchFinder::MatchCallback {
public: 
  FCastHandler(Replacements *Replace) : Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
    if(const CXXFunctionalCastExpr* fCast = Result.Nodes.getNodeAs<clang::CXXFunctionalCastExpr>("fCast")) {
      if (Result.SourceManager->isInMainFile(fCast->getExprLoc()) == false)
	return;

      std::string type = fCast->getTypeAsWritten().getAsString();
      std::string cexpr = getStringFor(fCast->getSubExprAsWritten());
      std::string rewrite = "((" + type + ")(" + cexpr + "))";
      llvm::outs() << "Triggered functional cast rewrite \n";
      Replacement Rep(*(Result.SourceManager), clang::CharSourceRange(clang::SourceRange(fCast->getLocStart(), fCast->getLocEnd()), true)
	, rewrite);
      Replace->insert(Rep); 		
    }
  }
private:
  Replacements *Replace;
};

class LeftShiftBy1Handler : public MatchFinder::MatchCallback {
public: 
  LeftShiftBy1Handler(Replacements *Replace) : Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
    if(const BinaryOperator* lShift = Result.Nodes.getNodeAs<clang::BinaryOperator>("lShift")) {
      if (Result.SourceManager->isInMainFile(lShift->getExprLoc()) == false)
	return;

      std::string op1 = getStringFor(lShift->getLHS());
      //std::string op2 = getStringFor(lShift->getRHS());
      std::string rewrite = op1 + " * 2";
      llvm::outs() << "Triggered left shift by 1 rewrite \n";
      Replacement Rep(*(Result.SourceManager), clang::CharSourceRange(clang::SourceRange(lShift->getLocStart(), lShift->getLocEnd()), true)
	, rewrite);
      Replace->insert(Rep); 		
    }
  }
private:
  Replacements *Replace;
};


class MultAssignHandler : public MatchFinder::MatchCallback {
public: 
  MultAssignHandler(Replacements *Replace) : Replace(Replace) {}
  std::map<int, std::vector<std::string> > assignmentsProcessed;

  virtual void run(const MatchFinder::MatchResult &Result) {
    if(const BinaryOperator* root = Result.Nodes.getNodeAs<clang::BinaryOperator>("root")) {
      if (Result.SourceManager->isInMainFile(root->getExprLoc()) == false)
	return;
      int line = Result.SourceManager->getExpansionLineNumber(root->getLocStart());
      std::string lineStr = print(root);
      if (assignmentsProcessed.find(line) != assignmentsProcessed.end()) {
	bool found = false;
	for (std::vector<std::string>::iterator vIt =
		assignmentsProcessed[line].begin(); vIt != 
		assignmentsProcessed[line].end(); vIt++) {
	    if ((*vIt).find(lineStr) != std::string::npos) {
		found = true;
		break;
	    }
	}
	if (found) return;
	else assignmentsProcessed[line].push_back(lineStr);
      } else {
	std::vector<std::string> vec(1, lineStr);
	assignmentsProcessed[line] = vec;
      }

      llvm::outs() << "Root: " << print(root) << "\n";
      std::vector<const Expr*> lhsExprs;
      Expr const* e = root;
      Expr const* rhs;
      while (isa<BinaryOperator>(e)) {
	BinaryOperator const* bo = dyn_cast<BinaryOperator>(e);
	Expr const* lhs = bo->getLHS()->IgnoreParenCasts();
	lhsExprs.push_back(lhs);
	e = bo->getRHS()->IgnoreParenCasts();
      }
      rhs = e;
      std::string rewrite = "{";
      std::string rhsStr = print(rhs);
      for (std::vector<const Expr*>::iterator eIt = lhsExprs.begin(); eIt != 
	    lhsExprs.end(); eIt++) {
	rewrite = rewrite + print(*eIt) + " = " + rhsStr + ";\n";
      }
      rewrite = rewrite + "}\n";
      SourceLocation START = root->getLocStart();
      SourceLocation END = root->getLocEnd();
      SourceLocation END1 = clang::Lexer::getLocForEndOfToken(END, 0, 
	    *(Result.SourceManager), Result.Context->getLangOpts());
      Replacement Rep(*(Result.SourceManager), 
	    clang::CharSourceRange(clang::SourceRange(START, END1), true),
	    rewrite);
      Replace->insert(Rep);
    }
  }
private:
  Replacements *Replace;
};

class ReturnStmtHandler : public MatchFinder::MatchCallback {
public:
  
  ReturnStmtHandler(Replacements *Replace) : Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
    // The matched 'if' statement was bound to 'ifStmt'.
    if(const FunctionDecl *FD = Result.Nodes.getNodeAs<clang::FunctionDecl>("fDecl")) {
       if(const IfStmt *IfS = Result.Nodes.getNodeAs<clang::IfStmt>("ifStmt")) {
	 if (Result.SourceManager->isInMainFile(IfS->getIfLoc()) == false)
	   return;
	 
	 llvm::outs() << "Triggered missing else before return rewrite \n";
	//const Stmt *Then = IfS->getThen();
	const Stmt *Else = IfS->getElse();

	if (Else == nullptr) {
	  if(!isa<CompoundStmt>(IfS)){
	    SourceLocation END = IfS->getLocEnd();
	    int offset = clang::Lexer::MeasureTokenLength(END,
							  *(Result.SourceManager),
							  Result.Context->getLangOpts()) + 1;
	    SourceLocation END1 = END.getLocWithOffset(offset);
	    Replacement Rep(*(Result.SourceManager), END1, 0, "\nelse{");
	    Replacement Rep1(*(Result.SourceManager), FD->getLocEnd(), 0,
			   "}\n");
	    Replace->insert(Rep);
	    Replace->insert(Rep1);
	  }
	  else{
	    Replacement Rep(*(Result.SourceManager), IfS->getLocEnd().getLocWithOffset(1), 
			    0, "else{");
	    Replacement Rep1(*(Result.SourceManager), FD->getLocEnd(), 0,
			   "}\n");
	    Replace->insert(Rep);
	    Replace->insert(Rep1);
	  }
	}
      }
    }
  }

private:
  Replacements *Replace;
};

class CompoundAssignHandler : public MatchFinder::MatchCallback {
public:
  
  // Original SourceLocation, originalStmt
  std::vector<std::pair<std::string, std::string> > stmtsReplaced;
  CompoundAssignHandler(Replacements *Replace) : Replace(Replace) {}

  virtual void run(const MatchFinder::MatchResult &Result) {
    bool forStmtMatched = false;
    if (const ForStmt* FS =
	    Result.Nodes.getNodeAs<clang::ForStmt>("forWithCompound")) {
	if (Result.SourceManager->isInMainFile(FS->getLocStart()) == false)
	    return;
	forStmtMatched = true;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found forStmt with compound assignment: "
		     << getStringFor(FS) << "\n";
#endif
    }

    if (const BinaryOperator* pBO =
	    Result.Nodes.getNodeAs<clang::BinaryOperator>("plusEq")) {
	if (Result.SourceManager->isInMainFile(pBO->getLocStart()) == false)
	    return;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found Stmt: " << getStringFor(pBO) << "\n";
#endif
	std::string lhs = getStringFor(pBO->getLHS());
	std::string rhs = getStringFor(pBO->getRHS());
	std::string rewritten = lhs + " = " + lhs + " + (" + rhs + ")";
	if (!forStmtMatched)
	    rewritten = rewritten + ";";

	SourceLocation START = pBO->getLocStart();
#ifdef DEBUG
	llvm::outs() << "DEBUG: SourceLocation: " 
		     << START.printToString(*Result.SourceManager) << "\n";
#endif
	std::string originalStmt = getStringFor(pBO);
	// Check if this is already rewritten
	bool exists = false;
	for (std::vector<std::pair<std::string, std::string> >::iterator 
		sIt = stmtsReplaced.begin(); sIt != stmtsReplaced.end(); sIt++)
	{
	    if (sIt->first.compare(START.printToString(*Result.SourceManager)) == 0 && 
		    sIt->second.compare(originalStmt) == 0) {
#ifdef DEBUG
		llvm::outs() << "Found duplicate: " << sIt->second << "\n";
#endif
		exists = true;
		break;
	    }
	}
	if (exists) return;
	else 
	    stmtsReplaced.push_back(std::make_pair(
		START.printToString(*Result.SourceManager), originalStmt));
#ifdef DEBUG
	llvm::outs() << "DEBUG: Rewrote stmt\n";
#endif
	SourceLocation END = pBO->getLocEnd();
	SourceLocation END1 = clang::Lexer::getLocForEndOfToken(END, 0, 
	    *(Result.SourceManager), Result.Context->getLangOpts());
	clang::CharSourceRange SR;
	if (forStmtMatched)
	    SR = clang::CharSourceRange(clang::SourceRange(START, END), true);
	else
	    SR = clang::CharSourceRange(clang::SourceRange(START, END1), true);
	Replacement Rep(*(Result.SourceManager), SR, rewritten);
	Replace->insert(Rep);
    }
    if (const BinaryOperator* mBO =
	    Result.Nodes.getNodeAs<clang::BinaryOperator>("minusEq")) {
	if (Result.SourceManager->isInMainFile(mBO->getLocStart()) == false)
	    return;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found Stmt: " << getStringFor(mBO) << "\n";
#endif
	std::string lhs = getStringFor(mBO->getLHS());
	std::string rhs = getStringFor(mBO->getRHS());
	std::string rewritten = lhs + " = " + lhs + " - (" + rhs + ")";
	if (!forStmtMatched)
	    rewritten = rewritten + ";";

	SourceLocation START = mBO->getLocStart();
	std::string originalStmt = getStringFor(mBO);
	// Check if this is already rewritten
	bool exists = false;
	for (std::vector<std::pair<std::string, std::string> >::iterator 
		sIt = stmtsReplaced.begin(); sIt != stmtsReplaced.end(); sIt++)
	{
	    if (sIt->first.compare(START.printToString(*Result.SourceManager)) == 0 && 
		    sIt->second.compare(originalStmt) == 0) {
		exists = true;
		break;
	    }
	}
	if (exists) return;
	else stmtsReplaced.push_back(std::make_pair(
	    START.printToString(*Result.SourceManager), originalStmt));
#ifdef DEBUG
	llvm::outs() << "DEBUG: Rewrote stmt\n";
#endif
	SourceLocation END = mBO->getLocEnd();
	SourceLocation END1 = clang::Lexer::getLocForEndOfToken(END, 0, 
	    *(Result.SourceManager), Result.Context->getLangOpts());
	clang::CharSourceRange SR;
	if (forStmtMatched)
	    SR = clang::CharSourceRange(clang::SourceRange(START, END), true);
	else
	    SR = clang::CharSourceRange(clang::SourceRange(START, END1), true);
	Replacement Rep(*(Result.SourceManager), SR, rewritten);
	Replace->insert(Rep);
    }
    if (const BinaryOperator* sBO =
	    Result.Nodes.getNodeAs<clang::BinaryOperator>("multEq")) {
	if (Result.SourceManager->isInMainFile(sBO->getLocStart()) == false)
	    return;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found Stmt: " << getStringFor(sBO) << "\n";
#endif
	std::string lhs = getStringFor(sBO->getLHS());
	std::string rhs = getStringFor(sBO->getRHS());
	std::string rewritten = lhs + " = " + lhs + " * (" + rhs + ")";
	if (!forStmtMatched)
	    rewritten = rewritten + ";";

	SourceLocation START = sBO->getLocStart();
	std::string originalStmt = getStringFor(sBO);
	// Check if this is already rewritten
	bool exists = false;
	for (std::vector<std::pair<std::string, std::string> >::iterator 
		sIt = stmtsReplaced.begin(); sIt != stmtsReplaced.end(); sIt++)
	{
	    if (sIt->first.compare(START.printToString(*Result.SourceManager)) == 0 && 
		    sIt->second.compare(originalStmt) == 0) {
		exists = true;
		break;
	    }
	}
	if (exists) return;
	else stmtsReplaced.push_back(std::make_pair(
	    START.printToString(*Result.SourceManager), originalStmt));
#ifdef DEBUG
	llvm::outs() << "DEBUG: Rewrote stmt\n";
#endif
	SourceLocation END = sBO->getLocEnd();
	SourceLocation END1 = clang::Lexer::getLocForEndOfToken(END, 0, 
	    *(Result.SourceManager), Result.Context->getLangOpts());
	clang::CharSourceRange SR;
	if (forStmtMatched)
	    SR = clang::CharSourceRange(clang::SourceRange(START, END), true);
	else
	    SR = clang::CharSourceRange(clang::SourceRange(START, END1), true);
	Replacement Rep(*(Result.SourceManager), SR, rewritten);
	Replace->insert(Rep);
    }
    if (const BinaryOperator* dBO =
	    Result.Nodes.getNodeAs<clang::BinaryOperator>("divEq")) {
	if (Result.SourceManager->isInMainFile(dBO->getLocStart()) == false)
	    return;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found Stmt: " << getStringFor(dBO) << "\n";
#endif
	std::string lhs = getStringFor(dBO->getLHS());
	std::string rhs = getStringFor(dBO->getRHS());
	std::string rewritten = lhs + " = " + lhs + " / (" + rhs + ")";
	if (!forStmtMatched)
	    rewritten = rewritten + ";";

	SourceLocation START = dBO->getLocStart();
	std::string originalStmt = getStringFor(dBO);
	// Check if this is already rewritten
	bool exists = false;
	for (std::vector<std::pair<std::string, std::string> >::iterator 
		sIt = stmtsReplaced.begin(); sIt != stmtsReplaced.end(); sIt++)
	{
	    if (sIt->first.compare(START.printToString(*Result.SourceManager)) == 0 && 
		    sIt->second.compare(originalStmt) == 0) {
		exists = true;
		break;
	    }
	}
	if (exists) return;
	else stmtsReplaced.push_back(std::make_pair(
	    START.printToString(*Result.SourceManager), originalStmt));
#ifdef DEBUG
	llvm::outs() << "DEBUG: Rewrote stmt\n";
#endif
	SourceLocation END = dBO->getLocEnd();
	SourceLocation END1 = clang::Lexer::getLocForEndOfToken(END, 0, 
	    *(Result.SourceManager), Result.Context->getLangOpts());
	clang::CharSourceRange SR;
	if (forStmtMatched)
	    SR = clang::CharSourceRange(clang::SourceRange(START, END), true);
	else
	    SR = clang::CharSourceRange(clang::SourceRange(START, END1), true);
	Replacement Rep(*(Result.SourceManager), SR, rewritten);
	Replace->insert(Rep);
    }
    if (const BinaryOperator* oBO =
	    Result.Nodes.getNodeAs<clang::BinaryOperator>("orEq")) {
	if (Result.SourceManager->isInMainFile(oBO->getLocStart()) == false)
	    return;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found Stmt: " << getStringFor(oBO) << "\n";
#endif
	std::string lhs = getStringFor(oBO->getLHS());
	std::string rhs = getStringFor(oBO->getRHS());
	std::string rewritten = lhs + " = " + lhs + " | (" + rhs + ")";
	if (!forStmtMatched)
	    rewritten = rewritten + ";";

	SourceLocation START = oBO->getLocStart();
	std::string originalStmt = getStringFor(oBO);
	// Check if this is already rewritten
	bool exists = false;
	for (std::vector<std::pair<std::string, std::string> >::iterator 
		sIt = stmtsReplaced.begin(); sIt != stmtsReplaced.end(); sIt++)
	{
	    if (sIt->first.compare(START.printToString(*Result.SourceManager)) == 0 && 
		    sIt->second.compare(originalStmt) == 0) {
		exists = true;
		break;
	    }
	}
	if (exists) return;
	else stmtsReplaced.push_back(std::make_pair(
	    START.printToString(*Result.SourceManager), originalStmt));
#ifdef DEBUG
	llvm::outs() << "DEBUG: Rewrote stmt\n";
#endif
	SourceLocation END = oBO->getLocEnd();
	SourceLocation END1 = clang::Lexer::getLocForEndOfToken(END, 0, 
	    *(Result.SourceManager), Result.Context->getLangOpts());
	clang::CharSourceRange SR;
	if (forStmtMatched)
	    SR = clang::CharSourceRange(clang::SourceRange(START, END), true);
	else
	    SR = clang::CharSourceRange(clang::SourceRange(START, END1), true);
	Replacement Rep(*(Result.SourceManager), SR, rewritten);
	Replace->insert(Rep);
    }
  }

private:
  Replacements *Replace;
};

int main(int argc, const char **argv) {
  CommonOptionsParser op(argc, argv, ToolingSampleCategory);
  RefactoringTool Tool(op.getCompilations(), op.getSourcePathList());

  // Set up AST matcher callbacks.
  ContStmtHandler HandlerForCont(&Tool.getReplacements());
  ReturnStmtHandler HandlerForReturn(&Tool.getReplacements());
  CondStmtHandler HandlerForCond(&Tool.getReplacements());
  FCastHandler HandlerForFCast(&Tool.getReplacements());
  MultAssignHandler HandlerForMultAssign(&Tool.getReplacements());
  LeftShiftBy1Handler HandlerForLeftShiftBy1(&Tool.getReplacements());
  StaticCastHandler HandlerForStaticCast(&Tool.getReplacements());
  CompoundAssignHandler HandlerForCompoundAssign(&Tool.getReplacements());
  MatchFinder Finder;
  StatementMatcher contMatcher = continueStmt(
					      hasAncestor(
							  ifStmt(
								 anyOf(
								       hasAncestor(forStmt().bind("stmt")), 
								       hasAncestor(whileStmt().bind("stmt"))
								       )
								 ).bind("ifStmt")
							  )
					      );
  //Finder.addMatcher(contMatcher, &HandlerForCont);
  //Finder.addMatcher(returnStmt(hasAncestor(ifStmt(hasAncestor(functionDecl().bind("fDecl")), hasParent(compoundStmt())).bind("ifStmt"))), &HandlerForReturn);
  Finder.addMatcher(conditionalOperator(hasAncestor(compoundStmt())).bind("condOp"), &HandlerForCond);
  Finder.addMatcher(cxxFunctionalCastExpr().bind("fCast"), &HandlerForFCast);
  Finder.addMatcher(cxxStaticCastExpr(hasDestinationType(builtinType())).bind("sCast"), &HandlerForStaticCast);
  // match a = b = c;
#if 0
  StatementMatcher assignMatcher = binaryOperator(
						  //hasParent(compoundStmt()), 
						  hasOperatorName("="), 
						  hasLHS(expr().bind("e1")), 
						  hasRHS(ignoringParenImpCasts(
									       binaryOperator(
											      unless(hasDescendant(binaryOperator(hasOperatorName("=")))), 
											      hasOperatorName("="), 
											      hasLHS(expr().bind("e2")), 
											      hasRHS(expr().bind("e3"))
											      )
									       )
							 )
						  ).bind("root");
#endif

  StatementMatcher assignMatcher = binaryOperator(hasOperatorName("="),
	hasRHS(ignoringParenImpCasts(binaryOperator(hasOperatorName("="))))
	).bind("root");
  
  Finder.addMatcher(assignMatcher, &HandlerForMultAssign);
  Finder.addMatcher(binaryOperator(hasOperatorName("<<"), hasRHS(integerLiteral(equals(1)))).bind("lShift"), &HandlerForLeftShiftBy1);
  StatementMatcher pAssignMatcher = binaryOperator(hasOperatorName("+=")).bind("plusEq");
  StatementMatcher mAssignMatcher = binaryOperator(hasOperatorName("-=")).bind("minusEq");
  StatementMatcher sAssignMatcher = binaryOperator(hasOperatorName("*=")).bind("multEq");
  StatementMatcher dAssignMatcher = binaryOperator(hasOperatorName("/=")).bind("divEq");
  StatementMatcher oAssignMatcher = binaryOperator(hasOperatorName("|=")).bind("orEq");
  StatementMatcher forStmtWithCompoundAssignMatcher =
    forStmt(hasIncrement(anyOf(pAssignMatcher,
    mAssignMatcher))).bind("forWithCompound");
  Finder.addMatcher(forStmtWithCompoundAssignMatcher, &HandlerForCompoundAssign);
  Finder.addMatcher(pAssignMatcher, &HandlerForCompoundAssign);
  Finder.addMatcher(mAssignMatcher, &HandlerForCompoundAssign);
  Finder.addMatcher(sAssignMatcher, &HandlerForCompoundAssign);
  Finder.addMatcher(dAssignMatcher, &HandlerForCompoundAssign);
  Finder.addMatcher(oAssignMatcher, &HandlerForCompoundAssign);
  // Run the tool and collect a list of replacements. We could call runAndSave,
  // which would destructively overwrite the files with their new contents.
  // However, for demonstration purposes it's interesting to show the
  // replacements.
  
  if (int Result = Tool.runAndSave(newFrontendActionFactory(&Finder).get())) {
    return Result;
  }
  
  llvm::outs() << "Replacements collected by the tool:\n";
  for (auto &r : Tool.getReplacements()) {
    llvm::outs() << r.toString() << "\n";
  }

  return 0;
}
