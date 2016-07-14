package feedback.update;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Scanner;
import java.util.Set;

import org.antlr.runtime.ANTLRStringStream;
import org.antlr.runtime.CharStream;
import org.antlr.runtime.CommonTokenStream;
import org.antlr.runtime.RecognitionException;
import org.antlr.runtime.TokenStream;
import org.antlr.runtime.tree.CommonTreeNodeStream;

import com.microsoft.z3.ArithExpr;
import com.microsoft.z3.BoolExpr;
import com.microsoft.z3.Context;
import com.microsoft.z3.Expr;
import com.microsoft.z3.FuncDecl;
import com.microsoft.z3.Model;
import com.microsoft.z3.Solver;
import com.microsoft.z3.Status;
import com.microsoft.z3.Z3Exception;

import feedback.inputs.ProgramInputs;
import feedback.logging.Logger;
import feedback.update.SATTest.TestFailedException;
import feedback.update.parser.ExpressionTreeWalker;
import feedback.update.parser.UpdateLexer;
import feedback.update.parser.UpdateParser;
import feedback.update.parser.UpdateParser.expression_return;
import feedback.utils.AssignmentStatement;
import feedback.utils.Expression;
import feedback.utils.HelperFunctions;
import feedback.utils.LoopStatement;
import feedback.utils.FragmentSummary;
import feedback.utils.Statement;
import feedback.utils.Expression.ExprType;

public class UpdateDriver {

	public static String smtmax = "(define-fun max ((x Int) (y Int)) Int\n"+
	"  (ite (< x y) y x))\n";
	
	public static void main(String[] args) {
		
		long startTime = System.nanoTime();
		
		if (args.length < 3) {
			System.out.println("ERROR: Invalid number of arguments\n");
			System.out.println("Input refsFile, subsFile and path to store logs\n");
			return;
		}
		
		Set<String> subsToSkip = new HashSet<String>();
		subsToSkip.add("/home/anirudh/submissions/MGCRNK/anshkhanna7/4564139.update.summary");
		subsToSkip.add("/home/anirudh/submissions/MGCRNK/ayushj10/1633640.update.summary");
		subsToSkip.add("/home/anirudh/submissions/MGCRNK/bablu/1702675.update.summary");
		subsToSkip.add("/home/anirudh/submissions/MGCRNK/durgesh333/1637029.update.summary");
		subsToSkip.add("/home/anirudh/submissions/MGCRNK/dragunov123/3652588.update.summary");
//		subsToSkip.add("/home/anirudh/submissions/MGCRNK/g1zm0/5310117.update.summary");
		
		subsToSkip.add("/home/anirudh/submissions/MGCRNK/megha_iiti/1648607.update.summary");
		subsToSkip.add("/home/anirudh/submissions/MGCRNK/mozaic/2184862.update.summary");
		subsToSkip.add("/home/anirudh/submissions/MGCRNK/nandhukrishnan/1628398.update.summary");
		subsToSkip.add("/home/anirudh/submissions/MGCRNK/proxc/1629412.update.summary");
		
		subsToSkip.add("/home/anirudh/submissions/MGCRNK/saurish/1650564.update.summary");
		subsToSkip.add("/home/anirudh/submissions/MGCRNK/nandhukrishnan/1628398.update.summary");
		subsToSkip.add("/home/anirudh/submissions/MGCRNK/proxc/1629412.update.summary");
		
		subsToSkip.add("/home/anirudh/submissions/MGCRNK/edge123/3119132.update.summary");
		
		String refsFile = args[0];
		String subsFile = args[1];
		String logFilePath = args[2];
		
		Logger ErrorLogger = new Logger(logFilePath + "curr.error.log");
		Logger OutputLogger = new Logger(logFilePath + "curr.output.log");
		
		List<UpdateSummary> refList = new ArrayList<UpdateSummary>();
		Scanner scr;
		try {
			scr = new Scanner(new File(refsFile));
		} catch (FileNotFoundException e) {
			System.out.println("ERROR: Cannot find file " + refsFile);
			e.printStackTrace();
			return;
		}
		
		while (scr.hasNext()) {
			UpdateSummary updateSum = new UpdateSummary(scr.next(), scr.next(), ErrorLogger);
			System.out.println("DEBUG: File " + updateSum.getProgUpdateSummaryName());
			//OutputLogger.writeLog("File: " + updateSum.progName);
			boolean ret = updateSum.processAsReference();
			if (!ret) {
				ErrorLogger.closeLog();
				OutputLogger.closeLog();
				return;
			}
			
			refList.add(updateSum);
		}
		System.out.println("Read all references");
		try {
			scr = new Scanner(new File(subsFile));
		} catch (FileNotFoundException e) {
			System.out.println("ERROR: Cannot find file " + subsFile);
			return;
		}
		
		while (scr.hasNext()) {
			String currentSub = scr.next();
			if (subsToSkip.contains(currentSub))
				continue;

			Logger FeedbackLogger = new Logger(currentSub + ".feedback");
			UpdateSummary updateSum = new UpdateSummary(currentSub, currentSub, ErrorLogger);
			System.out.println("DEBUG: File " + updateSum.progUpdateSummaryName);
			//OutputLogger.writeLog("File: " + updateSum.progName);

//			if (updateSum.getProgUpdateSummaryName().endsWith("anant_pushkar/1627945.update.summary")){
//				System.out.println("Am here!");
//				continue;
//			}
			
			for(UpdateSummary ref: refList){
				System.out.println("Matching against ref "+ ref.progUpdateSummaryName);
				FeedbackLogger.writeLog("Matching against ref "+ ref.progUpdateSummaryName);
				updateSum.score = 0;
				boolean ret = updateSum.processAsSubmission(ref, FeedbackLogger);
				if (!ret){
					continue;
				}
				
				System.out.println("==============================================");
			}
			FeedbackLogger.closeLog();
		}
			

		ErrorLogger.closeLog();
		OutputLogger.closeLog();
		
		// Finding elapsed time
		long endTime = System.nanoTime();
		
		long difference = (endTime - startTime) / 1000000; // in milliseconds
		
		long elapsed_secs = difference / 1000;
		
		if (elapsed_secs == 0) {
			System.out.println("Total time: " + difference + " ms");
		} else {
			long hrs = elapsed_secs / 3600;
			long remaining = elapsed_secs % 3600;
			
			long mins = remaining / 60;
			remaining = remaining % 60;
			
			long secs = remaining;
			
			if (hrs == 0) 
				if (mins == 0)
					System.out.println("Total time: " + secs + "s");
				else
					System.out.println("Total time: " + mins + "m " + secs + "s");
			else
				System.out.println("Total time: " + hrs +"h " + mins + "m " + secs + "s");
		}
	}

}
