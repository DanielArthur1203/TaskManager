#include "cpu.hpp"
#include <windows.h>
#include <winternl.h>
#include <sysinfoapi.h>
#include <powrprof.h>
#include <pdh.h>
#include <comdef.h>
#include <wbemidl.h>
#include <oleauto.h>
#include <chrono>
#include <thread>
#include <cmath>
#include <numeric>
#include <future>
#include <list>

using namespace std::chrono;

typedef struct _PROCESSOR_POWER_INFORMATION {
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

unsigned long long Cpu::fileTimeToULL(const FILETIME &ft){
    return (static_cast<unsigned long long>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}

double Cpu::getBaseSpeed(){
    HKEY hKey;
    DWORD mhz = 0;
    DWORD mhzSize = sizeof(mhz);
    
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"~MHz", nullptr, nullptr, reinterpret_cast<LPBYTE>(&mhz), &mhzSize);
        RegCloseKey(hKey);
    }
    else{
        RegCloseKey(hKey);
        throw std::runtime_error("Opening Registry Key Failed");
    }
    return static_cast<double>(mhz) / 1000.0;
}

std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> Cpu::getProcessorInfo(){
    DWORD buffer = 0;
    
    //Meant to fail
    if((GetLogicalProcessorInformation(nullptr, &buffer))){
        throw std::runtime_error(processes.formattedError("Getting Buffer Size"));
    }

    std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> infoVec(buffer / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));

    if(!(GetLogicalProcessorInformation(infoVec.data(), &buffer))){
        throw std::runtime_error(processes.formattedError("Getting Logical Processor Info"));
    }

    return infoVec;
}

unsigned long long Cpu::getSnap(DWORD pid){
    unsigned long long usage = 0;
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid);
    
    if(h == NULL){
        throw std::runtime_error(processes.formattedError("Opening Process " + pid));
    }

    FILETIME creation, exit, kernel, user;

    if(!(GetProcessTimes(h, &creation, &exit, &kernel, &user))){
        CloseHandle(h);
        throw std::runtime_error(processes.formattedError("Getting Process Times For Process " + pid));
    }

    usage += fileTimeToULL(kernel) + fileTimeToULL(user);
    CloseHandle(h);
    return usage;
}

double Cpu::currentUsage(){
    FILETIME idle, kernel, user;

    if(!GetSystemTimes(&idle, &kernel, &user)){
        throw std::runtime_error(processes.formattedError("Getting System Times"));
    }
    std::this_thread::sleep_for(seconds(1));

    FILETIME idle2, kernel2, user2;

    if(!GetSystemTimes(&idle2, &kernel2, &user2)){
        throw std::runtime_error(processes.formattedError("Getting System Times"));
    }

    auto i1 = fileTimeToULL(idle);
    auto k1 = fileTimeToULL(kernel);
    auto u1 = fileTimeToULL(user);

    auto i2 = fileTimeToULL(idle2);
    auto k2 = fileTimeToULL(kernel2);
    auto u2 = fileTimeToULL(user2);

    auto iD = i2 - i1;
    auto kD = k2 - k1;
    auto uD = u2 - u1;

    auto totalSysTime = kD + uD;

    auto returnUsage = std::floor(static_cast<double>(totalSysTime - iD) * 100.0 / totalSysTime);
    return returnUsage;
}

double Cpu::processUsage(DWORD pid){
    double usage = 0;
    
    unsigned long long snap1 = 0;

    try{
        snap1 = getSnap(pid);
    }
    catch(std::runtime_error& e){
        throw std::runtime_error(e.what());
    }

    ULONGLONG time1 = GetTickCount64();

    std::this_thread::sleep_for(seconds(1));

    unsigned long long snap2 = 0;

    try{
        snap2 = getSnap(pid);
    }
    catch(std::runtime_error& e){
        throw std::runtime_error(e.what());
    }

    ULONGLONG time2 = GetTickCount64();

    ULONGLONG tDelta = time2 - time1;

    auto snapDelta = snap2 - snap1;

    usage = static_cast<double>((snapDelta / (tDelta * 10000.0)));
    usage /= processorCount();
    usage *= 100;
    usage = std::round(usage * 100) / 100;
    return usage;
}

double Cpu::processNameTotalUsage(DWORD pid){
    double usage = 0;

    std::wstring singleName = processes.getNameFromPID(pid);
    std::vector<DWORD> allPIDS = processes.getPIDFromName(singleName);
    std::list<std::future<double>> results;

    for(auto it = allPIDS.begin(); it != allPIDS.end(); ++it){
        try{
            DWORD id = *it;
            results.push_back(std::async(std::launch::async, [this, id](){
                return processUsage(id);
            }));
        } 
        catch(std::runtime_error& e){
            throw std::runtime_error(e.what());
        }
    }

    for(auto it = results.begin(); it != results.end(); ++it){
        usage += it->get();
    }
    usage = std::round(usage * 10) / 10;
    return usage;
}

