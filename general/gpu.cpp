#include "gpu.hpp"
#include "general_functions.hpp"
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
#include <PdhMsg.h>

using namespace std::chrono;
using namespace Microsoft::WRL;

double GPU::utilization(){
    //std::unordered_map<uint64_t, double> utilization;
    double utilization = 0;
    PDH_HQUERY q = NULL;
    PDH_HCOUNTER count = NULL;

    PDH_STATUS status = PdhOpenQuery(NULL, 0, &q);
    if(status != ERROR_SUCCESS){
        throw std::runtime_error(GeneralFunctions::formattedError("Opening Query For GPU Utilization", status));
    }

    status = PdhAddCounterW(q, L"\\GPU Engine(*)\\Utilization Percentage", 0, &count);
    if(status != ERROR_SUCCESS){
        PdhCloseQuery(q);
        throw std::runtime_error(GeneralFunctions::formattedError("Adding Counter For GPU Utilization", status));
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
            double usage = 0;
            for(int i = 0; i < itemCount; i++){ 
                std::string hexString = std::format("{:016X}", id);
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
            //utilization.insert({*it, std::ceil(usage)});
            utilization = std::ceil(usage);
        }
    }
    PdhCloseQuery(q);
    return utilization;
}

double GPU::VRAM(){
    //std::unordered_map<uint64_t, double> vram;
    double vram = 0;
    ComPtr<IDXGIFactory4> fac;
    HRESULT res = CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&fac);
    
    if(FAILED(res)){
        throw std::runtime_error("DXGI Factory Creation Failed With DXGI Error " + std::to_string(res));
    }

    ComPtr<IDXGIAdapter3> adapt;
    res = fac->EnumAdapterByLuid(luid, __uuidof(IDXGIAdapter3), (void**)&adapt);

    if(FAILED(res)){
        throw std::runtime_error("Adapter Enumeration Failed With DXGI Error " + std::to_string(res));
    }

    DXGI_QUERY_VIDEO_MEMORY_INFO info = DXGI_QUERY_VIDEO_MEMORY_INFO{};
    res = adapt->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info);

    if(FAILED(res)){
        throw std::runtime_error("Video Mem Query Failed WIth DXGI Error " + std::to_string(res));
    }

    uint64_t bytes = info.Budget;
    double dBytes = static_cast<double>(bytes);
    dBytes /= (1024 * 1024 * 1024);
    dBytes = std::round(dBytes * 10) / 10;
    //vram.insert({ids.at(i), dBytes});
    vram = dBytes;
    return vram;
}

double GPU::sharedVRAM(){
    //std::unordered_map<uint64_t, double> shared;
    double shared = 0;

    ComPtr<IDXGIFactory4> fac;
    HRESULT res = CreateDXGIFactory1(IID_PPV_ARGS(&fac));

    if(FAILED(res)){
        throw std::runtime_error("DXGI Factory Creation Failed With DXGI Error " + std::to_string(res));
    }

    ComPtr<IDXGIAdapter> adapt;
    res = fac->EnumAdapterByLuid(luid, IID_PPV_ARGS(&adapt));

    if(FAILED(res)){
        throw std::runtime_error("Adapter Enumeration Failed With DXGI Error " + std::to_string(res));
    }

    DXGI_ADAPTER_DESC desc;
    res = adapt->GetDesc(&desc);

    if(FAILED(res)){
        throw std::runtime_error("Video Mem Query Failed WIth DXGI Error " + std::to_string(res));
    }

    double bytes = static_cast<double>(desc.SharedSystemMemory);
    bytes /= (1024 * 1024 * 1024);
    bytes = std::round(bytes * 10) / 10;

    //shared.insert({ids.at(i), bytes});
    shared = bytes;
    return shared;
}

double GPU::singleProcessUsage(DWORD pid){
    // double usage = 0;
    // HQUERY query = NULL;

    // PDH_STATUS status = PdhOpenQuery(NULL, 0, &query);
    // if(status != ERROR_SUCCESS){
    //     std::string msg = GeneralFunctions::formattedError("Opening Query For GPU Utilization", status);
    //     throw std::runtime_error(msg);
    // }

    // std::wstring wildcardPath = L"\\GPU Engine(pid_" + std::to_wstring(pid) + L"_luid_0x*_phys_*_engine_*)\\Utilization Percentage";

    // DWORD pathListSize = 0;
    // std::string strPath = wstrToLPCSTR(wildcardPath);
    // PDH_STATUS status = PdhExpandWildCardPath(NULL, strPath.c_str(), nullptr, &pathListSize, 0);
    
    // if (status == PDH_MORE_DATA || pathListSize > 0) {
    //     std::vector<wchar_t> pathList(pathListSize);
    //     status = PdhExpandWildCardPath(NULL, strPath.c_str(), pathList.data(), &pathListSize, 0);

    //     if (status == ERROR_SUCCESS) {
    //         wchar_t* currentPath = pathList.data();
    //         while (*currentPath != L'\0') {
    //             PDH_HCOUNTER counter;
    //             if (PdhAddCounter(query, currentPath, 0, &counter) == ERROR_SUCCESS) {
    //                 counters.push_back(counter);
    //             }
    //             currentPath += wcslen(currentPath) + 1;
    //         }
    //     }
    // }
    // return usage;
    return 0.0;
}

const uint64_t &GPU::getId() const noexcept{
    return id;
}

const LUID &GPU::getLuid() const noexcept{
    return luid;
}

const std::wstring &GPU::getName() const noexcept{
    return name;
}
