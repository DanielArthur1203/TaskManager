#include "disk_factory.hpp"
#include <windows.h>
#include <comdef.h>
#include <wbemidl.h>
#include <cwchar>
#include <stdexcept>

std::vector<std::wstring> DiskFactory::diskNames(){
    std::vector<std::wstring> names;

    DWORD count = GetLogicalDriveStringsW(0, NULL);
    if(count == 0){
        throw std::runtime_error("Getting Logical Drives");
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

std::vector<std::wstring> DiskFactory::diskManufacturerName(){
    std::vector<std::wstring> names;
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

std::vector<std::unique_ptr<Disk>> DiskFactory::makeDisks(){
    std::vector<std::unique_ptr<Disk>> disks;
    std::vector<std::wstring> names = diskNames();
    std::vector<std::wstring> manufacturerNames;

    try{
        manufacturerNames = diskManufacturerName();
    }
    catch(const std::runtime_error& e){
        manufacturerNames.assign(names.size(), L"N/A(Error Occurred)");
    }

    int i = 0;
    for(const auto& name: names){
        disks.push_back(std::make_unique<Disk>(name, i, manufacturerNames.at(i)));
        i++;
    }
    return disks;
}
