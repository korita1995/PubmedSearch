#include "Resource.h"
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <tchar.h>
#include <shlwapi.h>
#include "PubmedSearchUtils.h"
#include "PubmedSearchApp.h"
#include <regex>

/*
実装すべき機能 (2020/6/24)
1) Ctrl+Aで入力欄を全選択
2) キーワード検索で複数論文がヒットした場合、すべての検索結果を表示する
3) 論文ファイル名のトリミングをウムラウトなどの特殊文字に対応
4) ヘルプを表示するメッセージボックスのサイズを変更する
*/

#define GetMonitorRect(rc)  SystemParametersInfo(SPI_GETWORKAREA,0,rc,0)
#define SIZE 2000
#define VK_A 0x41

using namespace std;

// グローバル変数:
HINSTANCE hInst;	// 現在のインターフェイス
HWND hDlgCurrent;	// ダイアログのハンドル
HWND hEdit1;	// 入力欄のハンドル
HWND hEdit3;	// 検索結果欄のハンドル
HWND button1;
HWND button2;
HWND button3;
HWND button4;
TCHAR* commandLineArg;
DLGPROC OrghEdit1, OrghEdit3;   //オリジナルプロシージャのアドレス

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) {
	hInst = hInstance; // グローバル変数にインスタンスを格納
	commandLineArg = GetCommandLine();

	// モードレスダイアログボックスの作成およびハンドル取得
	hDlgCurrent = CreateDialog(hInst, TEXT("DLG"), NULL, (DLGPROC)DlgProc);
	ShowWindow(hDlgCurrent, nCmdShow);	// ダイアログ表示

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE:
		DialogBox(hInst, TEXT("DLG"), hwnd, (DLGPROC)DlgProc);
		break;

	case WM_CLOSE:
		
		DestroyWindow(hwnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		// ダイアログボックスの生成時
	case WM_INITDIALOG:
	{
		// ダイアログの位置を中央に移動
		//SetDlgPosCenter(hwnd);
		loadWindowState(hwnd);

		// エディットコントロールのハンドルを取得してグローバル変数に格納
		hEdit1 = GetDlgItem(hwnd, IDC_EDIT1);
		hEdit3 = GetDlgItem(hwnd, IDC_EDIT3);
		button1 = GetDlgItem(hwnd, IDC_EXACT_SEARCH);
		button2 = GetDlgItem(hwnd, IDC_CROSSREF_SEARCH);
		button3 = GetDlgItem(hwnd, IDOK);
		button4 = GetDlgItem(hwnd, IDC_CLIP_SEARCH);


		// WM_DROPFILESメッセージを処理するようにする
		DragAcceptFiles(hwnd, TRUE);

		// 左上にアイコン表示
		HICON hIcon;
		hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_APP), IMAGE_ICON, 16, 16, 0);
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

		HGLOBAL hg;
		PTSTR strText, strClip;

		// アプリ起動時にPDFファイルをドラッグ＆ドロップされたかチェック
		TCHAR* CommandLineArg = PathFindFileName(commandLineArg);
		WCHAR* CommandLineArgExtension = PathFindExtension(commandLineArg); // ファイル拡張子
		wchar_t extp[] = L".pdf";
		wchar_t extP[] = L".PDF";
		const wchar_t* pd = wcsstr(CommandLineArgExtension, extp);
		const wchar_t* Pd = wcsstr(CommandLineArgExtension, extP);
		// ドラッグ＆ドロップしたファイルの拡張子が.pdfもしくは.PDFかを判定
		if ((pd == NULL) & (Pd == NULL)) {
			// .pdfもしくは.PDFファイルがドラッグ＆ドロップされなかったとき（＝.exeファイルを通常起動した時）
			// クリップボードをコピーして入力欄にペースト
			if (OpenClipboard(hwnd) && (hg = GetClipboardData(CF_UNICODETEXT))) {
				strText = (PTSTR)malloc(GlobalSize(hg));
				strClip = (PTSTR)GlobalLock(hg);
				lstrcpy(strText, strClip);
				GlobalUnlock(hg);
				SetWindowText(hEdit1, strText);

				// クリップボードの内容が数字（PMID）だった場合
				if (checkPMID(strText)) {
					char* paperIdChar = wcharToChar(strText);
					string paperId = paperIdChar;
					try {
						//SetWindowText(hEdit3, TEXT("PubmedでPMID検索を実行開始"));
						string newFileName = searchPubmedId(paperId); // PMIDで検索を実行
						setSearchResult(hwnd, hEdit3, newFileName);
					}
					catch (...) {
						//MessageBox(hwnd, TEXT("この検索結果は無効です"), TEXT("エラー"), MB_OK);
						//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT(""));
						SetWindowText(hEdit3, TEXT(""));
					}

				}
				else {
					// 論文名で検索を実行
					char* szfileName = wcharToChar(strText);
					string szfileNameStr = string(szfileName);
					regex re("^[0-9]*\\..*/*");
					string newFileName;
					try {
						//SetWindowText(hEdit3, TEXT("Pubmedでキーワード検索を実行開始"));
						if (szfileNameStr.find("https://doi.org/") != string::npos) {
							szfileNameStr = replaceString(szfileNameStr, "https://doi.org/"s, ""s);
						}
						else if (szfileNameStr.find("doi: ") != string::npos) {
							szfileNameStr = replaceString(szfileNameStr, "doi: "s, ""s);
							if (szfileNameStr[szfileNameStr.size() - 1] == '.')
								szfileNameStr.pop_back();
						}
						else if (szfileNameStr.find("DOI: ") != string::npos) {
							szfileNameStr = replaceString(szfileNameStr, "DOI: "s, ""s);
							if (szfileNameStr[szfileNameStr.size() - 1] == '.')
								szfileNameStr.pop_back();
						}
						if (regex_search(szfileNameStr.c_str(), re)) {
							newFileName = searchPubmedKeyword(szfileNameStr.c_str());
						}
						else {
							szfileNameStr = replaceString(szfileNameStr, "/"s, ""s);
							newFileName = searchPubmedKeyword(szfileNameStr.c_str());
						}
						//string newFileName = searchPubmedKeyword(szfileName);
						setSearchResult(hwnd, hEdit3, newFileName);
					}
					catch (...) {
						//MessageBox(hwnd, TEXT("この検索結果は無効です"), TEXT("エラー"), MB_OK);
						//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT(""));
						SetWindowText(hEdit3, TEXT(""));
					}

				}
				free(strText);
				CloseClipboard();
			}
		}
		else {
			PathRemoveExtension(CommandLineArg);
			SetWindowText(hEdit1, CommandLineArg);
			char* cfileName = wcharToChar(CommandLineArg);
			try {
				string newFileName = searchPubmedKeyword(cfileName); // Pubmedでキーワード検索
				setSearchResult(hwnd, hEdit3, newFileName);
			}
			catch (...) {
				//MessageBox(hwnd, TEXT("この検索結果は無効です"), TEXT("エラー"), MB_OK);
				//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT(""));
				SetWindowText(hEdit3, TEXT(""));
			}
		}
		break;
	}

	// PDFをドラッグ＆ドロップ時に実行
	case WM_DROPFILES:
	{
		SetWindowText(hEdit3, TEXT(""));
		HDROP hDrop;
		UINT uFileNo;
		//static TCHAR dFile[SIZE] = TEXT("");
		static TCHAR dFile[SIZE];
		HANDLE hFile;
		hDrop = (HDROP)wParam; // ドロップされたファイル数
		uFileNo = DragQueryFile((HDROP)wParam, -1, NULL, 0);
		WCHAR* dFileName;
		WCHAR* dFileNameExtension;

		// 複数のPDFをドラッグ＆ドロップした時（エラー）
		if (uFileNo > 1) {
			//MessageBox(hwnd, TEXT("ファイルを開けませんでした"), TEXT("エラー"), MB_OK);
			//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("エラー: ファイルを開けませんでした"));
			SetWindowText(hEdit3, TEXT("エラー: 開けるPDFファイルは1つまでです"));
		}

		// 1つのPDFをドラッグ＆ドロップした時に実行
		else {
			DragQueryFile(hDrop, 0, dFile, sizeof(dFile));
			//MessageBox(hwnd, dFile, TEXT(""), MB_OK);
			hFile = CreateFile(dFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			Sleep(20);
			// 有効なファイルかチェックしてから実行
			if (hFile != INVALID_HANDLE_VALUE) {
			//if (TRUE){
				//MessageBox(hwnd, TEXT("ファイルを開くのに成功"), TEXT(""), MB_OK);
				CloseHandle(hFile);
				dFileName = PathFindFileName(dFile); // ファイル名
				dFileNameExtension = PathFindExtension(dFile); // .を含めたファイルの拡張子
				wchar_t ext[] = L".pdf";
				wchar_t Ext[] = L".PDF";
				const wchar_t* p = wcsstr(dFileNameExtension, ext);
				const wchar_t* P = wcsstr(dFileNameExtension, Ext);
				// ドラッグ＆ドロップしたファイルの拡張子が.pdfもしくは.PDFかを判定
				if ((p == NULL) & (P == NULL)) {
					//MessageBox(hwnd, TEXT("PDFのみ選択可能です"), TEXT("エラー"), MB_OK);
					//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("エラー: PDFのみ選択可能です"));
					SetWindowText(hEdit3, TEXT("エラー: PDFのみ選択可能です"));
				}
				else {
					PathRemoveExtension(dFileName);
					SetWindowText(hEdit1, dFileName);
					char* dfileName = wcharToChar(dFileName); // WCHAR*型からchar*型への変換
					try {
						disableAllButtons();
						//SetWindowText(hEdit3, TEXT("Pubmedでキーワード検索を実行開始"));
						string newFileName = searchPubmedKeyword(dfileName); // Pubmedでキーワード検索
						enableAllButtons();
						setSearchResult(hwnd, hEdit3, newFileName);
					}
					catch (...) {
						//MessageBox(hwnd, TEXT("この検索結果は無効です"), TEXT("エラー"), MB_OK);
						//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("エラー: 文献が見つかりませんでした"));
						enableAllButtons();
						SetWindowText(hEdit3, TEXT("エラー: 文献が見つかりませんでした"));
					}

				}
			}
		}
		DragFinish(hDrop);
		break;
	}

	case WM_CLOSE:
		saveWindowState(hwnd);
		DestroyWindow(hwnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	/*
	// システムキー以外のキーボードが押された時
	case WM_KEYDOWN:
		// Ctrl+Aを押したときに実行（うまくいってないためmust fix）
		if (LOWORD(wParam) == VK_A) {
			if (GetKeyState(VK_CONTROL) < 0) {
				SetFocus(hEdit1);
				SendDlgItemMessage(hwnd, IDC_EDIT1, EM_SETSEL, 0, -1);
			}
		}
		break;
	*/

	// ダイアログに対して何かしらの操作が行われた時
	case WM_COMMAND:
	{
		UINT wmId;
		wmId = LOWORD(wParam);

		switch (wmId) {
			// ファイル>終了を選択時
		case IDM_FILE_EXIT:

			EndDialog(hwnd, IDM_FILE_EXIT);
			break;

			// ヘルプ>操作方法を選択時
		case IDM_HELP_ABOUT:
		{
			TCHAR hCap[SIZE];
			TCHAR hText[SIZE];
			LoadString(hInst, HELP_CAPTION, hCap, SIZE);
			LoadString(hInst, HELP_TEXT, hText, SIZE);
			if (IDOK == ::MessageBox(NULL, hText, hCap, MB_OK)) {
				break;
			}
			break;
		}

		// ファイル>開くを選択時
		case IDM_FILE_OPEN:
		{
			SetWindowText(hEdit3, TEXT(""));
			TCHAR szFile[SIZE];
			// 選択した論文PDFのファイルのフルパスを取得してバッファーに格納
			if (getFileName(0, szFile, SIZE, _TEXT("C:\\"))) {
				WCHAR* szFileName;
				szFileName = PathFindFileName(szFile); // ファイルのフルパスからファイル名のみを取得
				char* szfileName = wcharToChar(szFileName); // WCHAR*型からchar*型への変換
				PathRemoveExtension(szFileName);
				SetWindowText(hEdit1, szFileName);
				try {
					//SetWindowText(hEdit3, TEXT("Pubmedでキーワード検索を実行開始"));
					disableAllButtons();
					string newFileName = searchPubmedKeyword(szfileName); // Pubmedでキーワード検索
					enableAllButtons();
					setSearchResult(hwnd, hEdit3, newFileName);
				}
				catch (...) {
					//MessageBox(hwnd, TEXT("この検索結果は無効です"), TEXT("エラー"), MB_OK);
					//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("エラー: 文献が見つかりませんでした"));
					enableAllButtons();
					SetWindowText(hEdit3, TEXT("エラー: 文献が見つかりませんでした"));
				}

			}
			break;
		}

		// 「入力欄を削除」ボタンを押した時
		case IDC_DEL:
			SendMessage(hEdit1, EM_SETSEL, 0, -1);
			SendMessage(hEdit1, WM_CLEAR, 0, 0);
			break;

			// 「クリップボードから検索」ボタンを押した時
		case IDC_CLIP_SEARCH:
		{
			SetWindowText(hEdit3, TEXT(""));
			HGLOBAL hg;
			PTSTR strText, strClip;
			// クリップボードをコピーしてPubmed検索
			if (OpenClipboard(hwnd) && (hg = GetClipboardData(CF_UNICODETEXT))) {
				strText = (PTSTR)malloc(GlobalSize(hg));
				strClip = (PTSTR)GlobalLock(hg);
				lstrcpy(strText, strClip);
				GlobalUnlock(hg);
				SetWindowText(hEdit1, strText);

				// クリップボードの内容が数字（PMID）だった場合
				if (checkPMID(strText)) {
					char* paperIdChar = wcharToChar(strText);
					string paperId = paperIdChar;
					try {
						disableAllButtons();
						//SetWindowText(hEdit3, TEXT("PubmedでPMID検索を実行開始"));
						string newFileName = searchPubmedId(paperId); // PMIDで検索を実行
						enableAllButtons();
						setSearchResult(hwnd, hEdit3, newFileName);
					}
					catch (...) {
						//MessageBox(hwnd, TEXT("この検索結果は無効です"), TEXT("エラー"), MB_OK);
						//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("エラー: 文献が見つかりませんでした"));
						enableAllButtons();
						SetWindowText(hEdit3, TEXT("エラー: 文献が見つかりませんでした"));
					}

				}
				else {
					// 論文名で検索を実行
					char* szfileName = wcharToChar(strText);
					string szfileNameStr = string(szfileName);
					regex re("^[0-9]*\\..*/*");
					string newFileName;
					try {
						//SetWindowText(hEdit3, TEXT("Pubmedでキーワード検索を実行開始"));
						if (szfileNameStr.find("https://doi.org/") != string::npos) {
							szfileNameStr = replaceString(szfileNameStr, "https://doi.org/"s, ""s);
						}
						else if (szfileNameStr.find("doi: ") != string::npos) {
							szfileNameStr = replaceString(szfileNameStr, "doi: "s, ""s);
							if (szfileNameStr[szfileNameStr.size() - 1] == '.')
								szfileNameStr.pop_back();
						}
						else if (szfileNameStr.find("DOI: ") != string::npos) {
							szfileNameStr = replaceString(szfileNameStr, "DOI: "s, ""s);
							if (szfileNameStr[szfileNameStr.size() - 1] == '.')
								szfileNameStr.pop_back();
						}
						disableAllButtons();
						if (regex_search(szfileNameStr.c_str(), re)) {
							newFileName = searchPubmedKeyword(szfileNameStr.c_str());
						}
						else {
							szfileNameStr = replaceString(szfileNameStr, "/"s, ""s);
							newFileName = searchPubmedKeyword(szfileNameStr.c_str());
						}
						//string newFileName = searchPubmedKeyword(szfileName);
						enableAllButtons();
						setSearchResult(hwnd, hEdit3, newFileName);
					}
					catch (...) {
						//MessageBox(hwnd, TEXT("この検索結果は無効です"), TEXT("エラー"), MB_OK);
						//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("エラー: 文献が見つかりませんでした"));
						enableAllButtons();
						SetWindowText(hEdit3, TEXT("エラー: 文献が見つかりませんでした"));
					}

				}

				free(strText);
				CloseClipboard();
			}
			break;
		}

		// 「入力欄から検索」ボタンを押した時
		case IDOK:
		{
			SetWindowText(hEdit3, TEXT(""));
			TCHAR szBuf[SIZE];
			// 検索ボタンを実行時
			GetDlgItemText(hwnd, IDC_EDIT1, szBuf, (int)sizeof(szBuf));
			if (isStrSpace(szBuf)) {
				//MessageBox(hwnd, TEXT("PMIDもしくは論文名（キーワード）を入力してください"), TEXT("エラー"), MB_OK);
				//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("エラー: PMIDもしくは論文名（キーワード）を入力してください"));
				SetWindowText(hEdit3, TEXT("エラー: PMIDもしくは論文名（キーワード）を入力してください"));
			}

			else {
				if (checkPMID(szBuf)){
					// PMIDで検索を実行
					char* paperIdChar = wcharToChar(szBuf);
					string paperId = string(paperIdChar);
					try {
						//SetWindowText(hEdit3, TEXT("PubmedでPMID検索を実行開始"));
						disableAllButtons();
						string newFileName = searchPubmedId(paperId);
						enableAllButtons();
						setSearchResult(hwnd, hEdit3, newFileName);
					}
					catch (...) {
						//MessageBox(hwnd, TEXT("この検索結果は無効です"), TEXT("エラー"), MB_OK);
						//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("エラー: 文献が見つかりませんでした"));
						enableAllButtons();
						SetWindowText(hEdit3, TEXT("エラー: 文献が見つかりませんでした"));
					}

				}
				else {
					// 論文名で検索を実行
					char* szfileName = wcharToChar(szBuf);
					string szfileNameStr = string(szfileName);
					regex re("^[0-9]*\\..*/*");
					string newFileName;
					try {
						//SetWindowText(hEdit3, TEXT("Pubmedでキーワード検索を実行開始"));
						if (szfileNameStr.find("https://doi.org/") != string::npos) {
							szfileNameStr = replaceString(szfileNameStr, "https://doi.org/"s, ""s);
						}
						else if (szfileNameStr.find("doi: ") != string::npos) {
							szfileNameStr = replaceString(szfileNameStr, "doi: "s, ""s);
							if (szfileNameStr[szfileNameStr.size() - 1] == '.')
								szfileNameStr.pop_back();
						}
						else if (szfileNameStr.find("DOI: ") != string::npos) {
							szfileNameStr = replaceString(szfileNameStr, "DOI: "s, ""s);
							if (szfileNameStr[szfileNameStr.size() - 1] == '.')
								szfileNameStr.pop_back();
						}
						disableAllButtons();
						if (regex_search(szfileNameStr.c_str(), re)) {
							newFileName = searchPubmedKeyword(szfileNameStr.c_str());
						}
						else {
							szfileNameStr = replaceString(szfileNameStr, "/"s, ""s);
							newFileName = searchPubmedKeyword(szfileNameStr.c_str());
						}
						//string newFileName = searchPubmedKeyword(szfileName);
						//string newFileName = exactSearchPubmedKeyword(szfileName);
						//string newFileName = searchCrossrefKeyword(szfileName);
						if (newFileName == "") {
							enableAllButtons();
							SetWindowText(hEdit3, TEXT("エラー: 文献が見つかりませんでした"));
						}
						else {
							enableAllButtons();
							setSearchResult(hwnd, hEdit3, newFileName);
						}
							
					}
					catch (...) {
						//MessageBox(hwnd, TEXT("この検索結果は無効です"), TEXT("エラー"), MB_OK);
						//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("エラー: 文献が見つかりませんでした"));
						enableAllButtons();
						SetWindowText(hEdit3, TEXT("エラー: 文献が見つかりませんでした"));
					}

				}
			}
			break;
		}

		case IDC_EXACT_SEARCH:
		{
			SetWindowText(hEdit3, TEXT(""));
			TCHAR szBuf[SIZE];
			// 検索ボタンを実行時
			GetDlgItemText(hwnd, IDC_EDIT1, szBuf, (int)sizeof(szBuf));
			if (isStrSpace(szBuf)) {
				SetWindowText(hEdit3, TEXT("エラー: PMIDもしくは論文名（キーワード）を入力してください"));
			}
			else {
				char* szfileName = wcharToChar(szBuf);
				string szfileNameStr = string(szfileName);
				regex re("^[0-9]*\\..*/*");
				string newFileName;
				try {
					//SetWindowText(hEdit3, TEXT("Pubmedでキーワード高精度検索を実行開始"));
					if (szfileNameStr.find("https://doi.org/") != string::npos) {
						szfileNameStr = replaceString(szfileNameStr, "https://doi.org/"s, ""s);
					}
					else if (szfileNameStr.find("doi: ") != string::npos) {
						szfileNameStr = replaceString(szfileNameStr, "doi: "s, ""s);
						if (szfileNameStr[szfileNameStr.size() - 1] == '.')
							szfileNameStr.pop_back();
					}
					else if (szfileNameStr.find("DOI: ") != string::npos) {
						szfileNameStr = replaceString(szfileNameStr, "DOI: "s, ""s);
						if (szfileNameStr[szfileNameStr.size() - 1] == '.')
							szfileNameStr.pop_back();
					}
					disableAllButtons();
					if (regex_search(szfileNameStr.c_str(), re)) {
						newFileName = exactSearchPubmedKeyword(szfileNameStr.c_str());
					}
					else {
						szfileNameStr = replaceString(szfileNameStr, "/"s, ""s);
						newFileName = exactSearchPubmedKeyword(szfileNameStr.c_str());
					}
					
					if (newFileName == "") {
						enableAllButtons();
						SetWindowText(hEdit3, TEXT("エラー: 文献が見つかりませんでした"));
					}
					else {
						enableAllButtons();
						setSearchResult(hwnd, hEdit3, newFileName);
					}
				}
				catch (...) {
					//MessageBox(hwnd, TEXT("この検索結果は無効です"), TEXT("エラー"), MB_OK);
					//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("エラー: 文献が見つかりませんでした"));
					enableAllButtons();
					SetWindowText(hEdit3, TEXT("エラー: 文献が見つかりませんでした"));
				}
			}
			break;
		}

		case IDC_CROSSREF_SEARCH:
		{
			SetWindowText(hEdit3, TEXT(""));
			TCHAR dzBuf[SIZE];
			// 検索ボタンを実行時
			GetDlgItemText(hwnd, IDC_EDIT1, dzBuf, (int)sizeof(dzBuf));
			if (isStrSpace(dzBuf)) {
				SetWindowText(hEdit3, TEXT("エラー: DOIもしくは論文名（キーワード）を入力してください"));
			}
			else {
				char* dzfileName = wcharToChar(dzBuf);
				string dzfileNameStr = string(dzfileName);
				regex re("^[0-9]*\\..*/*");
				try {
					if (dzfileNameStr.find("https://doi.org/") != string::npos) {
						dzfileNameStr = replaceString(dzfileNameStr, "https://doi.org/"s, ""s);
					}
					else if (dzfileNameStr.find("doi: ") != string::npos) {
						dzfileNameStr = replaceString(dzfileNameStr, "doi: "s, ""s);
						if (dzfileNameStr[dzfileNameStr.size()-1] == '.')
							dzfileNameStr.pop_back();
					}
					else if (dzfileNameStr.find("DOI: ") != string::npos) {
						dzfileNameStr = replaceString(dzfileNameStr, "DOI: "s, ""s);
						if (dzfileNameStr[dzfileNameStr.size()-1] == '.')
							dzfileNameStr.pop_back();
					}
					disableAllButtons();
					//if (string(dzfileName).find("/") != string::npos) {
					if (regex_search(dzfileNameStr.c_str(), re)){
						//SetWindowText(hEdit3, TEXT("CrossrefでDOI検索を実行開始"));
						string newFileName = searchCrossrefDoi(dzfileNameStr.c_str());
						enableAllButtons();
						setSearchResult(hwnd, hEdit3, newFileName);
					}
					else {
						//SetWindowText(hEdit3, TEXT("Crossrefでキーワード検索を実行開始"));
						string newFileName = searchCrossrefKeyword(dzfileNameStr.c_str());
						enableAllButtons();
						setSearchResult(hwnd, hEdit3, newFileName);
					}
				}
				catch (...) {
					enableAllButtons();
					SetWindowText(hEdit3, TEXT("エラー: 文献が見つかりませんでした"));
				}

			}
			break;
		}

		case IDC_EDIT3:
		{
			TCHAR cBuf[SIZE];
			// エディットボックスが変更された場合
			if (HIWORD(wParam) == EN_UPDATE) {
				GetDlgItemText(hwnd, IDC_EDIT3, (TCHAR*)cBuf, sizeof(cBuf) / sizeof(TCHAR));
				sendClip(hwnd, cBuf);
			}
			break;
		}
		default:
			break;
		}
	}
	}
	return 0;
}

