package feedback;

import java.sql.*;

public class ResultStats{
	
	public final boolean succeeded;
	public final int numInserts;
	public final int numReplace;
	public final String reasonForFailure;
	public int score;
	public int numSpurious;
	
	public String feedbackText;
	public Long processingTime;
	public int sizeBeforeOpt = 0;
	public int sizeAfterOpt = 0;
	
	public ResultStats(boolean successStatus, int score){
		succeeded = successStatus;
		numInserts = 0;
		numReplace = 0;
		numSpurious = 0;
		reasonForFailure = "";
		this.score = score;
	}

	public ResultStats(boolean succeeded, int numInserts, int numReplace,
			String reasonForFailure, int score) {
		super();
		this.succeeded = succeeded;
		this.numInserts = numInserts;
		this.numReplace = numReplace;
		numSpurious = 0;
		this.reasonForFailure = reasonForFailure;
		this.score = score;
	}

	public ResultStats(boolean succeeded, int numInserts, int numReplace, int numSpurious,
			String reasonForFailure, int score) {
		this.succeeded = succeeded;
		this.numInserts = numInserts;
		this.numReplace = numReplace;
		this.numSpurious = numSpurious;
		this.reasonForFailure = reasonForFailure;
		this.score = score;
	}

	public ResultStats(boolean succeeded, int numInserts, int numReplace, int numSpurious,
			String reasonForFailure, int score, int origSize, int newSize) {
		this.succeeded = succeeded;
		this.numInserts = numInserts;
		this.numReplace = numReplace;
		this.numSpurious = numSpurious;
		this.reasonForFailure = reasonForFailure;
		this.score = score;
		this.sizeBeforeOpt = origSize;
		this.sizeAfterOpt = newSize;
	}

	@Override
	public String toString() {
		return "ResultStats [succeeded=" + succeeded + ", numInserts="
				+ numInserts + ", numReplace=" + numReplace
				+ ", reasonForFailure=" + reasonForFailure + ", score=" + score
				+ ", numSpurious=" + numSpurious + "]";
	}	
	
	public int getNumCorrections(){
		return numSpurious + numReplace + numInserts;
	}
	
	/*SubmissionID, ReferenceName, IsFeedbackGenerated, FeedbackText,
	 *NumCorrections, Score, ProcessingTime, SizeBeforeOpt, SizeAfterOpt */
	public void writeSelf(Connection connection, int subId, 
			String ref, String tablename) throws SQLException{
//		String query = "INSERT INTO "+ tablename + 
//				" VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?)" +
//				" ON DUPLICATE KEY UPDATE " +
//				"`SubmissionId`=values(SubmissionId), " +
//				"`ReferenceName`=values(ReferenceName), " +
//				"`IsFeedbackGenerated`=values(IsFeedbackGenerated), " +
//				"`FeedbackText`=values(FeedbackText), " +
//				"`NumCorrections`=values(NumCorrections), " +				
//				"`Score`=values(Score), " +
//				"`ProcessingTime`=values(ProcessingTime), " +
//				"`SizeBeforeOpt`=values(SizeBeforeOpt), " +
//				"`SizeAfterOpt`=values(SizeAfterOpt);";
//		String query = "REPLACE INTO "+tablename+" VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?);";
		PreparedStatement preparedStatement = null;
		if((MainDriver.databaseName + ".input_results").equals(tablename)){
			preparedStatement = MainDriver.preparedStatementForInput;
		}
		else if((MainDriver.databaseName + ".init_results").equals(tablename)){
			preparedStatement = MainDriver.preparedStatementForInit;
		}
		else if((MainDriver.databaseName + ".update_results").equals(tablename)){
			preparedStatement = MainDriver.preparedStatementForUpdate;
		}
		else
			preparedStatement = MainDriver.preparedStatementForOutput;
		preparedStatement.setInt(1, subId);
	    preparedStatement.setString(2, ref);
	    preparedStatement.setBoolean(3, this.succeeded);
	    
	    if(this.succeeded)
	    	preparedStatement.setString(4, this.feedbackText);
	    else
	    	preparedStatement.setString(4, this.reasonForFailure);
	    
	    preparedStatement.setInt(5, this.getNumCorrections());
	    preparedStatement.setInt(6, this.score);
	    
	    preparedStatement.setString(7, this.processingTime.toString());
	    preparedStatement.setInt(8, this.sizeBeforeOpt);
	    preparedStatement.setInt(9, this.sizeAfterOpt);
	    
//	    preparedStatement.executeUpdate();
	    preparedStatement.addBatch();
	}
	
}