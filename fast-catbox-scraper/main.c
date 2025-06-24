#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define WEB_AGENT L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/137.0.0.0 Safari/537.36"    // Web agent for the HTTP requests
#define PATH L"C:\\Users\\prosp\\Downloads\\logs2.txt"                                                                                   // Change this to your desired path

// make sure the length of each individual extension doesnt go over 8 chars, else change ID_LENGTH
/*
const wchar_t* const EXTENSIONS[] = {
    L".zip",
    L".txt",
    L".pdf",
    L".jpg",
    L".png",
    L".gif",
    L".mp3",
    L".wav",
    L".avi",
    L".mkv",
    L".webm",
    L".flv",
    L".mov",
    L".wmv",
    L".docx",
    L".xlsx",
    L".pptx",
    L".rar",
    L".7z",
    L".tar.gz",
    L".exe",
    L".bat",
    L".sh",
    L".json",
    L".xml",
    L".html",
    L".css",
    L".js",
    L".mp4",
};
*/

const wchar_t* const EXTENSIONS[] = {
    L".png",
    L".mp4"
};

/* I'd recommend not to modify these */
#define HOST L"files.catbox.moe"
const wchar_t CHARSET[] = L"abcdefghijklmnopqrstuvwxyz0123456789";
#define ID_LENGTH 6 + 8 + 1 // max id len + max extension len? + null
#define TRUE_ID_LENGTH 6

void generate_random_string(wchar_t* str) {
    for (size_t i = 0; i < TRUE_ID_LENGTH; i++) {
        int randid2 = rand() % (_countof(CHARSET) - 1);
        str[i] = CHARSET[randid2];
    }
    str[TRUE_ID_LENGTH] = L'\0';
}


int check_url_exists(const wchar_t path[], int* retExtensionIndex) {
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

    int success = 0; // never goes on a 200 or even 200 or 503, 0
    for (int i = 0; i < _countof(EXTENSIONS); i++) {
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
            printf("GOT: %ls\n", path2);
        }
        else if (statusCode == 503) {
            printf("    NO HIT %ls\n", path2);
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

/*int getRandomInt() {
    return rand() % 1500; // 0 to 1500 inclusive
}*/

int main() {
    srand((unsigned int)time(NULL));

    DWORD exponentialTimer = 0;
    bool timerOccured = false;
    while (1) {
        Sleep(exponentialTimer);

        wchar_t path[ID_LENGTH];
		generate_random_string(path);

        int result;
        int iExtension;
        if ((result = check_url_exists(path, &iExtension))) {
            exponentialTimer, timerOccured = 0;
            HANDLE hFile = CreateFileW(
                PATH,
                GENERIC_WRITE,
                0,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            ); if (hFile != INVALID_HANDLE_VALUE) {
                DWORD dwPtr = SetFilePointer(hFile, 0, NULL, FILE_END);
                if (dwPtr == INVALID_SET_FILE_POINTER)
                    printf("Failed to move file pointer. Error: %lu\n", GetLastError());

				wchar_t fullPath[ID_LENGTH + 25]; // 25 is the length of "https://files.catbox.moe/"
				wcscpy_s(fullPath, ID_LENGTH + 25, L"https://files.catbox.moe/");
				wcscat_s(fullPath, ID_LENGTH + 25, path);
                wcscat_s(fullPath, ID_LENGTH + 25, EXTENSIONS[iExtension]);

				char fullPathA[ID_LENGTH + 25];
				WideCharToMultiByte(CP_UTF8, 0, fullPath, -1, fullPathA, sizeof(fullPathA), NULL, NULL);
                strcat_s(fullPathA, ID_LENGTH + 25, "\n");

                DWORD numBytesWritten;
                if (!WriteFile(hFile, fullPathA, strlen(fullPathA), &numBytesWritten, NULL) || numBytesWritten == 0)
                    printf("Failed to save. Error: %lu\n", GetLastError());

                CloseHandle(hFile);
            }
            else
                printf("Failed to save. Error: %lu\n", GetLastError());
        }
        else if (result == 2) {
            if (timerOccured == false)
                exponentialTimer = 1;
            timerOccured = true;
            exponentialTimer *= 2;
        }
    }
}
