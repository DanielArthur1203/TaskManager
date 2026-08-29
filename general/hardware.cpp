#include "hardware.hpp"
#include <intrin.h>
#include <vector>
#include <bitset>
#include <cstring>
#include <array>

std::string Hardware::cpuName(){
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