// 半角空白文字,全角空白文字のチェック
bool isStrSpace(TCHAR* str) {
	StrTrim(str, L" ");
	const size_t textSize = 256;
	char lpcText[textSize];
	WideCharToMultiByte(CP_ACP, 0, str, -1, lpcText, textSize, NULL, NULL);
	char* pPoint = lpcText;
	bool bSpace = TRUE; // 文字列が『全角スペース』又は『半角スペース』のみで構成されているかどうか
	while (*pPoint != '\0') {
		if (!memcmp(pPoint, "　", 2)) {
			pPoint += 2;
		}
		else if (!memcmp(pPoint, " ", 1)) {
			pPoint++;
		}
		else {
			bSpace = FALSE;
			break;
		}
	}
	return bSpace;
}

void sendClip(HWND hwnd, TCHAR* cBuf) {
	HGLOBAL hg;
	PTSTR	strMem;
	if (OpenClipboard(hwnd)) {
		EmptyClipboard();
		hg = GlobalAlloc(GHND | GMEM_SHARE, SIZE);
		strMem = (PTSTR)GlobalLock(hg);
		lstrcpy(strMem, cBuf);
		GlobalUnlock(hg);
		SetClipboardData(CF_UNICODETEXT, hg);
		CloseClipboard();
	}
}

