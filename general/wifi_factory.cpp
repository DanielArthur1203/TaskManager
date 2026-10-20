#include "wifi_factory.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2ipdef.h>
#include <windows.h>
#include <comdef.h>
#include <wbemidl.h>
#include <intrin.h>
#include <windot11.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <wlanapi.h>
#include <memory>
#include <stdexcept>

std::vector<std::wstring> WifiFactory::internetAdapterNames(){
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

std::vector<unsigned long> WifiFactory::getAdapterIndices(){
    std::vector<unsigned long> indices;
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
    ULONG fam = AF_UNSPEC;
    ULONG buff = 15000;

    std::unique_ptr<IP_ADAPTER_ADDRESSES, void(*)(void*)> pAddresses(
        static_cast<PIP_ADAPTER_ADDRESSES>(std::malloc(buff)), 
        std::free
    );
    
    if(!pAddresses){
        throw std::runtime_error("Memory Allocation Failed");
    }

    DWORD res = GetAdaptersAddresses(fam, flags, NULL, pAddresses.get(), &buff);

    if(res == ERROR_BUFFER_OVERFLOW){
        pAddresses.reset(static_cast<PIP_ADAPTER_ADDRESSES>(std::malloc(buff)));

        if(!pAddresses){
            throw std::runtime_error("Memory Allocation Failed");
        }

        res = GetAdaptersAddresses(fam, flags, NULL, pAddresses.get(), &buff);
    }

    if(res != NO_ERROR){
        throw std::runtime_error("Getting Adapters Failed With iphlpapi Code " + std::to_string(res));
    }

    PIP_ADAPTER_ADDRESSES curr = pAddresses.get();
    while(curr){
        if(curr->IfType == IF_TYPE_IEEE80211 && curr->OperStatus == IfOperStatusUp){
            indices.push_back(curr->IfIndex);
        }
        curr = curr->Next;
    }
    return indices;
}

std::vector<std::unique_ptr<WiFi>> WifiFactory::makeWiFi(){
    //I do realize that I treat these as noexcept when they're not in all factories
    //It's just b/c if one is thrown the program becomes FUBAR and I don't see a way other than just crashing to let it continue
    std::vector<std::unique_ptr<WiFi>> wifi;
    auto names = internetAdapterNames();
    auto indicies = getAdapterIndices();

    int i = 0;
    for(const auto& index: indicies){
        wifi.push_back(std::make_unique<WiFi>(index, names.at(i)));
        i++;
    }
    return wifi;
}
