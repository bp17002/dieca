/*
zlib License
Copyright (c) 2018 GPS_NMEA_JP

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/
#include <stdio.h>
#include <windows.h>
#include <winscard.h>
#include <stdint.h>
#pragma comment (lib, "winscard.lib")

/*
SW1,SW2のエラーに関しては以下を参照
http://eternalwindows.jp/security/scard/scard07.html

掲載されていないエラーの場合は，Felica SDKに記載されていることがある．
Felicaエラーに関しては別
*/

//--------------Felica PaSoRi------------------------

//IOCTL_PCSC_CCID_ESCAPE : SCARD_CTL_CODE(3500) //汎用制御処理
#define ESC_CMD_GET_INFO 0xC0 //バージョンなど各種情報の取得
#define ESC_CMD_SET_OPTION 0xC1 //情報の設定
#define ESC_CMD_TARGET_COMM 0xC4 //ターゲット通信
#define ESC_CMD_SNEP 0xC6 //SNEP通信
#define ESC_CMD_APDU_WRAP 0xFF //PC / SC 2.02のAPDU用ラッパ

	//ESC_CMD_GET_INFO
#define DRIVER_VERSION 0x01 //ドライババージョン(AA.BB.CC.DD)
#define FW_VERSION 0x02
#define VENDOR_ID 0x04
#define VENDOR_NAME 0x05
#define PRODUCT_ID 0x06
#define PRODUCT_NAME 0x07
#define PRODUCT_SERIAL_NUMBER 0x08
#define CAPTURED_CARD_ID 0x10 //0=UNCAPTURED, 0xFF=Unknown
#define NFC_DEP_ATR_REG_GENERAL_BYYES 0x12

	//----------APDU------------
#define APDU_CLA_GENERIC     0xFF

#define APDU_INS_GET_DATA    0xCA
#define APDU_INS_READ_BINARY 0xB0
#define APDU_INS_UPDATE_BINARY 0xD6
#define APDU_INS_DATA_EXCHANGE 0xFE
#define APDU_INS_SELECT_FILE    0xA4

#define APDU_P2_NONE  0x00

	//----APDU_INS_GET_DATA----
#define APDU_P1_GET_UID    0x00
#define APDU_P1_GET_PMm    0x01
#define APDU_P1_GET_CARD_ID    0xF0
#define APDU_P1_GET_CARD_NAME  0xF1
#define APDU_P1_GET_CARD_SPEED 0xF2
#define APDU_P1_GET_CARD_TYPE  0xF3
#define APDU_P1_GET_CARD_TYPE_NAME  0xF4
#define APDU_P1_NFC_DEP_TARGET_STATE  0xF9

#define APDU_LE_MAX_LENGTH 0x00

#define UID_SIZE_ISO14443B  4
#define UID_SIZE_PICOPASS  8
#define UID_SIZE_NFCTYPE1  7
#define UID_SIZE_FELICA  8

#define CARD_SPEED_106KBPS 0x01
#define CARD_SPEED_212KBPS 0x01
#define CARD_SPEED_424KBPS 0x01
#define CARD_SPEED_848KBPS 0x01

#define CARD_TYPE_UNKNOWN    0x00
#define CARD_TYPE_ISO14443A  0x01
#define CARD_TYPE_ISO14443B  0x02
#define CARD_TYPE_PICOPASSB  0x03
#define CARD_TYPE_FELICA     0x04
#define CARD_TYPE_NFC_TYPE_1 0x05
#define CARD_TYPE_MIFARE_EC  0x06
#define CARD_TYPE_ISO14443A_4A  0x07
#define CARD_TYPE_ISO14443B_4B  0x08
#define CARD_TYPE_TYPE_A_NFC_DEP  0x09
#define CARD_TYPE_FELICA_NFC_DEP  0x0A

	//APDU_INS_READ_BINARY
