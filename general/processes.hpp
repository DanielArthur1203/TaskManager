#ifndef PROCESSES_HPP
#define PROCESSES_HPP

#include <memory>
#include <vector>
#include <windows.h>
#include <tlhelp32.h>

class Processes{
    public:
        Processes(){};

        std::unique_ptr<std::vector<PROCESSENTRY32>> getAllActiveProcesses();
};

#endif