double Cpu::cpuClockSpeed(){
    double speed = 0;
    double baseSpeed = getBaseSpeed();
    PDH_HQUERY phQ = nullptr;
    PDH_HCOUNTER phC = nullptr;

    PDH_STATUS status = PdhOpenQuery(nullptr, 0, &phQ);
    if(status != ERROR_SUCCESS){
        throw std::runtime_error(processes.formattedError("Query Opening", status));
    }

    LPCWSTR counter = L"\\Processor Information(_Total)\\% Processor Performance";

    status = PdhAddEnglishCounterW(phQ, counter, 0, &phC);
    if(status != ERROR_SUCCESS){
        PdhCloseQuery(phQ);
        throw std::runtime_error(processes.formattedError("Adding English Counter", status));
    }

    status = PdhCollectQueryData(phQ);
    if(status != ERROR_SUCCESS){
        PdhRemoveCounter(phC);
        PdhCloseQuery(phQ);
        throw std::runtime_error(processes.formattedError("Collecting Query Data", status));
    }

    std::this_thread::sleep_for(seconds(1));

    status = PdhCollectQueryData(phQ);
    if(status != ERROR_SUCCESS){
        PdhRemoveCounter(phC);
        PdhCloseQuery(phQ);
        throw std::runtime_error(processes.formattedError("Collecting Query Data", status));
    }

    PDH_FMT_COUNTERVALUE cValue = PDH_FMT_COUNTERVALUE();
    DWORD type;

    status = PdhGetFormattedCounterValue(phC, PDH_FMT_DOUBLE, &type, &cValue);
    if(status != ERROR_SUCCESS){
        PdhRemoveCounter(phC);
        PdhCloseQuery(phQ);
        throw std::runtime_error(processes.formattedError("Formatting Counter Value", status));
    }

    speed = cValue.doubleValue / 100.0;
    speed *= baseSpeed;
    speed = std::round(speed * 100) / 100;

    PdhRemoveCounter(phC);
    PdhCloseQuery(phQ);
    return speed;
}

unsigned int Cpu::processorCount(){
    unsigned int count = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    if(count == 0){
        throw std::runtime_error(processes.formattedError("Retrieving Active Processors"));
    }
    return count;
}

unsigned int Cpu::threadCount(){
    unsigned int threads = 0;
    std::vector<PROCESSENTRY32> activeP = std::vector<PROCESSENTRY32>();

    try{
        activeP = processes.getAllActiveProcesses();
    }
    catch(const std::runtime_error& e){
        throw std::runtime_error(e.what());
    }

    threads = std::accumulate(activeP.begin(), activeP.end(), 0,
        [](int sum, const PROCESSENTRY32& p32){
            return sum + p32.cntThreads;
    });

    return threads;
}

unsigned int Cpu::handleCount(){
    unsigned int count = 0;
    std::vector<PROCESSENTRY32> activeP = std::vector<PROCESSENTRY32>();

    try{
        activeP = processes.getAllActiveProcesses();
    }
    catch(const std::runtime_error& e){
        throw std::runtime_error(e.what());
    }

    for(auto it = activeP.begin(); it != activeP.end(); ++it){
        DWORD pid = it->th32ProcessID;

        HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);

        if(handle != NULL){
            DWORD hCount = 0; 
            if(GetProcessHandleCount(handle, &hCount)){
                count += hCount;
            }
            else{
                CloseHandle(handle);
                throw std::runtime_error("Getting Handle Count For " + pid);
            }
            CloseHandle(handle);
        }
        else{
            continue;
        }
    }
    return count;
}

std::string Cpu::upTime(){
    std::string upTime = "";

    ULONGLONG ticks = GetTickCount64();
    ULONGLONG sec = ticks / 1000;
    ULONGLONG days = sec / (24 * 3600);
    sec %= (24 * 3600);
    ULONGLONG hours = sec / 3600;
    sec %= 3600;
    ULONGLONG min = sec / 60;
    sec %= 60;

    upTime.append(std::to_string(static_cast<int>(days)));
    upTime += ":";
    upTime.append(std::to_string(static_cast<int>(hours)));
    upTime += ":";
    upTime.append(std::to_string(static_cast<int>(min)));;
    upTime += ":";
    upTime.append(std::to_string(static_cast<int>(sec)));;
    return upTime;
}

