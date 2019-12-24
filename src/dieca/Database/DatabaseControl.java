package パッケージ0.Database;

import パッケージ0.String;

public class DatabaseControl {

	public boolean searchID(String id) {
		return false;
	}

	public String judgeState(String id) {
		return null;
	}

	public int getBalance(String id) {
		return 0;
	}

	public int getMinMoney(String stationname) {
		return 0;
	}

	public void registarEnterStation(String stationname, String id) {

	}

	public int getFare(String startstation, String endstation) {
		return 0;
	}

	public void registarExitStation(String endstation, String id) {

	}

	public void updateBalance(int balance, String id, int point) {

	}

	public void deleteID(String id) {

	}

	public void correctRidingStat(String correctiondata, String stat, String id) {

	}

	public void registarAncate(Ancate ancate) {

	}

	public void registarNewId(String id, String name, char sex, int DOB) {

	}

	public void registarPassInformation(PassInformaiton pass, String id) {

	}

	public int getPassfee(int duration, String startstation, String endstation) {
		return 0;
	}

	public boolean searchUser(String id, int DOB) {
		return false;
	}

	public History getHistory(String id) {
		return null;
	}

	public AncateList getAncateList(Ancate.AnswerCondition stationancate) {
		return null;
	}

	public void registarAncateAnswer(AncateAnswer ans) {

	}

	public PassInformaiton getPassInformation(String id) {
		return null;
	}

	public String getEnterStation(String Id) {
		return null;
	}

}
