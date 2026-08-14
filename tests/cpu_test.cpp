#include "processes.hpp"
#include "cpu.hpp"
#include <gtest/gtest.h>

TEST(Cpu, currentUsage){
    Cpu a;

    EXPECT_NO_THROW(a.currentUsage());
    ASSERT_TRUE(a.currentUsage() >= 0);
    EXPECT_TRUE(a.currentUsage() > 0);
}

TEST(Cpu, cpuClockSpeed){
    Cpu a;

    EXPECT_NO_THROW(a.cpuClockSpeed());
    EXPECT_TRUE(a.cpuClockSpeed() > 0);
}