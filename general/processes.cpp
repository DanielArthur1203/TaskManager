#include "processes.hpp"
#include "general_functions.hpp"
#include <windows.h>
#include <shellapi.h>
#include <intsafe.h>
#include <pdhmsg.h>
#include <shobjidl.h>
#include <propkey.h>
#include <stdexcept>

using namespace GeneralFunctions;
bool Processes::closed = false;

std::wstring Processes::fullPathFromPID(DWORD pid){
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);

    if(h == NULL){
        std::string start = "Opening Process" + std::to_string(pid);
        std::string msg = formattedError(start);
        throw std::runtime_error(msg);
    }

    std::wstring name(MAX_PATH, 0);
    DWORD size = MAX_PATH;

    if(!(QueryFullProcessImageNameW(h, 0, name.data(), &size))){
        CloseHandle(h);
        std::string msg = formattedError("Getting Full Path Name For Process" + std::to_string(pid));
        throw std::runtime_error(msg);
    }
    CloseHandle(h);
    return name;
}

std::wstring Processes::fileDescriptorName(std::wstring &fullPath){
    HRESULT res = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if(FAILED(res)){
        throw std::runtime_error("Getting File Descriptor For " + std::string(wStringToString(fullPath)) 
            + " Failed With HRESULT Code " + std::to_string(res));
    }

    IShellItem2* item = nullptr;
    res = SHCreateItemFromParsingName(fullPath.data(), NULL, IID_PPV_ARGS(&item));

    if(FAILED(res)){
        CoUninitialize();
        throw std::runtime_error("Creating Parsing Name For " + std::string(wStringToString(fullPath)) 
            + " Failed With HRESULT Code " + std::to_string(res));
    }

    PWSTR desc = nullptr;
    res = item->GetString(PKEY_FileDescription, &desc);

    if(FAILED(res) || desc == nullptr){
        CoUninitialize();
        item->Release();
        throw std::runtime_error("Reading File Desc. For " + std::string(wStringToString(fullPath)) + " Failed");
    }

    std::wstring d(desc);

    CoTaskMemFree(desc);
    item->Release();
    CoUninitialize();
    return d;
}

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

std::list<std::string> Processes::allProcessesNames(){
    std::list<std::string> names;

    auto p32s = getAllActiveProcesses();

    for(const auto& p32 : p32s){
        std::wstring path = L"";
        try{
            path = fullPathFromPID(p32.th32ProcessID);
        }
        catch(const std::runtime_error& e){
            std::string temp = p32.szExeFile;
            auto exePos = temp.find(".exe");

            if(exePos != std::string::npos){
                names.push_back(temp.substr(0, exePos));
            }
            else{
                names.push_back(temp);
            }
            continue;
        }

        std::wstring name = L"";
        try{
            name = fileDescriptorName(path);
        }
        catch(const std::runtime_error& e){
            std::string temp = p32.szExeFile;
            auto exePos = temp.find(".exe");

            if(exePos != std::string::npos){
                names.push_back(temp.substr(0, exePos));
            }
            else{
                names.push_back(temp);
            }
            continue;
        }

        if(name.empty()){
            std::string temp = p32.szExeFile;
            auto exePos = temp.find(".exe");

            if(exePos != std::string::npos){
                names.push_back(temp.substr(0, exePos));
            }
            else{
                names.push_back(temp);
            }
            continue;
        }
        names.push_back(std::string(wStringToString(name)));
    }
    return names;
}

std::list<std::string> Processes::allProcessNamesWthExe(){
    std::list<std::string> res;

    std::vector<PROCESSENTRY32> p32s;

    try{
        p32s = getAllActiveProcesses();
    }
    catch(const std::runtime_error& e){
        throw std::runtime_error(e.what());
    }

    for(const auto& p32 : p32s){
        res.push_back(p32.szExeFile);
    }
    return res;
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
            throw std::runtime_error(formattedError("Opening Process " + std::to_string(pid)));
        }

        DWORD res = WaitForSingleObject(h, 5000);

        if(res == WAIT_TIMEOUT){
            h = OpenProcess(PROCESS_TERMINATE, false, pid);

            if(h == NULL){
                throw std::runtime_error(formattedError("Opening Process " + std::to_string(pid)));
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
        throw std::runtime_error(formattedError("Attaching Console to Process " + std::to_string(pid)));
    }

    SetConsoleCtrlHandler(NULL, true);

    GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, pid);

    HANDLE h = OpenProcess(SYNCHRONIZE, false, pid);

    if(h == NULL){
        throw std::runtime_error(formattedError("Opening Process " + std::to_string(pid)));
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

bool Processes::windowProcess(DWORD pid){
    bool answer = true;
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid);
    if(h == NULL){
        throw std::runtime_error(formattedError("Opening Process " + std::to_string(pid)));
    }

    std::wstring exeName(MAX_PATH, 0);
    DWORD size = MAX_PATH;

    if(!QueryFullProcessImageNameW(h, 0, exeName.data(), &size)){
        CloseHandle(h);
        throw std::runtime_error(formattedError("Querying Process Name For " + std::to_string(pid)));
    }
    SHFILEINFOW s = SHFILEINFOW{};
    
    auto type = SHGetFileInfoW(exeName.data(), 0, &s, sizeof(s), SHGFI_EXETYPE);

    if(type == 0){
        answer = false;
        return answer;
    }

    WORD low = LOWORD(type);
    WORD high = HIWORD(type);

    if(low == IMAGE_NT_SIGNATURE || low == IMAGE_DOS_SIGNATURE){
        if(high == 0){
            answer = false;
            return answer;
        }
        else{
            answer = true;
        }
    }
    CloseHandle(h);
    return answer;
}
