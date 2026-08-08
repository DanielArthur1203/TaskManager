#include "processes.hpp"
#include <gtest/gtest.h>

TEST(ProcessesTest, getAllActiveProcesses){
    Processes a;
    ASSERT_NO_THROW(a.getAllActiveProcesses());

    auto result = a.getAllActiveProcesses();

    EXPECT_NE(result, nullptr);
}