#include "gpu_factory.hpp"
#include <windows.h>
#include <pdh.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <comdef.h>
#include <wbemidl.h>
#include <intrin.h>
#include <stdexcept>

std::vector<LUID> GpuFactory::getLuids(){
    std::vector<LUID> luids;
    IDXGIFactory1* fac = NULL;
    HRESULT res = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&fac);

    if(FAILED(res)){
        throw std::runtime_error("DXGI Factory Creation Failed With DXGI Error " + std::to_string(res));
    }

    IDXGIAdapter1* adapt = NULL;
    unsigned int idx = 0;

    while(fac->EnumAdapters1(idx, &adapt) != DXGI_ERROR_NOT_FOUND){
        DXGI_ADAPTER_DESC1 desc;
        adapt->GetDesc1(&desc);

        if(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE){
            adapt->Release();
            ++idx;
            continue;
        }
        luids.push_back(desc.AdapterLuid);
        adapt->Release();
        ++idx;
    }
    fac->Release();
    return luids;
}

std::vector<uint64_t> GpuFactory::deviceIds(){
    std::vector<uint64_t> values;
    IDXGIFactory1* fac = NULL;
    HRESULT res = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&fac);

    if(FAILED(res)){
        throw std::runtime_error("DXGI Factory Creation Failed With DXGI Error " + std::to_string(res));
    }

    IDXGIAdapter1* adapt = NULL;
    unsigned int idx = 0;

    while(fac->EnumAdapters1(idx, &adapt) != DXGI_ERROR_NOT_FOUND){
        DXGI_ADAPTER_DESC1 desc;
        adapt->GetDesc1(&desc);

        if(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE){
            adapt->Release();
            ++idx;
            continue;
        }
        unsigned long val = static_cast<uint64_t>(desc.AdapterLuid.LowPart) 
            | static_cast<uint64_t>((static_cast<uint64_t>(desc.AdapterLuid.HighPart)) << 32);
        values.push_back(val);
        adapt->Release();
        ++idx;
    }
    fac->Release();
    return values;
}

std::vector<std::wstring> GpuFactory::gpuNames(){
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
    
    hres = CoSetProxyBlanket(
        pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
        NULL, RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE
    );

    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        _bstr_t(L"WQL"),
        _bstr_t(L"SELECT AdapterCompatibility, Name FROM Win32_VideoController"),
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

        pclsObj->Get(L"AdapterCompatibility", 0, &vtProp, 0, 0);
        fullName += (vtProp.bstrVal ? vtProp.bstrVal : L"Unknown Manufacturer");
        VariantClear(&vtProp);

        fullName += L" ";

        pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
        fullName  += (vtProp.bstrVal ? vtProp.bstrVal : L"Unknown Name");
        VariantClear(&vtProp);
        pclsObj->Release();

        names.push_back(fullName);
    }
    pSvc->Release();
    pEnumerator->Release();
    CoUninitialize();
    return names;
}

std::vector<std::unique_ptr<GPU>> GpuFactory::makeGPUs(){
    std::vector<std::unique_ptr<GPU>> gpus;
    std::vector<LUID> luids = getLuids();
    std::vector<uint64_t> ids = deviceIds();
    std::vector<std::wstring> names = gpuNames();

    int i = 0;
    for(const auto& id: ids){
        gpus.push_back(std::make_unique<GPU>(id, luids.at(i), names.at(i)));
        i++;
    }
    return gpus;
}
