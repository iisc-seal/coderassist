package feedback.utils;

import java.text.NumberFormat;
import java.text.ParsePosition;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Random;
import java.util.Set;

import feedback.inputs.InputVar;
import feedback.inputs.ProgramInputs;

public class Expression {
	
	// Given an operator, what should the types of the operand be?
	public static final Map<String, String> typeMap;
    static {
        Map<String, String> aMap = new HashMap<String, String>();
        aMap.put("+", "Int");
        aMap.put("-", "Int");
        aMap.put("*", "Int");
        aMap.put("/", "Int");
        aMap.put("%", "Int");
        aMap.put("^", "Int");
        aMap.put("<", "Int");
        aMap.put("<=", "Int");
        aMap.put(">", "Int");
        aMap.put(">=", "Int");
        aMap.put("...", "Int");
        aMap.put("||", "Bool");
        aMap.put("&&", "Bool");
        aMap.put("!", "Bool");
        typeMap = Collections.unmodifiableMap(aMap);
    }
	
    // Given an operator, what should the type of the result be?
 	public static final Map<String, String> operatorResultType;
     static {
         Map<String, String> aMap = new HashMap<String, String>();
         aMap.put("+", "Int");
         aMap.put("-", "Int");
         aMap.put("*", "Int");
         aMap.put("/", "Int");
         aMap.put("%", "Int");
         aMap.put("^", "Int");
         aMap.put("...", "Int");
         aMap.put("<", "Bool");
         aMap.put("<=", "Bool");
         aMap.put(">", "Bool");
         aMap.put(">=", "Bool");
         aMap.put("||", "Bool");
         aMap.put("&&", "Bool");
         aMap.put("!", "Bool");
         aMap.put("!=", "Bool");
         aMap.put("==", "Bool");
         operatorResultType = Collections.unmodifiableMap(aMap);
     }
    
    public static final Map<String, String> operatorMap;
    static {
        Map<String, String> aMap = new HashMap<String, String>();
        aMap.put("||", "or");
        aMap.put("&&", "and");
        aMap.put("*", "*");
        aMap.put("/", "div");
        aMap.put("%", "mod");
        aMap.put("^", "^");
        aMap.put("<", "<");
        aMap.put("<=", "<=");
        aMap.put(">", ">");
        aMap.put(">=", ">=");
        aMap.put("==", "=");
        aMap.put("=", "=");
        aMap.put("+", "+");
        aMap.put("-", "-");
        aMap.put("", "");
        aMap.put("||", "or");
        aMap.put("&&", "and");
        aMap.put("!", "not");
        aMap.put("=>", "=>");
        operatorMap = Collections.unmodifiableMap(aMap);
    }
    
    public static final Map<Expression, Expression> rewriteMap;
    static {
        Map<Expression, Expression> rMap = new HashMap<Expression, Expression>();
        Expression i = Expression.getID("i");
        Expression j = Expression.getID("j");
        	
        /*!i && j -> i==0 && j!=0*/
        Expression e1 = Expression.getAnd(Expression.negate(i), j);
        Expression e2 = Expression.getAnd(Expression.getIsZero(i), Expression.getIsNotZero(j));
        rMap.put((Expression)e1.clone(), (Expression)e2.clone());
        
        /*i && !j -> i!=0 && j==0*/
        e1 = Expression.getAnd(i, Expression.negate(j));
        e2 = Expression.getAnd(Expression.getIsNotZero(i), Expression.getIsZero(j));
        rMap.put((Expression)e1.clone(), (Expression)e2.clone());
        
        /*i && j -> i!=0 && j!=0*/
        e1 = Expression.getAnd(i, j);
        e2 = Expression.getAnd(Expression.getIsNotZero(i), Expression.getIsNotZero(j));
        rMap.put((Expression)e1.clone(), (Expression)e2.clone());
        
        /*(i+j) && i -> (i+j)!=0 && i!=0 */
        Expression e0 = Expression.getPlus(i, j);
        e1 = Expression.getAnd(e0, i);
        e2 = Expression.getAnd(Expression.getIsNotZero(e0), Expression.getIsNotZero(i));
        rMap.put((Expression)e1.clone(), (Expression)e2.clone());
        
        /*(i+j) && i -> (i+j)!=0 && j!=0 */
        e1 = Expression.getAnd(e0, j);
        e2 = Expression.getAnd(Expression.getIsNotZero(e0), Expression.getIsNotZero(j));
        rMap.put((Expression)e1.clone(), (Expression)e2.clone());
        
        Expression one = new Expression(ExprType.CONST, "", "1");
        
        /*!(i-1) && j-1 -> i-1==0 && j-1!=0*/
        e1 = Expression.getAnd(Expression.negate(Expression.getMinus(i, one)), Expression.getMinus(j, one));
        e2 = Expression.getAnd(Expression.getIsZero(Expression.getMinus(i, one)), Expression.getIsNotZero(Expression.getMinus(j, one)));
        rMap.put((Expression)e1.clone(), (Expression)e2.clone());
        
        /*(i-1) && !(j-1) -> i-1!=0 && j-1==0*/
        e1 = Expression.getAnd(Expression.getMinus(i, one), Expression.negate(Expression.getMinus(j, one)));
        e2 = Expression.getAnd(Expression.getIsNotZero(Expression.getMinus(i, one)), Expression.getIsZero(Expression.getMinus(j, one)));
        rMap.put((Expression)e1.clone(), (Expression)e2.clone());
        
        rewriteMap = Collections.unmodifiableMap(rMap);
    }
    
	public enum ExprType {
	    ID, CONST, UNARY, BINARY, TERNARY, FN, ARRAY 
	}
	
