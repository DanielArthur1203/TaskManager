#include <gtest/gtest.h>
#include "wifi.hpp"

WiFi wifi;

TEST(WiFi, getAdapterIndices){
    ASSERT_EXIT((wifi.getAdapterIndices(), std::exit(0)), ::testing::ExitedWithCode(0), ".*");
    EXPECT_NO_THROW(wifi.getAdapterIndices());
    EXPECT_TRUE(wifi.getAdapterIndices().size() > 0);
}

TEST(WiFi, sendRate){
    EXPECT_NO_THROW(wifi.sendRate());
    EXPECT_TRUE(wifi.sendRate().size() != 0);
}

TEST(WiFi, receiveRate){
    EXPECT_NO_THROW(wifi.receiveRate());
    EXPECT_TRUE(wifi.receiveRate().size() != 0);
}

TEST(WiFi, SSID){
    ASSERT_EXIT((wifi.SSID(), std::exit(0)), ::testing::ExitedWithCode(0), ".*");
    EXPECT_TRUE(wifi.SSID().size() != 0);
}

TEST(WiFi, connectionType){
    EXPECT_NO_THROW(wifi.connectionType());
    EXPECT_TRUE(wifi.connectionType().size() != 0);
}

TEST(WiFi, ipV4Address){
    ASSERT_EXIT((wifi.ipV4Address(), std::exit(0)), ::testing::ExitedWithCode(0), ".*");
    EXPECT_TRUE(wifi.ipV4Address().size() != 0);
}

TEST(WiFi, ipV6Address){
    ASSERT_EXIT((wifi.ipV6Address(), std::exit(0)), ::testing::ExitedWithCode(0), ".*");
    EXPECT_TRUE(wifi.ipV6Address().size() != 0);
}