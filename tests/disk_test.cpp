#include "disk.hpp"
#include "disk_factory.hpp"
#include <gtest/gtest.h>

DiskFactory fac;
Disk disk = *(fac.makeDisks().at(0));

TEST(Disk, activeTime){
    EXPECT_NO_THROW(disk.activeTime());
    EXPECT_TRUE(disk.activeTime() >= 0);
}

TEST(Disk, readSpeed){
    EXPECT_NO_THROW(disk.readSpeed());
    EXPECT_TRUE(disk.readSpeed() >= 0);
}

TEST(Disk, writeSpeed){
    EXPECT_NO_THROW(disk.writeSpeed());
    EXPECT_TRUE(disk.writeSpeed() >= 0);
}

TEST(Disk, responseTime){
    //Could probably segfault
    EXPECT_EXIT((disk.responseTime(), std::exit(0)), ::testing::ExitedWithCode(0), ".*");
    EXPECT_TRUE(disk.responseTime() >= 0);
}

TEST(Disk, manufacturerName){
    EXPECT_TRUE(disk.getManufacturerName() != L"" || disk.getManufacturerName() != L"N/A(Error Occurred)");
}