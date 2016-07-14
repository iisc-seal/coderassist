package feedback.output;

import java.io.IOException;

import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.Set;

import org.antlr.runtime.ANTLRFileStream;
import org.antlr.runtime.CharStream;
import org.antlr.runtime.CommonTokenStream;
import org.antlr.runtime.RecognitionException;
import org.antlr.runtime.TokenStream;
import org.antlr.runtime.tree.CommonTree;
import org.antlr.runtime.tree.CommonTreeNodeStream;

import feedback.logging.Logger;
import feedback.output.parser.OutputLexer;
import feedback.output.parser.OutputParser;
import feedback.output.parser.OutputTreeWalker;
import feedback.output.parser.Rewrite;
import feedback.output.parser.OutputParser.primary_return;
import feedback.utils.AssignmentStatement;
import feedback.utils.Block;
import feedback.utils.Expression;
import feedback.utils.HelperFunctions;
import feedback.utils.Statement;
import feedback.utils.Variable;

public class OutputSummary {
	
	public String progName;
	public String progOutputSummaryName;
	public List<Statement> outputStatements;
	public Logger ErrorLogger;
	
	public Set<String> exprTypes;
	
	public Expression outputLhs;
	public Map<Expression, Expression> bindings;

	public List<Block> blocks;
	
	// encoding the max function in the SMTLIB2 format
	public static String smtmax = "(define-fun max ((x Int) (y Int)) Int\n"+
				"(ite (< x y) y x))\n";
	
	int score;
	
	public void incScore(int i) {
		score += i;
	}
	
	public OutputSummary(String progName, String progOutputSummaryName, Logger errorLogger) {
		super();
		this.progName = progName;
		this.progOutputSummaryName = progOutputSummaryName;
		this.outputStatements = null;
		ErrorLogger = errorLogger;
		score = 0;
	}
	
	public void prepareReference(){
		for(Block b: blocks){
			b.computeConstraintsAsRef();
		}
	}
	
	public List<Statement> getOutputStatements() {
		return outputStatements;
	}

	public String getProgOutputSummaryName() {
		return progOutputSummaryName;
	}
	
	public boolean parse() {
		String fileName = progOutputSummaryName;
		CharStream charStr;
		try {
			charStr = new ANTLRFileStream(fileName);
		} catch (IOException e) {
			ErrorLogger.writeLog(fileName);
			ErrorLogger.writeLog("ERROR: I don't know! IOException while reading file "
					+ fileName);
			return false;
		}
		
		OutputLexer lexer = new OutputLexer(charStr);
		TokenStream tokenStr = new CommonTokenStream(lexer);
		OutputParser parser = new OutputParser(tokenStr);
		primary_return prog;
		try {
			prog = parser.primary();
			
		} catch (RecognitionException e) {
			ErrorLogger.writeLog(fileName);
			ErrorLogger.writeLog("Out: " + e.toString());
			return false;
		} catch (RuntimeException e) {
			ErrorLogger.writeLog(fileName);
			ErrorLogger.writeLog("Out: " + e.toString());
			return false;
		}
		
		CommonTree t = (CommonTree)prog.getTree();
//	    CommonTreeNodeStream nodes = new CommonTreeNodeStream(t);
//	    Rewrite rw = new Rewrite(nodes);
//	    t = (CommonTree)rw.downup(t, true); // walk t, trace transforms
//	    System.out.println("Simplified tree: "+t.toStringTree());
	        
	    CommonTreeNodeStream nodeStream = new CommonTreeNodeStream(t);
	    OutputTreeWalker walker = new OutputTreeWalker(nodeStream);
	    
		try {
			outputStatements = walker.primary();			
		} catch (RecognitionException e) {
			ErrorLogger.writeLog(fileName);
			ErrorLogger.writeLog("Out: " + e.toString());
			return false;
		} catch (RuntimeException e) {
			ErrorLogger.writeLog(fileName);
			ErrorLogger.writeLog("Out: " + e.toString());
			return false;
		}
		int size = outputStatements.size();
		if(outputStatements != null && size > 0){
			Statement lastStmt = outputStatements.get(size - 1);
			AssignmentStatement aStmt = (AssignmentStatement) lastStmt;
			if(aStmt.getGuard().equalsVisitor(Expression.getTrue())){
				if(aStmt.hasNewline() && aStmt.getRhs() == null){
					for(int i = 0; i < outputStatements.size()-1; i++){
						AssignmentStatement stmt = (AssignmentStatement) outputStatements.get(i);
						stmt.setNewline();
					}
					outputStatements.remove(size - 1);
				}
			}
			
			
			ListIterator<Statement> iter = outputStatements.listIterator();
			while(iter.hasNext()){
				AssignmentStatement next = (AssignmentStatement) iter.next();
				if(next.getRhs() == null){
					iter.remove();
				}
			}
		}
		blocks = HelperFunctions.getAsBlocks(outputStatements);
		
		
		
//		HelperFunctions.identifySuspicious(outputStatements, progOutputSummaryName);
		return true;
	}
	
	public boolean compare(OutputSummary other){
		List<Statement> oList = other.getOutputStatements();
		if(outputStatements.size() != oList.size())
			return false;		
		for(int i = 0; i < outputStatements.size(); i++){
			Statement s1 = outputStatements.get(i);
			Statement s2 = oList.get(i);
			if(!s1.equals(s2))
				return false;
		}			
		return true;
	}
		
	boolean hasNewline(){
		for(Statement s: outputStatements){
			AssignmentStatement aStmt = ((AssignmentStatement) s);
			if(!aStmt.hasNewline())
				return false;
		}
		return true;
	}
	
	public int getScore() {
		return score;
	}
	
}
