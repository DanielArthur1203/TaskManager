#include "wifi.hpp"
#include <winsock2.h>
#include <ws2ipdef.h>
#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <wlanapi.h>
#include <memory>
#include <thread>
#include <chrono>
#include <cmath>
#include <string>

using namespace std::chrono;

std::vector<unsigned long> WiFi::getAdapterIndices(){
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
        throw std::runtime_error("Getting Adapters Failed With iphlpapi Code " + res);
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

std::unordered_map<unsigned long, double> WiFi::sendRate(){
    std::unordered_map<unsigned long, double> rates;

    for(auto it = adapterIndices.begin(); it != adapterIndices.end(); ++it){
        double bPs = 0;
        MIB_IF_ROW2 r1 = {0};
        r1.InterfaceIndex = *it;

        DWORD res = GetIfEntry2(&r1);
        if(res != NO_ERROR){
            throw std::runtime_error("Send Rate Retrieval Failed With netioapi Code " + res);
        }

        unsigned long long b1 = r1.OutOctets;

        auto start = steady_clock::now();
        std::this_thread::sleep_for(seconds(1));

        MIB_IF_ROW2 r2 = {0};
        r2.InterfaceIndex = *it;

        res = GetIfEntry2(&r2);

        if(res != NO_ERROR){
            throw std::runtime_error("Send Rate Retrieval Failed With netioapi Code " + res);
        }

        unsigned long long b2 = r2.OutOctets;
        auto end = steady_clock::now();

        duration<double> sec = end - start;
        bPs = (b2 > b1) ? static_cast<double>((b2 - b1)) / sec.count() : 0;
        bPs = std::round(bPs * 100) / 100;
        rates.insert({*it, bPs * 8}); //bits to bytes
    }
    return rates;
}

std::unordered_map<unsigned long, double> WiFi::receiveRate(){
    std::unordered_map<unsigned long, double> rates;

    for(auto it = adapterIndices.begin(); it != adapterIndices.end(); ++it){
        double bPs = 0;
        MIB_IF_ROW2 r1 = {0};
        r1.InterfaceIndex = *it;

        DWORD res = GetIfEntry2(&r1);
        if(res != NO_ERROR){
            throw std::runtime_error("Receive Rate Retrieval Failed With netioapi Code " + res);
        }

        unsigned long long b1 = r1.InOctets;

        auto start = steady_clock::now();
        std::this_thread::sleep_for(seconds(1));

        MIB_IF_ROW2 r2 = {0};
        r2.InterfaceIndex = *it;

        res = GetIfEntry2(&r2);

        if(res != NO_ERROR){
            throw std::runtime_error("Receive Rate Retrieval Failed With netioapi Code " + res);
        }

        unsigned long long b2 = r2.InOctets;
        auto end = steady_clock::now();

        duration<double> sec = end - start;
        bPs = (b2 > b1) ? static_cast<double>((b2 - b1)) / sec.count() : 0;
        bPs = std::round(bPs * 100) / 100;
        rates.insert({*it, bPs * 8}); //bits to bytes
    }
    return rates;
}

std::wstring WiFi::SSID(){
    std::wstring ssid = L"";
    HANDLE h;
    DWORD version = 0;
    DWORD size = 0;
    PWLAN_INTERFACE_INFO_LIST pL = PWLAN_INTERFACE_INFO_LIST{};
    PWLAN_CONNECTION_ATTRIBUTES pA = PWLAN_CONNECTION_ATTRIBUTES{};
    WLAN_OPCODE_VALUE_TYPE oT = wlan_opcode_value_type_invalid;

    DWORD res = WlanOpenHandle(2, NULL, &version, &h);
    if(res != ERROR_SUCCESS){
        switch(res){
            case ERROR_INVALID_PARAMETER:
                throw std::runtime_error("Invalid Param To Open WLAN Handle");
                break;
            case ERROR_INVALID_HANDLE:
                throw std::runtime_error("Invalid Handle To Open WLAN Handle");
                break;
            case ERROR_NOT_FOUND:
                throw std::runtime_error("Profile Not Found To Open WLAN Handle");
                break;
            case ERROR_ACCESS_DENIED:
                throw std::runtime_error("Invalid Access To Open WLAN Handle");
                break;
            case ERROR_NOT_SUPPORTED:
                throw std::runtime_error("Current Platform Unsupported To Open WLAN Handle");
                break;
            default:
                throw std::runtime_error("Unknown Error To Open WLAN Handle");
                break;
        }
    }

    res = WlanEnumInterfaces(h, NULL, &pL);

    if(res != ERROR_SUCCESS){
        WlanCloseHandle(h, NULL);
        switch(res){
            case ERROR_INVALID_PARAMETER:
                throw std::runtime_error("Invalid Param To Open WLAN Handle");
                break;
            case ERROR_INVALID_HANDLE:
                throw std::runtime_error("Invalid Handle To Open WLAN Handle");
                break;
            case ERROR_NOT_FOUND:
                throw std::runtime_error("Profile Not Found To Open WLAN Handle");
                break;
            case ERROR_ACCESS_DENIED:
                throw std::runtime_error("Invalid Access To Open WLAN Handle");
                break;
            case ERROR_NOT_SUPPORTED:
                throw std::runtime_error("Current Platform Unsupported To Open WLAN Handle");
                break;
            default:
                throw std::runtime_error("Unknown Error To Open WLAN Handle");
                break;
        }
    }
    
    if(pL->dwNumberOfItems){
        GUID g = pL->InterfaceInfo[0].InterfaceGuid;

        if(WlanQueryInterface(h, &g, wlan_intf_opcode_current_connection, NULL, &size, (PVOID*)&pA, &oT) == ERROR_SUCCESS){
            DOT11_SSID d11 = pA->wlanAssociationAttributes.dot11Ssid;

            if(d11.uSSIDLength > 0){
                for(auto i = 0; i < d11.uSSIDLength; i++){
                    ssid += (wchar_t)d11.ucSSID[i];
                }
            }
        }
        WlanFreeMemory(pA);
    }

    if(pL) WlanFreeMemory(pL);
    WlanCloseHandle(h, NULL);
    return ssid;
}

// std::unordered_map<unsigned long, std::string> WiFi::connectionType(){
//     std::unordered_map<unsigned long, std::string> type;
//     ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
//     ULONG fam = AF_UNSPEC;
//     ULONG buff = 15000;

//     std::unique_ptr<IP_ADAPTER_ADDRESSES, void(*)(void*)> pAddresses(
//         static_cast<PIP_ADAPTER_ADDRESSES>(std::malloc(buff)), 
//         std::free
//     );

//     if(!pAddresses){
//         throw std::runtime_error("Memory Allocation Failed");
//     }

//     DWORD res = GetAdaptersAddresses(fam, flags, NULL, pAddresses.get(), &buff);

//     if(res == ERROR_BUFFER_OVERFLOW){
//         pAddresses.reset(static_cast<PIP_ADAPTER_ADDRESSES>(std::malloc(buff)));

//         if(!pAddresses){
//             throw std::runtime_error("Memory Allocation Failed");
//         }

//         res = GetAdaptersAddresses(fam, flags, NULL, pAddresses.get(), &buff);
//     }

//     if(res != NO_ERROR){
//         throw std::runtime_error("Getting Adapters Failed With iphlpapi Code " + res);
//     }

//     std::string adapterString = "";

//     for(auto it = adapterIndices.begin(); it != adapterIndices.end(); ++it){
//         PIP_ADAPTER_ADDRESSES pip = pAddresses.get();
//         while(pip){
//             if(pip->IfIndex == *it){
//                 if(pip->IfType == IF_TYPE_IEEE80212){
//                     adapterString = pip->AdapterName;
//                     break;
//                 }

//             }
//             pip = pip->Next;
//         }

//         if(adapterString.empty()){
//             throw std::runtime_error("Current Adapter Index Is Not A Wi-Fi Interface");
//         }

//         GUID g;
//         std::wstring wideG(adapterString.begin(), adapterString.end());
//         std::wstring formattedG = L"{" + wideG + L"}";
//         if(CLSIDFromString(formattedG.c_str(), &g) != S_OK){
//             throw std::runtime_error("Failed To Convert String");
//         }

//         DWORD version = 0;
//         HANDLE h;

//         if(WlanOpenHandle(2, NULL, &version, &h) != ERROR_SUCCESS){
//             throw std::runtime_error("Failed To Open WLAN Handle");
//         }

//         DWORD dwDataSize = 0;
//         PWLAN_CONNECTION_ATTRIBUTES pConnAttributes = NULL;
//         WLAN_OPCODE_VALUE_TYPE opCodeTypeValue;

//         res = WlanQueryInterface(
//             h, 
//             &g, 
//             wlan_intf_opcode_current_connection, 
//             NULL, 
//             &dwDataSize, 
//             (PVOID*)&pConnAttributes, 
//             &opCodeTypeValue
//         );

//         if(res == ERROR_SUCCESS && pConnAttributes != NULL){
//             DOT11_PHY_TYPE type = pConnAttributes->wlanAssociationAttributes.dot11PhyType;

//         }
//     }
//     return type;
// }
