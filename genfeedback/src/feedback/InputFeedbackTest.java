package feedback;

import static org.junit.Assert.*;

import org.junit.BeforeClass;
import org.junit.Test;

import feedback.init.InitSummary;
import feedback.inputs.ProgramInputs;
import feedback.logging.Logger;
import feedback.output.OutputSummary;
import feedback.update.UpdateSummary;
import feedback.utils.Block;
import feedback.utils.TypeSummary;

public class InputFeedbackTest {
	
	static String progName = "/home/anirudh/workspace/fixdp/testFiles/testProg.wa.c";
//	static String initFile = "/home/anirudh/workspace/fixdp/testFiles/871229.init.summary";
	static String inputFile = "/home/anirudh/workspace/fixdp/testFiles/1615411.typemap";

	static String refProgName = "/home/anirudh/workspace/fixdp/testFiles/ref.test";
//	static String refInitFile = "/home/anirudh/workspace/fixdp/testFiles/ref.4.init.summary";
	static String refInputFile = "/home/anirudh/workspace/fixdp/testFiles/ref.test";
	
	static String errLog = "/home/anirudh/workspace/fixdp/testFiles/errorLog";
	
	@BeforeClass
	public static void setUpBeforeClass() throws Exception {
	}

	@Test
	public void test() {
		
		Logger ErrorLogger = new Logger(errLog);
		
//		InputSummary inputSum = new InputSummary(refProgName, refInputFile);
//		inputSum.parse();
		ProgramInputs p = new ProgramInputs();
		p.readInputConstriantsMGCRNK();
		
		InitSummary refSum = new InitSummary(refProgName, ErrorLogger, null);
		boolean ret = refSum.parse();
//		refSum.prepareReference();
		
		String trunc = progName.replace(".wa.cpp", "").replace(".ac.cpp", "").replace(".wa.c", "").replace(".ac.c", "");
		int index = refProgName.lastIndexOf('/');
		String refName = refProgName.substring(index + 1);
		Logger feedbackLogger = new Logger(trunc + ".feedback." + refName);
		InitSummary subSum = new InitSummary(trunc, ErrorLogger, null);
		ret = subSum.parse();
		subSum.compareSummary(refSum, feedbackLogger);
		feedbackLogger.closeLog();
	}

}
