cmake_minimum_required(VERSION 3.16)
project(profiler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(DIR ${CMAKE_CURRENT_LIST_DIR})
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-pthread -fPIC -O3 -std=c++2a")
add_compile_definitions("ImDrawIdx=unsigned int")
add_compile_definitions(IMGUI_DEFINE_MATH_OPERATORS)

option(PROFILER_BUILD_GUI "Profiler build GUI" ON)
option(PROFILER_BUILD_TEST "Profiler build TEST" ON)

add_subdirectory(${DIR}/external)
add_subdirectory(${DIR}/core)
