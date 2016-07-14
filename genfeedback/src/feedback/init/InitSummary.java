package feedback.init;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.Set;
import java.util.Map.Entry;

import org.antlr.runtime.ANTLRFileStream;
import org.antlr.runtime.CharStream;
import org.antlr.runtime.CommonTokenStream;
import org.antlr.runtime.RecognitionException;
import org.antlr.runtime.TokenStream;
import org.antlr.runtime.tree.CommonTree;
import org.antlr.runtime.tree.CommonTreeNodeStream;

import feedback.ResultStats;
import feedback.init.parser.InitTreeWalker;
import feedback.init.parser.Rewrite;
import feedback.init.parser.initParseLexer;
import feedback.init.parser.initParseParser;
import feedback.init.parser.initParseParser.prog_return;
import feedback.inputs.ProgramInputs;
import feedback.logging.Logger;
import feedback.update.UpdateSummary;
import feedback.utils.AssignmentStatement;
import feedback.utils.Block;
import feedback.utils.Expression;
import feedback.utils.HelperFunctions;
import feedback.utils.Expression.ExprType;
import feedback.utils.LoopStatement;
import feedback.utils.Statement;
import feedback.utils.Variable;

public class InitSummary {
	
	public String progName;
	public String progInitSummaryName;
	public List<AssignmentStatement> initStmts;
	public List<AssignmentStatement> globalInits;
	public Logger ErrorLogger;
	public ProgramInputs Inps;
	public UpdateSummary fragments;
	
	public List<Block> blocks;
	
	public Map<String, Integer> arrayNameToDimensionSize;
	
	public List<Set<AssignmentStatement>> initWithoutSequencing;
	
	public boolean spuriousGlobalInit;
	
	public int score;
	
	public enum repairKind {
		MISSING,
		INCORRECT,
		SPURIOUS,
		NOREPAIR,
		USEREF
	}
	
	public enum heuristicKind {
		EXACTMATCH,
		MATCHLHSANDRHS,
		FLIPDIMANDMATCHLHSANDRHS
	}

	class repairSteps {
		repairKind repair;
		AssignmentStatement faultySummary;
		AssignmentStatement repairedSummary;
			
		repairSteps() {
			faultySummary = new AssignmentStatement(false, null, null, new Expression(ExprType.CONST, "", "false"));
			repairedSummary = new AssignmentStatement(false, null, null, new Expression(ExprType.CONST, "", "false"));
		}
		
		repairSteps(repairKind r, AssignmentStatement f, AssignmentStatement rep) {
			repair = r;
			faultySummary = f;
			repairedSummary = rep;
		}
	};

	class InitRepair {
		Set<repairSteps> steps;
		heuristicKind heuristic;
		String refSummaryUsed;	// progName of reference summary used
		int weightOfRepair;
		
		public InitRepair() {
			weightOfRepair = 0;
			refSummaryUsed = "";
			steps = new HashSet<repairSteps>();
		} 
				
		public String displayRepair(Map<String, Variable> submissionProgramVariables) {
			StringBuilder repairString = new StringBuilder();
//			repairString.append("RefSummaryUsed: " + refSummaryUsed + "\n");
//			repairString.append("WeightOfRepair: " + weightOfRepair + "\n");
//			repairString.append("Heuristic: " + heuristic + "\n\n");
			boolean useRef = false;
			for (repairSteps s: steps) {
				if (s.repair == repairKind.USEREF) {
					useRef = true;
					break;
				}
//				repairString.append("Repair Type: " + s.repair + "\n");
				if (s.faultySummary != null && s.repairedSummary != null) {
					repairString.append("Found Init: " + 
							HelperFunctions.rewriteRepair(s.faultySummary.toStringFeedback(false), submissionProgramVariables));
					repairString.append("It should be: " + 
							HelperFunctions.rewriteRepair(s.repairedSummary.toStringFeedback(false), submissionProgramVariables) + "\n");
				} else if (s.faultySummary != null && s.repairedSummary == null) {
					repairString.append("Remove Init: " + 
							HelperFunctions.rewriteRepair(s.faultySummary.toStringFeedback(false), submissionProgramVariables));
				} else if (s.faultySummary == null && s.repairedSummary != null) {
					repairString.append("Missing Init: " + 
							HelperFunctions.rewriteRepair(s.repairedSummary.toStringFeedback(false), submissionProgramVariables) + "\n");
				} 
			}

			if (useRef) {
				repairString.append("Use the following solution:\n");
				for (repairSteps s: steps) {
					repairString.append(HelperFunctions.rewriteRepair(s.repairedSummary.toStringFeedback(false), submissionProgramVariables));
				}
			}
			
			return HelperFunctions.rewriteRepair(repairString.toString(), submissionProgramVariables);
		}
	}
	public Set<InitRepair> generatedRepairs;
	public int minWeightOfRepair;
	public InitRepair minRepair;
	public List<Statement> allInits;
	private Map<String, Variable> programVariables;
	public boolean isRange;
	
