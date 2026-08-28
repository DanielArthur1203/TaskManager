#include "disk.hpp"
#include <gtest/gtest.h>

Disk disk;

TEST(Disk, diskNames){
    //Could probably segfault
    EXPECT_EXIT((disk.diskNames(), std::exit(0)), ::testing::ExitedWithCode(0), ".*");
    EXPECT_TRUE(disk.diskNames().size() > 0);
}

TEST(Disk, numberFromName){
    EXPECT_NO_THROW(disk.diskNumberFromName(disk.diskNames().at(0)));
}

TEST(Disk, activeTime){
    EXPECT_NO_THROW(disk.activeTime());
    EXPECT_TRUE(disk.activeTime().size() > 0);
}

TEST(Disk, readSpeed){
    EXPECT_NO_THROW(disk.readSpeed());
    EXPECT_TRUE(disk.readSpeed().size() != 0);
}

TEST(Disk, writeSpeed){
    EXPECT_NO_THROW(disk.writeSpeed());
    EXPECT_TRUE(disk.writeSpeed().size() != 0);
}

TEST(Disk, responseTime){
    //Could probably segfault
    EXPECT_EXIT((disk.responseTime(), std::exit(0)), ::testing::ExitedWithCode(0), ".*");
    EXPECT_TRUE(disk.responseTime().size() != 0);
}

TEST(Disk, capacity){
    EXPECT_NO_THROW(disk.capacity());
    EXPECT_TRUE(disk.capacity().size() != 0);
}

TEST(Disk, type){
    EXPECT_NO_THROW(disk.type());
    EXPECT_TRUE(disk.type().size() != 0);
}

TEST(Disk, processDiskUsage){
    Processes b;
    std::wstring bad = L"Code.exe";
    std::vector<DWORD> pids = b.getPIDFromName(bad);

    EXPECT_NO_THROW(disk.processDiskUsage(pids.at(0)));
    EXPECT_TRUE(disk.processDiskUsage(pids.at(0)) >= 0);
}

TEST(Disk, allProcessNameDiskUsage){
    Processes b;
    std::wstring bad = L"Code.exe";
    std::vector<DWORD> pids = b.getPIDFromName(bad);

    EXPECT_NO_THROW(disk.allProcessNameDiskUsage(pids.at(0)));
    EXPECT_TRUE(disk.allProcessNameDiskUsage(pids.at(0)) > 0);
}