BOOL SetDlgPosCenter(HWND hwnd) {
	RECT    rc1;        // デスクトップ領域
	RECT    rc2;        // ウインドウ領域
	INT     cx, cy;     // ウインドウ位置
	INT     sx, sy;     // ウインドウサイズ

	// サイズの取得
	GetMonitorRect(&rc1);                            // デスクトップのサイズ
	GetWindowRect(hwnd, &rc2);                            // ウインドウのサイズ
	// いろいろと計算
	sx = (rc2.right - rc2.left);                            // ウインドウの横幅
	sy = (rc2.bottom - rc2.top);                            // ウインドウの高さ
	cx = (((rc1.right - rc1.left) - sx) / 2 + rc1.left);    // 横方向の中央座標軸
	cy = (((rc1.bottom - rc1.top) - sy) / 2 + rc1.top);     // 縦方向の中央座標軸
	// 画面中央に移動
	return SetWindowPos(hwnd, NULL, cx, cy, 0, 0, (SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER));
}

void setSearchResult(HWND hwnd, HWND hEdit, string newFileName) {
	const char* nfn = newFileName.c_str();
	WCHAR* Nfn = charToWchar(nfn); // char*型からWCHAR*型への変換
	sendClip(hwnd, Nfn);
	//DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), TEXT("CHILD"), hwnd, (DLGPROC)childDlgProc);
	SetWindowText(hEdit, Nfn);
}

