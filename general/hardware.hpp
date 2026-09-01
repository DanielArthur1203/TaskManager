#ifndef HARDWARE_HPP
#define HARDWARE_HPP

#include <string>
#include <list>

class Hardware{
    private:

    public:
        //Returns a string containing the installed CPU name
        std::string cpuName() noexcept;
        //Returns a string containing installed memory name
        std::wstring memoryName();
        //Returns a linked list of strings with all disk names
        std::list<std::wstring> diskManufacturerName();
        //Returns a linked list of strings with active internet adapter names
        std::list<std::wstring> internetAdapterNames();
        //Returns a linked list of strings with GPU(integrated and discrete) names
        std::list<std::wstring> gpuNames();
};

#endif