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

TEST(Cpu, processorCount){
    Cpu a;

    EXPECT_NO_THROW(a.processorCount());
    EXPECT_TRUE(a.processorCount() > 0);
}

TEST(Cpu, threadCount){
    Cpu a;

    EXPECT_NO_THROW(a.threadCount());
    EXPECT_TRUE(a.threadCount() > 0);
}

TEST(Cpu, handleCount){
    Cpu a;

    EXPECT_NO_THROW(a.handleCount());
    EXPECT_TRUE(a.handleCount() > 0);
}