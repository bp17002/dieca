package パッケージ0.StaffDevice;
import java.util.Scanner;


public class StaffDeviceControl {

	public static void main(String[] args) {

		StaffDeviceGUI sdg = new StaffDeviceGUI();
		ICCardReader icr = new ICCardReader();
		DatabaseControl dbc = new DatabaseControl();
		Scanner scan = new Scanner(System.in);

		int command;
		String correctiondata;
		String id = null;
		boolean hasID;

		while(true) {
			while(id == null) {
				System.out.println("ICカードをタッチしてください");
				id = icr.readIC();
				if(id == null) {
					sdg.displayReadingError();
				}
			}


			hasID = dbc.searchID(id);

			if(hasID = false)sdg.displayRegistrationError();
			else {
				System.out.println("以下から捜査を選択してください．");
				System.out.println("1:エラーを解消");
				System.out.println("2:IDを削除");

				command = scan.nextInt();

				if(command == 1) {
					sdg.displayStateError();
					correctiondata = sdg.requestCorrectingData();
					dbc.correctRidingStat();
					System.out.println("ICカードのデータを書き換えました．\n");
				}
				else if(command == 2) {
					dbc.deleteID();
					sdg.displayDeleteID();
				}
				else System.out.println("最初から操作してください\n");
			}
		}
	}

}