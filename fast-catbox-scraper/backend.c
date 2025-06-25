#include "util.h"
#include "backend.h"
#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <conio.h>

#define WEB_AGENT L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/137.0.0.0 Safari/537.36"    // Web agent for the HTTP requests
#define PATH L"C:\\Users\\prosp\\Downloads\\logs3.txt"   
#define ID_LENGTH 6 + 8 + 1 // max id len + max extension len? + null
#define TRUE_ID_LENGTH 6
#define HOST L"files.catbox.moe"
const wchar_t CHARSET[] = L"abcdefghijklmnopqrstuvwxyz0123456789";
extern const wchar_t* const EXTENSIONS[];
extern const int EXTENSION_COUNT;

void generate_random_string(wchar_t* str) {
    for (size_t i = 0; i < TRUE_ID_LENGTH; i++) {
        int randid2 = rand() % (_countof(CHARSET) - 1);
        str[i] = CHARSET[randid2];
    }
    str[TRUE_ID_LENGTH] = L'\0';
}


int check_url_exists(const wchar_t path[], int* retExtensionIndex, int* Hits, HANDLE* hConsoleWorkerMutex) {
    HINTERNET
        hSession = NULL,
        hConnect = NULL,
        hRequest = NULL;

    hSession = WinHttpOpen(
        WEB_AGENT,
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    ); if (!hSession)
        return 0;

    hConnect = WinHttpConnect(hSession, HOST, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return 0;
    }

    int success = 0; // 503 if for loop exits and none are found
    for (int i = 0; i < EXTENSION_COUNT; i++) {
        wchar_t path2[ID_LENGTH];
        wcscpy_s(path2, ID_LENGTH, path);
        wcscat_s(path2, ID_LENGTH, EXTENSIONS[i]);

        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);
        hRequest = WinHttpOpenRequest(
            hConnect,
            L"HEAD",
            path2,
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE
        ); if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return 0;
        }

        if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(hRequest, NULL)) {
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                NULL, &statusCode, &statusSize, NULL);
        }
        else
            printf("Failed to send a request or to query headers\n");
        if (statusCode == 200) {
            *retExtensionIndex = i;
            success = 1;
            WaitForSingleObject(*hConsoleWorkerMutex, INFINITE);
            (*Hits)++;
            ReleaseMutex(*hConsoleWorkerMutex);
        }
        else if (statusCode == 503) {
            //printf("    NO HIT %ls\n", path2);
            continue;
        }
        else {
            success = 2;
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return success;
}