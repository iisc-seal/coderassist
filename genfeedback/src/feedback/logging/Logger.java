package feedback.logging;

import java.io.FileNotFoundException;
import java.io.PrintWriter;

public class Logger {
	
	public enum Modes {INPUT, INIT, UPDATE, OUTPUT};
	
	PrintWriter log;
	public StringBuilder sb;
	
	Modes mode = Modes.INPUT;
	
	public StringBuilder input = new StringBuilder();
	public StringBuilder init = new StringBuilder();
	public StringBuilder update = new StringBuilder();
	public StringBuilder output = new StringBuilder();
	
	public Logger(String fileName) {
		sb = new StringBuilder();
		try {
			log = new PrintWriter(fileName);
		} catch (FileNotFoundException e) {
			System.out.println("ERROR: Cannot find log file " + fileName);
			log = null;
		}
	}
	
	public void writeLog(String txt) {
		log.println(txt);
//		sb.append(txt+'\n');
		switch(mode){
			case INPUT:	input.append(txt + '\n');
				break;
			case INIT:	init.append(txt + '\n');
				break;
			case UPDATE: update.append(txt + '\n');
				break;
			case OUTPUT: output.append(txt + '\n');						
				break;
			default:					
		}
			
	}
	
	public void flush() {
		switch(mode){
			case INPUT:	input.setLength(52);
				break;
			case INIT:	init.setLength(52);
				break;
			case UPDATE: update.setLength(52);
				break;
			case OUTPUT: output.setLength(52);						
				break;
			default:					
		}
			
	}
	
	public void closeLog() {
		log.close();
	}

	public PrintWriter getLog(){
		return log;
	}

	
	public String getSBLog(){
		return sb.toString();
	}
	
	public void setMode(Modes mode){
		this.mode = mode;
	}
	
	public boolean isOutputMode() {
		return (this.mode == Modes.OUTPUT);
	}
}
