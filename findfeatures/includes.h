// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"

#include <fstream>
#include <sstream>
#include <vector>

// To print debug information
//#define DEBUG
//#define DEBUGFULL
#define ERRORMSG
#define OUTPUT
//#define SANITYCHECK
#define SANITY
// To run only till update
//#define NOINIT

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;

#define INPUTNAME "dp_ProgInput"
#define DPARRNAME "dp_Array"