string replaceString(string target, string from, string to) {
	string result = target;
	string::size_type pos = 0;
	while (pos = result.find(from, pos), pos != string::npos) {
		result.replace(pos, from.length(), to);
		pos += to.length();
	}
	return result;
}

void enableAllButtons() {
	EnableWindow(button1, TRUE);
	EnableWindow(button2, TRUE);
	EnableWindow(button3, TRUE);
	EnableWindow(button4, TRUE);
}

void disableAllButtons() {
	EnableWindow(button1, FALSE);
	EnableWindow(button2, FALSE);
	EnableWindow(button3, FALSE);
	EnableWindow(button4, FALSE);
}

void saveRect(const RECT* pRect, HKEY hKey) {
	LONG result;
	TCHAR strNum[64];
	_ltow_s(pRect->left, strNum, 10);
	result = RegSetValueEx(
		hKey,	// 現在オープンしているキーのハンドル
		TEXT("left"),	// 値の「名前」が入った文字列へのポインタ
		0,	// 予約パラメータ。0を指定する
		REG_SZ,	// 値の「種類」を指定する。NULLで終わる文字列はREG_SZ、32ビット値はREG_DWORD
		(CONST BYTE*)(LPCTSTR)strNum,	// 格納する値の「データ」が入ったバッファへのポインタ
		(int)sizeof(strNum)		// dataのサイズを指定する
	);

	_ltow_s(pRect->top, strNum, 10);
	result = RegSetValueEx(
		hKey,	// 現在オープンしているキーのハンドル
		TEXT("top"),	// 値の「名前」が入った文字列へのポインタ
		0,	// 予約パラメータ。0を指定する
		REG_SZ,	// 値の「種類」を指定する。NULLで終わる文字列はREG_SZ、32ビット値はREG_DWORD
		(CONST BYTE*)(LPCTSTR)strNum,	// 格納する値の「データ」が入ったバッファへのポインタ
		(int)sizeof(strNum)		// dataのサイズを指定する
	);

	_ltow_s(pRect->right, strNum, 10);
	result = RegSetValueEx(
		hKey,	// 現在オープンしているキーのハンドル
		TEXT("right"),	// 値の「名前」が入った文字列へのポインタ
		0,	// 予約パラメータ。0を指定する
		REG_SZ,	// 値の「種類」を指定する。NULLで終わる文字列はREG_SZ、32ビット値はREG_DWORD
		(CONST BYTE*)(LPCTSTR)strNum,	// 格納する値の「データ」が入ったバッファへのポインタ
		(int)sizeof(strNum)		// dataのサイズを指定する
	);

	_ltow_s(pRect->bottom, strNum, 10);
	result = RegSetValueEx(
		hKey,	// 現在オープンしているキーのハンドル
		TEXT("bottom"),	// 値の「名前」が入った文字列へのポインタ
		0,	// 予約パラメータ。0を指定する
		REG_SZ,	// 値の「種類」を指定する。NULLで終わる文字列はREG_SZ、32ビット値はREG_DWORD
		(CONST BYTE*)(LPCTSTR)strNum,	// 格納する値の「データ」が入ったバッファへのポインタ
		(int)sizeof(strNum)		// dataのサイズを指定する
	);
}

