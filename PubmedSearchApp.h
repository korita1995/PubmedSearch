#ifndef INCLUDE_MAIN_APP_H
#define INCLUDE_MAIN_APP_H

#include "Resource.h"
#include <windows.h>
#include <windowsx.h>

LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
void Main_OnDestroy(HWND);
void Main_OnClose(HWND);
void Main_OnSize(HWND, UINT, int, int);
void Main_OnCommand(HWND, int, HWND, UINT);
BOOL SetDlgPosCenter(HWND);

#endif