#define USE_BLOCKLIST 0x80
#define NO_RFU 0

	//----------APDU_INS_DATA_EXCHANGE-----
#define APDU_P1_THRU     0x00
#define APDU_P1_DIRECT   0x01
#define APDU_P1_NFC_DEP  0x02
#define APDU_P1_DESELECT 0xFD
#define APDU_P1_RELEASE  0xFE

#define APDU_P2_TIMEOUT_AUTO  0x00
#define APDU_P2_TIMEOUT_INFINITY  0xFF

	//スマートカードリソースマネージャへの接続
int openService(LPSCARDCONTEXT hContextPtr)
{
	LONG res = SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, hContextPtr);
	if (res != SCARD_S_SUCCESS)
	{
		printf("[openService] スマートカードリソースマネージャへの接続に失敗:%d\n", res);
		return -1;
	}

	printf("[openService] スマートカードリソースマネージャへの接続に成功\n");
	return 0;
}

//スマートカードリソースマネージャからの切断・リーダー名領域開放
int closeService(SCARDCONTEXT hContext, LPTSTR lpszReaderName)
{
	if (lpszReaderName != NULL)
		SCardFreeMemory(hContext, lpszReaderName);
	SCardReleaseContext(hContext);

	printf("[closeService] スマートカードリソースマネージャから切断\n");
	return 0;
}


//カードリーダーのリストを取得．文字列ポインタのポインタ
int getCardreaderNameList(SCARDCONTEXT hContext, LPTSTR *lpszReaderName, LPDWORD readerNameListSize)
{
	DWORD dwAutoAllocate = SCARD_AUTOALLOCATE;

	LONG res = SCardListReaders(hContext, NULL, (LPTSTR)lpszReaderName, &dwAutoAllocate);
	if (res != SCARD_S_SUCCESS)
	{
		if (res == SCARD_E_NO_READERS_AVAILABLE)
			printf("[getCardreaderNameList] カードリーダが接続されていません。\n");
		else
			printf("[getCardreaderNameList] カードリーダの検出に失敗:%X\n", res);

		return -1;
	}

	*readerNameListSize = dwAutoAllocate;

	printf("[getCardreaderNameList] カードリーダー名リスト:");
	for (unsigned int i = 0; i < (*readerNameListSize); i++)
		putchar((*lpszReaderName)[i]);
	printf("\n");

	return 0;
}

//現在の状態を取得する(SCARD_STATE_UNAWARE)
int checkCardStatus(SCARDCONTEXT hContext, LPTSTR lpszReaderName, LPDWORD state, int timeout)
{
	SCARD_READERSTATE readerState;

	readerState.szReader = lpszReaderName;
	readerState.dwCurrentState = *state; //現在の状態は不明

	if (timeout == INFINITE)
		printf("[checkCardStatus] 状態スキャンを開始しました(timeout=無限に待機)\n");
	else if (timeout == 0)
		printf("[checkCardStatus] 状態スキャンを開始しました(timeout=待ち時間なし)\n");
	else
		printf("[checkCardStatus] 状態スキャンを開始しました(timeout=%dms)\n", timeout);

	LONG res = SCardGetStatusChange(hContext, timeout, &readerState, 1);
	if (res == SCARD_E_TIMEOUT)
	{
		printf("[checkCardStatus] 読み取りタイムアウト\n");
		return -1;
	}

	if (res != SCARD_S_SUCCESS)
	{
		printf("[checkCardStatus] 状態の取得に失敗:%X\n", res);
		SCardFreeMemory(hContext, lpszReaderName);
		SCardReleaseContext(hContext);
		return -2;
	}

	//状態を更新
	*state = readerState.dwEventState;

	if (readerState.dwEventState & SCARD_STATE_EMPTY)
	{
		printf("[checkCardStatus] カードがセットされていない\n");
		return -3;
	}

	if (readerState.dwEventState & SCARD_STATE_UNAVAILABLE)
	{
		printf("[checkCardStatus] カードリーダーが接続されていない\n");
		return -4;
	}

	if (!(readerState.dwEventState & SCARD_STATE_PRESENT))
	{
		printf("[checkCardStatus] 不明な状態\n");
		return -5;
	}

	printf("[checkCardStatus] カードがセットされています\n");
	return 0;
}

