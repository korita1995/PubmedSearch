
#include "Resource.h"
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <tchar.h>
#include <shlwapi.h>
#include "paperLabeling.h"
//#include "mainApp.h"

/*
実装すべき機能 (2020/6/17)
1) PDFをドラッグ＆ドロップで検索機能の追加
（ドラッグ＆ドロップでファイルパスを取得）
2) TitleかPMIDかの判定機能の追加
（入力内容が数字かを判定）
3) ウィンドウ起動時に、クリップボードの内容が文字列か数字か判定
4) 3)の判定次第で、論文名かPMIDのどちらかのラジオボタンを選択＆入力欄の初期値とする
5) 変換後のファイル名を自動でクリップボードにコピー
6) 変換後のファイル名を編集した場合、あとからクリップボードにコピー可能にする
6) 論文が複数ヒットした場合、その中から候補を選択する
*/

#define HANDLE_DLG_MSG(hwnd, msg, fn) \
    case(msg): \
        return SetDlgMsgResult(hwnd, msg, HANDLE_##msg(hwnd, wParam, lParam,fn))
#define GetMonitorRect(rc)  SystemParametersInfo(SPI_GETWORKAREA,0,rc,0)
#define SIZE 1000

using namespace std;

// プロトタイプ宣言
LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL SetDlgPosCenter(HWND);

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

LRESULT CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_INITDIALOG:
		SetDlgPosCenter(hwnd);
		break;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_COMMAND:
		{
			INT iCheck;
			UINT wmId;
			wmId = LOWORD(wParam);

			switch (wmId) {
			case IDCANCEL:
				{
				//TCHAR* szMsg = (TCHAR*)malloc(SIZE);
				//TCHAR* szCaption = (TCHAR*)malloc(SIZE);
				TCHAR szMsg[SIZE];
				TCHAR szCaption[SIZE];
				LoadString(GetModuleHandle(NULL), IDS_MSG_QUIT, szMsg, SIZE);
				LoadString(GetModuleHandle(NULL), IDS_CAPTION_CONF, szCaption, SIZE);
				if (IDYES == MessageBox(hwnd, szMsg, szCaption, MB_YESNO)) {
					// 閉じるボタンが押されたとき確認メッセージを出す
					DestroyWindow(hwnd);
					//free(szMsg);
					//free(szCaption);
				}
				break;
			}

			case IDM_FILE_EXIT:
				// ファイル>終了を選択時
				EndDialog(hwnd, IDM_FILE_EXIT);
				break;

			case IDM_HELP_ABOUT:
				{
				//TCHAR* hCap = (TCHAR*)malloc(SIZE);
				//TCHAR* hText = (TCHAR*)malloc(SIZE);
				TCHAR hCap[SIZE];
				TCHAR hText[SIZE];
				// ヘルプ>操作方法を選択時
				LoadString(GetModuleHandle(NULL), HELP_CAPTION, hCap, SIZE);
				LoadString(GetModuleHandle(NULL), HELP_TEXT, hText, SIZE);
				if (IDOK == ::MessageBox(NULL, hText, hCap, MB_OK)) {
					//free(hCap);
					//free(hText);
					break;
				}
				break;
			}

			case IDM_FILE_OPEN:
			{
				//TCHAR* szFile = (TCHAR*)malloc(SIZE);
				TCHAR szFile[SIZE];
				// ファイル>開くを選択時
				if (getFileName(0, szFile, SIZE, _TEXT("C:\\"))) {
					// szFile: 選択した論文PDFのファイルのフルパス(WCHAR*型)
					// 選択した論文PDFのファイル名を取得
					WCHAR* szFileName;
					szFileName = PathFindFileName(szFile);
					char* szfileName = wcharToChar(szFileName); // WCHAR*型からchar*型への変換

					// Pubmedで論文検索して、Esummaryの情報をもとにファイル名生成
					string newFileName = searchPubmedKeyword(szfileName);
					const char* nfn = newFileName.c_str();
					WCHAR* Nfn = charToWchar(nfn); // char*型からWCHAR*型への変換

					HWND child = CreateWindow(TEXT("EDIT"), Nfn, WS_OVERLAPPEDWINDOW | WS_VISIBLE | ES_AUTOVSCROLL | ES_MULTILINE | WS_VSCROLL,
						CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);
					if (!child) {
						break;
					}
				}
				break;
			}

			case IDOK:
			{
				//TCHAR* szBuf = (TCHAR*)malloc(SIZE);
				TCHAR szBuf[SIZE];
				// 検索ボタンを実行時
				iCheck = Button_GetCheck(GetDlgItem(hwnd, IDC_RADIO1));
				GetDlgItemText(hwnd, IDC_EDIT1, szBuf, (int)sizeof(szBuf));
				if (iCheck == BST_CHECKED) {
					// PMIDで検索を実行
					char* paperIdChar = wcharToChar(szBuf);
					//free(szBuf);
					string paperId = paperIdChar;
					string newFileName = searchPubmedId(paperId);
					const char* nfn = newFileName.c_str();
					WCHAR* Nfn = charToWchar(nfn); // char*型からWCHAR*型への変換

					HWND child = CreateWindow(TEXT("EDIT"), Nfn, WS_OVERLAPPEDWINDOW | WS_VISIBLE | ES_AUTOVSCROLL | ES_MULTILINE | WS_VSCROLL,
						CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);
					if (!child)
						break;
				}
				else {
					// 論文名で検索を実行
					char* szfileName = wcharToChar(szBuf);
					//free(szBuf);
					string newFileName = searchPubmedKeyword(szfileName);
					const char* nfn = newFileName.c_str();
					WCHAR* Nfn = charToWchar(nfn); // char*型からWCHAR*型への変換

					HWND child = CreateWindow(TEXT("EDIT"), Nfn, WS_OVERLAPPEDWINDOW | WS_VISIBLE | ES_AUTOVSCROLL | ES_MULTILINE | WS_VSCROLL,
						CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);
					if (!child)
						break;
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
	HACCEL haccel;
	MSG msg;
	WNDCLASS winc;

	winc.style = CS_HREDRAW | CS_VREDRAW;
	winc.lpfnWndProc = WndProc;
	winc.cbClsExtra = winc.cbWndExtra = 0;
	winc.hInstance = hInstance;
	winc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
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

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}
