#include "cpu.hpp"
#include <sysinfoapi.h>
#include <powrprof.h>
#include <pdh.h>
#include <chrono>
#include <thread>
#include <cmath>
#include <numeric>

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

double Cpu::currentUsage()
{
    FILETIME idle, kernel, user;

    if(!GetSystemTimes(&idle, &kernel, &user)){
        throw std::runtime_error(processes.formattedError("Getting System Times"));
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));

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

double Cpu::cpuClockSpeed(){
    double speed = 0;
    double baseSpeed = getBaseSpeed();
    PDH_HQUERY phQ = nullptr;
    PDH_HCOUNTER phC = nullptr;

    if(PdhOpenQuery(nullptr, 0, &phQ) != ERROR_SUCCESS){
        throw std::runtime_error(processes.formattedError("Query Opening", true));
    }

    LPCWSTR counter = L"\\Processor Information(_Total)\\% Processor Performance";

    if(PdhAddEnglishCounterW(phQ, counter, 0, &phC) != ERROR_SUCCESS){
        PdhCloseQuery(phQ);
        throw std::runtime_error(processes.formattedError("Adding English Counter", true));
    }

    if(PdhCollectQueryData(phQ) != ERROR_SUCCESS){
        PdhRemoveCounter(phC);
        PdhCloseQuery(phQ);
        throw std::runtime_error(processes.formattedError("Collecting Query Data", true));
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    if(PdhCollectQueryData(phQ) != ERROR_SUCCESS){
        PdhRemoveCounter(phC);
        PdhCloseQuery(phQ);
        throw std::runtime_error(processes.formattedError("Collecting Query Data", true));
    }

    PDH_FMT_COUNTERVALUE cValue = PDH_FMT_COUNTERVALUE();
    DWORD type;

    if(PdhGetFormattedCounterValue(phC, PDH_FMT_DOUBLE, &type, &cValue) != ERROR_SUCCESS){
        PdhRemoveCounter(phC);
        PdhCloseQuery(phQ);
        throw std::runtime_error(processes.formattedError("Formatting Counter Value", true));
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
