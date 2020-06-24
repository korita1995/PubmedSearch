
#include "Resource.h"
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <tchar.h>
#include <shlwapi.h>
#include "paperLabeling.h"
//#include "mainApp.h"

/*
�������ׂ��@�\ (2020/6/24)
1) Ctrl+A�œ��͗���S�I��
2) �L�[���[�h�����ŕ����_�����q�b�g�����ꍇ�A���ׂĂ̌������ʂ�\������
*/

#define HANDLE_DLG_MSG(hwnd, msg, fn) \
    case(msg): \
        return SetDlgMsgResult(hwnd, msg, HANDLE_##msg(hwnd, wParam, lParam,fn))
#define GetMonitorRect(rc)  SystemParametersInfo(SPI_GETWORKAREA,0,rc,0)
#define SIZE 1000
#define VK_A 0x41

using namespace std;

// �v���g�^�C�v�錾
LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL SetDlgPosCenter(HWND);
void sendClip(HWND, TCHAR*);
LRESULT CALLBACK childDlgProc(HWND, UINT, WPARAM, LPARAM);
bool IsStrSpace(TCHAR*);

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
			// �G�f�B�b�g�{�b�N�X���ύX���ꂽ�ꍇ
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
	// �_�C�A���O�{�b�N�X�̐�����
	case WM_INITDIALOG:
		// �_�C�A���O�̈ʒu�𒆉��Ɉړ�
		SetDlgPosCenter(hwnd);

		// WM_DROPFILES���b�Z�[�W����������悤�ɂ���
		DragAcceptFiles(hwnd, TRUE);

		// �N���b�v�{�[�h���R�s�[���ē��͗��Ƀy�[�X�g
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

		// ����ɃA�C�R���\��
		HICON hIcon;
		hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APP), IMAGE_ICON, 16, 16, 0);
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

		break;

	// PDF���h���b�O���h���b�v���Ɏ��s
	case WM_DROPFILES:
	{
		HDROP hDrop;
		UINT uFileNo;
		static TCHAR dFile[SIZE];
		HANDLE hFile;
		hDrop = (HDROP)wParam; // �h���b�v���ꂽ�t�@�C����
		uFileNo = DragQueryFile((HDROP)wParam, -1, NULL, 0);
		WCHAR* dFileName;
		WCHAR* dFileNameExtension;

		// ������PDF���h���b�O���h���b�v�������i�G���[�j
		if (uFileNo > 1)
			MessageBox(hwnd, TEXT("�t�@�C�����J���܂���ł���"), TEXT("�G���["), MB_OK);

		// 1��PDF���h���b�O���h���b�v�������Ɏ��s
		else {
			DragQueryFile(hDrop, 0, dFile, sizeof(dFile));
			hFile = CreateFile(dFile,GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
			// �L���ȃt�@�C�����`�F�b�N���Ă�����s
			if (hFile != INVALID_HANDLE_VALUE) {
				CloseHandle(hFile);
				dFileName = PathFindFileName(dFile); // �t�@�C����
				dFileNameExtension = PathFindExtension(dFile); // .���܂߂��t�@�C���̊g���q
				wchar_t ext[] = L".pdf";
				wchar_t Ext[] = L".PDF";
				const wchar_t* p = wcsstr(dFileNameExtension, ext);
				const wchar_t* P = wcsstr(dFileNameExtension, Ext);
				// �h���b�O���h���b�v�����t�@�C���̊g���q��.pdf��������.PDF���𔻒�
				if ((p == NULL)&(P == NULL))
					MessageBox(hwnd, TEXT("PDF�̂ݑI���\�ł�"), TEXT("�G���["), MB_OK);
				else {
					char* dfileName = wcharToChar(dFileName); // WCHAR*�^����char*�^�ւ̕ϊ�
					string newFileName = searchPubmedKeyword(dfileName); // Pubmed�ŃL�[���[�h����
					const char* nfn = newFileName.c_str(); // string�^����const char*�^�ւ̕ϊ�
					WCHAR* Nfn = charToWchar(nfn); // char*�^����WCHAR*�^�ւ̕ϊ�
					sendClip(hwnd, Nfn); // �N���b�v�{�[�h�Ɍ������ʂ�]��
					//DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), TEXT("CHILD"), hwnd, (DLGPROC)childDlgProc);
					SetWindowText(GetDlgItem(hwnd, IDC_EDIT3), Nfn); // �A�v�������̃G�f�B�b�g�R���g���[���Ɍ������ʂ��y�[�X�g
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

	// �V�X�e���L�[�ȊO�̃L�[�{�[�h�������ꂽ��
	case WM_KEYDOWN:
		// Ctrl+A���������Ƃ��Ɏ��s�i���܂������ĂȂ�����must fix�j
		if (LOWORD(wParam) == VK_A) {
			if (GetKeyState(VK_CONTROL) < 0) {
				int nCount;
				nCount = GetWindowTextLength(hwnd);
				SendMessage(GetDlgItem(hwnd, IDC_EDIT1), EM_SETSEL, 0, -1);
			}
		}
		break;

	// �_�C�A���O�ɑ΂��ĉ�������̑��삪�s��ꂽ��
	case WM_COMMAND:
		{
			INT iCheck;
			UINT wmId;
			wmId = LOWORD(wParam);

			switch (wmId) {
			// �t�@�C��>�I����I����
			case IDM_FILE_EXIT:
				
				EndDialog(hwnd, IDM_FILE_EXIT);
				break;

			// �w���v>������@��I����
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

			// �t�@�C��>�J����I����
			case IDM_FILE_OPEN:
			{
				TCHAR szFile[SIZE];
				// �I�������_��PDF�̃t�@�C���̃t���p�X���擾���ăo�b�t�@�[�Ɋi�[
				if (getFileName(0, szFile, SIZE, _TEXT("C:\\"))) {
					WCHAR* szFileName;
					szFileName = PathFindFileName(szFile); // �t�@�C���̃t���p�X����t�@�C�����݂̂��擾
					char* szfileName = wcharToChar(szFileName); // WCHAR*�^����char*�^�ւ̕ϊ�
					string newFileName = searchPubmedKeyword(szfileName); // Pubmed�ŃL�[���[�h����
					const char* nfn = newFileName.c_str();
					WCHAR* Nfn = charToWchar(nfn); // char*�^����WCHAR*�^�ւ̕ϊ�
					sendClip(hwnd, Nfn); // �N���b�v�{�[�h�Ɍ������ʂ�]��
					//DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), TEXT("CHILD"), hwnd, (DLGPROC)childDlgProc);
					SetWindowText(GetDlgItem(hwnd, IDC_EDIT3), Nfn); // �A�v�������̃G�f�B�b�g�R���g���[���Ɍ������ʂ��y�[�X�g
				}
				break;
			}

			// �u���͗����폜�v�{�^������������
			case IDC_DEL:
				SendMessage(GetDlgItem(hwnd, IDC_EDIT1), EM_SETSEL, 0, -1);
				SendMessage(GetDlgItem(hwnd, IDC_EDIT1), WM_CLEAR, 0, 0);
				break;

			// �u�N���b�v�{�[�h���猟���v�{�^������������
			case IDC_SEARCH:
			{
				HGLOBAL hg;
				PTSTR strText, strClip;
				// �N���b�v�{�[�h���R�s�[����Pubmed����
				if (OpenClipboard(hwnd) && (hg = GetClipboardData(CF_UNICODETEXT))) {
					strText = (PTSTR)malloc(GlobalSize(hg));
					strClip = (PTSTR)GlobalLock(hg);
					lstrcpy(strText, strClip);
					GlobalUnlock(hg);

					// �N���b�v�{�[�h�̓��e�������iPMID�j�������ꍇ
					int	nValue;
					if (nValue = ::_ttoi(strText)) {
						char* paperIdChar = wcharToChar(strText);
						string paperId = paperIdChar;
						try {
							string newFileName = searchPubmedId(paperId); // PMID�Ō��������s
							const char* nfn = newFileName.c_str();
							WCHAR* Nfn = charToWchar(nfn); // char*�^����WCHAR*�^�ւ̕ϊ�
							sendClip(hwnd, Nfn);
							//DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), TEXT("CHILD"), hwnd, (DLGPROC)childDlgProc);
							SetWindowText(GetDlgItem(hwnd, IDC_EDIT3), Nfn);
						}
						catch (...) { MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK); }

					}
					else {
						// �_�����Ō��������s
						char* szfileName = wcharToChar(strText);
						try {
							string newFileName = searchPubmedKeyword(szfileName);
							const char* nfn = newFileName.c_str();
							WCHAR* Nfn = charToWchar(nfn); // char*�^����WCHAR*�^�ւ̕ϊ�
							sendClip(hwnd, Nfn);
							//DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), TEXT("CHILD"), hwnd, (DLGPROC)childDlgProc);
							SetWindowText(GetDlgItem(hwnd, IDC_EDIT3), Nfn);
						}
						catch (...) { MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK); }

					}

					free(strText);
					CloseClipboard();
				}
				break;
			}

			// �u���͗����猟���v�{�^������������
			case IDOK:
			{
				TCHAR szBuf[SIZE];
				// �����{�^�������s��
				GetDlgItemText(hwnd, IDC_EDIT1, szBuf, (int)sizeof(szBuf));
				if (IsStrSpace(szBuf))
					MessageBox(hwnd, TEXT("PMID�������͘_�����i�L�[���[�h�j����͂��Ă�������"), TEXT("�G���["), MB_OK);
				else {
					int	nValue;
					if (nValue = ::_ttoi(szBuf)) {
						// PMID�Ō��������s
						char* paperIdChar = wcharToChar(szBuf);
						string paperId = paperIdChar;
						try {
							string newFileName = searchPubmedId(paperId);
							const char* nfn = newFileName.c_str();
							WCHAR* Nfn = charToWchar(nfn); // char*�^����WCHAR*�^�ւ̕ϊ�
							sendClip(hwnd, Nfn);
							//DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), TEXT("CHILD"), hwnd, (DLGPROC)childDlgProc);
							SetWindowText(GetDlgItem(hwnd, IDC_EDIT3), Nfn);
						}
						catch (...){ MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK); }
						
					}
					else {
						// �_�����Ō��������s
						char* szfileName = wcharToChar(szBuf);
						try {
							string newFileName = searchPubmedKeyword(szfileName);
							const char* nfn = newFileName.c_str();
							WCHAR* Nfn = charToWchar(nfn); // char*�^����WCHAR*�^�ւ̕ϊ�
							sendClip(hwnd, Nfn);
							//DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), TEXT("CHILD"), hwnd, (DLGPROC)childDlgProc);
							SetWindowText(GetDlgItem(hwnd, IDC_EDIT3), Nfn);
						}
						catch (...){ MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK); }
						
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

// ���p�󔒕���,�S�p�󔒕����̃`�F�b�N
bool IsStrSpace(TCHAR* str) {
	StrTrim(str, L" ");
	const size_t textSize = 256;
	char lpcText[textSize];
	WideCharToMultiByte(CP_ACP, 0, str, -1, lpcText, textSize, NULL, NULL);
	char* pPoint = lpcText;
	bool bSpace = TRUE; // �����񂪁w�S�p�X�y�[�X�x���́w���p�X�y�[�X�x�݂̂ō\������Ă��邩�ǂ���
	while (*pPoint != '\0') {
		if (!memcmp(pPoint, "�@", 2)) {
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