std::array<double, 3> Cpu::cacheAmounts(){
    auto caches = std::array<double, 3>();
    double L1 = 0;
    double L2 = 0;
    double L3 = 0;
    std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> pInfo;

    try{
        pInfo = getProcessorInfo();
    }
    catch(const std::runtime_error& e){
        throw std::runtime_error(e.what());
    }

    for(const auto& info: pInfo){
        if(info.Relationship == RelationCache){
            const CACHE_DESCRIPTOR& cache = info.Cache;
            
            switch(cache.Level){
                case 1:
                    //L1 is usually in KiB
                    L1 += (cache.Size / 1024.0);
                    break;
                case 2:
                    //L2 and L3 is usually in MiB
                    L2 += (cache.Size / (1024.0 * 1024.0));
                    break;
                case 3:
                    L3 += (cache.Size / (1024.0 * 1024.0));
                    break;
            }
        }
    }
    caches[0] = L1;
    caches[1] = L2;
    caches[2] = L3;
    return caches;
}

unsigned short Cpu::coreCount(){
    unsigned short count = 0;
    std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> pInfo;

    try{
        pInfo = getProcessorInfo();
    }
    catch(const std::runtime_error& e){
        throw std::runtime_error(e.what());
    }

    for(const auto& info: pInfo){
        if(info.Relationship == RelationProcessorCore){
            count++;
        }
    }
    return count;
}

// bool Cpu::virtualizationState(){
//     bool isEnabled = false;
//     HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
//     if(hres != 0){
//         throw std::runtime_error("Failed with HRESULT code " + hres);
//     }

//     hres = CoInitializeSecurity(
//         NULL, -1, NULL, NULL,
//         RPC_C_AUTHN_LEVEL_DEFAULT,
//         RPC_C_IMP_LEVEL_IMPERSONATE,
//         NULL, EOAC_NONE, NULL
//     );

//     IWbemLocator* pLoc = NULL;
//     hres = CoCreateInstance(
//         CLSID_WbemLocator, NULL,
//         CLSCTX_INPROC_SERVER,
//         IID_IWbemLocator, (void**)&pLoc
//     );
//     if (hres != 0 || pLoc == NULL) {
//         CoUninitialize();
//         throw std::runtime_error("Failed with HRESULT code " + hres);
//     }

//     // Use SysAllocString instead of _bstr_t for MinGW compatibility
//     BSTR bstrNamespace = SysAllocString(L"ROOT\\CIMV2");
//     IWbemServices* pSvc = NULL;
//     hres = pLoc->ConnectServer(bstrNamespace, NULL, NULL, 0, WBEM_FLAG_CONNECT_USE_MAX_WAIT, 0, 0, &pSvc);
//     SysFreeString(bstrNamespace);
    
//     if (hres != 0 || pSvc == NULL) {
//         pLoc->Release();
//         CoUninitialize();
//         throw std::runtime_error("Failed with HRESULT code " + hres);
//     }

//     BSTR bstrLanguage = SysAllocString(L"WQL");
//     BSTR bstrQuery = SysAllocString(L"SELECT VirtualizationFirmwareEnabled FROM Win32_Processor");
//     IEnumWbemClassObject* pEnumerator = NULL;
//     hres = pSvc->ExecQuery(bstrLanguage, bstrQuery, 
//         WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
//     SysFreeString(bstrLanguage);
//     SysFreeString(bstrQuery);

//     if (SUCCEEDED(hres) && pEnumerator != NULL) {
//         IWbemClassObject* pclsObj = NULL;
//         ULONG uReturn = 0;
        
//         if (SUCCEEDED(pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn))) {
//             VARIANT vtProp;
//             VariantInit(&vtProp);
//             if (SUCCEEDED(pclsObj->Get(L"VirtualizationFirmwareEnabled", 0, &vtProp, NULL, NULL))) {
//                 if (vtProp.vt == VT_BOOL) {
//                     isEnabled = (vtProp.boolVal != VARIANT_FALSE);
//                 }
//                 VariantClear(&vtProp);
//             }
//             pclsObj->Release();
//         }
//         else{
//             pEnumerator->Release();
//             pSvc->Release();
//             pLoc->Release();
//             CoUninitialize();
//             throw std::runtime_error("Failed with HRESULT code " + hres);
//         }
//         pEnumerator->Release();
//     }else{
//         pSvc->Release();
//         pLoc->Release();
//         CoUninitialize();
//         throw std::runtime_error("Failed with HRESULT code " + hres);
//     }

//     pSvc->Release();
//     pLoc->Release();
//     CoUninitialize();

//     return isEnabled;
// }