int connectCard(SCARDCONTEXT hContext, LPTSTR lpszReaderName, LPSCARDHANDLE  hCardPtr)
{
	DWORD dwActiveProtocol;
	LONG res = SCardConnect(hContext, lpszReaderName, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, hCardPtr, &dwActiveProtocol);
	if (res != SCARD_S_SUCCESS)
	{
		if (res == SCARD_W_REMOVED_CARD)
		{
			printf("[connectCard] カードがセットされていません\n");
			return -1;
		}
		else
		{
			printf("[connectCard] 不明なエラー\n");
			return -2;
		}
	}
	printf("[connectCard] カードに接続しました\n");

	return 0;
}

//直接接続(カードリーダーへダイレクトに指示可能)
int connectDirect(SCARDCONTEXT hContext, LPTSTR lpszReaderName, LPSCARDHANDLE  hCardPtr)
{
	DWORD dwActiveProtocol;
	LONG res = SCardConnect(hContext, lpszReaderName, SCARD_SHARE_DIRECT, 0, hCardPtr, &dwActiveProtocol);
	if (res != SCARD_S_SUCCESS)
	{
		if (res == SCARD_W_REMOVED_CARD)
		{
			printf("[connectDirect] 直接接続に失敗\n");
			return -1;
		}
		else
		{
			printf("[connectDirect] 不明なエラー\n");
			return -2;
		}
	}
	printf("[connectDirect] 直接接続しました\n");

	return 0;
}

int disconnectCard(SCARDHANDLE  hCard)
{
	printf("[disconnectCard] 接続を切断しました\n");

	SCardDisconnect(hCard, SCARD_LEAVE_CARD);
	return 0;
}

//-----------------


int getPaSoRiSerialNumber(SCARDHANDLE hCard, LPBYTE serialNumber)
{
	BYTE lpInBuffer[] = { ESC_CMD_GET_INFO,PRODUCT_SERIAL_NUMBER };
	BYTE lpOutBuffer[256];
	DWORD lpBytesReturned = 0;

	LONG res = SCardControl(hCard, SCARD_CTL_CODE(3500), lpInBuffer, sizeof(lpInBuffer), lpOutBuffer, sizeof(lpOutBuffer), &lpBytesReturned);
	if (res != SCARD_S_SUCCESS)
	{
		printf("[getPaSoRiSerialNumber] コマンドに誤りがあるか不適切な状態です %X\n", res);
		return -1;
	}

	if (serialNumber != NULL)
	{
		for (int i = 0; i < 256; i++)
			serialNumber[i] = lpOutBuffer[i];
	}

	printf("[getPaSoRiSerialNumber] カードリーダーシリアルナンバー:%s\n", lpOutBuffer);
	return 0;
}

int readIDm(SCARDHANDLE hCard)
{
	//CLA,INS,P1,P2,Le
	BYTE pbSendBuffer[5] = { APDU_CLA_GENERIC, APDU_INS_GET_DATA, APDU_P1_GET_UID,APDU_P2_NONE,APDU_LE_MAX_LENGTH };

	BYTE pbRecvBuffer[256];
	DWORD pcbRecvLength = 256;

	//コマンド送信
	LONG res = SCardTransmit(hCard, SCARD_PCI_T1, pbSendBuffer, sizeof(pbSendBuffer), NULL, pbRecvBuffer, &pcbRecvLength);
	if (res != SCARD_S_SUCCESS)
	{
		printf("[readIDm] コマンドに誤りがあるか不適切な状態です %X\n", res);
		return -1;
	}

	printf("[readIDm]:");
	for (unsigned int i = 0; i < pcbRecvLength - 2; i++) //最後の2バイトはレスポンス
	{
		printf("%02X,", pbRecvBuffer[i]);
	}
	printf("\n");

	//レスポンス解析
	BYTE SW1 = pbRecvBuffer[pcbRecvLength - 2];
	BYTE SW2 = pbRecvBuffer[pcbRecvLength - 1];

	if (SW1 != 0x90 || SW2 != 0x00)
	{
		printf("[readIDm] カードはエラー応答しました．SW1=%02X,SW2=%02X\n", SW1, SW2);
		return -2;
	}
	return 0;
}

