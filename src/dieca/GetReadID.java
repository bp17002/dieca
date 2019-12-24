package dieca;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;

class GetReadID {
  public void/* byte[] */ getReadID() {

    String line;
    String exefilename = ".\\PCSCReader\\Release\\PCSCReader.exe";
    try {
      Runtime run = Runtime.getRuntime();
      Process proc = run.exec(exefilename);
      InputStream is = proc.getInputStream();
      BufferedReader br = new BufferedReader(new InputStreamReader(is));

      int index;
      String id = "";

      while ((line = br.readLine()) != null) {
        // System.out.println("line: " + line);
        if ((index = line.indexOf("[readIDm]:")) >= 0) {
          index += "[readIDm]:".length();
          id = line.substring(index);
        }
      }
      proc.waitFor();
      System.out.println("id:" + id);

    } catch (Exception e) {
    }
  }
}
