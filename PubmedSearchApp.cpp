#include "Resource.h"
#include <windows.h>
#include <windowsx.h>
#include <string>
#include <tchar.h>
#include <shlwapi.h>
#include "PubmedSearchUtils.h"
#include "PubmedSearchApp.h"

/*
�������ׂ��@�\ (2020/6/24)
1) Ctrl+A�œ��͗���S�I��
2) �L�[���[�h�����ŕ����_�����q�b�g�����ꍇ�A���ׂĂ̌������ʂ�\������
3) �_���t�@�C�����̃g���~���O���E�����E�g�Ȃǂ̓��ꕶ���ɑΉ�
*/

#define GetMonitorRect(rc)  SystemParametersInfo(SPI_GETWORKAREA,0,rc,0)
#define SIZE 2000
#define VK_A 0x41

using namespace std;

// �O���[�o���ϐ�:
HINSTANCE hInst;	// ���݂̃C���^�[�t�F�C�X
HWND hDlgCurrent;	// �_�C�A���O�̃n���h��
HWND hEdit1;	// ���͗��̃n���h��
HWND hEdit3;	// �������ʗ��̃n���h��
TCHAR* commandLineArg;
DLGPROC OrghEdit1, OrghEdit3;   //�I���W�i���v���V�[�W���̃A�h���X

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) {
	hInst = hInstance; // �O���[�o���ϐ��ɃC���X�^���X���i�[
	commandLineArg = GetCommandLine();

	// ���[�h���X�_�C�A���O�{�b�N�X�̍쐬����уn���h���擾
	hDlgCurrent = CreateDialog(hInst, TEXT("DLG"), NULL, (DLGPROC)DlgProc);
	ShowWindow(hDlgCurrent, nCmdShow);	// �_�C�A���O�\��

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
		// �_�C�A���O�{�b�N�X�̐�����
	case WM_INITDIALOG:
	{
		// �_�C�A���O�̈ʒu�𒆉��Ɉړ�
		SetDlgPosCenter(hwnd);

		// �G�f�B�b�g�R���g���[���̃n���h�����擾���ăO���[�o���ϐ��Ɋi�[
		hEdit1 = GetDlgItem(hwnd, IDC_EDIT1);
		hEdit3 = GetDlgItem(hwnd, IDC_EDIT3);

		// WM_DROPFILES���b�Z�[�W����������悤�ɂ���
		DragAcceptFiles(hwnd, TRUE);

		// ����ɃA�C�R���\��
		HICON hIcon;
		hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_APP), IMAGE_ICON, 16, 16, 0);
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

		//�E�B���h�E�̃T�u�N���X��
		OrghEdit1 = (DLGPROC)GetWindowLongPtr(hEdit1, DWLP_DLGPROC);
		SetWindowLongPtr(hEdit1, DWLP_DLGPROC, (LONG)Edit1Proc);

		HGLOBAL hg;
		PTSTR strText, strClip;

		// �A�v���N������PDF�t�@�C�����h���b�O���h���b�v���ꂽ���`�F�b�N
		TCHAR* CommandLineArg = PathFindFileName(commandLineArg);
		WCHAR* CommandLineArgExtension = PathFindExtension(commandLineArg); // �t�@�C���g���q
		wchar_t extp[] = L".pdf";
		wchar_t extP[] = L".PDF";
		const wchar_t* pd = wcsstr(CommandLineArgExtension, extp);
		const wchar_t* Pd = wcsstr(CommandLineArgExtension, extP);
		// �h���b�O���h���b�v�����t�@�C���̊g���q��.pdf��������.PDF���𔻒�
		if ((pd == NULL) & (Pd == NULL)) {
			// .pdf��������.PDF�t�@�C�����h���b�O���h���b�v����Ȃ������Ƃ��i��.exe�t�@�C����ʏ�N���������j
			// �N���b�v�{�[�h���R�s�[���ē��͗��Ƀy�[�X�g
			if (OpenClipboard(hwnd) && (hg = GetClipboardData(CF_UNICODETEXT))) {
				strText = (PTSTR)malloc(GlobalSize(hg));
				strClip = (PTSTR)GlobalLock(hg);
				lstrcpy(strText, strClip);
				GlobalUnlock(hg);
				SetWindowText(hEdit1, strText);
				// �N���b�v�{�[�h�̓��e�������iPMID�j�������ꍇ
				int	nValue;
				if (nValue = ::_ttoi(strText)) {
					char* paperIdChar = wcharToChar(strText);
					string paperId = paperIdChar;
					try {
						string newFileName = searchPubmedId(paperId); // PMID�Ō��������s
						setSearchResult(hwnd, hEdit3, newFileName);
					}
					catch (...) {
						//MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK);
						//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT(""));
						SetWindowText(hEdit3, TEXT(""));
					}

				}
				else {
					// �_�����Ō��������s
					char* szfileName = wcharToChar(strText);
					try {
						string newFileName = searchPubmedKeyword(szfileName);
						setSearchResult(hwnd, hEdit3, newFileName);
					}
					catch (...) {
						//MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK);
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
				string newFileName = searchPubmedKeyword(cfileName); // Pubmed�ŃL�[���[�h����
				setSearchResult(hwnd, hEdit3, newFileName);
			}
			catch (...) {
				//MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK);
				//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT(""));
				SetWindowText(hEdit3, TEXT(""));
			}
		}
		break;
	}

	// PDF���h���b�O���h���b�v���Ɏ��s
	case WM_DROPFILES:
	{
		SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT(""));
		SetWindowText(hEdit3, TEXT(""));
		HDROP hDrop;
		UINT uFileNo;
		static TCHAR dFile[SIZE];
		HANDLE hFile;
		hDrop = (HDROP)wParam; // �h���b�v���ꂽ�t�@�C����
		uFileNo = DragQueryFile((HDROP)wParam, -1, NULL, 0);
		WCHAR* dFileName;
		WCHAR* dFileNameExtension;

		// ������PDF���h���b�O���h���b�v�������i�G���[�j
		if (uFileNo > 1) {
			//MessageBox(hwnd, TEXT("�t�@�C�����J���܂���ł���"), TEXT("�G���["), MB_OK);
			//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("�G���[: �t�@�C�����J���܂���ł���"));
			SetWindowText(hEdit3, TEXT("�G���[: �t�@�C�����J���܂���ł���"));
		}

		// 1��PDF���h���b�O���h���b�v�������Ɏ��s
		else {
			DragQueryFile(hDrop, 0, dFile, sizeof(dFile));
			hFile = CreateFile(dFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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
				if ((p == NULL) & (P == NULL)) {
					//MessageBox(hwnd, TEXT("PDF�̂ݑI���\�ł�"), TEXT("�G���["), MB_OK);
					//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("�G���[: PDF�̂ݑI���\�ł�"));
					SetWindowText(hEdit3, TEXT("�G���[: PDF�̂ݑI���\�ł�"));
				}
				else {
					PathRemoveExtension(dFileName);
					SetWindowText(hEdit1, dFileName);
					char* dfileName = wcharToChar(dFileName); // WCHAR*�^����char*�^�ւ̕ϊ�
					try {
						string newFileName = searchPubmedKeyword(dfileName); // Pubmed�ŃL�[���[�h����
						setSearchResult(hwnd, hEdit3, newFileName);
					}
					catch (...) {
						//MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK);
						//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("�G���[: ������������܂���ł���"));
						SetWindowText(hEdit3, TEXT("�G���[: ������������܂���ł���"));
					}

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
				SetFocus(hEdit1);
				SendDlgItemMessage(hwnd, IDC_EDIT1, EM_SETSEL, 0, -1);
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
			LoadString(hInst, HELP_CAPTION, hCap, SIZE);
			LoadString(hInst, HELP_TEXT, hText, SIZE);
			if (IDOK == ::MessageBox(NULL, hText, hCap, MB_OK)) {
				break;
			}
			break;
		}

		// �t�@�C��>�J����I����
		case IDM_FILE_OPEN:
		{
			SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT(""));
			SetWindowText(hEdit3, TEXT(""));
			TCHAR szFile[SIZE];
			// �I�������_��PDF�̃t�@�C���̃t���p�X���擾���ăo�b�t�@�[�Ɋi�[
			if (getFileName(0, szFile, SIZE, _TEXT("C:\\"))) {
				WCHAR* szFileName;
				szFileName = PathFindFileName(szFile); // �t�@�C���̃t���p�X����t�@�C�����݂̂��擾
				char* szfileName = wcharToChar(szFileName); // WCHAR*�^����char*�^�ւ̕ϊ�
				PathRemoveExtension(szFileName);
				SetWindowText(hEdit1, szFileName);
				try {
					string newFileName = searchPubmedKeyword(szfileName); // Pubmed�ŃL�[���[�h����
					setSearchResult(hwnd, hEdit3, newFileName);
				}
				catch (...) {
					//MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK);
					//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("�G���[: ������������܂���ł���"));
					SetWindowText(hEdit3, TEXT("�G���[: ������������܂���ł���"));
				}

			}
			break;
		}

		// �u���͗����폜�v�{�^������������
		case IDC_DEL:
			SendMessage(hEdit1, EM_SETSEL, 0, -1);
			SendMessage(hEdit1, WM_CLEAR, 0, 0);
			break;

			// �u�N���b�v�{�[�h���猟���v�{�^������������
		case IDC_SEARCH:
		{
			SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT(""));
			SetWindowText(hEdit3, TEXT(""));
			HGLOBAL hg;
			PTSTR strText, strClip;
			// �N���b�v�{�[�h���R�s�[����Pubmed����
			if (OpenClipboard(hwnd) && (hg = GetClipboardData(CF_UNICODETEXT))) {
				strText = (PTSTR)malloc(GlobalSize(hg));
				strClip = (PTSTR)GlobalLock(hg);
				lstrcpy(strText, strClip);
				GlobalUnlock(hg);
				SetWindowText(hEdit1, strText);

				// �N���b�v�{�[�h�̓��e�������iPMID�j�������ꍇ
				int	nValue;
				if (nValue = ::_ttoi(strText)) {
					char* paperIdChar = wcharToChar(strText);
					string paperId = paperIdChar;
					try {
						string newFileName = searchPubmedId(paperId); // PMID�Ō��������s
						setSearchResult(hwnd, hEdit3, newFileName);
					}
					catch (...) {
						//MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK);
						//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("�G���[: ������������܂���ł���"));
						SetWindowText(hEdit3, TEXT("�G���[: ������������܂���ł���"));
					}

				}
				else {
					// �_�����Ō��������s
					char* szfileName = wcharToChar(strText);
					try {
						string newFileName = searchPubmedKeyword(szfileName);
						setSearchResult(hwnd, hEdit3, newFileName);
					}
					catch (...) {
						//MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK);
						//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("�G���[: ������������܂���ł���"));
						SetWindowText(hEdit3, TEXT("�G���[: ������������܂���ł���"));
					}

				}

				free(strText);
				CloseClipboard();
			}
			break;
		}

		// �u���͗����猟���v�{�^������������
		case IDOK:
		{
			SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT(""));
			SetWindowText(hEdit3, TEXT(""));
			TCHAR szBuf[SIZE];
			// �����{�^�������s��
			GetDlgItemText(hwnd, IDC_EDIT1, szBuf, (int)sizeof(szBuf));
			if (isStrSpace(szBuf)) {
				//MessageBox(hwnd, TEXT("PMID�������͘_�����i�L�[���[�h�j����͂��Ă�������"), TEXT("�G���["), MB_OK);
				//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("�G���[: PMID�������͘_�����i�L�[���[�h�j����͂��Ă�������"));
				SetWindowText(hEdit3, TEXT("�G���[: PMID�������͘_�����i�L�[���[�h�j����͂��Ă�������"));
			}

			else {
				int	nValue;
				if (nValue = ::_ttoi(szBuf)) {
					// PMID�Ō��������s
					char* paperIdChar = wcharToChar(szBuf);
					string paperId = paperIdChar;
					try {
						string newFileName = searchPubmedId(paperId);
						setSearchResult(hwnd, hEdit3, newFileName);
					}
					catch (...) {
						//MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK);
						//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("�G���[: ������������܂���ł���"));
						SetWindowText(hEdit3, TEXT("�G���[: ������������܂���ł���"));
					}

				}
				else {
					// �_�����Ō��������s
					char* szfileName = wcharToChar(szBuf);
					try {
						string newFileName = searchPubmedKeyword(szfileName);
						setSearchResult(hwnd, hEdit3, newFileName);
					}
					catch (...) {
						//MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK);
						//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("�G���[: ������������܂���ł���"));
						SetWindowText(hEdit3, TEXT("�G���[: ������������܂���ł���"));
					}

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


LRESULT CALLBACK Edit1Proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (msg == WM_CHAR && wParam == 1) {
		SendMessage(hEdit1, EM_SETSEL, 0, -1);
		return 1;
	}
	else
		return CallWindowProc(OrghEdit1, hEdit1, msg, wParam, lParam);
}

// ���p�󔒕���,�S�p�󔒕����̃`�F�b�N
bool isStrSpace(TCHAR* str) {
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

void setSearchResult(HWND hwnd, HWND hEdit, string newFileName) {
	const char* nfn = newFileName.c_str();
	WCHAR* Nfn = charToWchar(nfn); // char*�^����WCHAR*�^�ւ̕ϊ�
	sendClip(hwnd, Nfn);
	//DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), TEXT("CHILD"), hwnd, (DLGPROC)childDlgProc);
	SetWindowText(hEdit, Nfn);
}