int checkFelica(SCARDHANDLE hCard)
{
	//CLA,INS,P1,P2,Le
	BYTE pbSendBuffer[5] = { APDU_CLA_GENERIC, APDU_INS_GET_DATA, APDU_P1_GET_CARD_TYPE,APDU_P2_NONE,APDU_LE_MAX_LENGTH };

	BYTE pbRecvBuffer[256];
	DWORD pcbRecvLength = 256;

	//コマンド送信
	LONG res = SCardTransmit(hCard, SCARD_PCI_T1, pbSendBuffer, sizeof(pbSendBuffer), NULL, pbRecvBuffer, &pcbRecvLength);
	if (res != SCARD_S_SUCCESS)
	{
		printf("[checkFelica] コマンドに誤りがあるか不適切な状態です %X\n", res);
		return -1;
	}

	printf("[checkFelica] レスポンス:");
	for (unsigned int i = 0; i < pcbRecvLength - 2; i++) //最後の2バイトはレスポンス
	{
		printf("%02X,", pbRecvBuffer[i]);
	}
	printf("\n");

	//レスポンス解析
	BYTE SW1 = pbRecvBuffer[pcbRecvLength - 2];
	BYTE SW2 = pbRecvBuffer[pcbRecvLength - 1];

	if (SW1 != 0x90 || SW2 != 0x00)
	{
		printf("[checkFelica] カードはエラー応答しました．SW1=%02X,SW2=%02X\n", SW1, SW2);
		return -2;
	}

	if (pbRecvBuffer[0] != CARD_TYPE_FELICA)
	{
		printf("[checkFelica] このカードはFelicaではありません．\n");
		return 1;
	}
	printf("[checkFelica] このカードはFelicaです．\n");
	return 0;
}


int readCardTypeString(SCARDHANDLE hCard)
{
	//CLA,INS,P1,P2,Le
	BYTE pbSendBuffer[5] = { APDU_CLA_GENERIC, APDU_INS_GET_DATA, APDU_P1_GET_CARD_TYPE_NAME,APDU_P2_NONE,APDU_LE_MAX_LENGTH };

	BYTE pbRecvBuffer[256];
	DWORD pcbRecvLength = 256;

	//コマンド送信
	LONG res = SCardTransmit(hCard, SCARD_PCI_T1, pbSendBuffer, sizeof(pbSendBuffer), NULL, pbRecvBuffer, &pcbRecvLength);
	if (res != SCARD_S_SUCCESS)
	{
		printf("[readCardTypeString] コマンドに誤りがあるか不適切な状態です %X\n", res);
		return -1;
	}

	printf("[readCardTypeString] カード種別名称:");
	for (unsigned int i = 0; i < pcbRecvLength - 2; i++) //最後の2バイトはレスポンス
	{
		printf("%c", pbRecvBuffer[i]);
	}
	printf("\n");

	//レスポンス解析
	BYTE SW1 = pbRecvBuffer[pcbRecvLength - 2];
	BYTE SW2 = pbRecvBuffer[pcbRecvLength - 1];

	if (SW1 != 0x90 || SW2 != 0x00)
	{
		printf("[readCardTypeString] カードはエラー応答しました．SW1=%02X,SW2=%02X\n", SW1, SW2);
		return -2;
	}

	return 0;
}



