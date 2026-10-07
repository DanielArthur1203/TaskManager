#ifndef PROCESSES_HPP
#define PROCESSES_HPP

#include <vector>
#include <string>
#include <list>
#include <windows.h>
#include <pdh.h>
#include <tlhelp32.h>
#include <stdexcept>

class Processes{
    private:
        static bool closed;
        static inline BOOL windowsEnumProc(HWND hwnd, LPARAM lparam){
            DWORD pid = static_cast<DWORD>(lparam);
            BOOL closedHere = true;
            DWORD windowPid = 0;
            auto res = GetWindowThreadProcessId(hwnd, &windowPid);

            if(!res){
                throw std::runtime_error("Get Window Thread Process For Process " + pid);
            }

            if(windowPid == pid && GetWindow(hwnd, GW_OWNER) == NULL && IsWindowVisible(hwnd)){
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                closedHere = false;
                closed = true;
            }
            return closedHere;
        }
    public:
        Processes(){};

        //Helper to convert wstring to string
        LPCSTR wStringToString(std::wstring &string);
        //Helper to get full exe path from a pid
        std::wstring fullPathFromPID(DWORD pid);
        //Helper to get a file descriptor(text not a number) from a full path
        std::wstring fileDescriptorName(std::wstring &fullPath);
        //Returns a vector of PROCESSENTRY32s for all active processes
        std::vector<PROCESSENTRY32> getAllActiveProcesses();
        //Returns a linked list of process names without their exe for all active processes
        std::list<std::string> allProcessesNames();
        //Returns a linked list of process names with their exe for all active processes
        std::list<std::string> allProcessNamesWthExe();
        //Returns a vector of PIDs of all processes that had the same exe name as the given param
        std::vector<DWORD> getPIDFromName(const std::wstring& name);
        //Returns the exe name of the process corresponding to the given pid
        std::wstring getNameFromPID(const DWORD pid);
        //First attempts to signal to the GUI process to close itself and after 5 seconds the process is forcefully terminated
        void closeWindowGUI(DWORD pid);
        //See comment in processes_test
        bool closeConsoleProcess(DWORD pid);
        //Returns true if the process is a GUI process and false otherwise
        bool windowProcess(DWORD pid);
        //Returns a string message of the last error returned by GetLastError
        std::string formattedError(std::string msg) noexcept;
        //Returns a string message for a PDH status code
        std::string formattedError(std::string msg, PDH_STATUS errorCode) noexcept;
};

#endif