#include "includes.h"

#include "RecursionClass.h"
#include "SortingClass.h"
#include "FunctionAnalysis.h"
#include "MainFunction.h"
#include "GlobalVars.h"

#include<iostream>
#include<cctype>

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...");

void findDP(int argc, const char **argv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: argc = " << argc << "\n";
#endif
    std::string progID(argv[3]);

    std::vector<std::string> inputFuncs;
    std::vector<std::string> outputFuncs;
    std::string problemName;

    for (int i = 2; i < argc; i++) {
	if (strcmp(argv[i], "-inp") == 0) {
	    std::string temp(argv[i+1]);
	    inputFuncs.push_back(temp);
	} else if (strcmp(argv[i], "-out") == 0) {
	    std::string temp(argv[i+1]);
	    outputFuncs.push_back(temp);
	} else if (strcmp(argv[i], "-problem") == 0) {
	    std::string temp(argv[i+1]);
	    problemName = temp;
	}
    }

#ifdef DEBUG
    for (std::vector<std::string>::iterator iIt = inputFuncs.begin(); iIt != 
	    inputFuncs.end(); iIt++)
	llvm::outs() << "DEBUG: InputFunc = " << *iIt << "\n";
    for (std::vector<std::string>::iterator oIt = outputFuncs.begin(); oIt != 
	    outputFuncs.end(); oIt++)
	llvm::outs() << "DEBUG: OutputFunc = " << *oIt << "\n";
    llvm::outs() << "DEBUG: problemName: " << problemName << "\n";
    llvm::outs() << "DEBUG: argc = " << argc << "\n";
#endif

    CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
    ClangTool Tool(OptionsParser.getCompilations(),
		   OptionsParser.getSourcePathList());

    // Checking for Recursion
    llvm::outs() << "DEBUG: Checking for recursion or class or struct\n";
    RecursionClass RObject;
    RObject.runRecursionMatcher(Tool);
    if (RObject.recursionFound) {
#ifdef OUTPUT
	llvm::outs() << "OUTPUT: Recursion\n" ;
#endif
	llvm::outs() << "!!SKIPPING!!Recursion\n";
	return;
    } else if (RObject.classOrStructFound) {
#ifdef OUTPUT
	llvm::outs() << "OUTPUT: class or struct\n";
#endif
	llvm::outs() << "!!SKIPPING!!class or struct\n";
	return;
    } else if (RObject.templateFuncFound) {
	llvm::outs() << "!!SKIPPING!!Template func\n";
	return;
    } else if (RObject.gotoFound) {
	llvm::outs() << "!!SKIPPING!!Goto Stmt\n";
	return;
    } else if (!RObject.arrayFound) {
	llvm::outs() << "!!SKIPPING!!No array\n";
	return;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: After RecursionClass\n";
#endif

    // Find sorting pattern
    SortingClass SObject;
    // Populates SObject.sortStmtMatches
    SObject.runSortingMatcher(Tool);

#ifdef DEBUG
    llvm::outs() << "DEBUG: After SortingClass\n";
#endif
    GlobalVars gvObj;
    gvObj.runMatcher(Tool);
    if (gvObj.error) {
	llvm::outs() << "ERROR: While obtaining global vars\n";
	return;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Global vars found:\n";
    int i = 0;
    for (std::vector<VarDeclDetails*>::iterator vIt = gvObj.globalVars.begin();
	    vIt != gvObj.globalVars.end(); vIt++) {
	llvm::outs() << "DEBUG: Global var " << ++i << ":\n";
	(*vIt)->prettyPrint();
    }
#endif

    MainFunction mObj;
    for (std::vector<std::string>::iterator iIt = inputFuncs.begin(); iIt != 
	    inputFuncs.end(); iIt++)
	mObj.addCustomInputFunc(*iIt);
    for (std::vector<std::string>::iterator oIt = outputFuncs.begin(); oIt != 
	    outputFuncs.end(); oIt++)
	mObj.addCustomOutputFunc(*oIt);
    mObj.setProblem(problemName);
    mObj.setProgID(progID);
    mObj.setGlobalVars(gvObj.globalVars);
    mObj.runMatcher(Tool);

    if (!(mObj.error))
	llvm::outs() << "!!SUCCESS!!\n";
    else 
	llvm::outs() << "!!ERROR!!\n";

    return;
}

int main(int argc, const char **argv) {
#ifdef DEBUG
    for (int i = 0; i < argc; i++) {
	llvm::outs() << "arg " << i << "\n";
	llvm::outs() << argv[i] << "\n";
    }

    llvm::outs() << "argc: " << argc << "\n";
#endif
    if (argc < 4) {
#ifdef ERROMSG
	llvm::outs() << "ERROR: Not enough arguments\n";
	llvm::outs() << argc << "\n";
#endif
	return -1;
    }

    findDP(argc, argv);

    return 0;
}