void saveWindowState(HWND hwnd) {
	HKEY hKey;
	DWORD dwDisposition;
	LONG result;
	WINDOWPLACEMENT wndPlace;
	wndPlace.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(hwnd, &wndPlace);
	RECT rcWnd = wndPlace.rcNormalPosition;

	result = RegCreateKeyEx(HKEY_CURRENT_USER,
		//HKEY_CLASSES_ROOT
		//HKEY_CURRENT_CONFIG
		//HKEY_CURRENT_USER
		//HKEY_LOCAL_MACHINE
		//HKEY_USERS
		//Windows NT/2000：HKEY_PERFORMANCE_DATA も指定できます。
		//Windows 95/98：HKEY_DYN_DATA も指定できます。
		TEXT("Yakusaku\\SearchPubmed2FileName\\Position"),//キー
		0,//予約
		NULL,//指定
		REG_OPTION_VOLATILE, //システムを再起動すると消える揮発性
		//REG_OPTION_NON_VOLATILE,//不揮発性
		KEY_ALL_ACCESS,//標準アクセス権のすべての権利を組み合わせたもの
		NULL,
		&hKey,
		&dwDisposition);
	//result が ERROR_SUCCESS であれば成功である．

	saveRect(&rcWnd, hKey);

	//キーを閉じる
	RegCloseKey(hKey);
}

