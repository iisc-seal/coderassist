package feedback.update;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.antlr.runtime.ANTLRFileStream;
import org.antlr.runtime.ANTLRStringStream;
import org.antlr.runtime.CharStream;
import org.antlr.runtime.CommonTokenStream;
import org.antlr.runtime.RecognitionException;
import org.antlr.runtime.TokenStream;
import org.antlr.runtime.tree.CommonTree;
import org.antlr.runtime.tree.CommonTreeNodeStream;

import com.microsoft.z3.Solver;
import com.microsoft.z3.Z3Exception;
import com.sun.org.apache.xerces.internal.impl.dv.ValidatedInfo;

import feedback.init.parser.InitTreeWalker;
import feedback.init.parser.initParseLexer;
import feedback.init.parser.initParseParser;
import feedback.logging.Logger;
import feedback.update.parser.Rewrite;
import feedback.update.parser.UpdateLexer;
import feedback.update.parser.UpdateParser;
import feedback.update.parser.UpdateParser.prog_return;
import feedback.update.parser.UpdateTreeWalker;
import feedback.utils.AssignmentStatement;
import feedback.utils.Block;
import feedback.utils.Expression;
import feedback.utils.HelperFunctions;
import feedback.utils.LoopStatement;
import feedback.utils.FragmentSummary;
import feedback.utils.Statement;
import feedback.utils.Expression.ExprType;
import feedback.utils.VCGen;
import feedback.utils.Variable;

public class UpdateSummary {
	public String progName;
	public String progUpdateSummaryName;
	public List<Statement> updateStatements;
	public Logger errorLogger;
	public Logger feedbackLogger;
	public List<Block> blocks;
	int score;
	
	public int getScore(){
		return score;
	}
	
	public UpdateSummary(String progName, String progUpdateSummaryName, 
			Logger errorLogger) {
		super();
		this.progName = progName;
		this.progUpdateSummaryName = progUpdateSummaryName;
		this.updateStatements = null;
		this.errorLogger = errorLogger;
		score = 0;
	}
	
	public List<Statement> getUpdateStatements() {
		return updateStatements;
	}

	public void setUpdateStatements(List<Statement> outputStatements) {
		this.updateStatements = outputStatements;
	}

	public String getProgUpdateSummaryName() {
		return progUpdateSummaryName;
	}

	public Logger getErrorLogger() {
		return errorLogger;
	}
	
	public boolean processAsReference(){		
		for(Block b: blocks){
			b.computeConstraintsAsRef();
		}
		return true;
	}
	
	public void prepareReference(){
		for(Block b: blocks){
			b.computeConstraintsAsRef();
		}
	}
	
//	public boolean processAsSubmission(UpdateSummary ref, 
//			Logger feedbackLogger, Expression inputConstraint){
//		this.feedbackLogger = feedbackLogger;		
//		if(ref.blocks.size() == 0 && blocks.size() != 0){
//			score += blocks.size();
//			feedbackLogger.writeLog("Spurious block!");
//			return true;
//		}
//		
//		if(ref.blocks.size() != 0 && blocks.size() == 0){
//			score += ref.blocks.size();
//			feedbackLogger.writeLog("Missing block!");
//			return true;
//		}
//		
//		if(blocks.size() != ref.blocks.size()){
//			feedbackLogger.writeLog("Mismatched number of blocks");
//			score += 10;
//			return true;
//		}
//
//		// if an entry at index i is false, it means the i'th fragment matched
//		List<Boolean> fragments = new ArrayList<Boolean>();
//		
//		List<Boolean> cMatch = new ArrayList<Boolean>();
//		for(int i = 0; i < blocks.size(); i++){
//			Block refBlock = ref.blocks.get(i);
//			Block subBlock = blocks.get(i);
//			feedbackLogger.writeLog("Matching block " + i);
//			VCGen checker = new VCGen(refBlock, subBlock, inputConstraint, feedbackLogger);
//			if(!checker.checkDepthAndDirection()){
//				score += 1;
//				continue;
//			}
//			checker.prepare();
//			try {
//				checker.testSpurious();
//			 	checker.testMissing();
//			 	checker.testEquivalence();
//			}
//			catch (Z3Exception e) {
//				e.printStackTrace();
//			}
//			score += checker.getScore();
//			boolean mismatch = checker.getMatched();
//			fragments.add(mismatch);
//			if(!mismatch)
//				feedbackLogger.writeLog("Fragment " + i + " Matched");
//			else
//				feedbackLogger.writeLog("Fragment " + i + " Mismatch");
//		}
//		
//		boolean result = true;
//		boolean conditional = false;
//		for(int i = 0; i < fragments.size(); i++){
//			if(fragments.get(i)){
//				result = false;
//				break;
//			}	
//		}
//		for(int i = 0; i < cMatch.size(); i++){
//			if(cMatch.get(i)){
//				conditional = true;
//				break;
//			}	
//		}
//		if(result){
//			System.out.print(progUpdateSummaryName + "["+progName+"]" + 
//					" Matched " + ref.progUpdateSummaryName);
//			feedbackLogger.writeLog(progUpdateSummaryName + 
//					" Matched " + ref.progUpdateSummaryName);
//			if(conditional)
//				System.out.println(" Conditionally");
//			else
//				System.out.println();
//		}
//		feedbackLogger.writeLog(Integer.toString(score));
//		//feedbackLogger.writeLog("------------------------------------------");
//		
//		return true;
//	}
		
