package feedback;

import java.util.HashMap;
import java.util.Map;
import java.util.SortedMap;
import java.util.TreeMap;

/*
 * A result summary object summarizes the results of matching a submission
 * against every available reference
*/
public class ResultSummary {

	// Was there at least one reference against which feedback was generated
	// for init, update and output
	boolean succeeded;
	
	// Stores references against which feedback was generated, sorted by score
	SortedMap<Integer, String> matches;
	
	// For each reference, stores the aggregate result of matching against that
	// reference. AggregateResult = Result for init, update and output
	SortedMap<String, AggregateResult> aggregateSummary;
	
	public ResultSummary() {
		this.matches = new TreeMap<Integer, String>();
		this.aggregateSummary = new TreeMap<String, AggregateResult>();
	}
	
	public ResultSummary(SortedMap<String, AggregateResult> aggregateSummary){
		this.aggregateSummary = aggregateSummary;
		this.matches = new TreeMap<Integer, String>();
		for(Map.Entry<String, AggregateResult> entry: aggregateSummary.entrySet()){
			String refName = entry.getKey();
			AggregateResult res = entry.getValue();
			
			if(res.init.succeeded && res.output.succeeded && res.update.succeeded){
				succeeded = true;
				System.out.println(refName + " Succeeded");
				int score = res.type.score + res.input.score + res.init.score + res.output.score + res.update.score;
				matches.put(score, refName);
			}
			
		}
	}

	@Override
	public String toString() {
		return "ResultSummary [succeeded=" + succeeded + ", matches=" + matches
				+ ", aggregateSummary=" + aggregateSummary + "]";
	}
	
	
	
}
