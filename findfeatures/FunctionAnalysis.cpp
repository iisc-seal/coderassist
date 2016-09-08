#include "FunctionAnalysis.h"
#include "MyASTVisitor.h"
#include "GetSymbolicExprVisitor.h"
#include "MainFunction.h"
#include <queue>
#include "clang/Analysis/AnalysisContext.h"
#include "clang/Analysis/Analyses/Dominators.h"
#include "llvm/Support/GenericDomTree.h"
#include "Helper.h"
#include <sstream>

/*********************Helper Functions***********************/
// Get num of statements in a CFGBlock
int getNumOfStmtsInBlock(CFGBlock* block) {
    int num = 0;

    for (CFGBlock::iterator eIt = block->begin(); eIt != block->end();
	    eIt++) {
	// Look only at those CFGElements that are Statements.
	if (eIt->getKind() == CFGElement::Kind::Statement) {
	    CFGStmt currCFGStmt = eIt->castAs<CFGStmt>();
	    const Stmt* currStmt = currCFGStmt.getStmt();
	    if (!currStmt) {
#ifdef ERRORMSG
		llvm::outs() << "ERROR: Cannot get Stmt from CFGStmt\n";
#endif
		return -1;
	    }
	    num++;
	}
    }

    return num;
}

// Given a src and dest in post-dominator tree, get all the nodes in the path
// from src to dest.
std::vector<unsigned> getNodesInPath(llvm::DominatorTreeBase<CFGBlock>
	*postdTree, CFGBlock* src, CFGBlock* dest) {
    std::vector<unsigned> path;
    std::set<unsigned> markedNodes;
    std::stack<llvm::DomTreeNodeBase<CFGBlock>* > workList;
    DomTreeNodeBase<CFGBlock> *srcNode = postdTree->getNode(src);
    workList.push(srcNode);
#ifdef DEBUG
    llvm::outs() << "DEBUG: Pushed block " << src->getBlockID() << "\n";
#endif

    while(!workList.empty()) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: markedNodes: ";
	for (std::set<unsigned>::iterator it = markedNodes.begin(); it !=
		markedNodes.end(); it++)
	    llvm::outs() << *it << " ";
	llvm::outs() << "\n";
	llvm::outs() << "DEBUG: path: ";
	for (std::vector<unsigned>::iterator it = path.begin(); it !=
		path.end(); it++)
	    llvm::outs() << *it << " ";
	llvm::outs() << "\n";
#endif
	DomTreeNodeBase<CFGBlock> *currNode = workList.top();
	// Get CFG block from dom tree node
	CFGBlock* currBlock = currNode->getBlock();

	// If the currBlock is already marked, then this is our second visit and
	// it needs to be popped.
	if (markedNodes.find(currBlock->getBlockID()) != markedNodes.end()) {
	    workList.pop();
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Popped block " << currBlock->getBlockID() << "\n";
#endif
#ifdef SANITY
	    unsigned pathEnd = path.back();
	    if (pathEnd != currBlock->getBlockID()) {
		llvm::outs() << "ERROR: End of path is " << pathEnd << " and not "
			     << currBlock->getBlockID() << "\n";
		path.clear();
		return path;
	    }
#endif
	    path.pop_back();
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Popped from path\n";
#endif
	} else {
	    // Mark the current node and add it to the current path
	    markedNodes.insert(currBlock->getBlockID());
	    path.push_back(currBlock->getBlockID());
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Marked block " << currBlock->getBlockID() << "\n";
	    llvm::outs() << "DEBUG: Pushed to path\n";
#endif

	    // If this is the dest, then break.
	    if (currBlock->getBlockID() == dest->getBlockID())
		return path;

	    // Add children to workList
	    const std::vector<DomTreeNodeBase<CFGBlock>* > children =
		currNode->getChildren();
	    for (std::vector<DomTreeNodeBase<CFGBlock>* >::const_iterator cIt =
		    children.begin(); cIt != children.end(); cIt++) {
		workList.push(*cIt);
#ifdef DEBUG
		CFGBlock* block = (*cIt)->getBlock();
		llvm::outs() << "DEBUG: Pushed block " << block->getBlockID() << "\n";
#endif
	    }
	}
    }

    return path;
}

Stmt* getStmtAtLineFromCompoundStmt(CompoundStmt* cs, int lineNum, const
SourceManager* SM) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering getStmtAtLineFromCompoundStmt("
		 << lineNum << ")\n";
#endif
    // Need to recursive into nested compound statements to obtain the required
    // stmt
    std::queue<Stmt*> workList;
    workList.push(cs);
#ifdef DEBUG
    llvm::outs() << "DEBUG: Initial compound stmt: (" 
		 << SM->getExpansionLineNumber(cs->getLocStart()) << " - " 
		 << SM->getExpansionLineNumber(cs->getLocEnd()) << ")\n";
#endif
    while (!workList.empty()) {
	Stmt* s = workList.front();
	workList.pop();

	if (s == NULL) {
	    llvm::outs() << "WARNING: Found NULL statement in workList\n";
	    continue;
	}

	int currLine = SM->getExpansionLineNumber(s->getLocStart());
#ifdef DEBUG
	llvm::outs() << "DEBUG: Stmt at line " << currLine << "\n";
#endif
	if (lineNum == currLine)
	    return s;

	if (isa<CompoundStmt>(s)) {
	    CompoundStmt* cst = dyn_cast<CompoundStmt>(s);
	    for (CompoundStmt::body_iterator bIt = cst->body_begin(); bIt !=
		    cst->body_end(); bIt++) {
		if (*bIt == NULL) {
		    llvm::outs() << "WARNING: Adding NULL statement while scanning CompoundStmt\n";
		}
		workList.push(*bIt);
	    }
	} else if (isa<IfStmt>(s)) {
	    IfStmt* ist = dyn_cast<IfStmt>(s);
	    workList.push(ist->getThen());
	    workList.push(ist->getElse());
	} else if (isa<ForStmt>(s)) {
	    ForStmt* fst = dyn_cast<ForStmt>(s);
	    workList.push(fst->getBody());
	} else if (isa<WhileStmt>(s)) {
	    WhileStmt* wst = dyn_cast<WhileStmt>(s);
	    workList.push(wst->getBody());
	}
    }

    return NULL;
}

bool isVarDefPresent(std::pair<VarDetails, int> p,
std::vector<std::pair<VarDetails, int> > vec) {
    for (std::vector<std::pair<VarDetails, int> >::iterator it =
	    vec.begin(); it != vec.end(); it++) {
	if (it->second != p.second) continue;

	if (it->first.equals(p.first))
	    return true;
    }

    return false;
}

bool isVarUsePresent(VarDetails v, std::vector<VarDetails> vec) {
    for (std::vector<VarDetails>::iterator it = vec.begin(); it != 
	    vec.end(); it++) {
	if (it->equals(v))
	    return true;
    }
    return false;
}

bool isDefPresentInVector(Definition d, std::vector<Definition> vec) {
    for (std::vector<Definition>::iterator it = vec.begin(); it != vec.end();
	    it++) {
	if (it->equals(d)) return true;
    }
    return false;
}

bool isDefPresentInSet(Definition d, std::set<Definition> s) {
    for (std::set<Definition>::iterator it = s.begin(); it != s.end();
	    it++) {
	//if (it->equals(d)) return true;
	// Need to do this because elements in a set are const by default.
	Definition itd = static_cast<Definition>(*it);
	if (itd.equals(d)) return true;
    }
    return false;
}
/************************************************************/

FunctionAnalysis::FunctionAnalysis(const SourceManager* M, ASTContext* C, MainFunction* main) {
    error = false;
    SM = M;
    mainObj = main;
    Context = C;
}

void FunctionAnalysis::analyzeFunc(bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Analyzing function: " << function->funcName << "\n";
#endif
    rv = true;
    if (function->isCustomInputFunction || function->isCustomOutputFunction) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found custom input function. Skipping...\n";
#endif
	return;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: InputVars:\n";
    for (std::vector<InputVar>::iterator it = inputVars.begin(); it !=
	    inputVars.end(); it++)
	it->print();
    llvm::outs() << "DEBUG: InputLines:\n";
    for (std::vector<int>::iterator it = inputLines.begin(); it !=
	    inputLines.end(); it++)
	llvm::outs() << *it << " ";
    llvm::outs() << "\n";
#endif

#if 0
    clang::LangOptions lo;
    lo.CPlusPlus = true;
    llvm::outs() << "DEBUG: printing CFG:\n";
    function->cfg->print(llvm::outs(), lo, false);
#endif

    populateBasicBlockDetails(function->cfg);
    getBlockDetails(function->cfg, rv);
    if (!rv)
	return;
    computeControlDependences(function->cfg, function->fd, rv);
    if (!rv)
	return;
    computeBlocksWithSameTerminatorExpr(rv);
    if (!rv)
	return;

    Stmt* funcBody = function->fd->getBody();
    // Find the parameters of the function. Do not do this if it is main.
    if (!(function->fd->isMain())) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Function is not main. Finding parameters\n";
#endif
	unsigned numParams = function->fd->param_size();
	//std::vector<std::string> parameters;
	for (unsigned i = 0; i < numParams; i++) {
	    const ParmVarDecl* paramVD =
		function->fd->getParamDecl(i);
	    if (!paramVD) {
		llvm::outs() << "ERROR: Cannot get ParamVarDecl of parameter " << i
			     << "of function " << function->funcName << "\n";
		rv = false;
		return;
	    }
	    VarDetails paramDetails = Helper::getVarDetailsFromDecl(SM, 
		paramVD, rv);
	    if (!rv)
		return;
	    paramDetails.kind = VarDetails::PARAMETER;
	    //parameters.push_back(paramVD->getQualifiedNameAsString());
	    parameters.push_back(paramDetails);
	}

#ifdef DEBUG
	llvm::outs() << "DEBUG: Found Parameters:\n";
	for (std::vector<VarDetails>::iterator pIt = parameters.begin(); pIt != 
		parameters.end(); pIt++) {
	    pIt->print();
	    llvm::outs() << "\n";
	}
#endif
    } else {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Function is main. Not finding parameters\n";
#endif
    }

    // Recomputing CFG to avoid an error. This most probably is because
    // computeControlDependences modifies the CFG (adds a dummy START node).
    CFG::BuildOptions bo;
    CFG::BuildOptions fbo = bo.setAllAlwaysAdd();
    function->cfg = CFG::buildCFG(function->fd, funcBody, Context, fbo);
    //function.cfg = funcCFG;
    function->funcBody = funcBody;
    function->C = Context;

    findBackEdgesInCFG(function->cfg, rv);
    if (!rv)
	return;
    populateReachingDefinitions(rv);
    if (!rv)
	return;
    computeAllLoops(function->cfg, rv);
    if (!rv) {
	rv = false;
	llvm::outs() << "ERROR: While computeAllLoops()\n";
	return;
    } else {
#ifdef DEBUG
	llvm::outs() << "DEBUG: rv is true\n";
#endif
    }
#ifdef DEBUG
    llvm::outs() << "DEBUG: After computeAllLoops() in analyzeFunc()\n";
#endif

    computeTransitiveControlDependences(rv);
    if (!rv)
	return;

    if (!isa<CompoundStmt>(funcBody)) {
	llvm::outs() << "ERROR: Function body is not a CompoundStmt\n";
	rv = false;
	return;
    }
}

void FunctionAnalysis::backSubstitution(bool &rv) {
    rv = true;

    if (function->isCustomInputFunction || function->isCustomOutputFunction) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Found custom input function. Skipping...\n";
#endif
	return;
    }

    getSpecialExprs(rv);
    if (!rv) {
	llvm::outs() << "ERROR: While getting special expr\n";
	return;
    }

    Stmt* funcBody = function->fd->getBody();
    if (!isa<CompoundStmt>(funcBody)) {
	llvm::outs() << "ERROR: Function body is not a CompoundStmt\n";
	rv = false;
	return;
    }

    CompoundStmt* body = dyn_cast<CompoundStmt>(funcBody);
#if 0
    std::unique_ptr<CFG> funcCFG = function.constructCFG(rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot construct CFG of function:\n";
	function.print();
	return;
    }
#endif
    computeSymbolicExprs(function->cfg, body, rv);
    if (!rv) {
	llvm::outs() << "ERROR: While computeSymbolicExprs()\n";
	return;
    }

    if (symVisitor->residualStmts.size() != 0) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Residual found: " << symVisitor->residualStmts.size() << "\n";
#endif
	for (std::vector<StmtDetails*>::iterator sIt =
		symVisitor->residualStmts.begin(); sIt != 
		symVisitor->residualStmts.end(); sIt++) {
	    if (*sIt) {
		residualStmts.push_back(*sIt);
	    } else {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Found a NULL resiudal stmt\n";
#endif
	    }
	}
	//residualStmts.insert(residualStmts.end(), symVisitor->residualStmts.begin(), residualStmts.end());
    } else {
#ifdef DEBUG
	llvm::outs() << "DEBUG: No residual found\n";
#endif
    }

}

void FunctionAnalysis::populateReachingDefinitions(bool &rv) {

#ifdef DEBUG
    llvm::outs() << "DEBUG: Calling populateReachingDefinitions on " 
		 << function->funcName << "(" << function->funcStartLine 
		 << " - " << function->funcEndLine << ")\n";
    //llvm::outs() << "DEBUG: CFG:\n";
    //clang::LangOptions lo;
    //lo.CPlusPlus = true;
    //function.cfg->print(llvm::outs(), lo, false);
#endif

    rv = true;

    //SourceManager &SM = Result.Context->getSourceManager();
    //CFG* cfg = function.cfg;

    // Find the return type of function. Used later
    const Type* t = function->fd->getReturnType().split().Ty;
    if (!t->isVoidType()) {
	std::pair<Helper::VarType, std::vector<const ArrayType*> > retType =
	    Helper::getVarType(t, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: Cannot obtain return type of function " 
			 << function->funcName << "\n";
	    return;
	}
	returnVar = VarDetails::createReturnVar(retType.first, retType.second);
    }


    // For pretty printing the CFG
    LangOptions LangOpts;
    LangOpts.CPlusPlus = true;

    // workList holding basic block ids
    std::queue<unsigned> workList;

    CFGBlock entry = function->cfg->getEntry();

    // Add dummy definitions for function parameters at the entry block
    std::map<std::string, Definition> entryGenSet;
    std::map<std::string, std::vector<Definition> > entryAllDefs;
    for (std::vector<VarDetails>::iterator pIt = parameters.begin(); pIt !=
	    parameters.end(); pIt++) {
	Definition def;
	def.lineNumber = -1;
	def.blockID = entry.getBlockID();
	def.expression = NULL;
	def.expressionStr = "DP_FUNC_PARAM";

	std::string paramVarID = pIt->varID;
	if (entryGenSet.find(paramVarID) == entryGenSet.end())
	    entryGenSet[paramVarID] = def;
	else {
	    llvm::outs() << "ERROR: entryGenSet already contains entry for var ";
	    pIt->print();
	    llvm::outs() << "\n";
	    rv = false;
	    return;
	}

	if (entryAllDefs.find(paramVarID) == entryAllDefs.end()) {
	    std::vector<Definition> allDefs;
	    allDefs.push_back(def);
	    entryAllDefs[paramVarID] = allDefs;
	} else {
	    llvm::outs() << "ERROR: entryAllDefs already contains entry for var ";
	    pIt->print();
	    llvm::outs() << "\n";
	    rv = false;
	    return;
	}
    }

    // Add dummy definitions for global vars in entryGenSet.
    for (std::vector<VarDeclDetails*>::iterator gvIt =
	    mainObj->globalVars.begin(); gvIt != mainObj->globalVars.end();
	    gvIt++) {
	if ((*gvIt)->var.isArray()) continue;
	Definition def;
	def.lineNumber = (*gvIt)->lineNum;
	def.blockID = entry.getBlockID();
	def.expression = NULL;
	def.expressionStr = "DP_GLOBAL_VAR";

	std::string globalVarID = (*gvIt)->var.varID;
	if (entryGenSet.find(globalVarID) == entryGenSet.end())
	    entryGenSet[globalVarID] = def;
	else {
	    llvm::outs() << "ERROR: entryGenSet already contains entry for var ";
	    (*gvIt)->prettyPrint();
	    llvm::outs() << "\n";
	    rv = false;
	    return;
	}

	if (entryAllDefs.find(globalVarID) == entryAllDefs.end()) {
	    std::vector<Definition> allDefs;
	    allDefs.push_back(def);
	    entryAllDefs[globalVarID] = allDefs;
	} else {
	    llvm::outs() << "ERROR: entryAllDefs already contains entry for var ";
	    (*gvIt)->prettyPrint();
	    llvm::outs() << "\n";
	    rv = false;
	    return;
	}
    }

    unsigned entryBlockID = entry.getBlockID();
    if (rd.genSet.find(entryBlockID) == rd.genSet.end())
	rd.genSet[entryBlockID] = entryGenSet;
    else {
#ifdef DEBUG
	llvm::outs() << "DEBUG: There already exists an entry for block " 
		     << entryBlockID << " in genSet\n";
	for (std::map<std::string, Definition>::iterator mIt =
		rd.genSet[entryBlockID].begin(); mIt != 
		rd.genSet[entryBlockID].end(); mIt++) {
	    llvm::outs() << "DEBUG: VarID: " << mIt->first << "\n";
	    llvm::outs() << "DEBUG: Def: (" << mIt->second.lineNumber << ", "
			 << Helper::prettyPrintExpr(mIt->second.expression) 
			 << ")\n";
	}
#endif
    }
    if (rd.allDefsInBlock.find(entryBlockID) == rd.allDefsInBlock.end())
	rd.allDefsInBlock[entryBlockID] = entryAllDefs;

    workList.push(entryBlockID);

    // Visitor to obtain all vars referenced in a Stmt/Expr
    GetVarVisitor gv(SM);
    gv.clear();

    // Keep track of blocks already processed so that we do not add same blocks
    // back into the workList
    std::set<unsigned> processedBlocks;
    while (true) {
	if (workList.empty())
	    break;

	CFGBlock* currBlock;
	unsigned currBlockID = workList.front();
	workList.pop();

	// Add this block to set of processedBlocks
	processedBlocks.insert(currBlockID);

	// get basic block from id
	currBlock = getBlockFromID(function->cfg, currBlockID);
	if (!currBlock) {
	    llvm::outs() << "ERROR: Could not find the block with ID " 
			 << currBlockID << "\n";
	    rv = false;
	    return;
	}

#ifdef DEBUG
	llvm::outs() << "DEBUG: Looking at basic block " << currBlockID << " (" 
		     << currBlock->size() << ")\n";
	//currBlock->dump();
#endif

	std::map<std::string, Definition> currGenSet;
	std::map<std::string, std::vector<Definition> > blockDefs;

	// Why is this code here?
	// If the genSet of current block is already present, retrieve it.
	if (rd.genSet.find(currBlockID) != rd.genSet.end())
	    currGenSet = rd.genSet[currBlockID];

	// Record all vars defined in the current block
	std::vector<std::pair<VarDetails, int> > varsDefined;
	std::map<int, std::vector<VarDetails> > varsUsedInBlock;

	// compute gen set of current block
	int i = 0;
	for (CFGBlock::iterator eIt = currBlock->begin(); eIt != currBlock->end();
		eIt++) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: CFElement " << ++i << "\n";
#endif
	    // Look only at those CFGElements that are Statements.
	    if (eIt->getKind() == CFGElement::Kind::Statement) {
		CFGStmt currCFGStmt = eIt->castAs<CFGStmt>();
		const Stmt* currStmt = currCFGStmt.getStmt();
		if (!currStmt) {
		    llvm::outs() << "ERROR: Cannot get Stmt from CFGStmt\n";
		    rv = false;
		    return;
		}
#ifdef DEBUG
		llvm::outs() << "DEBUG: currStmt: " 
			     << Helper::prettyPrintStmt(currStmt) << "\n";
#endif

		// If we encounter an input call stmt, we might have more than
		// one vars being read in the same stmt and each of these
		// instances create a new def. And hence the vector.
		//std::vector<std::pair<std::string, std::string> > defsFound;
		// We need to record the rhs expression for assignment
		// statements. But in case of input var, the rhs expression is a
		// custom string like "DP_INPUT"
		std::vector<std::pair<VarDetails, std::pair<const Expr*, std::string> 
		    > > defsFound;
		int line = SM->getExpansionLineNumber(currStmt->getLocStart(), 0);
		bool foundDef = false;
		if (isa<BinaryOperator>(currStmt)) { // Found an assignment statement
		    const BinaryOperator* currStmtAsBO =
			dyn_cast<BinaryOperator>(currStmt);
		    if (currStmtAsBO->isAssignmentOp()) {
			const Expr* stmtLHSExpr = currStmtAsBO->getLHS();
			if (!stmtLHSExpr) {
			    llvm::outs() << "ERROR: Cannot get LHS Expr from Assignment\n";
			    rv = false;
			    return;
			}
			stmtLHSExpr = stmtLHSExpr->IgnoreParenCasts();

			// An assignment statement could also be an input
			// statement that uses custom input function on the RHS.
			// Check for input statement
			if (std::find(inputLines.begin(), inputLines.end(), line)
				!= inputLines.end()) {
			    for (std::vector<InputVar>::iterator it = inputVars.begin(); 
				    it != inputVars.end(); it++) {
				if (it->inputCallLine != line) continue;
				//std::pair<std::string, std::string> def;
				std::pair<VarDetails, std::pair<const Expr*, std::string> >
				    def;
				def.first = it->var;
				def.second.first = NULL;
				def.second.second = "DP_INPUT";
				defsFound.push_back(def);
				foundDef = true;
			    }
			} else if (isa<DeclRefExpr>(stmtLHSExpr)) {
			    //std::pair<std::string, std::string> def;
			    std::pair<VarDetails, std::pair<const Expr*, std::string> >
				def;
			    def.first =
				Helper::getVarDetailsFromExpr(SM, stmtLHSExpr, rv);
			    if (!rv)
				return;
			    def.second.first = currStmtAsBO->getRHS();
			    def.second.second =
				Helper::prettyPrintExpr(def.second.first);
			    defsFound.push_back(def);
			    foundDef = true;

			    // Record vars used in the RHS. This can be done
			    // only if the statement is not an input statement.
			    gv.clear();
			    gv.TraverseStmt(currStmtAsBO->getRHS());
			    if (gv.error) {
				rv = false;
				return;
			    }
			    for (std::vector<struct GetVarVisitor::varOccurrence>::iterator 
				    vIt = gv.scalarVarsFound.begin(); vIt != 
				    gv.scalarVarsFound.end(); vIt++) {
				if (varsUsedInBlock.find(line) ==
					varsUsedInBlock.end()) {
				    std::vector<VarDetails> varsUsed;
				    varsUsed.push_back(vIt->vd.clone());
				    varsUsedInBlock[line] = varsUsed;
				} else {
				    if (!isVarUsePresent(vIt->vd,
					    varsUsedInBlock[line])) {
					varsUsedInBlock[line].push_back(vIt->vd.clone());
				    }
				}
			    }
			}

			// Record var defined.
			std::pair<VarDetails, int> p;
			VarDetails var =
			    Helper::getVarDetailsFromExpr(SM, stmtLHSExpr, rv);
			if (!rv)
			    return;
			p.first = var;
			p.second = line;
			// Check if this is already present in varsDefined
			if (!isVarDefPresent(p, varsDefined))
			    varsDefined.push_back(p);
		    }
		} else if (isa<UnaryOperator>(currStmt)) { // Found increment/decrement operator
#ifdef DEBUG
		    llvm::outs() << "DEBUG: currStmt is UnaryOperator\n";
#endif
		    const UnaryOperator* currStmtAsUO = 
			dyn_cast<UnaryOperator>(currStmt);
		    if (currStmtAsUO->isIncrementDecrementOp()) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: currStmt is increment/decrement\n";
#endif
			Expr* subExpr = currStmtAsUO->getSubExpr();
			if (!subExpr) {
			    llvm::outs() << "ERROR: Cannot get subexpr from UnaryOperator\n";
			    rv = false;
			    return;
			}
			subExpr = subExpr->IgnoreParenCasts();

			if (isa<DeclRefExpr>(subExpr)) {
#ifdef DEBUG
			    llvm::outs() << "DEBUG: UnaryOperand is DeclRefExpr\n";
#endif
			    //std::pair<std::string, std::string> def;
			    std::pair<VarDetails, std::pair<const Expr*, std::string> >
				def;
			    def.first = Helper::getVarDetailsFromExpr(SM,
				subExpr, rv);
			    if (!rv)
				return;
			    foundDef = true;
			    if (currStmtAsUO->isIncrementOp()) {
				def.second.first = currStmtAsUO;
				def.second.second = "++";
			    } else if (currStmtAsUO->isDecrementOp()) {
				def.second.first = currStmtAsUO;
				def.second.second = "--";
			    } else
				foundDef = false;
			    if (foundDef) defsFound.push_back(def);

			    // Record vars used in the RHS. This is the same var
			    // that is defined
			    if (varsUsedInBlock.find(line) ==
				    varsUsedInBlock.end()) {
				std::vector<VarDetails> varsUsed;
				varsUsed.push_back(def.first.clone());
				varsUsedInBlock[line] = varsUsed;
			    } else {
				if (!isVarUsePresent(def.first,
					varsUsedInBlock[line])) {
				    varsUsedInBlock[line].push_back(def.first.clone());
				}
			    }
			}

			// Record var defined.
			std::pair<VarDetails, int> p;
			VarDetails var =
			    Helper::getVarDetailsFromExpr(SM, subExpr, rv);
			if (!rv)
			    return;
			p.first = var;
			p.second = line;
			// Check if this is already present in varsDefined
			if (!isVarDefPresent(p, varsDefined)) {
			    varsDefined.push_back(p);
#ifdef DEBUG
			    llvm::outs() << "DEBUG: Adding var: ";
			    var.print();
			    llvm::outs() << " to varsDefined at line " << line
					 << "\n";
#endif
			}
		    }
		} else if (std::find(inputLines.begin(), inputLines.end(), line)
			    != inputLines.end()) {
		    for (std::vector<InputVar>::iterator it = inputVars.begin(); 
			    it != inputVars.end(); it++) {
			if (it->inputCallLine != line)	continue;
			// If the input var is an array, it need not be present
			// in the genSet. TODO: Not sure if this is correct
			if (it->var.arraySizeInfo.size() != 0) continue;

			std::pair<VarDetails, std::pair<const Expr*, std::string> >
			    def;
			def.first = it->var;
			def.second.first = NULL;
			def.second.second = "DP_INPUT";
			defsFound.push_back(def);
			foundDef = true;

			// Record var read.
			std::pair<VarDetails, int> p;
			p.first = it->var.clone();
			p.second = line;
			// Check if this is already present in varsDefined
			if (!isVarDefPresent(p, varsDefined))
			    varsDefined.push_back(p);
		    }
		} else if (isa<DeclStmt>(currStmt)) { // Found var decl
		    const DeclStmt* currStmtAsDeclStmt =
			dyn_cast<DeclStmt>(currStmt);
		    if (currStmtAsDeclStmt->isSingleDecl()) {
			const Decl* currDecl =
			    currStmtAsDeclStmt->getSingleDecl();
			if (isa<VarDecl>(currDecl)) {
			    const VarDecl* currVarDecl =
				dyn_cast<VarDecl>(currDecl);
			    const Type* currVarType = 
				currVarDecl->getType().split().Ty;
			    if (currVarType->isScalarType()) {
				//std::pair<std::string, std::string> def;
				std::pair<VarDetails, std::pair<const Expr*,
				    std::string> > def;
				def.first = Helper::getVarDetailsFromDecl(SM,
				    currVarDecl, rv);
				if (!rv)
				    return;
				if (currVarDecl->hasInit()) {
				    def.second.first = currVarDecl->getInit();
				    def.second.second =
					Helper::prettyPrintExpr(def.second.first);

				    // Record vars used in the init
				    gv.clear();
				    gv.TraverseStmt(const_cast<Expr*>(currVarDecl->getInit()));
				    if (gv.error) {
					rv = false;
					return;
				    }
				    for (std::vector<struct GetVarVisitor::varOccurrence>::iterator 
					    vIt = gv.scalarVarsFound.begin(); vIt != 
					    gv.scalarVarsFound.end(); vIt++) {
					if (varsUsedInBlock.find(line) ==
						varsUsedInBlock.end()) {
					    std::vector<VarDetails> varsUsed;
					    varsUsed.push_back(vIt->vd.clone());
					    varsUsedInBlock[line] = varsUsed;
					} else {
					    if (!isVarUsePresent(vIt->vd,
						    varsUsedInBlock[line])) {
						varsUsedInBlock[line].push_back(vIt->vd.clone());
					    }
					}
				    }
				} else {
				    def.second.first = NULL;
				    def.second.second = "DP_UNDEFINED";
				}
				defsFound.push_back(def);
				foundDef = true;
			    }
			}

			// Record var declared;
			std::pair<VarDetails, int> p;
			p.first = Helper::getVarDetailsFromDecl(SM, currDecl, rv);
			if (!rv)
			    return;
			p.second = line;
			// Check if this is already present in varsDefined
			if (!isVarDefPresent(p, varsDefined))
			    varsDefined.push_back(p);
		    } else {
			llvm::outs() << "ERROR: Found multiple Decl\n";
			rv = false;
			return;
		    }
		} else if (isa<ReturnStmt>(currStmt)) { // Found return stmt
		    const ReturnStmt* currStmtAsReturnStmt = 
			dyn_cast<ReturnStmt>(currStmt);
		    const Expr* retExpr = currStmtAsReturnStmt->getRetValue();
		    //std::pair<std::string, std::string> def;
		    std::pair<VarDetails, std::pair<const Expr*, std::string> > def;
#if 0
		    def.first = VarDetails::createReturnVar(retType.first,
			retType.second);
#endif
		    def.first = returnVar;
		    def.second.first = retExpr;
		    if (retExpr)
			def.second.second = Helper::prettyPrintExpr(retExpr);
		    defsFound.push_back(def);
		    foundDef = true;
		} else if (isa<CallExpr>(currStmt)) { // Found call expr
		    // If it is a user-defined function, then we generate dummy
		    // defs for all global vars.
		    const CallExpr* callExprStmt = dyn_cast<CallExpr>(currStmt);
		    const FunctionDecl* calleeFD = callExprStmt->getDirectCallee();
		    if (!calleeFD) {
			llvm::outs() << "ERROR: Cannot get FunctionDecl of callee from "
				     << Helper::prettyPrintExpr(callExprStmt) 
				     << "\n";
			rv = false;
			return;
		    }
		    calleeFD = calleeFD->getMostRecentDecl();
		    if (!calleeFD) {
			llvm::outs() << "ERROR: Cannot get most recent decl of callee "
				     << Helper::prettyPrintExpr(callExprStmt) 
				     << "\n";
			rv = false;
			return;
		    }
		    SourceLocation SL =
			SM->getExpansionLoc(calleeFD->getLocStart());
		    if (SM->isInSystemHeader(SL) ||
			    !SM->isWrittenInMainFile(SL)) {
			continue;
		    }

		    for (std::vector<VarDeclDetails*>::iterator gvIt =
			    mainObj->globalVars.begin(); gvIt != 
			    mainObj->globalVars.end(); gvIt++) {
			if ((*gvIt)->var.isArray()) continue;
			std::pair<VarDetails, std::pair<const Expr*, std::string> >
			    def;
			def.first = (*gvIt)->var;
			def.second.first = NULL;
			// Find the block level offset of statement
			StmtMapTy::iterator I = StmtMap.find(currStmt);
			if (I == StmtMap.end()) {
			    llvm::outs() << "ERROR: Cannot find entry for expression: "
					 << Helper::prettyPrintStmt(currStmt) 
					 << " in StmtMap\n";
			    rv = false;
			    return;
			}
			std::pair<unsigned, unsigned> offset =
			    std::make_pair(I->second.first, I->second.second);
			std::stringstream ss;
			ss << "DP_GLOBAL_VAR_AT_FUNC_" 
			   << calleeFD->getNameInfo().getAsString() << "_B" 
			   << offset.first << "." << offset.second;
			def.second.second = ss.str();
			defsFound.push_back(def);
			foundDef = true;

			// Record var read.
			std::pair<VarDetails, int> p;
			p.first = (*gvIt)->var.clone();
			p.second = line;
			// Check if this is already present in varsDefined
			if (!isVarDefPresent(p, varsDefined))
			    varsDefined.push_back(p);
		    }
		}

		// Add the def found to currGenSet
		if (foundDef) {
		    //for (std::vector<std::pair<std::string, std::string>
		    for (std::vector<std::pair<VarDetails, std::pair<const Expr*, std::string> >
			    >::iterator dIt = defsFound.begin(); dIt !=
			    defsFound.end(); dIt++) {
			std::string varID = dIt->first.varID;
			std::string varName = dIt->first.varName;
			std::string rhsExpr = dIt->second.second;

			Definition def;
			def.lineNumber = line;
			def.blockID = currBlockID;
			def.expression = dIt->second.first;
			def.expressionStr = rhsExpr;

			// If there is already a def for current var, then delete it
			// since the current def will kill it.
			if (currGenSet.find(varID) != currGenSet.end())
			    currGenSet.erase(currGenSet.find(varID));

#if 0
			llvm::outs() << "DEBUG: Adding VARID: " << varID 
				     << " and def: ";
			def.print();
			llvm::outs() << " to genSet\n";

#endif
			currGenSet[varID] = def;

			// Add this def to blockDefs
			if (blockDefs.find(varID) != blockDefs.end()) {
			    if (!isDefPresentInVector(def, blockDefs[varID]))
				blockDefs[varID].push_back(def);
			} else {
			    std::vector<Definition> defs;
			    defs.push_back(def);
			    blockDefs[varID] = defs;
			}
		    }
		}
	    }
	}

	// Add currGenSet to genSet
	if (rd.genSet.find(currBlockID) != rd.genSet.end()) {
	    rd.genSet.erase(rd.genSet.find(currBlockID));
	    rd.genSet[currBlockID] = currGenSet;
	} else 
	    rd.genSet[currBlockID] = currGenSet;

	// Add blockDefs to allDefsInBlock
	// Sort the blockDefs in the order of the line number
	if (rd.allDefsInBlock.find(currBlockID) != rd.allDefsInBlock.end()) {
	    rd.allDefsInBlock.erase(rd.allDefsInBlock.find(currBlockID));
	    rd.allDefsInBlock[currBlockID] = blockDefs;
	} else
	    rd.allDefsInBlock[currBlockID] = blockDefs;

	// Update blockToVarsDefined
	if (rd.blockToVarsDefined.find(currBlockID) !=
		rd.blockToVarsDefined.end()) {
	    rd.blockToVarsDefined.erase(rd.blockToVarsDefined.find(currBlockID));
	    rd.blockToVarsDefined[currBlockID] = varsDefined;
	} else
	    rd.blockToVarsDefined[currBlockID] = varsDefined;

	// Update blockLineToVarsUsedInDef
	if (rd.blockLineToVarsUsedInDef.find(currBlockID) != 
		rd.blockLineToVarsUsedInDef.end()) {
	    rd.blockLineToVarsUsedInDef.erase(rd.blockLineToVarsUsedInDef.find(currBlockID));
	    rd.blockLineToVarsUsedInDef[currBlockID] = varsUsedInBlock;
	} else
	    rd.blockLineToVarsUsedInDef[currBlockID] = varsUsedInBlock;

	// Add successors of currBlock to workList
	for (CFGBlock::succ_iterator sIt = currBlock->succ_begin(); sIt !=
		currBlock->succ_end(); sIt++) {
	    CFGBlock* succBlock = sIt->getReachableBlock();
	    if (processedBlocks.find(succBlock->getBlockID()) ==
		    processedBlocks.end()) {
		workList.push(succBlock->getBlockID());
	    }
	}
    }

