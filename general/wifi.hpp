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
};

#endif