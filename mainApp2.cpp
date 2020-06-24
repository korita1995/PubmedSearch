
#include "Resource.h"
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <tchar.h>
#include <shlwapi.h>
#include "paperLabeling.h"
//#include "mainApp.h"

/*
実装すべき機能 (2020/6/24)
1) Ctrl+Aで入力欄を全選択
2) キーワード検索で複数論文がヒットした場合、すべての検索結果を表示する
*/

#define HANDLE_DLG_MSG(hwnd, msg, fn) \
    case(msg): \
        return SetDlgMsgResult(hwnd, msg, HANDLE_##msg(hwnd, wParam, lParam,fn))
#define GetMonitorRect(rc)  SystemParametersInfo(SPI_GETWORKAREA,0,rc,0)
#define SIZE 1000
#define VK_A 0x41

using namespace std;

// プロトタイプ宣言
LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL SetDlgPosCenter(HWND);
void sendClip(HWND, TCHAR*);
LRESULT CALLBACK childDlgProc(HWND, UINT, WPARAM, LPARAM);
bool IsStrSpace(TCHAR*);

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

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE:
		DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), TEXT("DLG"), hwnd, (DLGPROC)DlgProc);
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

LRESULT CALLBACK childDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		SetDlgPosCenter(hwnd);
		HGLOBAL hg;
		PTSTR strText, strClip;

		if (OpenClipboard(hwnd) && (hg = GetClipboardData(CF_UNICODETEXT))) {
			strText = (PTSTR)malloc(GlobalSize(hg));
			strClip = (PTSTR)GlobalLock(hg);
			lstrcpy(strText, strClip);
			GlobalUnlock(hg);
			SetWindowText(GetDlgItem(hwnd, IDC_EDIT2), strText);
			free(strText);
			CloseClipboard();
		}
		break;

	case WM_CLOSE:
		EndDialog(hwnd, WM_CLOSE);
		break;

	case WM_DESTROY:
		break;

	case WM_COMMAND:
	{
		switch (LOWORD(wParam)) {
		case IDC_EDIT2:
			TCHAR cBuf[SIZE];
			HGLOBAL hg;
			PTSTR	strMem;
			// エディットボックスが変更された場合
			if (HIWORD(wParam) == EN_UPDATE) {
				GetDlgItemText(hwnd, IDC_EDIT2, (TCHAR*)cBuf, sizeof(cBuf) / sizeof(TCHAR));
				sendClip(hwnd, cBuf);
			}
			break;
		}
		break;
	}
	default:
		break;
	}
	return 0;
}

