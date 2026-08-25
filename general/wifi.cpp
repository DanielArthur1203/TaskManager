#include "wifi.hpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2ipdef.h>
#include <windows.h>
#include <windot11.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <wlanapi.h>
#include <memory>
#include <thread>
#include <chrono>
#include <cmath>
#include <string>
#include <iostream>

using namespace std::chrono;

std::string WiFi::phyToString(DOT11_PHY_TYPE &phy) noexcept{
    switch(phy){
        case dot11_phy_type_fhss:
            return "Frequency-Hopping Spread-Spectrum";
        case dot11_phy_type_dsss:
            return "Direct Sequence Spread Spectrum";
        case dot11_phy_type_irbaseband:
            return "Infrared";
        case dot11_phy_type_ofdm:
            return "802.11a";
        case dot11_phy_type_hrdsss:
            return "802.11b";
        case dot11_phy_type_erp:
            return "802.11g";
        case dot11_phy_type_ht:
            return "802.11n";
        case dot11_phy_type_vht:
            return "802.11ac (Wi-Fi 5)";
        case dot11_phy_type_dmg:
            return "802.11ad";
        case dot11_phy_type_he:
            return "802.11ax (Wi-Fi 6)";
        case dot11_phy_type_eht:
            return "802.11be (Wi-Fi 7)";
    }
    return "Unknown";
}

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
            const auto& d11 = pA->wlanAssociationAttributes.dot11Ssid;

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

std::unordered_map<unsigned long, std::string> WiFi::connectionType(){
    std::unordered_map<unsigned long, std::string> types;
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

    std::string adapterString = "";

    for(auto it = adapterIndices.begin(); it != adapterIndices.end(); ++it){
        PIP_ADAPTER_ADDRESSES pip = pAddresses.get();
        while(pip){
            if(pip->IfIndex == *it){
                if(pip->IfType == IF_TYPE_IEEE80211){
                    adapterString = pip->AdapterName;
                    break;
                }

            }
            pip = pip->Next;
        }

        if(adapterString.empty()){
            throw std::runtime_error("Current Adapter Index Is Not A Wi-Fi Interface");
        }

        GUID g = GUID{};
        std::wstring wideG(adapterString.begin(), adapterString.end());
        HRESULT convertRes = CLSIDFromString(wideG.c_str(), &g);
        if(convertRes != NOERROR){
            throw std::runtime_error("Failed To Convert String" + convertRes);
        }

        DWORD version = 0;
        HANDLE h;

        if(WlanOpenHandle(2, NULL, &version, &h) != ERROR_SUCCESS){
            throw std::runtime_error("Failed To Open WLAN Handle");
        }

        DWORD dwDataSize = 0;
        PWLAN_CONNECTION_ATTRIBUTES pConnAttributes = PWLAN_CONNECTION_ATTRIBUTES{};
        WLAN_OPCODE_VALUE_TYPE opCodeTypeValue;

        res = WlanQueryInterface(
            h, 
            &g, 
            wlan_intf_opcode_current_connection, 
            NULL, 
            &dwDataSize, 
            (PVOID*)&pConnAttributes, 
            &opCodeTypeValue
        );

        if(res == ERROR_SUCCESS && pConnAttributes != NULL){
            std::string conTypeString = "";
            DOT11_PHY_TYPE type = pConnAttributes->wlanAssociationAttributes.dot11PhyType;
            conTypeString = phyToString(type);
            types.insert({*it, conTypeString});
        }

        CloseHandle(h);
    }
    return types;
}

std::unordered_map<unsigned long, std::string> WiFi::ipV4Address(){
    std::unordered_map<unsigned long, std::string> addresses;
    PMIB_IPADDRTABLE table = NULL;
    DWORD size = 0;

    if(GetIpAddrTable(table, &size, 0) == ERROR_INSUFFICIENT_BUFFER){
        //Idk how to force a smart pointer here
        table = (PMIB_IPADDRTABLE)malloc(size);
    }

    if(!table){
        throw std::runtime_error("Memory Allocation Failed");
    }

    if(GetIpAddrTable(table, &size, 0) == ERROR_SUCCESS){
        for(int i = 0; i < table->dwNumEntries; i++){
            for(auto it = adapterIndices.begin(); it != adapterIndices.end(); ++it){
                std::string address(12, '\0');

                if(table->table[i].dwIndex == *it){
                    IN_ADDR addr;
                    addr.S_un.S_addr = table->table[i].dwAddr;

                    inet_ntop(AF_INET, &addr, address.data(), (socklen_t)address.size());
                    addresses.insert({*it, address});
                }
            }
        }
    }
    free(table);
    return addresses;
}

std::unordered_map<unsigned long, std::wstring> WiFi::ipV6Address(){
    std::unordered_map<unsigned long, std::wstring> addresses;
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG size = 15000;
    std::vector<BYTE> buff(size);
    PIP_ADAPTER_ADDRESSES pAddr = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buff.data());

    DWORD res = GetAdaptersAddresses(AF_INET6, flags, NULL, pAddr, &size);
    if(res == ERROR_BUFFER_OVERFLOW){
        buff.resize(size);
        pAddr = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buff.data());
        res = GetAdaptersAddresses(AF_INET6, flags, NULL, pAddr, &size);
    }

    if(res != NO_ERROR){
        throw std::runtime_error("Failed With iphlpapi Error Code " + res);
    }

    for(auto curr = pAddr; curr != NULL; curr = curr->Next){
        for(auto it = adapterIndices.begin(); it != adapterIndices.end(); ++it){
            if(curr->IfIndex == *it || curr->Ipv6IfIndex == *it){
                for(auto uni = curr->FirstUnicastAddress; uni != NULL; uni = uni->Next){
                    SOCKADDR* sock = uni->Address.lpSockaddr;
                    if(sock && sock->sa_family == AF_INET6){
                        sockaddr_in6* pSock = reinterpret_cast<sockaddr_in6*>(sock);
                        std::wstring text(INET6_ADDRSTRLEN, '\0');
                        InetNtopW(AF_INET6, &(pSock->sin6_addr), text.data(), INET6_ADDRSTRLEN);

                        addresses.insert({*it, text + L"%" + std::to_wstring(*it)});
                    }
                }
            }
        }
    }
    return addresses;
}
