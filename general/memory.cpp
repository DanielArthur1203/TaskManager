#include "memory.hpp"
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <pdh.h>
#include <sysinfoapi.h>
#include <vector>
#include <stdexcept>
#include <cmath>

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
        memTotal = pmc.WorkingSetSize/(1024 * 1024); //byte to megabyte conversion
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
    catch(const std::runtime_error& e){
        throw std::runtime_error(e.what());
    }

    for(const PROCESSENTRY32& p32: activeProcesses){
        try{
            totalMem += getPhysicalMemoryUsage(p32.th32ProcessID);
        }
        catch(const std::runtime_error& e){
            //Skip PPL and PP-L processes
            continue;
        }
    }

    if(totalMem <= 0){
        throw std::runtime_error("< 0MB of memory usage is not feasible");
    }
    return totalMem;
}

unsigned int MemoryInfo::getPercentageMemoryUsage(){
    unsigned int percentage = 0; 
    ULONGLONG totalMem = 0;

    if(!GetPhysicallyInstalledSystemMemory(&totalMem)){
        throw std::runtime_error(processes.formattedError("Physical System Memory Retrieval"));
    }

    SIZE_T currentUsage = getTotalPhysicalMemoryUsage();
    totalMem /= 1024; //kb to mb

    double totalDouble = static_cast<double>(totalMem);
    percentage = ((static_cast<double>(currentUsage)) / (totalDouble)) * 100.0;
    return percentage;
}

unsigned long MemoryInfo::getTotalCachedMemory(){
    unsigned long cachedMem = 0;
    //Most consistent with Task Manager
    LPCSTR paths[] = {"\\Memory\\Modified page list bytes",
        "\\Memory\\Standby Cache Core Bytes", "\\Memory\\Standby Cache Reserve Bytes",
        "\\Memory\\Standby Cache Normal Priority Bytes"};
    //LPCSTR counterPath = "\\Memory\\Cache Bytes"; //inconsistent 
    PDH_HQUERY query;
    DWORD_PTR dwP = 0;
    
    for(const LPCSTR& counterPath : paths){
        if(PdhOpenQueryA(NULL, dwP, &query) == ERROR_SUCCESS){
            PDH_HCOUNTER pCounter = NULL;

            if(PdhAddCounterA(query, counterPath, dwP, &pCounter) == ERROR_SUCCESS){
                if(PdhCollectQueryData(query) != ERROR_SUCCESS){
                    PdhCloseQuery(query);
                    throw std::runtime_error(processes.formattedError("Collecting Query Data", true));
                }
                Sleep(1000);
                if(PdhCollectQueryData(query) != ERROR_SUCCESS){
                    PdhCloseQuery(query);
                    throw std::runtime_error(processes.formattedError("Collecting Query Data", true));
                }

                PDH_FMT_COUNTERVALUE pValue = PDH_FMT_COUNTERVALUE{};
                if(PdhGetFormattedCounterValue(pCounter, PDH_FMT_LONG | PDH_FMT_NOSCALE, NULL, &pValue) == ERROR_SUCCESS){
                    pValue.longValue /= (1024 * 1024); //byte to mb
                    cachedMem += pValue.longValue;
                }
                else{
                    PdhCloseQuery(query);
                    PdhRemoveCounter(pCounter);
                    throw std::runtime_error(processes.formattedError("Getting Formatted Counter", true));
                }
            }
            else{
                PdhCloseQuery(query);
                PdhRemoveCounter(pCounter);
                throw std::runtime_error(processes.formattedError("Adding Counter", true));
            }
            PdhRemoveCounter(pCounter);
        }
        else{
            PdhCloseQuery(query);
            throw std::runtime_error(processes.formattedError("Query Opening", true));
        }
    }

    PdhCloseQuery(query);
    return cachedMem;
}

double MemoryInfo::getSystemCommitLimit(){
    double systemCommitLimit = 0;
    MEMORYSTATUSEX state = MEMORYSTATUSEX();
    state.dwLength = sizeof(state);

    if(GlobalMemoryStatusEx(&state)){
        systemCommitLimit = state.ullTotalPageFile / (1024.0 * 1024.0 * 1024.0); //byte to gb
        systemCommitLimit = std::round(systemCommitLimit * 10) / 10; //round by the 10s place
    }
    else{
        throw std::runtime_error(processes.formattedError("Getting Global Memory Status"));
    }
    return systemCommitLimit;
}

double MemoryInfo::getCurrentCommitUsage(){
    double currentCommit = 0;
    MEMORYSTATUSEX state = MEMORYSTATUSEX();
    state.dwLength = sizeof(state);

    if(GlobalMemoryStatusEx(&state)){
        double maxCommit = getSystemCommitLimit();
        double availCommit = state.ullAvailPageFile / (1024.0 * 1024.0 * 1024.0); //byte to gb
        currentCommit = maxCommit - availCommit;
        currentCommit = std::round(currentCommit * 10) / 10;
    }
    else{
        throw std::runtime_error(processes.formattedError("Getting Global Memory Status"));
    }
    return currentCommit;
}

unsigned long MemoryInfo::getPagedPool(){
    unsigned long pagedPool = 0;
    PERFORMANCE_INFORMATION pInfo = PERFORMANCE_INFORMATION();
    pInfo.cb = sizeof(PERFORMANCE_INFORMATION);

    if(GetPerformanceInfo(&pInfo, sizeof(pInfo))){
        pagedPool = (pInfo.KernelPaged * pInfo.PageSize) / (1024 * 1024); //b to mb
    }
    else{
        throw std::runtime_error(processes.formattedError("Getting Performance Info"));
    }
    return pagedPool;
}
