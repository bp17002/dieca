package dieca;

import dieca.*;

public class ICCardReader {
	public String getId(){
		GetReadId gr = new GetReadID();
		return gr.getReadID().toString();
	}
}