void loadRect(RECT* pRect, HKEY hKey)
{
	pRect->left = ::_ttoi(loadDataFromReg(hKey, TEXT("left")));
	pRect->top = ::_ttoi(loadDataFromReg(hKey, TEXT("top")));
	pRect->right = ::_ttoi(loadDataFromReg(hKey, TEXT("right")));
	pRect->bottom = ::_ttoi(loadDataFromReg(hKey, TEXT("bottom")));
}

TCHAR* loadDataFromReg(HKEY hKey, LPCWSTR query) {
	LONG result;
	TCHAR data[1024];
	DWORD dwType;		// 値の種類を受け取る
	DWORD dwSize;		// データのサイズを受け取る

	result = RegQueryValueEx(
		hKey,	// 現在オープンしているキーのハンドル
		query,	// 取得する値の「名前」が入った文字列へのポインタ
		NULL,	// 予約パラメータ。NULLを指定する
		&dwType,	// 値の「種類」を受け取る
		NULL,		// 値の「データ」を受け取る。NULLを指定することも可能だが、データは受け取れない
		&dwSize		// 終端文字'\0'を含んだDataのサイズを取得する
	);
	//実際にデータを取得（サイズの指定が正しくないと失敗することがある）
	result = RegQueryValueEx(
		hKey,	// 現在オープンしているキーのハンドル
		query,	// 取得する値の「名前」が入った文字列へのポインタ
		NULL,	// 予約パラメータ。NULLを指定する
		&dwType,	// 値の「種類」を受け取る
		(LPBYTE)(LPCTSTR)&data,	// 値の「データ」を受け取る。NULLを指定することも可能だが、データは受け取れない
		&dwSize		// Dataのサイズを指定する
	);

	return data;
}

