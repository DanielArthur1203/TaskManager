#ifndef WIFI_COMPAT_HPP
#define WIFI_COMPAT_HPP

#include <windows.h>
#include <windot11.h>

//For some reason my SDK(version 10.0.26100.0) and/or MINGW header doesn't have these types
inline constexpr DOT11_PHY_TYPE dot11_phy_type_vht = static_cast<DOT11_PHY_TYPE>(8);
inline constexpr DOT11_PHY_TYPE dot11_phy_type_dmg = static_cast<DOT11_PHY_TYPE>(9);
inline constexpr DOT11_PHY_TYPE dot11_phy_type_he = static_cast<DOT11_PHY_TYPE>(10);
inline constexpr DOT11_PHY_TYPE dot11_phy_type_eht = static_cast<DOT11_PHY_TYPE>(11);

#endif
