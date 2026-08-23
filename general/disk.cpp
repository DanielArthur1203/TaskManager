#include "disk.hpp"
#include <windows.h>
#include <winioctl.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <pdh.h>
#include <sysinfoapi.h>
#include <comdef.h>
#include <wbemidl.h>
#include <oleauto.h>
#include <string>
#include <chrono>
#include <thread>

LPCSTR Disk::wstrToLPCSTR(std::wstring &string){
    int size = WideCharToMultiByte(CP_UTF8, 0, string.c_str(), (int)string.length(), NULL, 0, NULL, NULL);

    static thread_local std::string str;
    str.assign(size, '\0');

    if (size > 0) {
        WideCharToMultiByte(CP_UTF8, 0, string.c_str(), (int)string.length(), str.data(), size, NULL, NULL);
    }

    return str.c_str();
}

std::vector<std::wstring> Disk::diskNames(){
    std::vector<std::wstring> names;

    DWORD count = GetLogicalDriveStringsW(0, NULL);
    if(count == 0){
        throw std::runtime_error("Get Logical Drives");
    }

    std::wstring buff(count, L'\0');
    GetLogicalDriveStringsW(count, &buff[0]);

    wchar_t* p = &buff[0];
    while (*p) {
        names.push_back(p);
        p += wcslen(p) + 1;
    }
    return names;
}

int Disk::diskNumberFromName(std::wstring &name){
    int num = 0;
    std::wstring path = L"\\\\.\\" + name.erase(2); 
    
    HANDLE h = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING, 0, NULL);
    
    if(h == INVALID_HANDLE_VALUE){
        throw std::runtime_error(processes.formattedError("Creating File For Disk Number"));
    }

    VOLUME_DISK_EXTENTS vol = VOLUME_DISK_EXTENTS{};
    DWORD bytes = 0;

    if(!(DeviceIoControl(h, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0,
        &vol, sizeof(vol), &bytes, NULL))){
        
        CloseHandle(h);
        throw std::runtime_error(processes.formattedError("Getting Volume Disk For Disk Number"));
    }

    num = vol.Extents[0].DiskNumber;
    CloseHandle(h);
    return num;
}

std::unordered_map<std::wstring, unsigned int> Disk::activeTime(){
    auto map = std::unordered_map<std::wstring, unsigned int>();
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

    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, 
        NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    
    if(FAILED(hres)){
        pLoc->Release();
        pSvc->Release();
        CoUninitialize();
        throw std::runtime_error("Failed with HRESULT code " + hres);
    }

    BSTR bstrLanguage = SysAllocString(L"WQL");
    BSTR bstrQuery = SysAllocString(L"SELECT Name, PercentIdleTime FROM Win32_PerfFormattedData_PerfDisk_PhysicalDisk");
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

        VARIANT name;
        VARIANT time;

        pclsObj->Get(L"Name", 0, &name, 0, 0);
        pclsObj->Get(L"PercentIdleTime", 0, &time, 0, 0);
        

        map.insert({name.bstrVal, 100 - std::stoi(time.bstrVal)});

        VariantClear(&name);
        VariantClear(&time);
        pclsObj->Release();
    }

    pEnumerator->Release();
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return map;
}

std::unordered_map<std::wstring, double> Disk::readSpeed(){
    std::unordered_map<std::wstring, double> speed;
    
    for(auto it = names.begin(); it != names.end(); ++it){
        it->erase(2); //Each name is formatted as L"C:\\"
        std::wstring path = L"\\\\.\\" + *it;

        HANDLE h = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES, 
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
            0, NULL);
        
        if(h == INVALID_HANDLE_VALUE){
            throw std::runtime_error(processes.formattedError("Create File Handle"));
        }

        DISK_PERFORMANCE p = DISK_PERFORMANCE{};
        DWORD bytesRet = 0;

        if(!(DeviceIoControl(h, IOCTL_DISK_PERFORMANCE, NULL, 0, &p, sizeof(p), &bytesRet, NULL))){
            CloseHandle(h);
            throw std::runtime_error(processes.formattedError("Getting Disk Performance"));
        }

        unsigned long long b1 = p.BytesRead.QuadPart;

        auto start = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds(1));

        p = DISK_PERFORMANCE{};
        bytesRet = 0;

        if(!(DeviceIoControl(h, IOCTL_DISK_PERFORMANCE, NULL, 0, &p, sizeof(p), &bytesRet, NULL))){
            CloseHandle(h);
            throw std::runtime_error(processes.formattedError("Getting Disk Performance"));
        }

        unsigned long long b2 = p.BytesRead.QuadPart;
        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<double> sec = end - start;
        double rate = static_cast<double>((b2 - b1)) / (sec.count());
        speed.insert({*it, rate});
        CloseHandle(h);
    }

    return speed;
}

