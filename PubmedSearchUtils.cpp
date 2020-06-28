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
#include "PubmedSearchUtils.h"
#include <cctype>
#include <algorithm>

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
    o.lStructSize = sizeof(o);      // 構造体サイズ
    o.hwndOwner = hWnd;             // 親ウィンドウのハンドル
    o.lpstrInitialDir = initDir;    // 初期フォルダー
    o.lpstrFile = filePath;         // 取得したファイル名を保存するバッファ
    o.nMaxFile = sz;                // 取得したファイル名を保存するバッファサイズ
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

string createNewFileName(json j) {
    // First author のlast name 抽出
    string firstAuthor = j["authors"][0]["name"];
    string a, b; // last name だけ取り出し
    int pos = firstAuthor.find(' ');
    a = firstAuthor.substr(0, pos);
    b = firstAuthor.substr((unsigned __int64)pos + 1);

    // Last author のlast name 抽出
    string lastAuthor = j["lastauthor"];
    string c, d; // last name だけ取り出し
    int pos2 = lastAuthor.find(' ');
    c = lastAuthor.substr(0, pos2);
    d = lastAuthor.substr((unsigned __int64)pos2 + 1);

    // 論文タイトルから末尾の「.」削除
    string papertitle = j["title"];
    size_t nCount = papertitle.size() - 1;
    if (papertitle[nCount] == '.')
        papertitle.pop_back();
    string paperTitle = regex_replace(papertitle, regex("[/:*\?\"\'<>|]"), ""); // ファイル名に使えない特殊文字の削除
    //string PaperTitle = regex_replace(paperTitle, regex("[\^\\s\\w]"), "");

    // 論文掲載誌の省略形 (例: Nature neuroscience→Nat neurosci)
    string journalName = j["source"];

    // 論文の出版年度
    string PubDate = j["pubdate"]; // 例: 2020 Apr 30
    string PubYear = PubDate.substr(0, 4); // 例: 2020

    // 論文情報をつなげて新規ファイル名を作成
    string newFileName = a + " "s + c + " ("s + journalName + " "s + PubYear + ") "s + paperTitle;

    return newFileName;
}

string extractPaperTitle(json j) {
    string paperTitle = j["title"];
    return paperTitle;
}

string searchPubmedKeyword(char* title) {
    // NCBI E-utilities (Esearch) を利用したPubmed キーワード検索
    string esearchPrefix = (string)"https://eutils.ncbi.nlm.nih.gov/entrez/eutils/esearch.fcgi?db=pubmed&sort=relevance&retmode=json&term=";
    string keywordOrg = string(title);
    //keyword = regex_replace(keyword, regex("\"\'"), " "); // replace "" to "+"
    string keyword = regex_replace(keywordOrg, regex("\\s"), "+"); // replace "" to "+"
    string esearchUrlStr = esearchPrefix + keyword;
    const char* esearchUrl = esearchUrlStr.c_str(); // std::string to const char*
    string esearchResultStr = getSummary(esearchUrl); // obtain paper ID
    json esearchResult = json::parse(esearchResultStr); // std::string to nlohmann::json
    string uid = esearchResult["esearchresult"]["idlist"][0]; // paper ID
    string newFileName = searchPubmedId(uid);

    return newFileName;
}

string searchPubmedId(string uid) {
    // NCBI E-utilities (Esummary) を利用したPubmed PMID検索
    string esummaryPrefix = (string)"https://eutils.ncbi.nlm.nih.gov/entrez/eutils/esummary.fcgi?db=pubmed&retmode=json&id=";
    string esummaryUrlStr = esummaryPrefix + uid;
    const char* esummaryUrl = esummaryUrlStr.c_str();
    string esummaryResultStr = getSummary(esummaryUrl);
    json esummaryResult = json::parse(esummaryResultStr);

    // Pubmed検索結果をJSON化
    json j = esummaryResult["result"][uid];

    // 論文情報をつなげて新規ファイル名を作成
    string newFileName = createNewFileName(j);

    return newFileName;
}

int levenshteinDistance(string x, string y) {
    int a = x.length();
    int b = y.length();
    int LD[200][200] = {};

    for (size_t j = 0; j < a; j++) LD[j][0] = j;
    for (size_t k = 0; k < b; k++) LD[0][k] = k;

    //aをbに近づけたい!
    for (size_t j = 1; j <= x.size(); j++) {
        for (size_t k = 1; k <= y.size(); k++) {
            //a[j]を削除するか、a[j+1]にb[k]と同じ文字を挿入するか
            //上記２つの行為の回数で最小な方を採用
            int m = min(LD[j - 1][k] + 1, LD[j][k - 1] + 1);
            if (x[j - 1] == y[k - 1]) {
                //最後の文字が同じだから最後の文字がなくても編集距離は同じ
                m = min(m, LD[j - 1][k - 1]);
                LD[j][k] = m;
            }
            else {
                //最後の文字を置換する
                m = min(m, LD[j - 1][k - 1] + 1);
                LD[j][k] = m;
            }
        }
    }
    int dist = LD[x.size()][y.size()];
    return dist;
}

