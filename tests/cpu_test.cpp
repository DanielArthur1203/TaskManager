#include "processes.hpp"
#include "cpu.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <variant>
Cpu a;

TEST(Cpu, cpuClockSpeed){
    EXPECT_NO_THROW(a.cpuClockSpeed());
    EXPECT_TRUE(a.cpuClockSpeed() > 0);
}

TEST(Cpu, threadCount){
    EXPECT_NO_THROW(a.threadCount());
    EXPECT_TRUE(a.threadCount() > 0);
}

TEST(Cpu, handleCount){
    EXPECT_NO_THROW(a.handleCount());
    EXPECT_TRUE(a.handleCount() > 0);
}

TEST(Cpu, upTime){
    ASSERT_NO_FATAL_FAILURE(a.upTime());
    EXPECT_NE(a.upTime(), "");
}

TEST(Cpu, virtualizationState){
    bool ans = a.virtualizationState();
    EXPECT_TRUE(ans == true);
}
