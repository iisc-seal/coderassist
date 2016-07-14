package feedback.utils;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

import org.antlr.runtime.ANTLRStringStream;
import org.antlr.runtime.CharStream;
import org.antlr.runtime.CommonTokenStream;
import org.antlr.runtime.RecognitionException;
import org.antlr.runtime.TokenStream;
import org.antlr.runtime.tree.CommonTreeNodeStream;

import feedback.logging.Logger;
import feedback.update.parser.ExprLexer;
import feedback.update.parser.ExprParser;
import feedback.update.parser.ExpressionTreeWalker;
import feedback.update.parser.ExprParser.startrule_return;

public class TypeSummary {
	static Map<String, String> constraintMap;
	static Map<String, List<String>> typeHeirarchy;
	String progName;
	String progInputSummaryName;
	Map<String, Variable> declaredVars;
	int score;
	
	static final Map<String, String> typeMap;
   
	enum VarType {
        BOOLVAR,
        CHARVAR,
        INTVAR,
        FLOATVAR,
        BOOLARR,
        CHARARR,
        INTARR,
        FLOATARR,
        VECTOR,
        UNKNOWNVAR
    };


	
	static {
        Map<String, String> aMap = new HashMap<String, String>();
        aMap.put("0", "bool");
        aMap.put("1", "char");
        aMap.put("2", "int");
        aMap.put("3", "float");
        aMap.put("4", "boolArray");
        aMap.put("5", "charArray");
        aMap.put("6", "intArray");
        aMap.put("7", "floatArray");
        aMap.put("8", "vector");
        aMap.put("9", "unknown");
        typeMap = Collections.unmodifiableMap(aMap);
    }

	static {
        Map<String, List<String>> aMap = new HashMap<String, List<String>>();
        aMap.put("bool", Arrays.asList("bool", "char", "int", "float"));
        aMap.put("char", Arrays.asList("char", "int", "float"));
        aMap.put("int", Arrays.asList("int", "float"));
        aMap.put("float", Arrays.asList("float"));
        aMap.put("boolArray", Arrays.asList("boolArray", "charArray", "intArray", "floatArray"));
        aMap.put("charArray", Arrays.asList("charArray", "intArray", "floatArray"));
        aMap.put("intArray", Arrays.asList("intArray", "floatArray"));
        aMap.put("floatArray", Arrays.asList("floatArray"));        
        typeHeirarchy = Collections.unmodifiableMap(aMap);
    }

	
	static {
        Map<String, String> aMap = new HashMap<String, String>();
        aMap.put("dp_ProgInput_2", "100");
        constraintMap = Collections.unmodifiableMap(aMap);
    }
	
	
	public TypeSummary(String progName, String progInputSummaryName) {
		this.progName = progName;
		this.progInputSummaryName = progInputSummaryName;
		declaredVars = new HashMap<String, Variable>();
		score = 0;
	}
	
	public Map<String, Variable> getDeclaredVars() {
		return declaredVars;
	}

	public void setDeclaredVars(Map<String, Variable> declaredVars) {
		this.declaredVars = declaredVars;
	}

	public void incScore(int i) {
		score += i;
	}
	
	public int getScore() {
		return score;
	}

	public void parse(){
		try (BufferedReader br = new BufferedReader(new FileReader(progInputSummaryName))) {
		    String line = br.readLine(); // skip first line
		    while ((line = br.readLine()) != null) {
		       String[] components = line.split(":");
		       String canonicalName = components[0];
		       String realName = components[1];
		       String type = typeMap.get(components[2]);
		       String dim1 = "", dim2 = "";
		       if(components.length > 3)
		    	   dim1 = components[3];//getValue(components[3], constraintMap);
		       if(components.length > 4)
		    	   dim2 = components[4];//getValue(components[4], constraintMap);
//		       LoopHeaderVariable outer = null, inner = null;
//		       if(dim1 != -1 && canonicalName.startsWith("dp_ProgInput")){
//		         String[] loop1Components = components[5].substring(1, components[5].length()-1).split(",");
//		         outer = new LoopHeaderVariable(loop1Components, null);
//		         if(dim2 != -1){
//		        	String[] loop2Components = components[6].substring(1, components[6].length()-1).split(",");
//		        	inner = new LoopHeaderVariable(loop2Components, outer);
//		         }
//		       }
		       Variable t = new Variable(canonicalName, realName, type, dim1, dim2, null, null);
		       declaredVars.put(canonicalName, t);
		    }
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (Exception e){
			System.out.println("Failed to parse" + progInputSummaryName);
		}
	}
	
//	public void getFeedback(TypeSummary other, Logger feedbackLogger){
//		Map<String, Variable> otherDeclVar = other.getDeclaredVars();
//		boolean matched = true;
//		for(Entry<String, Variable> entry : declaredVars.entrySet()){
//			String canonicalName = entry.getKey();
//			Variable v = entry.getValue();
//			Variable ov = otherDeclVar.get(canonicalName);
//			if(ov == null){
//				feedbackLogger.writeLog("Missing variable " + canonicalName);
//				continue;
//			}
//			matched = v.getFeedback(ov, feedbackLogger) && matched;
//			
//		}
//		if(matched)
//			System.out.println(other.progInputSummaryName + "[" + other.progName + 
//					"]" + " Matched " + progInputSummaryName);
//	}

	public static Integer getValue(String exp, Map<String, String> valuation){
		CharStream strm = new ANTLRStringStream(exp);
		ExprLexer lex = new ExprLexer(strm);
		TokenStream tokenStrm = new CommonTokenStream(lex);
		ExprParser pars = new ExprParser(tokenStrm);
		startrule_return expression = null;
		try {
			expression = pars.startrule();
		} catch (RecognitionException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		//System.out.println(expression.getTree().toStringTree());
		CommonTreeNodeStream nStream = new CommonTreeNodeStream(expression.getTree());
		ExpressionTreeWalker expWalker = new ExpressionTreeWalker(nStream);
		expWalker.variables.putAll(valuation);
		Integer value = 0;
		try {
			value = expWalker.eval();
		} catch (RecognitionException e) {
			System.out.println(exp);
			System.out.println(valuation);
			e.printStackTrace();
		} catch (ArithmeticException e) {
			System.out.println(e);
		}
		return value;
	}
	
}
