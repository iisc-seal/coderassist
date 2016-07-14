package feedback;

import feedback.init.InitSummary;
import feedback.inputs.ProgramInputs;
import feedback.logging.Logger;
import feedback.output.OutputSummary;
import feedback.update.UpdateSummary;
import feedback.utils.Expression;
import feedback.utils.HelperFunctions;
import feedback.utils.TypeSummary;
import feedback.utils.Variable;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.util.Map;
import java.util.Map.Entry;

public class FeedbackSummary {
	// the reference this was generated against
	String referenceName;
	
	// the summary this was generated for
	String programName;
	
	// A data structure representing constraints on the program inputs
	ProgramInputs inputs;

	// The reference we're trying to match this against
	SummaryModule ref;
	
	// Summaries for init, update and output
	private InitSummary initSummary;

	private UpdateSummary updateSummary;

	private UpdateSummary outputSummary;
	
	// the feedback logger object associated with this submission
	private Logger feedbackLogger;
	
	// the error logger object associated with this submission
	private Logger errorLogger;

	private AggregateResult aggregateResults;

	private Expression precondition;

	private UpdateSummary inputSummary;	
	private TypeSummary typeSummary;
	/*
	 * 1. Read in the names of the reference and submission
	 * 2. Set up the SummaryModule for the submission using the reference
	 * 3. Generate feedback
	 */
	FeedbackSummary(String progName, SummaryModule ref, ProgramInputs I, 
			Logger error){
		this.programName = progName;
		this.ref = ref;
		this.inputs = I;
		this.errorLogger = error;
		String trunc = getPrefix(programName);		
		String refName = getReferenceName();
		feedbackLogger = new Logger(trunc + ".feedback." + refName);
		precondition = I.getAsConstraint();
		aggregateResults = new AggregateResult();		
	}

	private int getSubmissionId(){
		int index = programName.lastIndexOf('/');
		String id = programName.substring(index + 1);
		id = getPrefix(id);
		return Integer.parseInt(id);
	}

	private String getReferenceName(){
		int index = ref.getName().lastIndexOf('/');
		String refName = ref.getName().substring(index + 1);
		return refName;
	}
	
	public ResultStats checkCompatibility(TypeSummary refSum, TypeSummary subSum){
		Map<String, Variable> refDeclVar = refSum.getDeclaredVars();
		Map<String, Variable> subDeclVar = subSum.getDeclaredVars();
		ResultStats typeResult = null;
		if(refDeclVar.size() != subDeclVar.size()){
//			feedbackLogger.writeLog("Mismatched # of variables");
			typeResult = new ResultStats(false, 0, 0, "Mismatched # of variables", 0);
			return typeResult;
		}
		// Print out the dp array name
		for (Entry<String, Variable> entry: subDeclVar.entrySet()) {
			if (entry.getKey().contains("dp_Array_")) {
				feedbackLogger.writeLog("We identified \"" + entry.getValue().getRealName() + "\" as the DP array");
				feedbackLogger.writeLog("===============================================");
				break;
			}
		}
		feedbackLogger.writeLog("Type Declaration:");
		feedbackLogger.writeLog("-----------------");
		int currFeedbackLength = feedbackLogger.input.length();
//		boolean matched = true;
		for(Entry<String, Variable> entry : refDeclVar.entrySet()){
			String canonicalName = entry.getKey();
			Variable v = entry.getValue();
			Variable ov = subDeclVar.get(canonicalName);
			if(ov == null){
//				feedbackLogger.writeLog("Variable " + canonicalName + " missing in sub");
				typeResult = new ResultStats(false, 0, 0, "Missing variable in sub", 0);
				return typeResult;
			}
			if (typeResult != null)
				System.out.println("Before v.getFeedback(): type score: " + typeResult.score);
			else
				System.out.println("Before v.getFeedback(): type score: 0");
			ResultStats tempResult = v.getFeedback(ov, feedbackLogger, precondition, subSum.getDeclaredVars());
			boolean successFlag; int score;
			if (typeResult != null) {
				successFlag = typeResult.succeeded && tempResult.succeeded;
				score = typeResult.score + tempResult.score;
			} else {
				successFlag = tempResult.succeeded;
				score = tempResult.score;
			}
			typeResult = new ResultStats(successFlag, score);
			System.out.println("After v.getFeedback(): type score: " + typeResult.score);
		}
		for(Entry<String, Variable> entry : subDeclVar.entrySet()){
			String canonicalName = entry.getKey();
			Variable ov = refDeclVar.get(canonicalName);
			if(ov == null){
//				feedbackLogger.writeLog("Missing variable from ref " + canonicalName);
				return new ResultStats(false, 0, 0, "Missing variable from ref", 0);
			}						
		}
		// If there was no feedback for input (actually type declaration), then it matched successfully
		if (feedbackLogger.input.length() - currFeedbackLength == 0) {
			feedbackLogger.writeLog("Correct");
		}
		return typeResult;
	}
	
