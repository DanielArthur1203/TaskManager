#ifndef DISK_FACTORY
#define DISK_FACTORY

#include "disk.hpp"
#include <memory>
#include <vector>
#include <string>

class DiskFactory{
    private:
        std::vector<std::wstring> diskNames();
    public:
        DiskFactory(){};

        std::vector<std::unique_ptr<Disk>> makeDisks();
};

#endif