int readBinary(SCARDHANDLE hCard, int adr, unsigned char dat[16])
{
	int DataSize = 3; //後続のデータ数(Le含めず)
	BYTE BlockNum = 1;

	int BlockList[3] = { 0 };
	BlockList[0] = 0; //0x00 : [長さ(1=1byte,0=2byte:1bit] [アクセスモード:3bit] [SERVICEコードリスト順番:4bit]
	BlockList[1] = (adr) & 0xFF;
	BlockList[2] = (adr >> 8) & 0xFF;

	int Le = 16 * BlockNum;

	BYTE pbSendBuffer[256] = { APDU_CLA_GENERIC,APDU_INS_READ_BINARY,USE_BLOCKLIST | NO_RFU,BlockNum };

	BYTE pbRecvBuffer[256];
	DWORD pcbRecvLength = 256;

	//データ生成
	int i = 0;
	for (i = 0; i < 3; i++)
	{
		pbSendBuffer[5 + i] = BlockList[i];
	}
	pbSendBuffer[4] = i; //データリストサイズを書き込む
	pbSendBuffer[5 + i] = Le; //最後に受信するブロックサイズを書き込む
	int bufsize = i + 6;


	//コマンド送信
	LONG res = SCardTransmit(hCard, SCARD_PCI_T1, pbSendBuffer, bufsize, NULL, pbRecvBuffer, &pcbRecvLength);
	if (res != SCARD_S_SUCCESS)
	{
		printf("[readBinary] コマンドに誤りがあるか不適切な状態です %X\n", res);
		return -1;
	}


	//レスポンス解析
	printf("[readBinary] レスポンス:");
	for (unsigned int i = 0; i < pcbRecvLength - 2; i++)
	{
		printf("%02X,", pbRecvBuffer[i]);
		if (i < 16)
			dat[i] = pbRecvBuffer[i];
	}
	printf("\n");


	BYTE SW1 = pbRecvBuffer[pcbRecvLength - 2];
	BYTE SW2 = pbRecvBuffer[pcbRecvLength - 1];

	if (SW1 != 0x90 || SW2 != 0x00)
	{
		printf("[readBinary] アドレス:%04X カードはエラー応答しました．SW1=%02X,SW2=%02X\n", SW1, SW2, adr);
		if (SW1 == 0x91 && SW2 == 0x00)
		{
			printf("Felicaカード側処理エラーです．コード:%02X,%02X ", pbRecvBuffer[0], pbRecvBuffer[1]);
			if (pbRecvBuffer[1] == 0xA2)
			{
				printf("ブロック数が0x01～0x04の範囲外");
			}
			if (pbRecvBuffer[1] == 0xA5)
			{
				printf("アクセス禁止領域である");
			}
			if (pbRecvBuffer[1] == 0xA6)
			{
				printf("サービスコードがRWまたはROではないか，不一致");
			}
			if (pbRecvBuffer[1] == 0xA8)
			{
				printf("ブロック番号が異常か，書き込み禁止領域への書き込み");
			}
			if (pbRecvBuffer[1] == 0xA9)
			{
				printf("書き込み失敗");
			}
			if (pbRecvBuffer[1] == 0xB0)
			{
				printf("MACとMAC_Aブロックを同時にアクセスしている");
			}
			if (pbRecvBuffer[1] == 0xB1)
			{
				printf("認証が必要なブロックで認証していない");
			}
			if (pbRecvBuffer[1] == 0xB2)
			{
				printf("MAC_Aブロックのアクセス前にRCブロックへの書き込みがない");
			}
			printf("\n");
		}

		return -2;
	}
	printf("[readBinary] アドレス:%04X 読み込み完了\n", adr);
	return 0;
}