	public AggregateResult processSubmission() {	
		long totalTimeStart = System.nanoTime();
		
		long startTime = System.nanoTime();
		int currFeedbackLength;
//		feedbackLogger.writeLog("--------------Matching types--------------");
		String trunc = getPrefix(programName);
		typeSummary = new TypeSummary(programName, trunc + ".typemap");
		typeSummary.parse();
//		feedbackLogger.writeLog("Matching " + trunc + ".typemap");
//		ref.typeSum.getFeedback(typeSum, feedbackLogger);
		long endTime = System.nanoTime();
		aggregateResults.type = checkCompatibility(ref.typeSum, typeSummary);
		if(!aggregateResults.type.succeeded){
			return failOnVariableCorrespondence(trunc, startTime, endTime);
		}
		typeSummary.incScore(aggregateResults.type.score);
				
		System.out.println("Type score: " + aggregateResults.type.score);
		feedbackLogger.setMode(Logger.Modes.INPUT);
		inputSummary = new UpdateSummary(programName, trunc + ".input.summary", errorLogger);
		startTime = System.nanoTime();
		boolean parsedInput = inputSummary.parse();
		if(parsedInput){
			System.out.println("--------------Matching against input--------------");
//			feedbackLogger.writeLog("--------------Matching against input--------------");
			feedbackLogger.writeLog("===============================================");
			feedbackLogger.writeLog("INPUT:");
			feedbackLogger.writeLog("------");
			currFeedbackLength = feedbackLogger.input.length();
			aggregateResults.input = HelperFunctions.matchBlocks(ref.input.blocks, inputSummary.blocks, 
					feedbackLogger, precondition, ref.input.progUpdateSummaryName, 
					ref.input.progName, inputSummary.progUpdateSummaryName, ref.typeSum.getDeclaredVars(), typeSummary.getDeclaredVars());
			inputSummary.incScore(aggregateResults.input.score);
			// If there was no feedback for input, then input matched successfully
			if (feedbackLogger.input.length() - currFeedbackLength == 0) {
				feedbackLogger.writeLog("Correct");
			}
		}
		else{
			aggregateResults.input = new ResultStats(false, 0, 0, "Failed to parse", 5);
			inputSummary.incScore(5);
		}
		endTime = System.nanoTime();
		aggregateResults.input.processingTime = endTime - startTime;
		aggregateResults.input.feedbackText = feedbackLogger.input.toString();
		
		feedbackLogger.setMode(Logger.Modes.INIT);
		System.out.println("--------------Matching against init--------------");
//		feedbackLogger.writeLog("--------------Matching against init--------------");
		feedbackLogger.writeLog("===============================================");
		feedbackLogger.writeLog("DP INITIALIZATION:");
		feedbackLogger.writeLog("------------------");
		currFeedbackLength = feedbackLogger.init.toString().trim().length();
		System.out.println("CurrFeedback Length: " + currFeedbackLength);
		initSummary = new InitSummary(trunc, errorLogger, inputs);
		startTime = System.nanoTime();
		boolean parsedInit = initSummary.parse();
		boolean expressAsRanges;
		if(parsedInit){
			expressAsRanges = initSummary.getAsRanges();
			if(expressAsRanges && ref.init.isRange){
				aggregateResults.init = initSummary.compareSummary(ref.init, feedbackLogger, typeSummary.getDeclaredVars());
			}			
			else if(!expressAsRanges && !ref.init.isRange){
//				ref.init.prepareReference();
				aggregateResults.init = HelperFunctions.matchBlocks(ref.init.blocks, initSummary.blocks, 
						feedbackLogger, precondition, ref.init.progInitSummaryName, 
						ref.init.progName, initSummary.progInitSummaryName, ref.typeSum.getDeclaredVars(), typeSummary.getDeclaredVars());
				initSummary.score += aggregateResults.init.score;
			}
			else{
				// output reference summary
				String feedback = HelperFunctions.rewriteUsingSubmissionProgramVariables(ref.init.blocks, feedbackLogger, 
						ref.typeSum.getDeclaredVars(), typeSummary.getDeclaredVars());
				feedbackLogger.writeLog("Use the reference summary instead\n");
				feedbackLogger.writeLog(feedback);
				aggregateResults.init = new ResultStats(true, feedback.length());
			}
			// If there was no feedback for init, then input matched successfully
			if (feedbackLogger.init.toString().trim().length() - currFeedbackLength == 0) {
				feedbackLogger.writeLog("Correct");
			}
		} else {
			
			aggregateResults.init = new ResultStats(false, 0, 0, "Failed to parse", 5);
			initSummary.score+=5;
		}
		endTime = System.nanoTime();
		aggregateResults.init.processingTime = endTime - startTime;
		aggregateResults.init.feedbackText = feedbackLogger.init.toString();
		
//		initSummary.score = initSummary.fragments.getScore();
//		}
		feedbackLogger.setMode(Logger.Modes.UPDATE);
		updateSummary = new UpdateSummary(programName, trunc + ".update.summary", errorLogger);
		startTime = System.nanoTime();
		boolean parsedUpdate = updateSummary.parse();
		if(parsedUpdate){
			System.out.println("--------------Matching against update--------------");
//			feedbackLogger.writeLog("--------------Matching against update--------------");
			feedbackLogger.writeLog("===============================================");
			feedbackLogger.writeLog("DP UPDATE:");
			feedbackLogger.writeLog("----------");
			currFeedbackLength = feedbackLogger.update.length();
//			updateSummary.processAsSubmission(ref.update, feedbackLogger);
			aggregateResults.update = HelperFunctions.matchBlocks(ref.update.blocks, updateSummary.blocks, 
					feedbackLogger, precondition, ref.update.progUpdateSummaryName, 
					ref.update.progName, updateSummary.progUpdateSummaryName, ref.typeSum.getDeclaredVars(), typeSummary.getDeclaredVars());
			updateSummary.incScore(aggregateResults.update.score);
			// If there was no feedback for update, then input matched successfully
			if (feedbackLogger.update.length() - currFeedbackLength == 0) {
				feedbackLogger.writeLog("Correct");
			}
		}
		else{
			aggregateResults.update = new ResultStats(false, 0, 0, "Failed to parse", 5);
			updateSummary.incScore(5);
		}
		endTime = System.nanoTime();
		aggregateResults.update.processingTime = endTime - startTime;
		aggregateResults.update.feedbackText = feedbackLogger.update.toString();
		
		feedbackLogger.setMode(Logger.Modes.OUTPUT);
		outputSummary = new UpdateSummary(programName, trunc + ".output.summary", errorLogger);
		startTime = System.nanoTime();
		boolean parsedOutput = outputSummary.parse();
		if(parsedOutput){
			System.out.println("--------------Matching against output--------------");
//			feedbackLogger.writeLog("--------------Matching against output--------------");
			feedbackLogger.writeLog("===============================================");
			feedbackLogger.writeLog("OUTPUT:");
			feedbackLogger.writeLog("-------");
			currFeedbackLength = feedbackLogger.output.length();
			aggregateResults.output = HelperFunctions.matchBlocks(ref.output.blocks, outputSummary.blocks, 
					feedbackLogger, precondition, ref.output.progUpdateSummaryName, 
					ref.output.progName, outputSummary.progUpdateSummaryName, ref.typeSum.getDeclaredVars(), typeSummary.getDeclaredVars());
			outputSummary.incScore(aggregateResults.output.score);
			// If there was no feedback for output, then input matched successfully
			if (feedbackLogger.output.length() - currFeedbackLength == 0) {
				feedbackLogger.writeLog("Correct");
			}
		}
		else{
			aggregateResults.output = new ResultStats(false, 0, 0, "Failed to parse", 5);
			outputSummary.incScore(5);
		}
		endTime = System.nanoTime();
		aggregateResults.output.processingTime = endTime - startTime;
		aggregateResults.output.feedbackText = feedbackLogger.output.toString();
		
		long totalTimeEnd = System.nanoTime();
		//writeToDatabase(aggregateResults, totalTimeEnd - totalTimeStart);
		
//		feedbackLogger.writeLog("Total: "+ getScore());
		feedbackLogger.writeLog("===============================================");
		System.out.println("Total: " + getScore() + " type =" + aggregateResults.type.score + " input=" + inputSummary.getScore()
				+ " init=" + initSummary.score + " update=" + updateSummary.getScore()
				+ " output=" + outputSummary.getScore());
		feedbackLogger.closeLog();
		return aggregateResults;
	}