	public InitSummary(String name, Logger Error, ProgramInputs I) {
		progName = name;
//		progInitSummaryName = progName.substring(0, progName.indexOf('.')) + ".init.summary";
		progInitSummaryName = progName + ".init.summary";
		score = 0;
		// Init stmts (with sequencing) read from the summary file.
		initStmts = new ArrayList<AssignmentStatement>();
		// Global init stmts read from the summary file. Distinct from initStmts.
		globalInits = new ArrayList<AssignmentStatement>();

		ErrorLogger = Error;
		Inps = I;
		
		arrayNameToDimensionSize = new HashMap<String, Integer>();
		initWithoutSequencing = new ArrayList<Set<AssignmentStatement>> ();
		
		spuriousGlobalInit = false;
		
		generatedRepairs = new HashSet<InitRepair>();
		minWeightOfRepair = -1;
		minRepair = new InitRepair();
	}
	
	public InitSummary(String refProgName, Logger errorLogger2, InitSummary I, Map<String, Variable> programVars) {
		progName = I.progName;
		progInitSummaryName = I.progInitSummaryName;
		initStmts = new ArrayList<AssignmentStatement>();
		for (AssignmentStatement s: I.initStmts)
			initStmts.add(s.clone());
		globalInits = new ArrayList<AssignmentStatement>();
		for (AssignmentStatement s: I.globalInits)
			globalInits.add(s.clone());
		ErrorLogger = I.ErrorLogger;
		Inps = I.Inps;
		programVariables = programVars;
		arrayNameToDimensionSize = new HashMap<String, Integer>();
		for (Map.Entry<String, Integer> entry: I.arrayNameToDimensionSize.entrySet())
			arrayNameToDimensionSize.put(entry.getKey(), entry.getValue());
		initWithoutSequencing = new ArrayList<Set<AssignmentStatement>>();
		for (Set<AssignmentStatement> set: I.initWithoutSequencing) {
			Set<AssignmentStatement> tempSet = new HashSet<AssignmentStatement>();
			for (AssignmentStatement s: set)
				tempSet.add(s.clone());
			initWithoutSequencing.add(tempSet);
		}
		spuriousGlobalInit = I.spuriousGlobalInit;

		generatedRepairs = new HashSet<InitRepair>();
		minWeightOfRepair = -1;
		minRepair = new InitRepair();
	}
	
