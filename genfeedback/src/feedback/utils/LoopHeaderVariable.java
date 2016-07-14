package feedback.utils;

import feedback.logging.Logger;

public class LoopHeaderVariable {
	String index;
	String lowerBound;
	String upperBound;
	private LoopHeaderVariable outer;
	
	public LoopHeaderVariable(String index, String lowerBound, String upperBound, LoopHeaderVariable outer) {
		super();
		this.index = index;
		this.lowerBound = lowerBound;
		this.upperBound = upperBound;
		this.outer = outer;
	}
	
	public LoopHeaderVariable(String[] components, LoopHeaderVariable outer) {
		super();
		this.index = components[0];
		this.lowerBound = components[1];
		this.upperBound = components[2];
		this.outer = outer;
	}
	
	public boolean compare(LoopHeaderVariable other, Logger feedbackLogger){
		boolean matched = true;
		if(other == null){
			matched = false;
			feedbackLogger.writeLog("Submission is missing an input loop");
		}
		
		Integer lb = TypeSummary.getValue(lowerBound, TypeSummary.constraintMap);
		Integer ub = TypeSummary.getValue(upperBound, TypeSummary.constraintMap);
		Integer otherLb = TypeSummary.getValue(other.lowerBound, TypeSummary.constraintMap);
		Integer otherUb = TypeSummary.getValue(other.upperBound, TypeSummary.constraintMap);
		
		if(this.outer == null || other.outer == null){
			if(lb == null || ub == null || otherLb == null || otherUb == null){
				feedbackLogger.writeLog("Unable to give feedback - unknown symbol in loop header");
				return false;
			}
		}
		
		if(ub == null && otherUb == null){
//			if(this.outer == null || other.outer == null){
//				feedbackLogger.writeLog("Unable to give feedback - comparing mismatched loops");
//				return false;
//			}
			if(!(upperBound.equals(outer.index) && other.upperBound.equals(other.outer.index))){
				feedbackLogger.writeLog("Loop is expected to run up to immediate outer loop index");
				return false;
			}
		}
		
		else{
			if(ub != null && lb != null && otherUb != null && otherLb != null){
				if(Math.abs(ub - lb) > Math.abs(otherUb - otherLb)){
				feedbackLogger.writeLog("Loop needs to read in " + Math.abs(ub - lb));
				feedbackLogger.writeLog("Submission reads in " + Math.abs(otherUb - otherLb));
				matched = false;
				}
			}
			else{
				feedbackLogger.writeLog("Unable to give feedback - unknown symbol in loop header");
				matched = false;
			}
		}
		return matched;
	}
}
