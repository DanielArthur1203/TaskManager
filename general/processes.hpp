#ifndef PROCESSES_HPP
#define PROCESSES_HPP

#include <vector>
#include <string>
#include <windows.h>
#include <tlhelp32.h>
#include <stdexcept>

class Processes{
    private:
        static bool closed;
        static inline BOOL windowsEnumProc(HWND hwnd, LPARAM lparam){
            DWORD pid = static_cast<DWORD>(lparam);
            BOOL closedHere = false;
            DWORD windowPid = 0;
            auto res = GetWindowThreadProcessId(hwnd, &windowPid);

            if(!res){
                throw std::runtime_error("Get Window Thread Process For Process " + pid);
            }

            if(windowPid == pid && GetWindow(hwnd, GW_OWNER) == NULL && IsWindowVisible(hwnd)){
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                closedHere = true;
                closed = true;
            }
            return closedHere;
        }
    public:
        Processes(){};

        std::vector<PROCESSENTRY32> getAllActiveProcesses();
        std::vector<DWORD> getPIDFromName(const std::wstring& name);
        std::wstring getNameFromPID(const DWORD pid);
        void closeWindowGUI(DWORD pid);
        std::string formattedError(std::string msg) noexcept;
        std::string formattedError(std::string msg, bool PDHError) noexcept;
};

#endif