	ExprType EType;	
	String operator; // the operator name, set only if the root node here is a unary/binary op
	List<Expression> eList;	// the subexpressions that constitute this expression, left to right
	String name; 	// this can be an identifier name, or a constant, or a fn name
	String type;	// if the expression involves a type cast, record it
	
	/*Constructor for unary operators*/
	public Expression(ExprType eType, String operator, Expression e,
			String name) {
		super();
		EType = eType;
		this.operator = operator;
		this.eList = new ArrayList<Expression>();
		this.eList.add(e);
		this.name = name;
	}
	
	/*Constructor for unary expressions involving typecasts*/
	public Expression(ExprType eType, String operator, Expression exp,
			String name, String type) {
		EType = eType;
		this.operator = operator;
		this.eList = new ArrayList<Expression>();
		this.eList.add(exp);
		this.name = name;
		this.type = type;
	}
	
	/*Constructor for binary operators*/
	public Expression(ExprType eType, String operator, Expression e1, Expression e2,
			String name) {
		super();
		EType = eType;
		this.operator = operator;
		this.eList = new ArrayList<Expression>();
		this.eList.add(e1);
		this.eList.add(e2);
		this.name = name;
	}
	
	/*Constructor for ternary operators*/
	public Expression(ExprType eType, String operator, Expression e1, Expression e2, Expression e3,
			String name) {
		super();
		EType = eType;
		this.operator = operator;
		this.eList = new ArrayList<Expression>();
		this.eList.add(e1);
		this.eList.add(e2);
		this.eList.add(e3);
		this.name = name;
	}
	
	/*Constructor for constants and identifiers*/
	public Expression(ExprType eType, String operator, String name) {
		super();
		EType = eType;
		this.operator = operator;
		this.eList = null;
		this.name = name;
	}
	
	/*Constructor for functions and array accesses*/
	public Expression(ExprType eType, List<Expression> eList,
			String name) {
		super();
		EType = eType;
		this.eList = (eList != null)? eList : new ArrayList<Expression>();
		this.name = name;
	}
	

	public ExprType getEType() {
		return EType;
	}

	public void setEType(ExprType eType) {
		EType = eType;
	}

	public String getOperator() {
		return operator;
	}

	public void setOperator(String operator) {
		this.operator = operator;
	}

	public String getName() {
		return name;
	}

	public void setName(String name) {
		this.name = name;
	}

	public List<Expression> geteList() {
		return eList;
	}

	public void seteList(List<Expression> eList) {
		this.eList = eList;
	}

	@Override
	public String toString() {
		return this.printVisitor();
	}
	
	/*Helper methods to rewrite assignment statements*/
	
	/*Create range expressions from variables*/
	public static Expression CreateRangeExpression(String lower, String upper){
		Expression lb = new Expression(ExprType.CONST, "", lower);
		Expression ub = new Expression(ExprType.CONST, "", upper);
		return new Expression(ExprType.BINARY, "...", lb, ub, "");
	}
	
	/*Create range expressions from lb and ub expressions*/
	public static Expression CreateRangeExpression(Expression lower, Expression upper){
		/*if lower and upper are the same, scrunch*/
		if(lower.equalsVisitor(upper))
			return (Expression) lower.clone();
		return new Expression(ExprType.BINARY, "...", lower, upper, "");
	}
	
	/*Create a 2D array using a base name, and index expressions*/
	public static Expression Create2DArray(String name, Expression dim1, Expression dim2){
		List<Expression> dimList = new ArrayList<Expression>();
		dimList.add(dim1);
		dimList.add(dim2);
		return new Expression(ExprType.ARRAY, dimList, name);
	}
	
	public static Expression Create1DArray(String name, Expression dim){
		List<Expression> dimList = new ArrayList<Expression>();
		dimList.add(dim);
		return new Expression(ExprType.ARRAY, dimList, name);
	}
	
	public boolean isNumeric()
	{
	 if(this.EType != ExprType.CONST){
//		 if(eList == null)
//			 return false;
//		 for(Expression e: this.eList)
//			 if(e.EType != ExprType.CONST)
//				 return false;
//		 if(this.EType != ExprType.UNARY)
			 return false;
	  }	
	  NumberFormat formatter = NumberFormat.getInstance();
	  ParsePosition pos = new ParsePosition(0);
	  formatter.parse(name, pos);
	  return name.length() == pos.getIndex();
	}
	
	public boolean isDouble(String str) {
        try {
            Double.parseDouble(str);
            return true;
        } catch (NumberFormatException e) {
            return false;
        }
    }
	
	public static boolean isInteger(String s) {
	    try { 
	        Integer.parseInt(s); 
	    } catch(NumberFormatException e) { 
	        return false; 
	    }
	    // only got here if we didn't return false
	    return true;
	}
	
	
	/*return the identifiers used in an expression as a list of strings*/
	public List<String> identifierVisitor(){
		List<String> l = new ArrayList<String>();
		switch (EType) {
        case UNARY:
            return eList.get(0).identifierVisitor();
                
        case BINARY:
        	l = eList.get(0).identifierVisitor();
        	l.addAll(eList.get(1).identifierVisitor());
        	return l;
    
                     
        case TERNARY:
        	l = eList.get(0).identifierVisitor();
        	l.addAll(eList.get(1).identifierVisitor());
        	l.addAll(eList.get(2).identifierVisitor());
        
        case FN:
        	for(Expression e : eList){
            	l.addAll(e.identifierVisitor());
            }
            return l; 
        
        case ARRAY:    
        	l.add(name);
            for(Expression e : eList){
            	l.addAll(e.identifierVisitor());
            }
            return l;
            
        case CONST:
            return l;
            
        default:
        	//if(!isNumeric(name))
        	l.add(name);
            return l;
		}
	}
	
