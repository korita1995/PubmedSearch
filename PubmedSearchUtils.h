#ifndef INCLUDE_PUBMED_SEARCH_UTILS_H
#define INCLUDE_PUBMED_SEARCH_UTILS_H
#include <iostream>
#include <string>
#include <windows.h>

using namespace std;

BOOL getFileName(HWND, TCHAR*, int, const wchar_t*);
string getSummary(const char*);
size_t callbackWrite(char*, size_t, size_t, string*);
string searchPubmedKeyword(char*);
string searchPubmedId(string);
char* wcharToChar(WCHAR*);
WCHAR* charToWchar(const char*);

#endif
