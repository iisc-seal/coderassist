package feedback.utils;

import java.util.HashMap;
import java.util.Map;

class Path{
	Expression pathCondition;
	Map<Expression, Expression> store;
	public Path(Expression pathCondition, Expression lhs, Expression rhs) {
		super();
		store = new HashMap<Expression, Expression>();
		this.pathCondition = pathCondition;
		this.lhs = lhs;
		this.rhs = rhs;
	}
	
	public Path(Expression pathCondition, Expression lhs, Expression rhs, 
			Map<Expression, Expression> store ) {
		super();
		store = new HashMap<Expression, Expression>();
		this.pathCondition = pathCondition;
		this.lhs = lhs;
		this.rhs = rhs;
		this.store = new HashMap<Expression, Expression>();
		this.store.putAll(store);
	}
	
	public void updateStore(Expression key, Expression value){
		key.setName(key.getName() + "Prime");
		store.put(key, value);
	}
	
	Expression lhs; 
	Expression rhs;
}