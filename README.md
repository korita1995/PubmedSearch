# PubmedSearch2FileName
論文名 or PMID or DOIで論文検索して、規定のフォーマットで論文情報を取得するWindowsアプリ  
PubmedSearch2FileName.exe ファイルをダウンロードするだけで実行可能です。  
（Visual Studio 2019 をインストールしていなくても実行可能）

# Visual studio 2019 でビルドする方法(64bit用, Release版)
1) https://github.com/curl/curl をダウンロード
2) x64 Native Tools Command Prompt for VS 2019 を起動
3) cd (ダウンロードしたdirectory)/curl-master/
4) ./buildconf.bat
5) cd winbuild
6) set RTLIBCFG=static
7) nmake /f Makefile.vc mode=static vc=16 debug=no
8) curlを利用したいプロジェクトのプロパティを開く  
（メニューバーのプロジェクト(P)>(プロジェクト名)のプロパティ(P)）
9) 構成(C):Release、プラットフォーム(P):x64 に設定
10) C/C++ > 全般 > 追加のインクルードディレクトリに以下のPathを追加  
(ダウンロードしたdirectory)/curl-master/builds/libcurl-vc16-x64-release-static-ipv6-sspi-winssl/include
11) C/C++ > プリプロセッサの定義　に CURL_STATICLIB を追加　（もしかしたらこの工程不要かも）
12) C/C++ > ランタイムライブラリ を マルチスレッド (/MT) に設定
13) リンカー > 全般 > 追加のライブラリディレクトリに以下のPathを追加  
(ダウンロードしたdirectory)/curl-master/builds/libcurl-vc16-x64-release-static-ipv6-sspi-winssl/lib
14) リンカー > 追加の依存ファイルに以下を追加する  
Normaliz.lib;Crypt32.lib;Wldap32.lib;Ws2_32.lib;(ダウンロードしたdirectory)/curl-master/builds/libcurl-vc16-x64-release-static-ipv6-sspi-winssl/lib/libcurl_a.lib

# C++ 用のJSONライブラリの導入方法
1) https://github.com/nlohmann/json をダウンロード
2) nlohmann jsonを利用したいプロジェクトのプロパティを開く
3) C/C++ > 全般 > 追加のインクルードディレクトリに以下のPathを追加  
(ダウンロードしたdirectory)/json-develop/include

# その他の必要事項
1) プロジェクトのプロパティを開く
2) リンカー > システム > サブシステム を Windows (/SUBSYSTEM:WINDOWS) に設定
3) リンカー > 入力 > 追加の依存ファイル に User32.lib を追加 (Windows.h の関数を利用するため)

# License
This software is released under the MIT License, see LICENSE.txt.
