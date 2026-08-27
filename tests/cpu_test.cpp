#include "processes.hpp"
#include "cpu.hpp"
#include <gtest/gtest.h>
#include <vector>

Cpu a;

TEST(Cpu, currentUsage){
    EXPECT_NO_THROW(a.currentUsage());
    ASSERT_TRUE(a.currentUsage() >= 0);
    EXPECT_TRUE(a.currentUsage() > 0);
}

TEST(Cpu, processUsage){
    Processes b;
    std::wstring bad = L"Code.exe";
    std::vector<DWORD> pids = b.getPIDFromName(bad);
    
    EXPECT_NO_THROW(a.processUsage(pids.at(0)));
    EXPECT_TRUE(a.processUsage(pids.at(0)) >= 0);
}

TEST(Cpu, processNameTotalUsage){
    Processes b;
    std::wstring bad = L"Code.exe";
    std::vector<DWORD> pids = b.getPIDFromName(bad);

    EXPECT_NO_THROW(a.processNameTotalUsage(pids.at(0)));
    EXPECT_TRUE(a.processNameTotalUsage(pids.at(0)) > 0);
}

TEST(Cpu, cpuClockSpeed){
    EXPECT_NO_THROW(a.cpuClockSpeed());
    EXPECT_TRUE(a.cpuClockSpeed() > 0);
}

TEST(Cpu, processorCount){
    EXPECT_NO_THROW(a.processorCount());
    EXPECT_TRUE(a.processorCount() > 0);
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

TEST(Cpu, cacheAmounts){
    ASSERT_NO_FATAL_FAILURE(a.cacheAmounts()); //in case array returned has more than 0 elements
    EXPECT_NO_THROW(a.cacheAmounts());
    EXPECT_TRUE(a.cacheAmounts().size() > 0);
}

TEST(Cpu, coreCount){
    EXPECT_NO_THROW(a.coreCount());
    EXPECT_TRUE(a.coreCount() > 0);
}
// TEST(Cpu, virtualizationState){
//     EXPECT_NO_FATAL_FAILURE(a.virtualizationState());
// }
