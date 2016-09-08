#include "MainFunction.h"
#include "MyASTVisitor.h"
#include "GetSymbolicExprVisitor.h"
#include <queue>
#include <z3++.h>

const char MainFunc[] = "MainFunc";

DeclarationMatcher MFDMatcher = 
    functionDecl(isDefinition(),
	hasName("main")).bind(MainFunc);

void demorgan() {
    llvm::outs() << "de-Morgan example\n";
    
    z3::context c;

    z3::expr x = c.bool_const("x");
    z3::expr y = c.bool_const("y");
    z3::expr conjecture = !(x && y) == (!x || !y);
    
    z3::solver s(c);
    // adding the negation of the conjecture as a constraint.
    s.add(!conjecture);
    llvm::outs() << s << "\n";
    switch (s.check()) {
    case z3::unsat:   llvm::outs() << "de-Morgan is valid\n"; break;
    case z3::sat:     llvm::outs() << "de-Morgan is not valid\n"; break;
    case z3::unknown: llvm::outs() << "unknown\n"; break;
    }

    z3::context arrayc;
    z3::sort int1DArray = arrayc.array_sort(arrayc.int_sort(), arrayc.int_sort());
    z3::sort int2DArray = arrayc.array_sort(arrayc.int_sort(), int1DArray);
    z3::expr mid = arrayc.constant("mid", int2DArray);
    z3::expr arr = arrayc.constant("arr", int2DArray);
    z3::expr i = arrayc.int_const("i");
    z3::expr j = arrayc.int_const("j");
    z3::expr mid_i_j = select(select(mid, i), j);
    z3::expr arr_i_1_j = select(select(arr, i+1), j);
    z3::expr mid_i_1_j = select(select(mid, i+1), j);
    z3::expr assertion1 = (mid_i_j + arr_i_1_j >
	select(select(arrayc.constant("mid", int2DArray), i),
	arrayc.int_const("j")+1-1));
    z3::solver sa(arrayc);
    sa.add(assertion1);
    sa.add(arr_i_1_j == 0);
    llvm::outs() << sa.to_smt2() << "\n";
    switch (sa.check()) {
    case z3::unsat: llvm::outs() << "UNSAT\n"; break;
    case z3::sat:   llvm::outs() << "SAT\n"; break;
    case z3::unknown: llvm::outs() << "UNKNOWN\n"; break;
    }
}

