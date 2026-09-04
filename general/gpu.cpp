#include "gpu.hpp"
#include <windows.h>
#include <pdh.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <format>
#include <cmath>

using namespace std::chrono;
using namespace Microsoft::WRL;

std::vector<LUID> GPU::getLuids(){
    std::vector<LUID> luids;
    IDXGIFactory1* fac = NULL;
    HRESULT res = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&fac);

    if(FAILED(res)){
        throw std::runtime_error("DXGI Factory Creation Failed With DXGI Error " + res);
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

std::vector<uint64_t> GPU::deviceIds(){
    std::vector<uint64_t> values;
    IDXGIFactory1* fac = NULL;
    HRESULT res = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&fac);

    if(FAILED(res)){
        throw std::runtime_error("DXGI Factory Creation Failed With DXGI Error " + res);
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

std::unordered_map<uint64_t, double> GPU::utilization(){
    std::unordered_map<uint64_t, double> utilization;
    PDH_HQUERY q = NULL;
    PDH_HCOUNTER count = NULL;

    PDH_STATUS status = PdhOpenQuery(NULL, 0, &q);
    if(status != ERROR_SUCCESS){
        throw std::runtime_error(processes.formattedError("Opening Query For GPU Utilization", status));
    }

    status = PdhAddCounterW(q, L"\\GPU Engine(*)\\Utilization Percentage", 0, &count);
    if(status != ERROR_SUCCESS){
        PdhCloseQuery(q);
        throw std::runtime_error(processes.formattedError("Adding Counter For GPU Utilization", status));
    }

    PdhCollectQueryData(q);
    std::this_thread::sleep_for(seconds(1));
    PdhCollectQueryData(q);

    DWORD buff = 0;
    DWORD itemCount = 0;

    PdhGetFormattedCounterArrayW(count, PDH_FMT_DOUBLE, &buff, &itemCount, NULL);

    if(buff > 0){
        std::vector<BYTE> b(buff);
        PPDH_FMT_COUNTERVALUE_ITEM_W items = reinterpret_cast<PPDH_FMT_COUNTERVALUE_ITEM_W>(b.data());

        if(PdhGetFormattedCounterArrayW(count, PDH_FMT_DOUBLE, &buff, &itemCount, items) == ERROR_SUCCESS){
            for(auto it = ids.begin(); it != ids.end(); ++it){
                double usage = 0;
                for(int i = 0; i < itemCount; i++){ 
                    std::string hexString = std::format("{:016X}", *it);
                    std::wstring_view name = items[i].szName;

                    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, &hexString[0], hexString.size(), NULL, 0);

                    std::wstring wideHex(sizeNeeded, '\0');
                    MultiByteToWideChar(CP_UTF8, 0, &hexString[0], hexString.size(), &wideHex[0], sizeNeeded);

                    size_t pos = wideHex.find_first_not_of(L'0');
                    if(pos != std::wstring::npos){
                        wideHex.erase(0, pos);
                    }

                    if(name.find(wideHex) != std::wstring::npos){
                        usage += items[i].FmtValue.doubleValue;
                    }
                }
                utilization.insert({*it, std::ceil(usage)});
            }
        }
    }
    PdhCloseQuery(q);
    return utilization;
}

std::unordered_map<uint64_t, double> GPU::VRAM(){
    std::unordered_map<uint64_t, double> vram;
    for(auto i = 0; i < luids.size(); ++i){
        ComPtr<IDXGIFactory4> fac;
        HRESULT res = CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&fac);
        
        if(FAILED(res)){
            throw std::runtime_error("DXGI Factory Creation Failed With DXGI Error " + res);
        }

        ComPtr<IDXGIAdapter3> adapt;
        res = fac->EnumAdapterByLuid(luids.at(i), __uuidof(IDXGIAdapter3), (void**)&adapt);

        if(FAILED(res)){
            throw std::runtime_error("Adapter Enumeration Failed With DXGI Error " + res);
        }

        DXGI_QUERY_VIDEO_MEMORY_INFO info = DXGI_QUERY_VIDEO_MEMORY_INFO{};
        res = adapt->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info);

        if(FAILED(res)){
            throw std::runtime_error("Video Mem Query Failed WIth DXGI Error " + res);
        }

        uint64_t bytes = info.Budget;
        double dBytes = static_cast<double>(bytes);
        dBytes /= (1024 * 1024 * 1024);
        dBytes = std::round(dBytes * 10) / 10;
        //Hope that luids and ids are properly matching but I can't test this since I only have one GPU 
        vram.insert({ids.at(i), dBytes});
    }
    return vram;
}

std::unordered_map<uint64_t, double> GPU::sharedVRAM(){
    std::unordered_map<uint64_t, double> shared;
    
    for(int i = 0; i < luids.size(); i++){
        ComPtr<IDXGIFactory4> fac;
        HRESULT res = CreateDXGIFactory1(IID_PPV_ARGS(&fac));

        if(FAILED(res)){
            throw std::runtime_error("DXGI Factory Creation Failed With DXGI Error " + res);
        }

        ComPtr<IDXGIAdapter> adapt;
        res = fac->EnumAdapterByLuid(luids.at(i), IID_PPV_ARGS(&adapt));

        if(FAILED(res)){
            throw std::runtime_error("Adapter Enumeration Failed With DXGI Error " + res);
        }

        DXGI_ADAPTER_DESC desc;
        res = adapt->GetDesc(&desc);

        if(FAILED(res)){
            throw std::runtime_error("Video Mem Query Failed WIth DXGI Error " + res);
        }

        double bytes = static_cast<double>(desc.SharedSystemMemory);
        bytes /= (1024 * 1024 * 1024);
        bytes = std::round(bytes * 10) / 10;

        shared.insert({ids.at(i), bytes});
    }
    return shared;
}

