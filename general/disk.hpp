#ifndef DISK_HPP
#define DISK_HPP

#include "processes.hpp"
#include <unordered_map>
#include <vector>

class Disk{
    private:
        Processes processes;
        LPCSTR wstrToLPCSTR(std::wstring& string);
        std::vector<std::wstring> names;
    public:
        Disk(){
            processes = Processes();
            names = diskNames();
        }
        //Returns a vector containing all current disk names
        std::vector<std::wstring> diskNames();
        //Returns a disks number from name
        int diskNumberFromName(std::wstring& name);
        //Returns a map where each disk is paired with it's percent active time
        std::unordered_map<std::wstring, unsigned int> activeTime();
        //Returns a map where each disk is paired with it's read speed in bytes/sec
        std::unordered_map<std::wstring, double> readSpeed();
        //Returns a map where each disk is paired with it's write speed in bytes/sec
        std::unordered_map<std::wstring, double> writeSpeed();
        //Returns a map where each disk is paired with it's response time in ms
        std::unordered_map<std::wstring, double> responseTime();
        /*Returns a map where each disk is paired with it's capacity in GiB

        Can be rather inaccurate since this method doesn't require elevated permissions 
        and will cap the total to whatever the user restriction is
        */
        std::unordered_map<std::wstring, unsigned int> capacity();
        /*Returns a map where each disk is paired with it's type

        0 -> Unknown, 1 -> No Root Dir, 2 -> Removable, 3 -> Fixed(SSD/HDD)
        4 -> Remote, 5 -> CD-ROM, 6 -> RAM disk*/
        std::unordered_map<std::wstring, int> type();
};

#endif