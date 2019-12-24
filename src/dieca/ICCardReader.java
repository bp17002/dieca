package dieca;


public class ICCardReader {
	public String getId(){
		GetReadID gr = new GetReadID();
		return gr.getReadID().toString();
	}
}