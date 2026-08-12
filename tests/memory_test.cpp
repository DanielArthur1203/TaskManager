#include "processes.hpp"
#include "memory.hpp"
#include <gtest/gtest.h>

TEST(MemoryInfo, getPhysicalMemoryUsage){
    MemoryInfo a;
    Processes b;
    DWORD pid = b.getPIDFromName(L"Code.exe").at(0);

    ASSERT_NO_THROW(a.getPhysicalMemoryUsage(pid));

    EXPECT_NE(a.getPhysicalMemoryUsage(pid), 0);
}

TEST(MemoryInfo, getTotalMemoryUsage){
    MemoryInfo a;
    SIZE_T mem = a.getTotalPhysicalMemoryUsage();
    EXPECT_TRUE(mem > 0) << "Size reported is " << mem << "\n";
}

TEST(MemoryInfo, getPercentageMemoryUsage){
    MemoryInfo a;

    ASSERT_NO_THROW(a.getPercentageMemoryUsage());
    EXPECT_TRUE(a.getPercentageMemoryUsage() > 0);
}

TEST(MemoryInfo, getTotalCachedMemory){
    MemoryInfo a;
    ULONGLONG totalRamKb = 0;

    ASSERT_TRUE(GetPhysicallyInstalledSystemMemory(&totalRamKb));
    const unsigned long cachedMb = a.getTotalCachedMemory();

    EXPECT_NO_THROW(a.getTotalCachedMemory());
    EXPECT_TRUE(cachedMb > 0);
    EXPECT_LT(cachedMb, static_cast<unsigned long>(totalRamKb / 1000ULL + 1ULL));
}

TEST(MemoryInfo, getSystemCommitLimit){
    MemoryInfo a;

    EXPECT_NO_THROW(a.getSystemCommitLimit());
    EXPECT_TRUE(a.getSystemCommitLimit() > 0);
}

TEST(MemoryInfo, getCurrentCommitUsage){
    MemoryInfo a;

    EXPECT_NO_THROW(a.getCurrentCommitUsage());
    EXPECT_TRUE(a.getCurrentCommitUsage() > 0);
}

TEST(MemoryInfo, getPagedPool){
    MemoryInfo a;

    EXPECT_NO_THROW(a.getPagedPool());
    EXPECT_TRUE(a.getPagedPool() > 0);
}