	@Override
	public String toString() {
		String str;
		str = "initStmts:\n";
		for (AssignmentStatement s: initStmts)
			str += s.toString() + "\n";
		str += "globalInits:\n";
		for (AssignmentStatement s: globalInits)
			str += s.toString() + "\n";
		str += "initWithoutSequencing:\n";
		for (Set<AssignmentStatement> set: initWithoutSequencing) 
			for (AssignmentStatement s: set)
				str += s.toString() + "\n";
		return str;
	}
	
//	public boolean vanillaParse(){
//		String fileName = progInitSummaryName;
//		CharStream charStr;
//		try {
//			charStr = new ANTLRFileStream(fileName);
//		} catch (IOException e) {
//			ErrorLogger.writeLog(fileName);
//			ErrorLogger.writeLog("ERROR: IOException while reading file"
//					+ fileName);
//			return false;
//		}
//		
//		initParseLexer lexer = new initParseLexer(charStr);
//		TokenStream tokenStr = new CommonTokenStream(lexer);
//		initParseParser parser = new initParseParser(tokenStr);
//		prog_return prog;
//		try {
//			prog = parser.prog();
//		} catch (RecognitionException e) {
//			ErrorLogger.writeLog(fileName);
//			ErrorLogger.writeLog("Init: " + e.toString());
//			return false;
//		} catch (RuntimeException e) {
//			ErrorLogger.writeLog(fileName);
//			ErrorLogger.writeLog("Init: " + e.toString());
//			return false;
//		}
//		System.out.println(prog.getTree().toStringTree());
//		/*Rewrite parse tree to convert some loop statements to 
//		 * assignement statements*/
//		CommonTree t = (CommonTree)prog.getTree();
//	    CommonTreeNodeStream nodes = new CommonTreeNodeStream(t);
//	    Rewrite rwt = new Rewrite(nodes);
//	    t = (CommonTree)rwt.downup(t, true); // walk t, trace transforms
//	    
//	    // Walk the parse tree and gather the list of statements    
//	    CommonTreeNodeStream nodeStream = new CommonTreeNodeStream(t);
//	    InitTreeWalker walker = new InitTreeWalker(nodeStream);
//		try {
//			List<Statement> allInits = walker.prog();
//			
//			for (Statement stmt: allInits) {
//				AssignmentStatement s = (AssignmentStatement) stmt;	
//				if (s.isGlobalInit())
//					globalInits.add(s);
//				else
//					initStmts.add(s);
//			}	
//				/*for a particular init summary, map the dp array to its dim size*/
//			
//		} catch (RecognitionException e) {
//			ErrorLogger.writeLog(fileName);
//			ErrorLogger.writeLog("Init: " + e.toString());
//			return false;
//		} catch (RuntimeException e) {
//			ErrorLogger.writeLog(fileName);
//			ErrorLogger.writeLog("Init: " + e.toString());
//			return false;
//		}
//		return true;
//	}

	public void prepareReference(){
		for(Block b: blocks){
			b.computeConstraintsAsRef();
		}
	}
	
	public boolean parse() {
		String fileName = progInitSummaryName;
		CharStream charStr;
		try {
			charStr = new ANTLRFileStream(fileName);
		} catch (IOException e) {
			ErrorLogger.writeLog(fileName);
			ErrorLogger.writeLog("ERROR: IOException while reading file"
					+ fileName);
			return false;
		}
		
		initParseLexer lexer = new initParseLexer(charStr);
		TokenStream tokenStr = new CommonTokenStream(lexer);
		initParseParser parser = new initParseParser(tokenStr);
		prog_return prog;
		try {
			prog = parser.prog();
		} catch (RecognitionException e) {
			ErrorLogger.writeLog(fileName);
			ErrorLogger.writeLog("Init: " + e.toString());
			return false;
		} catch (RuntimeException e) {
			ErrorLogger.writeLog(fileName);
			ErrorLogger.writeLog("Init: " + e.toString());
			return false;
		}
//		System.out.println(prog.getTree().toStringTree());
		/*Rewrite parse tree to convert some loop statements to 
		 * assignement statements*/
		CommonTree t = (CommonTree)prog.getTree();
	    CommonTreeNodeStream nodes = new CommonTreeNodeStream(t);
	    Rewrite rwt = new Rewrite(nodes);
	    t = (CommonTree)rwt.downup(t, true); // walk t, trace transforms
	    
	    // Walk the parse tree and gather the list of statements    
	    CommonTreeNodeStream nodeStream = new CommonTreeNodeStream(t);
	    InitTreeWalker walker = new InitTreeWalker(nodeStream);
		try {
			allInits = walker.prog();
		} catch (RecognitionException e) {
			ErrorLogger.writeLog(fileName);
			ErrorLogger.writeLog("Init: " + e.toString());
			return false;
		} catch (RuntimeException e) {
			ErrorLogger.writeLog(fileName);
			ErrorLogger.writeLog("Init: " + e.toString());
			return false;
		}

		return true;
	}