#ifdef DEBUG
    llvm::outs() << "========================================================\n";
    llvm::outs() << "DEBUG: genSet:\n";
    for (std::map<unsigned, std::map<std::string, Definition> >::iterator gIt = 
	    rd.genSet.begin(); gIt != rd.genSet.end(); gIt++) {
	llvm::outs() << "DEBUG: Basic block " << gIt->first << ":\n";
	for (std::map<std::string, Definition>::iterator mIt =
		gIt->second.begin(); mIt != gIt->second.end(); mIt++) {
	    llvm::outs() << "DEBUG: VarID: " << mIt->first << ", ";
	    mIt->second.print(); 
	}
    }
    llvm::outs() << "========================================================\n";
    llvm::outs() << "DEBUG: allDefsInBlock:\n";
    for (std::map<unsigned, std::map<std::string, std::vector<Definition> >
	    >::iterator it = rd.allDefsInBlock.begin(); it != 
	    rd.allDefsInBlock.end();
	    it++) {
	llvm::outs() << "DEBUG: Basic block " << it->first << ":\n";
	for (std::map<std::string, std::vector<Definition> >::iterator vIt = 
		it->second.begin(); vIt != it->second.end(); vIt++) {
	    llvm::outs() << "DEBUG: VarID: " << vIt->first << "\n";
	    for (std::vector<Definition>::iterator dIt = vIt->second.begin(); 
		    dIt != vIt->second.end(); dIt++)
		dIt->print();
	}
	llvm::outs() << "\n";
    }
    llvm::outs() << "========================================================\n";
    llvm::outs() << "DEBUG: blockToVarsDefined:\n";
    for (std::map<unsigned, std::vector<std::pair<VarDetails, int> >
	    >::iterator bIt = rd.blockToVarsDefined.begin(); bIt !=
	    rd.blockToVarsDefined.end(); bIt++) {
	llvm::outs() << "DEBUG: Basic block " << bIt->first << ":\n";
	for (std::vector<std::pair<VarDetails, int> >::iterator vIt = 
		bIt->second.begin(); vIt != bIt->second.end(); vIt++) {
	    llvm::outs() << "DEBUG: VarID: ";
	    vIt->first.print();
	    llvm::outs() << " at line " << vIt->second << "\n";
	}
    }
    llvm::outs() << "========================================================\n";
    llvm::outs() << "DEBUG: blockLineToVarsUsedInDef:\n";
    for (std::map<unsigned, std::map<int, std::vector<VarDetails> > >::iterator 
	    it = rd.blockLineToVarsUsedInDef.begin(); it != 
	    rd.blockLineToVarsUsedInDef.end(); it++) {
	llvm::outs() << "DEBUG: Basic block " << it->first << ":\n";
	for (std::map<int, std::vector<VarDetails> >::iterator lIt =
		it->second.begin(); lIt != it->second.end(); lIt++) {
	    llvm::outs() << "DEBUG: line: " << lIt->first << "\n";
	    for (std::vector<VarDetails>::iterator vIt = lIt->second.begin();
		    vIt != lIt->second.end(); vIt++)
		vIt->print();
	}
	llvm::outs() << "\n";
    }
    llvm::outs() << "========================================================\n";
#endif

    // Initialize reachOutSet of each block with the genSet
    for (std::map<unsigned, std::map<std::string, Definition> >::iterator bIt = 
	    rd.genSet.begin(); bIt != rd.genSet.end(); bIt++) {
	unsigned blockID = bIt->first;
	if (rd.reachOutSet.find(blockID) != rd.reachOutSet.end()) {
	    llvm::outs() << "ERROR: reachOutSet already contains entry for block "
			 << blockID << "\n";
	    rv = false;
	    return;
	}
	std::map<std::string, std::set<Definition> > blockDefs;
	for (std::map<std::string, Definition>::iterator mIt =
		bIt->second.begin(); mIt != bIt->second.end(); mIt++) {
	    std::set<Definition> defs;
	    defs.insert(mIt->second);
	    blockDefs[mIt->first] = defs;
	}
	rd.reachOutSet[blockID] = blockDefs;
    }

    // Clear the workList
    std::queue<unsigned> empty;
    std::swap (workList, empty);

    // Find block ID of CFG exit block.
    unsigned exitBlockID = (function->cfg->getExit()).getBlockID();
    //unsigned firstBlockID;

    // Insert first block into workList
    for (CFGBlock::succ_iterator sIt = entry.succ_begin(); sIt !=
	    entry.succ_end(); sIt++) {
	CFGBlock* succBlock = sIt->getReachableBlock();
	// Keeps the ID of successor to entryBlock
	//firstBlockID = succBlock->getBlockID();
	workList.push(succBlock->getBlockID());
    }

    // Keep track of blocks that are already processed.
    std::set<unsigned> markedBlocks;

    unsigned totalBlocks = function->cfg->size();
    // bitvector to store whether the outset of each block was modified or not.
    std::vector<bool> out(totalBlocks);
    // Initialize out to all true
    for (unsigned i = 0; i < totalBlocks; i++) {
	out[i] = true;
    }
    // Set entry block and exit block to false, since there are dummy blocks and
    // will not analyzed in the loop.
    out[entryBlockID] = false;
    out[exitBlockID] = false;

    // Loop to compute fixpoint
    while (true) {
	// If no block has its outset modified, we reached fixpoint and can
	// break from this loop.
	bool fixpoint = true;
	for (unsigned i = 0; i < totalBlocks; i++) {
	    if (out[i]) {
		fixpoint = false;
		break;
	    }
	}
	if (fixpoint)
	    break;

	// Go through all blocks once and then check for fix point
      for (unsigned i = 0; i < totalBlocks; i++) {
	CFGBlock *currBlock;
	unsigned currBlockID = i;
#if 0
	if (workList.empty()) {
	    currBlockID = firstBlockID;
	} else {
	    currBlockID = workList.front();
	    workList.pop();
	}
#endif

	// Get block from block ID
	bool foundBlock = false;
	for (CFG::iterator bIt = function->cfg->begin(); bIt != function->cfg->end();
		bIt++) {
	    CFGBlock* bl = *bIt;
	    if (bl->getBlockID() == currBlockID) {
		currBlock = *bIt;
		foundBlock = true;
		break;
	    }
	}
	if (!foundBlock) {
	    llvm::outs() << "ERROR: Could not find block with ID: " 
			 << currBlockID << "\n";
	    rv = false;
	    return;
	}

#ifdef DEBUG
	llvm::outs() << "DEBUG: Processing block " << currBlockID << "\n";
#endif

	std::map<std::string, std::set<Definition> > currReachInSet;
	std::map<std::string, std::set<Definition> > currReachOutSet;
	std::map<std::string, Definition> currGenSet;

	// populate current reach out set
	if (rd.reachOutSet.find(currBlockID) == rd.reachOutSet.end()) {
	    llvm::outs() << "ERROR: Could not find block " << currBlockID
			 << " in reachOutSet\n";
	    rv = false;
	    return;
	} else {
	    currReachOutSet = rd.reachOutSet[currBlockID];
	}

	// populate current genSet
	if (rd.genSet.find(currBlockID) == rd.genSet.end()) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Empty genSet for block " << currBlockID 
			 << "\n";
#endif
	} else {
	    currGenSet = rd.genSet[currBlockID];
	}

	// Populate current reach in set
	if (rd.reachInSet.find(currBlockID) == rd.reachInSet.end()) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Empty reachInSet for block " << currBlockID
			 << "\n";
#endif
	} else {
	    currReachInSet = rd.reachInSet[currBlockID];
	}

	// Compute currReachInSet from reachOutSet of predecessor blocks. While
	// doing this, add only those defs that are not present in the already
	// existing currReachInSet.
	for (CFGBlock::pred_iterator pIt = currBlock->pred_begin(); pIt !=
		currBlock->pred_end(); pIt++) {
	    unsigned predBlockID = pIt->getReachableBlock()->getBlockID();

#ifdef DEBUG
	    llvm::outs() << "DEBUG: Predecessor block: " << predBlockID << "\n";
#endif
	    std::map<std::string, std::set<Definition> > predReachOutSet;
	    if (rd.reachOutSet.find(predBlockID) != rd.reachOutSet.end()) {
#if 0
		llvm::outs() << "DEBUG: Found predBlockID " << predBlockID 
			     << " in reachOutSet\n";
#endif
		predReachOutSet = rd.reachOutSet[predBlockID];
	    }

	    for (std::map<std::string, std::set<Definition> >::iterator it = 
		    predReachOutSet.begin(); it != predReachOutSet.end(); it++) {
		std::string varID = it->first;
#if 0
		llvm::outs() << "DEBUG: Var: " << varID << ":\nDefs: ";
		for (std::set<Definition>::iterator sIt = 
			it->second.begin(); sIt != it->second.end(); sIt++)
		    sIt->print();
		llvm::outs() << "\n";
#endif
		std::set<Definition> defsForVar;
		if (currReachInSet.find(varID) != currReachInSet.end()) {
		    defsForVar.insert(currReachInSet[varID].begin(),
			currReachInSet[varID].end());
		    currReachInSet.erase(currReachInSet.find(varID));
		}

		if (isBackEdge(predBlockID, currBlockID)) {
		    for (std::set<Definition>::iterator dIt =
			    it->second.begin(); dIt != it->second.end(); dIt++) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: Checking Definition:\n";
			dIt->print();
#endif
			if (isDefPresentInSet(*dIt, defsForVar)) {
#ifdef DEBUG
			    llvm::outs() << "DEBUG: Definition already present"
					 << " in currReachInSet\n";
#endif
			    continue;
			} else {
#ifdef DEBUG
			    llvm::outs() << "DEBUG: Definition not present"
					 << " in currReachInSet\n";
#endif
			}

#ifdef DEBUG
			llvm::outs() << "DEBUG: Found definition through backEdge\n";
#endif

			Definition d;
			d.lineNumber = dIt->lineNumber;
			d.blockID = dIt->blockID;
			d.expression = dIt->expression;
			d.expressionStr = dIt->expressionStr;
			d.throughBackEdge = true;
			d.backEdgesTraversed.insert(dIt->backEdgesTraversed.begin(),
			    dIt->backEdgesTraversed.end());
			d.backEdgesTraversed.insert(std::pair<unsigned,
			    unsigned>(predBlockID, currBlockID));
			defsForVar.insert(d);
		    }
		} else {
		    defsForVar.insert(it->second.begin(),
			it->second.end());
		}
#if 0
		llvm::outs() << "DEBUG: defsForVar:\n";
		for (std::set<Definition>::iterator it = defsForVar.begin(); 
			it != defsForVar.end(); it++)
		    it->print();
#endif

		currReachInSet[varID] = defsForVar;
	    }
	}

#ifdef DEBUG
	llvm::outs() << "DEBUG: currReachOutSet:\n";
	for (std::map<std::string, std::set<Definition> >::iterator vIt = 
		currReachOutSet.begin(); vIt != currReachOutSet.end(); vIt++) {
	    llvm::outs() << "DEBUG: VarID: " << vIt->first << ": \n";
	    for (std::set<Definition>::iterator sIt = vIt->second.begin(); sIt != 
		    vIt->second.end(); sIt++) {
		sIt->print();
	    }
	    llvm::outs() << "\n";
	}
	llvm::outs() << "DEBUG: currReachInSet:\n";
	for (std::map<std::string, std::set<Definition> >::iterator vIt = 
		currReachInSet.begin(); vIt != currReachInSet.end(); vIt++) {
	    llvm::outs() << "DEBUG: VarID: " << vIt->first << ": \n";
	    for (std::set<Definition>::iterator sIt = vIt->second.begin(); sIt != 
		    vIt->second.end(); sIt++) {
		sIt->print();
	    }
	    llvm::outs() << "\n";
	}
#endif

	for (std::map<std::string, std::set<Definition> >::iterator inIt = 
		currReachInSet.begin(); inIt != currReachInSet.end(); inIt++) {
	    std::string varID = inIt->first;
	    if (currReachOutSet.find(varID) == currReachOutSet.end()) {
		// If currReachOutSet does not contain defs for this var,
		// simply propagate the defs in currReachInSet
		currReachOutSet[inIt->first] = inIt->second;
	    } else {
		// If currReachOutSet contains defs for this var, check this
		// block generated these defs.
		if (currGenSet.find(varID) == currGenSet.end()) {
		    // Current block does not generate any def for this var. So
		    // insert all the defs from currReachInSet to
		    // currReachOutSet.
		    currReachOutSet[inIt->first].insert(inIt->second.begin(),
			inIt->second.end());
		} else {
		    // Current block generates def for this var. Then it kils
		    // all the incoming defs from currReachInSet. Nothing to do
		    // here.
		}
	    }
	}
#ifdef DEBUG
	llvm::outs() << "DEBUG: currReachOutSet:\n";
	for (std::map<std::string, std::set<Definition> >::iterator
		vIt = currReachOutSet.begin(); vIt != currReachOutSet.end();
		vIt++) {
	    llvm::outs() << "DEBUG: VarID: " << vIt->first << ": \n";
	    for (std::set<Definition>::iterator sIt = 
		    vIt->second.begin(); sIt != vIt->second.end(); sIt++) {
		sIt->print();
		llvm::outs() << "   ";
	    }
	    llvm::outs() << "\n";
	}
#endif

	bool outChanged = false;
	if (rd.reachOutSet.find(currBlockID) == rd.reachOutSet.end()) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: current block not in reachOutSet\n";
#endif
	    rd.reachOutSet[currBlockID] = currReachOutSet;
	    outChanged = true;
	} else {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: current block in reachOutSet\n";
#endif
	    outChanged = false;
	    // Check if the out set has changed
	    if (rd.reachOutSet[currBlockID].size() != currReachOutSet.size()) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: rd.reachOutSet[currBlockID].size() != "
			     << "currReachOutSet.size()\n";
#endif
		outChanged = true;
	    } else {
		// Checks defs for each var
		for (std::map<std::string, std::set<Definition> >::iterator sIt
			= currReachOutSet.begin();
			sIt != currReachOutSet.end(); sIt++) {
		    std::string varID = sIt->first;
		    if (rd.reachOutSet[currBlockID].find(varID) ==
			rd.reachOutSet[currBlockID].end()) {
			// currReachOutSet contains a new var-def.
#ifdef DEBUG
			llvm::outs() << "DEBUG: reachOutSet for var " << varID 
				     << " is empty\n";
#endif
			outChanged = true;
			break;
		    } else {
			if (sIt->second.size() !=
			    rd.reachOutSet[currBlockID][varID].size()) {
#ifdef DEBUG
			    llvm::outs() << "DEBUG: reachOutSet for var size() "
					 << "!= currReachOutSet[var].size()\n";
#endif
			    outChanged = true;
			    break;
			} else {
			    std::set<Definition>::iterator cIt
				= sIt->second.begin();
			    std::set<Definition>::iterator rIt 
				= rd.reachOutSet[currBlockID][varID].begin();
			    for (; cIt != sIt->second.end() && rIt !=
				    rd.reachOutSet[currBlockID][varID].end();
				    cIt++,rIt++) {
				if (cIt->lineNumber != rIt->lineNumber || 
				    cIt->expressionStr.compare(rIt->expressionStr) != 0) {
#ifdef DEBUG
				    llvm::outs() << "DEBUG: Different defs for var " 
						 << varID << "\n";
#endif
				    outChanged = true;
				    break;
				}
			    }
			    if (outChanged) break;
			}
		    }
		}
	    }
	    if (outChanged) {
		out[currBlockID] = true;
		rd.reachOutSet.erase(rd.reachOutSet.find(currBlockID));
		rd.reachOutSet[currBlockID] = currReachOutSet;
#ifdef DEBUG
		llvm::outs() << "DEBUG: out changed for " << currBlockID << "\n";
		llvm::outs() << "DEBUG: reachOutSet updated\n";
#endif
	    } else {
		out[currBlockID] = false;
#ifdef DEBUG
		llvm::outs() << "DEBUG: out not changed for " << currBlockID << "\n";
#endif
	    }
	}

	if (rd.reachInSet.find(currBlockID) != rd.reachInSet.end())
	    rd.reachInSet.erase(currBlockID);
	rd.reachInSet[currBlockID] = currReachInSet;

	// Add successors of currBlock to workList
	for (CFGBlock::succ_iterator sIt = currBlock->succ_begin(); sIt !=
		currBlock->succ_end(); sIt++) {
	    CFGBlock* succBlock = sIt->getReachableBlock();
		workList.push(succBlock->getBlockID());
#ifdef DEBUG
		llvm::outs() << "DEBUG: Adding successor " << succBlock->getBlockID()
			     << " to workList\n";
#endif
	}
	if (outChanged && currBlock->succ_size() == 1) {
	    if (currBlock->succ_begin()->getReachableBlock()->getBlockID() ==
		    exitBlockID) {
		workList.push(entryBlockID);
	    }
	}
      }
    }

#ifdef DEBUG
    llvm::outs() << "========================================================\n";
    llvm::outs() << "DEBUG: reachOutSet:\n";
    for (std::map<unsigned, std::map<std::string, std::set<Definition> > 
	    >::iterator rIt = rd.reachOutSet.begin(); rIt != 
	    rd.reachOutSet.end(); rIt++) {
	llvm::outs() << "DEBUG: Basic block " << rIt->first << ":\n";
	for (std::map<std::string, std::set<Definition> >::iterator vIt = 
		rIt->second.begin(); vIt != rIt->second.end(); vIt++) {
	    llvm::outs() << "DEBUG: VarID: " << vIt->first << ": \n";
	    for (std::set<Definition>::iterator sIt = 
		    vIt->second.begin(); sIt != vIt->second.end(); sIt++) {
		sIt->print();
	    }
	}
	llvm::outs() << "\n";
    }
    llvm::outs() << "========================================================\n";
    llvm::outs() << "DEBUG: reachInSet:\n";
    for (std::map<unsigned, std::map<std::string, std::set<Definition> >
	    >::iterator rIt = rd.reachInSet.begin(); rIt != 
	    rd.reachInSet.end(); rIt++) {
	llvm::outs() << "DEBUG: Basic block " << rIt->first << ":\n";
	for (std::map<std::string, std::set<Definition> >::iterator vIt = 
		rIt->second.begin(); vIt != rIt->second.end();
		vIt++) {
	    llvm::outs() << "DEBUG: VarID: " << vIt->first << ": \n";
	    for (std::set<Definition>::iterator sIt = 
		    vIt->second.begin(); sIt != vIt->second.end(); sIt++) {
		sIt->print();
	    }
	}
	llvm::outs() << "\n";
    }
    llvm::outs() << "========================================================\n";
#endif
}

