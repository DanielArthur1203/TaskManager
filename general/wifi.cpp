#include "wifi.hpp"
#include <winsock2.h>
#include <ws2ipdef.h>
#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <memory>
#include <thread>
#include <chrono>
#include <cmath>

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
