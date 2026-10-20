#ifndef GPU_HPP
#define GPU_HPP

#include <windows.h>
#include <winnt.h>
#include <cstdint>
#include <string>

class GPU{
    private: 
        uint64_t id;
        LUID luid;
        std::wstring name;
    public:
        GPU() = delete;
        GPU(uint64_t id, LUID luid, std::wstring name) : id(id), luid(luid), name(name){};

        //Returns an ids corresponding GPU utilization
        double utilization();
        /*Returns VRAM total corresponding to LUID

        Inaccurate since the VRAM value is what the OS determines is the cap for optimal performance

        Tried to find a way to get the accurate amount and was going to have a stroke since 
        DXGI_ADAPTER_DESC1 values are either way to small to be accurate or 0 for integrated graphics
        */
        double VRAM();
        //Returns shared VRAM total corresponding to LUID
        double sharedVRAM();
        double singleProcessUsage(DWORD pid);
        const uint64_t& getId() const noexcept;
        const LUID& getLuid() const noexcept;
        const std::wstring& getName() const noexcept;
};


#endif