#if 0
void FunctionAnalysis::populateAvailableExpressions(const MatchFinder::
    MatchResult &Result, CFG* cfg, AvailExpr& ae) {

#ifdef DEBUG
    llvm::outs() << "DEBUG: Calling populateReachingDefinitions on " 
		 << ae.funcName << "(" << ae.funcStartLine << " - " 
		 << ae.funcEndLine << ")\n";
#endif

    SourceManager &SM = Result.Context->getSourceManager();

    // For pretty printing the CFG
    LangOptions LangOpts;
    LangOpts.CPlusPlus = true;

    // workList holding basic block ids
    std::queue<unsigned> workList;

    CFGBlock entry = cfg->getEntry();
    unsigned succSize = entry.succ_size();

    // Add successor of entry node to workList.
    if (succSize == 1) { // Unnecessary check since cfg's entry node has a
			 // single successor
	for (CFGBlock::succ_iterator sIt = entry.succ_begin(); sIt !=
		entry.succ_end(); sIt++) {
	    CFGBlock* succBlock = sIt->getReachableBlock();
#ifdef DEBUG
	    llvm::outs() << "DEBUG: successor: block " 
			 << succBlock->getBlockID() << "\n";
#endif
	    workList.push(succBlock->getBlockID());
	}
    }

    // Keep track of blocks already processed so that we do not add same blocks
    // back into the workList
    std::set<unsigned> processedBlocks;
    while (true) {
	if (workList.empty())
	    break;

	CFGBlock* currBlock;
	unsigned currBlockID = workList.front();
	workList.pop();

	// Add this block to set of processedBlocks
	processedBlocks.insert(currBlockID);

	// get basic block from id
	bool foundBlock = false;
	for (CFG::iterator bIt = cfg->begin(); bIt !=
		cfg->end(); bIt++) {
	    if ((*bIt)->getBlockID() == currBlockID) {
		currBlock = *bIt;
#if 0
		llvm::outs() << "DEBUG: Found block with ID " << currBlockID 
			     << "\n";
#endif
		foundBlock = true;
		break;
	    }
	}

	if (!foundBlock) {
	    llvm::outs() << "ERROR: Could not find the block with ID " 
			 << currBlockID << "\n";
	    return;
	}

#ifdef DEBUG
	llvm::outs() << "DEBUG: Looking at basic block " << currBlockID << "\n";
	currBlock->dump();
#endif

	std::set<std::pair<std::pair<int, unsigned>, std::string> > currGenSet;
	// Kill Set just records the variable names that are reassigned in the
	// block
	//std::set<std::pair<int, std::string> > currKillSet;
	std::map<std::string, std::vector<std::pair<int, std::string> >
	    > blockDefs;

	// Why is this code here?
	// If the genSet of current block is already present, retrieve it.
	if (ae.genSet.find(currBlockID) != ae.genSet.end())
	    currGenSet = ae.genSet[currBlockID];

	// compute gen set of current block
	for (CFGBlock::iterator eIt = currBlock->begin(); eIt != currBlock->end();
		eIt++) {
	    // Look only at those CFGElements that are Statements.
	    if (eIt->getKind() == CFGElement::Kind::Statement) {
		CFGStmt currCFGStmt = eIt->castAs<CFGStmt>();
		const Stmt* currStmt = currCFGStmt.getStmt();
		if (!currStmt) {
#ifdef ERRORMSG
		    llvm::outs() << "ERROR: Cannot get Stmt from CFGStmt\n";
#endif
		    return;
		}

		std::string varName, rhsExpr;
		std::vector<std::string> exprsGenerated;
		std::vector<std::string> varsDefined;
		int line = SM.getExpansionLineNumber(currStmt->getLocStart(), 0);
		bool foundDef = false;
		if (isa<BinaryOperator>(currStmt)) { // Found an assignment statement
		    const BinaryOperator* currStmtAsBO =
			dyn_cast<BinaryOperator>(currStmt);
		    if (currStmtAsBO->isAssignmentOp()) {
			const Expr* stmtLHSExpr = currStmtAsBO->getLHS();
			if (!stmtLHSExpr) {
#ifdef ERRORMSG
			    llvm::outs() << "ERROR: Cannot get LHS Expr from Assignment\n";
#endif
			    return;
			}

			if (isa<DeclRefExpr>(stmtLHSExpr)) {
			    varName =
				Helper::prettyPrintExpr(stmtLHSExpr);
			    varsDefined.push_back(varName);
			    rhsExpr = 
				Helper::prettyPrintExpr(currStmtAsBO->getRHS());
			    exprsGenerated.push_back(rhsExpr);
			    foundDef = true;
			} else if (isa<ArraySubscriptExpr>(stmtLHSExpr)) {
			    const ArraySubscriptExpr* arrayExpr =
				dyn_cast<ArraySubscriptExpr>(stmtLHSExpr);
			    const Expr* arrayBase = arrayExpr->getBase();
			    Expr const *base = arrayBase;
			    while (isa<ArraySubscriptExpr>(base)) {
				const ArraySubscriptExpr* arrExpr = 
				    dyn_cast<ArraySubscriptExpr>(base);
				base = arrExpr->getBase();
			    }
			    varName = Helper::prettyPrintExpr(base);
			    varsDefined.push_back(varName);
			}
		    }
		} else if (isa<UnaryOperator>(currStmt)) { // Found increment/decrement operator
		    const UnaryOperator* currStmtAsUO = 
			dyn_cast<UnaryOperator>(currStmt);
		    if (currStmtAsUO->isIncrementDecrementOp()) {
			Expr* subExpr = currStmtAsUO->getSubExpr();
			if (!subExpr) {
			    llvm::outs() << "ERROR: Cannot get subexpr from UnaryOperator\n";
			    return;
			}

			if (isa<DeclRefExpr>(subExpr)) {
			    varName =
				Helper::prettyPrintExpr(subExpr);
			    varsDefined.push_back(varName);
			    foundDef = true;
			    if (currStmtAsUO->isIncrementOp())
				rhsExpr = "++";
			    else if (currStmtAsUO->isDecrementOp())
				rhsExpr = "--";
			    else
				foundDef = false;
			} else if (isa<ArraySubscriptExpr>(subExpr)) {
			    const ArraySubscriptExpr* arrayExpr =
				dyn_cast<ArraySubscriptExpr>(subExpr);
			    Expr const *arrayBase = arrayExpr->getBase();
			    while (isa<ArraySubscriptExpr>(arrayBase)) {
				const ArraySubscriptExpr* arrExpr = 
				    dyn_cast<ArraySubscriptExpr>(arrayBase);
				arrayBase = arrExpr->getBase();
			    }
			    varName = Helper::prettyPrintExpr(arrayBase);
			    varsDefined.push_back(varName);
			}
		    }
		} else if (Helper::isInputStmt(currStmt)) { // Found input stmt
		    std::vector<const Expr*> args =
			Helper::getInputVarsFromStmt(currStmt);

		    for (std::vector<const Expr*>::iterator aIt = args.begin();
			    aIt != args.end(); aIt++) {
			if (isa<DeclRefExpr>(*aIt)) {
			    varsDefined.push_back(Helper::prettyPrintExpr(*aIt));
			    foundDef = true;
			} else if (isa<ArraySubscriptExpr>(*aIt)) {
			    ArraySubscriptExpr const *arrayExpr = 
				dyn_cast<ArraySubscriptExpr>(*aIt);
			    Expr const *arrayBase = arrayExpr->getBase();
			    while (isa<ArraySubscriptExpr>(arrayBase)) {
				arrayExpr =
				    dyn_cast<ArraySubscriptExpr>(arrayBase);
				arrayBase = arrayExpr->getBase();
			    }
			    varsDefined.push_back(Helper::prettyPrintExpr(arrayBase));
			}
		    }
#if 0
		} else if (isa<CXXOperatorCallExpr>(currStmt)) { // Found cin
		    const CXXOperatorCallExpr* currStmtAsCallExpr =
			dyn_cast<CXXOperatorCallExpr>(currStmt);
		    unsigned numArgs = currStmtAsCallExpr->getNumArgs();
		    const Expr* firstArg =
			currStmtAsCallExpr->getArg(0)->IgnoreParenCasts();
		    if (!firstArg) {
			llvm::outs() << "ERROR: Cannot get first argument of CXXOperatorCallExpr\n";
			return;
		    }
		    std::string operatorName = Helper::prettyPrintExpr(firstArg);
		    if (operatorName.compare("cin") != 0) {
			//return; // Not cin call.
			continue;
		    }
		    for (unsigned i = 1; i < numArgs; i++) {
			const Expr* cinArgExpr =
			    currStmtAsCallExpr->getArg(i)->IgnoreParenCasts();
			if (isa<DeclRefExpr>(cinArgExpr)) {
			    varName = Helper::prettyPrintExpr(cinArgExpr);
			    varsDefined.push_back(varName);
			    rhsExpr = "DP_INPUT";
			    foundDef = true;
			} else if (isa<ArraySubscriptExpr>(stmtLHSExpr)) {
			    const ArraySubscriptExpr* arrayExpr =
				dyn_cast<ArraySubscriptExpr>(stmtLHSExpr);
			    Expr const *arrayBase = arrayExpr->getBase();
			    while (isa<ArraySubscriptExpr>(arrayBase)) {
				const ArraySubscriptExpr* arrExpr = 
				    dyn_cast<ArraySubscriptExpr>(arrayBase);
				arrayBase = arrayBase->getBase();
			    }
			    varName = Helper::prettyPrintExpr(arrayBase);
			    varsDefined.push_back(varName);
			}
		    }
#endif
		} 

#if 0
		if (foundDef) {
		    std::pair<int, std::string> killInst;
		    killInst.first = line;
		    killInst.second = varName;
		    currKillSet.insert(killInst);

		    for (std::set<std::pair<int, std::string>
			    >::iterator sIt = currGenSet.begin(); sIt !=
			    currGenSet.end(); sIt++) {
			if (sIt->first < killInst.first &&
				Helper::indexPresentInString(killInst.second,
				sIt->second)) {
			    currGenSet.erase(sIt);
			}
		    }

		    if (!foundInp) {
			std::pair<int, std::string> genInst;
			genInst.first = line;
			genInst.second = rhsExpr;
			currGenSet.insert(genInst);
		    }
		}
#endif

		if (foundDef) {
		    for (std::vector<std::string>::iterator eIt =
			    exprsGenerated.begin(); eIt != exprsGenerated.end();
			    eIt++) {
			std::pair<std::pair<int, unsigned>, std::string> genExpr;
			genExpr.first.first = line;
			genExpr.first.second = currBlockID;
			genExpr.second = *eIt;

			currGenSet.insert(genExpr);
		    }

		    for (std::vector<std::string>::iterator vIt = 
			    varsDefined.begin(); vIt != varsDefined.end();
			    vIt++) {
			for (std::set<std::pair<std::pair<int, unsigned>, std::string> >::iterator sIt = 
				currGenSet.begin(); sIt != currGenSet.end(); sIt++) {
			    if (sIt->first.first < line &&
				    Helper::indexPresentInString(*vIt, sIt->second))
				currGenSet.erase(sIt);
			}
		    }
		}



#if 0
		// Add the def found to currGenSet
		if (foundDef) {
		    // Add this def to blockDefs
		    if (blockDefs.find(varName) != blockDefs.end())
			blockDefs[varName].push_back(def);
		    else {
			std::vector<std::pair<int, std::string> > defs;
			defs.push_back(def);
			blockDefs[varName] = defs;
		    }
		}
#endif
	    }
	}

	// Add currGenSet to genSet
	if (ae.genSet.find(currBlockID) != ae.genSet.end()) {
	    ae.genSet.erase(ae.genSet.find(currBlockID));
	    ae.genSet[currBlockID] = currGenSet;
	} else 
	    ae.genSet[currBlockID] = currGenSet;

#if 0
	// Add blockDefs to allDefsInBlock
	if (rd.allDefsInBlock.find(currBlockID) != rd.allDefsInBlock.end()) {
	    rd.allDefsInBlock.erase(rd.allDefsInBlock.find(currBlockID));
	    rd.allDefsInBlock[currBlockID] = blockDefs;
	} else
	    rd.allDefsInBlock[currBlockID] = blockDefs;
#endif

	// Add successors of currBlock to workList
	for (CFGBlock::succ_iterator sIt = currBlock->succ_begin(); sIt !=
		currBlock->succ_end(); sIt++) {
	    CFGBlock* succBlock = sIt->getReachableBlock();
	    if (processedBlocks.find(succBlock->getBlockID()) ==
		    processedBlocks.end()) {
		workList.push(succBlock->getBlockID());
	    }
	}
    }

#ifdef DEBUG
    llvm::outs() << "========================================================\n";
    llvm::outs() << "DEBUG: genSet:\n";
    for (std::map<unsigned, std::set<std::pair<std::pair<int, unsigned>, std::string> > >::iterator gIt =
	    ae.genSet.begin(); gIt != ae.genSet.end(); gIt++) {
	llvm::outs() << "DEBUG: Basic block " << gIt->first << ":\n";
	for (std::set<std::pair<std::pair<int, unsigned>, std::string> >::iterator mIt =
		gIt->second.begin(); mIt != gIt->second.end(); mIt++)
	    llvm::outs() << "(" << mIt->first.first << ", " << mIt->first.second 
			 << ", " << mIt->second << ") ";
	llvm::outs() << "\n";
    }
    llvm::outs() << "========================================================\n";
#endif

    // Initialize outSet of each block with the genSet
    for (std::map<unsigned, std::set<std::pair<std::pair<int, unsigned>, std::string> >
	    >::iterator bIt = ae.genSet.begin(); bIt != ae.genSet.end(); bIt++) {
	unsigned blockID = bIt->first;
	if (ae.availOutSet.find(blockID) != ae.availOutSet.end()) {
	    llvm::outs() << "ERROR: availOutSet already contains entry for block " 
			 <<  blockID << "\n";
	    continue;
	}
	std::set<std::pair<std::pair<int, unsigned>, std::string> > blockExprs;
	blockExprs.insert(bIt->second.begin(), bIt->second.end());
	ae.availOutSet[blockID] = blockExprs;
    }

    // Clear the workList
    std::queue<unsigned> empty;
    std::swap (workList, empty);

    // Find block ID of CFG exit block.
    unsigned entryBlockID = entry.getBlockID();
    unsigned exitBlockID = (cfg->getExit()).getBlockID();
    unsigned firstBlockID;

    // Insert first block into workList
    for (CFGBlock::succ_iterator sIt = entry.succ_begin(); sIt !=
	    entry.succ_end(); sIt++) {
	CFGBlock* succBlock = sIt->getReachableBlock();
	// Keeps the ID of successor to entryBlock
	firstBlockID = succBlock->getBlockID();
	workList.push(succBlock->getBlockID());
    }

    // Keep track of blocks that are already processed.
    std::set<unsigned> markedBlocks;

    unsigned totalBlocks = cfg->size();
    // bitvector to store whether the outset of each block was modified or not.
    //bool out[totalBlocks];
    std::vector<bool> out(totalBlocks);
    // Initialize out to all true
    for (unsigned i = 0; i < totalBlocks; i++) {
	out[i] = true;
    }
    // Set entry block and exit block to false, since there are dummy blocks and
    // will not analyzed in the loop.
    out[entryBlockID] = false;
    out[exitBlockID] = false;

    // Clear processedblocks
    std::set<unsigned> emptySet;
    std::swap(processedBlocks, emptySet);

    // Loop to compute fixpoint
    while (true) {
	// If no block has its outset modified, we reached fixpoint and can
	// break from this loop.

	bool fixpoint = true;
	for (unsigned i = 0; i < totalBlocks; i++) {
	    if (out[i]) {
		fixpoint = false;
		break;
	    }
	}
	if (fixpoint)
	    break;

	CFGBlock *currBlock;
	unsigned currBlockID;
	if (workList.empty()) {
	    currBlockID = firstBlockID;
	} else {
	    currBlockID = workList.front();
	    workList.pop();
	}

	// Get block from block ID
	bool foundBlock = false;
	for (CFG::iterator bIt = cfg->begin(); bIt != cfg->end();
		bIt++) {
	    CFGBlock* bl = *bIt;
	    if (bl->getBlockID() == currBlockID) {
		currBlock = *bIt;
		foundBlock = true;
		break;
	    }
	}
	if (!foundBlock) {
	    llvm::outs() << "ERROR: Could not find block with ID: " 
			 << currBlockID << "\n";
	    return;
	}

#ifdef DEBUG
	llvm::outs() << "DEBUG: Processing block " << currBlockID << "\n";
	processedBlocks.insert(currBlockID);
#endif

	std::set<std::pair<std::pair<int, unsigned>, std::string> > currAvailInSet;
	std::set<std::pair<std::pair<int, unsigned>, std::string> > currAvailOutSet;
	std::set<std::pair<std::pair<int, unsigned>, std::string> > currGenSet;

#if 0
	std::set<std::pair<std::pair<std::pair<int, unsigned>, std::string>,
	    std::set<std::pair<unsigned, bool> > > > guardedCurrAvailInSet;
	std::set<std::pair<std::pair<std::pair<int, unsigned>, std::string>,
	    std::set<std::pair<unsigned, bool> > > > guardedCurrAvailOutSet;
	std::set<std::pair<std::pair<std::pair<int, unsigned>, std::string>,
	    std::set<std::pair<unsigned, bool> > > > guardedCurrGenSet;
#endif

	// populate current avail out set
	if (ae.availOutSet.find(currBlockID) == ae.availOutSet.end()) {
	    llvm::outs() << "ERROR: Could not find block " << currBlockID
			 << " in availOutSet\n";
	    return;
	} else {
	    currAvailOutSet = ae.availOutSet[currBlockID];
	}

	// populate current genSet
	if (ae.genSet.find(currBlockID) == ae.genSet.end()) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Empty genSet for block " << currBlockID 
			 << "\n";
#endif
	} else {
	    currGenSet = ae.genSet[currBlockID];
	}

	// Compute currAvailInSet from availOutSet of predecessor blocks.
	for (CFGBlock::pred_iterator pIt = currBlock->pred_begin(); pIt !=
		currBlock->pred_end(); pIt++) {
	    unsigned predBlockID = pIt->getReachableBlock()->getBlockID();

#if 0
	    llvm::outs() << "DEBUG: Predecessor block: " << predBlockID << "\n";
#endif
	    std::set<std::pair<std::pair<int, unsigned>, std::string> >
		predAvailOutSet;
	    if (ae.availOutSet.find(predBlockID) != ae.availOutSet.end()) {
#if 0
		llvm::outs() << "DEBUG: Found predBlockID " << predBlockID 
			     << " in availOutSet\n";
#endif
		predAvailOutSet = ae.availOutSet[predBlockID];
	    }

#if 0
	    for (std::map<std::string, std::set<std::pair<int, std::string> > >::iterator it = 
		    predReachOutSet.begin(); it != predReachOutSet.end(); it++) {
		std::string varName = it->first;
#if 0
		llvm::outs() << "DEBUG: Var: " << varName << ":\nDefs:\n";
		for (std::set<std::pair<int, std::string> >::iterator sIt =
			it->second.begin(); sIt != it->second.end(); sIt++)
		    llvm::outs() << "DEBUG: (" << sIt->first << ", " 
				 << sIt->second << ") ";
		llvm::outs() << "\n";
#endif
		std::set<std::pair<int, std::string> > defsForVar;
		if (currReachInSet.find(varName) != currReachInSet.end()) {
		    defsForVar.insert(currReachInSet[varName].begin(),
			currReachInSet[varName].end());
		    currReachInSet.erase(currReachInSet.find(varName));
		}

		defsForVar.insert(it->second.begin(),
		    it->second.end());

		currReachInSet[varName] = defsForVar;
	    }
#endif
	}

#ifdef DEBUG
	llvm::outs() << "DEBUG: currReachOutSet:\n";
	for (std::map<std::string, std::set<std::pair<int, std::string> >
		>::iterator vIt = currReachOutSet.begin(); vIt != currReachOutSet.end();
		vIt++) {
	    llvm::outs() << "DEBUG: Var: " << vIt->first << ": \n";
	    llvm::outs() << "DEBUG: Defs:\n";
	    for (std::set<std::pair<int, std::string> >::iterator sIt =
		    vIt->second.begin(); sIt != vIt->second.end(); sIt++) {
		llvm::outs() << "(" << sIt->first << ", " << sIt->second 
			     << ")   ";
	    }
	    llvm::outs() << "\n";
	}
	llvm::outs() << "DEBUG: currReachInSet:\n";
	for (std::map<std::string, std::set<std::pair<int, std::string> >
		>::iterator vIt = currReachInSet.begin(); vIt != currReachInSet.end();
		vIt++) {
	    llvm::outs() << "DEBUG: Var: " << vIt->first << ": \n";
	    llvm::outs() << "DEBUG: Defs:\n";
	    for (std::set<std::pair<int, std::string> >::iterator sIt =
		    vIt->second.begin(); sIt != vIt->second.end(); sIt++) {
		llvm::outs() << "(" << sIt->first << ", " << sIt->second 
			     << ")   ";
	    }
	    llvm::outs() << "\n";
	}
#endif

	for (std::map<std::string, std::set<std::pair<int, std::string> > >::iterator inIt =
		currReachInSet.begin(); inIt != currReachInSet.end(); inIt++) {
	    std::string varName = inIt->first;
	    if (currReachOutSet.find(varName) == currReachOutSet.end()) {
		// If currReachOutSet does not contain defs for this var,
		// simply propagate the defs in currReachInSet
		currReachOutSet[inIt->first] = inIt->second;
	    } else {
		// If currReachOutSet contains defs for this var, check this
		// block generated these defs.
		if (currGenSet.find(varName) == currGenSet.end()) {
		    // Current block does not generate any def for this var. So
		    // insert all the defs from currReachInSet to
		    // currReachOutSet.
		    currReachOutSet[inIt->first].insert(inIt->second.begin(),
			inIt->second.end());
		} else {
		    // Current block generates def for this var. Then it kils
		    // all the incoming defs from currReachInSet. Nothing to do
		    // here.
		}
	    }
	}
#ifdef DEBUG
	llvm::outs() << "DEBUG: currReachOutSet:\n";
	for (std::map<std::string, std::set<std::pair<int, std::string> >
		>::iterator vIt = currReachOutSet.begin(); vIt != currReachOutSet.end();
		vIt++) {
	    llvm::outs() << "DEBUG: Var: " << vIt->first << ": \n";
	    llvm::outs() << "DEBUG: Defs:\n";
	    for (std::set<std::pair<int, std::string> >::iterator sIt =
		    vIt->second.begin(); sIt != vIt->second.end(); sIt++) {
		llvm::outs() << "(" << sIt->first << ", " << sIt->second 
			     << ")   ";
	    }
	    llvm::outs() << "\n";
	}
#endif

	if (rd.reachOutSet.find(currBlockID) == rd.reachOutSet.end()) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: current block not in reachOutSet\n";
#endif
	    rd.reachOutSet[currBlockID] = currReachOutSet;
	} else {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: current block in reachOutSet\n";
#endif
	    bool outChanged = false;
	    // Check if the out set has changed
	    if (rd.reachOutSet[currBlockID].size() != currReachOutSet.size()) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: rd.reachOutSet[currBlockID].size() != "
			     << "currReachOutSet.size()\n";
#endif
		outChanged = true;
	    } else {
		// Checks defs for each var
		for (std::map<std::string, std::set<std::pair<int, std::string>
			> >::iterator sIt = currReachOutSet.begin();
			sIt != currReachOutSet.end(); sIt++) {
		    std::string varName = sIt->first;
		    if (rd.reachOutSet[currBlockID].find(varName) ==
			rd.reachOutSet[currBlockID].end()) {
			// currReachOutSet contains a new var-def.
#ifdef DEBUG
			llvm::outs() << "DEBUG: reachOutSet for var " << varName 
				     << " is empty\n";
#endif
			outChanged = true;
			break;
		    } else {
			if (sIt->second.size() !=
			    rd.reachOutSet[currBlockID][varName].size()) {
#ifdef DEBUG
			    llvm::outs() << "DEBUG: reachOutSet for var size() "
					 << "!= currReachOutSet[var].size()\n";
#endif
			    outChanged = true;
			    break;
			} else {
			    std::set<std::pair<int, std::string> >::iterator cIt
				= sIt->second.begin();
			    std::set<std::pair<int, std::string> >::iterator rIt 
				= rd.reachOutSet[currBlockID][varName].begin();
			    for (; cIt != sIt->second.end() && rIt !=
				    rd.reachOutSet[currBlockID][varName].end();
				    cIt++,rIt++) {
				if (cIt->first != rIt->first || 
				    cIt->second.compare(rIt->second) != 0) {
#ifdef DEBUG
				    llvm::outs() << "DEBUG: Different defs for var " 
						 << varName << "\n";
#endif
				    outChanged = true;
				    break;
				}
			    }
			    if (outChanged) break;
			}
		    }
		}
	    }
	    if (outChanged) {
		out[currBlockID] = true;
		rd.reachOutSet.erase(rd.reachOutSet.find(currBlockID));
		rd.reachOutSet[currBlockID] = currReachOutSet;
#ifdef DEBUG
		llvm::outs() << "DEBUG: out changed\n";
		llvm::outs() << "DEBUG: reachOutSet updated\n";
#endif
	    } else {
		out[currBlockID] = false;
#ifdef DEBUG
		llvm::outs() << "DEBUG: out not changed\n";
#endif
	    }
	}

	// Add successors of currBlock to workList
	for (CFGBlock::succ_iterator sIt = currBlock->succ_begin(); sIt !=
		currBlock->succ_end(); sIt++) {
	    CFGBlock* succBlock = sIt->getReachableBlock();
	    if (processedBlocks.find(succBlock->getBlockID()) ==
		    processedBlocks.end()) {
		workList.push(succBlock->getBlockID());
#ifdef DEBUG
		llvm::outs() << "DEBUG: Adding successor " << succBlock->getBlockID()
			     << " to workList\n";
#endif
	    }
	}

#if 0
	llvm::outs() << "DEBUG: out array:\n";
	for (unsigned i = 0; i < totalBlocks; i++)
	    llvm::outs() << "out[" << i << "] = " << (out[i]? "true": "false")
			 << "\n";
#endif
    }

#if 0
    llvm::outs() << "========================================================\n";
    llvm::outs() << "DEBUG: reachOutSet:\n";
    for (std::map<unsigned, std::map<std::string, std::set<std::pair<int,
	    std::string> > > >::iterator rIt = rd.reachOutSet.begin(); rIt !=
	    rd.reachOutSet.end(); rIt++) {
	llvm::outs() << "DEBUG: Basic block " << rIt->first << ":\n";
	for (std::map<std::string, std::set<std::pair<int, std::string> >
		>::iterator vIt = rIt->second.begin(); vIt != rIt->second.end();
		vIt++) {
	    llvm::outs() << "DEBUG: Var: " << vIt->first << ": \n";
	    llvm::outs() << "DEBUG: Defs:\n";
	    for (std::set<std::pair<int, std::string> >::iterator sIt =
		    vIt->second.begin(); sIt != vIt->second.end(); sIt++) {
		llvm::outs() << "(" << sIt->first << ", " << sIt->second 
			     << ")   ";
	    }
	    llvm::outs() << "\n";
	}
    }
#endif
}
#endif