	/*store the Array Expressions used in 'this' into 
	 *a set of Expressions*/
	public Set<Expression> arrayExpressionVisitor(){
		Set<Expression> vars = new HashSet<Expression>();
		switch (EType) {
     
		case UNARY:
            vars.addAll(eList.get(0).arrayExpressionVisitor());
            return vars;
                
        case BINARY:
        	vars.addAll(eList.get(0).arrayExpressionVisitor());
        	vars.addAll(eList.get(1).arrayExpressionVisitor());
        	return vars;
    
                     
        case TERNARY:
        	vars.addAll(eList.get(0).arrayExpressionVisitor());
        	vars.addAll(eList.get(1).arrayExpressionVisitor());
        	vars.addAll(eList.get(2).arrayExpressionVisitor());
        	return vars;
        
        case FN:
        	for(Expression e : eList){
        			vars.addAll(e.arrayExpressionVisitor());
        		}
        	return vars;
        	
        case ARRAY:
        	// here is where this method differs from variableVisitor
        	// we don't recurse into array expressions...
        	vars.add(this);
            return vars;
        
        case CONST:
            return vars;
        	
        default:
//        	vars.add(this);  
            return vars;
		}
	}
	
	/*store the variables used in 'this' into 
	 *a set of Expressions*/
	public Set<Expression> variableVisitor(){
		Set<Expression> vars = new HashSet<Expression>();
		switch (EType) {
     
		case UNARY:
            vars.addAll(eList.get(0).variableVisitor());
            return vars;
                
        case BINARY:
        	vars.addAll(eList.get(0).variableVisitor());
        	vars.addAll(eList.get(1).variableVisitor());
        	return vars;
    
                     
        case TERNARY:
        	vars.addAll(eList.get(0).variableVisitor());
        	vars.addAll(eList.get(1).variableVisitor());
        	vars.addAll(eList.get(2).variableVisitor());
        	return vars;
        
        case ARRAY:
        	for(Expression e : eList){
    			vars.addAll(e.variableVisitor());
    		}
        	vars.add(new Expression(ExprType.ID, "", getName()));
        	return vars;
        	
        case FN:
        	for(Expression e : eList){
        			vars.addAll(e.variableVisitor());
        		}
        	return vars;
        	
        case CONST:
            return vars;
        	
        default:
        	vars.add(this); 
            return vars;
		}
	}
	
	public void renameIdentifiers(Map<String, String> bindings){
		switch (EType) {
		case UNARY:
            eList.get(0).renameIdentifiers(bindings);
            return;
                
        case BINARY:
        	eList.get(0).renameIdentifiers(bindings);
        	eList.get(1).renameIdentifiers(bindings);
        	return;
    
                     
        case TERNARY:
        	eList.get(0).renameIdentifiers(bindings);
        	eList.get(1).renameIdentifiers(bindings);
        	eList.get(2).renameIdentifiers(bindings);
        	return;
        
        case FN:
        	for(Expression e : eList){
        		e.renameIdentifiers(bindings);
        	}
        	return;
        	
        case ARRAY:
        	// This variable does not need renaming
        	if(bindings.containsKey(this.getName()))
        			this.setName(bindings.get(this.getName()));
        	for(Expression e : eList){
        		e.renameIdentifiers(bindings);
        	}
            return;
        
        case CONST:
        	return;
            
        default:
        	
        	if(!bindings.containsKey(this.getName()))
        		return;
        	this.setName(bindings.get(this.getName()));	
        	return;
		}
	}
	
	public int getDimension(){
		if(EType == ExprType.ARRAY)
			return eList.size();
		else
			return 0;
	}
	
	/*replace identifiers and Array 
	 *Expressions used in 'this' into what the map says*/
	public void renameVisitor(Map<Expression, Expression> bindings){
		switch (EType) {
		case UNARY:
            eList.get(0).renameVisitor(bindings);
            return;
                
        case BINARY:
        	eList.get(0).renameVisitor(bindings);
        	eList.get(1).renameVisitor(bindings);
        	return;
    
                     
        case TERNARY:
        	eList.get(0).renameVisitor(bindings);
        	eList.get(1).renameVisitor(bindings);
        	eList.get(2).renameVisitor(bindings);
        	return;
        
        case FN:
        	for(Expression e : eList){
        		e.renameVisitor(bindings);
        	}
        	return;
        	
        case ARRAY:
        	if(bindings.containsKey(this))
        		this.setName(bindings.get(this).getName());
        	this.setEType(ExprType.ID);
        	this.setOperator("");
        	this.seteList(null);
            return;
        case CONST:
        	return;
        	
        default:
//        	if(!this.isNumeric() && !("true".equals(name) || "false".equals(name))){
        	if(bindings.containsKey(this)){
        		this.setName(bindings.get(this).getName());
        		//this.setEType(ExprType.ID);
        		//this.setOperator("");
        	}	
        	return;
		}
	}
	
