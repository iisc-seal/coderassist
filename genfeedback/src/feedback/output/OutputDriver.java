package feedback.output;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Scanner;
import java.util.Set;
import java.util.Map.Entry;

import com.microsoft.z3.BoolExpr;
import com.microsoft.z3.Context;
import com.microsoft.z3.FuncDecl;
import com.microsoft.z3.Model;
import com.microsoft.z3.Solver;
import com.microsoft.z3.Status;
import com.microsoft.z3.Z3Exception;

import feedback.logging.Logger;
import feedback.utils.AssignmentStatement;
import feedback.utils.Expression;
import feedback.utils.HelperFunctions;
import feedback.utils.FragmentSummary;
import feedback.utils.Statement;
import feedback.update.UpdateDriver;
import feedback.update.UpdateSummary;

public class OutputDriver {
	
//	private static String refsFile = "/home/shalini/research/codechef/submissions/ref-impls/MARCHA1/curr.ref.file";
//	private static String sFile = "/home/shalini/research/codechef/submissions/MARCHA1/curr.subs.file";
	private static String logPath = "/home/submissions/MARCHA1/";
	
	private static Logger ErrorLogger;
	private static Logger MatchLogger;
	private static Logger NoMatchLogger;
	private static Logger SpuriousGlobalInitLogger;
	

	public static String smtmax = "(define-fun max ((x Int) (y Int)) Int\n"+
	"  (ite (< x y) y x))\n";

	public static void main(String[] args) {
		
		long startTime = System.nanoTime();
//		args = new String[3];
//		args[0] = refsFile;
//		args[1] = sFile;
//		args[2] = logPath;
		
		if (args.length < 3) {
			System.out.println("ERROR: Invalid number of arguments\n");
			System.out.println("Input ReferencesFile, SubmissionsFile and path to store logs\n");
			return;
		}
		
		Set<String> subsToSkip = new HashSet<String>();
		subsToSkip.add("/home/shalini/research/codechef/submissions/MARCHA1/dsun/734477");
		subsToSkip.add("/home/shalini/research/codechef/submissions/MARCHA1/gargnitin123/38373");
		subsToSkip.add("/home/shalini/research/codechef/submissions/MARCHA1/kerberozz/2980723");
		
		String refsFile = args[0];
		String subsFile = args[1];
		String logFilePath = args[2];
		
//		Logger ErrorLogger, MatchLogger, NoMatchLogger;
//		Logger SpuriousGlobalInitLogger;
//		Logger OutputLogger;
		
		ErrorLogger = new Logger(logFilePath + "curr.error.log");
		MatchLogger = new Logger(logFilePath + "curr.match.log");
		NoMatchLogger = new Logger(logFilePath + "curr.nomatch.log");
		
		
		List<OutputSummary> refList = new ArrayList<OutputSummary>();

		List<OutputSummary> subList = new ArrayList<OutputSummary>();
		
		Scanner scr;
		try {
			scr = new Scanner(new File(refsFile));
		} catch (FileNotFoundException e) {
			System.out.println("ERROR: Cannot find file " + refsFile);
			e.printStackTrace();
			return;
		}
		
		/*Read in reference implementations*/
		readRefSummaries(refList, scr, subsToSkip);
		
		
		try {
			scr = new Scanner(new File(subsFile));
		} catch (FileNotFoundException e) {
			System.out.println("ERROR: Cannot find file " + subsFile);
			return;
		}
		
		/*Read in submissions*/
		while (scr.hasNext()) {
			String currFile = scr.next();
			if (subsToSkip.contains(currFile))
				continue;
			OutputSummary opSum = new OutputSummary(currFile, currFile, ErrorLogger);
			boolean ret = opSum.parse();
			if (!ret) {
				System.out.println("Failed to parse "+ opSum.getProgOutputSummaryName());
				ErrorLogger.closeLog();
				MatchLogger.closeLog();
				NoMatchLogger.closeLog();
				SpuriousGlobalInitLogger.closeLog();
				continue;
			}
			
			for(OutputSummary ref: refList){
				System.out.println("Matching against ref "+ ref.progOutputSummaryName);
				Logger feedbackLogger = new Logger(opSum.progOutputSummaryName + ".feedback");
				feedbackLogger.writeLog("Matching against ref "+ ref.progOutputSummaryName);
				if(!opSum.hasNewline() && ref.hasNewline()){
					feedbackLogger.writeLog("Output expression missing newline");
					opSum.score += 1;
				}
				if(!ref.exprTypes.equals(opSum.exprTypes)){
					feedbackLogger.writeLog("Output expression types mismatch");
					feedbackLogger.writeLog("Ref: "+ref.exprTypes.toString());
					feedbackLogger.writeLog("Sub: "+opSum.exprTypes.toString());
					opSum.score += 1;
				}
				opSum.processAsSubmission(ref, feedbackLogger);
				
				feedbackLogger.closeLog();
				System.out.println("==============================================");
			}
		}	
		
		System.out.println("Done!");
	}

	
	
	public static void readRefSummaries(List<OutputSummary> refList, Scanner scr, Set<String> toSkip) {
		while (scr.hasNext()) {
			String currFile = scr.next();
			if (toSkip.contains(currFile))
				continue;
			OutputSummary opSum = new OutputSummary(currFile, currFile, ErrorLogger);
			boolean ret = opSum.parse();
			if (!ret) {
				System.out.println("Failed to parse "+ opSum.getProgOutputSummaryName());
				ErrorLogger.closeLog();
				MatchLogger.closeLog();
				NoMatchLogger.closeLog();
//				SpuriousGlobalInitLogger.closeLog();
				continue;
			}
			opSum.processAsReference();
			refList.add(opSum);
		}
	}
}
