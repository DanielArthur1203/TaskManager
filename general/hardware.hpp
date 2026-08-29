#ifndef HARDWARE_HPP
#define HARDWARE_HPP

#include <string>

class Hardware{
    private:

    public:
        //Returns a string containing the installed CPU name
        std::string cpuName();
        //To get memory name I have to use WMI in The Chronicles Of Riddick where he CoInitializeEx, CoInitializeSecurity,
        //and CoCreateInstance a million times
};

#endif