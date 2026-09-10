#ifndef WIFI_HPP
#define WIFI_HPP

#include "wifi_compat.hpp"
#include "processes.hpp"
#include <vector>
#include <unordered_map>

class WiFi{
    private:
        Processes processes;
        std::vector<unsigned long> adapterIndices;

        std::string phyToString(DOT11_PHY_TYPE& phy) noexcept;
        // void toggleConnectionStats(MIB_TCPROW_OWNER_PID row, bool enable);
        // void readConnectionBytes(MIB_TCPROW_OWNER_PID row, ULONG64& bytesReceived, ULONG64& bytesSent);
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
        //Returns a map where each adapter index is matched with its connection type(i.e 802.11ac for WiFi 5)
        std::unordered_map<unsigned long, std::string> connectionType();
        //Returns a map where each adapter index is matched with its IPv4 address
        std::unordered_map<unsigned long, std::string> ipV4Address();
        //Returns a map where each adapter index is matched with its IPv6 address
        std::unordered_map<unsigned long, std::wstring> ipV6Address();
        //Forgot why I commented this out but I'm assuming it's not accurate and I was going to have a stroke
        double processInternetUsage(DWORD pid);
};

#endif