	public static String getPrefix(String s) {
		return s.replace(".wa.copy.cpp", "").
				   				   replace(".ac.copy.cpp", "").
				   				   replace(".wa.copy.c", "").
								   replace(".ac.copy.c", "").
								   replace(".wa.cpp", "").
								   replace(".ac.cpp", "").
								   replace(".wa.c", "").
								   replace(".ac.c", "");
	}

	private AggregateResult failOnVariableCorrespondence(String trunc, long totalTimeEnd, long totalTimeStart) {
//		feedbackLogger.writeLog("Could not establish variable correspondence with this reference");
		aggregateResults.input = new ResultStats(false, 0, 0, feedbackLogger.input.toString(), 15);
		aggregateResults.input.processingTime = (long) 0;
		aggregateResults.init = new ResultStats(false, 0, 0, "Var Correspondence Failed", 15);
		aggregateResults.init.processingTime = (long) 0;
		aggregateResults.update = new ResultStats(false, 0, 0, "Var Correspondence Failed", 15);
		aggregateResults.update.processingTime = (long) 0;
		aggregateResults.output = new ResultStats(false, 0, 0, "Var Correspondence Failed", 15);
		aggregateResults.output.processingTime = (long) 0;
		inputSummary = new UpdateSummary(programName, trunc + ".input.summary", errorLogger);
		initSummary = new InitSummary(trunc, errorLogger, inputs);
		updateSummary = new UpdateSummary(programName, trunc + ".update.summary", errorLogger);
		outputSummary = new UpdateSummary(programName, trunc + ".output.summary", errorLogger);
		inputSummary.incScore(15); initSummary.score+=15; updateSummary.incScore(15); outputSummary.incScore(15);
		
		//writeToDatabase(aggregateResults, totalTimeEnd - totalTimeStart);
		feedbackLogger.closeLog();
		return aggregateResults;
	}
	
		
	private void writeToDatabase(AggregateResult aggregateResults, Long time){		
		writeResultTables(time, MainDriver.connection);		
	}

	private void writeResultTables(Long time, Connection connection) {
		PreparedStatement preparedStatement = MainDriver.preparedStatementForMain;
		int submissionId = getSubmissionId();
		String refName = getReferenceName();
		
		try {			 
			 preparedStatement.setInt(1, submissionId);
		     preparedStatement.setString(2, refName);
		     preparedStatement.setString(3, getCodechefStatus());
		     preparedStatement.setInt(4, getScore());
		     preparedStatement.setString(5, time.toString());	
		     preparedStatement.addBatch();
 			 aggregateResults.writeAggregateResult(connection, submissionId, refName);
		} catch (SQLException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();			
		}
	}

	private String getCodechefStatus() {
		if(programName.contains(".ac."))
			return "ac";
		else
			return "wa";
	}

	// get the score obtained on matching against this reference
	public int getScore(){
		return typeSummary.getScore() + inputSummary.getScore() + initSummary.score 
				+ updateSummary.getScore() + outputSummary.getScore();
	}
	
	// return the feedback obtained on matching against this reference
	public String feedback(){
		return "";
	}
}
