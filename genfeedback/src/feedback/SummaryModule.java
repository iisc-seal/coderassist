package feedback;

import feedback.init.InitSummary;
import feedback.output.OutputSummary;
import feedback.update.UpdateSummary;
import feedback.utils.TypeSummary;

// hold the summaries for a reference
public class SummaryModule {
	public TypeSummary typeSum;
	public UpdateSummary input;
	public InitSummary init;
	public UpdateSummary update;
	public UpdateSummary output;
	private String name;
	
	public SummaryModule(TypeSummary typeSum, UpdateSummary inputSum, 
			InitSummary init, UpdateSummary update, UpdateSummary output, 
			String name) {
		super();
		this.typeSum = typeSum;
		this.input = inputSum;
		this.init = init;
		this.update = update;
		this.output = output;
		this.name = name;
	}

	public String getName() {
		return name;
	}
	
	
}
