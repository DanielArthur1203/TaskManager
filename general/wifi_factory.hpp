#ifndef WIFI_FACTORY
#define WIFI_FACTORY

#include "wifi.hpp"
#include <vector>
#include <memory>
#include <string>

class WifiFactory{
    private:
        //Returns a vector of strings with active internet adapter names
        std::vector<std::wstring> internetAdapterNames();
        //Returns a vector holding the indices of every active WiFi adapter
        std::vector<unsigned long> getAdapterIndices();
    public:
        std::vector<std::unique_ptr<WiFi>> makeWiFi();
};

#endif