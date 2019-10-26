project(catch2)

add_library(catch2 define_main.cpp)
target_include_directories( catch2 
    PRIVATE ./catch2
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)