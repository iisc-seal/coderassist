#ifndef RECURSIONCLASS_H
#define RECURSIONCLASS_H

#include "includes.h"

// Class to check for recursion
class RecursionClass : public MatchFinder::MatchCallback {
  public :
    bool recursionFound;	// records whether recursion was found
    bool classOrStructFound;
    bool templateFuncFound;
    bool arrayFound;
    bool gotoFound;

    RecursionClass() {
	recursionFound = false;
	classOrStructFound = false;
	templateFuncFound = false;
	arrayFound = false;
	gotoFound = false;
    }

    virtual void run(const MatchFinder::MatchResult &Result);

    void runRecursionMatcher(ClangTool &Tool);
};

#endif /* RECURSIONCLASS_H */