void FunctionAnalysis::findBackEdgesInCFG(std::unique_ptr<CFG> &cfg, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering FunctionAnalysis::findBackEdgesInCFG\n";
#endif
    rv = true;

    // Compute dominator tree of CFG
    llvm::DominatorTreeBase<CFGBlock> *dTree = new
	llvm::DominatorTreeBase<CFGBlock>(false);
    dTree->recalculate(*cfg);

#ifdef DEBUG
    dTree->print(llvm::outs());
#endif

    std::queue<unsigned> workList;
    workList.push(cfg->getEntry().getBlockID());

    std::set<unsigned> processedBlocks;
    while (!workList.empty()) {
	unsigned currBlockID = workList.front();
	workList.pop();
	processedBlocks.insert(currBlockID);
#ifdef DEBUG
	llvm::outs() << "DEBUG: Processing block " << currBlockID << "\n";
#endif

	CFGBlock* currBlock = getBlockFromID(cfg, currBlockID);

	for (CFGBlock::succ_iterator sIt = currBlock->succ_begin(); sIt !=
		currBlock->succ_end(); sIt++) {
	    CFGBlock* succBlock = sIt->getReachableBlock();
	    unsigned succBlockID = succBlock->getBlockID();
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Processing edge (" << currBlockID << ", "
			 << succBlockID << ")\n";
#endif

	    // If succBlock dominates currBlock, this is a back edge
	    if (dTree->properlyDominates(succBlock, currBlock)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: " << succBlockID << " dominates " 
			     << currBlockID << "\n";
#endif
		std::pair<unsigned, unsigned> edge;
		edge.first = currBlockID;
		edge.second = succBlockID;

		// Check if this edge shares the destination node (or the loop
		// header) with another.
		for (std::set<std::pair<unsigned, unsigned> >::iterator eIt =
			backEdges.begin(); eIt != backEdges.end(); eIt++) {
		    if (eIt->second == edge.second && eIt->first != edge.first) {
			llvm::outs() << "ERROR: Two back edges with same loop "
				     << "header: (" << edge.first << ", " 
				     << edge.second << ") and (" << eIt->first
				     << ", " << eIt->second << ")\n";
			rv = false;
			return;
		    }
		}
#ifdef DEBUG
		llvm::outs() << "DEBUG: Adding back edge (" << edge.first << ","
			     << edge.second << ")\n";
#endif
		backEdges.insert(edge);
	    }
	    if (processedBlocks.find(succBlockID) ==
		    processedBlocks.end()) {
		workList.push(succBlockID);
	    }
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Back Edges in CFG\n";
    for (std::set<std::pair<unsigned, unsigned> >::iterator eIt =
	    backEdges.begin(); eIt != backEdges.end(); eIt++) {
	llvm::outs() << "(" << eIt->first << ", " << eIt->second << ") ";
    }
    llvm::outs() << "\n";
#endif
}

void FunctionAnalysis::computeAllLoops(std::unique_ptr<CFG> &cfg, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering FunctionAnalysis::computeAllLoops()\n";
    llvm::outs() << "DEBUG: BackEdges: " << backEdges.size() << ": ";
    for (std::set<std::pair<unsigned, unsigned> >::iterator eIt = 
	    backEdges.begin(); eIt != backEdges.end(); eIt++)
	llvm::outs() << "(" << eIt->first << ", " << eIt->second << ") ";
    llvm::outs() << "\n";
#endif

    rv = true;
    // Compute loops for each back edge in backEdges
    int i = 0;
    for (std::set<std::pair<unsigned, unsigned> >::iterator eIt =
	    backEdges.begin(); eIt != backEdges.end(); eIt++) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Back edge (" << eIt->first << ", " 
		     << eIt->second << "): " << ++i << "\n";
#endif
	std::set<unsigned> loop = findLoopFromBackEdge(cfg, eIt->first,
	    eIt->second);

	std::pair<unsigned, unsigned> edge;
	edge.first = eIt->first;
	edge.second = eIt->second;

	bool foundEdge = false;
	std::map<std::pair<unsigned, unsigned>, std::set<unsigned> >::iterator mIt;
	for (mIt = edgesToLoop.begin(); mIt != edgesToLoop.end(); mIt++) {
	    if (mIt->first.first == edge.first && mIt->first.second ==
		    edge.second) {
		foundEdge = true;
		break;
	    }
	}

	if (foundEdge)
	    edgesToLoop.erase(mIt);

	std::pair<std::map<std::pair<unsigned, unsigned>, std::set<unsigned>
	    >::iterator, bool> ret;
	ret = edgesToLoop.insert(std::pair<std::pair<unsigned, unsigned>,
		std::set<unsigned> >(edge, loop));
	if (ret.second == false) {
	    llvm::outs() << "ERROR: Entry already exists for edge (" 
			 << edge.first << ", " << edge.second 
			 << ") in edgesToLoop\n";
	    rv = false;
	    return;
	}

#ifdef DEBUG
	llvm::outs() << "DEBUG: Loop for back edge (" << edge.first << ", "
		     << edge.second << "): ";
	for (std::set<unsigned>::iterator lIt = loop.begin(); lIt != loop.end();
		lIt++) 
	    llvm::outs() << *lIt << " ";
	llvm::outs() << "\n";
#endif

	// Check if the body of the loop contains a break statement
	//bool breakFound = false;
	std::map<unsigned, std::set<unsigned> > blocksWithBreak =
	    getBlocksContainingBreakInLoop(function->cfg, loop, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While getBlocksContainingBreakInLoop()\n";
	    return;
	}

	// Find loop details of the current loop
	LoopDetails* ld = getIndexVariableForLoop(cfg, loop, edge, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While getIndexVariableForLoop()\n";
	    llvm::outs() << "ERROR: Inside computeAllLoops()\n";
	    rv = false;
	    return;
	} else {
#ifdef DEBUG
	    llvm::outs() << "rv is true after getIndexVariableForLoop()\n";
#endif
	}
	ld->hasBreak = (blocksWithBreak.size() != 0) ? true : false;
	//ld->blockIDsOfAllBreaksInLoopToBlocksGuardingThem = blocksWithBreak;
	ld->blockIDsOfAllBreaksInLoopToBlocksGuardingThem.insert(blocksWithBreak.begin(),
	    blocksWithBreak.end());
#ifdef DEBUG
	llvm::outs() << "DEBUG: After getIndexVariableForLoop()\n";
#endif
	// Set backedge in ld
	ld->backEdge.first = edge.first;
	ld->backEdge.second = edge.second;

	foundEdge = false;
	std::map<std::pair<unsigned, unsigned>, LoopDetails*>::iterator lIt;
	for (lIt = edgesToLoopDetails.begin(); lIt != edgesToLoopDetails.end();
		lIt++) {
	    if (lIt->first.first == edge.first && lIt->first.second ==
		    edge.second) {
		foundEdge = true;
		break;
	    }
	}
	if (foundEdge) edgesToLoopDetails.erase(lIt);

	std::pair<std::map<std::pair<unsigned, unsigned>,
	    LoopDetails*>::iterator, bool> r;
	r = edgesToLoopDetails.insert(std::pair<std::pair<unsigned, unsigned>,
		LoopDetails*>(edge, ld));
	if (r.second == false) {
	    llvm::outs() << "ERROR: Entry already exists for edge ("
			 << edge.first << ", " << edge.second
			 << ") in edgesToLoopDetails\n";
	    rv = false;
	    return;
	}
	allLoopsInFunction.push_back(ld);
#ifdef DEBUG
	llvm::outs() << "DEBUG: Back edges left: " 
		     << std::distance(eIt, backEdges.end()) << "\n";
#endif
    }

    // Set loop Index init val for all loops
    for (std::map<std::pair<unsigned, unsigned>, LoopDetails*>::iterator lIt =
	    edgesToLoopDetails.begin(); lIt != edgesToLoopDetails.end(); lIt++) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Before setLoopIndexInitValDef():\n";
	lIt->second->print();
#endif
	lIt->second->setLoopIndexInitValDef(this, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While setLoopIndexInitValDef()\n";
	    return;
	}
#ifdef DEBUG
	llvm::outs() << "DEBUG: After setLoopIndexInitValDef():\n";
	lIt->second->print();
#endif
    }

    // For each loop, find the loops nested within it.
    for (std::vector<LoopDetails*>::iterator l1It = allLoopsInFunction.begin();
	    l1It != allLoopsInFunction.end(); l1It++) {
	unsigned loop1HeaderBlock = (*l1It)->loopHeaderBlock;
	for (std::vector<LoopDetails*>::iterator l2It =
		allLoopsInFunction.begin(); l2It != 
		allLoopsInFunction.end(); l2It++) {
	    unsigned loop2HeaderBlock = (*l2It)->loopHeaderBlock;
	    // Skip if it is the same loop
	    if (loop1HeaderBlock == loop2HeaderBlock) continue;

	    if ((*l1It)->loopStartLine <= (*l2It)->loopStartLine &&
		(*l2It)->loopEndLine <= (*l1It)->loopEndLine) {
		(*l1It)->loopsNestedWithin.push_back(*l2It);
	    }
	}

	// Also check if the break blocks identified are actually within the
	// loop
	std::vector<unsigned> blocksToErase;
	for (std::map<unsigned, std::set<unsigned> >::iterator bIt =
		(*l1It)->blockIDsOfAllBreaksInLoopToBlocksGuardingThem.begin(); bIt != 
		(*l1It)->blockIDsOfAllBreaksInLoopToBlocksGuardingThem.end(); bIt++) {
	    unsigned breakBlock = bIt->first;
	    if (blockToLines.find(breakBlock) == blockToLines.end()) {
		llvm::outs() << "ERROR: Cannot find entry for block " << breakBlock
			     << " in blockToLines\n";
		rv = false;
		return;
	    }
	    if ((*l1It)->loopStartLine <= blockToLines[breakBlock].first && 
		blockToLines[breakBlock].second <= (*l1It)->loopEndLine)
		continue;
	    blocksToErase.push_back(breakBlock);
	}

	for (std::vector<unsigned>::iterator bIt = blocksToErase.begin(); bIt != 
		blocksToErase.end(); bIt++) {
	    (*l1It)->blockIDsOfAllBreaksInLoopToBlocksGuardingThem.erase(*bIt);
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: allLoopsInFunction:\n";
    int l = 0;
    for (std::vector<LoopDetails*>::iterator lIt = allLoopsInFunction.begin();
	    lIt != allLoopsInFunction.end(); lIt++) {
	llvm::outs() << "DEBUG: Loop " << ++l << ":\n";
	(*lIt)->print();
    }
#endif
}

std::set<unsigned> FunctionAnalysis::findLoopFromBackEdge(
std::unique_ptr<CFG> &cfg, unsigned src, unsigned dest) {

    std::stack<unsigned> workList;
    std::set<unsigned> loop;
    loop.insert(dest);

    if (loop.find(src) == loop.end()) {
	loop.insert(src);
	workList.push(src);
    }

    while (!workList.empty()) {
	unsigned top = workList.top();
	workList.pop();

	CFGBlock* currBlock = getBlockFromID(cfg, top);
	for (CFGBlock::pred_iterator pIt = currBlock->pred_begin(); pIt !=
		currBlock->pred_end(); pIt++) {
	    unsigned predBlockID = pIt->getReachableBlock()->getBlockID();
	    if (loop.find(predBlockID) == loop.end()) {
		loop.insert(predBlockID);
		workList.push(predBlockID);
	    }
	}
    }

    return loop;
}

std::map<unsigned, std::set<unsigned> > 
FunctionAnalysis::getBlocksContainingBreakInLoop(
std::unique_ptr<CFG> &cfg, std::set<unsigned> loop, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering FunctionAnalysis::getBlocksContainingBreakInLoop()\n";
#endif
    rv = true;
    std::map<unsigned, std::set<unsigned> > breakBlocks;

    for (std::set<unsigned>::iterator lIt = loop.begin(); lIt != loop.end();
	    lIt++) {
	CFGBlock* currBlock = getBlockFromID(cfg, *lIt);
	if (!currBlock) {
	    llvm::outs() << "ERROR: Cannot get block from ID: " << *lIt << "\n";
	    rv = false;
	    return breakBlocks;
	}

	for (CFGBlock::succ_iterator sIt = currBlock->succ_begin(); sIt
		!= currBlock->succ_end(); sIt++) {
            CFGBlock* succBlock = sIt->getReachableBlock();
            unsigned succBlockID = succBlock->getBlockID();
	    // If the successor is not in the loop, check if it has break
	    if (loop.find(succBlockID) == loop.end()) {
		for (CFGBlock::iterator eIt = succBlock->begin(); eIt != 
			succBlock->end(); eIt++) {
		    // Look only at those CFGElements that are Statements.
		    if (eIt->getKind() == CFGElement::Kind::Statement) {
			CFGStmt currCFGStmt = eIt->castAs<CFGStmt>();
			const Stmt* currStmt = currCFGStmt.getStmt();
			if (!currStmt) {
			    llvm::outs() << "ERROR: Cannot get Stmt from CFGStmt\n";
			    rv = false;
			    return breakBlocks;
			}
			BreakStmtVisitor bv(SM);
			bv.TraverseStmt(const_cast<Stmt*>(currStmt));
			if (bv.breakStmtFound) {
			    if (breakBlocks.find(succBlockID) == breakBlocks.end()) {
				std::set<unsigned> guardBlocks;
				guardBlocks.insert(*lIt);
				breakBlocks[succBlockID] = guardBlocks;
			    } else {
				breakBlocks[succBlockID].insert(*lIt);
			    }
			}
		    }
		}
	    }
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: breakBlocks: ";
    for (std::map<unsigned, std::set<unsigned> >::iterator bIt = 
	    breakBlocks.begin(); bIt != breakBlocks.end(); bIt++) {
	llvm::outs() << "Break in block: " << bIt->first << " - guards: ";
	for (std::set<unsigned>::iterator gIt = bIt->second.begin(); gIt !=
		bIt->second.end(); gIt++) {
	    llvm::outs() << *gIt << " ";
	}
	llvm::outs() << "\n";
    }
    llvm::outs() << "\n";
#endif

    return breakBlocks;
}

// loop - set of all basic blocks constituting the loop
// head - the basic block that is the head of the loop
LoopDetails* FunctionAnalysis::getIndexVariableForLoop(
std::unique_ptr<CFG> &cfg, std::set<unsigned> loop, 
std::pair<unsigned, unsigned> backedge, 
    bool &rv) {

    rv = true;
    LoopDetails* ld = new LoopDetails;
    unsigned head = backedge.second;
    unsigned tail = backedge.first;

    // Find basic induction variables in loop
    // Traverse through the blocks constituting the loop and pick up all
    // variables whose definitions are of the form x += c or x -= c

    std::queue<unsigned> workList;
    workList.push(head);

    std::set<unsigned> processedBlocks;
    //std::set<std::string> varsDefinedInLoop;
    std::vector<VarDetails> varsDefinedInLoop;

    while (!workList.empty()) {
	unsigned currBlockID = workList.front();
	workList.pop();

	processedBlocks.insert(currBlockID);

	CFGBlock* currBlock = getBlockFromID(cfg, currBlockID);

	for (CFGBlock::iterator eIt = currBlock->begin(); eIt != currBlock->end();
		eIt++) {
	    // Look only at those CFGElements that are Statements.
	    if (eIt->getKind() == CFGElement::Kind::Statement) {
		CFGStmt currCFGStmt = eIt->castAs<CFGStmt>();
		const Stmt* currStmt = currCFGStmt.getStmt();
		if (!currStmt) {
		    llvm::outs() << "ERROR: Cannot get Stmt from CFGStmt\n";
		    rv = false;
		    return ld;
		}
#ifdef DEBUG
		llvm::outs() << "DEBUG: currStmt: " << Helper::prettyPrintStmt(currStmt) << "\n";
#endif

		LoopDetails::inductionVar iv;
		iv.toDelete = false; // Initializing this field

		std::pair<int, int> lineAndStep;
		int lineNum = SM->getExpansionLineNumber(currStmt->getLocStart());
		if (isa<BinaryOperator>(currStmt)) {
		    const BinaryOperator* currStmtAsBO =
			dyn_cast<BinaryOperator>(currStmt);
		    if (!(currStmtAsBO->isAssignmentOp()))
			continue;
		    if (!(currStmtAsBO->isCompoundAssignmentOp())) {
			const Expr* lhsExpr = currStmtAsBO->getLHS();
			const Expr* rhsExpr = currStmtAsBO->getRHS();

			//std::string lhsVarName = "";
			//int varDeclLine;
			VarDetails lhsVar;
			if (isa<DeclRefExpr>(lhsExpr)) {
			    //lhsVarName = Helper::prettyPrintExpr(lhsExpr);

			    //const DeclRefExpr* lhsDRE =
				//dyn_cast<DeclRefExpr>(lhsExpr);
			    //const VarDecl* lhsVD =
				//dyn_cast<VarDecl>(lhsDRE->getDecl());
			    //varDeclLine =
				//SM->getExpansionLineNumber(lhsVD->getLocStart());
			    lhsVar = Helper::getVarDetailsFromExpr(SM, lhsExpr, rv);
			    if (!rv) {
				llvm::outs() << "ERROR: Cannot get VarDetails from "
					     << Helper::prettyPrintExpr(lhsExpr) 
					     << "\n";
				return ld;
			    }
			} else
			    continue;

			// Check if rhsExpr is lhs +/- constant
			if (!(isa<BinaryOperator>(rhsExpr))) {
			    //varsDefinedInLoop.insert(lhsVarName);
			    varsDefinedInLoop.push_back(lhsVar);
			    continue;
			}

			const BinaryOperator* rhsExprAsBO = 
			    dyn_cast<BinaryOperator>(rhsExpr);
			if (!rhsExprAsBO->isAdditiveOp()) {
			    //varsDefinedInLoop.insert(lhsVarName);
			    varsDefinedInLoop.push_back(lhsVar);
			    continue;
			}

			const Expr* lExpr =
			    rhsExprAsBO->getLHS()->IgnoreParenCasts();
			const Expr* rExpr =
			    rhsExprAsBO->getRHS()->IgnoreParenCasts();
			if (isa<DeclRefExpr>(lExpr) && isa<IntegerLiteral>(rExpr)) {
#if 0
			    if (lhsVarName.compare(Helper::prettyPrintExpr(lExpr))
				    != 0) {
				varsDefinedInLoop.insert(lhsVarName);
				continue;
			    }
#endif
			    VarDetails lVar = Helper::getVarDetailsFromExpr(SM,
				lExpr, rv);
			    if (!rv) {
				llvm::outs() << "ERROR: Cannot get VarDetails from "
					     << Helper::prettyPrintExpr(lExpr) << "\n";
				return ld;
			    }
			    if (!(lhsVar.equals(lVar))) {
				varsDefinedInLoop.push_back(lhsVar);
				continue;
			    }
			    //iv.varName = lhsVarName;
			    //iv.varDeclLine = varDeclLine;
			    iv.var = lhsVar.clone();
			    lineAndStep.first = lineNum;
			    lineAndStep.second =
				atoi(Helper::prettyPrintExpr(rExpr).c_str());
			    if (rhsExprAsBO->getOpcode() ==
				    BinaryOperatorKind::BO_Sub)
				lineAndStep.second = -(lineAndStep.second);
			    iv.inductionLinesAndSteps.push_back(lineAndStep);
#ifdef DEBUG
			    llvm::outs() << "DEBUG: Found induction var: ";
			    iv.var.print();
			    llvm::outs() << "\n";
#endif
			} else if (isa<IntegerLiteral>(lExpr) &&
			    isa<DeclRefExpr>(rExpr)) {
#if 0
			    if (lhsVarName.compare(Helper::prettyPrintExpr(rExpr))
				    != 0) {
				varsDefinedInLoop.insert(lhsVarName);
				continue;
			    }
#endif
			    VarDetails rVar = Helper::getVarDetailsFromExpr(SM,
				rExpr, rv);
			    if (!rv) {
				llvm::outs() << "ERROR: Cannot get VarDetails from "
					     << Helper::prettyPrintExpr(rExpr) << "\n";
				return ld;
			    }
			    if (!(lhsVar.equals(rVar))) {
				varsDefinedInLoop.push_back(lhsVar);
				continue;
			    }
			    //iv.varName = lhsVarName;
			    //iv.varDeclLine = varDeclLine;
			    iv.var = lhsVar.clone();
			    lineAndStep.first = lineNum;
			    lineAndStep.second =
				atoi(Helper::prettyPrintExpr(lExpr).c_str());
			    iv.inductionLinesAndSteps.push_back(lineAndStep);
#ifdef DEBUG
			    llvm::outs() << "DEBUG: Found induction var: ";
			    iv.var.print();
			    llvm::outs() << "\n";
#endif
			} else {
			    //varsDefinedInLoop.insert(lhsVarName);
			    varsDefinedInLoop.push_back(lhsVar);
			    continue;
			}
		    } else {
			const Expr* lhsExpr =
			    currStmtAsBO->getLHS()->IgnoreParenCasts();
			const Expr* rhsExpr =
			    currStmtAsBO->getRHS()->IgnoreParenCasts();

			if (currStmtAsBO->getOpcode() != BinaryOperatorKind::BO_AddAssign
				&& currStmtAsBO->getOpcode() != BinaryOperatorKind::BO_SubAssign)
			    continue;

			VarDetails lhsVar;
			if (isa<DeclRefExpr>(lhsExpr)) {
			    lhsVar = Helper::getVarDetailsFromExpr(SM, lhsExpr, rv);
			    if (!rv) {
				llvm::outs() << "ERROR: Cannot get VarDetails from "
					     << Helper::prettyPrintExpr(lhsExpr) 
					     << "\n";
				return ld;
			    }
			} else
			    continue;

			if (isa<IntegerLiteral>(rhsExpr)) {
			    iv.var = lhsVar.clone();
			    lineAndStep.first = lineNum;
			    lineAndStep.second =
				atoi(Helper::prettyPrintExpr(rhsExpr).c_str());
			    if (currStmtAsBO->getOpcode() ==
				    BinaryOperatorKind::BO_SubAssign)
				lineAndStep.second = -(lineAndStep.second);
			    iv.inductionLinesAndSteps.push_back(lineAndStep);
#ifdef DEBUG
			    llvm::outs() << "DEBUG: Found induction var: ";
			    iv.var.print();
			    llvm::outs() << "\n";
#endif
			} else {
			    //varsDefinedInLoop.insert(lhsVarName);
			    varsDefinedInLoop.push_back(lhsVar);
			    continue;
			}
		    }
		} else if (isa<UnaryOperator>(currStmt)) {
		    const UnaryOperator* currStmtAsUO =
			dyn_cast<UnaryOperator>(currStmt);
		    if (!(currStmtAsUO->isIncrementDecrementOp()))
			continue;
		    Expr* subExpr =
			currStmtAsUO->getSubExpr()->IgnoreParenCasts();
		    if (!subExpr) {
			llvm::outs() << "ERROR: Cannot get subexpr from UnaryOperator\n";
			rv = false;
			return ld;
		    }

		    if (isa<DeclRefExpr>(subExpr)) {
			//std::string varName =
			    //Helper::prettyPrintExpr(subExpr);

			//const DeclRefExpr* subDRE =
			    //dyn_cast<DeclRefExpr>(subExpr);
			//const VarDecl* subVD = 
			    //dyn_cast<VarDecl>(subDRE->getDecl());
			//int varDeclLine =
			    //SM->getExpansionLineNumber(subVD->getLocStart());
			VarDetails currVar = Helper::getVarDetailsFromExpr(SM, subExpr, rv);
			if (!rv) {
			    llvm::outs() << "ERROR: Cannot get VarDetails from "
					 << Helper::prettyPrintExpr(subExpr) << "\n";
			    return ld;
			}

			//iv.varName = varName;
			//iv.varDeclLine = varDeclLine;
			iv.var = currVar.clone();
			lineAndStep.first = lineNum;
#ifdef DEBUG
			llvm::outs() << "DEBUG: Found induction var: ";
			iv.var.print();
			llvm::outs() << "\n";
#endif
			if (currStmtAsUO->isIncrementOp()) {
			    lineAndStep.second = 1;
			    iv.inductionLinesAndSteps.push_back(lineAndStep);
			} else if (currStmtAsUO->isDecrementOp()) {
			    lineAndStep.second = -1;
			    iv.inductionLinesAndSteps.push_back(lineAndStep);
			} else
			    continue;
		    } else
			continue;
		} else
		    continue;

#ifdef DEBUG
		llvm::outs() << "DEBUG: varsDefinedInLoop:\n";
		for (std::vector<VarDetails>::iterator vIt =
			varsDefinedInLoop.begin(); vIt !=
			varsDefinedInLoop.end(); vIt++) {
		    vIt->print();
		    llvm::outs() << "\n";
		}
#endif

		// Remove vars identified as not induction vars from our list
		for (std::vector<struct LoopDetails::inductionVar>::iterator iIt 
			= ld->inductionVars.begin(); iIt !=
			ld->inductionVars.end(); iIt++) {
		    //if (varsDefinedInLoop.find(iIt->varName) !=
			    //varsDefinedInLoop.end())
			//ld.inductionVars.erase(iIt);
		    for (std::vector<VarDetails>::iterator vIt =
			    varsDefinedInLoop.begin(); vIt != 
			    varsDefinedInLoop.end(); vIt++) {
			if (iIt->var.equals(*vIt)) {
			    iIt->toDelete = true;
			    break;
			}
		    }
		}
		ld->inductionVars.erase(
		    std::remove_if(ld->inductionVars.begin(), ld->inductionVars.end(),
		    [](LoopDetails::inductionVar v){ return v.toDelete; }), 
		    ld->inductionVars.end());

		//if (varsDefinedInLoop.find(iv.varName) !=
			//varsDefinedInLoop.end())
		    //continue;
		bool foundVar = false;
		for (std::vector<VarDetails>::iterator vIt =
			varsDefinedInLoop.begin(); vIt != 
			varsDefinedInLoop.end(); vIt++) {
		    if (vIt->equals(iv.var)) {
			foundVar = true;
#ifdef DEBUG
			llvm::outs() << "DEBUG: Induction var: ";
			iv.var.print();
			llvm::outs() << " present in varsDefinedInLoop\n";
#endif
			break;
		    }
		}
		if (foundVar) continue;

		bool varFound = false;
		bool detailFound = false;
		for (std::vector<struct LoopDetails::inductionVar>::iterator iIt
			= ld->inductionVars.begin(); iIt !=
			ld->inductionVars.end(); iIt++) {
		    //if (iIt->varName.compare(iv.varName) != 0)
		    if (!(iIt->var.equals(iv.var)))
			continue;

		    varFound = true;
		    detailFound = false;
		    for (std::vector<std::pair<int, int> >::iterator lIt =
			    iIt->inductionLinesAndSteps.begin(); lIt != 
			    iIt->inductionLinesAndSteps.end(); lIt++) {
			if (lIt->first == iv.inductionLinesAndSteps[0].first && 
			    lIt->second == iv.inductionLinesAndSteps[0].second)
			{
			    detailFound = true;
			    break;
			}
		    }
		    if (!detailFound)
			iIt->inductionLinesAndSteps.push_back(lineAndStep);
		    break;
		}
		if (!varFound) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Inserting induction var: ";
		    iv.var.print();
		    llvm::outs() << "\n";
#endif
		    ld->inductionVars.push_back(iv);
		}
	    }
	}
	
	// Add successor of current block to workList.
	for (CFGBlock::succ_iterator sIt = currBlock->succ_begin(); sIt !=
		currBlock->succ_end(); sIt++) {
	    CFGBlock* succBlock = sIt->getReachableBlock();
	    unsigned succBlockID = succBlock->getBlockID();

	    if (loop.find(succBlockID) != loop.end()) {
		if (processedBlocks.find(succBlockID) == processedBlocks.end())
		    workList.push(succBlockID);
	    }
	}
    }

#ifdef DEBUG
    llvm::outs() << "Induction vars: " << ld->inductionVars.size() << "\n";
    for (std::vector<struct LoopDetails::inductionVar>::iterator iIt =
	    ld->inductionVars.begin(); iIt != ld->inductionVars.end(); iIt++) {
	//llvm::outs() << "Var: " << iIt->varName << " (decl " << iIt->varDeclLine 
		     //<< ") ";
	llvm::outs() << "Var: ";
	iIt->var.print();
	llvm::outs() << "\n";
	for (std::vector<std::pair<int, int> >::iterator lIt =
		iIt->inductionLinesAndSteps.begin(); lIt !=
		iIt->inductionLinesAndSteps.end(); lIt++)
	    llvm::outs() << "(" << lIt->first << ", " << lIt->second << ") ";
	llvm::outs() << "\n";
    }
#endif

    // We found all the induction vars in this loop. Now we need to find the
    // index variable. Assumption: Index variable is the one that occurs in the
    // loop exit condition. Find all the induction vars that occur in the loop
    // exit condition.
    // Loop exit condition is in the head of the loop.
    bool isDoWhileLoop = false;
    unsigned headerBlock = head;
    CFGBlock* headOfLoop = getBlockFromID(cfg, head);
    if (!headOfLoop) {
	llvm::outs() << "ERROR: Cannot obtain loop header from block " << head << "\n";
	rv = false;
	return ld;
    }
    Stmt* exitCondition = headOfLoop->getTerminatorCondition();
    if (!exitCondition) {
	clang::LangOptions lo;
	lo.CPlusPlus = true;
	headOfLoop->print(llvm::outs(), cfg.get(), lo, false);
	llvm::outs() << "DEBUG: Terminator condition is NULL\n"
		     << "Checking the tail of the loop\n";
	// Hacking to handle do-while loop
	isDoWhileLoop = true;
	headOfLoop = getBlockFromID(cfg, tail);
	if (!headOfLoop) {
	    llvm::outs() << "ERROR: Cannot obtain loop header from block " 
			 << tail << "\n";
	    rv = false;
	    return ld;
	}
	headOfLoop->print(llvm::outs(), cfg.get(), lo, false);
	exitCondition = headOfLoop->getTerminatorCondition();
	headerBlock = tail;
	if (!exitCondition) {
	    // Get the predecessor of tail.
	    if (headOfLoop->pred_size() != 1) {
		llvm::outs() << "ERROR: Cannot get terminator condition of loop "
			     << "with backedge (" << backedge.first << ", "
			     << backedge.second << ")\n";
		rv = false;
		return ld;
	    }
	    CFGBlock* predecessor = *(headOfLoop->pred_begin());
	    if (!predecessor) {
		llvm::outs() << "ERROR: Predecessor of block " << tail << " is NULL\n";
		rv = false;
		return ld;
	    }
	    predecessor->print(llvm::outs(), cfg.get(), lo, false);
	    exitCondition = predecessor->getTerminatorCondition();
	    if (!exitCondition) {
		llvm::outs() << "ERROR: Terminator condition is NULL\n";
		rv = false;
		return ld;
	    }
	    headOfLoop = predecessor;
	    headerBlock = predecessor->getBlockID();
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Predecessor block is " << headerBlock << "\n";
#endif
	}
    }
    ld->isDoWhileLoop = isDoWhileLoop;
    ld->loopHeaderBlock = headerBlock;
    Expr* exitCondExpr;
    if (isa<Expr>(exitCondition)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Exit condition is Expr\n";
#endif
	exitCondExpr = dyn_cast<Expr>(exitCondition)->IgnoreImpCasts();
    } else {
	llvm::outs() << "ERROR: Loop exit condition is not Expr\n";
	rv = false;
	return ld;
    }

    int exitConditionLine =
	SM->getExpansionLineNumber(exitCondition->getLocStart());

    GetVarVisitor gv(SM);
    gv.clear();
    gv.TraverseStmt(exitCondition);
    if (gv.error) {
	rv = false;
	return ld;
    }
#ifdef DEBUG
    gv.printVars();
#endif

    std::vector<struct LoopDetails::inductionVar> inductionVarsInLoopExit;
    // Find the induction vars in the set of vars in the loop exit condition
    for (std::vector<struct LoopDetails::inductionVar>::iterator iIt =
	    ld->inductionVars.begin(); iIt != ld->inductionVars.end(); iIt++) {
	if (gv.isScalarVarPresent(iIt->var.varName, iIt->var.varDeclLine))
	    inductionVarsInLoopExit.push_back(*iIt);
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Induction vars in loop exit condition: " 
		 << inductionVarsInLoopExit.size() << "\n";
    for (std::vector<struct LoopDetails::inductionVar>::iterator iIt =
	    inductionVarsInLoopExit.begin(); iIt != inductionVarsInLoopExit.end(); 
	    iIt++) {
	//llvm::outs() << "Var: " << iIt->varName << " (decl " << iIt->varDeclLine 
		     //<< ") ";
	llvm::outs() << "Var: ";
	iIt->var.print();
	llvm::outs() << "\n";
	for (std::vector<std::pair<int, int> >::iterator lIt =
		iIt->inductionLinesAndSteps.begin(); lIt !=
		iIt->inductionLinesAndSteps.end(); lIt++)
	    llvm::outs() << "(" << lIt->first << ", " << lIt->second << ") ";
	llvm::outs() << "\n";
    }
#endif

    if (inductionVarsInLoopExit.size() == 0) {
	llvm::outs() << "ERROR: Could not find induction var in loop exit condition: "
		     << Helper::prettyPrintStmt(exitCondition) << "\n";
	rv = false;
	return ld;
    } else if (inductionVarsInLoopExit.size() > 1) {
	llvm::outs() << "ERROR: Loop exit condition has more than one induction var\n";
	llvm::outs() << Helper::prettyPrintStmt(exitCondition) << "\n";
	rv = false;
	return ld;
    } else {
	// Found the loop index var
	struct LoopDetails::inductionVar iv = inductionVarsInLoopExit[0];

	// Check if this var is updated from multiple locations of the loop. If
	// so, this might not be the loop index var
	if (iv.inductionLinesAndSteps.size() > 1) {
	    llvm::outs() << "ERROR: Identified loop index var " << iv.var.varName 
			 << " is updated at multiple locations in the loop\n";
	    rv = false;
	    return ld;
	} else if (iv.inductionLinesAndSteps.size() == 0) {
	    llvm::outs() << "ERROR: Identified loop index var " << iv.var.varName 
			 << " is not updated in the loop\n";
	    rv = false;
	    return ld;
	}

	// TODO: Set all fields of VarDetails for loopIndexVar.
	//ld.loopIndexVar.varName = iv.varName;
	//ld.loopIndexVar.varDeclLine = iv.varDeclLine;
	ld->loopIndexVar = iv.var.clone();
	ld->loopStep = iv.inductionLinesAndSteps[0].second;
	ld->loopIndexVar.setVarID();

	// Find the loop start and end lines
	CFGTerminator termLoop = headOfLoop->getTerminator();
	Stmt* loopStmt = termLoop.getStmt();
	if (isa<ForStmt>(loopStmt) || isa<WhileStmt>(loopStmt) ||
		isa<DoStmt>(loopStmt)) {
	    ld->loopStartLine =
		SM->getExpansionLineNumber(loopStmt->getLocStart());
	    ld->loopEndLine = 
		SM->getExpansionLineNumber(loopStmt->getLocEnd());
	} else {
	    llvm::outs() << "ERROR: Loop stmt identified from terminator of "
			 << "CFBlock is not ForStmt/WhileStmt/DoStmt\n";
	    llvm::outs() << Helper::prettyPrintStmt(loopStmt) << "\n";
	    rv = false;
	    return ld;
	}

	// Find init val and final val of the loop index var
	// Init Val: Get reaching definitions of loop index var to head of loop.
	// Pick the one from outside the loop. If there are multiple defs, then
	// fail.
	std::set<Definition> defs;
	if (rd.reachInSet.find(head) == rd.reachInSet.end()) {
	    llvm::outs() << "ERROR: Cannot obtain reachInSet for head of loop\n";
	    rv = false;
	    return ld;
	}

	if (rd.reachInSet[head].find(ld->loopIndexVar.varID) ==
		rd.reachInSet[head].end()) {
	    llvm::outs() << "ERROR: Cannot obtain reachInSet for var " 
			 << ld->loopIndexVar.varName << " for head of loop\n";
	    rv = false;
	    return ld;
	}

	defs = rd.reachInSet[head][ld->loopIndexVar.varID];
	std::set<Definition> defsOutsideLoop;
	for (std::set<Definition>::iterator dIt = defs.begin(); dIt != 
		defs.end(); dIt++) {
	    if (loop.find(dIt->blockID) == loop.end())
		defsOutsideLoop.insert(*dIt);
	}

	if (defsOutsideLoop.size() == 0) {
	    llvm::outs() << "ERROR: Cannot find any definition of loop index var "
			 << ld->loopIndexVar.varName << " outside the loop\n";
	    rv = false;
	    return ld;
	} else if (defsOutsideLoop.size() > 1) {
#if 0
	    llvm::outs() << "ERROR: More than one definition for loop index var "
			 << ld.loopIndexVar.varName << " outside the loop\n";
	    rv = false;
	    return ld;
#endif
	}
	for (std::set<Definition>::iterator idIt = defsOutsideLoop.begin();
		idIt != defsOutsideLoop.end(); idIt++) {
	    std::pair<Definition, SymbolicExpr*> p;
	    p.first = *idIt;
	    p.second = NULL;
	    ld->setOfLoopIndexInitValDefs.push_back(p);
	}

	//ld->loopIndexInitValDef = *(defsOutsideLoop.begin());

	// Final val of loop index var can be obtained from the structure of the
	// loop exit condition
	Definition finalValDef;
	bool exitConditionIntegerType = false;
	if (isa<BinaryOperator>(exitCondExpr)) {
	    BinaryOperator* binaryExpr = dyn_cast<BinaryOperator>(exitCondExpr);
	    if (!(binaryExpr->isRelationalOp()) && binaryExpr->getOpcode() != 
		    BinaryOperatorKind::BO_NE) {
		llvm::outs() << "DEBUG: Loop exit condition does not hav a "
			     << "relational operator\n";
		llvm::outs() << "DEBUG: Checking for integer-valued condition\n";
		const Type* exitCondExprType = Helper::getTypeOfExpr(binaryExpr);
		if (!exitCondExprType) {
		    llvm::outs() << "ERROR: NULL Type of ExitConditionExpr\n";
		    rv = false;
		    return ld;
		}
		if (!exitCondExprType->isIntegerType()) {
		    llvm::outs() << "ERROR: Exit condidiont expr is not relational or integer valued\n";
		    rv = false;
		    return ld;
		}
		exitConditionIntegerType = true;
	    }

	    clang::BinaryOperatorKind op = binaryExpr->getOpcode();
	    Expr* lhs = binaryExpr->getLHS()->IgnoreImpCasts();
	    Expr* rhs = binaryExpr->getRHS()->IgnoreImpCasts();

	    bool leftVarIndexVar = false;
	    std::string lhsVar, rhsVar;
	    if (isa<DeclRefExpr>(lhs)) {
		lhsVar = Helper::prettyPrintExpr(lhs);
		if (lhsVar.compare(ld->loopIndexVar.varName) == 0)
		    leftVarIndexVar = true;
	    }

	    if (isa<DeclRefExpr>(rhs)) {
		rhsVar = Helper::prettyPrintExpr(rhs);
		if (rhsVar.compare(ld->loopIndexVar.varName) == 0) {
		    if (leftVarIndexVar) {
			llvm::outs() << "ERROR: Found loop index var "
				     << ld->loopIndexVar.varName << " on both sides "
				     << "of loop exit condition: " 
				     << Helper::prettyPrintExpr(binaryExpr) << "\n";
			rv = false;
			return ld;
		    } else {
			leftVarIndexVar = false;
		    }
		}
	    }

	    finalValDef.lineNumber = exitConditionLine;
	    finalValDef.blockID = head;
	    finalValDef.throughBackEdge = false;

	    if (op == BinaryOperatorKind::BO_LT) {
		if (leftVarIndexVar) {
		    finalValDef.expression = rhs;
		    finalValDef.expressionStr = Helper::prettyPrintExpr(rhs);
		    //finalValDef.expressionStr = finalValDef.expressionStr + "-1";
		    ld->upperBound = true;
		    ld->strictBound = true;
		} else {
		    finalValDef.expression = lhs;
		    finalValDef.expressionStr = Helper::prettyPrintExpr(lhs);
		    //finalValDef.expression = finalValDef.expression + "+1";
		    ld->upperBound = false;
		    ld->strictBound = true;
		}
	    } else if (op == BinaryOperatorKind::BO_LE) {
		if (leftVarIndexVar) {
		    finalValDef.expression = rhs;
		    finalValDef.expressionStr = Helper::prettyPrintExpr(rhs);
		    ld->upperBound = true;
		    ld->strictBound = false;
		} else {
		    finalValDef.expression = lhs;
		    finalValDef.expressionStr = Helper::prettyPrintExpr(lhs);
		    ld->upperBound = false;
		    ld->strictBound = false;
		}
	    } else if (op == BinaryOperatorKind::BO_GT) {
		if (leftVarIndexVar) {
		    finalValDef.expression = rhs;
		    finalValDef.expressionStr = Helper::prettyPrintExpr(rhs);
		    //finalValDef.expression = finalValDef.expression + "+1";
		    ld->upperBound = false;
		    ld->strictBound = true;
		} else {
		    finalValDef.expression = lhs;
		    finalValDef.expressionStr = Helper::prettyPrintExpr(lhs);
		    //finalValDef.expression = finalValDef.expression + "-1";
		    ld->upperBound = true;
		    ld->strictBound = true;
		}
	    } else if (op == BinaryOperatorKind::BO_GE) {
		if (leftVarIndexVar) {
		    finalValDef.expression = rhs;
		    finalValDef.expressionStr = Helper::prettyPrintExpr(rhs);
		    ld->upperBound = false;
		    ld->strictBound = false;
		} else {
		    finalValDef.expression = lhs;
		    finalValDef.expressionStr = Helper::prettyPrintExpr(lhs);
		    ld->upperBound = true;
		    ld->strictBound = false;
		}
	    } else if (op == BinaryOperatorKind::BO_NE) {
                if (leftVarIndexVar) {
                    finalValDef.expression = rhs;
		    finalValDef.expressionStr = Helper::prettyPrintExpr(rhs);
		    ld->upperBound = true;
		    ld->strictBound = true;
		} else {
		    finalValDef.expression = lhs;
		    finalValDef.expressionStr = Helper::prettyPrintExpr(lhs);
		    ld->strictBound = true;
		    ld->upperBound = true;
		}
	    } else if (exitConditionIntegerType) {
		if (op != BinaryOperatorKind::BO_Add && op !=
			BinaryOperatorKind::BO_Sub) {
		    llvm::outs() << "ERROR: Loop exit condition is "
				 << "integer-valued, but the op is not + or -\n";
		    llvm::outs() << "Exit Condition: " 
				 << Helper::prettyPrintExpr(binaryExpr) 
				 << "\n";
		    rv = false;
		    return ld;
		}
		if (leftVarIndexVar) {
		    if (op == BinaryOperatorKind::BO_Add) {
			clang::Stmt::EmptyShell empty;
			UnaryOperator finalExpression(empty);
			finalExpression.setOpcode(UnaryOperatorKind::UO_Minus);
			finalExpression.setSubExpr(rhs);
			finalValDef.expression = &finalExpression;
			finalValDef.expressionStr =
			    Helper::prettyPrintExpr(&finalExpression);
			ld->upperBound = false;
			ld->strictBound = true;
		    } else {
			finalValDef.expression = rhs;
			finalValDef.expressionStr = 
			    Helper::prettyPrintExpr(rhs);
			ld->upperBound = false;
			ld->strictBound = true;
		    }
		} else {
		    if (op == BinaryOperatorKind::BO_Add) {
			clang::Stmt::EmptyShell empty;
			UnaryOperator finalExpression(empty);
			finalExpression.setOpcode(UnaryOperatorKind::UO_Minus);
			finalExpression.setSubExpr(lhs);
			finalValDef.expression = &finalExpression;
			finalValDef.expressionStr =
			    Helper::prettyPrintExpr(&finalExpression);
			ld->upperBound = false;
			ld->strictBound = true;
		    } else {
			finalValDef.expression = lhs;
			finalValDef.expressionStr = 
			    Helper::prettyPrintExpr(lhs);
			ld->upperBound = true;
			ld->strictBound = true;
		    }
		}
	    } else {
		llvm::outs() << "ERROR: loop exit condition operator is not "
			     << "LT, LE, GT or GE\n";
		llvm::outs() << "Op: " << op << "\n";
		rv = false;
		return ld;
	    }

	    ld->loopIndexFinalValDef = finalValDef;

	} else if (isa<UnaryOperator>(exitCondExpr)) {
	    // Only allowed expression is t--, where t is the loop index var
	    UnaryOperator* unaryExpr = dyn_cast<UnaryOperator>(exitCondExpr);
	    if (!(unaryExpr->isDecrementOp())) {
		rv = false;
		return ld;
	    }

	    Expr* subExpr = unaryExpr->getSubExpr();
	    if (!subExpr) {
		llvm::outs() << "ERROR: Cannot get subexpr from UnaryOperator\n";
		rv = false;
		return ld;
	    }

	    if (isa<DeclRefExpr>(subExpr)) {
		std::string varName =
		    Helper::prettyPrintExpr(subExpr);
		if (ld->loopIndexVar.varName.compare(varName) != 0) {
		    rv = false;
		    return ld;
		}

		finalValDef.lineNumber = exitConditionLine;
		finalValDef.blockID = head;
		finalValDef.expression = unaryExpr;
		finalValDef.expressionStr = "--";
		finalValDef.throughBackEdge = false;

		ld->loopIndexFinalValDef = finalValDef;
		ld->upperBound = false;
		ld->strictBound = true;
	    }
	} else if (isa<DeclRefExpr>(exitCondExpr)) {    
	    DeclRefExpr* dExpr = dyn_cast<DeclRefExpr>(exitCondExpr);
	    std::string varName = Helper::prettyPrintExpr(dExpr);
	    if (ld->loopIndexVar.varName.compare(varName) != 0) {
		rv = false;
		return ld;
	    }
	    finalValDef.lineNumber = exitConditionLine;
	    finalValDef.blockID = head;
	    finalValDef.expression = dExpr;
	    finalValDef.expressionStr = varName;
	    finalValDef.throughBackEdge = false;

	    ld->loopIndexFinalValDef = finalValDef;
	    ld->upperBound = false;
	    ld->strictBound = true;
	} else {
	    llvm::outs() << "ERROR: loop exit condition is neither "
			 << "BinaryOperator nor UnaryOperator\n";
	    rv = false;
	    return ld;
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: LoopDetails found:\n";
    ld->print();
#endif


    return ld;
}

void FunctionAnalysis::computeControlDependences(
std::unique_ptr<CFG> &cfg, const FunctionDecl* fd, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering FunctionAnalysis::computeControlDependences()\n";
#endif

    rv = true;
    // Construct augmented CFG with a special predicate node ENTRY
    BumpVectorContext c1, c2;
    CFGBlock::AdjacentBlock succ1(&(cfg->getEntry()), true);
    CFGBlock::AdjacentBlock succ2(&(cfg->getExit()), true);

    CFGBlock* entry = cfg->createBlock();
    entry->addSuccessor(succ1, c1);
    entry->addSuccessor(succ2, c2);
    
    dummyEntryBlockID = entry->getBlockID();

    // Set a dummy terminator for the entry node
    clang::Stmt::EmptyShell empty;
    IfStmt termStmt(empty);
    CFGTerminator terminator(&termStmt);
    entry->setTerminator(terminator);
    
#if 0
    llvm::outs() << "DEBUG: special predicate node ENTRY\n";
    entry->dump();
#endif

    // Compute post-dominator tree of augmented CFG
    llvm::DominatorTreeBase<CFGBlock> *postdTree = new
	llvm::DominatorTreeBase<CFGBlock>(true);
    postdTree->recalculate(*cfg);

#ifdef DEBUG
    postdTree->print(llvm::outs());
#endif

    // Find set S = {(A,B) \in cfg | B is not an ancestor of A in the
    // post-dominator tree}. B does not post-dominate A. Each edge has
    // associated label "T" or "F". The source of such edges will have a
    // terminator set in the associated CFGBlock.

    // set S = vector of 3-tuple <label, source, destination>
    // label = {"T", "F"}
    std::vector<std::pair<char, std::pair<unsigned, unsigned> > > S;
    std::set<unsigned> processedBlocks;
    std::queue<unsigned> workList;
    workList.push(entry->getBlockID());
    while (!workList.empty()) {
	unsigned currBlockID = workList.front();
	workList.pop();

	CFGBlock* currBlock;
	// get basic block from id
	bool foundBlock = false;
	for (CFG::iterator bIt = cfg->begin(); bIt !=
		cfg->end(); bIt++) {
	    if ((*bIt)->getBlockID() == currBlockID) {
		currBlock = *bIt;
		foundBlock = true;
		break;
	    }
	}
	if (!foundBlock) {
	    llvm::outs() << "ERROR: Cannot obtain block from ID: " 
			 << currBlockID << "\n";
	    rv = false;
	    return;
	}

	processedBlocks.insert(currBlockID);

	const CFGTerminator term = currBlock->getTerminator();
	const Stmt* termStmt = term.getStmt();
	if (termStmt != NULL) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Non-NULL Terminator Stmt for block " 
			 << currBlockID << "\n";
#endif
	    // First sucessor has label "T"
	    CFGBlock::succ_iterator sIt = currBlock->succ_begin();
	    char label = 'T';
	    // If successor does not post-dominate currBlock, then add edge
	    // to set S.
	    if (!postdTree->dominates(sIt->getReachableBlock(), currBlock)) {
		std::pair<unsigned, unsigned> edge;
		edge.first = currBlockID;
		edge.second = sIt->getReachableBlock()->getBlockID();
		std::pair<char, std::pair<unsigned, unsigned> >
		    labelledEdge;
		labelledEdge.first = label;
		labelledEdge.second = edge;
		S.push_back(labelledEdge);
	    }

	    // Second successor has label "F"
	    sIt++;
#if 0
	    if (sIt == currBlock->succ_end()) {
		llvm::outs() << "ERROR: Cannot find second successor of block "
			     << currBlockID << " while checking control dependent edges\n";
		rv = false;
		return;
	    }
#endif
	    if (sIt != currBlock->succ_end()) {
		label = 'F';
		if (!postdTree->dominates(sIt->getReachableBlock(), currBlock)) {
		    std::pair<unsigned, unsigned> edge;
		    edge.first = currBlockID;
		    edge.second = sIt->getReachableBlock()->getBlockID();
		    std::pair<char, std::pair<unsigned, unsigned> >
			labelledEdge;
		    labelledEdge.first = label;
		    labelledEdge.second = edge;
		    S.push_back(labelledEdge);
		}
	    }

	} else {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: NULL Terminator Stmt for block " 
			 << currBlockID << "\n";
#endif
	}
	for (CFGBlock::succ_iterator sIt = currBlock->succ_begin(); sIt !=
		currBlock->succ_end(); sIt++) {
	    CFGBlock* succBlock = sIt->getReachableBlock();
	    
	    if (processedBlocks.find(succBlock->getBlockID()) ==
		    processedBlocks.end()) {
		workList.push(succBlock->getBlockID());
	    }
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Set S\n";
    for (std::vector<std::pair<char, std::pair<unsigned, unsigned> > >::iterator
	    vIt = S.begin(); vIt != S.end(); vIt++) {
	llvm::outs() << "<" << vIt->first << ", (" << vIt->second.first << ", "
		     << vIt->second.second << ")> ";
    }
    llvm::outs() << "\n";
#endif

    // For each edge in set S, find least common ancestor of source and
    // destination nodes in the post-dominator tree
    for (std::vector<std::pair<char, std::pair<unsigned, unsigned> > >::iterator
	    vIt = S.begin(); vIt != S.end(); vIt++) {
	// Get CFGBlock of source and destination of the edge (from their IDs).
	char label = vIt->first;
	CFGBlock* src = getBlockFromID(cfg, vIt->second.first);
	CFGBlock* dest = getBlockFromID(cfg, vIt->second.second);
	CFGBlock* ancestor = postdTree->findNearestCommonDominator(src, dest);

#ifdef DEBUG
	llvm::outs() << "DEBUG: Ancestor of " << src->getBlockID() << " and "
		     << dest->getBlockID() << ": " << ancestor->getBlockID() << "\n";
#endif
	// Get all the nodes in the path from ancestor to dest. These nodes are
	// control dependent on src.
	std::vector<unsigned> path = getNodesInPath(postdTree, ancestor, dest);
	// This path contains ancestor also in it. If ancestor is not src, then
	// remove it from the path (the ancestor is not control dependent on
	// src) Check page 325 of "Program Dependence Graph and its Use in
	// Optimization".

	if (ancestor->getBlockID() != src->getBlockID())
	    path.erase(path.begin());

#ifdef DEBUG
	llvm::outs() << "DEBUG: Ancestor of " << src->getBlockID() << " and "
		     << dest->getBlockID() << ": " << ancestor->getBlockID() << "\n";
	llvm::outs() << "DEBUG: Path: ";
	for (std::vector<unsigned>::iterator pIt = path.begin(); pIt !=
		path.end(); pIt++)
	    llvm::outs() << *pIt << " ";
	llvm::outs() << "\n";
#endif

	for (std::vector<unsigned>::iterator pIt = path.begin(); pIt !=
		path.end(); pIt++) {
	    std::set<std::pair<unsigned, bool> > cdgSet;
	    if (cdg.find(*pIt) != cdg.end()) {
		cdgSet.insert(cdg[*pIt].begin(), cdg[*pIt].end());
		cdg.erase(cdg.find(*pIt));
	    }
	    std::pair<unsigned, bool> edge;
	    edge.first = src->getBlockID();
	    edge.second = (label == 'T') ? true : false;
	    cdgSet.insert(edge);
	    cdg[*pIt] = cdgSet;
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: CDG\n";
    for (std::map<unsigned, std::set<std::pair<unsigned, bool> > >::iterator cIt = cdg.begin();
	    cIt != cdg.end(); cIt++) {
	llvm::outs() << "DEBUG: Block " << cIt->first << ": ";
	for (std::set<std::pair<unsigned, bool> >::iterator sIt = cIt->second.begin(); sIt !=
		cIt->second.end(); sIt++)
	    llvm::outs() << "(" << sIt->first << ", " << ((sIt->second)? "true": "false")  << ") ";
	llvm::outs() << "\n";
    }
#endif
}

void FunctionAnalysis::computeBlocksWithSameTerminatorExpr(bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering FunctionAnalysis::computeBlocksWithSameTerminatorExpr()\n";
#endif
    rv = true;

    for (std::map<unsigned, std::set<std::pair<unsigned, bool> > >::iterator 
	    cIt = cdg.begin(); cIt != cdg.end(); cIt++) {
	CFGBlock* b = getBlockFromID(function->cfg, cIt->first);
	if (!b) {
	    llvm::outs() << "ERROR: Cannot get CFGBlock* for basic block "
			 << cIt->first << "\n";
	    rv = false;
	    return;
	}

	Stmt* termStmt = b->getTerminatorCondition();
	if (!termStmt) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: No terminator for block " << cIt->first << "\n";
#endif
	    continue;
	}
#ifdef DEBUG
	llvm::outs() << "DEBUG: Terminator Condition of block " << cIt->first 
		     << ": " << Helper::prettyPrintStmt(termStmt) << "\n";
#endif
	SubExprVisitor sv(this);
	sv.TraverseStmt(termStmt);
#ifdef DEBUG
	llvm::outs() << "DEBUG: SubExprs: ";
	for (std::vector<std::pair<unsigned, unsigned> >::iterator it = 
		sv.subExprs.begin(); it != sv.subExprs.end(); it++) {
	    llvm::outs() << "B" << it->first << "." << it->second << ", ";
	}
	llvm::outs() << "\n";
#endif
	if (terminatorSubExprs.find(cIt->first) != terminatorSubExprs.end()) {
	    llvm::outs() << "ERROR: Entry already exists for block " 
			 << cIt->first << "terminatorSubExprs\n";
	    rv = false;
	    return;
	}
	terminatorSubExprs[cIt->first] = sv.subExprs;
    }
#ifdef DEBUG
    for (std::map<unsigned, std::vector<std::pair<unsigned, unsigned> >
	    >::iterator x = terminatorSubExprs.begin(); x !=
	    terminatorSubExprs.end(); x++) {
	llvm::outs() << "DEBUG: Block " << x->first << ": subExprs:- ";
	for (std::vector<std::pair<unsigned, unsigned> >::iterator y =
		x->second.begin(); y != x->second.end(); y++)
	    llvm::outs() << "[B" << y->first << "." << y->second << "] ";
	llvm::outs() << "\n";
    }
#endif

    for (std::map<unsigned, std::vector<std::pair<unsigned, unsigned> > 
	    >::iterator x = terminatorSubExprs.begin(); x != terminatorSubExprs.end(); 
	    x++) {
	for (std::map<unsigned, std::vector<std::pair<unsigned, unsigned> > 
		>::iterator y = terminatorSubExprs.begin(); y != terminatorSubExprs.end(); 
		y++) {
	    if (y == x) {
		continue;
	    }

	    // Check if x is subset of y.
	    bool isSubset;
	    if (blockToSupersetBlocks.find(x->first) !=
		    blockToSupersetBlocks.end()) {
		if (blockToSupersetBlocks[x->first].find(y->first)
			!= blockToSupersetBlocks[x->first].end()) {
		    isSubset = true;
		    continue;
		}
	    } 

	    isSubset = true;
	    for (std::vector<std::pair<unsigned, unsigned> >::iterator xIt = 
		    x->second.begin(); xIt != x->second.end(); xIt++) {
		if (std::find(y->second.begin(), y->second.end(), *xIt) ==
			y->second.end()) {
		    isSubset = false;
		    break;
		}
		if (!isSubset) break;
	    }

	    if (isSubset) {
		if (blockToSupersetBlocks.find(x->first) != 
			blockToSupersetBlocks.end()) {
		    blockToSupersetBlocks[x->first].insert(y->first);
		} else {
		    std::set<unsigned> blocks;
		    blocks.insert(y->first);
		    blockToSupersetBlocks[x->first] = blocks;
		}
	    }
	}
    }
    for (std::map<unsigned, std::set<std::pair<unsigned, bool> > >::iterator 
	    cIt = cdg.begin(); cIt != cdg.end(); cIt++) {
	if (cIt->second.size() <= 1) continue;

#ifdef DEBUG
	llvm::outs() << "DEBUG: Looking at edges from block " << cIt->first << "\n";
#endif
	std::set<std::pair<unsigned, bool> > finalSetOfEdges;
	// The following vector maps pair(unsigned, bool) representing the basic
	// block and guard's truth value to a vector of subexpressions
	// represented as pair(unsigned, unsigned), i.e., block num and line num.
	std::vector<std::pair<std::pair<unsigned, bool>, 
	    std::vector<std::pair<unsigned, unsigned> > > > terminatorSubExprs;
	for (std::set<std::pair<unsigned, bool> >::iterator x =
		cIt->second.begin(); x != cIt->second.end(); x++) {
	    if (x->first == dummyEntryBlockID) {
		finalSetOfEdges.insert(std::make_pair(x->first, x->second));
		continue;
	    }

	    for (std::set<std::pair<unsigned, bool> >::iterator y = 
		    cIt->second.begin(); y != cIt->second.end(); y++) {
		if (x == y) continue;

		// Check if x is subset of y.
		bool isSubset;
		if (blockToSupersetBlocks.find(x->first) != blockToSupersetBlocks.end()) {
		    if (blockToSupersetBlocks[x->first].find(y->first) != 
			    blockToSupersetBlocks[x->first].end())
			isSubset = true;
		    else
			isSubset = false;
		} else 
		    isSubset = false;
		if (!isSubset)
		    finalSetOfEdges.insert(std::make_pair(x->first, x->second));
	    }
	}

#ifdef DEBUG
	llvm::outs() << "DEBUG: Original Edges:\n";
	for (std::set<std::pair<unsigned, bool> >::iterator it = 
		cIt->second.begin(); it != cIt->second.end(); it++) {
	    llvm::outs() << "(" << it->first << ", " 
			 << (it->second?"true":"false") << ") ";
	}
	llvm::outs() << "\nDEBUG: FinalSetOfEdges:\n";
	for (std::set<std::pair<unsigned, bool> >::iterator it =
		finalSetOfEdges.begin(); it != finalSetOfEdges.end(); it++) {
	    llvm::outs() << "(" << it->first << ", " 
			 << (it->second?"true":"false") << ") ";
	}
	llvm::outs() << "\n";
#endif

	cIt->second.clear();
	cIt->second.insert(finalSetOfEdges.begin(), finalSetOfEdges.end());
    }
    for (std::map<unsigned, std::set<std::pair<unsigned, bool> > >::iterator 
	    cIt = cdg.begin(); cIt != cdg.end(); cIt++) {
	std::set<std::pair<unsigned, bool> > finalSet;
	for (std::set<std::pair<unsigned, bool> >::iterator sIt =
		cIt->second.begin(); sIt != cIt->second.end(); sIt++) {
	    if (blockToSupersetBlocks.find(sIt->first) !=
		    blockToSupersetBlocks.end()) {
		if (blockToSupersetBlocks[sIt->first].find(cIt->first) != 
			blockToSupersetBlocks[sIt->first].end()) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Superset(" << cIt->first << ") to "
				 << " subset(" << sIt->first << ")\n";
#endif
		    // Add edges from subset block to finalSet
		    if (cdg.find(sIt->first) != cdg.end()) {
			finalSet.insert(cdg[sIt->first].begin(), cdg[sIt->first].end());
		    }
		} else {
		    finalSet.insert(std::make_pair(sIt->first, sIt->second));
		}
	    } else {
		finalSet.insert(std::make_pair(sIt->first, sIt->second));
	    }
	}

	cIt->second.clear();
	cIt->second.insert(finalSet.begin(), finalSet.end());
    }
#ifdef DEBUG
    llvm::outs() << "DEBUG: New CDG\n";
    for (std::map<unsigned, std::set<std::pair<unsigned, bool> > >::iterator cIt = cdg.begin();
	    cIt != cdg.end(); cIt++) {
	llvm::outs() << "DEBUG: Block " << cIt->first << ": ";
	for (std::set<std::pair<unsigned, bool> >::iterator sIt = cIt->second.begin(); sIt !=
		cIt->second.end(); sIt++)
	    llvm::outs() << "(" << sIt->first << ", " << ((sIt->second)? "true": "false")  << ") ";
	llvm::outs() << "\n";
    }
    llvm::outs() << "DEBUG: blockToSupersetBlocks:\n";
    for (std::map<unsigned, std::set<unsigned> >::iterator it =
	    blockToSupersetBlocks.begin(); it != blockToSupersetBlocks.end();
	    it++) {
	llvm::outs() << "Block " << it->first << ": ";
	for (std::set<unsigned>::iterator sIt = it->second.begin(); sIt != 
		it->second.end(); sIt++) {
	    llvm::outs() << *sIt << " ";
	}
	llvm::outs() << "\n";
    }
#endif
}

void FunctionAnalysis::getBlockDetails(std::unique_ptr<CFG> &cfg, 
bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering FunctionAnalysis::getBlockDetails()\n";
#endif
    rv = true;
    if (!SM) {
	llvm::outs() << "ERROR: SourceManager is NULL\n";
	rv = false;
	return;
    }

    for (CFG::iterator bIt = cfg->begin(); bIt !=
	    cfg->end(); bIt++) {
	const CFGBlock* currBlock = *bIt;
	unsigned currBlockID = currBlock->getBlockID();

#ifdef DEBUG
	llvm::outs() << "DEBUG: Processing block: " << currBlockID << "\n";
#endif
	if (currBlock->empty()) continue;

	int firstLine = -1, lastLine = -1;
	CFGElement first = currBlock->front();
	if (first.getKind() == CFGElement::Kind::Statement) {
	    CFGStmt currCFGStmt = first.castAs<CFGStmt>();
	    const Stmt* currStmt = currCFGStmt.getStmt();
	    if (!currStmt) {
		llvm::outs() << "ERROR: Cannot get Stmt from CFGStmt\n";
		rv = false;
		return;
	    } else {
		firstLine = SM->getExpansionLineNumber(currStmt->getLocStart(), 0);
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: First Line: " << firstLine << "\n";
#endif
	}
	
	CFGElement last = currBlock->back();
	if (last.getKind() == CFGElement::Kind::Statement) {
	    CFGStmt currCFGStmt = last.castAs<CFGStmt>();
	    const Stmt* currStmt = currCFGStmt.getStmt();
	    if (!currStmt) {
		llvm::outs() << "ERROR: Cannot get Stmt from CFGStmt\n";
		rv = false;
		return;
	    } else {
		lastLine = SM->getExpansionLineNumber(currStmt->getLocStart(), 0);
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Last Line: " << lastLine << "\n";
#endif
	}

	// Populate maps
	if (blockToLines.find(currBlockID) == blockToLines.end()) {
	    std::pair<int, int> details;
	    if (firstLine != -1 && lastLine != -1) {
		details.first = firstLine;
		details.second = lastLine;
		blockToLines[currBlockID] = details;
	    }
	} else {
	    llvm::outs() << "ERROR: blockToLines already consists an entry for block "
			 << currBlockID << "\n";
	    rv = false;
	    return;
	}

	if (firstLine == -1 || lastLine == -1) continue;

	for (int i = firstLine; i <= lastLine; i++) {
	    std::set<unsigned> blocks;
	    if (lineToBlock.find(i) == lineToBlock.end()) {
		blocks.insert(currBlockID);
		lineToBlock[i] = blocks;
	    } else {
		lineToBlock[i].insert(currBlockID);
	    }
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: blockToLines:\n";
    for (std::map<unsigned, std::pair<int, int> >::iterator bIt =
	    blockToLines.begin(); bIt != blockToLines.end(); bIt++) {
	llvm::outs() << "Block " << bIt->first << ": (" << bIt->second.first 
		     << ", " << bIt->second.second << ")\n";
    }
    llvm::outs() << "DEBUG: lineToBlock:\n";
    for (std::map<int, std::set<unsigned> >::iterator lIt = lineToBlock.begin(); lIt !=
	    lineToBlock.end(); lIt++) {
	llvm::outs() << "Line " << lIt->first << ": ";
	for (std::set<unsigned>::iterator bIt = lIt->second.begin(); bIt !=
		lIt->second.end(); bIt++) 
	    llvm::outs() << *bIt << ", ";
	llvm::outs() << "\n";
    }
#endif
}

bool FunctionAnalysis::isBackEdge(unsigned src, unsigned dest) {
    for (std::set<std::pair<unsigned, unsigned> >::iterator it =
	    backEdges.begin(); it != backEdges.end(); it++) {
	if (it->first == src && it->second == dest) return true;
    }
    return false;
}

void FunctionAnalysis::computeSymbolicExprs(std::unique_ptr<CFG> &cfg,
CompoundStmt* funcBody, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering FunctionAnalysis::computeSymbolicExprs()\n";
#endif
    rv = true;

    symVisitor = new GetSymbolicExprVisitor(SM, this, mainObj);

    std::stack<unsigned> workList;
    std::vector<bool> visited(cfg->size());
    std::vector<bool> blocksResolved(cfg->size());
    std::vector<unsigned> numOfTimesBlockWasProcessed(cfg->size());
    for (unsigned i = 0; i < cfg->size(); i++) {
	visited[i] = false;
	blocksResolved[i] = false;
	numOfTimesBlockWasProcessed[i] = 0;
    }

    CFGBlock entry = cfg->getEntry();

    unsigned entryBlockID = entry.getBlockID();
    workList.push(entryBlockID);
    visited[entryBlockID] = true;

    // Keep track of blocks already processed so that we do not add same blocks
    // back into the workList
    std::set<unsigned> processedBlocks;
    std::set<unsigned> secondTime;
    //std::set<int> processedStmts;
    while (true) {
	if (workList.empty())
	    break;

	unsigned currBlockID = workList.top();
	workList.pop();

	//bool alreadyseen = false;
	//if (processedBlocks.find(currBlockID) != processedBlocks.end())
	    //alreadyseen = true;

	// Add this block to set of processedBlocks
	processedBlocks.insert(currBlockID);
	numOfTimesBlockWasProcessed[currBlockID]++;

	std::vector<std::vector<std::pair<unsigned, bool> > > guards =
	    getGuardsOfBlock(currBlockID);

	//symVisitor.setCurrBlock(currBlockID);
	symVisitor->setCurrBlock(currBlockID);
	//symVisitor.setCurrGuards(guards);
	symVisitor->setCurrGuards(guards);

	CFGBlock* currBlock = getBlockFromID(cfg, currBlockID);
	if (!currBlock) {
	    llvm::outs() << "ERROR: Cannot obtain CFGBlock of block "
			 << currBlockID << "\n";
	    rv = false;
	    return;
	}

#ifdef DEBUG
	llvm::outs() << "DEBUG: Looking at basic block " << currBlockID << " ("
		     << currBlock->size() << ")\n";
#endif

	std::vector<bool> blockStmtsResolved(currBlock->size());
	int i = 0;
	for (CFGBlock::iterator eIt = currBlock->begin(); eIt != currBlock->end();
		eIt++) {
	    // Look only at those CFGElements that are Statements.
	    if (eIt->getKind() == CFGElement::Kind::Statement) {
		CFGStmt currCFGStmt = eIt->castAs<CFGStmt>();
		const Stmt* currStmt = currCFGStmt.getStmt();
		if (!currStmt) {
		    llvm::outs() << "ERROR: Cannot get Stmt from CFGStmt\n";
		    rv = false;
		    return;
		}

		int line = SM->getExpansionLineNumber(currStmt->getLocStart());
		Stmt* s = const_cast<Stmt*>(currStmt);
#ifdef DEBUG
		llvm::outs() << "DEBUG: Obtained Stmt at line " << line << "\n";
		llvm::outs() << "DEBUG: Printing stmt:\n";
		//StmtPrinterHelper p;
		LangOptions LO;
		LO.CPlusPlus = true;
		PrintingPolicy Policy(LO);
		currStmt->printPretty(llvm::outs(), 0, Policy);
		llvm::outs() << "\n";
#endif
		int counter = 0;
		while (true) {
		    symVisitor->unsetWaitingForBlock();
		    symVisitor->TraverseStmt(s);
		    if (symVisitor->error) {
			rv = false;
			return;
		    }

#ifdef DEBUG
		    llvm::outs() << "DEBUG: After TraverseStmt(), counter = " 
				 << counter << "\n";
#endif
		    if (symVisitor->waitingSet) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: waitingSet\n";
			llvm::outs() << "DEBUG: Inserting blocks " 
				     << symVisitor->waitingForBlock << " and "
				     << currBlockID << " to workList\n";
#endif
			workList.push(symVisitor->waitingForBlock);
			workList.push(currBlockID);
		    } else {
#ifdef DEBUG
			llvm::outs() << "DEBUG: Checking if the stmt is resolved\n";
#endif

			// Check if the entire stmt is resolved
			StmtDetails* sED = StmtDetails::getStmtDetails(SM, s,
			    currBlockID, rv);
			if (!rv) {
			    llvm::outs() << "ERROR: Could not obtain StmtDetails for "
					 << "stmt at line " << line << " and block "
					 << currBlockID << "\n";
			    return;
			}
			//if (symVisitor->isResolved(sED)) 
			bool resolved = sED->isSymExprCompletelyResolved(symVisitor, rv);
			if (!rv) {
			    llvm::outs() << "ERROR: While isSymExprCompletelyResolved()\n";
			    return;
			}
			if (resolved)
			{
#ifdef DEBUG
			    llvm::outs() << "DEBUG: Stmt at line " << line 
					 << " and block " << currBlockID 
					 << " is resolved. Breaking...\n";
			    sED->prettyPrint();
			    llvm::outs() << "\n";
			    symVisitor->printSymExprs(sED);
			    llvm::outs() << "\n";
#endif
			    blockStmtsResolved[i] = true;
			    break;
			} else {
#ifdef DEBUG
			    llvm::outs() << "DEBUG: Stmt is not resolved\n";
			    sED->prettyPrint();
			    llvm::outs() << "\n";
#endif
			    blockStmtsResolved[i] = false;
			}
		    }

		    counter++;
		    if (counter >= 25) {
			llvm::outs() << "WARNING: Breaking AST traversal since counter >= 25\n";
			//rv = false;
			//return;
#if 0
			// Add the current block to worklist again, so that may
			// be next time it will get resolved.
			if (secondTime.find(currBlockID) == secondTime.end()) { //&& 
				//!alreadyseen)
#ifdef DEBUG
			    llvm::outs() << "DEBUG: Inserted block " 
					 << currBlockID
					 << " to workList a second time\n";
#endif
			    secondTime.insert(currBlockID);
			    workList.push(currBlockID);
			}
#endif
			break;
		    }
		}
#ifdef DEBUG
		llvm::outs() << "DEBUG: Counter value is " << counter << "\n";
#endif
	    } else {
		blockStmtsResolved[i] = true;
	    }
	    i++;
	}
	// Check if all the stmts in the block are resolved
	bool resolved = true;
	for (unsigned i = 0; i < currBlock->size(); i++) {
	    if (!blockStmtsResolved[i]) {
		resolved = false;
		break;
	    }
	}
	if (resolved) blocksResolved[currBlock->getBlockID()] = true;

	// Add successors of currBlock to workList. Add the true branch first
	for (CFGBlock::succ_reverse_iterator sIt = currBlock->succ_rbegin(); sIt
		!= currBlock->succ_rend(); sIt++) {
	    CFGBlock* succBlock = sIt->getReachableBlock();
	    if (!visited[succBlock->getBlockID()]) {
		visited[succBlock->getBlockID()] = true;
		workList.push(succBlock->getBlockID());
#ifdef DEBUG
		llvm::outs() << "DEBUG: Inserted block " 
			     << succBlock->getBlockID() << " to workList\n";
#endif
	    } else if (!blocksResolved[succBlock->getBlockID()]){
		if (numOfTimesBlockWasProcessed[succBlock->getBlockID()] <= 25) {
		    workList.push(succBlock->getBlockID());
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Inserted block "
				 << succBlock->getBlockID() 
				<< " to workList because it was not resolved before\n";
#endif
		}
	    }
	}
    }

    // Adding this here for no good reason.
    for (std::vector<LoopDetails*>::iterator lIt = allLoopsInFunction.begin(); lIt != 
	    allLoopsInFunction.end(); lIt++) {
	if ((*lIt)->loopIndexInitValSymExpr) continue;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Before getSymbolicValuesForLoopHeader()\n";
	(*lIt)->print();
#endif
	(*lIt)->getSymbolicValuesForLoopHeader(SM, symVisitor, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While getSymbolicValuesForLoopHeader()\n";
	    (*lIt)->prettyPrintHere();
	    symVisitor->prettyPrintSymExprMap(true);
	    return;
	}
#ifdef DEBUG
	llvm::outs() << "DEBUG: After getSymbolicValuesForLoopHeader()\n";
	(*lIt)->print();
#endif
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: allLoopsInFunction after symex\n";
    for (std::vector<LoopDetails*>::iterator lIt = allLoopsInFunction.begin();
	    lIt != allLoopsInFunction.end(); lIt++)
	(*lIt)->print();
#endif

    symVisitor->getGuardExprs(rv);
    if (!rv) {
	llvm::outs() << "ERROR: While obtaining GuardExprs\n";
	return;
    }

    // Get symbolic expressions for return var. We do not have to do this if the
    // function is main()
    if (function->funcName.compare("main") != 0 && returnVar.varID.compare("") != 0) {
	unsigned exitBlock = cfg->getExit().getBlockID();
	if (rd.reachInSet.find(exitBlock) == rd.reachInSet.end()) {
	    llvm::outs() << "ERROR: Cannot find entry for exit block in reachInSet\n";
	    rv = false;
	    return;
	}
	if (rd.reachInSet[exitBlock].find(returnVar.varID) ==
		rd.reachInSet[exitBlock].end()) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Cannot find entry for return var in exitBlock\n";
#endif
	    //rv = false;
	    return;
	} else {

	    std::set<Definition> returnDefs = rd.reachInSet[exitBlock][returnVar.varID];
	    for (std::set<Definition>::iterator rdIt = returnDefs.begin(); rdIt != 
		    returnDefs.end(); rdIt++) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: Ret def: ";
		rdIt->print();
		llvm::outs() << "\nBlockID " << rdIt->blockID;
		llvm::outs() << "\n";
#endif
		Expr* rE = const_cast<Expr*>(rdIt->expression);
		unsigned rB = rdIt->blockID;
		ExprDetails* retExpr = ExprDetails::getObject(SM, rE, rB, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: Cannot obtain ExprDetails from return expr "
				 << Helper::prettyPrintExpr(rdIt->expression) << "\n";
		    return;
		}
		if (!(symVisitor->isResolved(retExpr))) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Return Expr is not resolved\n";
#endif
		    if (!retExpr->origExpr) {
			llvm::outs() << "ERROR: ReturnExpr has NULL origExpr\n";
			rv = false;
			return;
		    }

		    int counter=0;
		    unsigned oldBlock = symVisitor->currBlock;
		    symVisitor->setCurrBlock(rB);
		    symVisitor->clearCurrGuards();
		    while (true) {
			if (counter >= 25 || symVisitor->isResolved(retExpr))
			    break;
			counter++;
			symVisitor->TraverseStmt(retExpr->origExpr);
			if (symVisitor->error) {
			    llvm::outs() << "ERROR: While calling GetSymbolicExprVisitor on "
					 << Helper::prettyPrintExpr(retExpr->origExpr)
					 << "\n";
			    rv = false;
			    return;
			}
		    }
		    if (!symVisitor->isResolved(retExpr)) {
			llvm::outs() << "ERROR: Cannot resolve retExpr: "
				     << Helper::prettyPrintExpr(retExpr->origExpr)
				     << "\n";
		    }
		    symVisitor->setCurrBlock(oldBlock);
		}
		std::vector<SymbolicStmt*> symExprs =
		    symVisitor->getSymbolicExprs(retExpr, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: Cannot obtain symexprs for return expr\n";
		    retExpr->print();
		    return;
		}
		if (symExprs.size() == 0) {
		    llvm::outs() << "ERROR: No symbolic expression for return "
				 << "expression:\n";
		    retExpr->print();
		    rv = false;
		    return;
		}

		for (std::vector<SymbolicStmt*>::iterator sIt = symExprs.begin();
			sIt != symExprs.end(); sIt++) {
		    if (!isa<SymbolicExpr>(*sIt)) {
			llvm::outs() << "ERROR: Symexpr for return expr is not "
				     << "<SymbolicExpr>\n";
			retExpr->print();
			rv = false;
			return;
		    }

		    retSymExprs.push_back(dyn_cast<SymbolicExpr>(*sIt));
		}
	    }
	}
    }

#if 0
    // Compute symbolic expressions for special exprs if any
    for (std::vector<SpecialExpr>::iterator sIt = spExprs.begin(); sIt != 
	    spExprs.end(); sIt++) {
	for (std::vector<Expr*>::iterator iIt =
		sIt->arrayIndexExprsAtAssignLine.begin(); iIt != 
		sIt->arrayIndexExprsAtAssignLine.end(); iIt++) {
	    ExprDetails* indexDetails = ExprDetails::getObject(SM, *iIt,
		assignmentBlock, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot get ExprDetails of "
			     << Helper::prettyPrintExpr(*iIt) << "\n";
		return;
	    }

	    std::vector<SymbolicStmt*> indexSymExprs =
		symVisitor->getSymbolicExprs(indexDetails, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot obtain symexprs for "
			     << Helper::prettyPrintExpr(*iIt) << "\n";
		return;
	    }
	    if (indexSymExprs.size() > 1) {
		llvm::outs() << "ERROR: Index expr has more than 1 symexprs\n";
		llvm::outs() << Helper::prettyPrintExpr(*iIt) << "\n";
	    }
	    
	}
    }
#endif

#ifdef DEBUG
    llvm::outs() << "DEBUG: After computeSymbolicExprs()\n";
    llvm::outs() << "==================================\n";
    symVisitor->printSymExprMap(false);
    llvm::outs() << "----------------------------------\n";
    llvm::outs() << "DEBUG: Return expressions: " << retSymExprs.size() << "\n";
    for (std::vector<SymbolicExpr*>::iterator sIt = retSymExprs.begin(); sIt != 
	    retSymExprs.end(); sIt++) {
	if (*sIt)
	    (*sIt)->print();
	else {
	    llvm::outs() << "ERROR: NULL return expression\n";
	    rv = false;
	    return;
	}
    }
    llvm::outs() << "==================================\n";
#endif

}

std::vector<LoopDetails*> FunctionAnalysis::getLoopsOfBlock(unsigned block, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering FunctionAnalysis::getLoopsOfBlock()\n";
    llvm::outs() << "DEBUG: Back edges: ";
    for (std::map<std::pair<unsigned, unsigned>, std::set<unsigned> >::iterator 
	    eIt = edgesToLoop.begin(); eIt != edgesToLoop.end(); eIt++) {
	llvm::outs() << "(" << eIt->first.first << ", " << eIt->first.second << "), ";
    }
    llvm::outs() << "\n";
#endif
    std::vector<LoopDetails*> loops;
    rv = true;
    for (std::map<std::pair<unsigned, unsigned>, std::set<unsigned> >::iterator
	    eIt = edgesToLoop.begin(); eIt != edgesToLoop.end(); eIt++) {
	if (eIt->second.find(block) == eIt->second.end()) continue;
#ifdef DEBUG
	llvm::outs() << "DEBUG: Block " << block << " is part of loop with backedge ("
		     << eIt->first.first << ", " << eIt->first.second << ")\n";
#endif

	std::pair<unsigned, unsigned> backEdgeOfLoop;
	backEdgeOfLoop.first = eIt->first.first;
	backEdgeOfLoop.second = eIt->first.second;

#if 0
	llvm::outs() << "DEBUG: edgesToLoopDetails:\n";
	for (std::map<std::pair<unsigned, unsigned>, LoopDetails>::iterator it = 
		edgesToLoopDetails.begin(); it != edgesToLoopDetails.end();
		it++) {
	    llvm::outs() << "Edge (" << it->first.first << ", " 
			 << it->first.second << "):\n";
	    it->second.print();
}
#endif

	// get loop details of this loop
	bool foundEdge = false;
	for (std::map<std::pair<unsigned, unsigned>, LoopDetails*>::iterator lIt
		= edgesToLoopDetails.begin(); lIt != edgesToLoopDetails.end(); 
		lIt++) {
	    if (lIt->first.first == backEdgeOfLoop.first && 
		    lIt->first.second == backEdgeOfLoop.second) {
		foundEdge = true;
		loops.push_back(lIt->second);
		break;
	    }
	}

	if (!foundEdge) {
	    llvm::outs() << "ERROR: Could not find loopdetails for the edge ("
			 << backEdgeOfLoop.first << ", " 
			 << backEdgeOfLoop.second << ")\n";
	    rv = false;
	    return loops;

	}
    }

    if (loops.size() == 0) {
	// Check if the current block is part of break in any loop
	for (std::vector<LoopDetails*>::iterator lIt =
		allLoopsInFunction.begin(); lIt != 
		allLoopsInFunction.end(); lIt++) {
	    if ((*lIt)->blockIDsOfAllBreaksInLoopToBlocksGuardingThem.size() == 0)
		continue;
	    for (std::map<unsigned, std::set<unsigned> >::iterator bIt = 
		    (*lIt)->blockIDsOfAllBreaksInLoopToBlocksGuardingThem.begin();
		    bIt != (*lIt)->blockIDsOfAllBreaksInLoopToBlocksGuardingThem.end();
		    bIt++) {
		if (block == bIt->first) {
		    loops.push_back(*lIt);
		    break;
		}
	    }
	}
    }

    std::sort(loops.begin(), loops.end(), 
	[](LoopDetails* l1, LoopDetails* l2) {
	    return l1->loopStartLine < l2->loopStartLine; 
	}
    );

    return loops;
}

bool FunctionAnalysis::isLoopIndexVar(VarDetails var, unsigned block,
bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering FunctionAnalysis::isLoopIndexVar() with var: ";
    var.print();
    llvm::outs() << " in block " << block << "\n";
#endif
    rv = true;

    std::vector<LoopDetails*> loops = getLoopsOfBlock(block, rv);

    if (!rv) {
	llvm::outs() << "ERROR: While getting loops of block " << block << "\n";
	return false;
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Loops of block " << block << "\n";
    for (std::vector<LoopDetails*>::iterator lIt = loops.begin(); lIt !=
	    loops.end(); lIt++) {
	(*lIt)->prettyPrintHere();
    }
#endif

    for (std::vector<LoopDetails*>::iterator it = loops.begin(); it != loops.end();
	    it++) {
	if ((*it)->loopIndexVar.varName.compare(var.varName) == 0 && 
		(*it)->loopIndexVar.varDeclLine == var.varDeclLine)
	    return true;
    }

    return false;
}

void FunctionAnalysis::getInputVars(std::vector<std::string> customInputFuncs, bool &rv) {
    rv = true;

    // If function is custom input function, do not analyze.
    if (function->isCustomInputFunction || function->isCustomOutputFunction)
	return;

    InputStmtVisitor iv(SM, customInputFuncs);
    iv.TraverseDecl(const_cast<FunctionDecl*>(function->fd));
    if (iv.error) {
	rv = false;
	return;
    }

    inputVars.insert(inputVars.end(), iv.inputVarsFound.begin(),
	iv.inputVarsFound.end());
    inputLines.insert(inputLines.end(), iv.inputLinesFound.begin(), 
	iv.inputLinesFound.end());
    inputStmts.insert(inputStmts.end(), iv.inputStmts.begin(),
	iv.inputStmts.end());
    for (std::vector<InputVar>::iterator inIt = inputVars.begin(); inIt != 
	    inputVars.end(); inIt++) {
	//Helper::copyFunctionDetails(function, inIt->func);
	inIt->func = function;
    }

    // TODO: Soundness check: If there are multiple input statements reading the same
    // input variable, we need to fail here. But this needs to take into account
    // those programs that use temporary variable to read in the input and then
    // assigns it to the actual input variable
    for (std::vector<InputVar>::iterator inIt = inputVars.begin(); inIt != 
	    inputVars.end(); inIt++) {
	VarDetails var = inIt->var;
	int callLine = inIt->inputCallLine;

	for (std::vector<InputVar>::iterator cIt = inIt+1; cIt !=
		inputVars.end(); cIt++) {
	    // Check if there is a duplicate entry.
	    if (var.varName.compare(cIt->var.varName) == 0 && 
		    var.varDeclLine == cIt->var.varDeclLine) {
		if (callLine == cIt->inputCallLine) {
		    llvm::outs() << "ERROR: Duplicate entry for var " 
				 << var.varName << " (read at " << callLine 
				 << ") in inputVars\n";
		    rv = false;
		    return;
		} else {
		    llvm::outs() << "ERROR: Var " << var.varName << " read at "
				 << callLine << " and " << cIt->inputCallLine 
				 << "\n";
		    rv = false;
		    return;
		}
	    }
	}
    }

    // Sort the inputVars based on line number
    std::sort(inputVars.begin(), inputVars.end());

    int i = 1;
    for (std::vector<InputVar>::iterator inIt = inputVars.begin(); inIt != 
	    inputVars.end(); inIt++) {
	std::stringstream s;
	s << function->funcName << "_" << i;
	std::string currID = s.str();
	inIt->funcID = currID;
	i++;
    }

}

void FunctionAnalysis::getSpecialExprs(bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering FunctionAnalysis::getSpecialExprs()\n";
#endif

    rv = true;
    SpecialStmtVisitor ssv(SM);
    ssv.TraverseFunctionDecl(const_cast<FunctionDecl*>(function->fd));

#ifdef DEBUG
    llvm::outs() << "DEBUG: Special exprs found:\n";
    for (std::vector<SpecialExpr>::iterator sIt = ssv.exprsFound.begin(); sIt != 
	    ssv.exprsFound.end(); sIt++)
	sIt->print();
#endif

    // At this point, we only have the assignment statement. Find the guard and
    // loops for the statement and then check if it matches the pattern for a
    // special statement.

    for (std::vector<SpecialExpr>::iterator sIt = ssv.exprsFound.begin(); sIt !=
	    ssv.exprsFound.end(); sIt++) {
	// Find the block containing this statement.
	if (lineToBlock.find(sIt->assignmentLine) == lineToBlock.end()) {
	    llvm::outs() << "ERROR: Cannot find the basic block containing line "
			 << sIt->assignmentLine << "\n";
	    rv = false;
	    return;
	}

	unsigned currBlock;
	if (lineToBlock[sIt->assignmentLine].size() > 1 &&
		!(sIt->fromTernaryOperator)) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: The assignment line " << sIt->assignmentLine 
			 << " is part of more than one "
			 << "block. I don't know how to handle this!\n";
#endif
	    bool toSkip = false;
	    for (std::set<unsigned>::iterator it =
		    lineToBlock[sIt->assignmentLine].begin(); it != 
		    lineToBlock[sIt->assignmentLine].end(); it++) {
		bool isLoopIndex = isLoopIndexVar(sIt->scalarVar, *it, rv);
		if (!rv) {
		    llvm::outs() << "ERROR: While checking if " 
				 << sIt->scalarVar.varName << " is a loop index\n";
		    return;
		}
		if (isLoopIndex) {
		    toSkip = true;
		    break;
		}
	    }
	    if (toSkip) continue;

	    // Check if the special assignment statement is part of the init
	    // block of some for loop, because of which the line is part of
	    // multiple blocks.
	    bool foundSpecialExprInLoopInit = false;
	    for (std::map<std::pair<unsigned, unsigned>, LoopDetails*>::iterator
		    lIt = edgesToLoopDetails.begin(); lIt !=
		    edgesToLoopDetails.end(); lIt++) {
		for (std::vector<std::pair<Definition, SymbolicExpr*> >::iterator idIt =
			lIt->second->setOfLoopIndexInitValDefs.begin(); idIt !=
			lIt->second->setOfLoopIndexInitValDefs.end(); idIt++) {
		    if (idIt->first.lineNumber == sIt->assignmentLine) {
			foundSpecialExprInLoopInit = true;
			sIt->assignmentBlock = idIt->first.blockID;
			currBlock = idIt->first.blockID;
			break;
		    }
		}
		if (foundSpecialExprInLoopInit) break;
	    }

	    if (!foundSpecialExprInLoopInit) {
		llvm::outs() << "ERROR: The assignment line " << sIt->assignmentLine
			     << " is part of more than one block. I don't know how "
			     << "to handle this!\n";
		llvm::outs() << "ERROR: The line " << sIt->assignmentLine 
			     << " is part of the following blocks: ";
		for (std::set<unsigned>::iterator bIt =
			lineToBlock[sIt->assignmentLine].begin(); bIt != 
			lineToBlock[sIt->assignmentLine].end(); bIt++)
		    llvm::outs() << *bIt << " ";
		llvm::outs() << "\n";
		rv = false;
		return;
	    }
	} else {
	    currBlock = *(lineToBlock[sIt->assignmentLine].begin());
	    sIt->assignmentBlock = currBlock;
	}

	// Find all loops surrounding this block.
	std::vector<LoopDetails*> allLoops = getLoopsOfBlock(currBlock, rv);
	if (!rv)
	    return;

	// Get guards of this block
	std::set<std::pair<unsigned, bool> > origGuardBlocks;
	std::set<std::pair<unsigned, bool> > guardBlocks;
	if (cdg.find(currBlock) == cdg.end()) {
	    llvm::outs() << "ERROR: No entry for block " << currBlock << " in cdg\n";
	    rv = false;
	    return;
	}

	origGuardBlocks = cdg[currBlock];

	// Remove from guardBlocks guards corresponding to loop headers
	for (std::set<std::pair<unsigned, bool> >::iterator gIt =
		origGuardBlocks.begin(); gIt != origGuardBlocks.end(); gIt++) {
	    bool isLoopHeader = false;
	    for (std::vector<LoopDetails*>::iterator it = allLoops.begin(); it != 
		    allLoops.end(); it++) {
		if ((*it)->loopHeaderBlock == gIt->first) {
		    isLoopHeader = true;
		    break;
		}
	    }
	    if (!isLoopHeader) {
		guardBlocks.insert(std::make_pair(gIt->first, gIt->second));
	    }
	}

#ifdef DEBUG
	llvm::outs() << "\nLoops with block " << currBlock << ":\n";
	int i = 0;
	for (std::vector<LoopDetails*>::iterator it = allLoops.begin(); it !=
		allLoops.end(); it++) {
	    llvm::outs() << "*******Loop " << ++i << "*******\n";
	    (*it)->print();
	}
	llvm::outs() << "Guards of block " << currBlock << ": ";
	for (std::set<std::pair<unsigned, bool> >::iterator gIt =
		guardBlocks.begin(); gIt != guardBlocks.end(); gIt++) {
	    llvm::outs() << "(" << gIt->first << ", " 
			 << (gIt->second ? "true": "false") << ") ";
	}
	llvm::outs() << "\n";
#endif

	VarDetails scalarVarDetails;
	std::vector<Definition> reachDefs;
	// If there is more than one guard, then this is not the special expr.
	if (guardBlocks.size() > 1) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: There are multiple guards for this special "
			 << "expr. Skipping this..\n";
#endif
	    continue;
	} else if (guardBlocks.size() == 1) {
	    // If SpecialExpr is SUM and there are guards to it, error.
	    if (sIt->kind == SpecialExprKind::SUM) {
		llvm::outs() << "ERROR: SpecialExpr SUM at line " 
			     << sIt->assignmentLine << " has guards\n";
		rv = false;
		return;
	    }
	    if (sIt->fromTernaryOperator) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: SpecialExpr fromTernaryOperator at line "
			     << sIt->assignmentLine << " has guards\n";
		llvm::outs() << "DEBUG: guardBlocks:\n";
		for (std::set<std::pair<unsigned, bool> >::iterator gbIt =
			guardBlocks.begin(); gbIt != guardBlocks.end(); gbIt++) {
		    llvm::outs() << "(" << gbIt->first << ", " 
				 << (gbIt->second? "true" : "false") << ") ";
		}
		llvm::outs() << "\n";
#endif
		//rv = false;
		//return;
		continue;
	    }

	    std::pair<unsigned, bool> exprGuardBlock = *(guardBlocks.begin());
	    // If the guard is the loop exit condition of any of the surrounding
	    // loop, then this special expr does not have a guard. If the special
	    // expr type is UNRESOLVED and the guard is the loop exit condition,
	    // then skip.
	    bool isLoopExit = false;
	    for (std::vector<LoopDetails*>::iterator lIt = allLoops.begin(); lIt != 
		    allLoops.end(); lIt++) {
		if (sIt->kind == SpecialExprKind::UNRESOLVED &&
			exprGuardBlock.first == (*lIt)->loopHeaderBlock) {
		    isLoopExit = true;
		    break;
		}
	    }
	    if (isLoopExit) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: The guard of special expr is the header of "
			     << "a surrounding loop. Skipping this..\n";
#endif
		continue;
	   }

	    bool isScalarVarLoopIndexVar = false;
	    for (std::vector<LoopDetails*>::iterator lIt = allLoops.begin(); lIt != 
		    allLoops.end(); lIt++) {
		if ((*lIt)->loopIndexVar.equals(sIt->scalarVar)) {
		    isScalarVarLoopIndexVar = true;
		    break;
		}
	    }

	    if (isScalarVarLoopIndexVar) {
		llvm::outs() << "ERROR: The scalar var is a loop index var\n";
		rv = false;
		return;
	    }

#if 0
	    std::unique_ptr<CFG> funcCFG = function.constructCFG(rv);
	    if (!rv) {
		llvm::outs() << "ERROR: Cannot construct CFG of function:\n";
		function.print();
		return;
	    }
#endif
	    // Check the structure of the guard to identify the special expr kind.
	    CFGBlock* guardBlock = getBlockFromID(function->cfg,
		exprGuardBlock.first);
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Trying to find terminator of block " 
			 << exprGuardBlock.first << "\n";
	    guardBlock->dump();
#endif
	    Stmt* guardBlockTerminator = guardBlock->getTerminatorCondition();
	    if (!isa<BinaryOperator>(guardBlockTerminator)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: The guard of special expr is not "
			     << "BinaryOperator. Skipping this..\n";
#endif
		continue;
	    }

	    int guardLineNumber =
		SM->getExpansionLineNumber(guardBlockTerminator->getLocStart());
	    BinaryOperator* guardExprAsBO =
		dyn_cast<BinaryOperator>(guardBlockTerminator);
	    if (!guardExprAsBO->isRelationalOp()) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: The guard of special expr is not relational op\n";
#endif
		continue;
	    }

	    BinaryOperatorKind opCode = guardExprAsBO->getOpcode();
	    Expr* lhsExpr = guardExprAsBO->getLHS()->IgnoreParenCasts();
	    Expr* rhsExpr = guardExprAsBO->getRHS()->IgnoreParenCasts();
	    DeclRefExpr* scalarExpr;
	    ArraySubscriptExpr* arrayExpr;
	    bool maxOrMin; // true if it is a maxexpr, false if it is a minexpr
	    if (isa<DeclRefExpr>(lhsExpr) && isa<ArraySubscriptExpr>(rhsExpr)) {
		scalarExpr = dyn_cast<DeclRefExpr>(lhsExpr);
		arrayExpr = dyn_cast<ArraySubscriptExpr>(rhsExpr);
		if (opCode == BinaryOperatorKind::BO_LE || 
			opCode == BinaryOperatorKind::BO_LT) {
		    if (exprGuardBlock.second)  // if the guard is in true form
			maxOrMin = true;
		    else
			maxOrMin = false;
		} else if (opCode == BinaryOperatorKind::BO_GE || 
			    opCode == BinaryOperatorKind::BO_GT) {
		    if (exprGuardBlock.second)
			maxOrMin = false;
		    else
			maxOrMin = true;
		} else {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: The guard is not x <= arr or x < arr or "
				 << "x >= arr or x > arr. Skipping this..\n";
#endif
		    continue;
		}
	    } else if (isa<ArraySubscriptExpr>(lhsExpr) && 
			isa<DeclRefExpr>(rhsExpr)) {
		scalarExpr = dyn_cast<DeclRefExpr>(rhsExpr);
		arrayExpr = dyn_cast<ArraySubscriptExpr>(lhsExpr);
		if (opCode == BinaryOperatorKind::BO_LE || 
			opCode == BinaryOperatorKind::BO_LT) {
		    if (exprGuardBlock.second)
			maxOrMin = false;
		    else
			maxOrMin = true;
		} else if (opCode == BinaryOperatorKind::BO_GE ||
			    opCode == BinaryOperatorKind::BO_GT) {
		    if (exprGuardBlock.second)
			maxOrMin = true;
		    else
			maxOrMin = false;
		} else {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: The guard is not arr <= x or arr < x or "
				 << "arr >= x or arr > x. Skipping this..\n";
#endif
		    continue;
		}
	    } else {
#ifdef DEBUG
		llvm::outs() << "DEBUG: The component expressions of guard are not "
			     << "DeclRefExpr & ArraySubscriptExpr. Skipping this..\n";
#endif
		continue;
	    }

	    // Check if the scalarExpr and arrayExpr in the guard correspond to those
	    // in the assignment statement.
	    scalarVarDetails =
		Helper::getVarDetailsFromExpr(SM, scalarExpr, rv);
	    if (!rv) return;
	    VarDetails arrayVarDetails = 
		Helper::getVarDetailsFromExpr(SM, arrayExpr, rv);
	    if (!rv) return;
	    std::vector<Expr*> arrayIndexExprsAtGuard =
		Helper::getArrayIndexExprs(arrayExpr);

#ifdef DEBUG
	    llvm::outs() << "DEBUG: scalarvar in guard: ";
	    scalarVarDetails.print();
	    llvm::outs() << "\n";
	    llvm::outs() << "DEBUG: scalarvar in special expr: ";
	    sIt->scalarVar.print();
	    llvm::outs() << "\n";
#endif
	    if (!scalarVarDetails.equals(sIt->scalarVar)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: The scalar var in guard is not the same as "
			     << "the one in special expr. Skipping this..\n";
#endif
		continue;
	    }

	    if (!arrayVarDetails.equals(sIt->arrayVar)) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: The array var in guard is not the same as "
			     << "the one in special expr. Skipping this..\n";
#endif
		continue;
	    }

	    if (arrayIndexExprsAtGuard.size() !=
		    sIt->arrayIndexExprsAtAssignLine.size()) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: The array expr in guard does not have the "
			     << "same index exprs as the one in special expr. "
			     << "Skipping this..\n";
#endif
		continue;
	    }

	    bool sameIndex = true;
	    for (unsigned i = 0; i < arrayIndexExprsAtGuard.size(); i++) {
		std::string gIndex =
		    Helper::prettyPrintExpr(arrayIndexExprsAtGuard[i]);
		std::string sIndex = 
		    Helper::prettyPrintExpr(sIt->arrayIndexExprsAtAssignLine[i]);
		if (gIndex.compare(sIndex) != 0) {
		    sameIndex = false;
		    break;
		}
	    }

	    if (!sameIndex) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: The array expr in guard does not have the "
			     << "same index exprs as the one in special expr. "
			     << "Skipping this..\n";
#endif
		continue;
	    }

	    // Scalar var and array var are the same in guard and assignStmt. The
	    // guard is a binaryoperator of the form we want.
	    if (maxOrMin)
		sIt->kind = SpecialExprKind::MAX;
	    else
		sIt->kind = SpecialExprKind::MIN;

	    sIt->guardLine = guardLineNumber;

	    // Find reaching definitions of scalar var at the guard statement. There
	    // must be more than one definition. One which is the assignment
	    // identified and the rest are the initial values of special expr. For
	    // each initial val, we should create a separate SpecialExpr object.

	    reachDefs =
		getReachDefsOfVarAtLine(scalarVarDetails, guardLineNumber,
		exprGuardBlock.first, rv);
	    if (!rv) return;

#ifdef DEBUG
	    llvm::outs() << "DEBUG: Reaching defs for var: " ;
	    scalarVarDetails.print();
	    llvm::outs() << " at line " << guardLineNumber << ":\n";
	    for (std::vector<Definition>::iterator dIt = reachDefs.begin(); dIt != 
		    reachDefs.end(); dIt++)
		dIt->print();
#endif
	    sIt->guardExpr = guardExprAsBO;

	} else {
	    // No guards.
	    if (sIt->kind != SpecialExprKind::SUM && !sIt->fromTernaryOperator) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: SpecialExpr at line " 
			     << sIt->assignmentLine << " has no guards\n";
#endif
		//continue;
	    } else {

		scalarVarDetails = sIt->scalarVar;
		// Find reaching definitions of scalar var at the assign statement. There
		// must be more than one definition. One which is the assignment
		// identified and the rest are the initial values of special expr. For
		// each initial val, we should create a separate SpecialExpr object.

		reachDefs = 
		    getReachDefsOfVarAtLine(scalarVarDetails, sIt->assignmentLine, 
		    sIt->assignmentBlock, rv);
		if (!rv) return;
#ifdef DEBUG
		llvm::outs() << "DEBUG: Reaching defs for var: " ;
		scalarVarDetails.print();
		llvm::outs() << " at line " << sIt->assignmentLine << ":\n";
		for (std::vector<Definition>::iterator dIt = reachDefs.begin(); dIt != 
			reachDefs.end(); dIt++)
		    dIt->print();
#endif
	    }
	}


	// Assumption: There cannot be more than one def for the same var at the
	// same statement. This assumption can be violated if the var is a
	// loopIndexVar, but we confirmed that the scalar var is not a loop
	// index var before.

	// Remove the def corresponding to the assignment statement identified
	// in special expr
	std::vector<Definition>::iterator dIt;
	bool assignFound = false;
	for (dIt = reachDefs.begin(); dIt != reachDefs.end(); dIt++) {
	    if (dIt->lineNumber == sIt->assignmentLine) {
		assignFound = true;
		break;
	    }
	}
	if (assignFound)
	    reachDefs.erase(dIt);

	// In the remaining reach defs, if any of them traverse a back edge,
	// fail.
	for (dIt = reachDefs.begin(); dIt != reachDefs.end(); dIt++) {
	    if (dIt->throughBackEdge) {
		llvm::outs() << "ERROR: Reaching definition through back edge\n";
		dIt->print();
		rv = false;
		return;
	    }
	}

#ifdef DEBUG
	llvm::outs() << "DEBUG: Initial val for special expr with scalar var "
		     << scalarVarDetails.varName << ":\n";
	for (std::vector<Definition>::iterator dIt = reachDefs.begin(); dIt != 
		reachDefs.end(); dIt++)
	    dIt->print();
#endif

	// Update the loops in special expr
	sIt->loops.insert(sIt->loops.end(), allLoops.begin(), allLoops.end());
	sIt->isLoopSet = true;
	//sIt->initialVal.insert(sIt->initialVal.end(), reachDefs.begin(),
	    //reachDefs.end());

	// Populate the base class (Definition) fields
	sIt->isSpecialExpr = true;
	sIt->lineNumber = sIt->assignmentLine;
	sIt->blockID = -1;
	sIt->expression = NULL;
	if (sIt->kind == SpecialExprKind::MAX)
	    sIt->expressionStr = "SYMEX_MAX";
	else if (sIt->kind == SpecialExprKind::MIN)
	    sIt->expressionStr = "SYMEX_MIN";
	else if (sIt->kind == SpecialExprKind::SUM)
	    sIt->expressionStr = "SYMEX_SUM";
	else
	    sIt->expressionStr = "SYMEX_UNKNOWN";
	sIt->throughBackEdge = false;

	for (std::vector<SpecialExpr*>::iterator it = spExprs.begin(); it != 
		spExprs.end(); it++) {
	    if (!((*it)->scalarVar.equals(sIt->scalarVar))) continue;
	    if (!((*it)->arrayVar.equals(sIt->arrayVar))) continue;
	    if ((*it)->assignmentLine != sIt->assignmentLine || (*it)->guardLine != 
		    sIt->guardLine) continue;

	    llvm::outs() << "ERROR: Current special expr already recorded in spExprs\n";
	    llvm::outs() << "ERROR: Existing entry:\n";
	    (*it)->print();
	    rv = false;
	    return;
	}

	// Populate initial val of the special expr. For each reachDef or for
	// each initial val, clone a new special expr.
	for (std::vector<Definition>::iterator iIt = reachDefs.begin(); iIt != 
		reachDefs.end(); iIt++) {
	    sIt->initialVal = iIt->clone();
	    SpecialExpr* clone = new SpecialExpr;
	    sIt->clone(clone);
	    spExprs.push_back(clone);
	}

    }

#ifdef DEBUG
    llvm::outs() << "Final set of special exprs:\n";
    for (std::vector<SpecialExpr*>::iterator it = spExprs.begin(); it != 
	    spExprs.end(); it++) {
	(*it)->print();
    }
#endif
}

std::vector<Definition> FunctionAnalysis::getReachDefsOfVarAtLine(
VarDetails var, int line, unsigned block, bool &rv) {

    std::vector<Definition> defs;
    // Check if this var is defined in this block
    if (rd.blockToVarsDefined.find(block) == rd.blockToVarsDefined.end()) {
	llvm::outs() << "ERROR: Cannot find blocksToVarsDefined of block "
		     << block << "\n";
	rv = false;
	return defs;
    }

    std::vector<std::pair<VarDetails, int> > varsDefinedInBlock =
	rd.blockToVarsDefined[block];
    bool defInBlock = false;
    if (varsDefinedInBlock.size() != 0) {
	for (std::vector<std::pair<VarDetails, int> >::iterator it = 
		varsDefinedInBlock.begin(); it != varsDefinedInBlock.end(); 
		it++) {
	    if (var.equals(it->first) && it->second < line) {
		defInBlock = true;
		break;
	    }
	}
    }

    if (defInBlock && line != -1) {
	if (rd.allDefsInBlock.find(block) == rd.allDefsInBlock.end()) {
	    llvm::outs() << "ERROR: Cannot find allDefsInBlock of block "
			 << block << "\n";
	    rv = false;
	    return defs;
	}

	if (rd.allDefsInBlock[block].find(var.varID) ==
		rd.allDefsInBlock[block].end()) {
	    llvm::outs() << "ERROR: Cannot find entry for var " << var.varID
			 << " in allDefsInBlock of block " << block << "\n";
	    rv = false;
	    return defs;
	}
    
	Definition lastDef;
	bool foundDef = false;
	for (std::vector<Definition>::iterator dIt =
		rd.allDefsInBlock[block][var.varID].begin(); dIt !=
		rd.allDefsInBlock[block][var.varID].end(); dIt++) {
	    if (dIt->lineNumber < line) {
		lastDef = dIt->clone();
		foundDef = true;
	    }
	}

	if (!foundDef) {
	    llvm::outs() << "ERROR: Cannot find definition of var " 
			 << var.varID << " in block " << block << "\n";
	    rv = false;
	    return defs;
	}

	//defs.insert(lastDef);
	defs.push_back(lastDef);
    } else {
	if (rd.reachInSet.find(block) == rd.reachInSet.end()) {
	    llvm::outs() << "ERROR: Cannot find reachInSet of block " 
			 << block << "\n";
	    rv = false;
	    return defs;
	}

	if (rd.reachInSet[block].find(var.varID) ==
		rd.reachInSet[block].end()) {
	    llvm::outs() << "ERROR: Cannot find reachInSet of var " 
			 << var.varID << " in block " << block << "\n";
	    rv = false;
	    return defs;
	}

	//defs.insert(rd.reachInSet[block][var.varID].begin(), 
	    //rd.reachInSet[block][var.varID].end());
	defs.insert(defs.end(), rd.reachInSet[block][var.varID].begin(), 
	    rd.reachInSet[block][var.varID].end());
    }

    return defs;
}

LoopDetails* FunctionAnalysis::getImmediateLoopOfBlock(unsigned block, bool &rv) {
    rv = true;
    LoopDetails* immLoop = nullptr;
    std::vector<LoopDetails*> allLoops = getLoopsOfBlock(block, rv);
    if (!rv)
	return immLoop;

    if (allLoops.size() == 1) {
	immLoop = allLoops[0];
	return immLoop;
    }

    bool foundImmLoop = false;
    for (std::vector<LoopDetails*>::iterator lIt = allLoops.begin(); lIt != 
	    allLoops.end(); lIt++) {
	bool innerLoopPresent = false;
	for (std::vector<LoopDetails*>::iterator it = lIt+1; it !=
		allLoops.end(); it++) {
	    if ((*lIt)->loopStartLine == (*it)->loopStartLine && (*lIt)->loopEndLine == 
		    (*it)->loopEndLine) {
		llvm::outs() << "ERROR: Found duplicate entries of loop while "
			     << "finding all loops of block " << block << "\n";
		(*lIt)->print();
		rv = false;
		return immLoop;
	    }

	    if ((*lIt)->loopStartLine <= (*it)->loopStartLine && (*it)->loopEndLine <= 
		    (*it)->loopEndLine) {
		innerLoopPresent = true;
		break;
	    }
	}

	if (!innerLoopPresent) {
	    //immLoop = (*lIt)->clone();
	    immLoop = (*lIt);
	    foundImmLoop = true;
	    break;
	}
    }

    if (!foundImmLoop) {
	llvm::outs() << "ERROR: Cannot find immediate loop of block " << block
		     << "\n";
	rv = false;
	return immLoop;
    }

    return immLoop;
}

std::vector<std::vector<std::pair<unsigned, bool> > > FunctionAnalysis::getGuardsOfBlock(unsigned currBlock) {
    std::vector<std::vector<std::pair<unsigned, bool> > > guards;
    if (transitiveCDG.find(currBlock) != transitiveCDG.end()) {
#if 0
	guards.insert(guards.end(), transitiveCDG[currBlock].begin(),
	    transitiveCDG[currBlock].end());
#endif
	for (std::vector<std::vector<std::pair<unsigned, bool> > >::iterator gIt
		= transitiveCDG[currBlock].begin(); gIt !=
		transitiveCDG[currBlock].end(); gIt++) {
	    std::vector<std::pair<unsigned, bool> > gVec;
	    for (std::vector<std::pair<unsigned, bool> >::iterator vIt =
		    gIt->begin(); vIt != gIt->end(); vIt++) {
		bool isLoopHeader = false;
		for (std::map<std::pair<unsigned, unsigned>,
			LoopDetails*>::iterator lIt = edgesToLoopDetails.begin(); 
			lIt != edgesToLoopDetails.end(); lIt++) {
		    if (vIt->first == lIt->second->loopHeaderBlock) {
			isLoopHeader = true;
			break;
		    }
		}
		if (!isLoopHeader) 
		    gVec.push_back(std::make_pair(vIt->first, vIt->second));
	    }
	    if (gVec.size() != 0)
		guards.push_back(gVec);
	}
    }
    return guards;
}

std::vector<std::vector<GUARD> > FunctionAnalysis::getGuardExprsOfBlock(unsigned currBlock,
bool &rv) {
    std::vector<std::vector<GUARD> > guardExprs;
    std::vector<std::vector<std::pair<unsigned, bool> > > guards = getGuardsOfBlock(currBlock);

#if 0
    std::unique_ptr<CFG> funcCFG = function.constructCFG(rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot construct CFG of function:\n";
	function.print();
	llvm::outs() << "ERROR: In FunctionAnalysis::getGuardExprsOfBlock\n";
	return guardExprs;
    }
#endif
    for (std::vector<std::vector<std::pair<unsigned, bool> > >::iterator gIt = guards.begin();
	    gIt != guards.end(); gIt++) {
	std::vector<GUARD> guardVec;
	for (std::vector<std::pair<unsigned, bool> >::iterator vIt =
		gIt->begin(); vIt != gIt->end(); vIt++) {
	    CFGBlock* guardBlock = getBlockFromID(function->cfg, vIt->first);
	    if (!guardBlock) {
		llvm::outs() << "ERROR: Cannot get block from ID " << vIt->first << "\n";
		rv = false;
		return guardExprs;
	    }

	    Stmt* guardStmt = guardBlock->getTerminatorCondition();
	    if (!isa<Expr>(guardStmt)) {
		llvm::outs() << "ERROR: TerminatorCondition of block " << vIt->first 
			     << " is not an expr\n";
		rv = false;
		return guardExprs;
	    }

	    ExprDetails* gEx = ExprDetails::getObject(SM, dyn_cast<Expr>(guardStmt),
		vIt->first, rv);
	    if (!rv) return guardExprs;
	    gEx->blockID = vIt->first;
	    gEx->lineNum = SM->getExpansionLineNumber(guardStmt->getLocStart());
	    gEx->origExpr = dyn_cast<Expr>(guardStmt)->IgnoreParenCasts();
	    guardVec.push_back(std::make_pair(gEx, vIt->second));
	}

	guardExprs.push_back(guardVec);
    }

    return guardExprs;
}

void FunctionAnalysis::computeTransitiveControlDependences(bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering FunctionAnalysis::computeTranstiveControlDependences()\n";
#endif
    rv = true;

    // For each block, find all transitive dependences
    for (std::map<unsigned, std::set<std::pair<unsigned, bool> > >::iterator cIt
	    = cdg.begin(); cIt != cdg.end(); cIt++) {
	unsigned currNode = cIt->first;
	std::vector<std::vector<std::pair<unsigned, bool> > > allPaths;
	std::stack<unsigned> dfsStack;
	std::map<unsigned, std::set<unsigned> > processedBlocks;
	//processedBlocks.insert(currNode);
	dfsStack.push(currNode);
#ifdef DEBUG
	llvm::outs() << "DEBUG: Pushed " << currNode << " to stack\n";
#endif

	std::vector<std::pair<unsigned, bool> > currPath;
	while (!dfsStack.empty()) {
	    unsigned currBlock = dfsStack.top();
	    //dfsStack.pop();
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Processing " << currBlock << "\n";
#endif

	    // add adjacent node to stack and to the currPath
	    if (cdg.find(currBlock) == cdg.end()) {
		llvm::outs() << "ERROR: Cannot find CDG entry for block "
			     << currBlock << "\n";
		rv = false;
		return;
	    }
	    std::set<std::pair<unsigned, bool> >::iterator nIt;
	    bool insertedBlock = false;
	    for (nIt = cdg[currBlock].begin(); nIt != cdg[currBlock].end();
		    nIt++) {
#ifdef DEBUG
		llvm::outs() << "Adjacent Block: " << nIt->first << "\n";
#endif
		// Check if this block is already present in the curr path,
		// then skip it.
		bool foundInPath = false;
		for (std::vector<std::pair<unsigned, bool> >::iterator pIt = 
			currPath.begin(); pIt != currPath.end(); pIt++) {
		    if (pIt->first == nIt->first) {
			foundInPath = true;
			break;
		    }
		}
		if (foundInPath && nIt->first != currBlock) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: There might be a cycle in the CDG.\n";
		    llvm::outs() << "DEBUG: The cycle is from " << nIt->first 
				 << " to " << currBlock << "\n";
		    llvm::outs() << "DEBUG: currPath: ";
		    for (std::vector<std::pair<unsigned, bool> >::iterator pIt = 
			    currPath.begin(); pIt != currPath.end(); pIt++)
			llvm::outs() << pIt->first << " ";
		    llvm::outs() << ": " << nIt->first << "\n";
#endif
		    // Check if the cycle is due to a break in any loop. You go
		    // through the path till now and see the edge break guard to
		    // loop header is part of the path.
		    bool cycleDueToBreak = false;
		    for (std::vector<LoopDetails*>::iterator loopIt =
			    allLoopsInFunction.begin(); loopIt != 
			    allLoopsInFunction.end(); loopIt++) {
			// Skip if loop does not have a break
			if (!(*loopIt)->hasBreak) continue;
			// nIt->first should be the loop header block and the
			// currBlock should be the guard of some breakstmt.
			unsigned loopHeaderBlock = (*loopIt)->loopHeaderBlock;
			for (std::map<unsigned, std::set<unsigned> >::iterator breakIt =
				(*loopIt)->blockIDsOfAllBreaksInLoopToBlocksGuardingThem.begin();
				breakIt != (*loopIt)->blockIDsOfAllBreaksInLoopToBlocksGuardingThem.end();
				breakIt++) {
			  for (std::set<unsigned>::iterator bguardIt =
				breakIt->second.begin(); bguardIt !=
				breakIt->second.end(); bguardIt++) {
			    unsigned guardBlock = *bguardIt;
			    // Go through current path see successor of guard
			    // block
			    for (std::vector<std::pair<unsigned, bool>
				    >::iterator pathIt = currPath.begin(); 
				    pathIt != currPath.end(); pathIt++) {
				if (pathIt+1 != currPath.end()) {
				    if (pathIt->first == guardBlock &&
					    (pathIt+1)->first == loopHeaderBlock) {
					cycleDueToBreak = true;
					break;
				    }
				} else {
				    if (pathIt->first == guardBlock &&
					    nIt->first == loopHeaderBlock) {
					cycleDueToBreak = true;
					break;
				    }
				}
			    }
			    if (cycleDueToBreak) break;
			  }
			  if (cycleDueToBreak) break;
			}
			if (cycleDueToBreak) break;

			for (std::vector<LoopDetails*>::iterator nestIt =
				(*loopIt)->loopsNestedWithin.begin(); nestIt !=
				(*loopIt)->loopsNestedWithin.end(); nestIt++) {
			    for (std::vector<std::pair<unsigned, bool>
				    >::iterator pathIt = currPath.begin(); 
				    pathIt != currPath.end(); pathIt++) {
				if (pathIt+1 != currPath.end()) {
				    if (pathIt->first == (*nestIt)->loopHeaderBlock &&
					(pathIt+1)->first == loopHeaderBlock) {
					cycleDueToBreak = true;
					break;
				    }
				} else {
				    if (pathIt->first == (*nestIt)->loopHeaderBlock &&
					    nIt->first == loopHeaderBlock) {
					cycleDueToBreak = true;
					break;
				    }
				}
			    }
			    if (cycleDueToBreak) break;
			}
			if (cycleDueToBreak) break;
		    }
		    if (cycleDueToBreak) {
			if (processedBlocks.find(currBlock) ==
				processedBlocks.end() ||
				processedBlocks[currBlock].find(nIt->first) ==
				processedBlocks[currBlock].end())
			    processedBlocks[currBlock].insert(nIt->first);
			continue;
		    } else {
			llvm::outs() << "ERROR: Cycle is not due to break in loop\n";
			llvm::outs() << "ERROR: There might be a cycle in the CDG.\n";
			rv = false;
			return;
		    }
		}
		if (nIt->first == dummyEntryBlockID) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: dummyEntryBlockID\n";
#endif
		    // Then we reached the end of a path. Insert the curr path to
		    // allPaths.
		    allPaths.push_back(currPath);
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Found path:\n";
		    for (std::vector<std::pair<unsigned, bool> >::iterator pIt =
			    currPath.begin(); pIt != currPath.end(); pIt++)
			llvm::outs() << pIt->first << " ";
		    llvm::outs() << "\nAdding " << currBlock
				 << " to processedBlocks\n";
#endif
		    //currPath.clear();
		    //dfsStack.push(nIt->first);
		    //insertedBlock = true;
		    processedBlocks[currBlock].insert(nIt->first);
		    break;
		} else if (nIt->first != dummyEntryBlockID && nIt->first != currBlock &&
			nIt->first != currNode && 
			(processedBlocks.find(currBlock) == processedBlocks.end()
			|| processedBlocks[currBlock].find(nIt->first) ==
			processedBlocks[currBlock].end())) {
		    if (foundInPath) continue;
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Adding " << nIt->first << " to path, "
				 << "dfsStack, processedBlocks\n";
#endif
		    currPath.push_back(std::make_pair(nIt->first, nIt->second));
		    dfsStack.push(nIt->first);
		    insertedBlock = true;
		    processedBlocks[currBlock].insert(nIt->first);
		    break;
		} else if (processedBlocks.find(nIt->first) !=
			    processedBlocks.end()) {
		    continue;
		}
	    }
	    llvm::outs() << "\n";
	    std::set<std::pair<unsigned, bool> >::iterator tempIt = nIt;
	    tempIt++;
	    if (!insertedBlock && (nIt == cdg[currBlock].end() || 
		    tempIt == cdg[currBlock].end())) {
		//processedBlocks.insert(currBlock);
		if (processedBlocks.find(currBlock) != processedBlocks.end()) {
		    processedBlocks.erase(processedBlocks.find(currBlock));
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Erasing " << currBlock 
				 << " from processedBlocks\n";
#endif
		}
		dfsStack.pop();
		if (currPath.size() != 0) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: currPath: ";
		    for (std::vector<std::pair<unsigned, bool> >::iterator pIt =
			    currPath.begin(); pIt != currPath.end(); pIt++) {
			llvm::outs() << pIt->first << " ";
		    }
		    llvm::outs() << "\n";
#endif
		    std::vector<std::pair<unsigned, bool> >::iterator pIt = currPath.end()-1;
		    if (pIt->first == currBlock) {
			currPath.erase(pIt);
#ifdef DEBUG
			llvm::outs() << "DEBUG: Last element in path cleared\n";
#endif
		    }
		}
#ifdef DEBUG
		llvm::outs() << "DEBUG: Stack popped\n";
		if (dfsStack.empty())
		    llvm::outs() << "DEBUG: Stack empty\n";
		else
		    llvm::outs() << "DEBUG: Top of stack is " << dfsStack.top()
				 << "\n";
#endif
	    }
	}
	if (transitiveCDG.find(currNode) == transitiveCDG.end()) {
	    transitiveCDG[currNode].insert(transitiveCDG[currNode].end(),
		allPaths.begin(), allPaths.end());
	} else {
	    llvm::outs() << "ERROR: transitiveCDG already contains an entry for "
			 << "block " << currNode << "\n";
	    for (std::vector<std::vector<std::pair<unsigned, bool> > >::iterator 
		    vIt = transitiveCDG[currNode].begin(); vIt !=
		    transitiveCDG[currNode].end(); vIt++) {
		for (std::vector<std::pair<unsigned, bool> >::iterator sIt =
			vIt->begin(); sIt != vIt->end(); sIt++) {
		    llvm::outs() << "(" << sIt->first << ", " 
				 << (sIt->second? "true": "false") << ") ";
		}
		llvm::outs() << "\n";
	    }
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: Transitive Control dependences:\n";
    for (std::map<unsigned, std::vector<std::vector<std::pair<unsigned, bool> > > >::iterator tIt
	    = transitiveCDG.begin(); tIt != transitiveCDG.end(); tIt++) {
	llvm::outs() << "Block " << tIt->first << ": ";
	for (std::vector<std::vector<std::pair<unsigned, bool> > >::iterator vIt =
		tIt->second.begin(); vIt != tIt->second.end(); vIt++) {
	    for (std::vector<std::pair<unsigned, bool> >::iterator sIt =
		    vIt->begin(); sIt != vIt->end(); sIt++) {
		llvm::outs() << "(" << sIt->first << ", " 
			     << (sIt->second? "true": "false") << ") ";
	    }
	    llvm::outs() << "\n";
	}
	llvm::outs() << "\n";
    }
#endif

    for (std::map<unsigned, std::vector<std::vector<std::pair<unsigned, bool> >
	    > >::iterator tIt = transitiveCDG.begin(); tIt !=
	    transitiveCDG.end(); tIt++) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Looking at block " << tIt->first << "\n";
#endif
	for (std::vector<std::vector<std::pair<unsigned, bool> > >::iterator vIt
		= tIt->second.begin(); vIt != tIt->second.end(); vIt++) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Has " << tIt->second.size() << " paths\n";
#endif
	    std::vector<std::pair<unsigned, bool> > finalSet;
	    for (std::vector<std::pair<unsigned, bool> >::iterator p1It =
		    vIt->begin(); p1It != vIt->end(); p1It++) {
#ifdef DEBUG
		llvm::outs() << "DEBUG: p1: (" << p1It->first << ", "
			     << (p1It->second? "true": "false") << ")\n";
#endif
		if (vIt->size() == 1) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: Singleton path\n";
#endif
		    finalSet.insert(finalSet.end(), vIt->begin(), vIt->end());
		    //continue;
		    break;
		}
		// If p1 is not a sub expr of any expression, then insert it to
		// finalSet
		if (blockToSupersetBlocks.find(p1It->first) == 
			blockToSupersetBlocks.end()) {
		    std::pair<unsigned, bool> p(p1It->first, p1It->second);
		    if (std::find(finalSet.begin(), finalSet.end(), p) == 
			    finalSet.end()) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: p1 is not a subexpr\n";
			llvm::outs() << "DEBUG: Inserting (" << p.first << ", "
				     << (p.second? "true": "false") << ")\n";
#endif
			finalSet.push_back(p);
			continue;
		    }
		}
		bool isSubExpr = false;
		for (std::vector<std::pair<unsigned, bool> >::iterator p2It = 
			vIt->begin(); p2It != vIt->end(); p2It++) {
#ifdef DEBUG
		    llvm::outs() << "DEBUG: p2: (" << p2It->first << ", "
				 << (p2It->second? "true": "false") << ")\n";
#endif
		    if (p1It == p2It) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: Skipping...\n";
#endif
			continue;
		    }
		    if (blockToSupersetBlocks[p1It->first].find(p2It->first) != 
			    blockToSupersetBlocks[p1It->first].end()) {
			isSubExpr = true;
			break;
		    }
		}
		if (!isSubExpr) {
		    std::pair<unsigned, bool> p(p1It->first, p1It->second);
		    if (std::find(finalSet.begin(), finalSet.end(), p) == 
			    finalSet.end()) {
#ifdef DEBUG
			llvm::outs() << "DEBUG: p1 is not a subexpr of anything in the path\n";
			llvm::outs() << "DEBUG: Inserting (" << p.first << ", "
				     << (p.second? "true": "false") << ")\n";
#endif
			finalSet.push_back(p);
			continue;
		    }
		}
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: finalSet: ";
	    for (std::vector<std::pair<unsigned, bool> >::iterator fIt = 
		    finalSet.begin(); fIt != finalSet.end(); fIt++) {
		llvm::outs() << "(" << fIt->first << ", " 
			     << (fIt->second?"true":"false") << ") ";
	    }
	    llvm::outs() << "\n";
