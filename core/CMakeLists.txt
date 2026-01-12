set(CDIR ${CMAKE_CURRENT_LIST_DIR})
set(PROFILER_CMAKE_DIR ${PROJECT_SOURCE_DIR}/cmake)


configure_file(
    ${CDIR}/src/defines.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/defines.hpp
)

add_library(profiler
    ${CDIR}/src/profiler/profiler.cpp
)
target_include_directories(profiler
    PUBLIC
        $<BUILD_INTERFACE:${CDIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include>
)

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
    target_include_directories(plotter
      PRIVATE
        $<BUILD_INTERFACE:${CDIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    )
endif()

if (PROFILER_BUILD_TEST)
    add_executable(test_profiler
        ${CDIR}/executables/test_profiler.cpp
    )
    target_link_libraries(test_profiler profiler)
    target_include_directories(test_profiler
      PRIVATE
        $<BUILD_INTERFACE:${CDIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    )
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# ---- Install library ----
install(
    TARGETS profiler
    EXPORT profilerTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    DIRECTORY ${CDIR}/src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp"
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/defines.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if (PROFILER_BUILD_GUI)
    install(
        TARGETS plotter
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# ---- Export targets ----
install(
    EXPORT profilerTargets
    NAMESPACE profiler::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/profiler
)

# ---- Package config ----
set(PROFILER_CMAKE_DIR ${PROJECT_SOURCE_DIR}/cmake)

configure_package_config_file(
    ${PROFILER_CMAKE_DIR}/profilerConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/profilerConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/profiler
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/profilerConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/profilerConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/profilerConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/profiler
)