void MainFunction::run(const MatchFinder::MatchResult &Result) {

    bool rv;
    SM = Result.SourceManager;

    FunctionDecl const *MainFD = 
	Result.Nodes.getNodeAs<clang::FunctionDecl>(MainFunc);

    if (!MainFD) {
	llvm::outs() << "ERROR: Cannot get FunctionDecl of main()\n";
	error = true;
	return;
    }

    // Initialize error to false only if we see a main
    error = false;

    SourceLocation SL = SM->getExpansionLoc(MainFD->getLocStart());
    if (SM->isInSystemHeader(SL) || !SM->isWrittenInMainFile(SL))
	return;
    
    FunctionDetails* f = new FunctionDetails;
    f->funcName = MainFD->getQualifiedNameAsString();
    f->funcStartLine = SM->getExpansionLineNumber(MainFD->getLocStart());
    f->funcEndLine = SM->getExpansionLineNumber(MainFD->getLocEnd());
    f->fd = MainFD;

    if (std::find(customInputFuncs.begin(), customInputFuncs.end(), f->funcName)
	    != customInputFuncs.end())
	f->isCustomInputFunction = true;
    else
	f->isCustomInputFunction = false;

    if (std::find(customOutputFuncs.begin(), customOutputFuncs.end(),
	    f->funcName) != customOutputFuncs.end())
	f->isCustomOutputFunction = true;
    else
	f->isCustomOutputFunction = false;

#ifdef DEBUG
    llvm::outs() << "DEBUG: Found function: " << f->funcName << " (" 
		 << f->funcStartLine << " - " << f->funcEndLine << ")\n";
#endif

    Stmt* mainBody = MainFD->getBody();
    if (!mainBody) {
	llvm::outs() << "ERROR: Cannot get function body from FunctionDecl (main)\n";
	error = true;
	return;
    }
    
    //CFG::BuildOptions bo;
    //CFG::BuildOptions fbo = bo.setAllAlwaysAdd();

    cg.addToCallGraph(const_cast<FunctionDecl*>(MainFD));
    CallExprVisitor cv(SM, &cg, customInputFuncs, customOutputFuncs);

    // workList to store each new function encountered
    std::queue<FunctionDetails*> workList;
    workList.push(f);

    while (!workList.empty()) {
	FunctionDetails* currFunc = workList.front();
	workList.pop();

	// Check if this function is already recorded. Then do not process
	bool foundFunc = false;
	for (std::vector<FunctionDetails*>::iterator fIt =
		functions.begin(); fIt != functions.end(); fIt++) {
	    if ((*fIt)->equals(currFunc)) {
		foundFunc = true;
		break;
	    }
	}

	if (!foundFunc) {
	    // Get body of current function
	    Stmt* body = currFunc->fd->getBody();
	    if (!body) {
		llvm::outs() << "ERROR: Cannot get body of function " 
			     << currFunc->funcName << " (" 
			     << currFunc->funcStartLine << " - "
			     << currFunc->funcEndLine << ")\n";
		error = true;
		return;
	    }

	    // Build CFG of current function
	    CFG::BuildOptions bo;
	    CFG::BuildOptions fbo = bo.setAllAlwaysAdd();
	    currFunc->cfg = CFG::buildCFG(currFunc->fd, body, Result.Context, fbo);
	    currFunc->funcBody = body;
	    currFunc->C = Result.Context;

	    // Record current function in "functions"
	    functions.push_back(currFunc);

	    // Check if this function is a custom input/output function, then do not
	    // descend into its body.
	    if (currFunc->isCustomInputFunction ||
		    currFunc->isCustomOutputFunction)
		continue;

	    cv.clear();
	    cv.TraverseStmt(body);

	    if (cv.error) {
		llvm::outs() << "ERROR!\n";
		error = true;
		return;
	    }

	    for (std::vector<FunctionDetails*>::iterator fIt = 
		    cv.funcsFound.begin(); fIt != cv.funcsFound.end(); fIt++) {
		FunctionDetails* newFunc = (*fIt)->cloneAsPtr();

		if (std::find(customInputFuncs.begin(), customInputFuncs.end(), 
			newFunc->funcName) != customInputFuncs.end())
		    newFunc->isCustomInputFunction = true;
		else
		    newFunc->isCustomInputFunction = false;
		if (std::find(customOutputFuncs.begin(),
			customOutputFuncs.end(), newFunc->funcName) !=
			customOutputFuncs.end())
		    newFunc->isCustomOutputFunction = true;
		else
		    newFunc->isCustomOutputFunction = false;

		workList.push(newFunc);
	    }
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: functions found:\n";
    for (std::vector<FunctionDetails*>::iterator fIt = functions.begin();
	    fIt != functions.end(); fIt++) {
	//llvm::outs() << (*fIt)->funcName << " (" << (*fIt)->funcStartLine << " - "
		     //<< (*fIt)->funcEndLine << ")\n";
	(*fIt)->print();
    }
#endif

    // Construct call graph of the program. It seems I only need to add the main
    // function declaration to obtain the entire call graph.

#ifdef DEBUG
    llvm::outs() << "DEBUG: CallGraph: nodes = " << cg.size() << "\n";
    cg.print(llvm::outs());
#endif

    // Sanity check: All the functions found are present in the call graph
    for (std::vector<FunctionDetails*>::iterator fIt = functions.begin(); 
	    fIt != functions.end(); fIt++) {
#if 0
	if (cg.getNode(fIt->fd) == nullptr) {
	    llvm::outs() << "ERROR: No node in call graph for function: "
			 << fIt->funcName << "\n";
	    error = true;
	    return;
	}
#endif
	bool foundNode = false;
	FunctionDecl const *fd = (*fIt)->fd->getMostRecentDecl();
	while (fd) {
	    if (cg.getNode(fd) != nullptr) {
		foundNode = true;
		break;
	    }
	    fd = fd->getPreviousDecl();
	}
	if (!foundNode) {
	    llvm::outs() << "ERROR: No node in call graph for function: "
			 << (*fIt)->funcName << "\n";
	    error = true;
	    return;
	}
    }

    // Topological sort of call graph
    std::vector<FunctionDetails*> topDownOrder;
    std::vector<FunctionDetails*> bottomUpOrder;
    std::vector<FunctionDetails*> visitedFunctions;
    std::stack<FunctionDetails*> stack;
    CallGraphNode* rootNode = cg.getRoot();
    if (!rootNode) {
	llvm::outs() << "ERROR: NULL root node for CallGraph\n";
	error = true;
	return;
    }
    for (CallGraphNode::iterator cIt = rootNode->begin(); cIt !=
	    rootNode->end(); cIt++) {
	FunctionDetails* currFunction = getFunctionDetailsFromCallGraphNode(*cIt, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot obtain FunctionDetails from "
			 << "CallGraphNode\n";
	    error = true;
	    return;
	}
	if (currFunction->isLibraryFunction) continue;

	// Check if this function is already visited
	bool visited = false;
	for (std::vector<FunctionDetails*>::iterator fIt =
		visitedFunctions.begin(); fIt != visitedFunctions.end(); fIt++) {
	    if ((*fIt)->equals(currFunction)) {
		visited = true;
		break;
	    }
	}

	if (visited) continue;

	topologicalSort(*cIt, visitedFunctions, stack, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While sorting callgraph\n";
	    error = true;
	    return;
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: stack:\n";
#endif
    while (!stack.empty()) {
#ifdef DEBUG
	stack.top()->print();
#endif
	topDownOrder.push_back(stack.top());
	stack.pop();
    }
#ifdef DEBUG
    llvm::outs() << "DEBUG: topDownOrder:\n";
    for (std::vector<FunctionDetails*>::iterator it = topDownOrder.begin(); 
	    it != topDownOrder.end(); it++)
	(*it)->print();
#endif
    for (std::vector<FunctionDetails*>::reverse_iterator rIt =
	    topDownOrder.rbegin(); rIt != topDownOrder.rend(); rIt++) {
	// topDownOrder will not contain cfg information. So get that from
	// functions vector
#ifdef DEBUG
	llvm::outs() << "DEBUG: Func in topDownOrder:\n";
	(*rIt)->print();
#endif
	for (std::vector<FunctionDetails*>::iterator fIt = functions.begin();
		fIt != functions.end(); fIt++) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Func in functions:\n";
	    (*fIt)->print();
#endif
	    if ((*fIt)->equals(*rIt)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Inserting func in bottomUpOrder\n";
#endif
		bottomUpOrder.push_back(*fIt);
		break;
	    } else {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Did not insert func in bottomUpOrder\n";
#endif
	    }
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Finished topological sort()\n";
    llvm::outs() << "DEBUG: bottomUpOrder:\n";
    for (std::vector<FunctionDetails*>::iterator it = bottomUpOrder.begin(); 
	    it != bottomUpOrder.end(); it++)
	(*it)->print();
#endif

    // Create Analysis objects for each function
    for (std::vector<FunctionDetails*>::iterator fIt = bottomUpOrder.begin();
	    fIt != bottomUpOrder.end(); fIt++) {
	if ((*fIt)->isCustomInputFunction || (*fIt)->isCustomOutputFunction) continue;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Function: ";
	(*fIt)->print();
	llvm::outs() << "\n";
#endif

	FunctionDetails* f = NULL;
	bool foundFunc = false;
	for (std::vector<FunctionDetails*>::iterator oIt = functions.begin();
		oIt != functions.end(); oIt++) {
	    if ((*fIt)->equals(*oIt)) {
		f = *oIt;
		foundFunc = true;
		break;
	    }
	}
	if (!foundFunc) {
	    llvm::outs() << "ERROR: Cannot find FunctionDetails object of "
			 << (*fIt)->funcName << "\n";
	    error = true;
	    return;
	}
	f->cfg = f->constructCFG(rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot construct CFG of function:\n";
	    f->print();
	    error = true;
	    return;
	}

	FunctionAnalysis* fObj = new FunctionAnalysis(SM, Result.Context, this);
	fObj->function = f;
	fObj->allFunctions.insert(fObj->allFunctions.end(), functions.begin(),
	    functions.end());
	analysisInfo.push_back(fObj);
    }

    // Set main FunctionDetails;
    for (std::vector<FunctionDetails*>::iterator fIt = functions.begin();
	    fIt != functions.end(); fIt++) {
	if ((*fIt)->funcName.compare("main") == 0) {
	    main = *fIt;
	    break;
	}
    }

#if 0
#ifdef DEBUG
    llvm::outs() << "DEBUG: Calling demorgan()\n";
#endif
    demorgan();
#endif

    getInputOutputStmts(rv);
    if (!rv) {
	error = true;
	return;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Inputs identified:\n";
    for (std::vector<InputVar>::iterator it = inputs.begin(); it !=
	    inputs.end(); it++)
	it->print();
    llvm::outs() << "DEBUG: Input Stmts identified:\n";
    for (std::vector<StmtDetails*>::iterator it = inputStmts.begin(); it != 
	    inputStmts.end(); it++) {
	llvm::outs() << "Line " << (*it)->lineNum << ": ";
	if (isa<DeclStmtDetails>(*it)) {
	    DeclStmtDetails* dsd = dyn_cast<DeclStmtDetails>(*it);
	    dsd->var.print();
	    if (dsd->vdd->initExpr && dsd->vdd->initExpr->origExpr)
		llvm::outs() << Helper::prettyPrintExpr(dsd->vdd->initExpr->origExpr) << "\n";
	} else {
	    if ((*it)->origStmt)
		llvm::outs() << Helper::prettyPrintStmt((*it)->origStmt) << "\n";
	}
    }
    llvm::outs() << "DEBUG: Output Stmts identified:\n";
    for (std::vector<StmtDetails*>::iterator it = outStmts.begin(); it != 
	    outStmts.end(); it++) {
	if ((*it)->origStmt)
	    llvm::outs() << Helper::prettyPrintStmt((*it)->origStmt) << "\n";
    }
#endif

    findTestCaseVar(rv);
    if (!rv) {
	error = true;
	return;
    }

    getInitUpdateStmts(rv);
    if (!rv) {
	llvm::outs() << "ERROR: While getInitUpdateStmts()\n";
	error = true;
	return;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Update Stmts:\n";
    for (std::vector<StmtDetails*>::iterator uIt = updateStmts.begin(); uIt != 
	    updateStmts.end(); uIt++) {
	if ((*uIt)->origStmt) {
	    llvm::outs() << "Update: " 
			 << Helper::prettyPrintStmt((*uIt)->origStmt) << " ["
			 << (*uIt)->lineNum << "]\n";
	} else {
	    llvm::outs() << "Update: NULL origStmt\n";
	}
    }
#endif

    getBlocks(rv);
    if (!rv) {
	error = true;
	return;
    }

    // Check if special exprs are sane
    for (std::vector<FunctionAnalysis*>::iterator fIt = analysisInfo.begin();
	    fIt != analysisInfo.end(); fIt++) {
	if ((*fIt)->function->isCustomInputFunction ||
	    (*fIt)->function->isCustomOutputFunction) {
	    continue;
	}

	bool sane = (*fIt)->checkIfSpecialExprsAreSane(rv);
	if (!rv) {
	    error = true;
	    return;
	}
	if (!sane) {
	    llvm::outs() << "ERROR: SpecialExprs in function " 
			 << (*fIt)->function->funcName << " are not sane\n";
	    error = true;
	    return;
	}
    }

    removeInputBlockForTestCaseVar(rv);
    if (!rv) {
	error = true;
	return;
    }

    removeTestCaseLoop(rv);
    if (!rv) {
	error = true;
	return;
    }

    combineBlocks(rv);
    if (!rv) {
	error = true;
	return;
    }

    checkIfArraysAreReadAsScalars(rv);
    if (!rv) {
	error = true;
	return;
    }

    for (std::vector<BlockDetails*>::iterator bIt = combinedBlocks.begin(); 
	    bIt != combinedBlocks.end(); bIt++) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Looking at block:\n";
	(*bIt)->prettyPrintHere();
#endif
	if ((*bIt)->isHeterogeneous() || (*bIt)->isDiffVar()) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Found heterogeneous block\n";
#endif
	    std::vector<BlockDetails*> homBlocks =
		(*bIt)->getHomogeneous(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Found heterogeneous block\n";
		(*bIt)->prettyPrintHere();
		error = true;
		return;
	    }
	    for (std::vector<BlockDetails*>::iterator hIt = homBlocks.begin();
		    hIt != homBlocks.end(); hIt++) {
		homogeneousBlocks.push_back(*hIt);
	    }
	} else {
	    BlockDetails* cloneBlock = (*bIt)->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot clone Block\n";
		(*bIt)->print();
		error = true;
		return;
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Adding block to homogeneousBlocks:\n";
	    cloneBlock->prettyPrintHere();
#endif
	    homogeneousBlocks.push_back(cloneBlock);
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: All homogeneous blocks!\n";
    for (std::vector<BlockDetails*>::iterator bIt = homogeneousBlocks.begin();
	    bIt != homogeneousBlocks.end(); bIt++) {
	(*bIt)->prettyPrintHere();
    }
#endif

    for (std::vector<BlockDetails*>::iterator bIt = homogeneousBlocks.begin();
	    bIt != homogeneousBlocks.end(); bIt++) {
	if ((*bIt)->isInit()) {
	    BlockDetails* cloneB = (*bIt)->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot clone Block\n";
		error = true;
		return;
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: About to convert block to range expr\n";
	    cloneB->prettyPrintHere();
#endif
	    bool ret = cloneB->convertBlockToRangeExpr(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While converting block to range expr\n";
		error = true;
		return;
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: After converting block to range expr\n";
	    cloneB->prettyPrintHere();
#endif
	    if (ret)
		finalBlocks.push_back(cloneB);
	    else
		finalBlocks.push_back(*bIt);
	} else {
	    BlockDetails* cloneB = (*bIt)->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot clone Block\n";
		error = true;
		return;
	    }
	    finalBlocks.push_back(cloneB);
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: finalBlocks:\n";
    for (std::vector<BlockDetails*>::iterator bIt = finalBlocks.begin(); bIt 
	    != finalBlocks.end(); bIt++) {
	if (*bIt) {
	    if ((*bIt)->isOutput())
		(*bIt)->print();
	    else
		(*bIt)->prettyPrintHere();
	}
    }
#endif

    for (std::vector<BlockDetails*>::iterator bIt = finalBlocks.begin(); bIt 
	    != finalBlocks.end(); bIt++) {
	(*bIt)->setLabel(rv);
	if (!rv) {
	    error = true;
	    return;
	}
    }

#if 0
    // If there are inits with guards over dp array, then relabel them as
    // updates.
    relabelInitAsUpdate(bool &rv);
    if (!rv) {
	llvm::outs() << "ERROR: While relabelInitAsUpdate()\n";
	error = true;
	return;
    }
#endif

    // Remove those inits for which there are no updates
    for (std::vector<BlockDetails*>::iterator bIt = finalBlocks.begin(); bIt 
	    != finalBlocks.end(); bIt++) {
	if (!((*bIt)->isInit())) continue;

	for (std::vector<BlockStmt*>::iterator isIt = (*bIt)->stmts.begin(); 
		isIt != (*bIt)->stmts.end(); isIt++) {
	    if (!isa<BlockInitStmt>(*isIt)) {
		error = true;
		return;
	    }
	    BlockInitStmt* bis = dyn_cast<BlockInitStmt>(*isIt);
	    SymbolicArraySubscriptExpr* lhsArray = bis->lhsArray;
	    SymbolicDeclRefExpr* baseArray = lhsArray->baseArray;
	    while (baseArray->substituteExpr) {
		if (!isa<SymbolicDeclRefExpr>(baseArray->substituteExpr)) {
		    error = true;
		    return;
		}
		baseArray =
		    dyn_cast<SymbolicDeclRefExpr>(baseArray->substituteExpr);
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: init base array:\n";
	    baseArray->prettyPrint(false);
#endif
	    bool foundUpdateForVar = false;
	    for (std::vector<BlockDetails*>::iterator buIt = finalBlocks.begin();
		    buIt != finalBlocks.end(); buIt++) {
		if (!(*buIt)) {
		    error = true;
		    return;
		}
		if (!((*buIt)->isUpdate())) continue;

#ifdef DEBUG
		llvm::outs() << "DEBUG: Found an update block\n";
		(*buIt)->prettyPrintHere();
#endif
		for (std::vector<BlockStmt*>::iterator usIt =
			(*buIt)->stmts.begin(); usIt != 
			(*buIt)->stmts.end(); usIt++) {
		    if (!isa<BlockUpdateStmt>(*usIt)) {
			error = true;
			return;
		    }
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Looking at stmt:\n";
		    (*usIt)->prettyPrintHere();
#endif
		    BlockUpdateStmt* bus = dyn_cast<BlockUpdateStmt>(*usIt);
		    for (std::vector<std::pair<SymbolicArraySubscriptExpr*,
			    SymbolicExpr*> >::iterator sIt = bus->updateStmts.begin();
			    sIt != bus->updateStmts.end(); sIt++) {
			SymbolicArraySubscriptExpr* uLhsArray = sIt->first;
			SymbolicDeclRefExpr* uBaseArray = uLhsArray->baseArray;
			while (uBaseArray->substituteExpr) {
			    if (!isa<SymbolicDeclRefExpr>(uBaseArray->substituteExpr)) {
				error = true;
				return;
			    }
			    uBaseArray =
				dyn_cast<SymbolicDeclRefExpr>(uBaseArray->substituteExpr);
			}
#ifdef DEBUG
			llvm::outs() << "DEBUG: updt base array:\n";
			uBaseArray->prettyPrint(false);
#endif

			if (uBaseArray->var.equals(baseArray->var)) {
			    foundUpdateForVar = true;
			    break;
			}
		    }
		    if (foundUpdateForVar) break;
		}
		if (foundUpdateForVar) break;
	    }

	    if (!foundUpdateForVar) {
		(*isIt)->toDelete = true;
		BlockStmt* clone = (*isIt)->clone(rv);
		if (!rv) {
		    error = true;
		    return;
		}
		residualInits.push_back(clone);
	    }
	}

	(*bIt)->stmts.erase(std::remove_if((*bIt)->stmts.begin(),
	    (*bIt)->stmts.end(), [](BlockStmt* bs) { return bs->toDelete; }),
	    (*bIt)->stmts.end());
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: finalBlocks after removing inits without updates:\n";
    for (std::vector<BlockDetails*>::iterator bIt = finalBlocks.begin(); bIt 
	    != finalBlocks.end(); bIt++) {
	if (*bIt) (*bIt)->prettyPrintHere();
    }
#endif

    getDPVars(rv);
    if (!rv) {
	error = true;
	return;
    }

    checkIfInputArraysAreReadMultipleTimes(rv);
    if (!rv) {
	error = true;
	return;
    }

    removeInputsAfterOutput(rv);
    if (!rv) {
	error = true;
	return;
    }

    writeClusteringFeatures(rv);
    if (!rv) {
	llvm::outs() << "ERROR: While writeClusteringFeatures()\n";
	error = true;
	return;
    }

    getSizeExprsOfSummaryVars(rv);
    if (!rv) {
	error = true;
	return;
    }

    // remove inits to input array variables
    removeInitToInputVars(rv);
    if (!rv) {
	error = true;
	return;
    }

    // create mutually disjoint guards in update blocks
    for (std::vector<BlockDetails*>::iterator bIt = finalBlocks.begin(); bIt != 
	    finalBlocks.end(); bIt++) {
	if ((*bIt)->isUpdate()) {
	    makeDisjointGuards(*bIt, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While makeDisjointGuards()\n";
		(*bIt)->prettyPrintHere();
		error = true;
		return;
	    }
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: About to call logSummary()\n";
#endif
    logSummary(rv);
    if (!rv) {
	error = true;
	return;
    }
}

void MainFunction::runMatcher(ClangTool &Tool) {
    MatchFinder MainFinder;
    MainFinder.addMatcher(MFDMatcher, this);
    Tool.run(newFrontendActionFactory(&MainFinder).get());
}

void MainFunction::getInputOutputStmts(bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering MainFunction::getInputOutputStmts\n";
#endif
    rv = true;

    InputOutputStmtVisitor ioVisitor(SM, this, customInputFuncs, 
	customOutputFuncs);
    ioVisitor.setCurrFunction(main);

    runVisitorOnInlinedProgram(main->cfg, &ioVisitor, rv);

    if (ioVisitor.error) {
	llvm::outs() << "ERROR: While running InputOutputStmtVisitor on inlined main\n";
	rv = false;
	return;
    }

    int i = 0;
    for (std::vector<InputVar>::iterator iIt = ioVisitor.inputVarsFound.begin();
	    iIt != ioVisitor.inputVarsFound.end(); iIt++) {
	iIt->progID = ++i;
	inputs.push_back(*iIt);
    }

    inputStmts.insert(inputStmts.end(), ioVisitor.inputStmts.begin(), 
	ioVisitor.inputStmts.end());
    outStmts.insert(outStmts.end(), ioVisitor.outputStmts.begin(),
	ioVisitor.outputStmts.end());
#ifdef DEBUG
    llvm::outs() << "DEBUG: ioVisitor.inputStmts:\n";
    for (std::vector<StmtDetails*>::iterator it = inputStmts.begin(); it != 
	    inputStmts.end(); it++) {
	if ((*it)->origStmt)
	    llvm::outs() << Helper::prettyPrintStmt((*it)->origStmt) << "\n";
    }
    llvm::outs() << "DEBUG: ioVisitor.outStmts:\n";
    for (std::vector<StmtDetails*>::iterator it = outStmts.begin(); it != 
	    outStmts.end(); it++) {
	if ((*it)->origStmt)
	    llvm::outs() << Helper::prettyPrintStmt((*it)->origStmt) << "\n";
    }
#endif


    // TODO: Ideally, I should copy only those input vars read in the given
    // function to the coresponding FunctionAnalysis object
    for (std::vector<FunctionAnalysis*>::iterator fIt = analysisInfo.begin();
	    fIt != analysisInfo.end(); fIt++) {
	(*fIt)->inputVars.insert((*fIt)->inputVars.end(), inputs.begin(), inputs.end());
	(*fIt)->inputLines.insert((*fIt)->inputLines.end(),
	    ioVisitor.inputLinesFound.begin(), ioVisitor.inputLinesFound.end());
	(*fIt)->inputStmts.insert((*fIt)->inputStmts.end(), 
	    ioVisitor.inputStmts.begin(), ioVisitor.inputStmts.end());
    }
}

void MainFunction::getInitUpdateStmts(bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering MainFunction::getInitUpdateStmts()\n";
#endif
    // Record the GetSymbolicExprVisitor object of main()
    GetSymbolicExprVisitor* mainSym = NULL;
    for (std::vector<FunctionAnalysis*>::iterator fIt = analysisInfo.begin();
	    fIt != analysisInfo.end(); fIt++) {
	if ((*fIt)->function->isCustomInputFunction ||
	    (*fIt)->function->isCustomOutputFunction) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Found custom input/output function. Skipping..\n";
#endif
	    continue;
	}

#ifdef DEBUG
	llvm::outs() << "DEBUG: CFG:\n";
	clang::LangOptions lo;
	lo.CPlusPlus = true;
	(*fIt)->function->cfg->print(llvm::outs(), lo, false);
#endif

	(*fIt)->analyzeFunc(rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While analyzeFunc()\n";
	    return;
	}
	//rv = false; return;
	(*fIt)->backSubstitution(rv);
	if (!rv)
	    return;

#ifdef DEBUG
	llvm::outs() << "DEBUG: symExprMap of function: " 
		     << (*fIt)->function->funcName << "\n";
	(*fIt)->symVisitor->printSymExprMap(true);
#endif

	if ((*fIt)->function->funcName.compare("main") == 0)
	    mainSym = (*fIt)->symVisitor;
    }

    if (!mainSym) {
	llvm::outs() << "ERROR: NULL GetSymExprVisitor for main\n";
	rv = false;
	return;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Before getSymExprsForInitialExprsOfGlobalVar()\n";
    for (std::vector<FunctionAnalysis*>::iterator fIt = analysisInfo.begin();
	    fIt != analysisInfo.end(); fIt++) {
	llvm::outs() << "DEBUG: symExprMap of function: " 
		     << (*fIt)->function->funcName << "\n";
	(*fIt)->symVisitor->printSymExprMap(true);
    }
#endif
    // Fix symbolic expressions for globalvar. First, get symbolic exprs for
    // initial values of all global vars
    getSymExprsForInitialExprsOfGlobalVars(rv);
    if (!rv) {
	llvm::outs() << "ERROR: While getSymExprsForInitialExprsOfGlobalVars()\n";
	rv = false;
	return;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: After getSymExprsForInitialExprsOfGlobalVar()\n";
    for (std::vector<FunctionAnalysis*>::iterator fIt = analysisInfo.begin();
	    fIt != analysisInfo.end(); fIt++) {
	llvm::outs() << "DEBUG: symExprMap of function: " 
		     << (*fIt)->function->funcName << "\n";
	(*fIt)->symVisitor->printSymExprMap(true);
    }
#endif
    // Then go through the inlined program and fix things
    GlobalVarSymExprVisitor gvVisitor(SM, this, mainSym, customInputFuncs,
	customOutputFuncs);
    gvVisitor.setCurrFunction(main);
    gvVisitor.setSymExprsForGlobalVars(globalVarInitialExprs);
    runVisitorOnInlinedProgram(main->cfg, &gvVisitor, rv);

    if (gvVisitor.error || !rv) {
	llvm::outs() << "ERROR: While running GlobalVarSymExprVisitor on inlined main\n";
	rv = false;
	return;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: After GlobalVarSymExprVisitor\n";
    for (std::vector<FunctionAnalysis*>::iterator fIt = analysisInfo.begin();
	    fIt != analysisInfo.end(); fIt++) {
	llvm::outs() << "DEBUG: symExprMap of function: " 
		     << (*fIt)->function->funcName << "\n";
	(*fIt)->symVisitor->printSymExprMap(true);
    }
#endif
    // Before running InitUpdateStmtVisitor, check whether the program exhibits
    // the pattern where the input array is read as a scalar and then assigned
    // to an array.
    ReadArrayAsScalarVisitor rasVisitor(SM, this, mainSym, customInputFuncs,
	customOutputFuncs);
    rasVisitor.setCurrFunction(main);
    runVisitorOnInlinedProgram(main->cfg, &rasVisitor, rv);

    if (rasVisitor.error) {
	llvm::outs() << "ERROR: While running ReadArrayAsScalarVisitor on inlined main\n";
	rv = false;
	return;
    }

    for (std::map<int, std::pair<InputVar, StmtDetails*> >::iterator mIt =
	    rasVisitor.inputLineToAssignmentLine.begin(); mIt != 
	    rasVisitor.inputLineToAssignmentLine.end(); mIt++) {
	for (std::vector<InputVar>::iterator inIt = inputs.begin(); inIt != 
		inputs.end(); inIt++) {
	    if (inIt->inputCallLine == mIt->first) {
		inIt->inputCallLine = mIt->second.first.inputCallLine;
		inIt->var = mIt->second.first.var;
		inIt->varExpr = mIt->second.first.varExpr;
		break;
	    }
	}

	unsigned i=0;
	for (std::vector<StmtDetails*>::iterator inIt = inputStmts.begin();
		inIt != inputStmts.end(); inIt++, i++) {
	    if ((*inIt)->lineNum == mIt->first) {
		inputStmts[i] = mIt->second.second;
		break;
	    }
	}
    }

#ifdef DEBUG
    for (std::vector<FunctionAnalysis*>::iterator fIt = analysisInfo.begin();
	    fIt != analysisInfo.end(); fIt++) {
	llvm::outs() << "DEBUG: symExprMap of function: " 
		     << (*fIt)->function->funcName << "\n";
	(*fIt)->symVisitor->printSymExprMap(true);
    }
#endif

    InitUpdateStmtVisitor iuVisitor(SM, this, mainSym, customInputFuncs, 
	customOutputFuncs);
    iuVisitor.setCurrFunction(main);

#if 0
    std::unique_ptr<CFG> mainCFG = main.constructCFG(rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot construct CFG of function:\n";
	main.print();
	return;
    }
#endif
    runVisitorOnInlinedProgram(main->cfg, &iuVisitor, rv);

    if (iuVisitor.error) {
	llvm::outs() << "ERROR: While running InitUpdateStmtVisitor on inlined main\n";
	rv = false;
	return;
    }

    initStmts.insert(initStmts.end(), iuVisitor.initStmts.begin(), 
	iuVisitor.initStmts.end());
    updateStmts.insert(updateStmts.end(), iuVisitor.updateStmts.begin(),
	iuVisitor.updateStmts.end());

#ifdef DEBUG
    llvm::outs() << "DEBUG: Init Stmts found: " << initStmts.size() << "\n";
    for (std::vector<StmtDetails*>::iterator iIt = initStmts.begin(); iIt !=
	    initStmts.end(); iIt++)
	//(*iIt)->print();
	if ((*iIt)->origStmt)
	    llvm::outs() << Helper::prettyPrintStmt((*iIt)->origStmt) << "\n";
	else
	    llvm::outs() << "NULL\n";
    llvm::outs() << "DEBUG: Update Stmts found: " << updateStmts.size() << "\n";
    for (std::vector<StmtDetails*>::iterator uIt = updateStmts.begin(); uIt !=
	    updateStmts.end(); uIt++)
	//(*uIt)->print();
	if ((*uIt)->origStmt)
	    llvm::outs() << Helper::prettyPrintStmt((*uIt)->origStmt) << " [" 
			 << (*uIt)->lineNum << "]\n";
	else
	    llvm::outs() << "NULL\n";
#endif
}

FunctionAnalysis* MainFunction::getAnalysisObjOfFunction(FunctionDetails* f,
	bool &rv) {
    rv = true;
    for (std::vector<FunctionAnalysis*>::iterator fIt = analysisInfo.begin(); 
	    fIt != analysisInfo.end(); fIt++) {
	if (f->funcName.compare((*fIt)->function->funcName) == 0 && 
		f->funcStartLine == (*fIt)->function->funcStartLine && 
		f->funcEndLine == (*fIt)->function->funcEndLine)
	    return *fIt;
    }

    llvm::outs() << "ERROR: Could not find FunctionAnalysis obj for "
		 << f->funcName << "\n";
    rv = false;
    return NULL;
}

FunctionDetails* MainFunction::getDetailsOfFunction(std::string fName,
int startLine, int endLine, bool &rv) {
    rv = true;
    for (std::vector<FunctionDetails*>::iterator fIt = functions.begin();
	    fIt != functions.end(); fIt++) {
	if ((*fIt)->funcName.compare(fName) == 0 && (*fIt)->funcStartLine == startLine
		&& (*fIt)->funcEndLine == endLine) {
	    return *fIt;
	}
    }

    llvm::outs() << "ERROR: Could not find FunctionDetails obj for "
		 << fName << "\n";
    rv = false;
    return NULL;
}

#if 0
void MainFunction::sequenceInputs(bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering MainFunction::sequenceInputs()\n";
#endif
    rv = true;
    FunctionAnalysis mainFA = getAnalysisObjOfFunction(main, rv);
    if (!rv)
	return;

    SequenceInputsVisitor siv(SM, &cg, this, main, mainFA.inputVars);
    siv.TraverseDecl(const_cast<FunctionDecl*>(main.fd));
    if (siv.error)
	return;

    inputs.insert(inputs.end(), siv.sequencedInputVars.begin(),
	siv.sequencedInputVars.end());
    inputs.insert(inputs.end(), siv.inputsInCurrFunc.begin(),
	siv.inputsInCurrFunc.end());

    int currID = 1;
    for (std::vector<InputVar>::iterator it = inputs.begin(); it !=
	    inputs.end(); it++) {
	it->progID = currID++;
    }
}
#endif

void MainFunction::findTestCaseVar(bool &rv) {
    rv = true;
    if (inputs.size() == 0) {
	llvm::outs() << "ERROR: No inputs found\n";
	rv = false;
	return;
    }
    testCaseVar = inputs[0].clone();
#ifdef DEBUG
    llvm::outs() << "DEBUG: Testcase var:\n";
    testCaseVar.print();
#endif
}

FunctionDetails* MainFunction::getFunctionDetailsFromCallGraphNode(CallGraphNode*
n, bool &rv) {
    rv = true;

    FunctionDetails* currFunction;
    if (!n) {
	llvm::outs() << "ERROR: Null CallGraphNode\n";
	rv = false;
	return NULL;
    }

    Decl* D = n->getDecl();
    if (!D) {
	llvm::outs() << "ERROR: Cannot get Decl from CallGraphNode\n";
	rv = false;
	return NULL;
    }

    if (!isa<FunctionDecl>(D)) {
	llvm::outs() << "ERROR: Decl obtained from CallGraphNode is not "
		     << "FunctionDecl\n";
	rv = false;
	return NULL;
    }

    FunctionDecl* fd = dyn_cast<FunctionDecl>(D);
    std::string funcName = fd->getQualifiedNameAsString();
    if (isCustomInputOutputFunction(funcName)) {
	currFunction = new FunctionDetails;
	currFunction->funcName = funcName;
	currFunction->funcStartLine = SM->getExpansionLineNumber(fd->getLocStart());
	currFunction->funcEndLine = SM->getExpansionLineNumber(fd->getLocEnd());
	if (isCustomInputFunction(funcName)) 
	    currFunction->isCustomInputFunction = true;
	if (isCustomOutputFunction(funcName))
	    currFunction->isCustomOutputFunction = true;
	currFunction->fd = fd;
    } else {
	currFunction = Helper::getFunctionDetailsFromDecl(SM, D, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot obtain FunctionDetails from Decl\n";
	    return NULL;
	}
    }

    return currFunction;
}

void MainFunction::topologicalSort(CallGraphNode* currNode,
std::vector<FunctionDetails*> &visitedFunctions, 
std::stack<FunctionDetails*> &stack, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: topologicalSort()\n";
    currNode->print(llvm::outs());
    llvm::outs() << "\n";
#endif
    rv = true;

    FunctionDetails* currFunction = getFunctionDetailsFromCallGraphNode(currNode, rv);
    if (!rv)
	return;
    // Skip if the currFunction is a library function
    if (currFunction->isLibraryFunction) return;
    // Skip if the currFunction is a custom input/output function
    if (currFunction->isCustomInputFunction ||
	    currFunction->isCustomOutputFunction)
	return;

    visitedFunctions.push_back(currFunction);

    for (CallGraphNode::iterator cIt = currNode->begin(); cIt !=
	    currNode->end(); cIt++) {
	FunctionDetails* adjFunction = getFunctionDetailsFromCallGraphNode(*cIt, rv);
	if (!rv)
	    return;
	// Skip if the adjFunction is a library function
	if (adjFunction->isLibraryFunction) continue;
	// Skip if the adjFunction is a custom input/output function
	if (adjFunction->isCustomInputFunction ||
		adjFunction->isCustomOutputFunction)
	    continue;

	// check if function is visited
	bool visited = false;
	for (std::vector<FunctionDetails*>::iterator fIt =
		visitedFunctions.begin(); fIt != visitedFunctions.end();
		fIt++) {
	    if ((*fIt)->equals(adjFunction)) {
		visited = true;
		break;
	    }
	}
	if (!visited) {
	    topologicalSort(*cIt, visitedFunctions, stack, rv);
	    if (!rv)
		return;
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Inserted currFunction into stack:\n";
    currFunction->print();
#endif
    stack.push(currFunction);
}

void MainFunction::runVisitorOnInlinedProgram(std::unique_ptr<CFG> &cfg,
InputOutputStmtVisitor* ioVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering MainFunction::runVisitorOnInlinedProgram(InputOutputStmtVisitor)\n";
#endif
    rv = true;

    std::stack<unsigned> workList;
    std::set<unsigned> processedBlocks;

    CFGBlock entry = cfg->getEntry();
    unsigned entryBlockID = entry.getBlockID();
    workList.push(entryBlockID);

    while (!(workList.empty())) {

	unsigned currBlockID = workList.top();
	workList.pop();

#ifdef DEBUG
	llvm::outs() << "DEBUG: currBlock: " << currBlockID << "\n";
#endif

	// Add this block to the set of processed blocks
	processedBlocks.insert(currBlockID);

	ioVisitor->setCurrBlock(currBlockID);

	CFGBlock* currBlock = getBlockFromID(cfg, currBlockID);
	if (!currBlock) {
	    llvm::outs() << "ERROR: Cannot obtain CFGBlock* of block " 
			 << currBlockID << "\n";
	    rv = false;
	    return;
	}

	for (CFGBlock::iterator eIt = currBlock->begin(); eIt !=
		currBlock->end(); eIt++) {
	    if (eIt->getKind() == CFGElement::Kind::Statement) {
		CFGStmt currCFGStmt = eIt->castAs<CFGStmt>();
		const Stmt* currStmt = currCFGStmt.getStmt();
		if (!currStmt) {
		    llvm::outs() << "ERROR: Cannot get Stmt* from CFGStmt in "
				 << "block " << currBlockID << "\n";
		    rv = false;
		    return;
		}

		Stmt* S = const_cast<Stmt*>(currStmt);

		ioVisitor->TraverseStmt(S);
		if (ioVisitor->error) {
		    llvm::outs() << "ERROR: While traversing stmt\n";
		    rv = false;
		    return;
		}
	    }
	}

	for (CFGBlock::succ_reverse_iterator sIt = currBlock->succ_rbegin(); 
		sIt != currBlock->succ_rend(); sIt++) {
	    CFGBlock* succBlock = sIt->getReachableBlock();
	    if (!succBlock) continue;
#ifdef DEBUG
	    llvm::outs() << "DEBUG: succBlock: " << succBlock->getBlockID() 
			 << "\n";
#endif
	    if (processedBlocks.find(succBlock->getBlockID()) == 
		    processedBlocks.end())
		workList.push(succBlock->getBlockID());
	}
    }
}

void MainFunction::runVisitorOnInlinedProgram(std::unique_ptr<CFG> &cfg,
ReadArrayAsScalarVisitor* rasVisitor, bool &rv) {
    rv = true;

    std::stack<unsigned> workList;
    std::set<unsigned> processedBlocks;

    CFGBlock entry = cfg->getEntry();
    unsigned entryBlockID = entry.getBlockID();
    workList.push(entryBlockID);

    while (!(workList.empty())) {

	unsigned currBlockID = workList.top();
	workList.pop();

	// Add this block to the set of processed blocks
	processedBlocks.insert(currBlockID);

	rasVisitor->setCurrBlock(currBlockID);

	CFGBlock* currBlock = getBlockFromID(cfg, currBlockID);
	if (!currBlock) {
	    llvm::outs() << "ERROR: Cannot obtain CFGBlock* of block " 
			 << currBlockID << "\n";
	    rv = false;
	    return;
	}

	for (CFGBlock::iterator eIt = currBlock->begin(); eIt !=
		currBlock->end(); eIt++) {
	    if (eIt->getKind() == CFGElement::Kind::Statement) {
		CFGStmt currCFGStmt = eIt->castAs<CFGStmt>();
		const Stmt* currStmt = currCFGStmt.getStmt();
		if (!currStmt) {
		    llvm::outs() << "ERROR: Cannot get Stmt* from CFGStmt in "
				 << "block " << currBlockID << "\n";
		    rv = false;
		    return;
		}

		Stmt* S = const_cast<Stmt*>(currStmt);

		rasVisitor->TraverseStmt(S);
		if (rasVisitor->error) {
		    llvm::outs() << "ERROR: While traversing stmt\n";
		    rv = false;
		    return;
		}
	    }
	}

	for (CFGBlock::succ_reverse_iterator sIt = currBlock->succ_rbegin(); 
		sIt != currBlock->succ_rend(); sIt++) {
	    CFGBlock* succBlock = sIt->getReachableBlock();
	    if (processedBlocks.find(succBlock->getBlockID()) == 
		    processedBlocks.end())
		workList.push(succBlock->getBlockID());
	}
    }
}

void MainFunction::runVisitorOnInlinedProgram(std::unique_ptr<CFG> &cfg,
GlobalVarSymExprVisitor* gvVisitor, bool &rv) {
    rv = true;

    std::stack<unsigned> workList;
    std::set<unsigned> processedBlocks;

    CFGBlock entry = cfg->getEntry();
    unsigned entryBlockID = entry.getBlockID();
    workList.push(entryBlockID);

    while (!(workList.empty())) {

	unsigned currBlockID = workList.top();
	workList.pop();

	// Add this block to the set of processed blocks
	processedBlocks.insert(currBlockID);

	gvVisitor->setCurrBlock(currBlockID);

	CFGBlock* currBlock = getBlockFromID(cfg, currBlockID);
	if (!currBlock) {
	    llvm::outs() << "ERROR: Cannot obtain CFGBlock* of block " 
			 << currBlockID << "\n";
	    rv = false;
	    return;
	}

	for (CFGBlock::iterator eIt = currBlock->begin(); eIt !=
		currBlock->end(); eIt++) {
	    if (eIt->getKind() == CFGElement::Kind::Statement) {
		CFGStmt currCFGStmt = eIt->castAs<CFGStmt>();
		const Stmt* currStmt = currCFGStmt.getStmt();
		if (!currStmt) {
		    llvm::outs() << "ERROR: Cannot get Stmt* from CFGStmt in "
				 << "block " << currBlockID << "\n";
		    rv = false;
		    return;
		}

		Stmt* S = const_cast<Stmt*>(currStmt);

		gvVisitor->TraverseStmt(S);
		if (gvVisitor->error) {
		    llvm::outs() << "ERROR: While traversing stmt\n";
		    rv = false;
		    return;
		}

		int line = gvVisitor->SM->getExpansionLineNumber(S->getLocStart());
		std::pair<unsigned, unsigned> offset;
		if (isa<DeclStmt>(S)) {
		    DeclStmt* DS = dyn_cast<DeclStmt>(S);
		    DeclMapTy::iterator I =
			gvVisitor->currAnalysisObj->DeclMap.find(DS->getSingleDecl());
		    if (I == gvVisitor->currAnalysisObj->DeclMap.end()) {
			llvm::outs() << "ERROR: Cannot find entry for expression in StmtMap\n";
			llvm::outs() << Helper::prettyPrintStmt(S);
			rv = false;
			return;
		    }
		    offset = std::make_pair(I->second.first, I->second.second);
		} else {
		    StmtMapTy::iterator I = gvVisitor->currAnalysisObj->StmtMap.find(S);
		    if (I == gvVisitor->currAnalysisObj->StmtMap.end()) {
			llvm::outs() << "ERROR: Cannot find entry for expression in StmtMap\n";
			llvm::outs() << Helper::prettyPrintStmt(S);
			rv = false;
			return;
		    }
		    offset = std::make_pair(I->second.first, I->second.second);
		}
		// Get the symexprs for global vars at this location
		std::vector<std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> > > 
		globalVarSymExprsAtSite;
		for (std::vector<std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> >
			>::iterator vIt = gvVisitor->globalVarSymExprs.begin(); vIt !=
			gvVisitor->globalVarSymExprs.end(); vIt++) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: globalVar:\n";
		    vIt->first->prettyPrint();
#endif
		    VarDetails currVar = vIt->first->var;
		    std::vector<SymbolicStmt*> symExprs =
			gvVisitor->getSymExprsOfVarInCurrentBlock(currVar, line, offset, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While getSymExprsOfVarInCurrentBlock()\n";
			currVar.print();
			rv = false;
			return;
		    }
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Num of symExprs: " << symExprs.size() << "\n";
		    for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); sIt != 
			    symExprs.end(); sIt++) {
			(*sIt)->prettyPrint(true);
			llvm::outs() << "\n";
		    }
#endif
		    globalVarSymExprsAtSite.push_back(std::make_pair(vIt->first, symExprs));
		}

#ifdef DEBUG
		llvm::outs() << "DEBUG: globalVarSymExprsAtCallSite:\n";
		for (std::vector<std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> >
			>::iterator gvIt = globalVarSymExprsAtSite.begin(); gvIt != 
			globalVarSymExprsAtSite.end();  gvIt++) {
		    llvm::outs() << "DEBUG: Global Var:\n";
		    gvIt->first->prettyPrint();
		    llvm::outs() << "\n";
		    llvm::outs() << "DEBUG: Num symbolic exprs: " << gvIt->second.size() << "\n";
		    for (std::vector<SymbolicStmt*>::iterator sIt = gvIt->second.begin();
			    sIt != gvIt->second.end(); sIt++) {
			(*sIt)->prettyPrint(true);
			llvm::outs() << "\n";
		    }
		}
#endif

		for (std::vector<std::pair<StmtDetails*, std::vector<SymbolicStmt*> > >::iterator 
			symIt = gvVisitor->symVisitor->symExprMap.begin(); symIt !=
			gvVisitor->symVisitor->symExprMap.end(); symIt++) {
		    std::vector<SymbolicStmt*> newSymExprsAfterReplacingGlobalVars;
		    for (std::vector<SymbolicStmt*>::iterator stIt = symIt->second.begin(); 
			    stIt != symIt->second.end(); stIt++) {
			std::vector<SymbolicStmt*> tempSymExprs =
			    gvVisitor->replaceGlobalVarsWithSymExprs(globalVarSymExprsAtSite,
			    *stIt, rv);
			if (!rv) {
			    llvm::outs() << "ERROR: While replacing global vars\n";
			    rv = false;
			    return;
			}
			newSymExprsAfterReplacingGlobalVars.insert(
			    newSymExprsAfterReplacingGlobalVars.end(), tempSymExprs.begin(),
			    tempSymExprs.end());
		    }
		    symIt->second = newSymExprsAfterReplacingGlobalVars;
		}
	    }
	}

	for (CFGBlock::succ_reverse_iterator sIt = currBlock->succ_rbegin(); 
		sIt != currBlock->succ_rend(); sIt++) {
	    CFGBlock* succBlock = sIt->getReachableBlock();
	    if (processedBlocks.find(succBlock->getBlockID()) == 
		    processedBlocks.end())
		workList.push(succBlock->getBlockID());
	}
    }

    // Get symExprs of global vars at the return site, only if the current
    // function is not main
    if (gvVisitor->currFunction->funcName.compare("main") == 0) return;
    CFGBlock exit = cfg->getExit();
    std::vector<std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> > >
    globalVarSymExprsAtReturnSite;
    gvVisitor->setCurrBlock(exit.getBlockID());
    int currLine;
    if (gvVisitor->currAnalysisObj->blockToLines.find(exit.getBlockID()) ==
	    gvVisitor->currAnalysisObj->blockToLines.end()) {
	currLine = -1;
    } else {
	currLine =
	    gvVisitor->currAnalysisObj->blockToLines[exit.getBlockID()].first;
    }
    std::pair<unsigned, unsigned> offset = std::make_pair(0,0);
    for (std::vector<std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> >
	    >::iterator vIt = gvVisitor->globalVarSymExprs.begin(); vIt !=
	    gvVisitor->globalVarSymExprs.end(); vIt++) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: globalVar:\n";
	vIt->first->prettyPrint();
	llvm::outs() << "\n";
#endif
	VarDetails currVar = vIt->first->var;
	std::vector<SymbolicStmt*> symExprs = 
	    gvVisitor->getSymExprsOfVarInCurrentBlock(currVar, currLine, offset, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While getSymExprsOfVarInCurrentBlock()\n";
	    currVar.print();
	    rv = false;
	    return;
	}
#ifdef DEBUG
	llvm::outs() << "DEBUG: Num of symExprs: " << symExprs.size() << "\n";
	for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin(); sIt != 
		symExprs.end(); sIt++) {
	    (*sIt)->prettyPrint(true);
	    llvm::outs() << "\n";
	}
#endif
	gvVisitor->globalVarSymExprsAtExit.push_back(std::make_pair(vIt->first,
	    symExprs));
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: globalVarSymExprsAtExit:\n";
    for (std::vector<std::pair<VarDeclDetails*, std::vector<SymbolicStmt*> >
	    >::iterator gvIt = gvVisitor->globalVarSymExprsAtExit.begin(); gvIt != 
	    gvVisitor->globalVarSymExprsAtExit.end(); gvIt++) {
	llvm::outs() << "DEBUG: Global Var:\n";
	gvIt->first->prettyPrint();
	llvm::outs() << "\n";
	llvm::outs() << "DEBUG: Num symbolic exprs: " << gvIt->second.size() 
		     << "\n";
	for (std::vector<SymbolicStmt*>::iterator sIt = gvIt->second.begin();
		sIt != gvIt->second.end(); sIt++) {
	    (*sIt)->prettyPrint(true);
	    llvm::outs() << "\n";
	}
    }
#endif
}

void MainFunction::runVisitorOnInlinedProgram(std::unique_ptr<CFG> &cfg,
InitUpdateStmtVisitor* iuVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering MainFunction::runVisitorOnInlinedProgram("
		 << iuVisitor->currFunction->funcName << ") with InitUpdateStmtVisitor\n";
#endif
    rv = true;

    std::stack<unsigned> workList;
    std::set<unsigned> processedBlocks;

    CFGBlock entry = cfg->getEntry();
    unsigned entryBlockID = entry.getBlockID();
    workList.push(entryBlockID);

    while (!(workList.empty())) {

	unsigned currBlockID = workList.top();
	workList.pop();

	// Add this block to the set of processed blocks
	processedBlocks.insert(currBlockID);

	iuVisitor->setCurrBlock(currBlockID);

	CFGBlock* currBlock = getBlockFromID(cfg, currBlockID);
	if (!currBlock) {
	    llvm::outs() << "ERROR: Cannot obtain CFGBlock* of block " 
			 << currBlockID << "\n";
	    rv = false;
	    return;
	}

	for (CFGBlock::iterator eIt = currBlock->begin(); eIt !=
		currBlock->end(); eIt++) {
	    if (eIt->getKind() == CFGElement::Kind::Statement) {
		CFGStmt currCFGStmt = eIt->castAs<CFGStmt>();
		const Stmt* currStmt = currCFGStmt.getStmt();
		if (!currStmt) {
		    llvm::outs() << "ERROR: Cannot get Stmt* from CFGStmt in "
				 << "block " << currBlockID << "\n";
		    rv = false;
		    return;
		}

		Stmt* S = const_cast<Stmt*>(currStmt);

		iuVisitor->TraverseStmt(S);
		if (iuVisitor->error) {
		    llvm::outs() << "ERROR: While traversing stmt\n";
		    rv = false;
		    return;
		}
	    }
	}

	for (CFGBlock::succ_reverse_iterator sIt = currBlock->succ_rbegin(); 
		sIt != currBlock->succ_rend(); sIt++) {
	    CFGBlock* succBlock = sIt->getReachableBlock();
	    if (processedBlocks.find(succBlock->getBlockID()) == 
		    processedBlocks.end())
		workList.push(succBlock->getBlockID());
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Init stmts in function " 
		 << iuVisitor->currFunction->funcName << ": " << iuVisitor->initStmts.size() 
		 << "\n";
    for (std::vector<StmtDetails*>::iterator iIt = iuVisitor->initStmts.begin();
	    iIt != iuVisitor->initStmts.end(); iIt++) {
	(*iIt)->prettyPrint();
	llvm::outs() << "\n";
    }
    llvm::outs() << "DEBUG: Update stmts in function " 
		 << iuVisitor->currFunction->funcName << ": " << iuVisitor->updateStmts.size() 
		 << "\n";
    for (std::vector<StmtDetails*>::iterator iIt = iuVisitor->updateStmts.begin();
	    iIt != iuVisitor->updateStmts.end(); iIt++) {
	(*iIt)->prettyPrint();
	llvm::outs() << "[" << (*iIt)->lineNum << "]\n";
    }
#endif

    std::sort(iuVisitor->initStmts.begin(), iuVisitor->initStmts.end(), 
	[](StmtDetails* x, StmtDetails* y) { return x->lineNum < y->lineNum; });
    std::sort(iuVisitor->callExprToInitStmts.begin(),
	iuVisitor->callExprToInitStmts.end(), 
	[](std::pair<CallExprDetails*, std::vector<StmtDetails*> > x, 
	   std::pair<CallExprDetails*, std::vector<StmtDetails*> > y)
	 { return x.first->lineNum < y.first->lineNum; });

    std::vector<StmtDetails*> tempVector = iuVisitor->initStmts;
    iuVisitor->initStmts.clear();
    std::vector<StmtDetails*>::iterator sIt = tempVector.begin();
    std::vector<std::pair<CallExprDetails*, std::vector<StmtDetails*> >
    >::iterator cIt = iuVisitor->callExprToInitStmts.begin();
    while (sIt != tempVector.end() || cIt !=
	    iuVisitor->callExprToInitStmts.end()) {
	if (sIt != tempVector.end() && cIt !=
		iuVisitor->callExprToInitStmts.end()) {
	    if ((*sIt)->lineNum < cIt->first->lineNum) {
		iuVisitor->initStmts.push_back(*sIt);
		sIt++;
	    } else {
		iuVisitor->initStmts.insert(iuVisitor->initStmts.end(),
		    cIt->second.begin(), cIt->second.end());
		cIt++;
	    }
	} else if (sIt == tempVector.end()) {
	    while (cIt != iuVisitor->callExprToInitStmts.end()) {
		iuVisitor->initStmts.insert(iuVisitor->initStmts.end(),
		    cIt->second.begin(), cIt->second.end());
		cIt++;
	    }
	} else if (cIt == iuVisitor->callExprToInitStmts.end()) {
	    while (sIt != tempVector.end()) {
		iuVisitor->initStmts.push_back(*sIt);
		sIt++;
	    }
	} else {
	    break;
	}
    }

    std::sort(iuVisitor->updateStmts.begin(), iuVisitor->updateStmts.end(), 
	[](StmtDetails* x, StmtDetails* y) { return x->lineNum < y->lineNum; });
    std::sort(iuVisitor->callExprToUpdateStmts.begin(),
	iuVisitor->callExprToUpdateStmts.end(), 
	[](std::pair<CallExprDetails*, std::vector<StmtDetails*> > x, 
	   std::pair<CallExprDetails*, std::vector<StmtDetails*> > y)
	 { return x.first->lineNum < y.first->lineNum; });

    tempVector = iuVisitor->updateStmts;
    iuVisitor->updateStmts.clear();
    sIt = tempVector.begin();
    cIt = iuVisitor->callExprToUpdateStmts.begin();
    while (sIt != tempVector.end() || cIt !=
	    iuVisitor->callExprToUpdateStmts.end()) {
	if (sIt != tempVector.end() && cIt !=
		iuVisitor->callExprToUpdateStmts.end()) {
	    if ((*sIt)->lineNum < cIt->first->lineNum) {
		iuVisitor->updateStmts.push_back(*sIt);
		sIt++;
	    } else {
		iuVisitor->updateStmts.insert(iuVisitor->updateStmts.end(),
		    cIt->second.begin(), cIt->second.end());
		cIt++;
	    }
	} else if (sIt == tempVector.end()) {
	    while (cIt != iuVisitor->callExprToUpdateStmts.end()) {
		iuVisitor->updateStmts.insert(iuVisitor->updateStmts.end(),
		    cIt->second.begin(), cIt->second.end());
		cIt++;
	    }
	} else if (cIt == iuVisitor->callExprToUpdateStmts.end()) {
	    while (sIt != tempVector.end()) {
		iuVisitor->updateStmts.push_back(*sIt);
		sIt++;
	    }
	} else {
	    break;
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: After sorting:\n";
    llvm::outs() << "DEBUG: Init stmts in function " 
		 << iuVisitor->currFunction->funcName << ": " << iuVisitor->initStmts.size() 
		 << "\n";
    for (std::vector<StmtDetails*>::iterator iIt = iuVisitor->initStmts.begin();
	    iIt != iuVisitor->initStmts.end(); iIt++) {
	(*iIt)->prettyPrint();
	llvm::outs() << "\n";
    }
    llvm::outs() << "DEBUG: Update stmts in function " 
		 << iuVisitor->currFunction->funcName << ": " << iuVisitor->updateStmts.size() 
		 << "\n";
    for (std::vector<StmtDetails*>::iterator iIt = iuVisitor->updateStmts.begin();
	    iIt != iuVisitor->updateStmts.end(); iIt++) {
	(*iIt)->prettyPrint();
	llvm::outs() << "[" << (*iIt)->lineNum << "]\n";
    }
#endif
}

void MainFunction::runVisitorOnInlinedProgram(std::unique_ptr<CFG> &cfg, 
FunctionAnalysis* funcAnalysis, BlockStmtVisitor* bVisitor, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering runVisitorOnInlinedProgram("
		 << funcAnalysis->function->funcName << ", BlockStmtVisitor)\n";
#endif
    rv = true;
#ifdef DEBUG
    llvm::outs() << "DEBUG: outStmts: " << bVisitor->outputStmts.size() << "\n";
#endif

    //GetSymbolicExprVisitor* funcSym = funcAnalysis->symVisitor;
    std::vector<BlockDetails*> allBlocksIdentified;
    std::vector<std::pair<CallExprDetails*, std::vector<BlockDetails*> > >
    callExprToAllBlocksIdentified;

    std::stack<unsigned> workList;
    std::set<unsigned> processedBlocks;

    CFGBlock entry = cfg->getEntry();
    unsigned entryBlockID = entry.getBlockID();
    workList.push(entryBlockID);

    while (!(workList.empty())) {

	unsigned currBlockID = workList.top();
	workList.pop();

	// Get loops surrounding this block
#ifdef DEBUG
	llvm::outs() << "DEBUG: Finding loops of block " << currBlockID << "\n";
#endif
	std::vector<LoopDetails*> loops =
	    funcAnalysis->getLoopsOfBlock(currBlockID, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While obtaining loops of block " 
			 << currBlockID << "\n";
	    rv = false;
	    return;
	}
	// For each loop, find the symbolic expressions for init and final val
	for (std::vector<LoopDetails*>::iterator lIt = loops.begin(); lIt != 
		loops.end(); lIt++) {
	    (*lIt)->getSymbolicValuesForLoopHeader(bVisitor->SM,
		bVisitor->symVisitor, rv);
	    if (!rv) return;
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Loops of block: " << currBlockID << "\n";
	    for (std::vector<LoopDetails*>::iterator lIt = loops.begin(); lIt != 
		    loops.end(); lIt++) {
		(*lIt)->prettyPrintHere();
	    }
#endif
	}

	// Check if this block is already processed
	if (processedBlocks.find(currBlockID) != processedBlocks.end())
	    continue;
	// Add this block to the set of processed blocks
	processedBlocks.insert(currBlockID);

	bVisitor->setCurrBlock(currBlockID);

	CFGBlock* currBlock = getBlockFromID(cfg, currBlockID);
	if (!currBlock) {
	    llvm::outs() << "ERROR: Cannot obtain CFGBlock* of block " 
			 << currBlockID << "\n";
	    rv = false;
	    return;
	}

	for (CFGBlock::iterator eIt = currBlock->begin(); eIt !=
		currBlock->end(); eIt++) {
	    if (eIt->getKind() == CFGElement::Kind::Statement) {
		CFGStmt currCFGStmt = eIt->castAs<CFGStmt>();
		const Stmt* currStmt = currCFGStmt.getStmt();
		if (!currStmt) {
		    llvm::outs() << "ERROR: Cannot get Stmt* from CFGStmt in "
				 << "block " << currBlockID << "\n";
		    rv = false;
		    return;
		}

		Stmt* S = const_cast<Stmt*>(currStmt);
		//int currLine =
		    //bVisitor->SM->getExpansionLineNumber(S->getLocStart());
#ifdef DEBUG
		llvm::outs() << "DEBUG: Calling BlockStmtVisitor on Stmt: "
			     << Helper::prettyPrintStmt(S) << " in block "
			     << currBlockID << "\n";
#endif

		bVisitor->TraverseStmt(S);
		if (bVisitor->error) {
		    llvm::outs() << "ERROR: While traversing stmt\n";
		    rv = false;
		    return;
		}

		StmtDetails* currDetails = StmtDetails::getStmtDetails(
		    bVisitor->SM, S, currBlockID, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: Cannot obtain StmtDetails\n";
		    return;
		}

#ifdef DEBUG
		llvm::outs() << "DEBUG: blocksIdentified: " 
			     << bVisitor->blocksIdentified.size() << "\n";
#endif
		for (std::vector<BlockDetails*>::iterator bIt =
			bVisitor->blocksIdentified.begin(); bIt != 
			bVisitor->blocksIdentified.end(); bIt++) {
#ifdef DEBUG
		    if (*bIt) (*bIt)->prettyPrintHere();
		    else llvm::outs() << "NULL block\n";
#endif
		    //(*bIt)->loops.insert((*bIt)->loops.begin(), loops.begin(),
			//loops.end());
		    for (std::vector<LoopDetails*>::reverse_iterator lIt =
			    loops.rbegin(); lIt != loops.rend(); lIt++) {
			(*bIt)->loops.insert((*bIt)->loops.begin(),
			    (*lIt)->clone());
		    }
#ifdef DEBUG
		    llvm::outs() << "DEBUG: About to insert block\n";
#endif
		    allBlocksIdentified.push_back(*bIt);
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Inserting block to "
				 << "allBlocksIdentified:\n";
		    (*bIt)->prettyPrintHere();
#endif
		}
		bVisitor->blocksIdentified.clear();
		for (std::vector<std::pair<CallExprDetails*,
			std::vector<BlockDetails*> > >::iterator cIt = 
			bVisitor->callExprToBlocksIdentified.begin(); cIt != 
			bVisitor->callExprToBlocksIdentified.end(); cIt++) {
		  std::vector<std::pair<CallExprDetails*,
		    std::vector<BlockDetails*> > >::iterator ccIt;
		  bool foundCallExpr = false;
		  for (ccIt = callExprToAllBlocksIdentified.begin(); ccIt != 
			    callExprToAllBlocksIdentified.end(); ccIt++) {
		    if (cIt->first->equals(ccIt->first)) {
			foundCallExpr = true;
			break;
		    }
		  }
		  if (!foundCallExpr) {
		    std::pair<CallExprDetails*, std::vector<BlockDetails*> > p;
		    p.first = cIt->first;
		    callExprToAllBlocksIdentified.push_back(p);
		    ccIt = callExprToAllBlocksIdentified.end()-1;
		  }
		  for (std::vector<BlockDetails*>::iterator bIt =
			cIt->second.begin(); bIt != cIt->second.end();
			bIt++) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Block in callExprToBlocksIdentified:\n";
		    (*bIt)->prettyPrintHere();
#endif
		    for (std::vector<LoopDetails*>::reverse_iterator lIt = 
			    loops.rbegin(); lIt != loops.rend(); lIt++) {
			(*bIt)->loops.insert((*bIt)->loops.begin(),
			    (*lIt)->clone());
		    }
		    ccIt->second.push_back(*bIt);
		  }
		}
		bVisitor->callExprToBlocksIdentified.clear();

#ifdef DEBUG
		llvm::outs() << "DEBUG: Looking for input stmt\n";
#endif
		// Check if the current statement is one of the labelled stmts.
		// If so, get BlockDetails of the statement
		bool foundStmt = false;
		for (std::vector<StmtDetails*>::iterator iIt =
			bVisitor->inputStmts.begin(); iIt != 
			bVisitor->inputStmts.end(); iIt++) {
		    if (currDetails->equals(*iIt)) {
			BlockDetails* block = getBlockDetailsOfStmt(*iIt,
			    BlockStmtKind::INPUT, bVisitor->symVisitor,
			    funcAnalysis->function, rv);
			if (!rv) {
			    llvm::outs() << "ERROR: Cannot get BlockDetails of "
					 << "inputStmt\n";
			    return;
			}
#ifdef DEBUG
			if (loops.size() == 0) 
			    llvm::outs() << "DEBUG: No loops for the stmt\n";
			else
			    llvm::outs() << "DEBUG: Loops exist for the stmt: "
					 << loops.size() << "\n";
#endif
			//block->loops.insert(block->loops.begin(), loops.begin(),
			    //loops.end());
			for (std::vector<LoopDetails*>::reverse_iterator lIt =
				loops.rbegin(); lIt != loops.rend(); lIt++) {
			    block->loops.insert(block->loops.begin(),
				(*lIt)->clone());
			}
			//bVisitor->blocksIdentified.push_back(block);
			allBlocksIdentified.push_back(block);
#ifdef DEBUG
			llvm::outs() << "DEBUG: Found input stmt: "
				     << Helper::prettyPrintStmt(S) << "\n";
			block->prettyPrintHere();
#endif
			foundStmt = true;
			break;
		    }
		}
		if (foundStmt) continue;

#ifdef DEBUG
		llvm::outs() << "DEBUG: Looking for init stmt\n";
#endif
		for (std::vector<StmtDetails*>::iterator iIt =
			bVisitor->initStmts.begin(); iIt != 
			bVisitor->initStmts.end(); iIt++) {
		    if (currDetails->equals(*iIt)) {
			BlockDetails* block = getBlockDetailsOfStmt(*iIt,
			    BlockStmtKind::INIT, bVisitor->symVisitor,
			    funcAnalysis->function, rv);
			if (!rv) {
			    llvm::outs() << "ERROR: Cannot get BlockDetails of "
					 << "initStmt\n";
			    return;
			}
#ifdef DEBUG
			if (loops.size() == 0) 
			    llvm::outs() << "DEBUG: No loops for the stmt\n";
			else
			    llvm::outs() << "DEBUG: Loops exist for the stmt: "
					 << loops.size() << "\n";
#endif
			//block->loops.insert(block->loops.begin(), loops.begin(),
			    //loops.end());
			for (std::vector<LoopDetails*>::reverse_iterator lIt =
				loops.rbegin(); lIt != loops.rend(); lIt++)
			    block->loops.insert(block->loops.begin(),
				(*lIt)->clone());
			//bVisitor->blocksIdentified.push_back(block);
			allBlocksIdentified.push_back(block);
#ifdef DEBUG
			llvm::outs() << "DEBUG: Found init stmt\n";
			block->prettyPrintHere();
#endif
			foundStmt = true;
			break;
		    }
		}
		if (foundStmt) continue;

#ifdef DEBUG
		llvm::outs() << "DEBUG: updateStmts: " 
			     << bVisitor->updateStmts.size() << "\n";
#endif
		for (std::vector<StmtDetails*>::iterator iIt =
			bVisitor->updateStmts.begin(); iIt != 
			bVisitor->updateStmts.end(); iIt++) {
		    if (currDetails->equals(*iIt)) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: About to insert an update stmt\n";
#endif
			BlockDetails* block = getBlockDetailsOfStmt(*iIt,
			    BlockStmtKind::UPDT, bVisitor->symVisitor,
			    funcAnalysis->function, rv);
			if (!rv) {
			    llvm::outs() << "ERROR: Cannot get BlockDetails of "
					 << "udpateStmt\n";
			    return;
			}
#ifdef DEBUG
			if (loops.size() == 0) 
			    llvm::outs() << "DEBUG: No loops for the stmt\n";
			else
			    llvm::outs() << "DEBUG: Loops exist for the stmt: "
					 << loops.size() << "\n";
#endif
			//block->loops.insert(block->loops.begin(), loops.begin(),
			    //loops.end());
			for (std::vector<LoopDetails*>::reverse_iterator lIt =
				loops.rbegin(); lIt != loops.rend(); lIt++)
			    block->loops.insert(block->loops.begin(),
				(*lIt)->clone());
			//bVisitor->blocksIdentified.push_back(block);
			allBlocksIdentified.push_back(block);
#ifdef DEBUG
			llvm::outs() << "DEBUG: Found updt stmt\n";
			block->prettyPrintHere();
#endif
			foundStmt = true;
			break;
		    }
		}
		if (foundStmt) continue;

#ifdef DEBUG
		llvm::outs() << "DEBUG: Looking for output stmt\n";
#endif
		for (std::vector<StmtDetails*>::iterator iIt =
			bVisitor->outputStmts.begin(); iIt != 
			bVisitor->outputStmts.end(); iIt++) {
		    if (currDetails->equals(*iIt)) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: Checking if stmt is output:\n";
			currDetails->print();
			llvm::outs() << "DEBUG: Output Stmt:\n";
			(*iIt)->print();
#endif
			BlockDetails* block = getBlockDetailsOfStmt(*iIt,
			    BlockStmtKind::OUT, bVisitor->symVisitor,
			    funcAnalysis->function, rv);
			if (!rv) {
			    llvm::outs() << "ERROR: Cannot get BlockDetails of "
					 << "outStmt\n";
			    return;
			}
#ifdef DEBUG
			if (loops.size() == 0) 
			    llvm::outs() << "DEBUG: No loops for the stmt\n";
			else
			    llvm::outs() << "DEBUG: Loops exist for the stmt: "
					 << loops.size() << "\n";
#endif
			//block->loops.insert(block->loops.begin(), loops.begin(),
			    //loops.end());
			for (std::vector<LoopDetails*>::reverse_iterator lIt =
				loops.rbegin(); lIt != loops.rend(); lIt++)
			    block->loops.insert(block->loops.begin(),
				(*lIt)->clone());
			//bVisitor->blocksIdentified.push_back(block);
			allBlocksIdentified.push_back(block);
#ifdef DEBUG
			llvm::outs() << "DEBUG: Found output stmt\n";
			block->prettyPrintHere();
#endif
			foundStmt = true;
			break;
		    } else {
#ifdef DEBUG
			llvm::outs() << "DEBUG: Did not match output stmt\n";
			currDetails->prettyPrint();
			(*iIt)->prettyPrint();
#endif
		    }
		}
		if (!foundStmt) {
		    if (isa<BinaryOperatorDetails>(currDetails)) {
			BinaryOperatorDetails* binOp =
			    dyn_cast<BinaryOperatorDetails>(currDetails);
			if (binOp->opKind == BinaryOperatorKind::BO_Assign && 
				isa<ArraySubscriptExprDetails>(binOp->lhs)) {
			    bool exists = false;
			    for (std::vector<StmtDetails*>::iterator sIt = 
				    bVisitor->residualStmts.begin(); sIt!= 
				    bVisitor->residualStmts.end(); sIt++) {
				if (binOp->equals(*sIt)) {
				    exists = true;
				    break;
				}
			    }
			    if (!exists) bVisitor->residualStmts.push_back(binOp);
			}
		    }
		}
	    }
	}

	for (CFGBlock::succ_reverse_iterator sIt = currBlock->succ_rbegin(); 
		sIt != currBlock->succ_rend(); sIt++) {
	    CFGBlock* succBlock = sIt->getReachableBlock();
	    if (processedBlocks.find(succBlock->getBlockID()) == 
		    processedBlocks.end())
		workList.push(succBlock->getBlockID());
	}
    }

    //bVisitor->blocksIdentified.clear();
    //bVisitor->blocksIdentified.insert(bVisitor->blocksIdentified.end(),
	//allBlocksIdentified.begin(), allBlocksIdentified.end());
#ifdef DEBUG
    llvm::outs() << "DEBUG: All blocks identified\n";
    for (std::vector<BlockDetails*>::iterator bIt = allBlocksIdentified.begin();
	    bIt != allBlocksIdentified.end(); bIt++) {
	(*bIt)->prettyPrintHere();
    }
#endif

    llvm::outs() << "DEBUG: Print something here!\n";
    bVisitor->blocksIdentified.clear();
    std::sort(allBlocksIdentified.begin(), allBlocksIdentified.end(), 
	[](BlockDetails* x, BlockDetails* y) { 
	return (*x->stmts.begin())->stmt->lineNum < 
	    (*y->stmts.begin())->stmt->lineNum; });
#ifdef DEBUG
    llvm::outs() << "DEBUG: After sorting, all blocks identified:\n";
    for (std::vector<BlockDetails*>::iterator b1It =
	    allBlocksIdentified.begin(); b1It != allBlocksIdentified.end();
	    b1It++)
	(*b1It)->prettyPrintHere();
    llvm::outs() << "DEBUG: Before sorting, callExprToBlocksIdentified:\n";
    for (std::vector<std::pair<CallExprDetails*, std::vector<BlockDetails*> > 
	    >::iterator c1It = callExprToAllBlocksIdentified.begin(); 
	    c1It != callExprToAllBlocksIdentified.end(); c1It++) {
      llvm::outs() << "\nCallExpr:";
      c1It->first->prettyPrint();
      for (std::vector<BlockDetails*>::iterator b1It = c1It->second.begin(); 
	    b1It != c1It->second.end(); b1It++) 
	(*b1It)->prettyPrintHere();
    }
#endif

    std::sort(callExprToAllBlocksIdentified.begin(),
	callExprToAllBlocksIdentified.end(), 
	[](std::pair<CallExprDetails*, std::vector<BlockDetails*> > x, 
	   std::pair<CallExprDetails*, std::vector<BlockDetails*> > y)
	 { return x.first->lineNum < y.first->lineNum; });
#ifdef DEBUG
    llvm::outs() << "DEBUG: After sorting, callExprToBlocksIdentified:\n";
    for (std::vector<std::pair<CallExprDetails*, std::vector<BlockDetails*> > 
	    >::iterator c1It = callExprToAllBlocksIdentified.begin(); 
	    c1It != callExprToAllBlocksIdentified.end(); c1It++) {
      llvm::outs() << "\nCallExpr:";
      c1It->first->prettyPrint();
      for (std::vector<BlockDetails*>::iterator b1It = c1It->second.begin(); 
	    b1It != c1It->second.end(); b1It++) 
	(*b1It)->prettyPrintHere();
    }
#endif

    std::vector<BlockDetails*>::iterator bIt = allBlocksIdentified.begin();
    std::vector<std::pair<CallExprDetails*, std::vector<BlockDetails*> >
    >::iterator cIt = callExprToAllBlocksIdentified.begin();
    while (bIt != allBlocksIdentified.end() || cIt !=
	    callExprToAllBlocksIdentified.end()) {
	if (bIt != allBlocksIdentified.end() && cIt !=
		callExprToAllBlocksIdentified.end()) {
	    if ((*(*bIt)->stmts.begin())->stmt->lineNum < cIt->first->lineNum) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Adding block to blocksIdentified:\n";
		(*bIt)->prettyPrintHere();
#endif
		bVisitor->blocksIdentified.push_back(*bIt);
		bIt++;
	    } else {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Adding blocks to blocksIdentified:\n";
		for (std::vector<BlockDetails*>::iterator b1It =
			cIt->second.begin(); b1It != cIt->second.end(); b1It++)
		    (*b1It)->prettyPrintHere();
#endif
		bVisitor->blocksIdentified.insert(bVisitor->blocksIdentified.end(),
		    cIt->second.begin(), cIt->second.end());
		cIt++;
	    }
	} else if (bIt == allBlocksIdentified.end()) {
	    while (cIt != callExprToAllBlocksIdentified.end()) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Adding blocks to blocksIdentified:\n";
		for (std::vector<BlockDetails*>::iterator b1It =
			cIt->second.begin(); b1It != cIt->second.end(); b1It++)
		    (*b1It)->prettyPrintHere();
#endif
		bVisitor->blocksIdentified.insert(bVisitor->blocksIdentified.end(),
		    cIt->second.begin(), cIt->second.end());
		cIt++;
	    }
	} else if (cIt == callExprToAllBlocksIdentified.end()) {
	    while (bIt != allBlocksIdentified.end()) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Adding block to blocksIdentified:\n";
		(*bIt)->prettyPrintHere();
#endif
		bVisitor->blocksIdentified.push_back(*bIt);
		bIt++;
	    }
	} else {
	    break;
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: After sorting:\n";
    llvm::outs() << "DEBUG: All blocks identified\n";
    for (std::vector<BlockDetails*>::iterator bIt = 
	    bVisitor->blocksIdentified.begin(); bIt != 
	    bVisitor->blocksIdentified.end(); bIt++) {
	(*bIt)->prettyPrintHere();
    }
#endif
}

void MainFunction::getBlocks(bool &rv) {
    rv = true;
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering MainFunction::getBlocks()\n";
#endif
    // Get SymbolicExprVisitor Object of main
    GetSymbolicExprVisitor* mainSym = NULL;
    FunctionAnalysis* mainInfo = NULL;
    bool foundSymVisitor = false;
    for (std::vector<FunctionAnalysis*>::iterator aIt = analysisInfo.begin();
	    aIt != analysisInfo.end(); aIt++) {
	if ((*aIt)->function->equals(main)) {
	    mainSym = (*aIt)->symVisitor;
	    mainInfo =*aIt;
	    foundSymVisitor = true;
	    break;
	}
    }
    if (!foundSymVisitor) {
	llvm::outs() << "ERROR: Cannot find GetsymbolicExprVisitor object of "
		     << "main()\n";
	rv = false;
	return;
    }

    BlockStmtVisitor bVisitor(SM, this, mainSym, customInputFuncs,
	customOutputFuncs, inputStmts, initStmts, updateStmts, outStmts);
    bVisitor.setCurrFunction(main);

#if 0
    std::unique_ptr<CFG> mainCFG = main.constructCFG(rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot construct CFG of function:\n";
	main.print();
	return;
    }
#endif
#ifdef DEBUG
    llvm::outs() << "DEBUG: Calling BlockStmtVisitor on main\n";
#endif
    runVisitorOnInlinedProgram(main->cfg, mainInfo, &bVisitor, rv);
#ifdef DEBUG
    llvm::outs() << "DEBUG: Returning BlockStmtVisitor on main\n";
#endif

    if (bVisitor.error) {
	llvm::outs() << "ERROR: While running BlockStmtVisitor on inlined main\n";
	rv = false;
	return;
    }
    if (!rv) {
	llvm::outs() << "ERROR: While running BlockStmtVisitor on inlined main\n";
	return;
    }

    originalSummaryBlocks.insert(originalSummaryBlocks.end(), bVisitor.blocksIdentified.begin(),
	bVisitor.blocksIdentified.end());

    residualStmts.insert(residualStmts.end(), bVisitor.residualStmts.begin(),
	bVisitor.residualStmts.end());

#ifdef DEBUG
    llvm::outs() << "DEBUG: Summary blocks:\n";
    for (std::vector<BlockDetails*>::iterator bIt = originalSummaryBlocks.begin();
	    bIt != originalSummaryBlocks.end(); bIt++) {
	if (*bIt)
#if 0
	    if ((*bIt)->isOutput())
		(*bIt)->print();
	    else
#endif
		(*bIt)->prettyPrintHere();
	else {
	    llvm::outs() << "ERROR: NULL Summary block\n";
	    rv = false;
	    return;
	}
    }
#endif
}

BlockDetails* MainFunction::getBlockDetailsOfStmt(StmtDetails* st, 
BlockStmtKind label, GetSymbolicExprVisitor* visitor, FunctionDetails* func, 
bool &rv) {
    rv = true;
    BlockDetails* block = new BlockDetails;
    std::vector<BlockStmt*> bStmts;

    if (label == BlockStmtKind::INPUT) {
	block->label = BlockDetails::BlockKind::INPUT;
	std::vector<BlockInputStmt*> iStmts = BlockInputStmt::getBlockStmt(st,
	    visitor, func, rv);
	if (!rv) return block;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Input stmts identified:\n";
	for (std::vector<BlockInputStmt*>::iterator it = iStmts.begin(); it != 
		iStmts.end(); it++)
	    (*it)->prettyPrintHere();
#endif
	bStmts.insert(bStmts.end(), iStmts.begin(), iStmts.end());
    } else if (label == BlockStmtKind::INIT) {
	std::pair<std::vector<BlockInitStmt*>, std::vector<BlockUpdateStmt*> > p
	    = BlockInitStmt::getBlockStmt(st, visitor, func, rv);
	if (!rv) return block;
	if (p.first.size() != 0 && p.second.size() == 0)
	    block->label = BlockDetails::BlockKind::INIT;
	else if (p.first.size() == 0 && p.second.size() != 0)
	    block->label = BlockDetails::BlockKind::UPDT;
	else if (p.first.size() != 0 && p.second.size() != 0)
	    block->label = BlockDetails::BlockKind::HETEROGENEOUS;
	else {
	    llvm::outs() << "ERROR: No init/update stmts\n";
	    rv = false;
	    return block;
	}
	bStmts.insert(bStmts.end(), p.first.begin(), p.first.end());
	bStmts.insert(bStmts.end(), p.second.begin(), p.second.end());
    } else if (label == BlockStmtKind::UPDT) {
	block->label = BlockDetails::BlockKind::UPDT;
	std::vector<BlockUpdateStmt*> uStmts = BlockUpdateStmt::getBlockStmt(st,
	    visitor, func, rv);
	if (!rv) return block;
	bStmts.insert(bStmts.end(), uStmts.begin(), uStmts.end());
    } else if (label == BlockStmtKind::OUT) {
	block->label = BlockDetails::BlockKind::OUT;
	std::vector<BlockOutputStmt*> oStmts = BlockOutputStmt::getBlockStmt(st,
	    visitor, func, rv);
	if (!rv) return block;
	bStmts.insert(bStmts.end(), oStmts.begin(), oStmts.end());
    } else {
	llvm::outs() << "ERROR: Unknown label for the statement:\n";
	st->print();
	rv = false;
	return block;
    }

    block->stmts.insert(block->stmts.end(), bStmts.begin(), bStmts.end());
    return block;
}

void MainFunction::removeInputBlockForTestCaseVar(bool &rv) {
    rv = true;
    std::vector<BlockDetails*>::iterator bIt;
    bool foundTestCaseInputBlock = false;
    for (bIt = originalSummaryBlocks.begin(); bIt != originalSummaryBlocks.end();
	    bIt++) {
	if ((*bIt)->label != BlockDetails::BlockKind::INPUT) continue;

	// Input block for test case var does(should) not have any loops
	if ((*bIt)->loops.size() != 0) continue;
	std::vector<BlockStmt*>::iterator sIt;
	bool foundTestCaseInpStmt = false;
	for (sIt = (*bIt)->stmts.begin(); sIt != (*bIt)->stmts.end(); sIt++) {
	    if (!isa<BlockInputStmt>(*sIt)) {
		llvm::outs() << "ERROR: Stmt in input block is not "
			     << "BlockInputStmt\n";
		(*bIt)->print();
		rv = false;
		return;
	    }
	    BlockInputStmt* inpStmt = dyn_cast<BlockInputStmt>(*sIt);
	    if (!isa<BlockScalarInputStmt>(inpStmt)) continue;
	    BlockScalarInputStmt* testCaseInpStmt =
		dyn_cast<BlockScalarInputStmt>(inpStmt);
	    if (testCaseInpStmt->inputVar.equals(testCaseVar.var)) {
		foundTestCaseInpStmt = true;
		break;
	    }
	}
	if (!foundTestCaseInpStmt) continue;

	if ((*bIt)->stmts.size() > 1) {
	    llvm::outs() << "ERROR: Input block with test case var input stmt "
			 << "has more than one stmt\n";
	    (*bIt)->print();
	    rv = false;
	    return;
	}
	foundTestCaseInputBlock = true;
	break;
    }

    if (!foundTestCaseInputBlock) {
	llvm::outs() << "ERROR: Could not find input block with test case var "
		     << "input stmt\n";
	rv = false;
	//summaryBlocksWithoutTestCaseVar.insert(summaryBlocksWithoutTestCaseVar.end(), 
	    //originalSummaryBlocks.begin(), originalSummaryBlocks.end());
	return;
    }

    if (bIt != originalSummaryBlocks.begin()) {
	// If the testcasevar input block is not the first block, then check all
	// blocks before it are init and skip them since these are global inits
	for (std::vector<BlockDetails*>::iterator iIt =
		originalSummaryBlocks.begin(); iIt != bIt; iIt++) {
	    //if (!(*iIt)->isInit()) {
		llvm::outs() << "WARNING: Block before testcasevar inputBlock\n";
		//TODO: Put it in residual
		residualBlocks.push_back(*iIt);
		//rv = false;
		//return;
	    //}
	}
	//llvm::outs() << "ERROR: TestCaseVar InputBlock is not the first block\n";
	//rv = false;
	//return;
    }

    for (std::vector<BlockDetails*>::iterator iIt =
	    bIt+1; iIt != originalSummaryBlocks.end();
	    iIt++) {
	summaryBlocksWithoutTestCaseVar.push_back((*iIt)->clone(rv));
	if (!rv) {
	    llvm::outs() << "ERROR: While cloning summary block:\n";
	    (*iIt)->print();
	    return;
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: originalSummaryBlocks: " 
		 << originalSummaryBlocks.size() << "\n";
    llvm::outs() << "DEBUG: summaryBlocksWithoutTestCaseVar: " 
		 << summaryBlocksWithoutTestCaseVar.size() << "\n";
#endif
}

void MainFunction::removeTestCaseLoop(bool &rv) {
    rv = true;
    for (std::vector<BlockDetails*>::iterator bIt = 
	    summaryBlocksWithoutTestCaseVar.begin(); bIt != 
	    summaryBlocksWithoutTestCaseVar.end(); bIt++) {
	std::vector<LoopDetails>::iterator lIt;
	for (lIt = (*bIt)->loops.begin(); lIt != (*bIt)->loops.end(); lIt++) {
	    bool res = (lIt->isTestCaseLoop(&testCaseVar, rv));
	    if (!rv) {
		llvm::outs() << "ERROR: While checking if loop is testcaseloop\n";
		return;
	    }
	    if (res) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Found testcase loop:\n";
		lIt->prettyPrintHere();
#endif
		break;
	    }
	}
	if (lIt == (*bIt)->loops.end()) {
	    BlockDetails* clone = (*bIt)->clone(rv);
	    if (!rv) return;
	    blocksOutsideTestCaseLoop.push_back(clone);

	    (*bIt)->toDelete = true;
	    //llvm::outs() << "ERROR: Could not find test case loop in summary block:\n";
	    //(*bIt)->prettyPrintHere();
	    //rv = false;
	    //return;
	} else if (lIt != (*bIt)->loops.begin()) {
	    llvm::outs() << "ERROR: Testcase loop is not the first loop in the "
			 << "summary block:\n";
	    (*bIt)->prettyPrintHere();
	    rv = false;
	    return;
	}

	//(*bIt)->loops.erase(lIt);
#if 0
	for (std::vector<BlockStmt*>::iterator sIt = (*bIt)->stmts.begin(); sIt != 
		(*bIt)->stmts.end(); sIt++) {
	    //TODO: Requires sanity check to decide which guard to remove
	    // Unconditionally removing the last guard assuming it is the loop
	    // exit condition of the test case loop.
	    for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
		    >::iterator gIt = (*sIt)->guards.begin(); gIt != 
		    (*sIt)->guards.end(); gIt++) {
		std::vector<std::pair<SymbolicExpr*, bool> >::iterator
		rIt = gIt->end()-1;
		if (rIt->second == false) {
		    llvm::outs() << "ERROR: Last guard has false value\n";
		    rv = false;
		    return;
		}
		gIt->erase(rIt);
	    }
	
	    (*sIt)->guards.erase(std::remove_if((*sIt)->guards.begin(), (*sIt)->guards.end(),
		[](std::vector<std::pair<SymbolicExpr*, bool> > v) {return v.size() == 0;}),
		(*sIt)->guards.end());
	}
#endif
    }

#if 0
    summaryBlocksWithoutTestCaseVar.erase(std::remove_if(
	summaryBlocksWithoutTestCaseVar.begin(),
	summaryBlocksWithoutTestCaseVar.end(), 
	[](BlockDetails* b) { return b->toDelete; }));
#endif

    for (std::vector<BlockDetails*>::iterator bIt =
	    summaryBlocksWithoutTestCaseVar.begin(); bIt != 
	    summaryBlocksWithoutTestCaseVar.end(); bIt++) {
	if ((*bIt)->toDelete) continue;

	BlockDetails* clone = (*bIt)->clone(rv);
	if (!rv) return;
	clone->loops = std::vector<LoopDetails>((*bIt)->loops.begin()+1, 
	    (*bIt)->loops.end());
	summaryBlocksWithoutTestCaseLoop.push_back(clone);
    }

    for (std::vector<BlockDetails*>::iterator bIt =
	    blocksOutsideTestCaseLoop.begin(); bIt != 
	    blocksOutsideTestCaseLoop.end(); bIt++) {
	if (!(*bIt)->isInput()) continue;
	if ((*bIt)->stmts.size() != 1) continue;
	BlockStmt* st = (*bIt)->stmts[0];
	VarDetails inpVar;
	if (isa<BlockScalarInputStmt>(st)) {
	    BlockScalarInputStmt* sSt = dyn_cast<BlockScalarInputStmt>(st);
	    inpVar = sSt->inputVar;
	} else if (isa<BlockArrayInputStmt>(st)) {
	    BlockArrayInputStmt* aSt = dyn_cast<BlockArrayInputStmt>(st);
	    inpVar = aSt->inputArray->baseArray->var;
	}

	bool varUsed = false;
	for (std::vector<BlockDetails*>::iterator bbIt =
		summaryBlocksWithoutTestCaseLoop.begin(); bbIt != 
		summaryBlocksWithoutTestCaseLoop.end(); bbIt++) {
	    varUsed = (*bbIt)->containsVar(inpVar, rv);
	    if (!rv) return;
	    if (varUsed)
		break;
	}

	if (!varUsed) {
	    // Remove from InputVar list
	    std::vector<InputVar> tempInputVars;
	    for (std::vector<InputVar>::iterator iIt = inputs.begin(); iIt != 
		    inputs.end(); iIt++) {
		if (iIt->var.equals(inpVar)) continue;
		tempInputVars.push_back(*iIt);
	    }
	    // Remap ids for inputs
	    //int i = 1;
	    int i = inputs.begin()->progID;
	    for (std::vector<InputVar>::iterator iIt = tempInputVars.begin();
		    iIt != tempInputVars.end(); iIt++) {
		iIt->progID = i;
		i++;
	    }
	    inputs = tempInputVars;
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: After removing test case loop\n";
    llvm::outs() << "DEBUG: Summary blocks:\n";
    for (std::vector<BlockDetails*>::iterator bIt = 
	    summaryBlocksWithoutTestCaseLoop.begin(); bIt != 
	    summaryBlocksWithoutTestCaseLoop.end(); bIt++) {
	if (*bIt)
	    (*bIt)->prettyPrintHere();
	else {
	    llvm::outs() << "ERROR: NULL Summary block\n";
	    rv = false;
	    return;
	}
    }
#endif
}

void MainFunction::combineBlocks(bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering MainFunction::combineBlocks()\n";
#endif
    rv = true;

    for (std::vector<BlockDetails*>::iterator bI =
	    summaryBlocksWithoutTestCaseLoop.begin(); bI != 
	    summaryBlocksWithoutTestCaseLoop.end(); bI++) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: BlockI:\n";
	(*bI)->prettyPrintHere();
#endif
	BlockDetails* newBI = (*bI)->clone(rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While cloning block:\n";
	    (*bI)->print();
	    return;
	}
	if ((*bI)->loops.size() == 0) {
	    newBI->setLabel(rv);
	    if (!rv) return;
	    combinedBlocks.push_back(newBI);
	    continue;
	}
	std::vector<BlockDetails*>::iterator bJ;
	for (bJ = bI+1; bJ != summaryBlocksWithoutTestCaseLoop.end(); bJ++) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: BlockJ:\n";
	    (*bJ)->prettyPrintHere();
#endif
	    if ((*bI)->isLoopHeaderEqual(*bJ)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: BlockI = BlockJ\n";
#endif
		for (std::vector<BlockStmt*>::iterator sIt =
			(*bJ)->stmts.begin(); sIt != (*bJ)->stmts.end(); sIt++) {
		    newBI->stmts.push_back((*sIt)->clone(rv));
		    if (!rv) {
			llvm::outs() << "ERROR: While cloning BlockStmt:\n";
			(*sIt)->prettyPrintHere();
			return;
		    }
		}
	    } else {
#ifdef DEBUG
		llvm::outs() << "DEBUG: BlockI != BlockJ\n";
#endif
		if ((*bI)->isLoopHeaderPartiallyEqual(*bJ)) {
		    llvm::outs() << "ERROR: Loop headers are partially "
				 << "equal\n";
		    llvm::outs() << "BlockI:\n";
		    (*bI)->prettyPrintHere();
		    llvm::outs() << "BlockJ:\n";
		    (*bJ)->prettyPrintHere();
		    rv = false;
		    return;
		
		}
		break;
	    }
	}
	newBI->setLabel(rv); // Compute new label for block, it could become heterogeneous.
	if (!rv) return;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Inserting newBI:\n";
	newBI->prettyPrintHere();
#endif
	combinedBlocks.push_back(newBI);
	bI = bJ-1; // -1 since bI is going to be incremented.
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: After combining blocks:\n";
    llvm::outs() << "DEBUG: Summary blocks:\n";
    for (std::vector<BlockDetails*>::iterator bIt = 
	    combinedBlocks.begin(); bIt != 
	    combinedBlocks.end(); bIt++) {
	if (*bIt)
	    (*bIt)->prettyPrintHere();
	else {
	    llvm::outs() << "ERROR: NULL Summary block\n";
	    rv = false;
	    return;
	}
    }
#endif
}

bool MainFunction::isDPVarRecorded(VarDetails v) {
    for (std::vector<DPVar>::iterator dIt = allDPVars.begin(); dIt != 
	    allDPVars.end(); dIt++) {
	if (dIt->dpArray.equals(v)) return true;
    }
    return false;
}

int DPVar::counter = 0;

void MainFunction::getDPVars(bool &rv) {
    rv = true;
    for (std::vector<BlockDetails*>::iterator bIt = finalBlocks.begin(); 
	    bIt != finalBlocks.end(); bIt++) {
	bool fromUpdate = false, fromInit = false;
	if ((*bIt)->isInit() || (*bIt)->isUpdate()) {
	    for (std::vector<BlockStmt*>::iterator sIt = (*bIt)->stmts.begin();
		    sIt != (*bIt)->stmts.end(); sIt++) {
		VarDetails dpVar;
		if ((*bIt)->isInit()) {
		    BlockInitStmt* st = dyn_cast<BlockInitStmt>(*sIt);
		    if (!st) {
			llvm::outs() << "ERROR: Cannot obtain BlockInitStmt with "
				     << "InitBlock\n";
			rv = false;
			return;
		    }
		    dpVar = st->getDPVar(rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While obtaining DPVar from initStmt\n";
			return;
		    }
		    fromInit = true;
		} else if ((*bIt)->isUpdate()) {
		    BlockUpdateStmt* st = dyn_cast<BlockUpdateStmt>(*sIt);
		    if (!st) {
			llvm::outs() << "ERROR: Cannot obtain BlockUpdateStmt with "
				     << "UpdateBlock\n";
			rv = false;
			return;
		    }
		    dpVar = st->getDPVar(rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While obtaining DPVar from initStmt\n";
			return;
		    }
		    fromUpdate = true;
		}

		if (!isDPVarRecorded(dpVar)) {
		    DPVar dp(dpVar);
		    if (fromUpdate) 
			dp.hasUpdate = true;
		    else 
			dp.hasUpdate = false;
		    if (fromInit) 
			dp.hasInit = true;
		    else 
			dp.hasInit = false;
		    allDPVars.push_back(dp);
		}
	    }
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: allDPVars:\n";
    for (std::vector<DPVar>::iterator dIt = allDPVars.begin(); dIt != 
	    allDPVars.end(); dIt++) {
	dIt->print();
	llvm::outs() << "\n";
    }
#endif

    if (allDPVars.size() == 1) return;

    for (std::vector<DPVar>::iterator dIt = allDPVars.begin(); dIt != 
	    allDPVars.end(); dIt++) {
	for (std::vector<InputVar>::iterator inpIt = inputs.begin(); inpIt != 
		inputs.end(); inpIt++) {
	    if (dIt->dpArray.equals(inpIt->var) && !(dIt->hasUpdate)) {
		dIt->toDelete = true;
		break;
	    }
	}
    }

    allDPVars.erase(std::remove_if(allDPVars.begin(), allDPVars.end(), 
	[](DPVar d) {return d.toDelete;}), allDPVars.end());

    // Renumber the dp vars
    int counter = 1;
    for (std::vector<DPVar>::iterator dIt = allDPVars.begin(); dIt != 
	    allDPVars.end(); dIt++) {
	dIt->id = counter;
	counter++;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: After renumbering, allDPVars:\n";
    for (std::vector<DPVar>::iterator dIt = allDPVars.begin(); dIt != 
	    allDPVars.end(); dIt++) {
	dIt->print();
	llvm::outs() << "\n";
    }
#endif
}

void MainFunction::writeInput(std::ofstream &logFile, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering MainFunction::writeInput()\n";
#endif
    rv = true;
    logFile << "INPUT:\n";
    for (std::vector<BlockDetails*>::iterator bIt = finalBlocks.begin();
	    bIt != finalBlocks.end(); bIt++) {
	if ((*bIt)->isInput()) {
	    (*bIt)->prettyPrint(logFile, this, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While pretty printing Input Block:\n";
		(*bIt)->print();
		return;
	    }
	}
    }
}

void MainFunction::writeInit(std::ofstream &logFile, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering MainFunction::writeInit()\n";
#endif
    rv = true;
    logFile << "INIT:\n";
    for (std::vector<BlockDetails*>::iterator bIt = finalBlocks.begin(); 
	    bIt != finalBlocks.end(); bIt++) {
	if ((*bIt)->isInit()) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Writing InitBlock\n";
	    (*bIt)->prettyPrintHere();
#endif
	    (*bIt)->prettyPrint(logFile, this, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While pretty printing Init Block:\n";
		(*bIt)->prettyPrintHere();
		return;
	    }
	}
    }
}

void MainFunction::writeUpdate(std::ofstream &logFile, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering MainFunction::writeUpdate()\n";
#endif
    rv = true;
    logFile << "UPDATE:\n";
    for (std::vector<BlockDetails*>::iterator bIt = finalBlocks.begin(); 
	    bIt != finalBlocks.end(); bIt++) {
	if ((*bIt)->isUpdate()) {
#ifdef DEBUG
	    llvm::outs() << "Update:\n";
	    (*bIt)->prettyPrintHere();
#endif
	    (*bIt)->prettyPrint(logFile, this, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While pretty printing Update Block:\n";
		(*bIt)->print();
		return;
	    }
	}
    }
}

void MainFunction::writeOutput(std::ofstream &logFile, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering MainFunction::writeOutput()\n";
#endif
    rv = true;
    logFile << "OUTPUT:\n";
    for (std::vector<BlockDetails*>::iterator bIt = finalBlocks.begin(); 
	    bIt != finalBlocks.end(); bIt++) {
	if ((*bIt)->isOutput()) {
	    (*bIt)->prettyPrintHere();
	    (*bIt)->prettyPrint(logFile, this, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While pretty printing Output Block:\n";
		(*bIt)->print();
		return;
	    }
	}
    }
}

void MainFunction::writeResidual(std::ofstream &logFile, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering MainFunction::writeResidual()\n";
#endif
    rv = true;
    logFile << "RESIDUAL:\n";
    for (std::vector<FunctionAnalysis*>::iterator fIt = analysisInfo.begin(); 
	    fIt != analysisInfo.end(); fIt++) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Residual Stmts: " << (*fIt)->residualStmts.size()
		     << "\n";
#endif
	for (std::vector<StmtDetails*>::iterator rIt =
		(*fIt)->residualStmts.begin(); rIt != (*fIt)->residualStmts.end();
		rIt++) {
	    if (isa<ExprDetails>(*rIt)) {
		ExprDetails* expr = dyn_cast<ExprDetails>(*rIt);
		if (expr->origExpr) {
		    logFile << "Line " << expr->lineNum << ": " 
			    << Helper::prettyPrintExpr(expr->origExpr) << "\n";
		}
	    } else {
		if ((*rIt)->origStmt) {
		    logFile << "Line " << (*rIt)->lineNum << ": "
			    << Helper::prettyPrintStmt((*rIt)->origStmt) << "\n";
		}
	    }
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Residual Blocks: " << residualBlocks.size() << "\n";
#endif
    for (std::vector<BlockDetails*>::iterator bIt = residualBlocks.begin();
	    bIt != residualBlocks.end(); bIt++) {
	(*bIt)->prettyPrint(logFile, this, rv, true);
    }

    for (std::vector<StmtDetails*>::iterator sIt = residualStmts.begin();
	    sIt != residualStmts.end(); sIt++) {
	if ((*sIt)->origStmt)
	    logFile << Helper::prettyPrintStmt((*sIt)->origStmt) << "\n";
    }

    for (std::vector<BlockStmt*>::iterator sIt = residualInits.begin();
	    sIt != residualInits.end(); sIt++) {
	(*sIt)->prettyPrint(logFile, this, rv, true);
    }
}

void MainFunction::writeTypeMap(std::ofstream &logFile, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering MainFunction::writeTypeMap()\n";
#endif
    rv = true;
    logFile << "TYPEMAP:\n";
    for (std::vector<InputVar>::iterator iIt = inputs.begin(); iIt != 
	    inputs.end(); iIt++) {
	if (iIt->equals(testCaseVar)) continue;
#if 0
	// Don't do this now
	// Skip input variable that is the same as dp array
	bool isDPArray = false;
	for (std::vector<DPVar>::iterator dpIt = allDPVars.begin(); dpIt != 
		allDPVars.end(); dpIt++) {
	    if (iIt->var.equals(dpIt->dpArray)) {
		isDPArray = true;
		break;
	    }
	}
	if (isDPArray) continue;
#endif
#ifdef DEBUG
	llvm::outs() << "DEBUG: Going to print " << INPUTNAME << "_" 
		     << iIt->progID << "\n";
#endif
	// Check if there is any substituteArray for this input
	// var
	if (iIt->substituteArray.array) {
	    logFile << INPUTNAME << "_" << iIt->progID << ":" << iIt->var.varName << ":" 
		    << iIt->substituteArray.array->baseArray->var.type;
	    for (std::vector<SymbolicExpr*>::iterator sIt =
		    iIt->substituteArray.array->baseArray->var.arraySizeSymExprs.begin();
		    sIt != 
		    iIt->substituteArray.array->baseArray->var.arraySizeSymExprs.end();
		    sIt++) {
		logFile << ":";
		(*sIt)->prettyPrintSummary(logFile, this, false, rv, false,
		    false, std::vector<LoopDetails>());
	    }
	} else {
	    logFile << INPUTNAME << "_" << iIt->progID << ":" << iIt->var.varName 
		    << ":" << iIt->var.type;
	
	    for (std::vector<SymbolicExpr*>::iterator sIt = iIt->sizeExprs.begin();
		    sIt != iIt->sizeExprs.end(); sIt++) {
		logFile << ":";
		(*sIt)->prettyPrintSummary(logFile, this, false, rv, false,
		    false, std::vector<LoopDetails>());
		if (!rv) return;
	    }
	    if (iIt->sizeExprs.size() == 0) {
		for (std::vector<const ArrayType*>::iterator tIt =
			iIt->var.arraySizeInfo.begin(); tIt !=
			iIt->var.arraySizeInfo.end(); tIt++) {
		    logFile << ":";
		    if (isa<ConstantArrayType>(*tIt)) {
			const llvm::APInt size =
			    dyn_cast<ConstantArrayType>(*tIt)->getSize();
			SymbolicIntegerLiteral* sil = new SymbolicIntegerLiteral;
			sil->val = size;
			(sil)->prettyPrintSummary(logFile, this, false, rv, false,
			    false, std::vector<LoopDetails>());
		    } else if (isa<VariableArrayType>(*tIt)) {
		    } else {
			llvm::outs() << "ERROR: Unsupported array type\n";
			rv = false;
			return;
		    }
		}
	    }
	}
	logFile << "\n";
    }

    for (std::vector<DPVar>::iterator dIt = allDPVars.begin(); dIt != 
	    allDPVars.end(); dIt++) {
	logFile << DPARRNAME << "_" << dIt->id << ":" << dIt->dpArray.varName 
		<< ":" << dIt->dpArray.type;
	for (std::vector<SymbolicExpr*>::iterator sIt = dIt->sizeExprs.begin();
		    sIt != dIt->sizeExprs.end(); sIt++) {
	    logFile << ":";
	    (*sIt)->prettyPrintSummary(logFile, this, false, rv, false, false,
		std::vector<LoopDetails>());
	    if (!rv) return;
	}

	if (dIt->sizeExprs.size() == 0) {
	    for (std::vector<const ArrayType*>::iterator tIt =
		    dIt->dpArray.arraySizeInfo.begin(); tIt !=
		    dIt->dpArray.arraySizeInfo.end(); tIt++) {
		logFile << ":";
		if (isa<ConstantArrayType>(*tIt)) {
		    const llvm::APInt size =
			dyn_cast<ConstantArrayType>(*tIt)->getSize();
		    SymbolicIntegerLiteral* sil = new SymbolicIntegerLiteral;
		    sil->val = size;
		    (sil)->prettyPrintSummary(logFile, this, false, rv, false,
			false, std::vector<LoopDetails>());
		} else if (isa<VariableArrayType>(*tIt)) {
		} else {
		    llvm::outs() << "ERROR: Unsupported array type\n";
		    rv = false;
		    return;
		}
	    }
        }
	logFile << "\n";
    }
}

void MainFunction::logSummary(bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering MainFunction::logSummary()\n";
#endif
    rv = true;
    std::ofstream logFile;

    std::string typeLogFileName = progID + ".typemap";
    logFile.open(typeLogFileName.c_str(), std::ios::trunc);
    if (!logFile.is_open()) {
	llvm::outs() << "ERROR: Cannot open logFile " << typeLogFileName << "\n";
	rv = false;
	return;
    }
    writeTypeMap(logFile, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While writing type map\n";
	return;
    }
    logFile.close();

    std::string inputLogFileName = progID + ".input.summary";
    logFile.open(inputLogFileName.c_str(), std::ios::trunc);
    if (!logFile.is_open()) {
	llvm::outs() << "ERROR: Cannot open logFile " << inputLogFileName << "\n";
	rv = false;
	return;
    }
    writeInput(logFile, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While writing input summary\n";
	return;
    }
    logFile.close();

    std::string initLogFileName = progID + ".init.summary";
    logFile.open(initLogFileName.c_str(), std::ios::trunc);
#ifdef DEBUG
    llvm::outs() << "initlogfilename " << initLogFileName << "\n";
#endif
    if (!logFile.is_open()) {
	llvm::outs() << "ERROR: Cannot open logFile " << initLogFileName << "\n";
	rv = false;
	return;
    }
    writeInit(logFile, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While writing init summary\n";
	return;
    }
    logFile.close();

    std::string updateLogFileName = progID + ".update.summary";
    logFile.open(updateLogFileName.c_str(), std::ios::trunc);
    if (!logFile.is_open()) {
	llvm::outs() << "ERROR: Cannot open logFile " << updateLogFileName << "\n";
	rv = false;
	return;
    }
#ifdef DEBUG
    llvm::outs() << "DEBUG: Calling writeUpdate()\n";
#endif
    writeUpdate(logFile, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While writing update summary\n";
	return;
    }
    logFile.close();

    std::string outputLogFileName = progID + ".output.summary";
    logFile.open(outputLogFileName.c_str(), std::ios::trunc);
    if (!logFile.is_open()) {
	llvm::outs() << "ERROR: Cannot open logFile " << outputLogFileName << "\n";
	rv = false;
	return;
    }
#ifdef DEBUG
    llvm::outs() << "DEBUG: Calling writeOutput()\n";
#endif
    writeOutput(logFile, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While writing output summary\n";
	return;
    }
    logFile.close();

    std::string residualLogFileName = progID + ".residual";
    logFile.open(residualLogFileName.c_str(), std::ios::trunc);
    if (!logFile.is_open()) {
	llvm::outs() << "ERROR: Cannot open logFile " << residualLogFileName << "\n";
	rv = false;
	return;
    }
    writeResidual(logFile, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While writing residual\n";
	return;
    }
    logFile.close();

    std::string otherinfoLogFileName = progID + ".info";
    logFile.open(otherinfoLogFileName.c_str(), std::ios::trunc);
    if (!logFile.is_open()) {
	llvm::outs() << "ERROR: Cannot open logFile " << otherinfoLogFileName << "\n";
	rv = false;
	return;
    }
    writeOtherInfo(logFile, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While writing other info\n";
	return;
    }
    logFile.close();
}

void MainFunction::getSizeExprsOfSummaryVars(bool &rv) {
    // Get SymbolicExprVisitor Object of main
    GetSymbolicExprVisitor* mainSym = NULL;
    FunctionAnalysis* mainInfo = NULL;
    bool foundSymVisitor = false;
    for (std::vector<FunctionAnalysis*>::iterator aIt = analysisInfo.begin();
	    aIt != analysisInfo.end(); aIt++) {
	if ((*aIt)->function->equals(main)) {
	    mainSym = (*aIt)->symVisitor;
	    mainInfo = *aIt;
	    foundSymVisitor = true;
	    break;
	}
    }
    if (!foundSymVisitor) {
	llvm::outs() << "ERROR: Cannot find GetsymbolicExprVisitor object of "
		     << "main()\n";
	rv = false;
	return;
    }

    DeclStmtVisitor dVisitor(SM, this, mainSym, customInputFuncs,
	customOutputFuncs);
    dVisitor.setCurrFunction(main);

#if 0
    std::unique_ptr<CFG> mainCFG = main.constructCFG(rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot construct CFG of function:\n";
	main.print();
	return;
    }
#endif
    runVisitorOnInlinedProgram(main->cfg, mainInfo, &dVisitor, rv);
    if (dVisitor.error) {
	llvm::outs() << "ERROR: While running DeclStmtVisitor on inlined main\n";
	rv = false;
	return;
    }
    if (!rv) {
	llvm::outs() << "ERROR: While running DeclStmtVisitor on inlined main\n";
	return;
    }

    for (std::vector<InputVar>::iterator inpIt = inputs.begin(); inpIt != 
	    inputs.end(); inpIt++) {
	bool found = false;
	for (std::vector<InputVar>::iterator iIt =
		dVisitor.inputVarsUpdated.begin(); iIt != 
		dVisitor.inputVarsUpdated.end(); iIt++) {
	    if (inpIt->var.equals(iIt->var)) {
		if (found) {
		    llvm::outs() << "ERROR: Duplicate entries for input var in DeclStmtVisitor\n";
		    rv = false;
		    return;
		}
		inpIt->sizeExprs.clear();
		inpIt->sizeExprs.insert(inpIt->sizeExprs.end(),
		    iIt->sizeExprs.begin(), iIt->sizeExprs.end());
		found = true;
	    }
	}
    }

    for (std::vector<DPVar>::iterator dpIt = allDPVars.begin(); dpIt != 
	    allDPVars.end(); dpIt++) {
	bool found = false;
	for (std::vector<DPVar>::iterator dIt = dVisitor.dpVarsUpdated.begin();
		dIt != dVisitor.dpVarsUpdated.end(); dIt++) {
	    if (dpIt->dpArray.equals(dIt->dpArray)) {
		if (found) {
		    llvm::outs() << "ERROR: Duplicate entries for dp var in DeclStmtVisitor\n";
		    rv = false;
		    return;
		}
		dpIt->sizeExprs.clear();
		dpIt->sizeExprs.insert(dpIt->sizeExprs.end(),
		    dIt->sizeExprs.begin(), dIt->sizeExprs.end());
		found = true;
	    }
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: InputVars\n";
    for (std::vector<InputVar>::iterator inpIt = inputs.begin(); inpIt != 
	    inputs.end(); inpIt++) {
	inpIt->print();
    }
    llvm::outs() << "DEBUG: DPVars\n";
    for (std::vector<DPVar>::iterator dpIt = allDPVars.begin(); dpIt != 
	    allDPVars.end(); dpIt++) {
	dpIt->print();
    }
    llvm::outs() << "\n";
#endif
}

void MainFunction::runVisitorOnInlinedProgram(std::unique_ptr<CFG> &cfg, 
FunctionAnalysis* funcAnalysis, DeclStmtVisitor* dVisitor, bool &rv) {
    rv = true;

    std::stack<unsigned> workList;
    std::set<unsigned> processedBlocks;

    CFGBlock entry = cfg->getEntry();
    unsigned entryBlockID = entry.getBlockID();
    workList.push(entryBlockID);

    while (!(workList.empty())) {

	unsigned currBlockID = workList.top();
	workList.pop();

	// Check if this block is already processed
	if (processedBlocks.find(currBlockID) != processedBlocks.end())
	    continue;
	// Add this block to the set of processed blocks
	processedBlocks.insert(currBlockID);

	dVisitor->setCurrBlock(currBlockID);

	CFGBlock* currBlock = getBlockFromID(cfg, currBlockID);
	if (!currBlock) {
	    llvm::outs() << "ERROR: Cannot obtain CFGBlock* of block " 
			 << currBlockID << "\n";
	    rv = false;
	    return;
	}

	for (CFGBlock::iterator eIt = currBlock->begin(); eIt !=
		currBlock->end(); eIt++) {
	    if (eIt->getKind() == CFGElement::Kind::Statement) {
		CFGStmt currCFGStmt = eIt->castAs<CFGStmt>();
		const Stmt* currStmt = currCFGStmt.getStmt();
		if (!currStmt) {
		    llvm::outs() << "ERROR: Cannot get Stmt* from CFGStmt in "
				 << "block " << currBlockID << "\n";
		    rv = false;
		    return;
		}

		Stmt* S = const_cast<Stmt*>(currStmt);
		//int currLine =
		    //bVisitor->SM->getExpansionLineNumber(S->getLocStart());
#ifdef DEBUG
		llvm::outs() << "DEBUG: Calling DeclStmtVisitor on Stmt: "
			     << Helper::prettyPrintStmt(S) << " in block "
			     << currBlockID << "\n";
#endif

		dVisitor->TraverseStmt(S);
		if (dVisitor->error) {
		    llvm::outs() << "ERROR: While traversing stmt\n";
		    rv = false;
		    return;
		}

	    }
	}

	for (CFGBlock::succ_reverse_iterator sIt = currBlock->succ_rbegin(); 
		sIt != currBlock->succ_rend(); sIt++) {
	    CFGBlock* succBlock = sIt->getReachableBlock();
	    if (processedBlocks.find(succBlock->getBlockID()) == 
		    processedBlocks.end())
		workList.push(succBlock->getBlockID());
	}
    }
}

void MainFunction::removeInitToInputVars(bool &rv) {
    rv = true;

    for (std::vector<BlockDetails*>::iterator bIt = finalBlocks.begin(); bIt != 
	    finalBlocks.end(); bIt++) {
	if (!((*bIt)->isInit())) continue;
	for (std::vector<BlockStmt*>::iterator sIt = (*bIt)->stmts.begin();
		sIt != (*bIt)->stmts.end(); sIt++) {
	    if (!isa<BlockInitStmt>(*sIt)) {
		llvm::outs() << "ERROR: Stmt in Init block is not InitStmt\n";
		rv = false;
		return;
	    }

	    BlockInitStmt* initStmt = dyn_cast<BlockInitStmt>(*sIt);
	    SymbolicArraySubscriptExpr* lhsArray = initStmt->lhsArray;
	    SymbolicDeclRefExpr* baseArray = lhsArray->baseArray;
	    while (baseArray->substituteExpr) {
		if (!isa<SymbolicDeclRefExpr>(baseArray->substituteExpr)) {
		    rv = false;
		    return;
		}
		baseArray =
		    dyn_cast<SymbolicDeclRefExpr>(baseArray->substituteExpr);
	    }

	    // If the base array is not a DP array, put this stmt in residual.
	    bool isDPArray = false;
	    for (std::vector<DPVar>::iterator dpIt = allDPVars.begin(); dpIt != 
		    allDPVars.end(); dpIt++) {
		if (baseArray->var.equals(dpIt->dpArray)) {
		    isDPArray = true;
		    break;
		}
	    }

	    if (!isDPArray) {
		BlockStmt* residualInit = (*sIt)->clone(rv);
		if (!rv) return;
		(*sIt)->toDelete = true;
		residualInits.push_back(residualInit);
	    }
	}

	(*bIt)->stmts.erase(std::remove_if((*bIt)->stmts.begin(),
(*bIt)->stmts.end(), 
	    [](BlockStmt* s) { return s->toDelete; }), (*bIt)->stmts.end());
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: finalBlocks after removeInitToInputVars:\n";
    for (std::vector<BlockDetails*>::iterator bIt = finalBlocks.begin(); bIt 
	    != finalBlocks.end(); bIt++) {
	if (*bIt) {
	    if ((*bIt)->isOutput())
		(*bIt)->print();
	    else
		(*bIt)->prettyPrintHere();
	}
    }
#endif
}

void MainFunction::getSymExprsForInitialExprsOfGlobalVars(bool &rv) {
    rv = true;
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering MainFunction::getSymExprsForInitialExprsOfGlobalVars()\n";
#endif

    for (std::vector<VarDeclDetails*>::iterator vIt = globalVars.begin(); vIt != 
	    globalVars.end(); vIt++) {
	// Skip array vars
	if ((*vIt)->var.isArray()) continue;
	// Look only at those global vars which have initExprs
	if (!((*vIt)->initExpr)) {
	    std::pair<VarDeclDetails*, SymbolicExpr*> p;
	    p.first = *vIt;
	    p.second = NULL;
	    globalVarInitialExprs.push_back(p);
	    continue;
	}
	ExprDetails* initExpr = (*vIt)->initExpr;
	bool foundUnaryExpr = false;
	UnaryOperatorKind opKind;
	if (isa<UnaryOperatorDetails>(initExpr)) {
	    UnaryOperatorDetails* uExpr =
		dyn_cast<UnaryOperatorDetails>(initExpr);
	    if (uExpr->opKind != UnaryOperatorKind::UO_Plus && uExpr->opKind !=
		    UnaryOperatorKind::UO_Minus) {
		llvm::outs() << "ERROR: Initial Expr of global var is an "
			     << "unhandled UnaryOperator:";
		uExpr->opExpr->prettyPrint();
		llvm::outs() << "\n";
		rv = false; 
		return;
	    }
	    foundUnaryExpr = true;
	    opKind = uExpr->opKind;
	    initExpr = uExpr->opExpr;
	}
	SymbolicExpr* initSymExpr = NULL;
	if (isa<IntegerLiteralDetails>(initExpr)) {
	    IntegerLiteralDetails* ild =
		dyn_cast<IntegerLiteralDetails>(initExpr);
	    SymbolicIntegerLiteral* sil = new SymbolicIntegerLiteral;
	    sil->val = ild->val;
	    initSymExpr = sil;
	} else if (isa<FloatingLiteralDetails>(initExpr)) {
	    FloatingLiteralDetails* fld =
		dyn_cast<FloatingLiteralDetails>(initExpr);
	    SymbolicFloatingLiteral* sfl = new SymbolicFloatingLiteral;
	    sfl->val = fld->val;
	    initSymExpr = sfl;
	} else if (isa<CXXBoolLiteralExprDetails>(initExpr)) {
	    CXXBoolLiteralExprDetails* bld = 
		dyn_cast<CXXBoolLiteralExprDetails>(initExpr);
	    SymbolicCXXBoolLiteralExpr* sbl = new SymbolicCXXBoolLiteralExpr;
	    sbl->val = bld->val;
	    initSymExpr = sbl;
	} else if (isa<CharacterLiteralDetails>(initExpr)) {
	    CharacterLiteralDetails* cld =
		dyn_cast<CharacterLiteralDetails>(initExpr);
	    SymbolicCharacterLiteral* scl = new SymbolicCharacterLiteral;
	    scl->val = cld->val;
	    initSymExpr = scl;
	} else if (isa<StringLiteralDetails>(initExpr)) {
	    StringLiteralDetails* sld = 
		dyn_cast<StringLiteralDetails>(initExpr);
	    SymbolicStringLiteral* ssl = new SymbolicStringLiteral;
	    ssl->val = sld->val;
	    initSymExpr = ssl;
	} else {
	    llvm::outs() << "ERROR: Unsupported initialExpr for global var: ";
	    (*vIt)->prettyPrint();
	    rv = false;
	    return;
	}
    	for (std::vector<std::pair<VarDeclDetails*, SymbolicExpr*>
		>::iterator gvIt = globalVarInitialExprs.begin(); gvIt != 
		globalVarInitialExprs.end(); gvIt++) {
	    if (gvIt->first->equals(*vIt)) {
		llvm::outs() << "ERROR: Duplicate entry found for global var: ";
		gvIt->first->prettyPrint();
		rv = false;
		return;
	    }
	}
	if (foundUnaryExpr) {
	    SymbolicUnaryOperator* suo = new SymbolicUnaryOperator;
	    suo->opKind = opKind;
	    suo->opExpr = initSymExpr;
	    initSymExpr = suo;
	}
	std::pair<VarDeclDetails*, SymbolicExpr*> p;
	p.first = *vIt;
	p.second = initSymExpr;
	globalVarInitialExprs.push_back(p);
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: globalVarInitialExprs:\n";
    for (std::vector<std::pair<VarDeclDetails*, SymbolicExpr*> >::iterator gIt =
	    globalVarInitialExprs.begin(); gIt != globalVarInitialExprs.end();
	    gIt++) {
	llvm::outs() << "DEBUG: Global var:\n";
	gIt->first->prettyPrint();
	llvm::outs() << "\n";
	llvm::outs() << "DEBUG: InitialExpr:\n";
	if (gIt->second)
	    gIt->second->prettyPrint(true);
	else 
	    llvm::outs() << "NULL";
	llvm::outs() << "\n";
    }
#endif
}

void MainFunction::writeClusteringFeatures(bool &rv) {
    rv = true;
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering MainFunction::writeClusteringFeatures()\n";
#endif
    std::ofstream logFile;
    std::string featureLogFileName = progID + ".features";
    logFile.open(featureLogFileName.c_str(), std::ios::trunc);
    if (!logFile.is_open()) {
	llvm::outs() << "ERROR: Cannot open logFile " << featureLogFileName << "\n";
	rv = false;
	return;
    }

    if (allDPVars.size() == 0) {
#ifdef DEBUG
	llvm::outs() << "ERROR: No DP Vars\n";
#endif
	rv = false;
	return;
    }

    logFile << "numDPVars: " << allDPVars.size() << "\n";
#ifdef DEBUG
    for (std::vector<DPVar>::iterator iIt = allDPVars.begin(); iIt != 
	    allDPVars.end(); iIt++) {
	llvm::outs() << "dp: " << iIt->dpArray.varName << "\n";
    }
    for (std::vector<InputVar>::iterator iIt = inputs.begin(); iIt != 
	    inputs.end(); iIt++) {
	llvm::outs() << "input: " << iIt->var.varName << "\n";
    }
#endif
    // Check if any of the dp vars is an input var
    bool isInputVarReusedAsDPVar = false;
    for (std::vector<DPVar>::iterator dpIt = allDPVars.begin(); dpIt != 
	    allDPVars.end(); dpIt++) {
	if (isInputVarReusedAsDPVar) break;
	for (std::vector<InputVar>::iterator inpIt = inputs.begin(); inpIt != 
		inputs.end(); inpIt++) {
	    if (dpIt->dpArray.equals(inpIt->var)) {
		isInputVarReusedAsDPVar = true;
#ifdef DEBUG
		llvm::outs() << "DEBUG: inputVar: " << inpIt->var.varName 
			     << " reused as dp array: "  << dpIt->dpArray.varName
			     << "\n";
#endif
		break;
	    }
	}
    }
    logFile << "inp-dp-distinction:" << (isInputVarReusedAsDPVar? "false": "true") << "\n";
    logFile << "type: < ";
    for (std::vector<DPVar>::iterator dpIt = allDPVars.begin(); dpIt !=
	    allDPVars.end(); dpIt++) {
	// A hack not to distinguish int and float arrays
	if (dpIt->dpArray.type == Helper::VarType::FLOATARR)
	    logFile << Helper::VarType::INTARR << " ";
	else
	    logFile << dpIt->dpArray.type << " ";
    }
    logFile << ">\n";
    logFile << "dimension: < ";
    for (std::vector<DPVar>::iterator dpIt = allDPVars.begin(); dpIt != 
	    allDPVars.end(); dpIt++) 
	logFile << dpIt->dpArray.arraySizeInfo.size() << " ";
    logFile << ">\n";
    // get num of update blocks
    int numUpdateBlocks = 0;
    std::stringstream depthStr, directionStr, indicesStr;
    std::vector<std::string> indicesSeen;
    for (std::vector<BlockDetails*>::iterator it = finalBlocks.begin();
	    it != finalBlocks.end(); it++) {
	if ((*it)->isUpdate()) {
	    numUpdateBlocks++;
	    depthStr << (*it)->loops.size() << " ";
	    directionStr << "< ";
	    std::vector<VarDetails> loopIndices;
	    for (std::vector<LoopDetails>::iterator lIt = (*it)->loops.begin();
		    lIt != (*it)->loops.end(); lIt++) {
		directionStr << lIt->loopStep << " ";
		loopIndices.push_back(lIt->loopIndexVar);
	    }
	    directionStr << "> ";
	    for (std::vector<BlockStmt*>::iterator sIt = (*it)->stmts.begin();
		    sIt != (*it)->stmts.end(); sIt++) {
		std::string index;
		BlockUpdateStmt* bus = dyn_cast<BlockUpdateStmt>(*sIt);
		for (std::vector<std::pair<SymbolicArraySubscriptExpr*,
			SymbolicExpr*> >::iterator usIt = bus->updateStmts.begin();
			usIt != bus->updateStmts.end(); usIt++) {
		    index = usIt->first->getArrayIndices((*it)->loops, inputs);
#ifdef DEBUG
		    llvm::outs() << "DEBUG: found index: " << index << "\n";
#endif
		    bool found = false;
		    for (std::vector<std::string>::iterator inIt =
			    indicesSeen.begin(); inIt != indicesSeen.end(); inIt++) {
			if (index.compare(*inIt) == 0) {
			    found = true;
#ifdef DEBUG
			    llvm::outs() << "DEBUG: existing index: " << *inIt
					 << "\n";
#endif
			    break;
			}
		    }
		    if (!found) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: New index: " << index << ": " 
				     << index.length() << "\n";
#endif
			indicesSeen.push_back(index);
			indicesStr << index << " ";
		    }
		}
	    }
	}
    }
    logFile << "loops: " << numUpdateBlocks << "\n";
    logFile << "depth: " << depthStr.str() << "\n";
    logFile << "direction: " << directionStr.str() << "\n";
    logFile << "indices: " << indicesStr.str() << "\n";

    logFile.close();
}

void MainFunction::checkIfArraysAreReadAsScalars(bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering MainFunction::checkIfArraysAreReadAsScalars()\n";
#endif
    rv = true;
    for (std::vector<BlockDetails*>::iterator bIt = combinedBlocks.begin(); bIt
	    != combinedBlocks.end(); bIt++) {
	// Look through blocks with loops
	if ((*bIt)->loops.size() == 0) continue;
	// Go through statements in the body and look for input statement that
	// reads a scalar
	for (std::vector<BlockStmt*>::iterator sIt = (*bIt)->stmts.begin(); 
		sIt != (*bIt)->stmts.end(); sIt++) {
	    if (!(*sIt)->isInput()) continue;
	    // Found input stmt. Look for scalar input
	    if (!isa<BlockScalarInputStmt>(*sIt)) continue;
	    BlockScalarInputStmt* inpStmt = dyn_cast<BlockScalarInputStmt>(*sIt);
	    VarDetails scalarVar = inpStmt->inputVar;
	    if (scalarVar.isArray()) continue;
	    // if substituteExpr of scalar var is array, then skip
	    if (inpStmt->substituteExpr) {
		SymbolicExpr* subExpr = inpStmt->substituteExpr;
		SymbolicDeclRefExpr* lastExpr = NULL;
		while (subExpr && isa<SymbolicDeclRefExpr>(subExpr)) {
		    SymbolicDeclRefExpr* sdre =
			dyn_cast<SymbolicDeclRefExpr>(subExpr);
		    lastExpr = sdre;
		    subExpr = sdre->substituteExpr;
		}

		if (lastExpr) {
		    if (lastExpr->vKind == SymbolicDeclRefExpr::VarKind::FUNC ||
			    lastExpr->vKind == SymbolicDeclRefExpr::VarKind::ARRAY)
			continue;
		    //scalarVar = lastExpr->var;
		}
	    }

	    if (!(*sIt)->func) {
		llvm::outs() << "ERROR: NULL FunctionDetails in BlockStmt\n";
		rv = false;
		return;
	    }

	    // Find reaching defs for this var at the beginning of the innermost
	    // loop
	    FunctionAnalysis* FA = getAnalysisObjOfFunction((*sIt)->func, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While getAnalysisObjOfFunction() of\n";
		(*sIt)->func->print();
		rv = false;
		return;
	    }
	    unsigned innermostLoopHeaderBlock =
		(*bIt)->loops.rbegin()->loopHeaderBlock;
	    int outermostloopStartLine = (*bIt)->loops.begin()->loopStartLine;
	    int outermostloopEndLine = (*bIt)->loops.begin()->loopEndLine;
#ifdef DEBUG
	    llvm::outs() << "DEBUG: outermostloop (" << outermostloopStartLine 
			 << " - " << outermostloopEndLine << ")\n";
#endif
	    if (FA->blockToLines.find(innermostLoopHeaderBlock) ==
		    FA->blockToLines.end()) {
		llvm::outs() << "ERROR: Cannot find entry for block " 
			     << innermostLoopHeaderBlock << " in blockToLines\n";
		rv = false;
		return;
	    }
	    int loopHeaderLine =
		FA->blockToLines[innermostLoopHeaderBlock].first;
	    std::vector<Definition> reachDefsOfScalarInputVar =
		FA->getReachDefsOfVarAtLine(scalarVar, loopHeaderLine,
		innermostLoopHeaderBlock, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While getReachDefsOfVarAtLine()\n";
		return;
	    }

	    // Go through the reachDefs, remove any def that comes through the
	    // test case loop. Check if the def is at a line that is after
	    // outer most loop end line.
	    for (std::vector<Definition>::iterator dIt =
		    reachDefsOfScalarInputVar.begin(); dIt != 
		    reachDefsOfScalarInputVar.end(); dIt++) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: dIt->lineNumber: " << dIt->lineNumber
			     << "\n";
		dIt->print();
		llvm::outs() << "\n";
#endif
		dIt->toDelete = false;
		if (dIt->lineNumber > outermostloopEndLine) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: defintion outside loop\n";
#endif
		    dIt->toDelete = true;
		}
	    }
	    reachDefsOfScalarInputVar.erase(std::remove_if(
		reachDefsOfScalarInputVar.begin(),
		reachDefsOfScalarInputVar.end(),
		[](Definition d) { return d.toDelete; }),
		reachDefsOfScalarInputVar.end());
#ifdef DEBUG
	    llvm::outs() << "DEBUG: After deleting reachDefs outside loop:\n";
	    for (std::vector<Definition>::iterator dIt = 
		    reachDefsOfScalarInputVar.begin(); dIt != 
		    reachDefsOfScalarInputVar.end(); dIt++) {
		dIt->print();
	    }
#endif

	    Definition* declarationDef = NULL;
	    Definition* inputDef = NULL;
	    for (std::vector<Definition>::iterator dIt =
		    reachDefsOfScalarInputVar.begin(); dIt != 
		    reachDefsOfScalarInputVar.end(); dIt++) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: ScalarVar reachDef:\n";
		dIt->print();
		llvm::outs() << "\n";
#endif
		if (declarationDef && inputDef) {
		    llvm::outs() << "ERROR: Scalar var read inside loops, but cannot model it as a scalar var\n";
		    llvm::outs() << "ERROR: declarationDef:\n";
		    declarationDef->print();
		    llvm::outs() << "\nERROR: inputDef:\n";
		    inputDef->print();
		    llvm::outs() << "\n";
		    rv = false;
		    return;
		}
		if (dIt->expressionStr.compare("DP_INPUT") == 0) {
		    if (inputDef) {
			llvm::outs() << "ERROR: Multiple input defintions for var: ";
			scalarVar.print();
			rv = false;
			return;
		    } else {
			inputDef = &(*dIt);
			continue;
		    }
		} else if (dIt->expressionStr.compare("DP_FUNC_PARAM") == 0 || 
			    dIt->expressionStr.compare("DP_GLOBAL_VAR") == 0 ||
			    dIt->expressionStr.compare("DP_UNDEFINED") == 0) {
		    if (declarationDef) {
			llvm::outs() << "ERROR: Multiple declaration defintions for var: ";
			scalarVar.print();
			rv = false;
			return;
		    } else {
			declarationDef = &(*dIt);
			continue;
		    }
		} else {
		    // Check if this def is outside the outermost loop
		    if (dIt->lineNumber < outermostloopStartLine || 
			    outermostloopEndLine < dIt->lineNumber) {
			// In this case, we are good
		    } else {
			llvm::outs() << "ERROR: Extra definitions for scalar var\n";
			rv = false;
			return;
		    }
		}
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: After finding declarationDef and inputDef\n";
#endif

	    // Create an array variable for this var. First, obtain the InputVar
	    // object corresponding to this var.
	    InputVar* origInputVar = NULL;
	    for (std::vector<InputVar>::iterator iIt = inputs.begin(); iIt != 
		    inputs.end(); iIt++) {
		if (scalarVar.equals(iIt->var)) {
		    origInputVar = &(*iIt);
		    break;
		}
	    }
	    if (!origInputVar) {
		llvm::outs() << "ERROR: Cannot find original InputVar for: ";
		scalarVar.print();
		rv = false;
		return;
	    }

	    if (origInputVar->substituteArray.array) {
		llvm::outs() << "ERROR: substituteArray already set for inputvar: ";
		origInputVar->print();
		rv = false;
		return;
	    }

#ifdef DEBUG
	    llvm::outs() << "DEBUG: About to create new array variable\n";
#endif
	    // Now create a new array variable
	    VarDetails arrayVarDetails;
	    std::stringstream ss;
	    ss << "substituteArray" << scalarVar.varName;
	    arrayVarDetails.varName = ss.str();
	    arrayVarDetails.varDeclLine = -1;
	    arrayVarDetails.type = Helper::getArrayVarType(scalarVar.type, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While getArrayVarType()\n";
		return;
	    }
	    SymbolicDeclRefExpr* newArrayVar = new SymbolicDeclRefExpr;
	    newArrayVar->var = arrayVarDetails;
	    newArrayVar->vKind = SymbolicDeclRefExpr::VarKind::ARRAY;
	    SymbolicArraySubscriptExpr* newArrayExpr = new SymbolicArraySubscriptExpr;
	    newArrayExpr->baseArray = newArrayVar;
	    // we got the base of the array, get the index expressions as well
	    // as the size expressions from the loops surrounding it
	    std::vector<std::pair<VarDetails, SymbolicExpr*> > loopIndexVarsTillNow;
	    for (std::vector<LoopDetails>::iterator lIt = (*bIt)->loops.begin();
		    lIt != (*bIt)->loops.end(); lIt++) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Looking at loop:\n";
		lIt->prettyPrintHere();
		llvm::outs() << "\n";
#endif
		// Get loop index var as SymbolicDeclRefExpr
		SymbolicDeclRefExpr* indexExpr = new SymbolicDeclRefExpr;
		indexExpr->var = lIt->loopIndexVar;
		indexExpr->vKind = SymbolicDeclRefExpr::VarKind::LOOPINDEX;
		newArrayExpr->indexExprs.push_back(indexExpr);

#ifdef DEBUG
		llvm::outs() << "DEBUG: After setting indexExpr\n";
#endif

		// Get bounds as sizeExprs for the new array
		if (!lIt->loopIndexInitValSymExpr ||
			!lIt->loopIndexFinalValSymExpr) {
		    llvm::outs() << "ERROR: NULL loopIndexInitValSymExpr/"
				 << "loopIndexFinalValSymExpr\n";
		    rv = false;
		    return;
		}
		SymbolicExpr* sizeExpr = NULL;
		if (lIt->upperBound) {
		    if (lIt->strictBound)
			sizeExpr = lIt->loopIndexFinalValSymExpr;
		    else {
			SymbolicBinaryOperator* sbo = new SymbolicBinaryOperator;
			sbo->lhs = lIt->loopIndexFinalValSymExpr;
			sbo->opKind = BinaryOperatorKind::BO_Add;
			SymbolicIntegerLiteral* sil = new SymbolicIntegerLiteral;
			sil->val = llvm::APInt(32, 1);
			sbo->rhs = sil;
			sizeExpr = sbo;
		    }
		} else {
		    SymbolicBinaryOperator* sbo = new SymbolicBinaryOperator;
		    sbo->lhs = lIt->loopIndexInitValSymExpr;
		    sbo->opKind = BinaryOperatorKind::BO_Add;
		    SymbolicIntegerLiteral* sil = new SymbolicIntegerLiteral;
		    sil->val = llvm::APInt(32, 1);
		    sbo->rhs = sil;
		    sizeExpr = sbo;
		}
#if 0
		// clone the sizeExpr here to store in loopIndexVarsTillNow
		SymbolicExpr* cloneSizeExpr; 
		SymbolicStmt* cs = sizeExpr->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While cloning sizeExpr\n";
		    return;
		}
		if (!isa<SymbolicExpr>(cs)) {
		    llvm::outs() << "ERROR: Clone of SymbolicExpr is not SymbolicExpr\n";
		    rv = false;
		    return;
		}
		cloneSizeExpr = dyn_cast<SymbolicExpr>(cs);
#endif
#ifdef DEBUG
		llvm::outs() << "DEBUG: Found sizeExpr, checking if it is sane\n";
#endif
		bool loopIndexVarPresent = false;
		for (std::vector<std::pair<VarDetails, SymbolicExpr*> >::iterator 
			ltIt = loopIndexVarsTillNow.begin(); ltIt != 
			loopIndexVarsTillNow.end(); ltIt++) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: loopIndexVarsTillNow: " 
				 << ltIt->first.varName << ", bound: ";
		    if (ltIt->second) 
			ltIt->second->prettyPrint(true);
		    else 
			llvm::outs() << "NULL";
		    llvm::outs() << "\n";
#endif
		    bool ret = sizeExpr->containsVar(ltIt->first, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While containsVar()\n";
			return;
		    }
		    if (ret) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: loopIndexVar present in bound\n";
#endif
			if (loopIndexVarPresent) {
			    llvm::outs() << "ERROR: Multiple loop index vars present in loop bounds\n";
			    llvm::outs() << "ERROR: Can't rewrite scalar var as array var\n";
			    rv = false;
			    return;
			}
			loopIndexVarPresent = true;
			// Check if the sizeExpr is itself a loop index var or
			// loop index var + 1
			if (isa<SymbolicDeclRefExpr>(sizeExpr)) {
#ifdef DEBUG
			    llvm::outs() << "DEBUG: sizeExpr is DeclRef\n";
#endif
			    SymbolicDeclRefExpr* sdre = dyn_cast<SymbolicDeclRefExpr>(sizeExpr);
			    if (sdre->vKind ==
				    SymbolicDeclRefExpr::VarKind::LOOPINDEX) {
#ifdef DEBUG
				llvm::outs() << "DEBUG: sizeExpr is loopindex\n";
#endif
				std::vector<VarDetails> vars;
				vars.push_back(ltIt->first);
				std::vector<SymbolicExpr*> exprs;
				exprs.push_back(ltIt->second);
#ifdef DEBUG
				llvm::outs() << "DEBUG: ltIt->second:\n";
				if (ltIt->second) ltIt->second->prettyPrint(true);
				else llvm::outs() << "NULL";
				llvm::outs() << "\n";
#endif
				sizeExpr->replaceVarsWithExprs(vars, exprs, rv);
				if (!rv) {
				    llvm::outs() << "ERROR: While replaceVarsWithExprs() in ";
				    sizeExpr->prettyPrint(true);
				    llvm::outs() << ", replacing var: ";
				    ltIt->first.print();
				    return;
				}
			    } else {
				llvm::outs() << "ERROR: Can't rewrite scalar var as array var\n";
				rv = false;
				return;
			    }
			} else if (isa<SymbolicBinaryOperator>(sizeExpr)) {
#ifdef DEBUG
			    llvm::outs() << "DEBUG: sizeExpr is BinaryOperator\n";
#endif
			    SymbolicBinaryOperator* sbo = 
				dyn_cast<SymbolicBinaryOperator>(sizeExpr);
			    if (!sbo->lhs) {
				llvm::outs() << "ERROR: NULL lhs in BinaryOperator\n";
				rv = false;
				return;
			    }
			    if (!sbo->rhs) {
				llvm::outs() << "ERROR: NULL rhs in BinaryOperator\n";
				rv = false;
				return;
			    }
			    if (isa<SymbolicDeclRefExpr>(sbo->lhs) &&
				    isa<SymbolicIntegerLiteral>(sbo->rhs)) {
#ifdef DEBUG
				llvm::outs() << "DEBUG: size is lhs +/- integer\n";
#endif
				SymbolicDeclRefExpr* sdre =
				    dyn_cast<SymbolicDeclRefExpr>(sbo->lhs);
				if (sdre->vKind ==
					SymbolicDeclRefExpr::VarKind::LOOPINDEX) {
				    std::vector<VarDetails> vars;
				    vars.push_back(ltIt->first);
				    std::vector<SymbolicExpr*> exprs;
				    exprs.push_back(ltIt->second);
				    sizeExpr->replaceVarsWithExprs(vars, exprs, rv);
				    if (!rv) {
					llvm::outs() << "ERROR: While replaceVarsWithExprs() in ";
					sizeExpr->prettyPrint(true);
					llvm::outs() << ", replacing var: ";
					ltIt->first.print();
					return;
				    }
				} else {
				    llvm::outs() << "ERROR: Can't rewrite scalar var as array var\n";
				    rv = false;
				    return;
				}
			    } else {
				llvm::outs() << "ERROR: Can't rewrite scalar var as array var\n";
				rv = false;
				return;
			    }
			} else {
			    llvm::outs() << "ERROR: loop bounds have other loop index vars\n";
			    llvm::outs() << "ERROR: Can't rewrte scalar var as array var\n";
			    rv = false;
			    return;
			}
		    }
		}
		newArrayExpr->baseArray->var.arraySizeSymExprs.push_back(sizeExpr);
		loopIndexVarsTillNow.push_back(
		    std::make_pair(lIt->loopIndexVar, sizeExpr));
#ifdef DEBUG
		llvm::outs() << "DEBUG: End of iteration\n";
#endif
	    }

#ifdef DEBUG
	    llvm::outs() << "DEBUG: After creating new array variable\n";
#endif
	    origInputVar->substituteArray.array = newArrayExpr;
	    for (std::vector<LoopDetails>::iterator lIt = (*bIt)->loops.begin(); 
		    lIt != (*bIt)->loops.end(); lIt++) {
		origInputVar->substituteArray.loops.push_back(&(*lIt));
	    }
	}
    }
}

void MainFunction::checkIfInputArraysAreReadMultipleTimes(bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering MainFunction::checkIfInputArraysAreReadMultipleTimes()\n";
#endif
    rv = true;

    std::vector<SymbolicDeclRefExpr*> arrayVarsAnalyzed;
    for (std::vector<BlockDetails*>::iterator bIt = finalBlocks.begin(); bIt != 
	    finalBlocks.end(); bIt++) {
	bool existingBlock = false;
	for (std::vector<BlockDetails*>::iterator bbIt =
		blocksAfterUnifyingInputs.begin(); bbIt != 
		blocksAfterUnifyingInputs.end(); bbIt++) {
	    if (*bbIt == *bIt) {
		existingBlock = true;
		break;
	    }
	}
	if (existingBlock) continue;
	if (!(*bIt)->isInput()) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Inserting block into blocksAfterUnifyingInputs:\n";
	    (*bIt)->prettyPrintHere();
#endif
	    blocksAfterUnifyingInputs.push_back(*bIt);
	    continue;
	}
	if ((*bIt)->stmts.size() > 1) {
	    llvm::outs() << "ERROR: More than one statement in InputBlock\n";
	    rv = false;
	    return;
	} else if ((*bIt)->stmts.size() == 0) {
	    llvm::outs() << "ERROR: No statements in InputBlock\n";
	    rv = false;
	    return;
	}
	BlockStmt* inputStmt = (*bIt)->stmts[0];
	if (!isa<BlockArrayInputStmt>(inputStmt)) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Inserting block into blocksAfterUnifyingInputs:\n";
	    (*bIt)->prettyPrintHere();
#endif
	    blocksAfterUnifyingInputs.push_back(*bIt);
	    continue;
	}
	BlockArrayInputStmt* arrInputStmt =
	    dyn_cast<BlockArrayInputStmt>(inputStmt);
	std::vector<BlockDetails*> inputBlocksUnderAnalysis;
	inputBlocksUnderAnalysis.push_back(*bIt);
	for (std::vector<BlockDetails*>::iterator b2It = bIt+1; b2It !=
		finalBlocks.end(); b2It++) {
	    if (!(*b2It)->isInput()) continue;
	    if ((*b2It)->stmts.size() == 0 || (*b2It)->stmts.size() > 1) {
		llvm::outs() << "ERROR: Something wrong with stmts in "
			     << "InputBlock\n";
		rv = false;
		return;
	    }
	    inputStmt = (*b2It)->stmts[0];
	    if (!isa<BlockArrayInputStmt>(inputStmt)) continue;
	    BlockArrayInputStmt* arrInputStmt2 = 
		dyn_cast<BlockArrayInputStmt>(inputStmt);
#ifdef DEBUG
	    llvm::outs() << "DEBUG: arrInputStmt:\n";
	    arrInputStmt->prettyPrintHere();
	    llvm::outs() << "\nDEBUG: arrInputStmt2:\n";
	    arrInputStmt2->prettyPrintHere();
	    llvm::outs() << "\n";
#endif
	    if (arrInputStmt->inputArray->baseArray->equals(
		    arrInputStmt2->inputArray->baseArray)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Stmts with same array\n";
#endif
		inputBlocksUnderAnalysis.push_back(*b2It);
	    } else {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Stmts with different array\n";
#endif
	    }
	}

	if (inputBlocksUnderAnalysis.size() == 1) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Inserting block into blocksAfterUnifyingInputs:\n";
	    (*bIt)->prettyPrintHere();
#endif
	    blocksAfterUnifyingInputs.push_back(*bIt);
	    continue;
	}

	if (inputBlocksUnderAnalysis.size() != 2) {
	    llvm::outs() << "ERROR: More than two input blocks with the same array: "
			 << inputBlocksUnderAnalysis.size() << "\n";
	    for (std::vector<BlockDetails*>::iterator b3It =
		    inputBlocksUnderAnalysis.begin(); b3It !=
		    inputBlocksUnderAnalysis.end(); b3It++) {
		(*b3It)->prettyPrintHere();
	    }
	    rv = false;
	    return;
	}

	if (inputBlocksUnderAnalysis[0]->loops.size() != 0 ||
		inputBlocksUnderAnalysis[1]->loops.size() == 0) {
	    llvm::outs() << "ERROR: Can't handle this kind of input block\n"
			 << "Only handling single input followed by inpt with loops\n";
	    rv = false;
	    return;
	}

	BlockArrayInputStmt* arrInputStmt2 =
	    dyn_cast<BlockArrayInputStmt>(inputBlocksUnderAnalysis[1]->stmts[0]);
	// Ensure that the loop indices are the array index expressions
	if (inputBlocksUnderAnalysis[1]->loops.size() !=
		arrInputStmt2->inputArray->indexExprs.size()) {
	    llvm::outs() << "ERROR: Can't handle this kind of input block\n";
	    rv = false;
	    return;
	}
	for (unsigned i = 0; i < inputBlocksUnderAnalysis[1]->loops.size(); i++) {
	    if (!isa<SymbolicDeclRefExpr>(arrInputStmt2->inputArray->indexExprs[i])) {
		llvm::outs() << "ERROR: Array Index is not SymbolicDeclRefExpr\n";
		rv = false;
		return;
	    }
	    SymbolicDeclRefExpr* indexExpr =
		dyn_cast<SymbolicDeclRefExpr>(arrInputStmt2->inputArray->indexExprs[i]);
	    if (indexExpr->vKind != SymbolicDeclRefExpr::VarKind::LOOPINDEX) {
		llvm::outs() << "ERROR: Array Index is not Loop Index\n";
		rv = false;
		return;
	    }
	    if(!indexExpr->var.equals(inputBlocksUnderAnalysis[1]->loops[i].loopIndexVar)) {
		llvm::outs() << "ERROR: Array index is not the corresponding loop index\n";
		llvm::outs() << "indexExpr: ";
		indexExpr->prettyPrint(true);
		llvm::outs() << "\nLoopIndexVar: ";
		inputBlocksUnderAnalysis[1]->loops[i].loopIndexVar.print();
		llvm::outs() << "\n";
		rv = false;
		return;
	    }
	}

	// Now ensure single input stmts' index expressions are less than the
	// initial values of second guy's loops.
	if (arrInputStmt->inputArray->indexExprs.size() !=
		inputBlocksUnderAnalysis[1]->loops.size()) {
	    llvm::outs() << "ERROR: Array index exprs of first input stmt does "
			 << "not match the loop indices of second\n";
	    rv = false;
	    return;
	}
	for (unsigned i = 0; i < inputBlocksUnderAnalysis[1]->loops.size(); i++) {
	    if (isa<SymbolicIntegerLiteral>(arrInputStmt->inputArray->indexExprs[i]) &&
		isa<SymbolicIntegerLiteral>(inputBlocksUnderAnalysis[1]->loops[i].loopIndexInitValSymExpr)) {
		SymbolicIntegerLiteral* first = 
		    dyn_cast<SymbolicIntegerLiteral>(
		    arrInputStmt->inputArray->indexExprs[i]);
		SymbolicIntegerLiteral* second = 
		    dyn_cast<SymbolicIntegerLiteral>(
		    inputBlocksUnderAnalysis[1]->loops[i].loopIndexInitValSymExpr);
		if (inputBlocksUnderAnalysis[1]->loops[i].upperBound) {
		    if (first->val.ult(second->val)) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: Before updating loopInit: ";
			inputBlocksUnderAnalysis[1]->loops[i].loopIndexInitValSymExpr->prettyPrint(true);
#endif
			inputBlocksUnderAnalysis[1]->loops[i].loopIndexInitValSymExpr
			    = first;
#ifdef DEBUG
			llvm::outs() << "DEBUG: Updated loopInit: ";
			inputBlocksUnderAnalysis[1]->loops[i].loopIndexInitValSymExpr->prettyPrint(true);
#endif
		    } else if (first->val == second->val) {
		    } else {
			llvm::outs() << "ERROR: Cannot merge input blocks\n";
			rv = false;
			return;
		    }
		} else {
		    if (second->val.ult(first->val)) {
			inputBlocksUnderAnalysis[1]->loops[i].loopIndexInitValSymExpr
			    = first;
		    } else if (second->val == first->val) {
		    } else {
			llvm::outs() << "ERROR: Cannot merge input blocks\n";
			rv = false;
			return;
		    }
		}
	    } else {
		llvm::outs() << "ERROR: Can't merge blocks because loop init "
			     << "vals are not integers\n";
		rv = false;
		return;
	    }
	}
#ifdef DEBUG
	llvm::outs() << "DEBUG: Inserting block into blocksAfterUnifyingInputs: "
		     << "modified block \n";
	inputBlocksUnderAnalysis[1]->prettyPrintHere();
#endif
	blocksAfterUnifyingInputs.push_back(inputBlocksUnderAnalysis[1]);

	// We need to update the InputVars recorded
	std::vector<InputVar> updatedInputs;
	bool foundOnce = false;
	for (std::vector<InputVar>::iterator iIt = inputs.begin(); iIt != 
		inputs.end(); iIt++) {
	    if (iIt->var.equals(arrInputStmt->inputArray->baseArray->var)) {
		if (foundOnce) continue;
		foundOnce = true;
		updatedInputs.push_back(*iIt);
	    } else {
		updatedInputs.push_back(*iIt);
	    }
	}
	// Remap the ids for inputvars
	int i = inputs.begin()->progID;
	for (std::vector<InputVar>::iterator iIt = updatedInputs.begin(); iIt != 
		updatedInputs.end(); iIt++) {
	    iIt->progID = i;
	    i++;
	}
#ifdef DEBUG
	llvm::outs() << "DEBUG: updatedInputs:\n";
	for (std::vector<InputVar>::iterator iIt = updatedInputs.begin(); iIt != 
		updatedInputs.end(); iIt++) {
	    iIt->print();
	}
#endif
	inputs = updatedInputs;
#ifdef DEBUG
	llvm::outs() << "DEBUG: inputs:\n";
	for (std::vector<InputVar>::iterator iIt = inputs.begin(); iIt != 
		inputs.end(); iIt++) {
	    iIt->print();
	}
#endif

    }

    if (finalBlocks.size() > blocksAfterUnifyingInputs.size()) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: blocksAfterUnifyingInputs:\n";
	for (std::vector<BlockDetails*>::iterator bIt =
	    blocksAfterUnifyingInputs.begin(); bIt != 
	    blocksAfterUnifyingInputs.end(); bIt++) {
	    (*bIt)->prettyPrintHere();
	}
#endif
	finalBlocks = blocksAfterUnifyingInputs;

    } else {
#ifdef DEBUG
	llvm::outs() << "DEBUG: No change in finalBlocks\n";
	for (std::vector<BlockDetails*>::iterator bIt = finalBlocks.begin();
		bIt != finalBlocks.end(); bIt++) {
	    (*bIt)->prettyPrintHere();
	}
#endif
    }
}

void MainFunction::makeDisjointGuards(BlockDetails* block, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering MainFunction::makeDisjointGuards()\n";
    block->prettyPrintHere();
#endif
    rv = true;

    std::vector<BlockUpdateStmt*> stmtsInDisjointSet;
    for (std::vector<BlockStmt*>::iterator sIt = block->stmts.begin(); sIt != 
	    block->stmts.end(); sIt++) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: stmtsInDisjointSet at the start of iteration: "
		     << stmtsInDisjointSet.size() << "\n";
	for (std::vector<BlockUpdateStmt*>::iterator stIt =
		stmtsInDisjointSet.begin(); stIt != stmtsInDisjointSet.end(); stIt++) {
	    (*stIt)->prettyPrintHere();
	    llvm::outs() << "\n";
	}
#endif
	if (!isa<BlockUpdateStmt>(*sIt)) {
	    llvm::outs() << "ERROR: Not update stmt\n";
	    rv = false;
	    return;
	}
	BlockUpdateStmt* stmt = dyn_cast<BlockUpdateStmt>(*sIt);
#ifdef DEBUG
	llvm::outs() << "DEBUG: Looking at stmt:\n";
	(*sIt)->prettyPrintHere();
#endif
	if (stmtsInDisjointSet.size() == 0) {
	    stmtsInDisjointSet.push_back(stmt);
	    continue;
	}

	std::vector<BlockUpdateStmt*> tempDisjointSet;
	for (std::vector<BlockUpdateStmt*>::iterator dIt = stmtsInDisjointSet.begin(); 
		dIt != stmtsInDisjointSet.end(); dIt++) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: dIt:\n";
	    (*dIt)->prettyPrintHere();
	    llvm::outs() << "\n";
#endif
	    z3::context c;
	    z3::expr loopBounds = block->getLoopBoundsAsZ3Expr(&c, this,
		std::vector<LoopDetails>(), false, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While getLoopBoundsAsZ3Expr()\n";
		return;
	    }
	    BlockUpdateStmt* dStmt = *dIt;
	    BlockStmt* cloneStmt = dStmt->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While cloning stmt:\n";
		(*sIt)->prettyPrintHere();
		return;
	    }
	    if (!isa<BlockUpdateStmt>(cloneStmt)) {
		llvm::outs() << "ERROR: Not update stmt\n";
		rv = false;
		return;
	    }
	    BlockUpdateStmt* cloneDStmt =
		dyn_cast<BlockUpdateStmt>(cloneStmt);

	    cloneStmt = stmt->clone(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While cloning stmt:\n";
		(*sIt)->prettyPrintHere();
		return;
	    }
	    if (!isa<BlockUpdateStmt>(cloneStmt)) {
		llvm::outs() << "ERROR: Not update stmt\n";
		rv = false;
		return;
	    }
	    BlockUpdateStmt* cloneUpdateStmt =
		dyn_cast<BlockUpdateStmt>(cloneStmt);

	    // check if stmt in disjoint set(dStmt) can execute together with cloneStmt
	    // get z3::expr for guards in  dStmt
	    z3::expr guardD = dStmt->getZ3ExprForGuard(&c, this, block->loops,
		false, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While getZ3ExprForGuard()\n";
		dStmt->prettyPrintHere();
		return;
	    }

	    // get z3::expr for guards in  cloneStmt
	    z3::expr guardC = cloneUpdateStmt->getZ3ExprForGuard(&c, this,
		block->loops, false, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While getZ3ExprForGuard()\n";
		cloneUpdateStmt->prettyPrintHere();
		return;
	    }

	    // check if guardD and guardC are disjoint
#ifdef DEBUG
	    llvm::outs() << "DEBUG: guardD && guardC\n";
#endif
	    z3::expr disjointCheck = guardD && guardC;
	    z3::expr inputconstraints = getProblemInputConstraints(&c, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While getProblemInputConstraints()\n";
		return;
	    }
	    z3::solver s(c);
	    s.add(inputconstraints);
	    s.add(loopBounds);
	    s.add(disjointCheck);

#ifdef DEBUG
	    llvm::outs() << "DEBUG: guardD && !guardC\n";
#endif
	    z3::solver sa(c);
	    sa.add(inputconstraints);
	    sa.add(loopBounds);
	    sa.add(guardD && !guardC);

#ifdef DEBUG
	    llvm::outs() << "DEBUG: Disjoint check:\n" << s.to_smt2() << "\n";
#endif
	    z3::check_result result = s.check();
	    if (result == z3::unsat) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: guards are disjoint\n";
#endif
		tempDisjointSet.push_back(dStmt);
		continue;
	    } else if (result == z3::sat) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: guards are not disjoint\n";
#endif
		// (1) Add stmt with guard gD and gC
		// Replace array expressions in guard and stmts of cloneStmt
		// with rhs of stmts in dStmt
		std::vector<SymbolicArraySubscriptExpr*> lhsArrays;
		std::vector<SymbolicExpr*> rhsExprs;
		for (std::vector<std::pair<SymbolicArraySubscriptExpr*,
			SymbolicExpr*> >::iterator dsIt =
			dStmt->updateStmts.begin(); dsIt != 
			dStmt->updateStmts.end(); dsIt++) {
		    lhsArrays.push_back(dsIt->first);
		    rhsExprs.push_back(dsIt->second);
		}
		// Replace guard of cloneStmt
		for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
			>::iterator cgIt = cloneUpdateStmt->guards.begin(); cgIt
			!= cloneUpdateStmt->guards.end(); cgIt++) {
		    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator
			    cg1It = cgIt->begin(); cg1It != cgIt->end();
			    cg1It++) {
#ifdef DEBUG
			llvm::outs() << "\nDEBUG: Before replacing dStmtlhs in cloneStmt guard\n";
			cg1It->first->prettyPrint(false);
			llvm::outs() << "\n";
#endif
			cg1It->first->replaceArrayExprWithExpr(lhsArrays,
			    rhsExprs, rv);
			if (!rv) {
			    llvm::outs() << "ERROR: While replaceArrayExprWithExpr\n";
			    return;
			}
#ifdef DEBUG
			llvm::outs() << "\nDEBUG: After replacing dStmtlhs in cloneStmt guard\n";
			cg1It->first->prettyPrint(false);
			llvm::outs() << "\n";
#endif
		    }
		}
		// Replace rhsExprs of cloneStmt
		for (std::vector<std::pair<SymbolicArraySubscriptExpr*,
			SymbolicExpr*> >::iterator usIt =
			cloneUpdateStmt->updateStmts.begin(); usIt != 
			cloneUpdateStmt->updateStmts.end(); usIt++) {
#ifdef DEBUG
		    llvm::outs() << "\nDEBUG: Before replacing dStmt lhs in cloneStmt rhs\n";
		    usIt->second->prettyPrint(false);
		    llvm::outs() << "\n";
#endif
		    usIt->second->replaceArrayExprWithExpr(lhsArrays, rhsExprs,
			rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While replaceArrayExprWithExpr\n";
			return;
		    }
#ifdef DEBUG
		    llvm::outs() << "\nDEBUG: After replacing dStmt lhs in cloneStmt rhs\n";
		    usIt->second->prettyPrint(false);
		    llvm::outs() << "\n";
#endif
		}
#ifdef DEBUG
		llvm::outs() << "DEBUG: About to append guards\n";
		llvm::outs() << "DEBUG: dStmt:\n";
		dStmt->prettyPrintHere();
		llvm::outs() << "DEBUG: cloneUpdateStmt:\n";
		cloneUpdateStmt->prettyPrintHere();
#endif
		// Append guards of dStmt to cloneStmt
		cloneUpdateStmt->guards = appendGuards(cloneUpdateStmt->guards,
		    dStmt->guards, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While appendGuards()\n";
		    return;
		}

#ifdef DEBUG
		llvm::outs() << "DEBUG: check whether lhsarray is written in stmt: \n";
		llvm::outs() << "DEBUG: dStmt:\n";
		dStmt->prettyPrintHere();
		llvm::outs() << "\nDEBUG: cloneUpdateStmt:\n";
		cloneUpdateStmt->prettyPrintHere();
		llvm::outs() << "\n";
#endif

		// Append statements in dStmt to cloneStmt if required
		for (std::vector<std::pair<SymbolicArraySubscriptExpr*,
			SymbolicExpr*> >::iterator dsIt =
			dStmt->updateStmts.begin(); dsIt != 
			dStmt->updateStmts.end(); dsIt++) {
		    SymbolicArraySubscriptExpr* lhsArray = dsIt->first;
		    bool foundLHS = false;
		    for (std::vector<std::pair<SymbolicArraySubscriptExpr*,
			    SymbolicExpr*> >::iterator csIt =
			    cloneUpdateStmt->updateStmts.begin(); csIt != 
			    cloneUpdateStmt->updateStmts.end(); csIt++) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: csIt->first: ";
			csIt->first->prettyPrint(false);
			llvm::outs() << "\nDEBUG: lhsArray: ";
			lhsArray->prettyPrint(false);
			llvm::outs() << "\n";
#endif
			//if (csIt->first->equals(lhsArray)) 
			// equals() does not work. Using strings instead
			std::stringstream ss1;
			csIt->first->toString(ss1, this, false, rv, false, false,
			    block->loops);
			std::stringstream ss2;
			lhsArray->toString(ss2, this, false, rv, false, false,
			    block->loops);
			if (ss1.str().compare(ss2.str()) == 0)
			{
#ifdef DEBUG
			    llvm::outs() << "DEBUG: Equal\n";
#endif
			    foundLHS = true;
			    break;
			} else {
#ifdef DEBUG
			    llvm::outs() << "DEBUG: Not equal\n";
#endif
			}
		    }
		    if (!foundLHS) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: Lhs not found\n";
#endif
			cloneUpdateStmt->updateStmts.push_back(*dsIt);
		    }
		}
		// Check if this guard is satisfiable:
		z3::expr guard1 = cloneUpdateStmt->getZ3ExprForGuard(&c, this, block->loops,
		    false, rv);
		z3::solver sg1(c);
		sg1.add(inputconstraints);
		sg1.add(loopBounds);
		sg1.add(guard1);
		z3::check_result res = sg1.check();
		if (res == z3::sat) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: New disjoint stmt: case 1: guardD && guardC\n";
		    cloneUpdateStmt->prettyPrintHere();
#endif
		    tempDisjointSet.push_back(cloneUpdateStmt);
		} else if (res == z3::unknown) {
		    llvm::outs() << "ERROR: Solver returned unknown\n";
		    llvm::outs() << sg1.to_smt2() << "\n";
		    rv = false;
		    return;
		}

		// (2) Add dStmt with guard gD and not gC. First check if this
		// formula is SAT
		res = sa.check();
		if (res == z3::unsat) continue;
		else if (res == z3::unknown) {
		    llvm::outs() << "ERROR: Solver returned unknown\n";
		    rv = false;
		    return;
		}
		cloneStmt = stmt->clone(rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While cloning stmt\n";
		    return;
		}
		if (!isa<BlockUpdateStmt>(cloneStmt)) {	
		    llvm::outs() << "ERROR: Clone of update stmt is not updatestmt\n";
		    rv = false;
		    return;
		}
		cloneUpdateStmt = dyn_cast<BlockUpdateStmt>(cloneStmt);
		for (std::vector<std::vector<std::pair<SymbolicExpr*, bool> >
			>::iterator cgIt = cloneUpdateStmt->guards.begin(); cgIt
			!= cloneUpdateStmt->guards.end(); cgIt++) {
		    for (std::vector<std::pair<SymbolicExpr*, bool> >::iterator
			    cg1It = cgIt->begin(); cg1It != cgIt->end();
			    cg1It++) {
			cg1It->first->replaceArrayExprWithExpr(lhsArrays,
			    rhsExprs, rv);
			if (!rv) {
			    llvm::outs() << "ERROR: While replaceArrayExprWithExpr\n";
			    return;
			}
		    }
		}
		std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
		negatedGuard = negateGuard(cloneUpdateStmt->guards, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While negatGuard()\n";
		    return;
		}
		cloneDStmt->guards = appendGuards(cloneDStmt->guards,
		    negatedGuard, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While appendGuards()\n";
		    return;
		}

		// Check if this guard is satisfiable:
		z3::expr guard2 = cloneDStmt->getZ3ExprForGuard(&c, this, block->loops,
		    false, rv);
		z3::solver sg2(c);
		sg2.add(inputconstraints);
		sg2.add(loopBounds);
		sg2.add(guard2);
		res = sg2.check();
		if (res == z3::sat) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: New disjoint stmt: case 2: guardD && !guardC\n";
		    cloneDStmt->prettyPrintHere();
#endif
		    tempDisjointSet.push_back(cloneDStmt);
		} else if (res == z3::unknown) {
		    llvm::outs() << "ERROR: Solver returned unknown\n";
		    llvm::outs() << sg2.to_smt2() << "\n";
		    rv = false;
		    return;
		}
	    } else {
		llvm::outs() << "ERROR: Can't check disjointness\n";
		rv = false;
		return;
	    }
	}

	// Create new stmt s with guard = stmt's guard && negation of guards of
	// all stmts in tempDisjointSet. 
	z3::context c;
	z3::expr loopBounds = block->getLoopBoundsAsZ3Expr(&c, this,
	    std::vector<LoopDetails>(), false, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While getLoopBoundsAsZ3Expr()\n";
	    return;
	}
	BlockStmt* cloneStmt = stmt->clone(rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While cloning stmt:\n";
	    (*sIt)->prettyPrintHere();
	    return;
	}
	if (!isa<BlockUpdateStmt>(cloneStmt)) {
	    llvm::outs() << "ERROR: Not update stmt\n";
	    rv = false;
	    return;
	}
	BlockUpdateStmt* cloneUpdateStmt =
	    dyn_cast<BlockUpdateStmt>(cloneStmt);
	z3::expr guardC = cloneUpdateStmt->getZ3ExprForGuard(&c, this,
	    block->loops, false, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While getZ3ExprForGuard()\n";
	    return;
	}

#ifdef DEBUG
	llvm::outs() << "DEBUG: guardC && !guard\n";
#endif
	z3::expr guardCcopy = guardC;
	for (std::vector<BlockUpdateStmt*>::iterator dIt =
		stmtsInDisjointSet.begin(); dIt != stmtsInDisjointSet.end(); dIt++) {
	    guardC = guardC && !((*dIt)->getZ3ExprForGuard(&c, this,
		block->loops, false, rv));
	    if (!rv) {
		llvm::outs() << "ERROR: While getZ3ExprForGuard()\n";
		return;
	    }
	}
	z3::expr inputconstraints = getProblemInputConstraints(&c, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While getProblemInputConstraints()\n";
	    return;
	}
	z3::solver s(c);
	s.add(inputconstraints);
	s.add(loopBounds);
	s.add(guardC);
#ifdef DEBUG
	llvm::outs() << "DEBUG: look at this:\n" << s.to_smt2() << "\n";
#endif
	z3::check_result result = s.check();
	if (result == z3::unknown) {
	    llvm::outs() << "ERROR: Solver returned unknown\n";
	    llvm::outs() << s.to_smt2() << "\n";
	    rv = false;
	    return;
	} else if (result == z3::sat) {
	    // Even if guardC && !all guardD is sat, check if the rewritten
	    // guard is same as the original guard guardC.
	    z3::solver s1(c);
	    s1.add(inputconstraints);
	    s1.add(loopBounds);
	    s1.add((guardC && !guardCcopy) || (!guardC && guardCcopy));
	    result = s1.check();
	    if (result == z3::sat) {
		// Negate and conjunct all guards in disjoint set
		std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
		finalGuards;
		for (std::vector<BlockUpdateStmt*>::iterator dIt = 
			stmtsInDisjointSet.begin(); dIt != stmtsInDisjointSet.end();
			dIt++) {
		    std::vector<std::vector<std::pair<SymbolicExpr*, bool> > >
		    negatedGuard = negateGuard((*dIt)->guards, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While negating guard\n";
			return;
		    }
		    finalGuards = appendGuards(finalGuards, negatedGuard, rv);
		    if (!rv) {
			llvm::outs() << "ERROR: While appendGuards()\n";
			return;
		    }
		}
		finalGuards = appendGuards(finalGuards, cloneUpdateStmt->guards, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While appendGuards()\n";
		    return;
		}
		cloneUpdateStmt->guards = finalGuards;
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: New disjoint stmt: case 3: !guardD && guardC\n";
	    cloneUpdateStmt->prettyPrintHere();
#endif
	    tempDisjointSet.push_back(cloneUpdateStmt);
	}

	// Append negation of stmt's guard with guards of all stmts in
	// tempDisjointSet. Not sure if this needs to be done.

	stmtsInDisjointSet = tempDisjointSet;
#ifdef DEBUG
	llvm::outs() << "DEBUG: stmtsInDisjointSet at the end of iteration: "
		     << stmtsInDisjointSet.size() << "\n";
	for (std::vector<BlockUpdateStmt*>::iterator stIt =
		stmtsInDisjointSet.begin(); stIt != stmtsInDisjointSet.end(); stIt++) {
	    (*stIt)->prettyPrintHere();
	    llvm::outs() << "\n";
	}
#endif
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: stmtsInDisjointSet:\n";
    for (std::vector<BlockUpdateStmt*>::iterator sIt =
	    stmtsInDisjointSet.begin(); sIt != stmtsInDisjointSet.end(); sIt++) {
	(*sIt)->prettyPrintHere();
	llvm::outs() << "\n";
    }
#endif

    // If there are multiple lhs, then we need to emit dummy update for lhs vars
    // that are not updated
    std::vector<SymbolicArraySubscriptExpr*> lhsVarsUpdated;
    for (std::vector<BlockUpdateStmt*>::iterator sIt = 
	    stmtsInDisjointSet.begin(); sIt != stmtsInDisjointSet.end(); sIt++) {
	for (std::vector<std::pair<SymbolicArraySubscriptExpr*, SymbolicExpr*>
		>::iterator uIt = (*sIt)->updateStmts.begin(); uIt != 
		(*sIt)->updateStmts.end(); uIt++) {
	    // If this lhs is not in lhsVarsUpdated, then add it
	    bool present = false;
	    for (std::vector<SymbolicArraySubscriptExpr*>::iterator lIt =
		    lhsVarsUpdated.begin(); lIt != lhsVarsUpdated.end(); lIt++) {
		if (uIt->first->equals(*lIt)) {
		    present = true;
		    break;
		}
	    }
	    if (!present) {
		lhsVarsUpdated.push_back(uIt->first);
	    }
	}
    }

    if (lhsVarsUpdated.size() > 1) {
	for (std::vector<BlockUpdateStmt*>::iterator sIt =
		stmtsInDisjointSet.begin(); sIt != stmtsInDisjointSet.end();
		sIt++) {
	    for (std::vector<SymbolicArraySubscriptExpr*>::iterator lIt =
		    lhsVarsUpdated.begin(); lIt != lhsVarsUpdated.end(); lIt++) {
		bool present = false;
		for (std::vector<std::pair<SymbolicArraySubscriptExpr*,
			SymbolicExpr*> >::iterator uIt =
			(*sIt)->updateStmts.begin(); uIt != 
			(*sIt)->updateStmts.end(); uIt++) {
		    if (uIt->first->equals(*lIt)) {
			present = true;
			break;
		    }
		}
		if (!present) {
		    std::pair<SymbolicArraySubscriptExpr*, SymbolicExpr*> p;
		    p.first = *lIt;
		    p.second = *lIt;
		    (*sIt)->updateStmts.push_back(p);
		}
	    }
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: stmtsInDisjointSet: after emitting dummy updates\n";
    for (std::vector<BlockUpdateStmt*>::iterator sIt =
	    stmtsInDisjointSet.begin(); sIt != stmtsInDisjointSet.end(); sIt++) {
	(*sIt)->prettyPrintHere();
	llvm::outs() << "\n";
    }
#endif

    //block->stmts = stmtsInDisjointSet;
    block->stmts.clear();
    block->stmts.insert(block->stmts.end(), stmtsInDisjointSet.begin(), 
	stmtsInDisjointSet.end());
}

z3::expr MainFunction::getProblemInputConstraints(z3::context* c, bool &rv) {
    rv = true;
    z3::expr constraint(*c);
    if (problem == problemName::MARCHA1) {
	std::stringstream ss;
	ss << INPUTNAME << "_" << 1;
	z3::expr testcase = c->int_const(ss.str().c_str());
	ss.str("");
	ss << INPUTNAME << "_" << 2;
	z3::expr num = c->int_const(ss.str().c_str());
	ss.str("");
	ss << INPUTNAME << "_" << 3;
	z3::expr sum = c->int_const(ss.str().c_str());
	constraint = (1 <= testcase) && (testcase <= 100) && 
	    (0 <= num) && (num <= 20) && (0 <= sum) && (sum <= 20000);
	return constraint;
    } else if (problem == problemName::MGCRNK) {
	std::stringstream ss;
	ss << INPUTNAME << "_" << 1;
	z3::expr testcase = c->int_const(ss.str().c_str());
	ss.str("");
	ss << INPUTNAME << "_" << 2;
	z3::expr num = c->int_const(ss.str().c_str());
	constraint = (1 <= testcase) && (testcase <= 20) && 
	    (2 <= num) && (num <= 100);
	return constraint;
    } else if (problem == problemName::PPTEST) {
	std::stringstream ss;
	ss << INPUTNAME << "_" << 1;
	z3::expr testcase = c->int_const(ss.str().c_str());
	ss.str("");
	ss << INPUTNAME << "_" << 2;
	z3::expr n = c->int_const(ss.str().c_str());
	ss.str("");
	ss << INPUTNAME << "_" << 3;
	z3::expr w = c->int_const(ss.str().c_str());
	constraint = (1 <= testcase) && (testcase <= 100) && 
	    (1 <= n) && (n <= 100) && (1 <= w) && (w <= 100);
	return constraint;
    } else if (problem == problemName::SUMTRIAN) {
	std::stringstream ss;
	ss << INPUTNAME << "_" << 1;
	z3::expr testcase = c->int_const(ss.str().c_str());
	ss.str("");
	ss << INPUTNAME << "_" << 2;
	z3::expr num = c->int_const(ss.str().c_str());
	constraint = (1 <= testcase) && (testcase <= 1000) && 
	    (1 <= num) && (num < 100);
	return constraint;
    } else {
	return constraint;
    }
}

void MainFunction::removeInputsAfterOutput(bool &rv) {
    rv = true;
    // Go through input vars and see if there is a corresponding input block
    std::vector<InputVar> newInputs;
    for (std::vector<InputVar>::iterator iIt = inputs.begin(); iIt !=
	    inputs.end(); iIt++) {
	bool inputFound = false;
	for (std::vector<BlockDetails*>::iterator bIt = finalBlocks.begin(); bIt
		!= finalBlocks.end(); bIt++) {
	    if (!(*bIt)->isInput()) continue;
	    for (std::vector<BlockStmt*>::iterator sIt = (*bIt)->stmts.begin();
		    sIt != (*bIt)->stmts.end(); sIt++) {
		bool ret = (*sIt)->containsVar(iIt->var, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While containsVar() in "
				 << "removeInputsAfterOutput()\n";
		    return;
		}
		if (ret) {
		    inputFound = true;
		    break;
		}
	    }
	    if (inputFound) break;
	}
	if (inputFound) newInputs.push_back(*iIt);
    }
    int i = inputs.begin()->progID;
    for (std::vector<InputVar>::iterator iIt = newInputs.begin(); iIt != 
	    newInputs.end(); iIt++) {
	iIt->progID = i;
	i++;
    }
    inputs = newInputs;
    return;
}

void MainFunction::writeOtherInfo(std::ofstream &logFile, bool &rv) {
    rv = true;
    bool printed = false;
    for (std::vector<BlockDetails*>::iterator bIt = finalBlocks.begin(); bIt != 
	    finalBlocks.end(); bIt++) {
	if (!(*bIt)->isInput()) continue;
	for (std::vector<BlockStmt*>::iterator sIt = (*bIt)->stmts.begin(); sIt
		!= (*bIt)->stmts.end(); sIt++) {
	    if (isa<BlockArrayInputStmt>(*sIt)) {
		BlockArrayInputStmt* arrStmt =
		    dyn_cast<BlockArrayInputStmt>(*sIt);
		// check array indices are same as loop indices
		if (arrStmt->inputArray->indexExprs.size() ==
			(*bIt)->loops.size()) {
		    bool equal = true;
		    for (unsigned i = 0; i < (*bIt)->loops.size(); i++) {
			if (!isa<SymbolicDeclRefExpr>(arrStmt->inputArray->indexExprs[i])
			     || (!(arrStmt->inputArray->indexExprs[i]->containsVar((*bIt)->loops[i].loopIndexVar, rv))))
			{
			    equal = false;
			    break;
			}
		    }
		    if (equal) {
			(*bIt)->loops[0].loopIndexInitValSymExpr->prettyPrintSummary(logFile,
			    this, false, rv, false, false, std::vector<LoopDetails>());
			printed = true;
			break;
		    }
		}
	    }
	}
	if (printed) break;
    }
}
