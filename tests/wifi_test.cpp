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

TEST(wiFi, receiveRate){
    EXPECT_NO_THROW(wifi.receiveRate());
    EXPECT_TRUE(wifi.receiveRate().size() != 0);
}