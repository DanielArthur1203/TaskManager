#ifndef DISK_HPP
#define DISK_HPP

#include "processes.hpp"
#include <unordered_map>
#include <vector>
#include <variant>
#include <stdexcept>
#include <unordered_map>

enum class DiskTypes{
    Unknown = 0,
    NoRootDir = 1,
    Removable = 2,
    Fixed = 3, //SSD/HDD
    Remote = 4,
    CD_ROM = 5,
    RAMDisk = 6
};

struct ProcessDiskSample{
    unsigned long long totalActivity;
    unsigned long long creationTime;
    ULONGLONG sampledAtMs; 
};

class Disk{
    private:
        Processes processes;
        LPCSTR wstrToLPCSTR(std::wstring& string);
        std::wstring name;
        int diskNumber;
        unsigned int diskCapacity = 0;
        DiskTypes Type; //ran out of ways to say type
        std::wstring manufacturerName = L""; 

        std::vector<std::wstring> names;
        std::unordered_map<DWORD, ProcessDiskSample> previousSamples;

        unsigned long long fileTimeToULL(const FILETIME& ft);
        ProcessDiskSample processDiskSampleReformed(DWORD pid);
        bool sampleProcessUsage(DWORD pid, double& usage);
        /*Returns a disks capacity in GiB

        Can be rather inaccurate since this method doesn't require elevated permissions 
        and will cap the total to whatever the user restriction is
        */
        unsigned int capacity();
        /*Returns an enum where it's type matches to the following

        0 -> Unknown, 1 -> No Root Dir, 2 -> Removable, 3 -> Fixed(SSD/HDD)
        4 -> Remote, 5 -> CD-ROM, 6 -> RAM disk*/
        DiskTypes type();
    public:
        //Don't use this one use the factory. Only used with a reason
        Disk(){
            processes = Processes();
            names = diskNames();
        }
        Disk(std::wstring name, int diskNumber, std::wstring manufacturerName = L""): name(name), diskNumber(diskNumber){
            diskCapacity = capacity();
            Type = type();
            if(manufacturerName != L"" && this->manufacturerName == L""){
                this->manufacturerName = manufacturerName;
            }
        };
        //Returns a vector containing all current disk names
        std::vector<std::wstring> diskNames();
        //Returns a disks number from name
        int diskNumberFromName(std::wstring& name);
        //Returns a disks percent active time
        unsigned int activeTime();
        //Returns a disks read speed in bytes/sec
        double readSpeed();
        //Returns a disks write speed in bytes/sec
        double writeSpeed();
        //Returns a disks response time in ms
        double responseTime();
        /*Returns the disk usage of a specific process in MiB/sec

        The value returned is likely inflated in comparison to something like Task Manager
        since I can't filter irrelevant I/O traffic without kernel level stuff
        */
        double processDiskUsage(DWORD pid);
        /*Returns disk usage of all processes with the same exe name as the given process
        
        See the comment for processDiskUsage for inflated values
        */
        double allProcessNameDiskUsage(DWORD pid);
        void allProcessNameDiskUsage(std::vector<DWORD> pids, std::variant<double, std::string>& usage, std::exception_ptr& ptr);
        void allProcessNameDiskUsage(std::vector<DWORD> pids, std::variant<double, std::string>& usage);
        const std::wstring& getName() const noexcept;
        const int getDiskCapacity() const noexcept;
        const DiskTypes& getDiskType() const noexcept;
        const std::wstring& getManufacturerName() const noexcept;
};

#endif