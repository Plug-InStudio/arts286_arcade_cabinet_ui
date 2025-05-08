#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <string.h>

// ───── GLOBAL HOOK SETUP ─────
HHOOK g_hHook = NULL;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        return 1;  // Block all key input
    }
    return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

void InstallKeyboardBlockHook() {
    g_hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);
}

void UninstallKeyboardBlockHook() {
    if (g_hHook) {
        UnhookWindowsHookEx(g_hHook);
        g_hHook = NULL;
    }
}
// ─────────────────────────────

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

void force_foreground(HWND hwnd, DWORD pid) {
    if (hwnd != NULL) {
        printf("Bringing window to foreground...\n");

        DWORD foregroundThread = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
        DWORD targetThread = GetWindowThreadProcessId(hwnd, NULL);

        AttachThreadInput(foregroundThread, targetThread, TRUE);

        ShowWindow(hwnd, SW_RESTORE);  // Ensure it's not minimized
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
        SetActiveWindow(hwnd);

        AttachThreadInput(foregroundThread, targetThread, FALSE);

        HWND current = GetForegroundWindow();
        while (current != hwnd) {
            SetForegroundWindow(hwnd);
            Sleep(300);
            current = GetForegroundWindow();
        }
    } else {
        printf("Unable to find window for PID: %lu\n", pid);
    }
}

const char* GetFileNameFromPath(const char* path) {
    const char* p = strrchr(path, '\\');
    return p ? p + 1 : path;
}

int main(int argc, char* argv[]) {
    InstallKeyboardBlockHook();  // ⛔ Disable input at the start

    if (argc != 2) {
        printf("Usage: %s <full_path_to_executable>\n", argv[0]);
        UninstallKeyboardBlockHook();
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
            UninstallKeyboardBlockHook();  // ☑️ Re-enable input on failure
            return 1;
        }

        printf("Process started (PID: %lu)\n", pi.dwProcessId);
        pid = pi.dwProcessId;
        WaitForInputIdle(pi.hProcess, 10000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    HWND hwnd = NULL;
    for (int i = 0; i < 1000 && hwnd == NULL; ++i) {
        hwnd = FindMainWindow(pid);
        Sleep(100);
    }

    force_foreground(hwnd, pid);
    UninstallKeyboardBlockHook();  // ✅ Re-enable input once window is ready

    if (hwnd != NULL) {
        printf("Maintaining focus on window...\n");
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
