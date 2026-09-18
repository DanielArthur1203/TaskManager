#include "processes.hpp"
#include <gtest/gtest.h>
#include <string>

TEST(ProcessesTest, getAllActiveProcesses){
    Processes a;
    ASSERT_NO_THROW(a.getAllActiveProcesses());

    auto result = a.getAllActiveProcesses();

    EXPECT_NE(result.size(), 0);
}

TEST(ProcessesTest, allProcessesNames){
    Processes a;

    EXPECT_NO_THROW(a.allProcessesNames());
    EXPECT_FALSE(a.allProcessesNames().empty());
}

TEST(ProcessesTest, allProcessNamesWithExe){
    Processes a;
    EXPECT_NO_THROW(a.allProcessNamesWthExe());
    EXPECT_FALSE(a.allProcessNamesWthExe().empty());
}

TEST(ProcessesTest, getPIDFromName){
    Processes a;
    std::wstring name = L"Code.exe";

    ASSERT_NO_THROW(a.getPIDFromName(name));
    
    EXPECT_NE(0, a.getPIDFromName(name).size());
}

TEST(ProcessesTest, getNameFromPID){
    Processes a;
    std::wstring bad = L"Code.exe";
    DWORD pid = a.getPIDFromName(bad).at(0);

    ASSERT_NO_THROW(a.getNameFromPID(pid));
    std::wstring result = a.getNameFromPID(pid);
    EXPECT_EQ(bad, result);
}

TEST(ProcessesTest, closeWindowGUI){
    Processes a;
    std::wstring name = L"opera.exe";
    DWORD pid = a.getPIDFromName(name).at(0);

    EXPECT_EXIT((a.closeWindowGUI(pid), std::exit(0)), ::testing::ExitedWithCode(0), ".*");
    //EXPECT_NO_THROW(a.closeWindowGUI(pid));
}

TEST(ProcessesTest, windowProcess){
    Processes a;
    std::wstring name = L"opera.exe";
    DWORD pid = a.getPIDFromName(name).at(0);

    EXPECT_NO_THROW(a.windowProcess(pid));
    EXPECT_TRUE(a.windowProcess(pid));
}

//I want to think that this works but every console process I can make will give ERROR_ACCESS_DENIED in AttachConsole
// TEST(ProcessTest, closeConsoleProcess){
//     Processes a;
//     DWORD pid = 11480; //
//     bool exit;

//     EXPECT_NO_THROW(exit = a.closeConsoleProcess(pid));
//     EXPECT_TRUE(exit);
// }
