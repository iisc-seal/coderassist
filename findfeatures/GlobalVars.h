#ifndef GLOBALVARS_H
#define GLOBALVARS_H

#include "Helper.h"
#include "StmtDetails.h"

class GlobalVars : public MatchFinder::MatchCallback {
public:
    bool error;
    std::vector<VarDeclDetails*> globalVars;

    GlobalVars() {
	error = false;
    }

    virtual void run(const MatchFinder::MatchResult &Result);
    void runMatcher(ClangTool &Tool);
};
#endif /* GLOBALVARS_H */
