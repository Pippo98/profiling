cmake_minimum_required(VERSION 3.16)
project(profiler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(DIR ${CMAKE_CURRENT_LIST_DIR})

add_compile_definitions("ImDrawIdx=unsigned int")

if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi /W4 /WX")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG /w")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -Wall -Wextra -Werror")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG -w")
endif()

option(PROFILER_BUILD_GUI "Profiler build GUI" ON)
option(PROFILER_BUILD_TEST "Profiler build TEST" ON)

add_subdirectory(${DIR}/external)
add_subdirectory(${DIR}/core)
