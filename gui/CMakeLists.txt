# gui/CMakeLists.txt

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

qt_add_executable(TaskManager
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

target_link_libraries(TaskManager
    PRIVATE
        taskmanager_core
        taskmanager_general
        Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(TaskManager
    PRIVATE
        ${PROJECT_SOURCE_DIR}/general
)

target_compile_features(TaskManager PUBLIC cxx_std_23)

set_target_properties(TaskManager PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS TaskManager
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
