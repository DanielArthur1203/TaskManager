cmake_minimum_required(VERSION 3.20)

project(TaskManager VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

enable_testing()
include(CTest)

add_subdirectory(src)
add_subdirectory(general)
add_subdirectory(gui)
add_subdirectory(tests)