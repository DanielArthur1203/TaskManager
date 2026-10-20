#ifndef GENERAL_FUNCTIONS
#define GENERAL_FUNCTIONS

#include <windows.h>
#include <pdh.h>
#include <tlhelp32.h>
#include <pdhmsg.h>
#include <intsafe.h>
#include <string>

namespace GeneralFunctions{
    //Helper to convert wstring to string
    inline LPCSTR wStringToString(std::wstring &string) noexcept{
        int size = WideCharToMultiByte(CP_UTF8, 0, string.c_str(), (int)string.length(), NULL, 0, NULL, NULL);

        static thread_local std::string str;
        str.assign(size, '\0');

        if (size > 0) {
            WideCharToMultiByte(CP_UTF8, 0, string.c_str(), (int)string.length(), str.data(), size, NULL, NULL);
        }

        return str.c_str();
    }

    //Returns a string message of the last error returned by GetLastError
    inline std::string formattedError(std::string msg) noexcept{
        std::string response = "";
        DWORD errorNum;
        int errorNumInt;
        TCHAR sysMsg[256];
        TCHAR *p;

        errorNum = GetLastError( );
        FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, errorNum,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            sysMsg, 256, NULL );

        // Trim the end of the line and terminate it with a null
        p = sysMsg;
        while( ( *p > 31 ) || ( *p == 9 ) )
            ++p;
        do { *p-- = 0; } while( ( p >= sysMsg ) && ( ( *p == '.' ) || ( *p < 33 ) ) );
        

        if(DWordToInt(errorNum, &errorNumInt) == INTSAFE_E_ARITHMETIC_OVERFLOW){
            response = "Failed to generate error code";
        }
        else{
            //idk how to get sysMsg in here without some hijinks
            response += msg + " failed with error number " + std::to_string(errorNumInt);
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
            text.resize(len - 1);
            WideCharToMultiByte(CP_UTF8, 0, pMessage, -1, &text[0], len, NULL, NULL);
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