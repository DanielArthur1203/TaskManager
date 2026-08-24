#ifndef WIFI_HPP
#define WIFI_HPP

#include "processes.hpp"
#include <vector>
#include <unordered_map>

class WiFi{
    private:
        Processes processes;
        std::vector<unsigned long> adapterIndices;
    public:
        WiFi(){
            processes = Processes();
            adapterIndices = getAdapterIndices();
        }
        //Returns a vector holding the indices of every active WiFi adapter
        std::vector<unsigned long> getAdapterIndices();
        //Returns a map where each adapter index is matched with its current send rate in bytes per sec
        std::unordered_map<unsigned long, double> sendRate();
        //Returns a map where each adapter index is matched with its current receive rate in bytes per sec
        std::unordered_map<unsigned long, double> receiveRate();
        /* Returns the SSID of whatever WLAN device shows up first in WLAN_INTERFACE_INFO_LIST
            Not sure how to match this with a adapter index
        */
        std::wstring SSID();
        std::unordered_map<unsigned long, std::string> connectionType();
};

#endif