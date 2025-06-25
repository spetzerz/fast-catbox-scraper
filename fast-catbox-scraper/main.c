#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <conio.h>
#include "util.h"
#include "backend.h"

#define DO_SLEEP        // Uncomment this line to disable sleeping between requests. this randomly sleeps based on the vars below
#define DO_SLEEP_MIN 0     // Minimum time to sleep between requests in milliseconds. For this to be effective, keep it under DWORD integer limit and place u as a prefix at the front.
#define DO_SLEEP_MAX 500   // Maximum time to sleep between requests in milliseconds.

#define CONSOLE_SLEEP_TIME 200

#define WEB_AGENT L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/137.0.0.0 Safari/537.36"    // Web agent for the HTTP requests
#define PATH L"C:\\Users\\prosp\\Downloads\\logs3.txt"                                                                                   // Change this to your desired path

/* make sure the length of each individual extension doesnt go over 8 chars, else change ID_LENGTH 
   Add the extensions to search for here, for instance this makes the program search for .mp4 and .txt and .png files. */
const wchar_t* const EXTENSIONS[] = {
    L".png"
};


/* I'd recommend not to modify these */
#define ID_LENGTH 6 + 8 + 1 // max id len + max extension len? + null

const int EXTENSION_COUNT = _countof(EXTENSIONS); // Number of extensions

extern void generate_random_string(wchar_t* str);

int Hits = 0;

DWORD WINAPI consoleWorker(LPVOID* params) {
	int* Hits = (int*)(params[0]);
	int* hConsoleWorkerMutex = (HANDLE*)(params[1]);
    while (1) {
        Sleep(CONSOLE_SLEEP_TIME);

        WaitForSingleObject(hConsoleWorkerMutex, INFINITE);
        printf("Hits: %d\n", *Hits);
		ReleaseMutex(hConsoleWorkerMutex);
    }
	return 0;
}


int main() {
    srand((unsigned int)time(NULL));

    HANDLE hConsoleWorkerMutex = CreateMutexW(NULL, FALSE, NULL);
    if (hConsoleWorkerMutex == NULL) {
        printf("Failed to create console worker mutex. Error: %lu\n", GetLastError());
        return 1;
	}

	void* consoleWorkerArgs[] = { &Hits, &hConsoleWorkerMutex };
    if (CreateThread(
        NULL,
        0,
        consoleWorker,
        consoleWorkerArgs,
        0,
        NULL
    ) == NULL) {
		printf("Failed to create console worker thread. Error: %lu\n", GetLastError());
        return 1;
    }

    DWORD exponentialTimer = 0;
    bool timerOccured = false;

    while (1) {
#ifdef DO_SLEEP
        Sleep(get_random_number(DO_SLEEP_MIN, DO_SLEEP_MAX));
#endif
        Sleep(exponentialTimer);

        wchar_t path[ID_LENGTH];
		generate_random_string(path);

        int iExtension;
        int result = check_url_exists(path, &iExtension, &Hits, &hConsoleWorkerMutex);
        if (result && result != 2) {
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