	public boolean getAsRanges() {
		blocks = HelperFunctions.getAsBlocks(allInits);
		HelperFunctions.identifySuspicious(allInits, progName);
		List<AssignmentStatement> inits = new ArrayList<AssignmentStatement>();
		for(Statement stmt: allInits){
		AssignmentStatement s = null;
		if(stmt instanceof LoopStatement){
		  s = HelperFunctions.simplify((LoopStatement) stmt);
		  if(s == null)
			return false;
		}
		else
		  s = (AssignmentStatement) stmt;
		if("true".compareTo(s.getGuard().toString()) != 0)
		  return false;
		inits.add(s);
		}
		for (AssignmentStatement s: inits) {
  		  if (s.isGlobalInit())
  			  globalInits.add(s);
		  else
			  initStmts.add(s);
				
		/*for a particular init summary, map the dp array to its dim size*/
		Expression lhs = s.getLhs();
		int dimSize = lhs.geteList().size();
		if (!arrayNameToDimensionSize.containsKey(lhs.getName()))
			arrayNameToDimensionSize.put(lhs.getName(), dimSize);
		else if (arrayNameToDimensionSize.get(lhs.getName()) != dimSize) {
			ErrorLogger.writeLog(progInitSummaryName);
			ErrorLogger.writeLog("dimensionSize for array " + lhs.getName() + " is set to " +
							arrayNameToDimensionSize.get(lhs.getName()) + ", but statement " +
							s.toString() + " shows dimensionSize " + dimSize);
					return false;
				}
		}
		
		/*Ugly hack*/
		//boolean hack = handleCommonCaseSequencing();
		
		removeSequencing();
		isRange = true;
		return true;
	}
	
//	/*This is a hack to hard code */
//	@SuppressWarnings("unused")
//	private boolean handleCommonCaseSequencing() {		
//		
//		if(initStmts.size() != 2)
//			return false;
//		
//		AssignmentStatement s1 = initStmts.get(0);
//		AssignmentStatement s2 = initStmts.get(1);
//		
//		if(s1.getRhs().equalsVisitor(s2.getRhs()))
//			return true;
//		
//		
//		
//		return true;
//	}

	public void removeSequencing() {
		System.out.println("Before removing sequencing:");
		for (AssignmentStatement s: initStmts) {
			System.out.println(s.toString());
		}
//		Expression lastRHS = null;
//		boolean flag = true;
		
		/*If every assignment statement initializes to the same constant/expression
		 *we don't need an intersection check. This assumes that the same array
		 *is assigned to, and the result of an init does not interfere with another*/
		
		// But we need the init without sequencing for the feedback generation right now.
		// Might need to fix it later. Commenting this out for now.
//		for (AssignmentStatement s: initStmts) {
//			System.out.println(s.toString());
//			if(lastRHS == null)
//				lastRHS = s.getRhs();
//			else if(!lastRHS.equalsVisitor(s.getRhs())){
//				flag = false; 
//			}
//			lastRHS = s.getRhs();
//		}
//
//		if(flag)
//			return;
		
		Set<AssignmentStatement> finalSet = new HashSet<AssignmentStatement>();

		ListIterator<AssignmentStatement> lIt = initStmts.listIterator(initStmts.size());
		while (lIt.hasPrevious()) {
			if (finalSet.isEmpty()) {
//				AssignmentStatement newStmt = new AssignmentStatement(lIt.previous());
				AssignmentStatement newStmt = lIt.previous().clone();
//				finalSet.add(lIt.previous());
				finalSet.add(newStmt);
			} else {
				Set<AssignmentStatement> resultSet = getCanonicalSet(finalSet, lIt.previous());
				if (resultSet != null)
					finalSet = resultSet;
			}
		}
		
		if (finalSet != null)
			initWithoutSequencing.add(finalSet);
		
		System.out.println("After removing sequencing:");
		for (Set<AssignmentStatement> s: initWithoutSequencing) {
			System.out.println("Start");
			for (AssignmentStatement stmt: s) {
				System.out.println(stmt.toString());
			}
			System.out.println("End");
		}
		System.out.println("initStmts");
		for (AssignmentStatement s: initStmts) {
			System.out.println(s.toString());
		}
	}
	
