set(CDIR ${CMAKE_CURRENT_LIST_DIR})

configure_file(
    ${CDIR}/src/defines.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/defines.hpp
)

set(DIRS_TO_INCLUDE
    ${CDIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_library(profiler
    ${CDIR}/src/profiler/profiler.cpp
)
target_include_directories(profiler PUBLIC ${DIRS_TO_INCLUDE})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY         ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/bin)

if (PROFILER_BUILD_GUI)
    add_executable(plotter
        ${CDIR}/src/app_utils/implementation.cpp
        ${CDIR}/executables/plotter/plotter.cpp
        ${CDIR}/executables/plotter/csv.cpp
        ${CDIR}/executables/plotter/kvp.cpp
    )
    target_link_libraries(plotter PUBLIC
        imgui
        tinyfiledialogs
    )
    target_include_directories(plotter PRIVATE ${DIRS_TO_INCLUDE})
endif()

if (PROFILER_BUILD_TEST)
    add_executable(test_profiler
        ${CDIR}/executables/test_profiler.cpp
    )
    target_link_libraries(test_profiler profiler)
    target_include_directories(test_profiler PRIVATE ${DIRS_TO_INCLUDE})
endif()
