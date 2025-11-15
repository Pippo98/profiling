cmake_minimum_required(VERSION 3.16)
project(profiler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(DIR ${CMAKE_CURRENT_LIST_DIR})

add_compile_definitions("ImDrawIdx=unsigned int")

if (MSVC)
    add_compile_options(/O2)
else()
    add_compile_options(-O3 -pthread -fPIC)
endif()

option(PROFILER_BUILD_GUI "Profiler build GUI" ON)
option(PROFILER_BUILD_TEST "Profiler build TEST" ON)

add_subdirectory(${DIR}/external)
add_subdirectory(${DIR}/core)