LRESULT CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	// ダイアログボックスの生成時
	case WM_INITDIALOG:
		// ダイアログの位置を中央に移動
		SetDlgPosCenter(hwnd);

		// WM_DROPFILESメッセージを処理するようにする
		DragAcceptFiles(hwnd, TRUE);

		// クリップボードをコピーして入力欄にペースト
		HGLOBAL hg;
		PTSTR strText, strClip;
		
		if (OpenClipboard(hwnd) && (hg = GetClipboardData(CF_UNICODETEXT))) {
			strText = (PTSTR)malloc(GlobalSize(hg));
			strClip = (PTSTR)GlobalLock(hg);
			lstrcpy(strText, strClip);
			GlobalUnlock(hg);
			SetWindowText(GetDlgItem(hwnd, IDC_EDIT1), strText);
			free(strText);
			CloseClipboard();
		}

		// 左上にアイコン表示
		HICON hIcon;
		hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APP), IMAGE_ICON, 16, 16, 0);
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

		break;

	// PDFをドラッグ＆ドロップ時に実行
	case WM_DROPFILES:
	{
		HDROP hDrop;
		UINT uFileNo;
		static TCHAR dFile[SIZE];
		HANDLE hFile;
		hDrop = (HDROP)wParam; // ドロップされたファイル数
		uFileNo = DragQueryFile((HDROP)wParam, -1, NULL, 0);
		WCHAR* dFileName;
		WCHAR* dFileNameExtension;

		// 複数のPDFをドラッグ＆ドロップした時（エラー）
		if (uFileNo > 1)
			MessageBox(hwnd, TEXT("ファイルを開けませんでした"), TEXT("エラー"), MB_OK);

		// 1つのPDFをドラッグ＆ドロップした時に実行
		else {
			DragQueryFile(hDrop, 0, dFile, sizeof(dFile));
			hFile = CreateFile(dFile,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
			// 有効なファイルかチェックしてから実行
			if (hFile != INVALID_HANDLE_VALUE) {
				CloseHandle(hFile);
				dFileName = PathFindFileName(dFile); // ファイル名
				dFileNameExtension = PathFindExtension(dFile); // .を含めたファイルの拡張子
				wchar_t ext[] = L".pdf";
				wchar_t Ext[] = L".PDF";
				const wchar_t* p = wcsstr(dFileNameExtension, ext);
				const wchar_t* P = wcsstr(dFileNameExtension, Ext);
				// ドラッグ＆ドロップしたファイルの拡張子が.pdfもしくは.PDFかを判定
				if ((p == NULL)&(P == NULL))
					MessageBox(hwnd, TEXT("PDFのみ選択可能です"), TEXT("エラー"), MB_OK);
				else {
					char* dfileName = wcharToChar(dFileName); // WCHAR*型からchar*型への変換
					string newFileName = searchPubmedKeyword(dfileName); // Pubmedでキーワード検索
					const char* nfn = newFileName.c_str(); // string型からconst char*型への変換
					WCHAR* Nfn = charToWchar(nfn); // char*型からWCHAR*型への変換
					sendClip(hwnd, Nfn); // クリップボードに検索結果を転送
					//DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), TEXT("CHILD"), hwnd, (DLGPROC)childDlgProc);
					SetWindowText(GetDlgItem(hwnd, IDC_EDIT3), Nfn); // アプリ下側のエディットコントロールに検索結果をペースト
				}
			}
		}
		DragFinish(hDrop);
		break;
	}

	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	// システムキー以外のキーボードが押された時
	case WM_KEYDOWN:
		// Ctrl+Aを押したときに実行（うまくいってないためmust fix）
		if (LOWORD(wParam) == VK_A) {
			if (GetKeyState(VK_CONTROL) < 0) {
				int nCount;
				nCount = GetWindowTextLength(hwnd);
				SendMessage(GetDlgItem(hwnd, IDC_EDIT1), EM_SETSEL, 0, -1);
			}
		}
		break;

	// ダイアログに対して何かしらの操作が行われた時
	case WM_COMMAND:
		{
			INT iCheck;
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
				LoadString(GetModuleHandle(NULL), HELP_CAPTION, hCap, SIZE);
				LoadString(GetModuleHandle(NULL), HELP_TEXT, hText, SIZE);
				if (IDOK == ::MessageBox(NULL, hText, hCap, MB_OK)) {
					break;
				}
				break;
			}

			// ファイル>開くを選択時
			case IDM_FILE_OPEN:
			{
				TCHAR szFile[SIZE];
				// 選択した論文PDFのファイルのフルパスを取得してバッファーに格納
				if (getFileName(0, szFile, SIZE, _TEXT("C:\\"))) {
					WCHAR* szFileName;
					szFileName = PathFindFileName(szFile); // ファイルのフルパスからファイル名のみを取得
					char* szfileName = wcharToChar(szFileName); // WCHAR*型からchar*型への変換
					string newFileName = searchPubmedKeyword(szfileName); // Pubmedでキーワード検索
					const char* nfn = newFileName.c_str();
					WCHAR* Nfn = charToWchar(nfn); // char*型からWCHAR*型への変換
					sendClip(hwnd, Nfn); // クリップボードに検索結果を転送
					//DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), TEXT("CHILD"), hwnd, (DLGPROC)childDlgProc);
					SetWindowText(GetDlgItem(hwnd, IDC_EDIT3), Nfn); // アプリ下側のエディットコントロールに検索結果をペースト
				}
				break;
			}

			// 「入力欄を削除」ボタンを押した時
			case IDC_DEL:
				SendMessage(GetDlgItem(hwnd, IDC_EDIT1), EM_SETSEL, 0, -1);
				SendMessage(GetDlgItem(hwnd, IDC_EDIT1), WM_CLEAR, 0, 0);
				break;

			// 「クリップボードから検索」ボタンを押した時
			case IDC_SEARCH:
			{
				HGLOBAL hg;
				PTSTR strText, strClip;
				// クリップボードをコピーしてPubmed検索
				if (OpenClipboard(hwnd) && (hg = GetClipboardData(CF_UNICODETEXT))) {
					strText = (PTSTR)malloc(GlobalSize(hg));
					strClip = (PTSTR)GlobalLock(hg);
					lstrcpy(strText, strClip);
					GlobalUnlock(hg);

					// クリップボードの内容が数字（PMID）だった場合
					int	nValue;
					if (nValue = ::_ttoi(strText)) {
						char* paperIdChar = wcharToChar(strText);
						string paperId = paperIdChar;
						try {
							string newFileName = searchPubmedId(paperId); // PMIDで検索を実行
							const char* nfn = newFileName.c_str();
							WCHAR* Nfn = charToWchar(nfn); // char*型からWCHAR*型への変換
							sendClip(hwnd, Nfn);
							//DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), TEXT("CHILD"), hwnd, (DLGPROC)childDlgProc);
							SetWindowText(GetDlgItem(hwnd, IDC_EDIT3), Nfn);
						}
						catch (...) { MessageBox(hwnd, TEXT("この検索結果は無効です"), TEXT("エラー"), MB_OK); }

					}
					else {
						// 論文名で検索を実行
						char* szfileName = wcharToChar(strText);
						try {
							string newFileName = searchPubmedKeyword(szfileName);
							const char* nfn = newFileName.c_str();
							WCHAR* Nfn = charToWchar(nfn); // char*型からWCHAR*型への変換
							sendClip(hwnd, Nfn);
							//DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), TEXT("CHILD"), hwnd, (DLGPROC)childDlgProc);
							SetWindowText(GetDlgItem(hwnd, IDC_EDIT3), Nfn);
						}
						catch (...) { MessageBox(hwnd, TEXT("この検索結果は無効です"), TEXT("エラー"), MB_OK); }

					}

					free(strText);
					CloseClipboard();
				}
				break;
			}

			// 「入力欄から検索」ボタンを押した時
			case IDOK:
			{
				TCHAR szBuf[SIZE];
				// 検索ボタンを実行時
				GetDlgItemText(hwnd, IDC_EDIT1, szBuf, (int)sizeof(szBuf));
				if (IsStrSpace(szBuf))
					MessageBox(hwnd, TEXT("PMIDもしくは論文名（キーワード）を入力してください"), TEXT("エラー"), MB_OK);
				else {
					int	nValue;
					if (nValue = ::_ttoi(szBuf)) {
						// PMIDで検索を実行
						char* paperIdChar = wcharToChar(szBuf);
						string paperId = paperIdChar;
						try {
							string newFileName = searchPubmedId(paperId);
							const char* nfn = newFileName.c_str();
							WCHAR* Nfn = charToWchar(nfn); // char*型からWCHAR*型への変換
							sendClip(hwnd, Nfn);
							//DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), TEXT("CHILD"), hwnd, (DLGPROC)childDlgProc);
							SetWindowText(GetDlgItem(hwnd, IDC_EDIT3), Nfn);
						}
						catch (...){ MessageBox(hwnd, TEXT("この検索結果は無効です"), TEXT("エラー"), MB_OK); }
						
					}
					else {
						// 論文名で検索を実行
						char* szfileName = wcharToChar(szBuf);
						try {
							string newFileName = searchPubmedKeyword(szfileName);
							const char* nfn = newFileName.c_str();
							WCHAR* Nfn = charToWchar(nfn); // char*型からWCHAR*型への変換
							sendClip(hwnd, Nfn);
							//DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), TEXT("CHILD"), hwnd, (DLGPROC)childDlgProc);
							SetWindowText(GetDlgItem(hwnd, IDC_EDIT3), Nfn);
						}
						catch (...){ MessageBox(hwnd, TEXT("この検索結果は無効です"), TEXT("エラー"), MB_OK); }
						
					}
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



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) {

	HWND hwnd;
	MSG msg;
	WNDCLASS winc;

	winc.style = CS_HREDRAW | CS_VREDRAW;
	winc.lpfnWndProc = WndProc;
	winc.cbClsExtra = winc.cbWndExtra = 0;
	winc.hInstance = hInstance;
	winc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP));
	winc.hCursor = LoadCursor(NULL, IDC_ARROW);
	winc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	winc.lpszMenuName = NULL;
	winc.lpszClassName = TEXT("TEST");

	if (!RegisterClass(&winc))
		return -1;

	hwnd = CreateWindow(
		TEXT("TEST"), TEXT("Test"),
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL,
		hInstance, NULL
	);

	if (hwnd == NULL)
		return -1;

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

// 半角空白文字,全角空白文字のチェック
bool IsStrSpace(TCHAR* str) {
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