int updateBinary(SCARDHANDLE hCard, int adr, unsigned char dat[16])
{
	int DataSize = 3; //後続のデータ数(Le含めず)
	BYTE BlockNum = 1;

	int BlockList[3] = { 0 };
	BlockList[0] = 0; //0x00 : [長さ(1=1byte,0=2byte:1bit] [アクセスモード:3bit] [SERVICEコードリスト順番:4bit]
	BlockList[1] = (adr) & 0xFF;
	BlockList[2] = (adr >> 8) & 0xFF;

	int Le = 16 * BlockNum;

	BYTE pbSendBuffer[256] = { APDU_CLA_GENERIC,APDU_INS_UPDATE_BINARY,USE_BLOCKLIST | NO_RFU,BlockNum };

	BYTE pbRecvBuffer[256];
	DWORD pcbRecvLength = 256;

	//データ生成
	int i, j;
	for (i = 0; i < 3; i++)
	{
		pbSendBuffer[5 + i] = BlockList[i];
	}
	for (j = 0; j < 16; j++)
	{
		pbSendBuffer[5 + i + j] = dat[j];
	}
	pbSendBuffer[4] = i + j; //データサイズを書き込む
	pbSendBuffer[5 + i + j] = Le; //最後に受信するブロックサイズを書き込む
	int bufsize = i + j + 6;

	//コマンド送信
	LONG res = SCardTransmit(hCard, SCARD_PCI_T1, pbSendBuffer, sizeof(pbSendBuffer), NULL, pbRecvBuffer, &pcbRecvLength);
	if (res != SCARD_S_SUCCESS)
	{
		printf("[updateBinary] コマンドに誤りがあるか不適切な状態です %X\n", res);
		return -1;
	}

	//レスポンス解析
	BYTE SW1 = pbRecvBuffer[pcbRecvLength - 2];
	BYTE SW2 = pbRecvBuffer[pcbRecvLength - 1];

	if (SW1 != 0x90 || SW2 != 0x00)
	{
		printf("[updateBinary]  アドレス:%04X カードはエラー応答しました．SW1=%02X,SW2=%02X\n", SW1, SW2, adr);
		if (SW1 == 0x91 && SW2 == 0x00)
		{
			printf("Felicaカード側処理エラーです．コード:%02X,%02X ", pbRecvBuffer[0], pbRecvBuffer[1]);
			if (pbRecvBuffer[1] == 0xA2)
			{
				printf("ブロック数が0x01～0x04の範囲外");
			}
			if (pbRecvBuffer[1] == 0xA5)
			{
				printf("アクセス禁止領域である");
			}
			if (pbRecvBuffer[1] == 0xA6)
			{
				printf("サービスコードがRWまたはROではないか，不一致");
			}
			if (pbRecvBuffer[1] == 0xA8)
			{
				printf("ブロック番号が異常か，書き込み禁止領域への書き込み");
			}
			if (pbRecvBuffer[1] == 0xA9)
			{
				printf("書き込み失敗");
			}
			if (pbRecvBuffer[1] == 0xB0)
			{
				printf("MACとMAC_Aブロックを同時にアクセスしている");
			}
			if (pbRecvBuffer[1] == 0xB1)
			{
				printf("認証が必要なブロックで認証していない");
			}
			if (pbRecvBuffer[1] == 0xB2)
			{
				printf("MAC_Aブロックのアクセス前にRCブロックへの書き込みがない");
			}
			printf("\n");
		}

		printf("[updateBinary] レスポンス:");
		for (unsigned int i = 0; i < pcbRecvLength - 2; i++)
		{
			printf("%02X,", pbRecvBuffer[i]);
		}
		printf("\n");

		return -2;
	}
	printf("[updateBinary]  アドレス:%04X 書き込み成功\n", adr);
	return 0;
}