#endif
	    vIt->clear();
	    vIt->insert(vIt->end(), finalSet.begin(),
		finalSet.end());
	}
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: New Transitive Control dependences:\n";
    for (std::map<unsigned, std::vector<std::vector<std::pair<unsigned, bool> > > >::iterator tIt
	    = transitiveCDG.begin(); tIt != transitiveCDG.end(); tIt++) {
	llvm::outs() << "Block " << tIt->first << ": ";
	for (std::vector<std::vector<std::pair<unsigned, bool> > >::iterator vIt =
		tIt->second.begin(); vIt != tIt->second.end(); vIt++) {
	    for (std::vector<std::pair<unsigned, bool> >::iterator sIt =
		    vIt->begin(); sIt != vIt->end(); sIt++) {
		llvm::outs() << "(" << sIt->first << ", " 
			     << (sIt->second? "true": "false") << ") ";
	    }
	    llvm::outs() << "\n";
	}
	llvm::outs() << "\n";
    }
#endif
}

ExprDetails* FunctionAnalysis::getTerminatorExpressionOfBlock(unsigned
currBlock, bool &rv) {

    rv = true;

    if (currBlock == dummyEntryBlockID) {
	llvm::outs() << "ERROR: Trying to obtain terminator expression of dummy entry block:\n";
	rv = false;
	return nullptr;
    }

    ExprDetails* gEx = nullptr;
#if 0
    std::unique_ptr<CFG> cfg = function.constructCFG(rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot construct CFG of function:\n";
	function.print();
	return gEx;
    }
#endif
    CFGBlock* guardBlock = getBlockFromID(function->cfg, currBlock);
    if (!guardBlock) {
	llvm::outs() << "ERROR: Cannot get block from ID " << currBlock << "\n";
	rv = false;
	return gEx;
    }

    Stmt* guardStmt = guardBlock->getTerminatorCondition();
    if (!guardStmt) {
	llvm::outs() << "ERROR: Cannot obtain terminator condition of block "
		     << currBlock << "\n";
	rv = false;
	return gEx;
    }

    if (!isa<Expr>(guardStmt)) {
	llvm::outs() << "ERROR: TerminatorCondition of block " << currBlock 
		     << " is not an expr\n";
	rv = false;
	return gEx;
    }

    gEx = ExprDetails::getObject(SM, dyn_cast<Expr>(guardStmt), currBlock, rv);
    if (!rv) {
	llvm::outs() << "ERROR: Cannot get ExprDetails of terminator condition\n";
	return gEx;
    }

    return gEx;
}

