#ifndef GENERAL_FUNCTIONS
#define GENERAL_FUNCTIONS

#include <windows.h>
#include <pdh.h>
#include <tlhelp32.h>
#include <pdhmsg.h>
#include <intsafe.h>
#include <string>
#include <stdexcept>

namespace GeneralFunctions{
    //Helper to convert wstring to string
    inline std::string wStringToString(const std::wstring& string){
        if (string.empty()) {
            return {};
        }

        const int size = WideCharToMultiByte(
            CP_UTF8, 0, string.data(), static_cast<int>(string.size()),
            NULL, 0, NULL, NULL);
        if (size == 0) {
            throw std::runtime_error("Converting wide string to UTF-8 failed");
        }

        std::string result(static_cast<std::size_t>(size), '\0');
        const int converted = WideCharToMultiByte(
            CP_UTF8, 0, string.data(), static_cast<int>(string.size()),
            result.data(), size, NULL, NULL);
        if (converted == 0) {
            throw std::runtime_error("Converting wide string to UTF-8 failed");
        }

        result.resize(static_cast<std::size_t>(converted));
        return result;
    }

    //Returns a string message of the last error returned by GetLastError
    inline std::string formattedError(std::string msg) noexcept{
        std::string response = "";
        const DWORD errorNum = GetLastError();
        int errorNumInt;
        TCHAR sysMsg[256] = {};

        const DWORD chars = FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorNum,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            sysMsg, static_cast<DWORD>(sizeof(sysMsg) / sizeof(sysMsg[0])), NULL);

        if (chars != 0) {
            TCHAR* end = sysMsg + chars;
            while (end > sysMsg &&
                   (end[-1] == TEXT('.') || end[-1] < 33)) {
                *--end = TEXT('\0');
            }
        }

        if (DWordToInt(errorNum, &errorNumInt) == INTSAFE_E_ARITHMETIC_OVERFLOW) {
            response = "Failed to generate error code";
        }
        else {
            response = msg + " failed with error number " + std::to_string(errorNumInt);
        }

        return response;
    }

    //Returns a string message for a PDH status code
    inline std::string formattedError(std::string msg, PDH_STATUS errorCode) noexcept{
        HMODULE hPdhLibrary = LoadLibraryA("pdh.dll");
        if(hPdhLibrary == NULL){
            DWORD loadErr = GetLastError();
            return msg + " failed (couldn't load pdh.dll), error " + std::to_string(loadErr);
        }

        LPWSTR pMessage = NULL;

        if(!FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE |
                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_IGNORE_INSERTS,
                        hPdhLibrary,
                        static_cast<DWORD>(errorCode),
                        0,
                        (LPWSTR)&pMessage,
                        0,
                        NULL))
        {
            FreeLibrary(hPdhLibrary);
            return msg + " failed with error number " + std::to_string(static_cast<DWORD>(errorCode));
        }

        // Convert wide string to UTF-8 std::string
        std::string text;
        int len = WideCharToMultiByte(CP_UTF8, 0, pMessage, -1, NULL, 0, NULL, NULL);
        if(len > 0){
            text.resize(len);
            const int converted = WideCharToMultiByte(
                CP_UTF8, 0, pMessage, -1, text.data(), len, NULL, NULL);
            if (converted > 0) {
                text.resize(static_cast<std::size_t>(converted - 1));
            }
            else {
                text.clear();
            }
        }

        LocalFree(pMessage);
        FreeLibrary(hPdhLibrary);

        if(text.empty()){
            return msg + " failed with error number " + std::to_string(static_cast<DWORD>(errorCode));
        }

        return msg + " failed: " + text;
        }
}
#endif 