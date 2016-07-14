package feedback;


import org.junit.BeforeClass;
import org.junit.Test;

import feedback.init.InitSummary;
import feedback.inputs.ProgramInputs;
import feedback.logging.Logger;
import feedback.output.OutputSummary;
import feedback.update.UpdateSummary;
import feedback.utils.TypeSummary;

public class MainDriverTest {
	
	static String progName = "/home/anirudh/workspace/fixdp/testFiles/1838600.wa.c";
//	static String initFile = "/home/anirudh/workspace/fixdp/testFiles/871229.init.summary";
	static String inputFile = "/home/anirudh/workspace/fixdp/testFiles/1838600.typemap";
//	static String updateFile = "/home/anirudh/workspace/fixdp/testFiles/871229.update.summary";
//	static String outputFile = "/home/anirudh/workspace/fixdp/testFiles/871229.output.summary";
	
	static String refProgName = "/home/anirudh/workspace/fixdp/testFiles/ref.10";
//	static String refInitFile = "/home/anirudh/workspace/fixdp/testFiles/ref.4.init.summary";
	static String refInputFile = "/home/anirudh/workspace/fixdp/testFiles/ref.10.typemap";
	static String refUpdateFile = "/home/anirudh/workspace/fixdp/testFiles/ref.10.update.summary";
	static String refOutputFile = "/home/anirudh/workspace/fixdp/testFiles/ref.10.output.summary";
	
	static String errLog = "/home/anirudh/workspace/fixdp/testFiles/errorLog";
	
	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
	}

	@Test
	public void test() {
		
		Logger ErrorLogger = new Logger(errLog);
		
		TypeSummary typeSum = new TypeSummary(refProgName, refInputFile);
		typeSum.parse();
		ProgramInputs p = new ProgramInputs();
		p.readInputConstriantsMGCRNK();
		
		UpdateSummary inputSum = new UpdateSummary(refProgName, refUpdateFile, ErrorLogger);
		System.out.println("DEBUG: File " + inputSum.getProgUpdateSummaryName());
		boolean ret = inputSum.parse();
		inputSum.prepareReference();
		
		InitSummary inSum = new InitSummary(refProgName, ErrorLogger, null);
		ret = inSum.parse();
		inSum.prepareReference();
		
		UpdateSummary updateSum = new UpdateSummary(refProgName, refUpdateFile, ErrorLogger);
		System.out.println("DEBUG: File " + updateSum.getProgUpdateSummaryName());
		ret = updateSum.parse();
		updateSum.prepareReference();
		
		OutputSummary opSum = new OutputSummary(refProgName, refOutputFile, ErrorLogger);
		ret = opSum.parse();
		opSum.prepareReference();
		
		SummaryModule refModule = new SummaryModule(typeSum, inputSum, inSum, updateSum, opSum, refProgName);
		
		FeedbackSummary feedSummary = new FeedbackSummary(progName, refModule, null, ErrorLogger);
		feedSummary.processSubmission();
		
	}

}
