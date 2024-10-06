cmake_minimum_required(VERSION 3.16)
project(imgui-template)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(DIR ${CMAKE_CURRENT_LIST_DIR})
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-pthread -O3 -std=c++2a")
add_compile_definitions("ImDrawIdx=unsigned int")
add_compile_definitions(IMGUI_DEFINE_MATH_OPERATORS)

find_package(GLEW      REQUIRED)
find_package(OpenGL    REQUIRED)
find_package(glfw3 3.3 REQUIRED)

if (NOT GLEW_LIBRARIES)
    if (GLEW_LIBRARY_DEBUG AND GLEW_LIBRARY_RELEASE)
        set(GLEW_LIBRARIES "optimized;${GLEW_LIBRARY_RELEASE};debug;${GLEW_LIBRARY_DEBUG}")
    else ()
        set(GLEW_LIBRARIES GLEW::GLEW)
    endif ()
endif ()

include_directories(
    ${DIR}

    # thirdparty
    ${DIR}/external/imgui
    ${DIR}/external/imgui/misc/cpp
    ${DIR}/external/imgui/backends
    ${DIR}/external/implot

    # GLEW
    ${GLEW_INCLUDE_DIRS}
)

add_subdirectory(${DIR}/external)
add_subdirectory(${DIR}/core)