bool LoopDetails::isTestCaseLoop(InputVar* testCaseVar, bool &rv) {
    rv = true;
    // TODO: this function needs to be better.
    if (loopIndexVar.equals(testCaseVar->var)) return true;
#ifdef DEBUG
    llvm::outs() << "DEBUG: loopIndexVar:";
    loopIndexVar.print();
    llvm::outs() << "\nDEBUG: testCaseVar:";
    testCaseVar->var.print();
    llvm::outs() << "\n";
#endif
    if (!loopIndexInitValSymExpr) {
	llvm::outs() << "ERROR: loopIndexInitValSymExpr is not set\n";
	rv = false;
	return false;
    }
    if (!loopIndexFinalValSymExpr) {
	llvm::outs() << "ERROR: loopIndexFinalValSymExpr is not set\n";
	rv = false;
	return false;
    }
    bool skipcase = false;
    // Case 1: initial value is testcase var. Final value is 0. Step is -1.
    // Bound is strict.
    if (isa<SymbolicDeclRefExpr>(loopIndexInitValSymExpr) &&
	isa<SymbolicIntegerLiteral>(loopIndexFinalValSymExpr)) {
	SymbolicDeclRefExpr* sdre =
	    dyn_cast<SymbolicDeclRefExpr>(loopIndexInitValSymExpr);
	SymbolicIntegerLiteral* sil = 
	    dyn_cast<SymbolicIntegerLiteral>(loopIndexFinalValSymExpr);

	while (sdre->substituteExpr) {
	    if (isa<SymbolicDeclRefExpr>(sdre->substituteExpr)) {
		sdre = dyn_cast<SymbolicDeclRefExpr>(sdre->substituteExpr);
	    } else {
#ifdef DEBUG
		llvm::outs() << "DEBUG: substitute expr of initval is not <SymbolicDeclRefExpr>\n";
#endif
#if 0
		rv = false;
		return false;
#endif
		skipcase = true;
		break;
	    }
	}

	if (!skipcase) {
	    if (sdre->var.equals(testCaseVar->var) && sil->val == 0 && strictBound &&
		    !upperBound) {
		return true;
	    } else if (sdre->var.equals(testCaseVar->var) && sil->val == 1 && !strictBound &&
		    !upperBound) {
	    return true;
	    } 
	}
    }

    skipcase = false;
    // Case 2: initial value is 0. Final value is testcase var. Step is +1.
    // Bound is strict.
    if (isa<SymbolicDeclRefExpr>(loopIndexFinalValSymExpr) &&
	isa<SymbolicIntegerLiteral>(loopIndexInitValSymExpr)) {
#ifdef DEBUG
	llvm::outs() << "DEBUG: Case 2 \n";
#endif
	SymbolicDeclRefExpr* sdre =
	    dyn_cast<SymbolicDeclRefExpr>(loopIndexFinalValSymExpr);
	SymbolicIntegerLiteral* sil = 
	    dyn_cast<SymbolicIntegerLiteral>(loopIndexInitValSymExpr);

#ifdef DEBUG
	llvm::outs() << "DEBUG: Before finding substituteExpr:\n";
	sdre->prettyPrint(false);
	llvm::outs() << "\n";
#endif
	while (sdre->substituteExpr) {
	    if (isa<SymbolicDeclRefExpr>(sdre->substituteExpr)) {
		sdre = dyn_cast<SymbolicDeclRefExpr>(sdre->substituteExpr);
	    } else {
#ifdef DEBUG
		llvm::outs() << "DEBUG: substitute expr of finalval is not <SymbolicDeclRefExpr>\n";
#endif
#if 0
		rv = false;
		return false;
#endif
		skipcase = true;
		break;
	    }
	}

	if (!skipcase) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: After finding substituteExpr:\n";
	    sdre->prettyPrint(false);
	    llvm::outs() << "\n";

	    llvm::outs() << "DEBUG: Testcasevar: ";
	    testCaseVar->var.print();
	    llvm::outs() << "\n";
	    print();
	    if (sdre->var.equals(testCaseVar->var)) llvm::outs() << "Equal\n";
	    else llvm::outs() << "Not equal\n";
	    sil->prettyPrint(false);
	    llvm::outs() << "\n";
#endif

	    if (sdre->var.equals(testCaseVar->var) && sil->val == 0 && strictBound &&
		    upperBound) {
		return true;
	    } else if (sdre->var.equals(testCaseVar->var) && sil->val == 1 && !strictBound &&
		    upperBound) {
		return true;
	    }
	}
    }
    return false;
}

