#ifndef DISK_FACTORY
#define DISK_FACTORY

#include "disk.hpp"
#include <memory>
#include <vector>
#include <string>

class DiskFactory{
    private:
        std::vector<std::wstring> diskNames();
        //Returns a vector of wstrings with all disk names
        std::vector<std::wstring> diskManufacturerName();
    public:
        DiskFactory(){};

        std::vector<std::unique_ptr<Disk>> makeDisks();
};

#endif