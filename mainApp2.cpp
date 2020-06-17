
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
1) PDF���h���b�O���h���b�v�Ō����@�\�̒ǉ�
�i�h���b�O���h���b�v�Ńt�@�C���p�X���擾�j
2) Title��PMID���̔���@�\�̒ǉ�
�i���͓��e���������𔻒�j
3) �E�B���h�E�N�����ɁA�N���b�v�{�[�h�̓��e�������񂩐���������
4) 3)�̔��莟��ŁA�_������PMID�̂ǂ��炩�̃��W�I�{�^����I�������͗��̏����l�Ƃ���
5) �ϊ���̃t�@�C�����������ŃN���b�v�{�[�h�ɃR�s�[
6) �ϊ���̃t�@�C������ҏW�����ꍇ�A���Ƃ���N���b�v�{�[�h�ɃR�s�[�\�ɂ���
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

					HWND child = CreateWindow(TEXT("EDIT"), Nfn, WS_OVERLAPPEDWINDOW | WS_VISIBLE | ES_AUTOVSCROLL | ES_MULTILINE | WS_VSCROLL,
						CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), NULL);
					if (!child)
						break;
				}
				else {
					// �_�����Ō��������s
					char* szfileName = wcharToChar(szBuf);
					//free(szBuf);
					string newFileName = searchPubmedKeyword(szfileName);
					const char* nfn = newFileName.c_str();
					WCHAR* Nfn = charToWchar(nfn); // char*�^����WCHAR*�^�ւ̕ϊ�

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
