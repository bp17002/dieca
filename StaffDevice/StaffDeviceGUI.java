package パッケージ0.StaffDevice;

import パッケージ0.String;

import java.util.Scanner;

public class StaffDeviceGUI {
	Scanner scan = new Scanner(System.in);

	public void displayReadingError() {
		System.out.println("ICカードの読み込みに失敗しました．");
	}
	public void displayRegistrationError() {
		System.out.println("ICカードが登録されていません．");
	}
	public void displayStateError() {
		System.out.println("ICカードの乗降データが不正です．");
	}
	public String requestCorrectingData() {
		System.out.println("直前の乗降駅を入力してください．");
		String correctingdata = scan.next();
		return correctingdata;
	}
	public void displayDeleteID() {
		System.out.println("IDを削除しました．\n1");
	}
}

}
