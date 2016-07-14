package feedback;
import java.sql.*;

public class AggregateResult{
	ResultStats type;
	ResultStats input;
	ResultStats init;
	ResultStats update;
	ResultStats output;
	
	@Override
	public String toString() {
		return "AggregateResult [type =" + type + "input=" + input + ", init=" + init
				+ ", update=" + update + ", output=" + output + "]";
	}

	public void writeAggregateResult(Connection connection, int subId, String ref) 
			throws SQLException{

		input.writeSelf(connection, subId, ref, MainDriver.databaseName + ".input_results");
		init.writeSelf(connection, subId, ref, MainDriver.databaseName + ".init_results");
		update.writeSelf(connection, subId, ref, MainDriver.databaseName + ".update_results");
		output.writeSelf(connection, subId, ref, MainDriver.databaseName + ".output_results");
}	
}
