#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <string> 
#include <iostream> 
#include <regex>
#include <stdlib.h>
#include <nlohmann/json.hpp>
#include <curl/curl.h> 
#include "paperLabeling.h"
#define SIZE 1000
using namespace std;
using json = nlohmann::json;

// WCHAR*型からchar*型への変換
char* wcharToChar(WCHAR* wStrW) {
    char* wStrC = (char*)malloc(SIZE); //変換文字列格納バッファ
    //char wStrC[SIZE];
    size_t wLen = 0;
    errno_t err = 0;
    err = wcstombs_s(&wLen, wStrC, SIZE, wStrW, _TRUNCATE); //変換
    return wStrC;
}

// char*型からWCHAR*型への変換
WCHAR* charToWchar(const char* wStrC) {
    WCHAR* wStrW = (WCHAR*)malloc(SIZE); //変換文字列格納バッファ
    //WCHAR wStrW[SIZE];
    size_t wLen = 0;
    errno_t err = 0;
    err = mbstowcs_s(&wLen, wStrW, SIZE, wStrC, _TRUNCATE); //変換
    return wStrW;
}


// ファイル選択ダイアログを開く
BOOL getFileName(HWND hWnd, TCHAR* filePath, int sz, const wchar_t* initDir) {
    OPENFILENAME o;
    filePath[0] = _T('\0');
    ZeroMemory(&o, sizeof(o));
    o.lStructSize = sizeof(o);              //      構造体サイズ
    o.hwndOwner = hWnd;                             //      親ウィンドウのハンドル
    o.lpstrInitialDir = initDir;    //      初期フォルダー
    o.lpstrFile = filePath;                    //      取得したファイル名を保存するバッファ
    o.nMaxFile = sz;                                //      取得したファイル名を保存するバッファサイズ
    o.lpstrFilter = _TEXT("PDFファイル(*.pdf)\0*.pdf\0") _TEXT("全てのファイル(*.*)\0*.*\0");
    o.lpstrDefExt = _TEXT("PDF");
    o.lpstrTitle = _TEXT("リネームしたい論文PDFを指定");
    o.nFilterIndex = 1;
    return GetOpenFileName(&o);
}

size_t callbackWrite(char* ptr, size_t size, size_t nmemb, string* stream) {
    int dataLength = size * nmemb;
    stream->append(ptr, dataLength);
    return dataLength;
}

string getSummary(const char* url) {
    CURL* curl;
    CURLcode ret;

    curl = curl_easy_init();
    string chunk;


    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callbackWrite);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
    ret = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    /*if (ret != CURLE_OK) {
        cerr << "curl_easy_perform() failed." << endl;
        return 1;
    }*/

    return chunk;
}

string searchPubmedKeyword(char* title) {
    // 論文をpubmed検索
    string esearchPrefix = (string)"https://eutils.ncbi.nlm.nih.gov/entrez/eutils/esearch.fcgi?db=pubmed&usehistory=y&retmode=json&term=";
    string keyword = regex_replace(title, regex("\\s"), "+"); // replace "" to "+"
    string esearchUrlStr = esearchPrefix + keyword;
    const char* esearchUrl = esearchUrlStr.c_str(); // std::string to const char*
    string esearchResultStr = getSummary(esearchUrl); // obtain paper ID
    json esearchResult = json::parse(esearchResultStr); // std::string to nlohmann::json
    string uid = esearchResult["esearchresult"]["idlist"][0]; // paper ID
    string newFileName = searchPubmedId(uid);

    return newFileName;
}

string searchPubmedId(string uid) {
    // Obtain paper summary using NCBI E-utilities(Esummary)
    string esummaryPrefix = (string)"https://eutils.ncbi.nlm.nih.gov/entrez/eutils/esummary.fcgi?db=pubmed&retmode=json&id=";
    string esummaryUrlStr = esummaryPrefix + uid;
    const char* esummaryUrl = esummaryUrlStr.c_str();
    string esummaryResultStr = getSummary(esummaryUrl);
    json esummaryResult = json::parse(esummaryResultStr);

    // 論文情報をつなげて新規ファイル名を作成
    json j = esummaryResult["result"][uid];
    string firstAuthor = j["authors"][0]["name"];
    string lastAuthor = j["lastauthor"];
    string paperTitle = j["title"];
    string journalName = j["source"];
    string ePubDate = j["epubdate"];
    string newFileName = firstAuthor + ", "s + lastAuthor + " ("s + journalName + " "s + ePubDate + ") "s + paperTitle;

    return newFileName;
}
