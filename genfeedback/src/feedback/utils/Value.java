package feedback.utils;

public class Value {
	public enum valueType {
		INT,
		BOOL,
		CHAR
	}
	
	public enum ErrorVal {
		INVALIDCHARVALUE,
		INVALIDTYPE,
		SUCCESS
	}
	
	public valueType type;
	public String value;
	
	public int intValue;
	public boolean boolValue;
	public char charValue;
	
	public Value(valueType t, String v) {
		type = t;
		value = v;
		
		ErrorVal rv = populateValue();
		if (rv != ErrorVal.SUCCESS) {
			System.out.println("ERROR: Could not obtain value from string " + v);
		}
	}
	
	private ErrorVal populateValue() {
		switch(type) {
		case INT:
			intValue = Integer.parseInt(value);
			return ErrorVal.SUCCESS;
		case BOOL:
			boolValue = Boolean.parseBoolean(value);
			return ErrorVal.SUCCESS;
		case CHAR:
			if (value.length() > 1) 
				return ErrorVal.INVALIDCHARVALUE;
			charValue = value.charAt(0);
			return ErrorVal.SUCCESS;
		default:
			return ErrorVal.INVALIDTYPE;
		}
	}
}
