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