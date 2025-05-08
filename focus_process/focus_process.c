#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <string.h>

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

DWORD FindRunningProcess(const char* exeName) {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    if (Process32First(snapshot, &entry)) {
        do {
            if (_stricmp(entry.szExeFile, exeName) == 0) {
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return 0;
}

const char* GetFileNameFromPath(const char* path) {
    const char* p = strrchr(path, '\\');
    return p ? p + 1 : path;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <full_path_to_executable>\n", argv[0]);
        return 1;
    }

    const char* fullPath = argv[1];
    const char* exeName = GetFileNameFromPath(fullPath);

    DWORD pid = FindRunningProcess(exeName);
    if (pid != 0) {
        printf("Process already running (PID: %lu). Focusing window...\n", pid);
    } else {
        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi;

        if (!CreateProcess(NULL, (LPSTR)fullPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            printf("Failed to launch process. Error code: %lu\n", GetLastError());
            return 1;
        }

        printf("Process started (PID: %lu)\n", pi.dwProcessId);
        pid = pi.dwProcessId;
        WaitForInputIdle(pi.hProcess, 10000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    // Try to find and focus the main window
    HWND hwnd = NULL;
    for (int i = 0; i < 50 && hwnd == NULL; ++i) {
        hwnd = FindMainWindow(pid);
        Sleep(100);
    }

    if (hwnd != NULL) {
        printf("Bringing window to foreground.\n");
        while (IsWindow(hwnd)) {
            SetForegroundWindow(hwnd);
            Sleep(500);
        }
        printf("Window closed or no longer valid.\n");
    } else {
        printf("Unable to find window for PID: %lu\n", pid);
    }

    return 0;
}
