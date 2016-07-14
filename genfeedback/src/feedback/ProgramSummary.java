package feedback;

import java.util.ArrayList;
import java.util.List;

import feedback.init.InitSummary;
import feedback.inputs.ProgramInputs;
import feedback.logging.Logger;

public class ProgramSummary {
	public String progName;
	public InitSummary Init;
	
	public Logger ErrorLogger;
	public ProgramInputs Inps;
	
	public ProgramSummary(String name, Logger Error, ProgramInputs I) {
		progName = name;
		ErrorLogger = Error;
		Inps = I;

		Init = new InitSummary(name, ErrorLogger, Inps);
	}
	
	public boolean readSummary() {
		boolean ret = Init.parse();
		if (ret)
			return true;
		else
			return false;
	}
	
//	public boolean generateFeedback(List<ProgramSummary> refList, Logger FeedbackLog) {
//		List<InitSummary> refInits = new ArrayList<InitSummary>();
//		for (ProgramSummary P: refList) {
//			refInits.add(P.Init);
//		}
//		boolean retInit = Init.generateFeedback(refInits, FeedbackLog);
//		if (retInit)
//			return true;
//		else
//			return false;
//	}
}
