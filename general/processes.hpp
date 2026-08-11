#ifndef PROCESSES_HPP
#define PROCESSES_HPP

#include <memory>
#include <vector>
#include <string>
#include <windows.h>
#include <tlhelp32.h>

class Processes{
    public:
        Processes(){};

        std::vector<PROCESSENTRY32> getAllActiveProcesses() noexcept(false);
        std::vector<DWORD> getPIDFromName(const std::wstring& name) noexcept(false);
        std::wstring getNameFromPID(const DWORD pid) noexcept(false);
        std::string formattedError(std::string msg) noexcept;
};

#endif