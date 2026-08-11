#include "processes.hpp"
#include <windows.h>
#include <intsafe.h>
#include <stdexcept>

std::vector<PROCESSENTRY32> Processes::getAllActiveProcesses() noexcept(false){
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

std::vector<DWORD> Processes::getPIDFromName(const std::wstring &name) noexcept(false){
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

std::wstring Processes::getNameFromPID(const DWORD pid) noexcept(false){
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
