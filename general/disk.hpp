#ifndef DISK_HPP
#define DISK_HPP

#include "processes.hpp"
#include <unordered_map>
#include <vector>

class Disk{
    private:
        Processes processes;
        LPCSTR wstrToLPCSTR(std::wstring& string);
    public:
        Disk(){processes = Processes();};
        std::vector<std::wstring> diskNames();
        //Returns a map where each disk is paired with it's percent active time
        std::unordered_map<std::wstring, unsigned int> activeTime();
        //Returns a map where each disk is paired with it's read speed in bytes/sec
        std::unordered_map<std::wstring, double> readSpeed();
        //Returns a map where each disk is paired with it's write speed in bytes/sec
        std::unordered_map<std::wstring, double> writeSpeed();
        //Returns a map where each disk is paired with it's response time in ms
        std::unordered_map<std::wstring, double> responseTime();
};

#endif