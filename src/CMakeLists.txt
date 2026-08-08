# src/CMakeLists.txt
add_library(taskmanager_core STATIC
    main.cpp
)

target_link_libraries(taskmanager_core
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
)

target_include_directories(taskmanager_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/gui
)

target_compile_features(taskmanager_core PUBLIC cxx_std_23)