//1次発行を行う
void format1st(SCARDHANDLE hCard)
{
	//CKV = 0
	unsigned char ckeyv[16] = { 0 };
	updateBinary(hCard, 0x86, ckeyv);

	//CK1 = 0, CK2 = 0
	unsigned char ckey[16] = { 0 };
	updateBinary(hCard, 0x87, ckey);


	//MC_ALL = RO
	//SYS_OP = NDEF対応
	//MC_CKCKV_W_MAC_A = 1 (書き込みにMAC必要/あとからカード鍵書き換え許可 )
	//MC_STATE_W_MAC_A = 1 (相互認証有効)

	unsigned char dat[16] = { 0xFF,0xFF,0x00,0x01,0x07,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00 };
	updateBinary(hCard, 0x88, dat);
}

void printDatHex(int adr, unsigned char dat[16])
{
	printf("|");
	for (int i = 0; i < 16; i++)
	{
		if (dat == NULL)
			printf("--|");
		else
			printf("%02X|", dat[i]);
	}
	printf("0x%04X\n", adr);
}

void FelicaLiteSDump(SCARDHANDLE hCard)
{
	unsigned char dat[256][16] = { 0 };
	unsigned char mc[16] = { 0 };
	//  printDatHex(dat);

	for (int i = 0; i < 14; i++)
	{
		readBinary(hCard, i, dat[i]);
	}
	readBinary(hCard, 0x0E, dat[0x0E]);
	for (int i = 0; i < 9; i++)
	{
		readBinary(hCard, 0x80 + i, dat[0x80 + i]);
	}
	readBinary(hCard, 0x90, dat[0x90]);
	//readBinary(hCard, 0x91, dat[0x91]);
	readBinary(hCard, 0x92, dat[0x92]);
	readBinary(hCard, 0xA0, dat[0xA0]);

	readBinary(hCard, 0x88, mc);

	//-----------------------
	printf("\n --- FelicaLiteSDump ---\n");
	for (int i = 0; i < 14; i++)
	{
		printf("S_PAD%02d", i);
		printDatHex(i, dat[i]);
	}

	printf("  REG  ");
	printDatHex(0x0E, dat[0x0E]);

	char name[][8] = { "   RC  ","  MAC  ","   ID  ","  D_ID "," SER_C "," SYS_C ","  CKV  ","   CK  ","   MC  " };
	for (int i = 0; i < 9; i++)
	{
		printf("%s", name[i]);
		printDatHex(0x80 + i, dat[0x80 + i]);
	}

	printf("  WCNT ");
	printDatHex(0x90, dat[0x90]);

	printf("  MAC_A");
	printDatHex(0x91, NULL);

	printf(" STATE ");
	printDatHex(0x92, dat[0x92]);

	printf("  CRC  ");
	printDatHex(0xA0, dat[0xA0]);

	//----MC----

	printf("\n--- MCレジスタの状態 ---\n");

	//MC_SP_REG_ALL_RW
	printf("MC_SP_REG_ALL_RW : %02X %02X\n", mc[0] & 0xFF, mc[1] & 0xFF);
	if ((mc[1] & 0x80) == 0)
	{
		printf("MC_SP_REG_ALL_RW : 2次発行済み\n");
	}
	else {
		printf("MC_SP_REG_ALL_RW : 2次発行はまだ行われていません\n");
	}

	//MC_ALL
	if (mc[2] == 0x00)
	{
		printf("MC_ALL           : 1次発行済み(システムブロック書き込み禁止)\n");
	}
	else if (mc[2] == 0xFF)
	{
		printf("MC_ALL           : 1次発行はまだ行われていません(システムブロック書き込み許可)\n");
	}
	else
	{
		printf("MC_ALL           : 異常な設定\n");
	}

	//SYS_OP
	if (mc[3] == 0x01)
	{
		printf("SYS_OP           : NDEF対応\n");
	}
	else if (mc[3] == 0x00)
	{
		printf("SYS_OP           : NDEF非対応\n");
	}
	else
	{
		printf("SYS_OP           : 異常な設定\n");
	}

	//RF_PRM
	if (mc[4] == 0x07)
	{
		printf("RF_PRM           : 正常\n");
	}
	else
	{
		printf("RF_PRM           : 不明\n");
	}

	//C_CKCKV_W_MAC_A
	if (mc[5] == 0x01)
	{
		printf("C_CKCKV_W_MAC_A  : 書き込みにMACが必要(ロック後書き込み可)\n");
	}
	else if (mc[5] == 0x00)
	{
		printf("C_CKCKV_W_MAC_A  : 書き込みにMACが不要(ロック後書き込み不可)\n");
	}
	else
	{
		printf("C_CKCKV_W_MAC_A  : 異常な設定\n");
	}

	//MC_SP_REG_R_REST
	printf("MC_SP_REG_R_REST : %02X %02X\n", mc[6], mc[7]);

	//MC_SP_REG_W_REST
	printf("MC_SP_REG_W_REST : %02X %02X\n", mc[8], mc[9]);

	//MC_SP_REG_W_MAC_A
	printf("MC_SP_REG_W_MAC_A: %02X %02X\n", mc[10], mc[11]);

	//MC_STATE_W_MAC_A
	if (mc[12] == 0x01)
	{
		printf("MC_STATE_W_MAC_A : STATE書き込みにMACが必要\n");
	}
	else if (mc[5] == 0x00)
	{
		printf("MC_STATE_W_MAC_A : STATE書き込みにMACが不要\n");
	}
	else
	{
		printf("MC_STATE_W_MAC_A : 異常な設定\n");
	}

	printf("\n ---END ---\n");

}





