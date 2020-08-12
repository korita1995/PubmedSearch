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
�������ׂ��@�\ (2020/6/24)
1) Ctrl+A�œ��͗���S�I��
2) �L�[���[�h�����ŕ����_�����q�b�g�����ꍇ�A���ׂĂ̌������ʂ�\������
3) �_���t�@�C�����̃g���~���O���E�����E�g�Ȃǂ̓��ꕶ���ɑΉ�
4) �w���v��\�����郁�b�Z�[�W�{�b�N�X�̃T�C�Y��ύX����
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
HWND button1;
HWND button2;
HWND button3;
HWND button4;
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
		//SetDlgPosCenter(hwnd);
		loadWindowState(hwnd);

		// �G�f�B�b�g�R���g���[���̃n���h�����擾���ăO���[�o���ϐ��Ɋi�[
		hEdit1 = GetDlgItem(hwnd, IDC_EDIT1);
		hEdit3 = GetDlgItem(hwnd, IDC_EDIT3);
		button1 = GetDlgItem(hwnd, IDC_EXACT_SEARCH);
		button2 = GetDlgItem(hwnd, IDC_CROSSREF_SEARCH);
		button3 = GetDlgItem(hwnd, IDOK);
		button4 = GetDlgItem(hwnd, IDC_CLIP_SEARCH);


		// WM_DROPFILES���b�Z�[�W����������悤�ɂ���
		DragAcceptFiles(hwnd, TRUE);

		// ����ɃA�C�R���\��
		HICON hIcon;
		hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_APP), IMAGE_ICON, 16, 16, 0);
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

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
				if (checkPMID(strText)) {
					char* paperIdChar = wcharToChar(strText);
					string paperId = paperIdChar;
					try {
						//SetWindowText(hEdit3, TEXT("Pubmed��PMID���������s�J�n"));
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
					string szfileNameStr = string(szfileName);
					regex re("^[0-9]*\\..*/*");
					string newFileName;
					try {
						//SetWindowText(hEdit3, TEXT("Pubmed�ŃL�[���[�h���������s�J�n"));
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
		SetWindowText(hEdit3, TEXT(""));
		HDROP hDrop;
		UINT uFileNo;
		//static TCHAR dFile[SIZE] = TEXT("");
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
			SetWindowText(hEdit3, TEXT("�G���[: �J����PDF�t�@�C����1�܂łł�"));
		}

		// 1��PDF���h���b�O���h���b�v�������Ɏ��s
		else {
			DragQueryFile(hDrop, 0, dFile, sizeof(dFile));
			//MessageBox(hwnd, dFile, TEXT(""), MB_OK);
			hFile = CreateFile(dFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			Sleep(20);
			// �L���ȃt�@�C�����`�F�b�N���Ă�����s
			if (hFile != INVALID_HANDLE_VALUE) {
			//if (TRUE){
				//MessageBox(hwnd, TEXT("�t�@�C�����J���̂ɐ���"), TEXT(""), MB_OK);
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
						disableAllButtons();
						//SetWindowText(hEdit3, TEXT("Pubmed�ŃL�[���[�h���������s�J�n"));
						string newFileName = searchPubmedKeyword(dfileName); // Pubmed�ŃL�[���[�h����
						enableAllButtons();
						setSearchResult(hwnd, hEdit3, newFileName);
					}
					catch (...) {
						//MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK);
						//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("�G���[: ������������܂���ł���"));
						enableAllButtons();
						SetWindowText(hEdit3, TEXT("�G���[: ������������܂���ł���"));
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
	*/

	// �_�C�A���O�ɑ΂��ĉ�������̑��삪�s��ꂽ��
	case WM_COMMAND:
	{
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
					//SetWindowText(hEdit3, TEXT("Pubmed�ŃL�[���[�h���������s�J�n"));
					disableAllButtons();
					string newFileName = searchPubmedKeyword(szfileName); // Pubmed�ŃL�[���[�h����
					enableAllButtons();
					setSearchResult(hwnd, hEdit3, newFileName);
				}
				catch (...) {
					//MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK);
					//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("�G���[: ������������܂���ł���"));
					enableAllButtons();
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
		case IDC_CLIP_SEARCH:
		{
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
				if (checkPMID(strText)) {
					char* paperIdChar = wcharToChar(strText);
					string paperId = paperIdChar;
					try {
						disableAllButtons();
						//SetWindowText(hEdit3, TEXT("Pubmed��PMID���������s�J�n"));
						string newFileName = searchPubmedId(paperId); // PMID�Ō��������s
						enableAllButtons();
						setSearchResult(hwnd, hEdit3, newFileName);
					}
					catch (...) {
						//MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK);
						//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("�G���[: ������������܂���ł���"));
						enableAllButtons();
						SetWindowText(hEdit3, TEXT("�G���[: ������������܂���ł���"));
					}

				}
				else {
					// �_�����Ō��������s
					char* szfileName = wcharToChar(strText);
					string szfileNameStr = string(szfileName);
					regex re("^[0-9]*\\..*/*");
					string newFileName;
					try {
						//SetWindowText(hEdit3, TEXT("Pubmed�ŃL�[���[�h���������s�J�n"));
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
						//MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK);
						//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("�G���[: ������������܂���ł���"));
						enableAllButtons();
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
				if (checkPMID(szBuf)){
					// PMID�Ō��������s
					char* paperIdChar = wcharToChar(szBuf);
					string paperId = string(paperIdChar);
					try {
						//SetWindowText(hEdit3, TEXT("Pubmed��PMID���������s�J�n"));
						disableAllButtons();
						string newFileName = searchPubmedId(paperId);
						enableAllButtons();
						setSearchResult(hwnd, hEdit3, newFileName);
					}
					catch (...) {
						//MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK);
						//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("�G���[: ������������܂���ł���"));
						enableAllButtons();
						SetWindowText(hEdit3, TEXT("�G���[: ������������܂���ł���"));
					}

				}
				else {
					// �_�����Ō��������s
					char* szfileName = wcharToChar(szBuf);
					string szfileNameStr = string(szfileName);
					regex re("^[0-9]*\\..*/*");
					string newFileName;
					try {
						//SetWindowText(hEdit3, TEXT("Pubmed�ŃL�[���[�h���������s�J�n"));
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
							SetWindowText(hEdit3, TEXT("�G���[: ������������܂���ł���"));
						}
						else {
							enableAllButtons();
							setSearchResult(hwnd, hEdit3, newFileName);
						}
							
					}
					catch (...) {
						//MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK);
						//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("�G���[: ������������܂���ł���"));
						enableAllButtons();
						SetWindowText(hEdit3, TEXT("�G���[: ������������܂���ł���"));
					}

				}
			}
			break;
		}

		case IDC_EXACT_SEARCH:
		{
			SetWindowText(hEdit3, TEXT(""));
			TCHAR szBuf[SIZE];
			// �����{�^�������s��
			GetDlgItemText(hwnd, IDC_EDIT1, szBuf, (int)sizeof(szBuf));
			if (isStrSpace(szBuf)) {
				SetWindowText(hEdit3, TEXT("�G���[: PMID�������͘_�����i�L�[���[�h�j����͂��Ă�������"));
			}
			else {
				char* szfileName = wcharToChar(szBuf);
				string szfileNameStr = string(szfileName);
				regex re("^[0-9]*\\..*/*");
				string newFileName;
				try {
					//SetWindowText(hEdit3, TEXT("Pubmed�ŃL�[���[�h�����x���������s�J�n"));
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
						SetWindowText(hEdit3, TEXT("�G���[: ������������܂���ł���"));
					}
					else {
						enableAllButtons();
						setSearchResult(hwnd, hEdit3, newFileName);
					}
				}
				catch (...) {
					//MessageBox(hwnd, TEXT("���̌������ʂ͖����ł�"), TEXT("�G���["), MB_OK);
					//SetWindowText(GetDlgItem(hwnd, IDC_STATIC3), TEXT("�G���[: ������������܂���ł���"));
					enableAllButtons();
					SetWindowText(hEdit3, TEXT("�G���[: ������������܂���ł���"));
				}
			}
			break;
		}

		case IDC_CROSSREF_SEARCH:
		{
			SetWindowText(hEdit3, TEXT(""));
			TCHAR dzBuf[SIZE];
			// �����{�^�������s��
			GetDlgItemText(hwnd, IDC_EDIT1, dzBuf, (int)sizeof(dzBuf));
			if (isStrSpace(dzBuf)) {
				SetWindowText(hEdit3, TEXT("�G���[: DOI�������͘_�����i�L�[���[�h�j����͂��Ă�������"));
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
						//SetWindowText(hEdit3, TEXT("Crossref��DOI���������s�J�n"));
						string newFileName = searchCrossrefDoi(dzfileNameStr.c_str());
						enableAllButtons();
						setSearchResult(hwnd, hEdit3, newFileName);
					}
					else {
						//SetWindowText(hEdit3, TEXT("Crossref�ŃL�[���[�h���������s�J�n"));
						string newFileName = searchCrossrefKeyword(dzfileNameStr.c_str());
						enableAllButtons();
						setSearchResult(hwnd, hEdit3, newFileName);
					}
				}
				catch (...) {
					enableAllButtons();
					SetWindowText(hEdit3, TEXT("�G���[: ������������܂���ł���"));
				}

			}
			break;
		}

		case IDC_EDIT3:
		{
			TCHAR cBuf[SIZE];
			// �G�f�B�b�g�{�b�N�X���ύX���ꂽ�ꍇ
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
		hKey,	// ���݃I�[�v�����Ă���L�[�̃n���h��
		TEXT("left"),	// �l�́u���O�v��������������ւ̃|�C���^
		0,	// �\��p�����[�^�B0���w�肷��
		REG_SZ,	// �l�́u��ށv���w�肷��BNULL�ŏI��镶�����REG_SZ�A32�r�b�g�l��REG_DWORD
		(CONST BYTE*)(LPCTSTR)strNum,	// �i�[����l�́u�f�[�^�v���������o�b�t�@�ւ̃|�C���^
		(int)sizeof(strNum)		// data�̃T�C�Y���w�肷��
	);

	_ltow_s(pRect->top, strNum, 10);
	result = RegSetValueEx(
		hKey,	// ���݃I�[�v�����Ă���L�[�̃n���h��
		TEXT("top"),	// �l�́u���O�v��������������ւ̃|�C���^
		0,	// �\��p�����[�^�B0���w�肷��
		REG_SZ,	// �l�́u��ށv���w�肷��BNULL�ŏI��镶�����REG_SZ�A32�r�b�g�l��REG_DWORD
		(CONST BYTE*)(LPCTSTR)strNum,	// �i�[����l�́u�f�[�^�v���������o�b�t�@�ւ̃|�C���^
		(int)sizeof(strNum)		// data�̃T�C�Y���w�肷��
	);

	_ltow_s(pRect->right, strNum, 10);
	result = RegSetValueEx(
		hKey,	// ���݃I�[�v�����Ă���L�[�̃n���h��
		TEXT("right"),	// �l�́u���O�v��������������ւ̃|�C���^
		0,	// �\��p�����[�^�B0���w�肷��
		REG_SZ,	// �l�́u��ށv���w�肷��BNULL�ŏI��镶�����REG_SZ�A32�r�b�g�l��REG_DWORD
		(CONST BYTE*)(LPCTSTR)strNum,	// �i�[����l�́u�f�[�^�v���������o�b�t�@�ւ̃|�C���^
		(int)sizeof(strNum)		// data�̃T�C�Y���w�肷��
	);

	_ltow_s(pRect->bottom, strNum, 10);
	result = RegSetValueEx(
		hKey,	// ���݃I�[�v�����Ă���L�[�̃n���h��
		TEXT("bottom"),	// �l�́u���O�v��������������ւ̃|�C���^
		0,	// �\��p�����[�^�B0���w�肷��
		REG_SZ,	// �l�́u��ށv���w�肷��BNULL�ŏI��镶�����REG_SZ�A32�r�b�g�l��REG_DWORD
		(CONST BYTE*)(LPCTSTR)strNum,	// �i�[����l�́u�f�[�^�v���������o�b�t�@�ւ̃|�C���^
		(int)sizeof(strNum)		// data�̃T�C�Y���w�肷��
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
		//Windows NT/2000�FHKEY_PERFORMANCE_DATA ���w��ł��܂��B
		//Windows 95/98�FHKEY_DYN_DATA ���w��ł��܂��B
		TEXT("Yakusaku\\SearchPubmed2FileName\\Position"),//�L�[
		0,//�\��
		NULL,//�w��
		REG_OPTION_VOLATILE, //�V�X�e�����ċN������Ə����������
		//REG_OPTION_NON_VOLATILE,//�s������
		KEY_ALL_ACCESS,//�W���A�N�Z�X���̂��ׂĂ̌�����g�ݍ��킹������
		NULL,
		&hKey,
		&dwDisposition);
	//result �� ERROR_SUCCESS �ł���ΐ����ł���D

	saveRect(&rcWnd, hKey);

	//�L�[�����
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
	DWORD dwType;		// �l�̎�ނ��󂯎��
	DWORD dwSize;		// �f�[�^�̃T�C�Y���󂯎��

	result = RegQueryValueEx(
		hKey,	// ���݃I�[�v�����Ă���L�[�̃n���h��
		query,	// �擾����l�́u���O�v��������������ւ̃|�C���^
		NULL,	// �\��p�����[�^�BNULL���w�肷��
		&dwType,	// �l�́u��ށv���󂯎��
		NULL,		// �l�́u�f�[�^�v���󂯎��BNULL���w�肷�邱�Ƃ��\�����A�f�[�^�͎󂯎��Ȃ�
		&dwSize		// �I�[����'\0'���܂�Data�̃T�C�Y���擾����
	);
	//���ۂɃf�[�^���擾�i�T�C�Y�̎w�肪�������Ȃ��Ǝ��s���邱�Ƃ�����j
	result = RegQueryValueEx(
		hKey,	// ���݃I�[�v�����Ă���L�[�̃n���h��
		query,	// �擾����l�́u���O�v��������������ւ̃|�C���^
		NULL,	// �\��p�����[�^�BNULL���w�肷��
		&dwType,	// �l�́u��ށv���󂯎��
		(LPBYTE)(LPCTSTR)&data,	// �l�́u�f�[�^�v���󂯎��BNULL���w�肷�邱�Ƃ��\�����A�f�[�^�͎󂯎��Ȃ�
		&dwSize		// Data�̃T�C�Y���w�肷��
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
		//Windows NT/2000�FHKEY_PERFORMANCE_DATA ���w��ł��܂��B
		//Windows 95/98�FHKEY_DYN_DATA ���w��ł��܂��B
		TEXT("Yakusaku\\SearchPubmed2FileName\\Position"),//�L�[
		0,//�\��
		NULL,//�w��
		REG_OPTION_VOLATILE, //�V�X�e�����ċN������Ə����������
		//REG_OPTION_NON_VOLATILE,//�s������
		KEY_ALL_ACCESS,//�W���A�N�Z�X���̂��ׂĂ̌�����g�ݍ��킹������
		NULL,
		&hKey,
		&dwDisposition);
	//result �� ERROR_SUCCESS �ł���ΐ����ł���D

	loadRect(&rcWnd, hKey);

	// �Ώۃ��j�^�̏����擾
	HMONITOR hMonitor = MonitorFromRect(
		&rcWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(hMonitor, &mi);

	// �ʒu�␳
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
	// �E�B���h�E�ʒu����
	
	if (!((rcWnd.left == 0) & (rcWnd.top == 0) & (rcWnd.right == 0) & (rcWnd.bottom == 0))) {
		SetWindowPos(
			hwnd, NULL, rcWnd.left, rcWnd.top,
			rcWnd.right - rcWnd.left, rcWnd.bottom - rcWnd.top,
			SWP_NOZORDER);
	}
	else {
		SetDlgPosCenter(hwnd);
	}

	//�L�[�����
	RegCloseKey(hKey);
}