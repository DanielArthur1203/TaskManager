#include "hardware.hpp"
#include <gtest/gtest.h>

Hardware hardware;

TEST(Hardware, cpuName){
    EXPECT_EXIT((hardware.cpuName(), std::exit(0)), ::testing::ExitedWithCode(0), ".*");
    EXPECT_FALSE(hardware.cpuName().empty());
}