std::unordered_map<std::wstring, double> Disk::writeSpeed(){
    std::unordered_map<std::wstring, double> speed;
    
    for(auto it = names.begin(); it != names.end(); ++it){
        it->erase(2); //Each name is formatted as L"C:\\"
        std::wstring path = L"\\\\.\\" + *it;

        HANDLE h = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES, 
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
            0, NULL);
        
        if(h == INVALID_HANDLE_VALUE){
            throw std::runtime_error(processes.formattedError("Create File Handle"));
        }

        DISK_PERFORMANCE p = DISK_PERFORMANCE{};
        DWORD bytesRet = 0;

        if(!(DeviceIoControl(h, IOCTL_DISK_PERFORMANCE, NULL, 0, &p, sizeof(p), &bytesRet, NULL))){
            CloseHandle(h);
            throw std::runtime_error(processes.formattedError("Getting Disk Performance"));
        }

        unsigned long long b1 = p.BytesWritten.QuadPart;

        auto start = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::seconds(1));

        p = DISK_PERFORMANCE{};
        bytesRet = 0;

        if(!(DeviceIoControl(h, IOCTL_DISK_PERFORMANCE, NULL, 0, &p, sizeof(p), &bytesRet, NULL))){
            CloseHandle(h);
            throw std::runtime_error(processes.formattedError("Getting Disk Performance"));
        }

        unsigned long long b2 = p.BytesWritten.QuadPart;
        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<double> sec = end - start;
        double rate = static_cast<double>((b2 - b1)) / (sec.count());
        speed.insert({*it, rate});
        CloseHandle(h);
    }

    return speed;
}

std::unordered_map<std::wstring, double> Disk::responseTime(){
    std::unordered_map<std::wstring, double> rTime;
    

    for(auto it = names.begin(); it != names.end(); ++it){
        PDH_HQUERY ph;
        PDH_HCOUNTER hc;
        PDH_FMT_COUNTERVALUE val;

        it->erase(2); //Each name is formatted as L"C:\\"
        if(PdhOpenQuery(NULL, 0, &ph) != ERROR_SUCCESS){
            throw std::runtime_error(processes.formattedError("Opening Query For Disk", true));
        }

        std::wstring path = L"\\PhysicalDisk(0 " + *it + L")\\Avg. Disk sec/Transfer";
        LPCSTR ex = wstrToLPCSTR(path);
        if(PdhAddCounter(ph, wstrToLPCSTR(path), 0, &hc) != ERROR_SUCCESS){
            PdhCloseQuery(ph);
            throw std::runtime_error(processes.formattedError("Adding Counter For Disk Rsp. Time", true));
        }

        PdhCollectQueryData(ph);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        PdhCollectQueryData(ph);

        if(PdhGetFormattedCounterValue(hc, PDH_FMT_DOUBLE, NULL, &val) != ERROR_SUCCESS){
            PdhCloseQuery(ph);
            throw std::runtime_error(processes.formattedError("Getting Disk Rsp. Counter Value", true));
        }

        rTime.insert({*it, val.doubleValue * 1000.0});
        PdhCloseQuery(ph);
    }
    return rTime;
}

std::unordered_map<std::wstring, unsigned int> Disk::capacity(){
    std::unordered_map<std::wstring, unsigned int> capacity;
    

    for(auto it = names.begin(); it != names.end(); ++it){
        ULARGE_INTEGER total;

        if(!(GetDiskFreeSpaceExW(it->c_str(), NULL, &total, NULL))){
            throw std::runtime_error(processes.formattedError("Getting Free Disk Space"));
        }

        unsigned int GiB = total.QuadPart / (1024 * 1024 * 1024);
        capacity.insert({it->erase(2), GiB});
    }

    return capacity;
}

std::unordered_map<std::wstring, int> Disk::type(){
    std::unordered_map<std::wstring, int> type;

    for(auto it = names.begin(); it != names.end(); ++it){
        unsigned int a = GetDriveTypeA(wstrToLPCSTR(*it));
        type.insert({it->erase(2), a});
    }
    return type;
}