	// Takes as input a set of statements and a single statement s, returns a new set of statements 
	// that is a union of the original set with s. This new set does not have any overlapping statements.
	// Computing non-overlapping statements forms the major part of this function.
	public Set<AssignmentStatement> getCanonicalSet(Set<AssignmentStatement> stmtSet, AssignmentStatement s) {
		int dimSize = arrayNameToDimensionSize.get(s.getLhs().getName());
		if (stmtSet.isEmpty()) {
			AssignmentStatement newStmt = s.clone();
			stmtSet.add(newStmt);
		} else {
			Set<AssignmentStatement> newStmtSet = new HashSet<AssignmentStatement>();
			Set<AssignmentStatement> tempStmtSet = new HashSet<AssignmentStatement>();
				
			tempStmtSet.add(s);

			for (AssignmentStatement stmt: stmtSet) {
				for (AssignmentStatement tempStmt: tempStmtSet) {
					// if the later one contains the earlier one,
					// can ignore the earlier one
					if(stmt.contains(tempStmt, dimSize))
						continue;
					// we retain the earlier statement as is if it assigns the
					// same rhs or the intersection between the ranges
					// assigned to is empty
//					if (stmt.getRhs().equalsVisitor(tempStmt.getRhs()) || stmt.intersects(tempStmt) == false) {
					// Even if the rhs is the same, we need the statements as a disjoint set.
					if (stmt.intersects(tempStmt, dimSize) == false) {
						AssignmentStatement newStmt = tempStmt.clone();
						newStmtSet.add(newStmt);
						continue;
					}	
					
					Set<AssignmentStatement> set = null;
					if (dimSize == 1)
						set = AssignmentStatement.find1DIntersection(tempStmt, stmt);
					else if (dimSize == 2)
						set = AssignmentStatement.find2DIntersection(tempStmt, stmt);
					newStmtSet.addAll(set);
				}
				tempStmtSet.clear();
				tempStmtSet.addAll(newStmtSet);
				newStmtSet.clear();
			}		
			if (!(stmtSet.equals(tempStmtSet) || tempStmtSet.isEmpty()))
				stmtSet.addAll(tempStmtSet);
		}
		return stmtSet;
	}
	
//	public boolean generateFeedback(List<InitSummary> refInits, Logger FeedbackLog) {
//		for (InitSummary R: refInits) {
//			if (compareSummary(R, FeedbackLog).succeeded)
//				break;
//		}
//		
//		FeedbackLog.writeLog("Min repair");
//		FeedbackLog.writeLog("Min weight: " + minWeightOfRepair);
//		FeedbackLog.writeLog(minRepair.displayRepair());
//		
//		FeedbackLog.writeLog("Generated repairs:");
//		for (InitRepair ir: generatedRepairs) {
//			FeedbackLog.writeLog("InitRepair");
//			FeedbackLog.writeLog(ir.displayRepair());
//		}
//		
//		if (minWeightOfRepair >= 0)
//			return true;
//		else
//			return false;
//	}
	
