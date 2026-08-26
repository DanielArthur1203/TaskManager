#ifndef GPU_HPP
#define GPU_HPP

#include "processes.hpp"
#include <unordered_map>
#include <cstdint>

class GPU{
    private: 
        Processes processes;
        std::vector<uint64_t> ids;
        std::vector<LUID> luids;
        //Returns all adapter LUIDs
        std::vector<LUID> getLuids();
    public:
        GPU(){
            processes = Processes();
            ids = deviceIds();
            luids = getLuids();
        }
        //Returns all adapter LUIDs as a uint64_t
        std::vector<uint64_t> deviceIds();
        //Returns a map where each LUID is matched to its corresponding GPU utilization
        std::unordered_map<uint64_t, double> utilization();
        /*Returns a map where each LUID is matched to its VRAM total

        Inaccurate(off by 500MB for me) since the VRAM value is what the OS determines is the cap for optimal performance

        Tried to find a way to get the accurate amount and was going to have a stroke since 
        DXGI_ADAPTER_DESC1 values are either way to small to be accurate or 0 for integrated graphics
        */
        std::unordered_map<uint64_t, double> VRAM();
        //Returns a map where each LUID is matched to its shared VRAM total
        std::unordered_map<uint64_t, double> sharedVRAM();
};


#endif