void LoopDetails::prettyPrint(std::ofstream &logFile, 
const MainFunction* mObj, bool &rv, bool inputFirst, bool inResidual, 
std::vector<LoopDetails> surroundingLoops) {
    rv = true;
    logFile << "loop(";
    if (loopStep == 1) logFile << "1";
    else if (loopStep == -1) logFile << "-1";
    else {
	llvm::outs() << "ERROR: Loop step is not 1 or -1\n";
	print();
	rv = false;
	return;
    }
    logFile << ", " << loopIndexVar.varName << ", " ;
    if (loopIndexInitValSymExpr) {
	//llvm::outs() << Helper::prettyPrintExpr(loopIndexInitValDef.expression);
	loopIndexInitValSymExpr->prettyPrintSummary(logFile, mObj, false, rv,
	    inputFirst, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: while prettyprinting loopIndexInitValSymExpr\n";
	    return;
	}
    } else {
	llvm::outs() << "ERROR: NULL loopIndexInitValSymExpr\n";
	print();
	rv = false;
	return;
    }
    logFile << ", ";
    if (loopIndexFinalValSymExpr) {
	//llvm::outs() << Helper::prettyPrintExpr(loopIndexFinalValDef.expression);
	loopIndexFinalValSymExpr->prettyPrintSummary(logFile, mObj, false, rv,
	    inputFirst, inResidual, surroundingLoops);
	if (!rv) {
	    llvm::outs() << "ERROR: while prettyprinting loopIndexFinalValSymExpr\n";
	    return;
	}
	if (strictBound && loopStep == 1)
	    logFile << " - 1";
	else if (strictBound && loopStep == -1)
	    logFile << " + 1";
    } else {
	llvm::outs() << "ERROR: NULL loopIndexFinalValSymExpr\n";
	print();
	rv = false;
	return;
    }
    logFile << ") [" << loopStartLine << "] {\n";
}

