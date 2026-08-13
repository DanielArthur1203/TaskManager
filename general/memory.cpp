#include "memory.hpp"
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <pdh.h>
#include <sysinfoapi.h>
#include <comdef.h>
#include <wbemidl.h>
#include <oleauto.h>
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

unsigned long MemoryInfo::getNonPagedPool(){
    unsigned long nonPagedPool = 0;
    PERFORMANCE_INFORMATION pInfo = PERFORMANCE_INFORMATION();
    pInfo.cb = sizeof(PERFORMANCE_INFORMATION);

    if(GetPerformanceInfo(&pInfo, sizeof(pInfo))){
        nonPagedPool = (pInfo.KernelNonpaged * pInfo.PageSize) / (1024 * 1024); //b to mb
    }
    else{
        throw std::runtime_error(processes.formattedError("Getting Performance Info"));
    }
    return nonPagedPool;
}

unsigned long MemoryInfo::getMemorySpeed(){
    unsigned long memorySpeed = 0;

    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if(hres != 0){
        throw std::runtime_error("Failed with HRESULT code " + hres);
    }

    hres = CoInitializeSecurity(
        NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL
    );

    IWbemLocator* pLoc = NULL;
    hres = CoCreateInstance(
        CLSID_WbemLocator, NULL,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (void**)&pLoc
    );
    if (hres != 0 || pLoc == NULL) {
        CoUninitialize();
        throw std::runtime_error("Failed with HRESULT code " + hres);
    }

    // Use SysAllocString instead of _bstr_t for MinGW compatibility
    BSTR bstrNamespace = SysAllocString(L"ROOT\\CIMV2");
    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(bstrNamespace, NULL, NULL, 0, WBEM_FLAG_CONNECT_USE_MAX_WAIT, 0, 0, &pSvc);
    SysFreeString(bstrNamespace);
    
    if (hres != 0 || pSvc == NULL) {
        pLoc->Release();
        CoUninitialize();
        throw std::runtime_error("Failed with HRESULT code " + hres);
    }

    BSTR bstrLanguage = SysAllocString(L"WQL");
    BSTR bstrQuery = SysAllocString(L"SELECT Speed, ConfiguredClockSpeed FROM Win32_PhysicalMemory");
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(bstrLanguage, bstrQuery, 
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    SysFreeString(bstrLanguage);
    SysFreeString(bstrQuery);

    if (SUCCEEDED(hres) && pEnumerator != NULL) {
        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;
        
        if (SUCCEEDED(pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn)) && uReturn > 0) {
            VARIANT vtProp;
            VariantInit(&vtProp);
            if (SUCCEEDED(pclsObj->Get(L"ConfiguredClockSpeed", 0, &vtProp, NULL, NULL))) {
                if (vtProp.vt == VT_I4) {
                    memorySpeed = vtProp.lVal;
                }
                VariantClear(&vtProp);
            }
            pclsObj->Release();
        }
        else{
            pEnumerator->Release();
            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
            throw std::runtime_error("Failed with HRESULT code " + hres);
        }
        pEnumerator->Release();
    }else{
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        throw std::runtime_error("Failed with HRESULT code " + hres);
    }

    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return memorySpeed;
}

unsigned short MemoryInfo::getNumUsedRAMSlots(){
    unsigned short usedSlots = 0;
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if(hres != 0){
        throw std::runtime_error("Failed with HRESULT code " + hres);
    }

    hres = CoInitializeSecurity(
        NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL
    );

    hres = CoInitializeSecurity(
        NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL
    );

    IWbemLocator* pLoc = NULL;
    hres = CoCreateInstance(
        CLSID_WbemLocator, NULL,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (void**)&pLoc
    );

    BSTR bstrNamespace = SysAllocString(L"ROOT\\CIMV2");
    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(bstrNamespace, NULL, NULL, 0, WBEM_FLAG_CONNECT_USE_MAX_WAIT, 0, 0, &pSvc);
    SysFreeString(bstrNamespace);
    
    if (hres != 0 || pSvc == NULL) {
        pLoc->Release();
        CoUninitialize();
        throw std::runtime_error("Failed with HRESULT code " + hres);
    }

    BSTR bstrLanguage = SysAllocString(L"WQL");
    BSTR bstrQuery = SysAllocString(L"SELECT * FROM Win32_PhysicalMemory");
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(bstrLanguage, bstrQuery, 
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    SysFreeString(bstrLanguage);
    SysFreeString(bstrQuery);
    
    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;

    while (pEnumerator) {
        hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (0 == uReturn) {
            break;
        }
        
        usedSlots++;
        pclsObj->Release();
    }

    pEnumerator->Release();
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();

    return usedSlots;
}

unsigned short MemoryInfo::getTotalRAMSlots(){
    unsigned short totalSlots = 0;
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if(hres != 0){
        throw std::runtime_error("Failed with HRESULT code " + hres);
    }

    hres = CoInitializeSecurity(
        NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL
    );

    hres = CoInitializeSecurity(
        NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL, EOAC_NONE, NULL
    );

    IWbemLocator* pLoc = NULL;
    hres = CoCreateInstance(
        CLSID_WbemLocator, NULL,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (void**)&pLoc
    );

    BSTR bstrNamespace = SysAllocString(L"ROOT\\CIMV2");
    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(bstrNamespace, NULL, NULL, 0, WBEM_FLAG_CONNECT_USE_MAX_WAIT, 0, 0, &pSvc);
    SysFreeString(bstrNamespace);
    
    if (hres != 0 || pSvc == NULL) {
        pLoc->Release();
        CoUninitialize();
        throw std::runtime_error("Failed with HRESULT code " + hres);
    }

    BSTR bstrLanguage = SysAllocString(L"WQL");
    BSTR bstrQuery = SysAllocString(L"SELECT MemoryDevices FROM Win32_PhysicalMemoryArray");
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(bstrLanguage, bstrQuery, 
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    SysFreeString(bstrLanguage);
    SysFreeString(bstrQuery);
    
    if (SUCCEEDED(hres) && pEnumerator != NULL) {
        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;
        
        if (SUCCEEDED(pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn)) && uReturn > 0) {
            VARIANT vtProp;
            VariantInit(&vtProp);
            if (SUCCEEDED(pclsObj->Get(L"MemoryDevices", 0, &vtProp, NULL, NULL))) {
                if (vtProp.vt == VT_I4) {
                    totalSlots = vtProp.lVal;
                }
                VariantClear(&vtProp);
            }
            pclsObj->Release();
        }
        else{
            pEnumerator->Release();
            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
            throw std::runtime_error("Enumeration failed");
        }
        pEnumerator->Release();
    }else{
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        throw std::runtime_error("Failed with HRESULT code " + hres);
    }

    pSvc->Release();
    pLoc->Release();
    CoUninitialize();

    return totalSlots;
}