	public boolean parse() {
		String fileName = progUpdateSummaryName;
		CharStream charStr;
		try {
			charStr = new ANTLRFileStream(fileName);
		} catch (IOException e) {
			errorLogger.writeLog(fileName);
			errorLogger.writeLog("ERROR: I don't know! IOException while reading file "
					+ fileName);
			return false;
		}
		
		UpdateLexer lexer = new UpdateLexer(charStr);
		TokenStream tokenStr = new CommonTokenStream(lexer);
		UpdateParser parser = new UpdateParser(tokenStr);
		prog_return prog;
		try {
			prog = parser.prog();
		} catch (RecognitionException e) {
			errorLogger.writeLog(fileName);
			errorLogger.writeLog("Update: " + e.toString());
			return false;
		} catch (RuntimeException e) {
			errorLogger.writeLog(fileName);
			errorLogger.writeLog("Update: " + e.toString());
			return false;
		}
		String t1 = prog.getTree().toStringTree();
		CommonTree t = (CommonTree)prog.getTree();
	    CommonTreeNodeStream nodes = new CommonTreeNodeStream(t);
	    
	    /*Uncomment to enable rewriting*/
//	    Rewrite s = new Rewrite(nodes);
//	    t = (CommonTree)s.downup(t, true); 
//	    String t2 = t.toStringTree();
//		if(t1.compareTo(t2) != 0){
//			System.out.println("Rewrote " + progUpdateSummaryName);
//		}
		CommonTreeNodeStream nodeStr = new CommonTreeNodeStream(t);
		UpdateTreeWalker walker = new UpdateTreeWalker(nodeStr);
		try {
			updateStatements = walker.prog();			
		} catch (RecognitionException e) {
			errorLogger.writeLog(fileName);
			errorLogger.writeLog("Update: " + e.toString());
			return false;
		} catch (RuntimeException e) {
			errorLogger.writeLog(fileName);
			errorLogger.writeLog("Update: " + e.toString());
			return false;
		}
		blocks = HelperFunctions.getAsBlocks(updateStatements);
		return true;
	}
		
	public static List<Statement> parse(String fileName) {
		CharStream charStr;
		List<Statement> op;
		try {
			charStr = new ANTLRFileStream(fileName);
		} catch (IOException e) {
			System.out.println("ERROR: I don't know! IOException while reading file "
					+ fileName);
			return null;
		}
		
		UpdateLexer lexer = new UpdateLexer(charStr);
		TokenStream tokenStr = new CommonTokenStream(lexer);
		UpdateParser parser = new UpdateParser(tokenStr);
		prog_return prog;
		try {
			prog = parser.prog();
		} catch (RecognitionException e) {
			System.out.println("UPDATE: " + e.toString());
			return null;
		} catch (RuntimeException e) {
			System.out.println("UPDATE: " + e.toString());
			return null;
		}

		CommonTreeNodeStream nodeStr = new CommonTreeNodeStream(prog.getTree());
		UpdateTreeWalker walker = new UpdateTreeWalker(nodeStr);
		try {
			op = walker.prog();			
		} catch (RecognitionException e) {
			System.out.println("Update: " + e.toString());
			return null;
		} catch (RuntimeException e) {
			System.out.println("Update: " + e.toString());
			return null;
		}
		return op;
	}

	public static List<Statement> parse(ANTLRStringStream str) {
		UpdateLexer lexer = new UpdateLexer(str);
		TokenStream tokenStr = new CommonTokenStream(lexer);
		UpdateParser parser = new UpdateParser(tokenStr);
		prog_return prog;
		try {
			prog = parser.prog();
		} catch (RecognitionException e) {
			System.out.println("UPDATE: " + e.toString());
			return null;
		} catch (RuntimeException e) {
			System.out.println("UPDATE: " + e.toString());
			return null;
		}

		CommonTreeNodeStream nodeStr = new CommonTreeNodeStream(prog.getTree());
		UpdateTreeWalker walker = new UpdateTreeWalker(nodeStr);
		List<Statement> op;
		try {
			op = walker.prog();			
		} catch (RecognitionException e) {
			System.out.println("Update: " + e.toString());
			return null;
		} catch (RuntimeException e) {
			System.out.println("Update: " + e.toString());
			return null;
		}
		return op;
	}

	private static boolean checkSanity(Map<Expression, String> refTypes,
			Map<Expression, String> subTypes) {
		for (Expression e : refTypes.keySet()) {
			  if(subTypes.containsKey(e))
				  if(!(refTypes.get(e) == subTypes.get(e)));
			  		return false;
		}
		return true;
	}

	public void incScore(int i) {
		score += i;
	}

	
}
