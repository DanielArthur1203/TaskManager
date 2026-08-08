#include "processes.hpp"
#include <windows.h>

std::unique_ptr<std::vector<PROCESSENTRY32>> Processes::getAllActiveProcesses(){
    auto processes = std::make_unique<std::vector<PROCESSENTRY32>>();
    HANDLE handleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if(handleSnap == INVALID_HANDLE_VALUE){
        //idk do something
        return nullptr;
    }

    PROCESSENTRY32 p32;
    p32.dwSize = sizeof(PROCESSENTRY32);

    if(!Process32First(handleSnap, &p32)){
        //idk do something
        return nullptr;
    }

    processes->push_back(p32);

    while(Process32Next(handleSnap, &p32)){
        processes->push_back(p32);
    }

    CloseHandle(handleSnap);
    return processes;
}