string exactSearchPubmedKeyword(char* title) {
    string esearchPrefix = (string)"https://eutils.ncbi.nlm.nih.gov/entrez/eutils/esearch.fcgi?db=pubmed&sort=relevance&retmode=json&term=";
    string keywordOrg = string(title);
    string keyword = regex_replace(keywordOrg, regex("\\s"), "+"); // replace "" to "+"
    string esearchUrlStr = esearchPrefix + keyword;
    const char* esearchUrl = esearchUrlStr.c_str(); // std::string to const char*
    string esearchResultStr = getSummary(esearchUrl); // obtain paper ID
    json esearchResult = json::parse(esearchResultStr); // std::string to nlohmann::json

    auto uids = esearchResult["esearchresult"]["idlist"];
    size_t uidsNum = esearchResult["esearchresult"]["idlist"].size();

    vector<string> uidsVec;
    for (size_t i = 0; i < uidsNum; ++i) {
        uidsVec.push_back(uids[i]);
    }

    int lowestDist = 1000;
    string probNewFileName;
    string esummaryPrefix = (string)"https://eutils.ncbi.nlm.nih.gov/entrez/eutils/esummary.fcgi?db=pubmed&retmode=json&id=";
    for (size_t i = 0; i < uidsNum; ++i) {
        string uid = uidsVec.at(i);
        string esummaryUrlStr = esummaryPrefix + uid;
        const char* esummaryUrl = esummaryUrlStr.c_str();
        string esummaryResultStr = getSummary(esummaryUrl);
        json esummaryResult = json::parse(esummaryResultStr);
        json j = esummaryResult["result"][uid];

        string paperTitle = extractPaperTitle(j);
        string newFileName = createNewFileName(j);
        int lvDist = levenshteinDistance(keywordOrg, paperTitle);
        if (lvDist < lowestDist) {
            lowestDist = lvDist;
            probNewFileName = newFileName;
        }
    }
    return probNewFileName;
}

string extractFromCrossrefJson(json probPaperJson) {
    string journalName;
    // Authors
    size_t authorNum = probPaperJson["author"].size();
    string firstAuthorFamilyName = probPaperJson["author"][0]["family"];
    string lastAuthorFamilyName = probPaperJson["author"][authorNum - 1]["family"];

    // Publication year
    int pubYear = probPaperJson["created"]["date-parts"][0][0];

    // Paper title
    string paperTitle = probPaperJson["title"][0];

    if (probPaperJson["type"] == "journal-article") {
        // Journal short name
        journalName = probPaperJson["short-container-title"][0];
        journalName = regex_replace(journalName, regex("\\."), "");
    }
    else if (probPaperJson["type"] == "posted-content") {
        journalName = probPaperJson["institution"]["name"];
    }
    else if (probPaperJson["type"] == "proceedings-article") {
        journalName = probPaperJson["container-title"][0];
    }

    // New file name
    string newFileName = firstAuthorFamilyName + " "s + lastAuthorFamilyName + " ("s \
        + journalName + " "s + to_string(pubYear) + ") "s + paperTitle;
    newFileName = regex_replace(newFileName, regex("[/:*\?\"\'<>|]"), "");

    return newFileName;
}

string searchCrossrefKeyword(char* title) {
    string urlPrefix = (string)"https://api.crossref.org/works?sort=relevance&query=";
    string query = string(title);
    query = regex_replace(query, regex("\\s"), "+");
    string url = urlPrefix + query;
    string summaryStr = getSummary(url.c_str());
    json summaryJson = json::parse(summaryStr);
    json probPaperJson = summaryJson["message"]["items"][0];
    return extractFromCrossrefJson(probPaperJson);
}

string searchCrossrefDoi(char* doi) {
    string urlPrefixDoi = (string)"https://api.crossref.org/works/";
    string Doi = string(doi);
    string urlDoi = urlPrefixDoi + Doi;
    string summaryStrDoi = getSummary(urlDoi.c_str());
    json summaryJsonDoi = json::parse(summaryStrDoi);
    json paperJsonDoi = summaryJsonDoi["message"];
    return extractFromCrossrefJson(paperJsonDoi);
}

bool checkPMID(TCHAR* inputs)
{
    char* strC = wcharToChar(inputs);
    string str = string(strC);
    if (std::all_of(str.cbegin(), str.cend(), isdigit))
    {
        return true;
    }
    return false;
}
