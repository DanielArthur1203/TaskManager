#ifndef PROCESSES_HPP
#define PROCESSES_HPP

#include <vector>
#include <string>
#include <windows.h>
#include <tlhelp32.h>

class Processes{
    public:
        Processes(){};

        std::vector<PROCESSENTRY32> getAllActiveProcesses();
        std::vector<DWORD> getPIDFromName(const std::wstring& name);
        std::wstring getNameFromPID(const DWORD pid);
        std::string formattedError(std::string msg) noexcept;
        std::string formattedError(std::string msg, bool PDHError) noexcept;
};

#endif