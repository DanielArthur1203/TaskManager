#include "memory.hpp"
#include <stdexcept>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <vector>

SIZE_T MemoryInfo::getPhysicalMemoryUsage(const DWORD pid){
    SIZE_T memTotal = 0;
    HANDLE pHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE, pid);

    if(pHandle == NULL){
        CloseHandle(pHandle);
        throw std::runtime_error(processes.formattedError("Process Handle Opening"));
    }

    PROCESS_MEMORY_COUNTERS_EX pmc;

    if(GetProcessMemoryInfo(pHandle, (PPROCESS_MEMORY_COUNTERS)&pmc, sizeof(pmc))){
        memTotal = pmc.WorkingSetSize/1e+6; //byte to megabyte conversion
    }
    else{
        CloseHandle(pHandle);
        throw std::runtime_error(processes.formattedError("Memory Info Retrieval"));
    }
    CloseHandle(pHandle);
    return memTotal;
}

SIZE_T MemoryInfo::getTotalPhysicalMemoryUsage(){
    SIZE_T totalMem = 0;
    std::vector<PROCESSENTRY32> activeProcesses;

    try{
        activeProcesses = processes.getAllActiveProcesses();
    }
    catch(std::runtime_error e){
        throw std::runtime_error(e.what());
    }

    for(const PROCESSENTRY32& p32: activeProcesses){
        try{
            totalMem += getPhysicalMemoryUsage(p32.th32ProcessID);
        }
        catch(const std::runtime_error& e){
            //Some processes cannot have their memory read and I don't see a pattern in their names
            continue;
        }
    }

    if(totalMem <= 0){
        throw std::runtime_error("< 0MB of memory usage is not feasible");
    }
    return totalMem;
}
