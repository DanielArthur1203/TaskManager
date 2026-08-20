include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
    DOWNLOAD_EXTRACT_TIMESTAMP ON
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(taskmanager_tests
    processes_test.cpp
    memory_test.cpp
    cpu_test.cpp
    disk_test.cpp
)

target_include_directories(taskmanager_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/gui
)

target_link_libraries(taskmanager_tests
    PRIVATE
        GTest::gtest_main
        taskmanager_core
        Qt${QT_VERSION_MAJOR}::Widgets
        taskmanager_general
)

include(GoogleTest)
gtest_discover_tests(taskmanager_tests)