	public void getTypeConstraints(Map<Expression, Expression> equalityConstr){
		switch (EType) {
		case UNARY:
            eList.get(0).getTypeConstraints(equalityConstr);
            return;
                
        case BINARY:
			if(operator.compareTo("!=") == 0 || operator.compareTo("==") == 0){
				equalityConstr.put(eList.get(0), eList.get(1));
			}
			
        	eList.get(0).getTypeConstraints(equalityConstr);
        	eList.get(1).getTypeConstraints(equalityConstr);
        	return;
    
                     
        case TERNARY:
        	eList.get(0).getTypeConstraints(equalityConstr);
        	equalityConstr.put(eList.get(1), eList.get(2));
        	eList.get(1).getTypeConstraints(equalityConstr);
        	eList.get(2).getTypeConstraints(equalityConstr);
        	return;
        
        // We assume that we only have functions with all integer arguments
        case FN:
        	
        	
        case ARRAY:
        	for(Expression e : eList){
        		e.getTypeConstraints(equalityConstr);
        	}
            return;
            
        default:
        	return;
		}
	}
	
	public void typeVisitor(Map<Expression, String> types, String currtype){
		typeVisitorCore(types, currtype);
		Map<Expression, Expression> equalityConstr = 
				new HashMap<Expression, Expression>();
		getTypeConstraints(equalityConstr);
		for (Map.Entry<Expression,Expression> entry : equalityConstr.entrySet()) {
			  Expression key = entry.getKey();
			  Expression value = entry.getValue();
			  if(key.EType == ExprType.ID && (!types.containsKey(key) || types.get(key)=="")&& types.containsKey(value))
				  types.put(key, types.get(value));
			  if(value.EType == ExprType.ID && (!types.containsKey(value) || types.get(value)=="") && types.containsKey(key))
				  types.put(value, types.get(key));
		}
	}
	
	public void typeVisitorCore(Map<Expression, String> types, String currtype){
		String cType = "";
		switch (EType) {
		case UNARY:
			if(typeMap.containsKey(operator))
				cType = typeMap.get(operator);
			else 
				cType = currtype;
            eList.get(0).typeVisitorCore(types, cType);
            return;
                
        case BINARY:
			if(typeMap.containsKey(operator))
				cType = typeMap.get(operator);
			
			/*ugly special case handling cause I haven't implemented a bottom up pass*/
			else if(operator.compareTo("!=") == 0 || operator.compareTo("==") == 0){
				if(eList.get(0).isNumeric()){
					eList.get(1).typeVisitorCore(types, "Int");
					return;
				}
				if(eList.get(1).isNumeric()){
					eList.get(0).typeVisitorCore(types, "Int");
					return;
				}
				if("true".compareTo(eList.get(0).getName())==0 || "false".compareTo(eList.get(0).getName())==0){
					eList.get(1).typeVisitorCore(types, "Bool");
					return;
				}
				if("true".compareTo(eList.get(1).getName())==0 || "false".compareTo(eList.get(1).getName())==0){
					eList.get(0).typeVisitorCore(types, "Bool");
					return;
				}

	        	eList.get(0).typeVisitorCore(types, "Int");
	        	eList.get(1).typeVisitorCore(types, "Int");
	        	return;
			}
			
        	eList.get(0).typeVisitorCore(types, cType);
        	eList.get(1).typeVisitorCore(types, cType);
        	return;
    
                     
        case TERNARY:
        	eList.get(0).typeVisitorCore(types, "Bool");
        	eList.get(1).typeVisitorCore(types, "");
        	eList.get(2).typeVisitorCore(types, "");
        	return;
        
        // We assume that we only have functions with all integer arguments
        case FN:
        	for(Expression e : eList){
        		e.typeVisitorCore(types, "Int");
        	}
        	return;
        	
        case ARRAY:
        	for(Expression e : eList){
        		e.typeVisitorCore(types, "Int");
        	}
            return;
        
        case CONST:    
        	if(!this.isNumeric() && !("true".equals(name) || "false".equals(name))){
        		//Has to be a string const, which we rep as its int hashcode for SAT solving
        		if(!types.containsKey(this) || (types.get(this) == ""))
        			types.put(this, "Int");
        	}
        	
        default:
    		if(!types.containsKey(this) || (types.get(this) == ""))
    			types.put(this, currtype);
        	return;
		}
	}
	
	//get the top-level type of an expression
	public String getType(Map<Expression, String> types){
		if(this.isNumeric())
			return "Int";
		
		if(this.EType == ExprType.CONST){
			if("true".compareTo(name) == 0 || "false".compareTo(name) == 0)
				return "Bool";
		}
		
		if(this.EType == ExprType.ARRAY){
			return "Int";
		}
		
		if(this.EType == ExprType.UNARY){
			return typeMap.get(operator)==null? "Int": typeMap.get(operator);
		}
		
		if(this.EType == ExprType.BINARY){
			if(operator.compareTo("!=") == 0 || operator.compareTo("==") == 0)
				return "Bool";
			return typeMap.get(operator);
		}
		
		if(this.EType == ExprType.TERNARY){
			if(types.containsKey(eList.get(1)))
				return types.get(eList.get(1));
			if(types.containsKey(eList.get(2)))
				return types.get(eList.get(2));
			return "";
		}
		
		if(this.EType == ExprType.FN){
			if(name.compareTo("max") == 0 || name.compareTo("min") == 0)
				return "Int";
		}
		
		return "";
	}
	
