#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h> // For GetModuleFileNameEx
#include <Lmcons.h> // For UNLEN
#include <iostream>
#include <string>

// ユーザーID（ユーザー名）を取得する関数
std::wstring GetCurrentUserID() {
    wchar_t username[UNLEN + 1];
    DWORD usernameLen = UNLEN + 1;

    if (GetUserNameW(username, &usernameLen)) {
        return std::wstring(username);
    }
    else {
        return L"";
    }
}

bool KillProcessByName(const std::wstring& fullPath) {
    bool result = false;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
            if (hProcess) {
                wchar_t processPath[MAX_PATH];
                if (GetModuleFileNameExW(hProcess, NULL, processPath, MAX_PATH)) {
                    if (fullPath == processPath) {
                        TerminateProcess(hProcess, 0);
                        result = true;
                    }
                }
                CloseHandle(hProcess);
            }
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return result;
}

bool RenameFile(const std::wstring& oldName, const std::wstring& newName) {
    return MoveFileW(oldName.c_str(), newName.c_str());
}

bool StartProcess(const std::wstring& exePath) {
    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(si);

    if (CreateProcessW(
        exePath.c_str(),  // Executable path
        nullptr,          // Command line arguments
        nullptr,          // Process handle not inheritable
        nullptr,          // Thread handle not inheritable
        FALSE,            // Do not inherit handles
        0,                // No creation flags
        nullptr,          // Use parent's environment block
        nullptr,          // Use parent's starting directory
        &si,              // Pointer to STARTUPINFO structure
        &pi               // Pointer to PROCESS_INFORMATION structure
    )) {
        // ハンドルを閉じる
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }

    return false;
}

int main() {
    // ユーザーIDを取得
    std::wstring userID = GetCurrentUserID();
    if (userID.empty()) {
        std::wcerr << L"Failed to get current user ID.\n";
        return 1;
    }

    // パスを動的に構成
    std::wstring targetProcessPath = L"C:\\Users\\" + userID + L"\\AppData\\Local\\SyugKpcClient2\\app\\SyugKpcClient2.exe";
    std::wstring backupProcessPath = L"C:\\Users\\" + userID + L"\\AppData\\Local\\SyugKpcClient2\\app\\SyugKpcClient2_backup.exe";

    // 機能1: プロセスをKillしてファイル名を変更
    if (KillProcessByName(targetProcessPath)) {
        std::wcout << L"Process at " << targetProcessPath << L" killed.\n";
        if (RenameFile(targetProcessPath, backupProcessPath)) {
            std::wcout << L"File renamed from " << targetProcessPath << L" to " << backupProcessPath << L".\n";
            system("pause");
            return 0;
        }
        else {
            std::wcerr << L"Failed to rename file " << targetProcessPath << L".\n";
            return 1;
        }
    }
    else {
        std::wcout << L"Process at " << targetProcessPath << L" not running.\n";
    }

    // 機能2: プロセスが存在しない場合、元に戻して起動
    if (!KillProcessByName(targetProcessPath)) {
        if (RenameFile(backupProcessPath, targetProcessPath)) {
            std::wcout << L"File renamed back from " << backupProcessPath << L" to " << targetProcessPath << L".\n";
            if (StartProcess(targetProcessPath)) {
                std::wcout << L"Process at " << targetProcessPath << L" started.\n";
            }
            else {
                std::wcerr << L"Failed to start process at " << targetProcessPath << L".\n";
                return 1;
            }
        }
        else {
            std::wcerr << L"Failed to rename file " << backupProcessPath << L" back to " << targetProcessPath << L".\n";
            return 1;
        }
    }
    system("pause");
    return 0;
}