int main(void)
{
	//スマートカードリソースマネージャへの接続
	SCARDCONTEXT hContext;
	if (openService(&hContext) != 0)
		return -1;


	//カードリーダー名リストを取得
	LPTSTR       lpszReaderName;
	DWORD        readerNameListSize;
	if (getCardreaderNameList(hContext, &lpszReaderName, &readerNameListSize))
	{
		closeService(hContext, NULL);
		return -1;
	}

	//カードリーダーに接続
	SCARDHANDLE  hCardreader;
	if (connectDirect(hContext, lpszReaderName, &hCardreader) != 0)
	{
		closeService(hContext, lpszReaderName);
		return -1;
	}
	//カードリーダーのシリアルを読む
	getPaSoRiSerialNumber(hCardreader, NULL);
	//カードリーダーに接続
	disconnectCard(hCardreader);

	//現在のカードの状態を取得する
	DWORD state = SCARD_STATE_UNAWARE;
	if (checkCardStatus(hContext, lpszReaderName, &state, 0) == -3)
	{
		//カードがセットされるまで待つ
		if (checkCardStatus(hContext, lpszReaderName, &state, INFINITE) != 0)
		{
			closeService(hContext, lpszReaderName);
			return -1;
		}
	}

	//カードに接続
	SCARDHANDLE  hCard;
	if (connectCard(hContext, lpszReaderName, &hCard) != 0)
	{
		closeService(hContext, lpszReaderName);
		return -1;
	}


	//-----------------------------

	//カードと通信します
	//CardComm(hContext, hCard);

	readIDm(hCard);
	readCardTypeString(hCard);
	if (checkFelica(hCard) != 0)
	{
		disconnectCard(hCard);
		closeService(hContext, lpszReaderName);
		return 0;
	}

	//unsigned char rdat[16];
	//readBinary(hCard, 0x1780, rdat);

	//unsigned char dat[16] = { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16 };
	//updateBinary(hCard, 0x1780, dat);

	//Felica Lite-Sのダンプと発行状態の確認
	//FelicaLiteSDump(hCard);

	//-----------------------------

	//カード開放
	disconnectCard(hCard);

	//Service開放
	closeService(hContext, lpszReaderName);

	return 0;
}