	public ResultStats compareSummary(InitSummary refSummary, Logger FeedbackLog, 
			Map<String, Variable> submissionProgramVariables) {
		
		if((refSummary.initStmts == null || refSummary.initStmts.isEmpty()) 
				&& (initStmts == null || initStmts.isEmpty())){
//			FeedbackLog.writeLog("Both ref and sub have empty init");
//			FeedbackLog.writeLog("Init matched");
			System.out.println(refSummary.progInitSummaryName + " Matched " + progInitSummaryName);
			return new ResultStats(true, 0);
		}
		
		if(refSummary.initStmts == null || refSummary.initStmts.isEmpty()){
			if(initStmts != null && !initStmts.isEmpty())
				FeedbackLog.writeLog("All init statements are spurious");
				score += initStmts.size();				
				return new ResultStats(true, 0, 0, initStmts.size(), "", score);
		}
		
		if(initStmts == null || initStmts.isEmpty()){
			if(refSummary.initStmts != null && !refSummary.initStmts.isEmpty())
				//FeedbackLog.writeLog("Missing init statements " + refSummary.initStmts);
				FeedbackLog.writeLog("Missing initialization:");
				for (AssignmentStatement s: refSummary.initStmts) {
					FeedbackLog.writeLog(HelperFunctions.rewriteRepair(s.toStringFeedback(false), submissionProgramVariables));
				}
				score += refSummary.initStmts.size();
				return new ResultStats(true, initStmts.size(), 0, 0, "", score);
		}
		
		// We are checking if all the dp arrays are of the same dimension.
		int refDim = -1;
		for (Map.Entry<String, Integer> e: refSummary.arrayNameToDimensionSize.entrySet()) {
			if (refDim != -1 && refDim != e.getValue())
				return new ResultStats(false, 0, 0, "Mismatched dimensions", 5);
			else if (refDim == -1)
				refDim = e.getValue();
		}
		
		int refSub = -1;
		for (Map.Entry<String, Integer> e: arrayNameToDimensionSize.entrySet()) {
			if (refSub != -1 && refSub != e.getValue())
				return new ResultStats(false, 0, 0, "Mismatched dimensions", 5);
			else if (refSub == -1)
				refSub = e.getValue();
		}
		
		// If the dp array in ref is not of the same dimension as the sub, our heuristic does not work.
		if (refDim != refSub){
//			FeedbackLog.writeLog("Ref DP array dimensions don't match submission");	
			return new ResultStats(false, 0, 0, "Mismatched dimensions", 5);
		}
		boolean exactMatch = false;
		if (exactMatchLocalSummary(refSummary)) {
			FeedbackLog.writeLog("Matches " + refSummary.progName);
			InitRepair currRepair = new InitRepair();
			currRepair.refSummaryUsed = refSummary.progName;
			currRepair.heuristic = heuristicKind.EXACTMATCH;
			if (globalInits.size() != 0) {
				spuriousGlobalInit = true;
				FeedbackLog.writeLog("Spurious Global Init");
				for (AssignmentStatement s: globalInits) {
					repairSteps step = new repairSteps(repairKind.SPURIOUS, s, null);
					currRepair.steps.add(step);
					currRepair.weightOfRepair++;
				}
			} else {
				for (AssignmentStatement s: initStmts) {
					repairSteps step = new repairSteps(repairKind.NOREPAIR, s, s);
					currRepair.steps.add(step);
					currRepair.weightOfRepair = 0;
				}
			}
			
			generatedRepairs.add(currRepair);
			if (minWeightOfRepair == -1 || minWeightOfRepair > currRepair.weightOfRepair) {
				minWeightOfRepair = currRepair.weightOfRepair;
				minRepair = currRepair;
			}
			
			exactMatch = true;
		} else {
			exactMatch = false;
			findRepair(refSummary, FeedbackLog, false, refDim);
//			FeedbackLog.writeLog("Min repair");
//			FeedbackLog.writeLog("Min weight: " + minWeightOfRepair);
			FeedbackLog.writeLog(minRepair.displayRepair(submissionProgramVariables));
//			System.out.println("DEBUG: Before flipping: " + refSummary.toString());
//			InitSummary flipRefSummary = refSummary.flipInitSummary();
//			System.out.println("DEBUG: After flipping: " + flipRefSummary.toString());
//			
//			findRepair(flipRefSummary, FeedbackLog, true);
			
		}
		score = minWeightOfRepair;
		int numInsert = 0, numReplace = 0, numSpurious = 0;
		for(repairSteps s : minRepair.steps){
			switch(s.repair){
				case INCORRECT: numReplace++; break;
				case MISSING: numInsert++; break;
				case SPURIOUS: numSpurious++; break;
				case USEREF: numReplace++; break;
			}
		}
		
		return new ResultStats(true, numInsert, numReplace, numSpurious, "", score);
	}
	
//	public InitSummary flipInitSummary() {
//		InitSummary flippedSummary = new InitSummary(this);
//		
//		Set<AssignmentStatement> tempSet = new HashSet<AssignmentStatement>();
//		for (AssignmentStatement s: flippedSummary.initStmts) {
//			s.flipDimensions();
//			tempSet.add(s);
//		}
//		flippedSummary.initStmts.clear();
//		flippedSummary.initStmts.addAll(tempSet);
//		
//		tempSet.clear();
//		for (AssignmentStatement s: flippedSummary.globalInits) {
//			s.flipDimensions();
//			tempSet.add(s);
//		}
//		flippedSummary.globalInits.clear();
//		flippedSummary.globalInits.addAll(tempSet);
//		
//		List<Set<AssignmentStatement>> tempList = new ArrayList<Set<AssignmentStatement>>();
//		for (Set<AssignmentStatement> set: flippedSummary.initWithoutSequencing) {
//			tempSet.clear();
//			for (AssignmentStatement s: set) {
//				s.flipDimensions();
//				tempSet.add(s);
//			}
//			tempList.add(tempSet);
//		}
//		flippedSummary.initWithoutSequencing.clear();
//		flippedSummary.initWithoutSequencing.addAll(tempList);
//		
//		return flippedSummary;
//	}
	
	private boolean exactMatchLocalSummary(InitSummary init) {
		if (init.initStmts.size() != initStmts.size())
			return false;
		for (int i = 0; i < init.initStmts.size(); i++) {
			if (!init.initStmts.get(i).equals(initStmts.get(i)))
				return false;
		}
		
		return true;
	}
	
