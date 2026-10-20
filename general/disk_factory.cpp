#include "disk_factory.hpp"
#include <windows.h>
#include <cwchar>

std::vector<std::wstring> DiskFactory::diskNames(){
    std::vector<std::wstring> names;

    DWORD count = GetLogicalDriveStringsW(0, NULL);
    if(count == 0){
        throw std::runtime_error("Getting Logical Drives");
    }

    std::wstring buff(count, L'\0');
    GetLogicalDriveStringsW(count, &buff[0]);

    wchar_t* p = &buff[0];
    while (*p) {
        names.push_back(p);
        p += wcslen(p) + 1;
    }
    return names;
}

std::vector<std::unique_ptr<Disk>> DiskFactory::makeDisks(){
    std::vector<std::unique_ptr<Disk>> disks;
    std::vector<std::wstring> names = diskNames();

    int i = 0;
    for(const auto& name: names){
        disks.push_back(std::make_unique<Disk>(name, i));
        i++;
    }
    return disks;
}
