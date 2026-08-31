#include "hardware.hpp"
#include <gtest/gtest.h>

Hardware hardware;

TEST(Hardware, cpuName){
    EXPECT_EXIT((hardware.cpuName(), std::exit(0)), ::testing::ExitedWithCode(0), ".*");
    EXPECT_FALSE(hardware.cpuName().empty());
}

TEST(Hardware, memoryName){
    EXPECT_NO_THROW(hardware.memoryName());
    EXPECT_FALSE(hardware.memoryName().empty());
}

TEST(Hardware, diskManufacturerName){
    EXPECT_NO_THROW(hardware.diskManufacturerName());
    EXPECT_FALSE(hardware.memoryName().empty());
}

TEST(Hardware, internetAdapterNames){
    EXPECT_NO_THROW(hardware.internetAdapterNames());
    EXPECT_FALSE(hardware.memoryName().empty());
}