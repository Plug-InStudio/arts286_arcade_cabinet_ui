#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>

HWND FindMainWindow(DWORD processID) {
    HWND hwnd = GetTopWindow(NULL);
    while (hwnd) {
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == processID && IsWindowVisible(hwnd)) {
            return hwnd;
        }
        hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <full_path_to_executable>\n", argv[0]);
        return 1;
    }

    char* exePath = argv[1];
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;

    if (!CreateProcess(NULL, exePath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        printf("Failed to launch process. Error code: %lu\n", GetLastError());
        return 1;
    }

    printf("Process started (PID: %lu)\n", pi.dwProcessId);

    // Wait for process to initialize
    WaitForInputIdle(pi.hProcess, 10000);

    HWND targetHwnd = NULL;
    int attempts = 0;

    while (attempts < 100) {
        targetHwnd = FindMainWindow(pi.dwProcessId);
        if (targetHwnd != NULL) break;
        Sleep(100);
        attempts++;
    }

    if (targetHwnd == NULL) {
        printf("Failed to find main window for the process.\n");
        return 1;
    }

    printf("Focusing window...\n");

    // Loop: Bring it to front as long as the process is running
    while (WaitForSingleObject(pi.hProcess, 100) == WAIT_TIMEOUT) {
        SetForegroundWindow(targetHwnd);
        Sleep(500);  // Reduce flicker
    }

    printf("Process exited.\n");

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}
