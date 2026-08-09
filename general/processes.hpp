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

        std::unique_ptr<std::vector<PROCESSENTRY32>> getAllActiveProcesses() noexcept(false);
        std::unique_ptr<std::vector<DWORD>> getPIDFromName(const std::wstring& name) noexcept(false);
        std::unique_ptr<std::wstring> getNameFromPID(const DWORD pid) noexcept(false);
        std::unique_ptr<std::wstring> getParentNameFromChildPID(const DWORD pid) noexcept(false);
        std::string formattedError(std::string msg) noexcept;
};

#endif