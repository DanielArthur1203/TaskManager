#ifndef WIFI_HPP
#define WIFI_HPP

#include "wifi_compat.hpp"
#include <vector>
#include <string>
#include <unordered_map>

class WiFi{
    private:
        unsigned long adapterIndex;
        std::wstring adapterName;

        std::string phyToString(DOT11_PHY_TYPE& phy) noexcept;
    public:
        WiFi() = delete;
        WiFi(unsigned long index, std::wstring name) : adapterIndex(index), adapterName(name){};

        //Returns current send rate in bytes per sec
        double sendRate();
        //Returns current receive rate in bytes per sec
        double receiveRate();
        /* Returns the SSID of whatever WLAN device shows up first in WLAN_INTERFACE_INFO_LIST
            Not sure how to match this with a adapter index
        */
        std::wstring SSID();
        //Returns connection type(i.e 802.11ac for WiFi 5)
        std::string connectionType();
        //Returns IPv4 address
        std::string ipV4Address();
        //Returns IPv6 address
        std::wstring ipV6Address();
        const unsigned long &getAdapterIndexWiFi() const noexcept;
        const std::wstring &getAdapterName() const noexcept;
        //Forgot why I commented this out but I'm assuming it's not accurate and I was going to have a stroke
        double processInternetUsage(DWORD pid);
};

#endif