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

TEST(MemoryInfo, getNamePhysicalMemoryUsage){
    MemoryInfo a;
    Processes b;
    DWORD pid = b.getPIDFromName(L"Code.exe").at(0);

    EXPECT_NO_THROW(a.getNamePhysicalMemoryUsage(pid));
    EXPECT_TRUE(a.getNamePhysicalMemoryUsage(pid) > 0);
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

TEST(MemoryInfo, getNonPagedPool){
    MemoryInfo a;

    EXPECT_NO_THROW(a.getNonPagedPool());
    EXPECT_TRUE(a.getNonPagedPool() > 0);
}

TEST(MemoryInfo, getMemorySpeed){
    MemoryInfo a;

    EXPECT_NO_THROW(a.getMemorySpeed());
    EXPECT_TRUE(a.getMemorySpeed() > 0);
}

TEST(MemoryInfo, getNumUsedRAMSlots){
    MemoryInfo a;

    EXPECT_NO_THROW(a.getNumUsedRAMSlots());
    EXPECT_TRUE(a.getNumUsedRAMSlots() > 0);
}

TEST(MemoryInfo, getTotalRAMSlots){
    MemoryInfo a;

    EXPECT_NO_THROW(a.getTotalRAMSlots());
    EXPECT_TRUE(a.getTotalRAMSlots() > 0);
}

TEST(MemoryInfo, getRAMType){
    MemoryInfo a;

    EXPECT_NO_THROW(a.getRAMType());
    EXPECT_FALSE(a.getRAMType().empty());
}