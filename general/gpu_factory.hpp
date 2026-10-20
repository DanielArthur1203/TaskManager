#ifndef GPU_FACTORY
#define GPU_FACTORY

#include "gpu.hpp"
#include <memory>
#include <vector>

class GpuFactory{
    private:
        //Returns all adapter LUIDs
        std::vector<LUID> getLuids();
        //Returns all adapter LUIDs as a uint64_t
        std::vector<uint64_t> deviceIds();
        //Returns a vector of wstrings with GPU(integrated and discrete) names
        std::vector<std::wstring> gpuNames();
    public:
        std::vector<std::unique_ptr<GPU>> makeGPUs();
};

#endif