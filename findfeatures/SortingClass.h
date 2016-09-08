#ifndef SORTINGCLASS_H
#define SORTINGCLASS_H

#include "includes.h"

class SortingClass : public MatchFinder::MatchCallback {
  public :
    // Class to store details of one statement in the sorting (swap) template
    class swapStmt {
      public:
	int loopStartLine;
	int loopEndLine;
	int swapStmtLine1;
	int swapStmtLine2;
	int swapStmtLine3;
	std::string tempVarName;
	bool tempVarOrArr;  // true if var
	std::string arrayName;
	bool arrayVarOrArr; // true if var

	swapStmt() {
	    loopStartLine = -1;
	    loopEndLine = -1;
	    swapStmtLine1 = -1;
	    swapStmtLine2 = -1;
	    swapStmtLine3 = -1;
	    tempVarName = "";
	    tempVarOrArr = true;
	    arrayName = "";
	    arrayVarOrArr = false;
	}
    };

    // Class to store details of all matched swap/sorting statements
    class sortingMatchesList {
      public:
	std::vector<swapStmt> stmt1;
	std::vector<swapStmt> stmt2;
	std::vector<swapStmt> stmt3;

      void printSortStmt3() {
	for (std::vector<swapStmt>::iterator it = stmt3.begin();
		it != stmt3.end(); it++) {
	    llvm::outs() << "(" << it->swapStmtLine1 << ", "
			 << it->swapStmtLine2 << ", "
			 << it->swapStmtLine3 << ")\n";
	}
      }
    };

    sortingMatchesList sortStmtMatches;

    virtual void run (const MatchFinder::MatchResult &Result);

    void runSortingMatcher(ClangTool &Tool);
};

#endif /* SORTINGCLASS_H */