	/*Retrofitted code: for use by output summary only!*/
	public String computeTypeBottomUp(){
		if(this.isNumeric()){
			if(isInteger(this.name))
				return "int";
			if(isDouble(this.name))
				return "double";
		}
		
		if(this.EType == ExprType.CONST){
			if("true".compareTo(name) == 0 || "false".compareTo(name) == 0)
				return "Bool";
			return "String";
		}
		
		if(this.EType == ExprType.ARRAY){
			return "int";
		}
		
		if(this.EType == ExprType.UNARY){
			if(type != null)
				return type;
			
			return promote(eList.get(0).computeTypeBottomUp(), operator);
		}
		
		if(this.EType == ExprType.BINARY){
			if(operator.compareTo("!=") == 0 || operator.compareTo("==") == 0)
				return "Bool";
			return promote(eList.get(0).computeTypeBottomUp(), 
								eList.get(1).computeTypeBottomUp(), operator);
		}
		
		if(this.EType == ExprType.TERNARY){
			return promote(eList.get(1).computeTypeBottomUp(), 
								eList.get(2).computeTypeBottomUp(), operator);
		}
		
		if(this.EType == ExprType.FN){
			if(name.compareTo("max") == 0 || name.compareTo("min") == 0)
				return "Int";
		}
		
		return "";
	}
	
	private String promote(String type, String operator) {
		if(type != null)
			return type;
		return typeMap.get(operator);
	}

	private String promote(String t1, String t2, String operator) {
		if(t1 != null){
			if(t2 != null){
				if("double".compareTo(t1)==0 || "float".compareTo(t1)==0 
				   || "double".compareTo(t2)==0 || "float".compareTo(t2)==0
				   || "long double".compareTo(t1) == 0 || "long double".compareTo(t2) == 0){
					return "double";
				}
				else
					return typeMap.get(operator);
			}
			else
				return t1;
		}
		else{
			if(t2 != null)
				return t2;
		}
		return typeMap.get(operator);
	}

	public boolean equalsVisitor(Expression other){
		if(other == null)
			return false;
		
		List<Expression> l = other.geteList();
		
		if(other.getEType() != EType)
			return false;
		
		switch (EType) {
        case UNARY:
            return (operator.compareTo(other.getOperator()) == 0) && 
            		eList.get(0).equalsVisitor(l.get(0));
                
        case BINARY:
        	return eList.get(0).equalsVisitor(l.get(0)) &&
        			operator.compareTo(other.getOperator()) == 0
        			&& eList.get(1).equalsVisitor(l.get(1));
    
                     
        case TERNARY:
        	return eList.get(0).equalsVisitor(l.get(0)) &&
        			eList.get(1).equalsVisitor(l.get(1)) &&
        			eList.get(2).equalsVisitor(l.get(2));
        
        case FN:
        	boolean eq = true;
            if(name.compareTo(other.getName()) != 0)
            	return false;
            int i = 0;
            for(Expression e : eList){
            	eq = eq && e.equalsVisitor(l.get(i));
            	i++;
            	if(eq == false)
            		return false;
            }
            return true;
        
        case ARRAY:    
        	boolean equals = true;
            if(name.compareTo(other.getName()) != 0)
            	return false;
            int index = 0;
            for(Expression e : eList){
            	equals = equals && e.equalsVisitor(l.get(index));
            	index++;
            	if(equals == false)
            		return false;
            }
            return true;
            
        case CONST:
        	if (name.compareTo(other.getName()) == 0)
        		return true;
        	if (name.compareTo("true") == 0 && other.getName().compareTo("1") == 0)
        		return true;
        	if (name.compareTo("1") == 0 && other.getName().compareTo("true") == 0)
        		return true;
        	if (name.compareTo("false") == 0 && other.getName().compareTo("0") == 0)
        		return true;
        	if (name.compareTo("0") == 0 && other.getName().compareTo("false") == 0)
        		return true;
        	return false;

        default:
            return (name.compareTo(other.getName()) == 0);
		}	
	}
	
