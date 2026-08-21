#include "disk.hpp"
#include <gtest/gtest.h>

Disk disk;

TEST(Disk, diskNames){
    //Could probably segfault
    EXPECT_EXIT((disk.diskNames(), std::exit(0)), ::testing::ExitedWithCode(0), ".*");
    EXPECT_NO_THROW(disk.diskNames());
    EXPECT_TRUE(disk.diskNames().size() > 0);
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
    EXPECT_NO_THROW(disk.responseTime());
    EXPECT_TRUE(disk.responseTime().size() != 0);
}