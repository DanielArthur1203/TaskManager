#ifndef DISK_HPP
#define DISK_HPP

#include "processes.hpp"
#include <unordered_map>
#include <vector>

class Disk{
    private:
        Processes processes;
    public:
        Disk(){processes = Processes();};
        std::vector<std::wstring> diskNames();
        //Returns a map where each disk is paired with it's percent active time
        std::unordered_map<std::wstring, unsigned int> activeTime();
        //Returns read 
        double readSpeed();
};

#endif