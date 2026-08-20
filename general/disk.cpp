#include "disk.hpp"
#include <windows.h>
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

std::unordered_map<std::wstring, unsigned int> Disk::activeTime()
{
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

double Disk::readSpeed(){
    double speed = 0;
    HQUERY q;
    HCOUNTER hc;

    if(PdhOpenQuery(NULL, 0, &q) != ERROR_SUCCESS){
        throw std::runtime_error(processes.formattedError("Opening Query", true));
    }

    if(PdhAddCounter(q, "\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", 0, &hc) != ERROR_SUCCESS){
        PdhCloseQuery(q);
        throw std::runtime_error(processes.formattedError("Adding Counter", true));
    }

    PdhCollectQueryData(q);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    PdhCollectQueryData(q);

    PDH_FMT_COUNTERVALUE ph = PDH_FMT_COUNTERVALUE{};
    if(PdhGetFormattedCounterValue(hc, PDH_FMT_DOUBLE, NULL, &ph) != ERROR_SUCCESS){
        PdhCloseQuery(q);
        throw std::runtime_error(processes.formattedError("Getting Formatted Counter", true));
    }

    speed = ph.doubleValue;

    PdhCloseQuery(q);
    return speed;
}
