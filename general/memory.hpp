#ifndef MEMORYINFO_HPP
#define MEMORYINFO_HPP

#include "processes.hpp"
#include <memory>
#include <string>
#include <variant>
#include <stdexcept>
#include <windows.h>
#include <pdh.h>

class MemoryInfo{
    private:
        Processes processes;
        inline static std::wstring name = L"";
        inline static std::string type = "";
        inline static std::variant<std::monostate, unsigned short, std::string> usedSlots;
        inline static std::variant<std::monostate, unsigned short, std::string> totalSlots;
        inline static std::variant<std::monostate, unsigned long, std::string> speed;

        unsigned long PDHQueryHelper(PDH_HQUERY& q, DWORD_PTR& dwP, LPCSTR path);
        //Returns a wstring containing installed memory name
        std::wstring memoryName();
        //Returns the installed RAM type
        std::string getRAMType();
        //Returns number of used RAM slots
        unsigned short getNumUsedRAMSlots();
        //Returns total number of RAM slots
        unsigned short getTotalRAMSlots();
        //Returns configured memory clock speed in MT/s
        unsigned long getMemorySpeed();
    public:
        MemoryInfo();

        //Returns private working set size of one process in MiB
        double getPhysicalMemoryUsage(const DWORD pid);
        //Returns private working set size in MiB of all processes with the same exe name as the given PID
        double getNamePhysicalMemoryUsage(const DWORD pid);
        //Same as the one above except it doesn't perform a call to get all active processes
        void getNamePhysicalMemoryUsage(std::vector<DWORD> pids, std::variant<double, std::string>& usage, std::exception_ptr& ptr);
        void getNamePhysicalMemoryUsage(std::vector<DWORD> pids, std::variant<double, std::string>& usage);
        //Returns working set size of all non PPL and PP-L processes in MiB
        double getTotalPhysicalMemoryUsage();
        //Returns total usage of all non PPL and PP-L processes as a percentage
        double getPercentageMemoryUsage();
        //Returns total cached memory in MiB
        unsigned long getTotalCachedMemory();
        //Returns system commit limit in GiB
        double getSystemCommitLimit();
        //Returns current system commit usage in GiB
        double getCurrentCommitUsage();
        //Returns current paged pool in MiB
        unsigned long getPagedPool();
        //Returns non paged pool in MiB
        unsigned long getNonPagedPool();
        std::wstring getName() const noexcept;
        std::string getType() const noexcept;
        const std::variant<std::monostate, unsigned short, std::string>& getUsedSlots() const noexcept;
        const std::variant<std::monostate, unsigned short, std::string>& getTotalSlots() const noexcept;
        const std::variant<std::monostate, unsigned long, std::string>& getSpeed() const noexcept;
};

#endif