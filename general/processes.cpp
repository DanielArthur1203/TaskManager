#include "processes.hpp"
#include <windows.h>
#include <intsafe.h>
#include <pdhmsg.h>
#include <stdexcept>

bool Processes::closed = false;

std::vector<PROCESSENTRY32> Processes::getAllActiveProcesses(){
    std::vector<PROCESSENTRY32> processes = std::vector<PROCESSENTRY32>();
    HANDLE handleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if(handleSnap == INVALID_HANDLE_VALUE){
        CloseHandle(handleSnap);
        throw std::runtime_error(formattedError("Process Snapshot"));
    }

    PROCESSENTRY32 p32;
    p32.dwSize = sizeof(PROCESSENTRY32);

    if(!Process32First(handleSnap, &p32)){
        CloseHandle(handleSnap);
        throw std::runtime_error(formattedError("Process Reading"));
    }

    processes.push_back(p32);

    while(Process32Next(handleSnap, &p32)){
        processes.push_back(p32);
    }

    CloseHandle(handleSnap);
    return processes;
}

std::vector<DWORD> Processes::getPIDFromName(const std::wstring &name){
    std::vector<DWORD> pids = std::vector<DWORD>();
    HANDLE handleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if(handleSnap == INVALID_HANDLE_VALUE){
        CloseHandle(handleSnap);
        throw std::runtime_error(formattedError("Process Snapshot"));
    }

    PROCESSENTRY32W p32;
    p32.dwSize = sizeof(PROCESSENTRY32W);

    if(Process32FirstW(handleSnap, &p32)){
        do{
            if(name == p32.szExeFile){
                pids.push_back(p32.th32ProcessID);
            }
        } while(Process32NextW(handleSnap, &p32));
    }
    else{
        CloseHandle(handleSnap);
        throw std::runtime_error(formattedError("Process Reading"));
    }

    CloseHandle(handleSnap);
    return pids;
}

std::wstring Processes::getNameFromPID(const DWORD pid){
    std::wstring exeName = std::wstring(L"");
    HANDLE handleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if(handleSnap == INVALID_HANDLE_VALUE){
        CloseHandle(handleSnap);
        throw std::runtime_error(formattedError("Process Snapshot"));
    }

    PROCESSENTRY32W p32;
    p32.dwSize = sizeof(PROCESSENTRY32W);
    
    if(Process32FirstW(handleSnap, &p32)){
        do{
            if(pid == p32.th32ProcessID){
                exeName.append(p32.szExeFile);
                break;
            }
        } while(Process32NextW(handleSnap, &p32));
    }
    else{
        CloseHandle(handleSnap);
        throw std::runtime_error(formattedError("Process Reading"));
    }

    CloseHandle(handleSnap);
    return exeName;
}

void Processes::closeWindowGUI(DWORD pid){
    EnumWindows(windowsEnumProc, pid);

    if(closed == true){
        HANDLE h = OpenProcess(SYNCHRONIZE, false, pid);

        if(h == NULL){
            throw std::runtime_error(formattedError("Opening Process " + pid));
        }

        DWORD res = WaitForSingleObject(h, 5000);

        if(res == WAIT_TIMEOUT){
            h = OpenProcess(PROCESS_TERMINATE, false, pid);

            if(h == NULL){
                throw std::runtime_error(formattedError("Opening Process " + pid));
            }

            TerminateProcess(h, 1);
        }
        CloseHandle(h);
        // return (res == WAIT_OBJECT_0);
    }
}

bool Processes::closeConsoleProcess(DWORD pid){
    bool res = false;

    if(!(AttachConsole(pid))){
        throw std::runtime_error(formattedError("Attaching Console to Process " + pid));
    }

    SetConsoleCtrlHandler(NULL, true);

    GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pid);

    HANDLE h = OpenProcess(SYNCHRONIZE, false, pid);

    if(h == NULL){
        throw std::runtime_error(formattedError("Opening Process " + pid));
    }

    DWORD wait = WaitForSingleObject(h, 2000);
    if(wait == WAIT_TIMEOUT){
        h = OpenProcess(PROCESS_TERMINATE, false, pid);
        TerminateProcess(h, 1);
        CloseHandle(h);
    }

    res = true;
    CloseHandle(h);
    SetConsoleCtrlHandler(NULL, false);
    FreeConsole();
    return res;
}

std::string Processes::formattedError(std::string msg) noexcept{
    std::string response = "";
    DWORD errorNum;
    int errorNumInt;
    TCHAR sysMsg[256];
    TCHAR *p;

    errorNum = GetLastError( );
    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorNum,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        sysMsg, 256, NULL );

    // Trim the end of the line and terminate it with a null
    p = sysMsg;
    while( ( *p > 31 ) || ( *p == 9 ) )
        ++p;
    do { *p-- = 0; } while( ( p >= sysMsg ) && ( ( *p == '.' ) || ( *p < 33 ) ) );
    

    if(DWordToInt(errorNum, &errorNumInt) == INTSAFE_E_ARITHMETIC_OVERFLOW){
        response = "Failed to generate error code";
    }
    else{
        //idk how to get sysMsg in here without some hijinks
        response += msg + " failed with error number " + std::to_string(errorNumInt);
    }

    return response;
}

std::string Processes::formattedError(std::string msg, bool PDHError) noexcept{
    // Try to format an error message using the PDH module. PDH functions
    // return PDH_STATUS codes rather than Win32 GetLastError(), but callers
    // of this overload currently call it when they expect PDH-related errors
    // so we attempt to format using the last error code if available.
    HMODULE hPdhLibrary = LoadLibraryA("pdh.dll");
    if(hPdhLibrary == NULL){
        DWORD loadErr = GetLastError();
        return msg + " failed (couldn't load pdh.dll), error " + std::to_string(loadErr);
    }

    DWORD dwErrorCode = GetLastError();
    LPWSTR pMessage = NULL;

    if(!FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE |
                    FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    hPdhLibrary,
                    dwErrorCode,
                    0,
                    (LPWSTR)&pMessage,
                    0,
                    NULL))
    {
        FreeLibrary(hPdhLibrary);
        return msg + " failed with error number " + std::to_string(dwErrorCode);
    }

    // Convert wide string to UTF-8 std::string
    std::string text;
    int len = WideCharToMultiByte(CP_UTF8, 0, pMessage, -1, NULL, 0, NULL, NULL);
    if(len > 0){
        text.resize(len - 1);
        WideCharToMultiByte(CP_UTF8, 0, pMessage, -1, &text[0], len, NULL, NULL);
    }

    LocalFree(pMessage);
    FreeLibrary(hPdhLibrary);

    if(text.empty()){
        return msg + " failed with error number " + std::to_string(dwErrorCode);
    }

    return msg + " failed: " + text;
}
