#include "hardware.hpp"
#include <comdef.h>
#include <wbemidl.h>
#include <intrin.h>
#include <vector>
#include <bitset>
#include <cstring>
#include <array>
#include <stdexcept>
#include <vector>

std::string Hardware::cpuName() noexcept{
    std::array<int, 4> info = {0};
    char brandString[49] = {0};

    __cpuid(info.data(), 0x80000000);
    unsigned int exIds = info[0];

    if(exIds > 0x80000004){
        __cpuid(info.data(), 0x80000002);
        std::memcpy(brandString, info.data(), sizeof(info));

        __cpuid(info.data(), 0x80000003);
        std::memcpy(brandString + 16, info.data(), sizeof(info));

        __cpuid(info.data(), 0x80000004);
        std::memcpy(brandString + 32, info.data(), sizeof(info));
    }
    else{
        return "Unkown Processor";
    }

    std::string name(brandString);
    auto first = name.find_first_not_of(" ");
    if(first != std::string::npos){
        name = name.substr(first);
    }
    return name;
}

std::wstring Hardware::memoryName(){
    std::wstring name = L"";
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);

    if(FAILED(hres)){
        throw std::runtime_error("CoInitializeEx Failed For Getting Memory Name");
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
        IID_IWbemLocator, (LPVOID*)&pLoc
    );

    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL, NULL, 0, WBEM_FLAG_CONNECT_USE_MAX_WAIT, 0, 0, &pSvc
    );
    pLoc->Release();

    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        _bstr_t(L"WQL"),
        _bstr_t(L"SELECT Manufacturer, PartNumber FROM Win32_PhysicalMemory"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL, &pEnumerator
    );

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;
    while (pEnumerator) {
        hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (uReturn == 0) break;

        VARIANT vtProp;
        
        VariantInit(&vtProp);
        pclsObj->Get(L"Manufacturer", 0, &vtProp, 0, 0);
        name += (vtProp.bstrVal ? vtProp.bstrVal : L"N/A");
        VariantClear(&vtProp);

        VariantInit(&vtProp);
        pclsObj->Get(L"PartNumber", 0, &vtProp, 0, 0);
        name += L" ";
        name += (vtProp.bstrVal ? vtProp.bstrVal : L"N/A");
        VariantClear(&vtProp);
        pclsObj->Release();
    }
    pSvc->Release();
    pEnumerator->Release();
    CoUninitialize();
    return name;
}

std::list<std::wstring> Hardware::diskManufacturerName(){
    std::list<std::wstring> names;

    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);

    if(FAILED(hres)){
        throw std::runtime_error("CoInitializeEx Failed For Getting Memory Name");
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
        IID_IWbemLocator, (LPVOID*)&pLoc
    );

    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL, NULL, 0, WBEM_FLAG_CONNECT_USE_MAX_WAIT, 0, 0, &pSvc
    );
    pLoc->Release();

    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        _bstr_t(L"WQL"),
        _bstr_t(L"SELECT Model FROM Win32_DiskDrive"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL, &pEnumerator
    );

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;
    while (pEnumerator) {
        std::wstring fullName = L"";
        hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (uReturn == 0) break;

        VARIANT vtProp;

        pclsObj->Get(L"Model", 0, &vtProp, 0, 0);
        fullName += (vtProp.bstrVal ? vtProp.bstrVal : L"Unknown Model");
        VariantClear(&vtProp);
        pclsObj->Release();
        names.push_back(fullName);
    }
    pSvc->Release();
    pEnumerator->Release();
    CoUninitialize();
    return names;
}

std::list<std::wstring> Hardware::internetAdapterNames(){
    std::list<std::wstring> names;
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);

    if(FAILED(hres)){
        throw std::runtime_error("CoInitializeEx Failed For Getting Memory Name");
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
        IID_IWbemLocator, (LPVOID*)&pLoc
    );

    IWbemServices* pSvc = NULL;
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL, NULL, 0, WBEM_FLAG_CONNECT_USE_MAX_WAIT, 0, 0, &pSvc
    );
    pLoc->Release();

    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        _bstr_t(L"WQL"),
        _bstr_t(L"SELECT Name, Manufacturer FROM Win32_NetworkAdapter WHERE NetConnectionStatus = 2"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL, &pEnumerator
    );

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;
    while (pEnumerator) {
        std::wstring fullName = L"";
        hres = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (uReturn == 0) break;

        VARIANT vtProp;

        pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
        fullName += (vtProp.bstrVal ? vtProp.bstrVal : L"Unknown Name");
        VariantClear(&vtProp);

        fullName += L" ";

        pclsObj->Get(L"Manufacturer", 0, &vtProp, 0, 0);
        fullName  += (vtProp.bstrVal ? vtProp.bstrVal : L"Unknown Manufacturer");
        VariantClear(&vtProp);
        pclsObj->Release();

        names.push_back(fullName);
    }
    pSvc->Release();
    pEnumerator->Release();
    CoUninitialize();
    return names;
}
