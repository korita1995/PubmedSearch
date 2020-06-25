#ifndef INCLUDE_PUBMED_SEARCH_APP_H
#define INCLUDE_PUBMED_SEARCH_APP_H

#include "Resource.h"
#include <windows.h>
#include <windowsx.h>
#include <string>

using namespace std;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK childDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Edit1Proc(HWND, UINT, WPARAM, LPARAM);
bool isStrSpace(TCHAR*);
void sendClip(HWND, TCHAR*);
BOOL SetDlgPosCenter(HWND);
void setSearchResult(HWND, HWND, string);

#endif