	// flippedHeuristic is true if we flipped refSummary before calling this function.
	private void findRepair(InitSummary refSummary, Logger FeedbackLog, boolean flippedHeuristic, int dimSize) {
		
		if (refSummary.initWithoutSequencing.size() > 1 || 
				this.initWithoutSequencing.size() > 1) {
//			System.out.println("Sequencing present either in refSummary or in subSummary");
			return;
		}


		InitRepair currRepair = new InitRepair();
		if (flippedHeuristic)
			currRepair.heuristic = heuristicKind.FLIPDIMANDMATCHLHSANDRHS;
		else
			currRepair.heuristic = heuristicKind.MATCHLHSANDRHS;
		currRepair.refSummaryUsed = refSummary.progName;
		List<AssignmentStatement> spuriousInits = new LinkedList<AssignmentStatement>();
		List<AssignmentStatement> missingInits = new LinkedList<AssignmentStatement>();		
		
//		System.out.println(refSummary.toString());
//		System.out.println(this.toString());

		if (refSummary.initWithoutSequencing.size() > 0)
			for (AssignmentStatement s: refSummary.initWithoutSequencing.get(0)) {
				missingInits.add(s);
//				System.out.println(s.toString());
			}
		if (this.initWithoutSequencing.size() > 0)
			for (AssignmentStatement s: this.initWithoutSequencing.get(0)) {
				spuriousInits.add(s);
//				System.out.println(s.toString());
			}
		
		List<AssignmentStatement> nullIntersectMissingInits = new LinkedList<AssignmentStatement>();

		boolean presentRefInit = false;
		while (missingInits.size() > 0) {
			AssignmentStatement currRef = missingInits.remove(0);
			boolean nullIntersectionForRef = true; // Flag to check whether currRef has null intersection with all subs.
			
			List<AssignmentStatement> nullIntersectSpuriousInits = new LinkedList<AssignmentStatement>();
			while (spuriousInits.size() > 0) {
				AssignmentStatement currSub = spuriousInits.remove(0);
//				List<DimensionSummary> intersection = currRef.findDimensionIntersection(currSub);
//				Set<AssignmentStatement> intersection = AssignmentStatement.find2DIntersection(currRef, currSub);
//				System.out.println("******Finding intersection between:");
//				System.out.println(currSub.toString());
//				System.out.println(currRef.toString());
				AssignmentStatement intersection = null;
				if (dimSize == 1)
					intersection = AssignmentStatement.find1DStmtIntersection(currSub, currRef);
				else if (dimSize == 2)
					intersection = AssignmentStatement.find2DStmtIntersection(currSub, currRef);
				if (intersection == null) {
//					System.out.println("NULL");
					nullIntersectSpuriousInits.add(currSub);
					continue;
				}
				
//				System.out.println("Intersection:");
//				System.out.println(intersection.toString());
				
				nullIntersectionForRef = false;
				
//				if (!(AssignmentStatement.relaxedEquals(currRef.constInitializer, currSub.constInitializer))) {
//					AssignmentStatement intersectionStmt = new AssignmentStatement(currSub.baseVariable, intersection, currSub.constInitializer, currSub.globalInit);
//					AssignmentStatement repairedStmt = new AssignmentStatement(currSub.baseVariable, intersection, currRef.constInitializer, currSub.globalInit);
//					repairSteps step = new repairSteps(repairKind.INCORRECT, intersectionStmt, repairedStmt);
//					currRepair.steps.add(step);
//					currRepair.weightOfRepair += 2;
//				}
				
				if (!(currRef.getRhs().equalsVisitor(currSub.getRhs()))) {
					boolean needsRepair = true;
					if(currRef.getRhs().getEType() != ExprType.CONST && currRef.getRhs().contains(Expression.getID("x"))){
						Expression toTest = (Expression) currRef.getRhs().clone();
						toTest.replace(new Expression(ExprType.ID, "", "x"), (Expression) currSub.getRhs().clone());
//						System.out.println(toTest);
						int value = HelperFunctions.evaluateExpression(new HashMap<String, String>(), toTest);
						if(value == 1)
							needsRepair = false;
						if("||".compareTo(currRef.getRhs().getOperator()) == 0){
							Expression sub = currSub.getRhs();
							Expression e1 = currRef.getRhs().geteList().get(0);
							Expression e2 = currRef.getRhs().geteList().get(1);
							if(e1.equalsVisitor(sub) || e2.equalsVisitor(sub))
								needsRepair = false;
						}
							
					}
					if(currRef.getRhs().getOperator()!=null && "OR".compareTo(currRef.getRhs().getOperator()) == 0){
						    Expression sub = currSub.getRhs();
						    Expression e1 = currRef.getRhs().geteList().get(0);
						    Expression e2 = currRef.getRhs().geteList().get(1);
						    if(e1.equalsVisitor(sub) || e2.equalsVisitor(sub))
						    	needsRepair = false;						
					}
					if(needsRepair){
						AssignmentStatement repairedStmt = new AssignmentStatement(intersection.getGuard(), 
								intersection.getLhs(), currRef.getRhs());
						repairSteps step = new repairSteps(repairKind.INCORRECT, intersection, repairedStmt);
						currRepair.steps.add(step);
						currRepair.weightOfRepair += 1;
					}
				}
				
				// 1. find ref-sub
//				Set<AssignmentStatement> diffSet = currRef.findDifferenceAndGetProduct(intersection);
//				if (diffSet != null && diffSet.size() != 0) {
//					for (AssignmentStatement s: diffSet) 
//						missingInits.add(s);
//				}
//				System.out.println("*******Finding difference between:");
//				System.out.println(currRef.toString());
//				System.out.println(intersection.toString());
//				System.out.println("Difference: ");
				Set<AssignmentStatement> diffSet = null;
				if (dimSize == 1)
					diffSet = AssignmentStatement.find1DIntersection(currRef, intersection);
				else if (dimSize == 2)
					diffSet = AssignmentStatement.find2DIntersection(currRef, intersection);
				if (diffSet != null && diffSet.size() != 0) {
					for (AssignmentStatement s: diffSet) {
						missingInits.add(s);
//						System.out.println(s.toString());
					}
				} else {
//					System.out.println("NULL");
				}
				
				// 2. find sub-ref
//				diffSet = currSub.findDifferenceAndGetProduct(intersection);
//				if (diffSet != null && diffSet.size() != 0) {
//					for (AssignmentStatement s: diffSet) 
//						spuriousInits.add(s);
//				}
//				System.out.println("******Finding difference between:");
//				System.out.println(currSub.toString());
//				System.out.println(intersection.toString());
//				System.out.println("Difference: ");
				if (dimSize == 1)
					diffSet = AssignmentStatement.find1DIntersection(currSub, intersection);
				else if (dimSize == 2)
					diffSet = AssignmentStatement.find2DIntersection(currSub, intersection);
				if (diffSet != null && diffSet.size() != 0) {
					for (AssignmentStatement s: diffSet) {
						spuriousInits.add(s);
//						System.out.println(s.toString());
					}
				} else {
//					System.out.println("NULL");
				}
				
				// Why am I doing this? To avoid some infinite loop (I forgot which one!)
				break;
			}
			spuriousInits.addAll(nullIntersectSpuriousInits);
			
			if (nullIntersectionForRef)
				nullIntersectMissingInits.add(currRef);
		}
		missingInits.addAll(nullIntersectMissingInits);
		
		if (spuriousInits.size() + missingInits.size() + currRepair.steps.size() >= 5)
			presentRefInit = true;
		else
			presentRefInit = false;
		
		if (!presentRefInit) {

			if (spuriousInits.size() > 0) {
				for (AssignmentStatement s: spuriousInits) {
					repairSteps step = new repairSteps(repairKind.SPURIOUS, s, null);
					currRepair.steps.add(step);
					currRepair.weightOfRepair += 1;
				}
			} 
			if (missingInits.size() > 0) {
				for (AssignmentStatement s: missingInits) {
					repairSteps step = new repairSteps(repairKind.MISSING, null, s);
					currRepair.steps.add(step);
					currRepair.weightOfRepair += 1;
				}
			}
		
			generatedRepairs.add(currRepair);
			if (minWeightOfRepair == -1 || minWeightOfRepair > currRepair.weightOfRepair) {
				minWeightOfRepair = currRepair.weightOfRepair;
				minRepair = currRepair;
			}
			score = currRepair.weightOfRepair;
		} else {
			InitRepair refRepair = new InitRepair();
			for (AssignmentStatement s: refSummary.initStmts) {
				repairSteps step = new repairSteps(repairKind.USEREF, null, s);
				refRepair.steps.add(step);
				refRepair.weightOfRepair += 1;
			}
			
			generatedRepairs.add(refRepair);
			if (minWeightOfRepair == -1 || minWeightOfRepair > refRepair.weightOfRepair) {
				minWeightOfRepair = refRepair.weightOfRepair;
				minRepair = refRepair;
			}
			score = refRepair.weightOfRepair;
		}
	}
}