	@Override
	public int hashCode() {
		final int prime = 31;
		int result = 1;
		result = prime * result + ((EType == null) ? 0 : EType.hashCode());
		result = prime * result + ((eList == null) ? 0 : eList.hashCode());
		result = prime * result + ((name == null) ? 0 : name.hashCode());
		result = prime * result
				+ ((operator == null) ? 0 : operator.hashCode());
		return result;
	}

	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		Expression other = (Expression) obj;
		if (EType != other.EType)
			return false;
		if (eList == null) {
			if (other.eList != null)
				return false;
		} else if (!eList.equals(other.eList))
			return false;
		if (name == null) {
			if (other.name != null)
				return false;
		} else if (!name.equals(other.name))
			return false;
		if (operator == null) {
			if (other.operator != null)
				return false;
		} else if (!operator.equals(other.operator))
			return false;
		return true;
	}

	public String printVisitor(){
		String s;
		switch (EType) {
        case UNARY:
//        	if(type != null)
//        		return "(" + type + ")" + operator + "(" + eList.get(0).printVisitor() + ")";
        	if (eList.get(0).EType == ExprType.CONST)
        		return operator + eList.get(0).printVisitor();
        	else
        		return operator + "(" + eList.get(0).printVisitor() + ")";
                
        case BINARY:
        	if (operator.equals("..."))
        		return eList.get(0).printVisitor() + ' ' + operator + ' ' + eList.get(1).printVisitor();
        	else
        		return "(" + eList.get(0).printVisitor() + ' ' + operator + ' ' + eList.get(1).printVisitor() + ")";
    
                     
        case TERNARY:
        	return "(" + eList.get(0).printVisitor() +
        			 operator + ' ' +	
            eList.get(1).printVisitor() + ":" +
            eList.get(2).printVisitor() + ")" ;
        
        case FN:
            s = name + '(';
            StringBuilder sb = new StringBuilder(s);
            String prefix = "";
            for(Expression e : eList){
            	sb.append(prefix);
            	prefix = ", ";
            	sb.append(e.printVisitor());
            }
            sb.append(')');
            return sb.toString();
        
        case ARRAY:    
        	StringBuilder arStr = new StringBuilder(name);
            for(Expression e : eList){
            	arStr.append("[");
            	arStr.append(e.printVisitor());
            	arStr.append("]");
            }
            return arStr.toString();
            
        default:
            return name;
		}	
	}

	public String SMTVisitor(){
		switch (EType) {
        case UNARY:
        	// if this unary expr came from a cast, operator is empty
        	if("".compareTo(operator)==0)
        		return eList.get(0).SMTVisitor();
            return "(" + operatorMap.get(operator) + " " + eList.get(0).SMTVisitor() + ")";
                
        case BINARY:
        	if(operator.compareTo("!=") == 0)
        		return "(not (= " +  eList.get(0).SMTVisitor()+ " " + eList.get(1).SMTVisitor() + "))";
        	return "(" + operatorMap.get(operator) + " " + 
        		eList.get(0).SMTVisitor()+ " " + eList.get(1).SMTVisitor() + ")";
    
                     
        case TERNARY:
        	return "(ite " +  eList.get(0).SMTVisitor() + " " + eList.get(1).SMTVisitor() + " " +
            		eList.get(2).SMTVisitor() +
            		")";
        
        case FN:
        	if("symex_max".compareTo(name) == 0)
        		name = "max";
        	if(name.compareTo("max")!=0)
        		name = "max";
//            assert(name.compareTo("max") == 0 || name.compareTo("min") == 0) : name;
            String pre = "(" + name;
            for(Expression e: eList){
            	pre += (" " + e.SMTVisitor());
            }
            pre += ')';
            return pre;
        
        case ARRAY:    
        	Integer I = new Integer(this.hashCode());
            return I.toString();
            
        case CONST:
        	if(this.isNumeric()){
        		if(this.isDouble(name)){
        			Double d = Double.parseDouble(name);
        			Integer i = d.intValue();
        			return i.toString();
        		}
        	}
        	if("String".compareTo(this.computeTypeBottomUp()) == 0){
        		Integer rep = name.hashCode(); 
        		return rep.toString();
        	}
            return name;
		
		default:
			return name;
		}
	}

	
	@Override
	public Object clone() {
		Expression e1, e2, e3;
		switch (EType) {
        case UNARY:
            return new Expression(ExprType.UNARY, operator, (Expression) eList.get(0).clone(), name); 
                
        case BINARY:
        	e1 = (Expression) eList.get(0).clone();
        	e2 = (Expression) eList.get(1).clone();
        	return new Expression(ExprType.BINARY, operator, e1, e2, name);
                        
        case TERNARY:
        	e1 = (Expression) eList.get(0).clone();
        	e2 = (Expression) eList.get(1).clone();
        	e3 = (Expression) eList.get(2).clone();
        	return new Expression(ExprType.TERNARY, operator, e1, e2, e3, name);
        
        case FN:
        	List<Expression> el = new ArrayList<Expression>();
            for(Expression e: eList){
            	el.add((Expression) e.clone());
            }
            return new Expression(ExprType.FN, el, name);
            
        case ARRAY:    
        	el = new ArrayList<Expression>();
            for(Expression e: eList){
            	el.add((Expression) e.clone());
            }
            return new Expression(ExprType.ARRAY, el, name);
            
        case CONST:
        	return new Expression(ExprType.CONST, operator, name);
        	
        default:
        	return new Expression(ExprType.ID, operator, name);	
		}
	}
	
	public static Expression negate(Expression e){
		Expression exp = new Expression(ExprType.UNARY, "!", (Expression) e.clone(), "");
		return exp;
	}
	
	public static Expression conjunct(Expression e1, Expression e2){
		if(e1 == null && e2 == null)
			return Expression.getTrue();
		
		if(e1 == null)
			return e2;
		if(e2 == null)
			return e1;
		Expression exp = new Expression(ExprType.BINARY, "&&",
				(Expression)e1.clone(), (Expression) e2.clone(), "");
		return exp;
	}
	
	public static Expression addOne(Expression e){
		if(e.isNumeric()){
			Integer newVal = getNumericValue(e)+1;
			return new Expression(ExprType.CONST, "", newVal.toString());
		}
		
		// if the expression already has a addition/subtraction of a constant,
		// we have some room for constant folding
		if(e.EType == ExprType.BINARY){
			Expression operand2 = e.geteList().get(1);
			if(operand2.isNumeric()){
				String op = e.getOperator();
				Integer newVal;
				if(op.compareTo("+") == 0){
					newVal = getNumericValue(operand2)+1;
					Expression op2 = new Expression(ExprType.CONST, "", newVal.toString());
					return new Expression(ExprType.BINARY, op, e.geteList().get(0), op2, "");
				}
				else if(op.compareTo("-") == 0){
					newVal = getNumericValue(operand2)-1;
					Expression op2 = new Expression(ExprType.CONST, "", newVal.toString());
					return new Expression(ExprType.BINARY, op, e.geteList().get(0), op2, "");
				}
			}
		}
		
		Expression one = new Expression(ExprType.CONST, "", "1");
		return new Expression(ExprType.BINARY, "+", e, one, "");
	}
	
	public static Expression subtractOne(Expression e){
		if(e.isNumeric()){
			assert(e.EType == ExprType.CONST);
			Integer newVal = getNumericValue(e)-1;
			return new Expression(ExprType.CONST, "", newVal.toString());
		}
		// if the expression already has a addition/subtraction of a constant,
		// we have some room for constant folding
		if(e.EType == ExprType.BINARY){
			Expression operand2 = e.geteList().get(1);
			if(operand2.isNumeric()){
				String op = e.getOperator();
				Integer newVal;
				if(op.compareTo("+") == 0){
					newVal = getNumericValue(operand2)-1;
					Expression op2 = new Expression(ExprType.CONST, "", newVal.toString());
					return new Expression(ExprType.BINARY, op, e.geteList().get(0), op2, "");
				}
				else if(op.compareTo("-") == 0){
					newVal = getNumericValue(operand2)+1;
					Expression op2 = new Expression(ExprType.CONST, "", newVal.toString());
					return new Expression(ExprType.BINARY, op, e.geteList().get(0), op2, "");
				}
			}
		}
		Expression one = new Expression(ExprType.CONST, "", "1");
		return new Expression(ExprType.BINARY, "-", e, one, "");
	}
	
	public static Expression getMax(Expression e1, Expression e2){
		if(Expression.lessThanOrEquals(e1, e2))
			return e2;
		return e1;
	}
	
	public static Expression getMin(Expression e1, Expression e2){
		if(Expression.lessThanOrEquals(e1, e2))
			return e1;
		return e2;
	}
	
	public static Integer getNumericValue(Expression e){
		assert(e.EType == ExprType.CONST); 
		assert(e.isNumeric());
		return Integer.parseInt(e.getName());
	}
	
	public static boolean lessThan(Expression e1, Expression e2){
		if(e1.isNumeric() && e2.isNumeric())
			return getNumericValue(e1) < getNumericValue(e2);
		if(e1.equalsVisitor(e2))
			return false;
		return HelperFunctions.lessThan(e1, e2, ProgramInputs.getAsConstraint());	
	}
	
	public static boolean lessThanOrEquals(Expression e1, Expression e2){
		if(e1.isNumeric() && e2.isNumeric())
			return getNumericValue(e1) <= getNumericValue(e2);
		if(e1.equalsVisitor(e2))
			return true;
		return HelperFunctions.lessThanOrEquals(e1, e2, ProgramInputs.getAsConstraint());
	}

	// return the expression obtained by replacing occurrences of source 
	// in 'this' by target	
	public Expression replace(Expression source, Expression target) {
		
		if(this.equalsVisitor(source)){
			this.setName(target.getName());
			this.setEType(target.getEType());
			this.setOperator(target.getOperator());
			this.seteList(target.geteList());
			return target;
		}
		
		switch (EType) {
		case UNARY:
			eList.set(0, eList.get(0).replace(source, target));
            return this;
                
        case BINARY:
        	eList.set(0, eList.get(0).replace(source, target));
        	eList.set(1, eList.get(1).replace(source, target));
        	return this;
        	         
        case TERNARY:
        	eList.set(0, eList.get(0).replace(source, target));
        	eList.set(1, eList.get(1).replace(source, target));
        	eList.set(2, eList.get(2).replace(source, target));  	
        	return this;
        
        case FN:
        	List<Expression> newList = new ArrayList<>();
        	for(Expression e : eList){
        		newList.add(e.replace(source, target));
        	}
        	return new Expression(ExprType.FN, newList, name);
        	
        case ARRAY:
        	List<Expression> nList = new ArrayList<>();
        	for(Expression e : eList){
        		nList.add(e.replace(source, target));
        	}
        	return new Expression(ExprType.ARRAY, nList, name);
            
        default:
        	if(name.compareTo(source.getName())==0)
        		return target;
        	return this;
		}
	}
	
	public boolean contains(Expression source) {
		boolean found = false;
		if(this.equalsVisitor(source))
			return true;
		
		switch (EType) {
		case UNARY:
			return (eList.get(0).contains(source));
                            
        case BINARY:
        	return eList.get(0).contains(source) || eList.get(1).contains(source);
        	         
        case TERNARY:
        	return eList.get(0).contains(source) || 
        			eList.get(1).contains(source) || eList.get(2).contains(source);
        
        case FN:
        	
        	
        case ARRAY:
        	for(Expression e : eList){
        		found = found || e.contains(source);
        	}
        	return found;
            
        default:
        	return false;
		}
	}
	
	public static int getAstSize(Expression source) {
		int count = 0;
		switch (source.EType) {
		case UNARY:
			return 1 + getAstSize(source.eList.get(0));
                            
        case BINARY:
        	return 1 + getAstSize(source.eList.get(0)) + getAstSize(source.eList.get(1));
        	         
        case TERNARY:
        	return 1 + getAstSize(source.eList.get(0)) + getAstSize(source.eList.get(1)) + 
        			getAstSize(source.eList.get(2)); 
        			
        
        case FN:
        	        	
        case ARRAY:
        	
        	for(Expression e : source.eList){
        		count += getAstSize(e);
        	}
        	return count+1;
            
        default:
        	return 1;
		}
	}
	
	boolean hasOperator(String op){
		boolean found = false;
		if(operator!=null && op.compareTo(operator) == 0)
			return true;
		
		switch (EType) {
		case UNARY:
			return (eList.get(0).hasOperator(op));
                            
        case BINARY:
        	return eList.get(0).hasOperator(op) || eList.get(1).hasOperator(op);
        	         
        case TERNARY:
        	return eList.get(0).hasOperator(op) || 
        			eList.get(1).hasOperator(op) || eList.get(2).hasOperator(op);
        
        case FN:
        	
        	
        case ARRAY:
        	for(Expression e : eList){
        		found = found || e.hasOperator(op);
        	}
        	return found;
            
        default:
        	return false;
		}
	}
	
	boolean isEqualAfterRename(Expression exp, 
			Map<String, String> rename){
		Expression dup = (Expression) exp.clone();
		dup.renameIdentifiers(rename);
		return this.equalsVisitor(dup);
	}

	public static Expression disjunct(Expression e1, Expression e2) {
		if(e1 == null)
			return e2;
		Expression exp = new Expression(ExprType.BINARY, "||",
				(Expression)e1.clone(), (Expression) e2.clone(), "");
		return exp;
	}

	public static Expression getTrue() {
		return new Expression(ExprType.CONST, "", "true");
	}

	public static Expression getFalse() {
		return new Expression(ExprType.CONST, "", "false");
	}
	
	public static Expression getID(String s){
		return new Expression(ExprType.ID, "", s);
	}
	
	public static Expression getAnd(Expression e1, Expression e2){
		return new Expression(ExprType.BINARY, "&&", e1, e2, "");
	}
	
	public static Expression getZero() {
		return Expression.getConst("0");
	}
	
	public static Expression getIsZero(Expression e){
		return new Expression(ExprType.BINARY, "==", e, Expression.getZero(), "");
	}
	
	public static Expression getIsNotZero(Expression e){
		return new Expression(ExprType.BINARY, "!=", e, Expression.getZero(), "");
	}
	
	public static Expression getPlus(Expression e1, Expression e2){
		return new Expression(ExprType.BINARY, "+", e1, e2, "");
	}

	public static Expression getMinus(Expression e1, Expression e2){
		return new Expression(ExprType.BINARY, "-", e1, e2, "");
	}
	
	public void triggerRewrite() {
		for(Entry<Expression, Expression> entry : rewriteMap.entrySet()){
			this.replace(entry.getKey(), entry.getValue());
		}
	}

	public static Expression getConst(String str) {
		return new Expression(ExprType.CONST, "", str);
	}
	
	public Expression getAsInteger(){
		Expression condition = new Expression(ExprType.BINARY, "==", 
			this, Expression.getTrue(), "");
		return new Expression(ExprType.TERNARY, 
				"?", condition, Expression.getConst("1"), Expression.getConst("0"), "");
	}
	
	public Expression getAsBoolean(){
		return new Expression(ExprType.BINARY, "!=", 
				this, Expression.getZero(), "");
	}

	private void convertToInteger() {
		Expression condition = new Expression(ExprType.BINARY, "==", 
				(Expression)this.clone(), Expression.getTrue(), "");
		this.EType = ExprType.TERNARY;
		this.operator = "?";
		this.eList = new ArrayList<Expression>();
		eList.add(condition);
		eList.add(Expression.getConst("1"));
		eList.add(Expression.getConst("0"));
	}

	private void convertToBoolean() {
		Expression lhs = (Expression)this.clone();
		this.EType = ExprType.BINARY;
		this.operator = "!=";
		this.eList = new ArrayList<Expression>();
		eList.add(lhs);
		eList.add(Expression.getZero());
	}

	
	public void fixTypes(Map<String, String> types, String... suggested){
		/*
		 * At the root, call on children
		 * After the call, determine types of the children
		 * and compare them against the expected types
		 * If the types differ, trigger rewrites.
		 * Return 
		 * */
		String expectedType = null, currentType;
		switch (EType) {
		case UNARY:
			if(typeMap.containsKey(operator))
				expectedType = typeMap.get(operator);
			else 
				return;
			eList.get(0).fixTypes(types);
            currentType = eList.get(0).inferType(types);
            updateSubtree(expectedType, currentType, 0);
            return;
                
        case BINARY:
        	if(typeMap.containsKey(operator))
				expectedType = typeMap.get(operator);
			else 
				return;
        	eList.get(0).fixTypes(types);
        	eList.get(1).fixTypes(types);
			
			currentType = eList.get(0).inferType(types);
            updateSubtree(expectedType, currentType, 0);
            currentType = eList.get(1).inferType(types);
            updateSubtree(expectedType, currentType, 1);
        	return;
    
                     
        case TERNARY:
        	eList.get(0).fixTypes(types);
        	expectedType = "Bool";
        	currentType = eList.get(0).inferType(types);
            updateSubtree(expectedType, currentType, 0);
        	return;
        
        // We assume that we only have functions with all integer arguments
        // and arrays with all integer arguments	
        case FN:
        	        	
        case ARRAY:
        	expectedType = "Int";
        	int index = 0;
        	for(Expression e : eList){
        		e.fixTypes(types);
        		currentType = eList.get(index).inferType(types);
                updateSubtree(expectedType, currentType, index);
                index++;
        	}
        	return;
        
        
        case ID:
        	currentType = this.inferType(types);
        	if(suggested.length > 0)
        		expectedType = suggested[0];
        	else
        		expectedType = "Int";
        	if(!currentType.equals(expectedType)){
        		if(expectedType.compareTo("Bool") == 0)
    				this.convertToBoolean();
    			else
    				this.convertToInteger();
        	}
        	return;
        	
        default:
        	return;
		}
	}

	private void updateSubtree(String expectedType, String currentType, int index) {
		if(!currentType.equals(expectedType)){
			if(expectedType.compareTo("Bool") == 0){
				eList.set(index, eList.get(index).getAsBoolean());
			}
			else
				eList.set(index, eList.get(index).getAsInteger());
		}
	}

	public String inferType(Map<String, String> types) {
		
		if(EType == ExprType.CONST){
			if("true".equals(name) || "false".equals(name))
				return "Bool";
			
			return "Int";
		}
		
		if(EType == ExprType.ID || EType == ExprType.ARRAY || EType == ExprType.FN){
			if(types.containsKey(name))
				return types.get(name);
			return "Int";
		}
		
		if(this.operator == "")
			return "Int";
		
		if(this.operator == "?")
			return this.eList.get(1).inferType(types);
		
		return operatorResultType.get(this.operator);
	}

}
