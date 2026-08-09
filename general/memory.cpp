#include "memory.hpp"
#include <stdexcept>
#include <windows.h>
#include <psapi.h>

std::unique_ptr<SIZE_T> MemoryInfo::getPhysicalMemoryUsage(const DWORD pid){
    auto memTotal = std::make_unique<SIZE_T>();
    HANDLE pHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE, pid);

    if(pHandle == NULL){
        throw std::runtime_error(processes.formattedError("Process Handle Opening"));
    }

    PROCESS_MEMORY_COUNTERS_EX pmc;

    if(GetProcessMemoryInfo(pHandle, (PPROCESS_MEMORY_COUNTERS)&pmc, sizeof(pmc))){
        *memTotal = pmc.WorkingSetSize/1e+6;
    }
    else{
        throw std::runtime_error(processes.formattedError("Memory Info Retrieval"));
    }
    CloseHandle(pHandle);
    return memTotal;
}