add_library(taskmanager_general STATIC
    processes.cpp
    memory.cpp
    cpu.cpp
    disk.cpp
)

target_include_directories(taskmanager_general
    PUBLIC
        ${PROJECT_SOURCE_DIR}/general
)

target_compile_features(taskmanager_general PUBLIC cxx_std_23)
target_link_libraries(taskmanager_general PRIVATE pdh wbemuuid ole32 oleaut32 PowrProf)

if(WIN32)
    target_compile_definitions(taskmanager_general
        PUBLIC
            WIN32_LEAN_AND_MEAN
            NOMINMAX
    )
endif()