void LoopDetails::prettyPrintHere() {
    llvm::outs() << "loop(";
    if (loopStep == 1) llvm::outs() << "1";
    else if (loopStep == -1) llvm::outs() << "-1";
    else {
	llvm::outs() << "ERROR: Loop step is not 1 or -1\n";
    }
    llvm::outs() << ", " << loopIndexVar.varName << ", " ;
    if (loopIndexInitValSymExpr) {
	loopIndexInitValSymExpr->prettyPrint(false);
    } else {
	if (loopIndexInitValDef.expression)
	    llvm::outs() << Helper::prettyPrintExpr(loopIndexInitValDef.expression);
    }
    llvm::outs() << ", ";
    if (loopIndexFinalValSymExpr) {
	loopIndexFinalValSymExpr->prettyPrint(false);
	if (strictBound && loopStep == 1)
	    llvm::outs() << " - 1";
	else if (strictBound && loopStep == -1)
	    llvm::outs() << " + 1";
    } else {
	if (loopIndexFinalValDef.expression) {
	    llvm::outs() << Helper::prettyPrintExpr(loopIndexFinalValDef.expression);
	    if (strictBound) llvm::outs() << " - 1";
	}
    }
    llvm::outs() << ") [" << loopStartLine << "] \n";
}

void FunctionAnalysis::populateBasicBlockDetails(std::unique_ptr<CFG> &cfg) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering FunctionAnalysis::populateBasicBlockDetails()\n";
    llvm::outs() << "DEBUG: CFG size: " << cfg->size() << "\n";
#endif
    for (CFG::const_iterator I = cfg->begin(), E = cfg->end(); I != E; ++I ) {
      unsigned j = 1;
      for (CFGBlock::const_iterator BI = (*I)->begin(), BEnd = (*I)->end() ;
           BI != BEnd; ++BI, ++j ) {        
        if (Optional<CFGStmt> SE = BI->getAs<CFGStmt>()) {
          const Stmt *stmt= SE->getStmt();
          std::pair<unsigned, unsigned> P((*I)->getBlockID(), j);
          StmtMap[stmt] = P;

          switch (stmt->getStmtClass()) {
            case Stmt::DeclStmtClass:
                DeclMap[cast<DeclStmt>(stmt)->getSingleDecl()] = P;
                break;
            case Stmt::IfStmtClass: {
              const VarDecl *var = cast<IfStmt>(stmt)->getConditionVariable();
              if (var)
                DeclMap[var] = P;
              break;
            }
            case Stmt::ForStmtClass: {
              const VarDecl *var = cast<ForStmt>(stmt)->getConditionVariable();
              if (var)
                DeclMap[var] = P;
              break;
            }
            case Stmt::WhileStmtClass: {
              const VarDecl *var =
                cast<WhileStmt>(stmt)->getConditionVariable();
              if (var)
                DeclMap[var] = P;
              break;
            }
            case Stmt::SwitchStmtClass: {
              const VarDecl *var =
                cast<SwitchStmt>(stmt)->getConditionVariable();
              if (var)
                DeclMap[var] = P;
              break;
            }
            case Stmt::CXXCatchStmtClass: {
              const VarDecl *var =
                cast<CXXCatchStmt>(stmt)->getExceptionDecl();
              if (var)
                DeclMap[var] = P;
              break;
            }
            default:
              break;
          }
        }
      }
    }

#ifdef DEBUG
    llvm::outs() << "DEBUG: StmtMap:\n";
    for (StmtMapTy::iterator sIt = StmtMap.begin(); sIt != StmtMap.end(); sIt++) {
	llvm::outs() << "Stmt: " << Helper::prettyPrintStmt(sIt->first) << "\n";
	llvm::outs() << "[B" << sIt->second.first << "." << sIt->second.second
		     << "]\n";
    }
#endif
}

bool FunctionAnalysis::checkIfSpecialExprsAreSane(bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering FunctionAnalysis::checkIfSpecialExprsAreSame()\n";
#endif
    rv = true;
 
    bool retVal = true;
    for (std::vector<SpecialExpr*>::iterator sIt = spExprs.begin(); sIt != 
	    spExprs.end(); sIt++) {
	// Check whether any of the loops have another loop index as the
	// initial/final value.
	bool sane = true;
	std::vector<VarDetails> loopIndexVarsTillNow;
	for (std::vector<LoopDetails*>::iterator lIt = (*sIt)->loops.begin(); lIt != 
		(*sIt)->loops.end(); lIt++) {
	    (*lIt)->getSymbolicValuesForLoopHeader(SM, symVisitor, rv);
	    if (!rv) return false;
	    if (!((*lIt)->loopIndexInitValSymExpr) &&
		    !((*lIt)->loopIndexFinalValSymExpr)) {
	    } else if (!((*lIt)->loopIndexInitValSymExpr)) {
	    } else if (!((*lIt)->loopIndexFinalValSymExpr)) {
	    } else {
		for (std::vector<VarDetails>::iterator iIt =
			loopIndexVarsTillNow.begin(); iIt != 
			loopIndexVarsTillNow.end(); iIt++) {
		    bool ret = (*lIt)->loopIndexInitValSymExpr->containsVar(*iIt, rv);
		    if (!rv) return ret;
		    if (ret) {
			(*sIt)->sane = false;
#ifdef DEBUG
			llvm::outs() << "DEBUG: Special Expr not sane\n";
			(*sIt)->print();
#endif
			sane = false;
			break;
			//rv = false;
			//return false;
		    }

		    ret = (*lIt)->loopIndexFinalValSymExpr->containsVar(*iIt, rv);
		    if (!rv) return ret;
		    if (ret) {
			(*sIt)->sane = false;
#ifdef DEBUG
			llvm::outs() << "DEBUG: Special Expr not sane\n";
			(*sIt)->print();
#endif
			sane = false;
			break;
			//rv = false;
			//return false;
		    }
		}
	    }
	    
	    loopIndexVarsTillNow.push_back((*lIt)->loopIndexVar);

	    if (!sane) break;
	}
	if (retVal && !sane) retVal = false;
    }

    if (spExprs.size() == 0) return true;
    // Update SymbolicSpecialExpr as well
#ifdef DEBUG
    llvm::outs() << "DEBUG: About to update SymbolicSpecialExpr\n";
#endif
    if (!symVisitor) {
	llvm::outs() << "ERROR: NULL symVisitor\n";
	rv = false;
	return false;
    }
    for (std::vector<std::pair<StmtDetails*, std::vector<SymbolicStmt*> >
	    >::iterator vIt = symVisitor->symExprMap.begin(); vIt != 
	    symVisitor->symExprMap.end(); vIt++) {
	for (std::vector<SymbolicStmt*>::iterator symIt = vIt->second.begin();
		symIt != vIt->second.end(); symIt++) {
	    if (!*symIt) {
		rv = false;
		return false;
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Looking at SymbolicStmt: ";
	    (*symIt)->prettyPrint(false);
	    llvm::outs() << "\n";
#endif
	    if (!isa<SymbolicSpecialExpr>(*symIt)) continue;
	    SymbolicSpecialExpr* sse = dyn_cast<SymbolicSpecialExpr>(*symIt);
#ifdef DEBUG
	    llvm::outs() << "DEBUG: Got symbolic specialExpr\n";
#endif
	    if (!sse->originalSpecialExpr) {
		llvm::outs() << "ERROR: NULL originalSpecialExpr in SSE\n";
		rv = false;
		return false;
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: originalSpecialExpr not NULL\n";
#endif
	    for (std::vector<SpecialExpr*>::iterator sIt = spExprs.begin(); sIt != 
		    spExprs.end(); sIt++) {
		if (!*sIt) {
		    rv = false;
		    return false;
		}
#ifdef DEBUG
		llvm::outs() << "DEBUG: Looking at special expr assignment: " 
			     << (*sIt)->assignmentLine << "(" 
			     << (sse->originalSpecialExpr->assignmentLine) 
			     << ") guardLine: " << (*sIt)->guardLine << "("
			     << (sse->originalSpecialExpr->guardLine) << ")\n";
		llvm::outs() << "DEBUG: scalarVar: ";
		(*sIt)->scalarVar.print();
		llvm::outs() << "\n";
		sse->originalSpecialExpr->scalarVar.print();
		llvm::outs() << "\n";
		llvm::outs() << "DEBUG: arrayVar: ";
		(*sIt)->arrayVar.print();
		llvm::outs() << "\n";
		sse->originalSpecialExpr->arrayVar.print();
		llvm::outs() << "\n";
#endif
		if ((*sIt)->equals(*(sse->originalSpecialExpr))) {
		    sse->originalSpecialExpr->sane = (*sIt)->sane;
		    sse->sane = (*sIt)->sane;
		    break;
		}
	    }
	}
    }
    return true;
}

void LoopDetails::print() {
    llvm::outs() << "LoopDetails: (" << loopStartLine << " - "
		 << loopEndLine << "): var: " << loopIndexVar.varName
		 << " step: " << loopStep << " " 
		 << (isDoWhileLoop? "do-while": "") << "\n";
    loopIndexVar.print();
    llvm::outs() << "\n";
    llvm::outs() << "InitVal: ";
    loopIndexInitValDef.print();
    if (loopIndexInitValSymExpr) loopIndexInitValSymExpr->print();
    else llvm::outs() << "NULL InitSymExpr\n";
    llvm::outs() << "FinalVal: ";
    loopIndexFinalValDef.print();
    if (loopIndexFinalValSymExpr) loopIndexFinalValSymExpr->print();
    else llvm::outs() << "NULL FinalSymExpr\n";
    llvm::outs() << (upperBound? "Upper" : "Lower") << " " 
		 << (strictBound? "Strict" : "Not strict") << "\n";
    llvm::outs() << "Loop header block: " << loopHeaderBlock << "\n";
    llvm::outs() << "Back Edge: (" << backEdge.first << ", " 
		 << backEdge.second << ")\n";
    llvm::outs() << "Induction vars:\n";
    for (std::vector<struct inductionVar>::iterator iIt = 
	    inductionVars.begin(); iIt != inductionVars.end(); iIt++) {
	//llvm::outs() << "var " << iIt->varName << " (decl "
		     //<< iIt->varDeclLine << "): ";
	llvm::outs() << "var ";
	iIt->var.print();
	llvm::outs() << "\n";
	for (std::vector<std::pair<int, int> >::iterator it = 
		iIt->inductionLinesAndSteps.begin(); it != 
		iIt->inductionLinesAndSteps.end(); it++) {
	    llvm::outs() << "(" << it->first << ", " << it->second 
			 << ")";
	}
	llvm::outs() << "\n";
    }
    llvm::outs() << "Set of Loop Index InitVals: " 
		 << setOfLoopIndexInitValDefs.size() << "\n";
    unsigned i = 0;
    for (std::vector<std::pair<Definition, SymbolicExpr*> >::iterator it =
	    setOfLoopIndexInitValDefs.begin(); it != 
	    setOfLoopIndexInitValDefs.end(); it++) {
	llvm::outs() << "Init Val " << ++i << ":\n";
	it->first.print();
    }

    if (hasBreak) {
	llvm::outs() << "Break blocks: ";
	for (std::map<unsigned, std::set<unsigned> >::iterator bIt =
		blockIDsOfAllBreaksInLoopToBlocksGuardingThem.begin(); bIt != 
		blockIDsOfAllBreaksInLoopToBlocksGuardingThem.end(); bIt++) {
	    llvm::outs() << "Break block: " << bIt->first << " - guards: ";
	    for (std::set<unsigned>::iterator gIt = bIt->second.begin(); gIt !=
		    bIt->second.end(); gIt++) {
		llvm::outs() << *gIt << " ";
	    }
	    llvm::outs() << "\n";
	}
	llvm::outs() << "\n";
    } else {
	llvm::outs() << "No break in loop\n";
    }

    llvm::outs() << "Loops nested within:\n";
    for (std::vector<LoopDetails*>::iterator nestIt = loopsNestedWithin.begin(); 
	    nestIt != loopsNestedWithin.end(); nestIt++) {
	llvm::outs() << (*nestIt)->loopIndexVar.varName << " (" 
		     << (*nestIt)->loopStartLine << "-" 
		     << (*nestIt)->loopEndLine << "): header "
		     << (*nestIt)->loopHeaderBlock << "\n";
    }
}

void LoopDetails::getSymbolicValuesForLoopHeader(
const SourceManager* SM, GetSymbolicExprVisitor* symVisitor, bool &rv) {
    rv = true;
    if (loopIndexInitValSymExpr && loopIndexFinalValSymExpr) return;

    for (std::vector<std::pair<Definition, SymbolicExpr*> >::iterator dIt =
	    setOfLoopIndexInitValDefs.begin(); dIt != 
	    setOfLoopIndexInitValDefs.end(); dIt++) {
	// If SymbolicExpr is already set of the def, skip
	if (dIt->first.expression) {
	    ExprDetails* initDetails = ExprDetails::getObject(SM, 
		const_cast<Expr*>(dIt->first.expression),
		dIt->first.blockID, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While obtaining ExprDetails of "
			     << Helper::prettyPrintExpr(dIt->first.expression)
			     << "\n";
		return;
	    }
#ifdef DEBUG
	    llvm::outs() << "DEBUG: initDetails: ";
	    initDetails->prettyPrint();
	    llvm::outs() << "\n";
#endif
	    std::vector<SymbolicStmt*> initSymExprs =
		symVisitor->getSymbolicExprs(initDetails, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: No symbolic exprs for "
			     << Helper::prettyPrintExpr(dIt->first.expression)
			     << "\n";
		return;
	    }
	    if (initSymExprs.size() > 1) {
		llvm::outs() << "ERROR: loopIndexInitVal "
			     << Helper::prettyPrintExpr(dIt->first.expression)
			     << " has more than one symexprs\n";
		rv = false;
		return;
	    } else if (initSymExprs.size() == 0) {
		llvm::outs() << "ERROR: No sym exprs for initial value of loop\n";
		prettyPrintHere();
		rv = false;
		return;
	    }
	    if (isa<SymbolicExpr>(initSymExprs[0])) {
		// Check if the init val is from another loop
		bool defFromAnotherLoop = false;
		for (std::vector<LoopDetails*>::iterator llIt =
			loopIndexInitValFromLoop.begin(); llIt != 
			loopIndexInitValFromLoop.end(); llIt++) {
		    if ((*llIt)->loopIndexFinalValDef.equals(dIt->first) && 
			    (*llIt)->strictBound) {
			SymbolicBinaryOperator* sbo = new SymbolicBinaryOperator;
			sbo->lhs = dyn_cast<SymbolicExpr>(initSymExprs[0]);
			if ((*llIt)->upperBound)
			    sbo->opKind = BinaryOperatorKind::BO_Add;
			else
			    sbo->opKind = BinaryOperatorKind::BO_Sub;
			llvm::APInt val(32, std::abs((*llIt)->loopStep));
			SymbolicIntegerLiteral* sil = new SymbolicIntegerLiteral;
			sil->val = val;
			sbo->rhs = sil;
			//loopIndexInitValSymExpr = sbo;
			dIt->second = sbo;
			defFromAnotherLoop = true;
			break;
		    }
		}
		if (!defFromAnotherLoop) {
		    //loopIndexInitValSymExpr = dyn_cast<SymbolicExpr>(initSymExprs[0]);
		    dIt->second = dyn_cast<SymbolicExpr>(initSymExprs[0]);
#ifdef DEBUG
		    llvm::outs() << "DEBUG: InitSymExpr:\n";
		    dIt->second->prettyPrint(true);
#endif
		}
	    } else {
		llvm::outs() << "ERROR: Symbolic expr of loopIndexInitVal is "
			     << "not <SymbolicExpr>\n";
		rv = false;
		return;
	    }
	} else if (dIt->first.expressionStr.compare("DP_INPUT") == 0) {
	    // Initialized with an input var. This is a special case where
	    // the loop index variable itself is an input var
	    SymbolicDeclRefExpr* inpSymExpr = new SymbolicDeclRefExpr;
	    inpSymExpr->vKind = SymbolicDeclRefExpr::VarKind::INPUTVAR;
	    inpSymExpr->var = loopIndexVar.clone();
	    //loopIndexInitValSymExpr = inpSymExpr;
	    dIt->second = inpSymExpr;
	} else if (dIt->first.expressionStr.compare("DP_UNDEFINED") == 0) {
	    // Uninitialized loopIndexVar.
#ifdef DEBUG
	    llvm::outs() << "DEBUG Garbage loopIndexInitValDef\n";
#endif
	    SymbolicDeclRefExpr* garbageSymExpr = new SymbolicDeclRefExpr;
	    garbageSymExpr->vKind = SymbolicDeclRefExpr::VarKind::GARBAGE;
	    //loopIndexInitValSymExpr = garbageSymExpr;
	    dIt->second = garbageSymExpr;
	} else if (dIt->first.expressionStr.compare("DP_GLOBAL_VAR") == 0) {
	    // Initialized with a global var. 
	    SymbolicDeclRefExpr* globalSymExpr = new SymbolicDeclRefExpr;
	    globalSymExpr->vKind = SymbolicDeclRefExpr::VarKind::GLOBALVAR;
	    globalSymExpr->var = loopIndexVar.clone();
	    
	    std::string prefix("DP_GLOBAL_VAR");
	    auto res = std::mismatch(prefix.begin(), prefix.end(),
		dIt->first.expressionStr.begin());
	    if (res.second != dIt->first.expressionStr.end()) {
		std::string id(res.second, dIt->first.expressionStr.end());
		globalSymExpr->callExprID = id;
	    }
	    
	    dIt->second = globalSymExpr;
	} else {
	    llvm::outs() << "ERROR: NULL loopIndexInitValDef.expression\n";
	    print();
	    rv = false;
	    return;
	}
    }

    // Check if all initial values are same
    for (std::vector<std::pair<Definition, SymbolicExpr*> >::iterator idIt = 
	    setOfLoopIndexInitValDefs.begin(); idIt !=
	    setOfLoopIndexInitValDefs.end(); idIt++) {
	if (setOfLoopIndexInitValDefs.size() == 1) break;
	if (idIt+1 == setOfLoopIndexInitValDefs.end()) break;
	if (!(idIt->second) || !((idIt+1)->second)) {
	    llvm::outs() << "ERROR: Null expression for init val def\n";
	    print();
	    if (!(idIt->second)) {
		idIt->first.print();
	    }
	    if (!((idIt+1)->second)) {
		(idIt+1)->first.print();
	    }
	    rv = false;
	    return;
	}
#ifdef DEBUG
	llvm::outs() << "DEBUG: Comparing idIt->second: ";
	idIt->second->prettyPrint(true);
	llvm::outs() << "\nidIt+1->second: ";
	(idIt+1)->second->prettyPrint(true);
	llvm::outs() << "\n";
#endif
	
	if (!(idIt->second->equals((idIt+1)->second))) {
	    llvm::outs() << "ERROR: More than one definition for loop index var\n";
	    print();
	    rv = false;
	    return;
	}
    }
    loopIndexInitValDef = (setOfLoopIndexInitValDefs.begin())->first;
    loopIndexInitValSymExpr = (setOfLoopIndexInitValDefs.begin())->second;
#ifdef DEBUG
    llvm::outs() << "DEBUG: Found loopIndexInitValSymExpr:\n";
    loopIndexInitValSymExpr->prettyPrint(true);
    llvm::outs() << "\n";
#endif

    if (loopIndexFinalValDef.expressionStr.compare("--") == 0) {
	// Special case. while (x--)
	SymbolicIntegerLiteral* sil = new SymbolicIntegerLiteral;
	// Create llvm::APInt for 0
	llvm::APInt val(32, 0);
	sil->val = val;
	loopIndexFinalValSymExpr = sil;
	strictBound = true;
    } else if (loopIndexFinalValDef.expression) {
	ExprDetails* finalDetails = ExprDetails::getObject(SM, 
	    const_cast<Expr*>(loopIndexFinalValDef.expression),
	    loopIndexFinalValDef.blockID, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: While obtaining ExprDetails of "
			 << Helper::prettyPrintExpr(loopIndexFinalValDef.expression)
			 << "\n";
	    return;
	}
#ifdef DEBUG
	llvm::outs() << "DEBUG: finalDetails: ";
	finalDetails->prettyPrint();
	llvm::outs() << "\n";
#endif
	std::vector<SymbolicStmt*> finalSymExprs =
	    symVisitor->getSymbolicExprs(finalDetails, rv);
	if (!rv) {
	    llvm::outs() << "ERROR: No symbolic exprs for "
			 << Helper::prettyPrintExpr(loopIndexFinalValDef.expression)
			 << "\n";
	    // A hack, I think the loopIndexFinalValDef is not set correctly. So
	    // trying a different block in it
	    finalDetails = ExprDetails::getObject(SM,
		const_cast<Expr*>(loopIndexFinalValDef.expression), 
		loopHeaderBlock, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: While obtaining ExprDetails of "
			     << Helper::prettyPrintExpr(loopIndexFinalValDef.expression)
			     << "\n";
		return;
	    }
	    finalSymExprs = symVisitor->getSymbolicExprs(finalDetails, rv);
	    if (!rv) {
		llvm::outs() << "ERROR: No symbolic exprs for "
			     << Helper::prettyPrintExpr(loopIndexFinalValDef.expression)
			     << "\n";
		return;
	    }
	}
	if (finalSymExprs.size() > 1) {
	    llvm::outs() << "ERROR: loopIndexInitVal "
			 << Helper::prettyPrintExpr(loopIndexFinalValDef.expression)
			 << " has more than one symexprs\n";
	    rv = false;
	    return;
	} else if (finalSymExprs.size() == 0) {
	    llvm::outs() << "ERROR: No symexprs for final value of loop\n";
	    prettyPrintHere();
	    rv = false;
	    return;
	}
	if (isa<SymbolicExpr>(finalSymExprs[0])) {
	    loopIndexFinalValSymExpr = dyn_cast<SymbolicExpr>(finalSymExprs[0]);
#ifdef DEBUG
	    llvm::outs() << "DEBUG: finalSymExpr:\n";
	    loopIndexFinalValSymExpr->prettyPrint(true);
#endif
	} else {
	    llvm::outs() << "ERROR: Symbolic expr of loopIndexFinalVal "
			 << "is not <SymbolicExpr>\n";
	    rv = false;
	    return;
	}
    } else {
	llvm::outs() << "ERROR: NULL loopIndexFinalValDef.expression\n";
	print();
	rv = false;
	return;
    }
}

bool GuardedDefinition::equals(GuardedDefinition g) {
    if (!g.def.equals(def)) return false;
    if (g.guards.size() != guards.size()) return false;
    for (unsigned i = 0; i < guards.size(); i++) {
	if (g.guards[i].size() != guards[i].size()) return false;
	for (unsigned j = 0; j < guards[i].size(); j++) {
	    if (!(g.guards[i][j].first->equals(guards[i][j].first)))
		return false;
	    if (g.guards[i][j].second != guards[i][j].second) return false;
	}
    }

    return true;
}

void GuardedDefinition::print() {
    llvm::outs() << "Guarded Definition:\n";
    llvm::outs() << "Guards:\n";
    for (std::vector<std::vector<GUARD> >::iterator gIt = guards.begin(); gIt != 
	    guards.end(); gIt++) {
	for (std::vector<GUARD>::iterator vIt = gIt->begin(); vIt !=
		gIt->end(); vIt++) {
	    llvm::outs() << "(" << (vIt->second? "true" : "false") << ", ";
	    vIt->first->print();
	    llvm::outs() << ")";
	    if (vIt + 1 != gIt->end()) llvm::outs() << " && ";
	    llvm::outs() << "\n";
	}
	if (gIt + 1 != guards.end()) llvm::outs() << " || ";
	llvm::outs() << "\n";
    }
    def.print();
}

void FunctionAnalysis::setMainObjToNULL() {
    mainObj = NULL;
}

void LoopDetails::setLoopIndexInitValDef(FunctionAnalysis* FA, bool &rv) {
#ifdef DEBUG
    llvm::outs() << "DEBUG: Entering LoopDetails::setLoopIndexInitValDef()\n";
#endif
    rv = true;

    if (setOfLoopIndexInitValDefs.size() == 0) return;

    if (setOfLoopIndexInitValDefs.size() == 1) {
	std::pair<Definition, SymbolicExpr*> p;
	p.first = (setOfLoopIndexInitValDefs.begin())->first;
	p.second = (setOfLoopIndexInitValDefs.begin())->second;
	loopIndexInitValDef = p.first;
	return;
    }

    std::vector<LoopDetails*> loopIndexDefs;
    for (std::vector<std::pair<Definition, SymbolicExpr*> >::iterator dIt =
	    setOfLoopIndexInitValDefs.begin(); dIt != 
	    setOfLoopIndexInitValDefs.end(); dIt++) {
	bool isLoopIndex = FA->isLoopIndexVar(loopIndexVar,
	    dIt->first.blockID, rv);
	if (!rv) return;
	if (isLoopIndex) {
#ifdef DEBUG
	    llvm::outs() << "DEBUG: LoopIndexInitValDef is from a loop index\n";
#endif
	    std::vector<LoopDetails*> loops =
		FA->getLoopsOfBlock(dIt->first.blockID, rv);
	    if (!rv) return;
	    for (std::vector<LoopDetails*>::iterator it = loops.begin(); it
		    != loops.end(); it++) {
		if ((*it)->loopIndexVar.equals(loopIndexVar)) {
		    if ((*it)->hasBreak) {
			llvm::outs() << "ERROR: More than one definition for "
				     << "loop index var of loop: ";
			print();
			rv = false;
			return;
		    }
		    dIt->first.toDelete = true;
		    loopIndexDefs.push_back(*it);
		    break;
		}
	    }
	}
    }
    for (std::vector<std::pair<Definition, SymbolicExpr*> >::iterator dIt =
	    setOfLoopIndexInitValDefs.begin(); dIt != 
	    setOfLoopIndexInitValDefs.end(); dIt++) {
	if (dIt->first.toDelete) continue;
	for (std::vector<LoopDetails*>::iterator it =
		loopIndexDefs.begin(); it != loopIndexDefs.end(); it++) {
	    for (std::vector<std::pair<Definition, SymbolicExpr*> >::iterator initIt =
		    (*it)->setOfLoopIndexInitValDefs.begin(); initIt != 
		    (*it)->setOfLoopIndexInitValDefs.end(); initIt++) {
		if (dIt->first.equals(initIt->first)) {
		    dIt->first.toDelete = true;
		    break;
		}
	    }
	    if (dIt->first.toDelete) break;
	}
    }
    setOfLoopIndexInitValDefs.erase(
	std::remove_if(setOfLoopIndexInitValDefs.begin(),
	    setOfLoopIndexInitValDefs.end(),
	[](std::pair<Definition, SymbolicExpr*> d) { return d.first.toDelete; }),
	setOfLoopIndexInitValDefs.end());

    // If there are still other defs left in the set or there are more than
    // one defs in loopIndexDefs, we don't handle it.
    if (setOfLoopIndexInitValDefs.size() > 0 || 
	    loopIndexDefs.size() > 1) {
#if 0

	llvm::outs() << "ERROR: More than one definition for "
		     << "loop index var of loop: ";
	print();
	rv = false;
	return;
#endif
	for (std::vector<LoopDetails*>::iterator lIt = loopIndexDefs.begin(); lIt
		!= loopIndexDefs.end(); lIt++) {
	    std::pair<Definition, SymbolicExpr*> p;
	    p.first = (*lIt)->loopIndexFinalValDef;
	    p.second = NULL;
	    setOfLoopIndexInitValDefs.push_back(p);
	    loopIndexInitValFromLoop.push_back(*lIt);
	}
    } else {
	//setOfLoopIndexInitValDefs.push_back((*(loopIndexDefs.begin()))->loopIndexFinalValDef);
	std::pair<Definition, SymbolicExpr*> p;
	p.first = (*(loopIndexDefs.begin()))->loopIndexFinalValDef;
	p.second = NULL;
	setOfLoopIndexInitValDefs.push_back(p);
	loopIndexInitValDef = (*(loopIndexDefs.begin()))->loopIndexFinalValDef;
	loopIndexInitValFromLoop.push_back(*(loopIndexDefs.begin()));
    }
}

void Definition::print() const {
    llvm::outs() << "Def: (" << lineNumber << ", " << expressionStr 
		 << ", block " << blockID << ")\n";
#ifdef DEBUGFULL
    if (expression) {
	std::string str = Helper::prettyPrintExpr(expression);
	if (str.compare(expressionStr) != 0) {
	    llvm::outs() << "WARNING: expressionStr above is not same as "
			 << "expression: " << str << "\n";
	}
    }
#endif
    if (throughBackEdge) {
	llvm::outs() << "BackEdges: ";
	for (std::set<std::pair<unsigned, unsigned> >::iterator sIt = 
		backEdgesTraversed.begin(); sIt !=
		backEdgesTraversed.end(); sIt++) {
	    llvm::outs() << "(" << sIt->first << ", " << sIt->second << ") ";
	}
	llvm::outs() << "\n";
    }
}

void SpecialExpr:: print() {
    llvm::outs() << "Special expr: ";
    if (kind == MAX) llvm::outs() << "MAX";
    else if (kind == MIN) llvm::outs() << "MIN";
    else if (kind == SUM) llvm::outs() << "SUM";
    else if (kind == UNRESOLVED) llvm::outs() << "UNRESOLVED";
    else llvm::outs() << "Unknown ExprKind";

    llvm::outs() << "\n" 
		 << (fromTernaryOperator? "fromTernaryOperator": "not fromTernaryOperator");
    llvm::outs() << "\nScalar Var: ";
    scalarVar.print();
    llvm::outs() << "\nArray Var: ";
    arrayVar.print();
    llvm::outs() << "\n";

    if (arrayRange.first)
	llvm::outs() << "Lower bound: " 
		     << Helper::prettyPrintExpr(arrayRange.first) << "\n";
    if (arrayRange.second)
	llvm::outs() << "Upper bound: " 
		     << Helper::prettyPrintExpr(arrayRange.second) << "\n";
    llvm::outs() << "Array index exprs at assign line: ";
    for (std::vector<Expr*>::iterator iIt = arrayIndexExprsAtAssignLine.begin(); iIt != 
	    arrayIndexExprsAtAssignLine.end(); iIt++)
	if (*iIt != NULL)
	    llvm::outs() << Helper::prettyPrintExpr(*iIt) << " ";
    llvm::outs() << "\n";
    llvm::outs() << "Assignment line: " << assignmentLine << " (block "
		 << assignmentBlock << ")\n";
    llvm::outs() << "Guard line: " << guardLine << "\n";
    llvm::outs() << "Guard: ";
    if (guardExpr)
	llvm::outs() << Helper::prettyPrintExpr(guardExpr);
    else
	llvm::outs() << "NULL";
    llvm::outs() << "\n";
    llvm::outs() << "Initial val:\n";
    initialVal.print();

    if (isLoopSet) {
	llvm::outs() << "Loop Details:\n";
	for (std::vector<LoopDetails*>::iterator lIt = loops.begin(); lIt != 
		loops.end(); lIt++)
	    (*lIt)->print();
    }
    llvm::outs() << "Stmts part of expr: ";
    for (std::vector<int>::iterator it = stmtsPartOfExpr.begin(); it != 
	    stmtsPartOfExpr.end(); it++)
	llvm::outs() << *it << " ";
    llvm::outs() << "\n";
    llvm::outs() << (sane? "Sane" : "Not Sane") << "\n";
}

