#ifndef INCLUDE_PUBMED_SEARCH_UTILS_H
#define INCLUDE_PUBMED_SEARCH_UTILS_H
#include <iostream>
#include <string>
#include <windows.h>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

BOOL getFileName(HWND, TCHAR*, int, const wchar_t*);
string getSummary(const char*);
size_t callbackWrite(char*, size_t, size_t, string*);
string createNewFileName(json);
string extractPaperTitle(json);
string searchPubmedKeyword(const char*);
string exactSearchPubmedKeyword(const char*);
string extractFromCrossrefJson(json);
string searchCrossrefKeyword(const char*);
string searchCrossrefDoi(const char*);
string searchPubmedId(string);
string authorNameStrip(string);
vector<string> split(string&, char);
char* wcharToChar(WCHAR*);
WCHAR* charToWchar(const char*);
int levenshteinDistance(string, string);
bool checkPMID(TCHAR*);

#endif
