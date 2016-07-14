#include "Utils.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::driver;
using namespace clang::tooling;

std::string getStringFor(const Expr* E){
    clang::LangOptions LangOpts;
    LangOpts.CPlusPlus = true;
    clang::PrintingPolicy Policy(LangOpts);

    std::string TypeS;
    llvm::raw_string_ostream s(TypeS);
    E->printPretty(s, 0, Policy);
    return s.str();
}

std::string getStringFor(const Stmt* S) {
    clang::LangOptions LangOpts;
    LangOpts.CPlusPlus = true;
    clang::PrintingPolicy Policy(LangOpts);

    std::string TypeS;
    llvm::raw_string_ostream s(TypeS);
    S->printPretty(s, 0, Policy);
    return s.str();
}
