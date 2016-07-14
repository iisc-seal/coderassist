package feedback;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Scanner;
import java.util.Set;

import feedback.init.InitSummary;
import feedback.inputs.ProgramInputs;
import feedback.logging.Logger;
import feedback.output.OutputSummary;
import feedback.update.UpdateSummary;
import feedback.utils.FragmentSummary;
import feedback.utils.HelperFunctions;
import feedback.utils.Statement;

public class SanityChecker {

	public static void main(String[] args) {
		
		long startTime = System.nanoTime();
		
		if (args.length != 1) {
			System.out.println("ERROR: Invalid number of arguments\n");
			System.out.println("Input subsFile logs\n");
			return;
		}
		String subsFile = args[0];
		Set<String> subsToSkip = new HashSet<String>();
		subsToSkip.add("/home/anirudh/mar15/submissions/SUMTRIAN/dr_go_fast/428669.ac.c");
		subsToSkip.add("/home/anirudh/mar15/submissions/SUMTRIAN/guru1306/377513.ac.cpp");
		subsToSkip.add("/home/anirudh/mar15/submissions/SUMTRIAN/mjansari/2709269.wa.cpp");
		subsToSkip.add("/home/anirudh/mar15/submissions/SUMTRIAN/mjansari/2709274.wa.cpp");

		subsToSkip.add("/home/anirudh/mar15/submissions/SUMTRIAN/nitiraj/13944.ac.cpp");
		subsToSkip.add("/home/anirudh/mar15/submissions/SUMTRIAN/pradinski/2252404.wa.cpp");
		subsToSkip.add("/home/anirudh/mar15/submissions/MGCRNK/ankurmarshall/4116309.wa.cpp");
		subsToSkip.add("/home/anirudh/mar15/submissions/MGCRNK/ankurmarshall/4116314.ac.cpp");

		subsToSkip.add("/home/anirudh/mar15/submissions/MGCRNK/disguise/2076912.ac.cpp");
		subsToSkip.add("/home/anirudh/mar15/submissions/MGCRNK/grv95/4600310.ac.cpp");
		subsToSkip.add("/home/anirudh/mar15/submissions/MGCRNK/math/1629396.wa.cpp");
		subsToSkip.add("/home/anirudh/mar15/submissions/MGCRNK/nitish712/1632372.wa.cpp");
		
		subsToSkip.add("/home/anirudh/mar15/submissions/MGCRNK/nitish712/1632425.wa.cpp");
		subsToSkip.add("/home/anirudh/mar15/submissions/MGCRNK/sergej_t/2569672.ac.c");
		subsToSkip.add("/home/anirudh/mar15/submissions/MGCRNK/shivamh71/4853326.ac.cpp");
		subsToSkip.add("/home/anirudh/mar15/submissions/MGCRNK/tyrant/2538342.ac.cpp");
		
		Logger ErrorLogger = new Logger("/home/anirudh/" + "curr.error.log");
		
		Scanner scr;
		
		try {
			scr = new Scanner(new File(subsFile));
		} catch (FileNotFoundException e) {
			System.out.println("ERROR: Cannot find file " + subsFile);
			return;
		}
		
		
		while (scr.hasNext()) {
//			String currentSub = "/home/anirudh/MARCHA1/"+scr.next();
			
			String currentSub = scr.next();
			
			if (subsToSkip.contains(currentSub))
				continue;
			System.out.println("Processing -> "+ currentSub);
			
			String trunc = currentSub.replace(".wa.cpp", "").replace(".ac.cpp", "").replace(".wa.c", "").replace(".ac.c", "");
			
			/*Check input sanity*/
			InitSummary initSummary = new InitSummary(trunc, ErrorLogger, null);
//			initSummary.fragments = new UpdateSummary(currentSub, trunc + ".init.summary", ErrorLogger);
			boolean parsedInit = initSummary.parse();
			if(!parsedInit){
				System.out.println("Failed to parse " + initSummary.progInitSummaryName+ "%"+ currentSub);
				continue;
			}
			
			HelperFunctions.identifySuspicious(initSummary.allInits, initSummary.progInitSummaryName + "%"+ currentSub);
			HelperFunctions.identifySuspiciousInit(initSummary.fragments.updateStatements, initSummary.progInitSummaryName+ "%"+ currentSub);
			
			/*Check update sanity*/
			UpdateSummary updateSum = new UpdateSummary(currentSub, trunc+".update.summary", ErrorLogger);
			System.out.println("DEBUG: File " + updateSum.getProgUpdateSummaryName());
			//OutputLogger.writeLog("File: " + updateSum.progName);
			boolean ret = updateSum.parse();
			
			if(!ret){
				System.out.println("Failed to parse " + updateSum.getProgUpdateSummaryName()+ "%"+ currentSub);
				continue;
			}

			HelperFunctions.identifySuspicious(updateSum.updateStatements, updateSum.progUpdateSummaryName+ "%"+ currentSub);
			
			/*Check output sanity*/
			OutputSummary opSum = new OutputSummary(currentSub, trunc+".output.summary", ErrorLogger);
			ret = opSum.parse();
			if (!ret) {
				System.out.println("Failed to parse " + opSum.progOutputSummaryName+ "%"+ currentSub);
				continue;
			}
			HelperFunctions.identifySuspicious(opSum.outputStatements, opSum.getProgOutputSummaryName()+ "%"+ currentSub);
		}
	}		
}
