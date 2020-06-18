
#include "Resource.h"
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <tchar.h>
#include <shlwapi.h>
#include "paperLabeling.h"
//#include "mainApp.h"

/*
�������ׂ��@�\ (2020/6/17)
1) PDF���h���b�O���h���b�v�Ō����@�\�̒ǉ� (done)
�i�h���b�O���h���b�v�Ńt�@�C���p�X���擾�j
2) Title��PMID���̔���@�\�̒ǉ� (done)
�i���͓��e���������𔻒�j
3) �E�B���h�E�N�����ɁA�N���b�v�{�[�h�̓��e�������񂩐��������� (done)
4) 3)�̔��莟��ŁA�_������PMID�̂ǂ��炩�̃��W�I�{�^����I�������͗��̏����l�Ƃ��� (done)
5) �ϊ���̃t�@�C�����������ŃN���b�v�{�[�h�ɃR�s�[ (done)
6) �ϊ���̃t�@�C������ҏW�����ꍇ�A���Ƃ���N���b�v�{�[�h�ɃR�s�[�\�ɂ��� (done)
6) �_���������q�b�g�����ꍇ�A���̒��������I������
*/

#define HANDLE_DLG_MSG(hwnd, msg, fn) \
    case(msg): \
        return SetDlgMsgResult(hwnd, msg, HANDLE_##msg(hwnd, wParam, lParam,fn))
#define GetMonitorRect(rc)  SystemParametersInfo(SPI_GETWORKAREA,0,rc,0)
#define SIZE 1000

using namespace std;

// �v���g�^�C�v�錾
LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL SetDlgPosCenter(HWND);
void sendClip(HWND, TCHAR*);
LRESULT CALLBACK childDlgProc(HWND, UINT, WPARAM, LPARAM);

BOOL SetDlgPosCenter(HWND hwnd) {
	RECT    rc1;        // �f�X�N�g�b�v�̈�
	RECT    rc2;        // �E�C���h�E�̈�
	INT     cx, cy;     // �E�C���h�E�ʒu
	INT     sx, sy;     // �E�C���h�E�T�C�Y

	// �T�C�Y�̎擾
	GetMonitorRect(&rc1);                            // �f�X�N�g�b�v�̃T�C�Y
	GetWindowRect(hwnd, &rc2);                            // �E�C���h�E�̃T�C�Y
	// ���낢��ƌv�Z
	sx = (rc2.right - rc2.left);                            // �E�C���h�E�̉���
	sy = (rc2.bottom - rc2.top);                            // �E�C���h�E�̍���
	cx = (((rc1.right - rc1.left) - sx) / 2 + rc1.left);    // �������̒������W��
	cy = (((rc1.bottom - rc1.top) - sy) / 2 + rc1.top);     // �c�����̒������W��
	// ��ʒ����Ɉړ�
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
			//MessageBox(hwnd, strText, TEXT("�N���b�v�{�[�h�̓��e"), MB_OK);
			SetWindowText(GetDlgItem(hwnd, IDC_EDIT2), strText);
			free(strText);
			CloseClipboard();
		}
		break;

	case WM_CLOSE:
		//DestroyWindow(hwnd);
		EndDialog(hwnd, WM_CLOSE);
		break;

	case WM_DESTROY:
		//PostQuitMessage(0);
		break;

	case WM_COMMAND:
	{
		switch (LOWORD(wParam)) {
		case IDC_EDIT2:
			TCHAR cBuf[SIZE];
			HGLOBAL hg;
			PTSTR	strMem;
			if (HIWORD(wParam) == EN_UPDATE) {       //      �G�f�B�b�g�{�b�N�X���ύX���ꂽ�ꍇ
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
	case WM_INITDIALOG:
		SetDlgPosCenter(hwnd);
		// WM_DROPFILES���b�Z�[�W����������悤�ɂ���
		DragAcceptFiles(hwnd, TRUE);
		// �N���b�v�{�[�h����R�s�[
		HGLOBAL hg;
		PTSTR strText, strClip;
		
		if (OpenClipboard(hwnd) && (hg = GetClipboardData(CF_UNICODETEXT))) {
			strText = (PTSTR)malloc(GlobalSize(hg));
			strClip = (PTSTR)GlobalLock(hg);
			lstrcpy(strText, strClip);
			GlobalUnlock(hg);
			//MessageBox(hwnd, strText, TEXT("�N���b�v�{�[�h�̓��e"), MB_OK);
			SetWindowText(GetDlgItem(hwnd, IDC_EDIT1), strText);
			// PMID������
			int	nValue;
			if (nValue = ::_ttoi(strText)) {
				Button_SetCheck(GetDlgItem(hwnd, IDC_RADIO1), BST_CHECKED);
			}
			else {
				Button_SetCheck(GetDlgItem(hwnd, IDC_RADIO2), BST_CHECKED);
			}
			free(strText);
			CloseClipboard();
		}
		break;

	case WM_DROPFILES:
	{
		// �h���b�O���h���b�v���Ɏ��s
		HDROP hDrop;
		UINT uFileNo;
		static TCHAR dFile[SIZE];
		HANDLE hFile;
		hDrop = (HDROP)wParam; // �h���b�v���ꂽ�t�@�C�������擾
		uFileNo = DragQueryFile((HDROP)wParam, -1, NULL, 0);
		WCHAR* dFileName;
		WCHAR* dFileNameExtension;

		if (uFileNo > 1)
			MessageBox(hwnd, TEXT("�t�@�C�����J���܂���ł���"), TEXT("�G���["), MB_OK);
		else {
			DragQueryFile(hDrop, 0, dFile, sizeof(dFile));
			hFile = CreateFile(dFile,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
			if (hFile != INVALID_HANDLE_VALUE) {
				CloseHandle(hFile);
				dFileName = PathFindFileName(dFile);
				dFileNameExtension = PathFindExtension(dFile);
				wchar_t ext[] = L".pdf";
				const wchar_t* p = wcsstr(dFileNameExtension, ext);
				// �h���b�O���h���b�v�����t�@�C����PDF���𔻒�
				if (p == NULL) {
					MessageBox(hwnd, TEXT("PDF�̂ݑI���\�ł�"), TEXT("�G���["), MB_OK);
				}
				else {
					//MessageBox(hwnd, dFileNameExtension, TEXT("�t�@�C���̊g���q"), MB_OK);
					char* dfileName = wcharToChar(dFileName); // WCHAR*�^����char*�^�ւ̕ϊ�

					// Pubmed�Ř_���������āAEsummary�̏������ƂɃt�@�C��������
					string newFileName = searchPubmedKeyword(dfileName);
					const char* nfn = newFileName.c_str();
					WCHAR* Nfn = charToWchar(nfn); // char*�^����WCHAR*�^�ւ̕ϊ�

					//HWND child = CreateWindow(TEXT("EDIT"), Nfn, WS_OVERLAPPEDWINDOW | WS_VISIBLE | ES_AUTOVSCROLL | ES_MULTILINE | WS_VSCROLL,
					//	CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);
					//if (!child) {
					//	break;
					//}
					sendClip(hwnd, Nfn);
					DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), TEXT("CHILD"), hwnd, (DLGPROC)childDlgProc);
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
					// ����{�^���������ꂽ�Ƃ��m�F���b�Z�[�W���o��
					DestroyWindow(hwnd);
					//free(szMsg);
					//free(szCaption);
				}
				break;
			}

			case IDM_FILE_EXIT:
				// �t�@�C��>�I����I����
				EndDialog(hwnd, IDM_FILE_EXIT);
				break;

			case IDM_HELP_ABOUT:
				{
				//TCHAR* hCap = (TCHAR*)malloc(SIZE);
				//TCHAR* hText = (TCHAR*)malloc(SIZE);
				TCHAR hCap[SIZE];
				TCHAR hText[SIZE];
				// �w���v>������@��I����
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
				// �t�@�C��>�J����I����
				if (getFileName(0, szFile, SIZE, _TEXT("C:\\"))) {
					// szFile: �I�������_��PDF�̃t�@�C���̃t���p�X(WCHAR*�^)
					// �I�������_��PDF�̃t�@�C�������擾
					WCHAR* szFileName;
					szFileName = PathFindFileName(szFile);
					char* szfileName = wcharToChar(szFileName); // WCHAR*�^����char*�^�ւ̕ϊ�

					// Pubmed�Ř_���������āAEsummary�̏������ƂɃt�@�C��������
					string newFileName = searchPubmedKeyword(szfileName);
					const char* nfn = newFileName.c_str();
					WCHAR* Nfn = charToWchar(nfn); // char*�^����WCHAR*�^�ւ̕ϊ�

					//HWND child = CreateWindow(TEXT("EDIT"), Nfn, WS_OVERLAPPEDWINDOW | WS_VISIBLE | ES_AUTOVSCROLL | ES_MULTILINE | WS_VSCROLL,
					//	CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);
					//if (!child) {
					//	break;
					//}
					sendClip(hwnd, Nfn);
					DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), TEXT("CHILD"), hwnd, (DLGPROC)childDlgProc);
				}
				break;
			}

			case IDOK:
			{
				//TCHAR* szBuf = (TCHAR*)malloc(SIZE);
				TCHAR szBuf[SIZE];
				// �����{�^�������s��
				iCheck = Button_GetCheck(GetDlgItem(hwnd, IDC_RADIO1));
				GetDlgItemText(hwnd, IDC_EDIT1, szBuf, (int)sizeof(szBuf));
				if (iCheck == BST_CHECKED) {
					// PMID�Ō��������s
					char* paperIdChar = wcharToChar(szBuf);
					//free(szBuf);
					string paperId = paperIdChar;
					string newFileName = searchPubmedId(paperId);
					const char* nfn = newFileName.c_str();
					WCHAR* Nfn = charToWchar(nfn); // char*�^����WCHAR*�^�ւ̕ϊ�

					/*
					HWND child = CreateWindow(TEXT("EDIT"), Nfn, WS_OVERLAPPEDWINDOW | WS_VISIBLE | ES_AUTOVSCROLL | ES_MULTILINE | WS_VSCROLL,
						CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);
					if (!child)
						break;
						*/
					sendClip(hwnd, Nfn);
					DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), TEXT("CHILD"), hwnd, (DLGPROC)childDlgProc);
				}
				else {
					// �_�����Ō��������s
					char* szfileName = wcharToChar(szBuf);
					//free(szBuf);
					string newFileName = searchPubmedKeyword(szfileName);
					const char* nfn = newFileName.c_str();
					WCHAR* Nfn = charToWchar(nfn); // char*�^����WCHAR*�^�ւ̕ϊ�

					/*
					HWND child = CreateWindow(TEXT("EDIT"), Nfn, WS_OVERLAPPEDWINDOW | WS_VISIBLE | ES_AUTOVSCROLL | ES_MULTILINE | WS_VSCROLL,
						CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);
					if (!child)
						break;
					*/
					sendClip(hwnd, Nfn);
					DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), TEXT("CHILD"), hwnd, (DLGPROC)childDlgProc);
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