void loadWindowState(HWND hwnd) {
	HKEY hKey;
	DWORD dwDisposition;
	LONG result;
	WINDOWPLACEMENT wndPlace;
	wndPlace.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(hDlgCurrent, &wndPlace);
	RECT rcWnd = wndPlace.rcNormalPosition;

	result = RegCreateKeyEx(HKEY_CURRENT_USER,
		//HKEY_CLASSES_ROOT
		//HKEY_CURRENT_CONFIG
		//HKEY_CURRENT_USER
		//HKEY_LOCAL_MACHINE
		//HKEY_USERS
		//Windows NT/2000：HKEY_PERFORMANCE_DATA も指定できます。
		//Windows 95/98：HKEY_DYN_DATA も指定できます。
		TEXT("Yakusaku\\SearchPubmed2FileName\\Position"),//キー
		0,//予約
		NULL,//指定
		REG_OPTION_VOLATILE, //システムを再起動すると消える揮発性
		//REG_OPTION_NON_VOLATILE,//不揮発性
		KEY_ALL_ACCESS,//標準アクセス権のすべての権利を組み合わせたもの
		NULL,
		&hKey,
		&dwDisposition);
	//result が ERROR_SUCCESS であれば成功である．

	loadRect(&rcWnd, hKey);

	// 対象モニタの情報を取得
	HMONITOR hMonitor = MonitorFromRect(
		&rcWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(hMonitor, &mi);

	// 位置補正
	if (rcWnd.right > mi.rcMonitor.right)
	{
		rcWnd.left -= rcWnd.right - mi.rcMonitor.right;
		rcWnd.right = mi.rcMonitor.right;
	}
	if (rcWnd.left < mi.rcMonitor.left)
	{
		rcWnd.right += mi.rcMonitor.left - rcWnd.left;
		rcWnd.left = mi.rcMonitor.left;
	}
	if (rcWnd.bottom > mi.rcMonitor.bottom)
	{
		rcWnd.top -= rcWnd.bottom - mi.rcMonitor.bottom;
		rcWnd.bottom = mi.rcMonitor.bottom;
	}
	if (rcWnd.top < mi.rcMonitor.top)
	{
		rcWnd.bottom += mi.rcMonitor.top - rcWnd.top;
		rcWnd.top = mi.rcMonitor.top;
	}

	wstring wsb = to_wstring(rcWnd.bottom);
	//MessageBox(hwnd, wsb.c_str(), TEXT("bottom"), MB_OK);
	wstring wsl = to_wstring(rcWnd.left);
	//MessageBox(hwnd, wsl.c_str(), TEXT("left"), MB_OK);
	wstring wsr = to_wstring(rcWnd.right);
	//MessageBox(hwnd, wsr.c_str(), TEXT("right"), MB_OK);
	wstring wst = to_wstring(rcWnd.top);
	//MessageBox(hwnd, wst.c_str(), TEXT("top"), MB_OK);
	// ウィンドウ位置復元
	
	if (!((rcWnd.left == 0) & (rcWnd.top == 0) & (rcWnd.right == 0) & (rcWnd.bottom == 0))) {
		SetWindowPos(
			hwnd, NULL, rcWnd.left, rcWnd.top,
			rcWnd.right - rcWnd.left, rcWnd.bottom - rcWnd.top,
			SWP_NOZORDER);
	}
	else {
		SetDlgPosCenter(hwnd);
	}

	//キーを閉じる
	RegCloseKey(hKey);
}