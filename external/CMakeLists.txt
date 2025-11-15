set(CDIR ${CMAKE_CURRENT_LIST_DIR})

if (PROFILER_BUILD_GUI)

file(GLOB imgui_srcs ${CDIR}/imgui/*.cpp ${CDIR}/imgui/*.h)
file(GLOB implot_srcs ${CDIR}/implot/*.cpp ${CDIR}/implot/*.h)

# Cross-platform GL + GLFW
find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)

if (NOT GLEW_LIBRARIES)
    if (GLEW_LIBRARY_DEBUG AND GLEW_LIBRARY_RELEASE)
        set(GLEW_LIBRARIES
            "optimized;${GLEW_LIBRARY_RELEASE};debug;${GLEW_LIBRARY_DEBUG}"
        )
    else()
        set(GLEW_LIBRARIES GLEW::GLEW)
    endif()
endif()

add_library(imgui STATIC
    ${imgui_srcs}
    ${CDIR}/imgui/backends/imgui_impl_glfw.cpp
    ${CDIR}/imgui/backends/imgui_impl_opengl2.cpp
    ${CDIR}/imgui/misc/cpp/imgui_stdlib.cpp

    ${implot_srcs}
    ${CDIR}/implot/implot.cpp
    ${CDIR}/implot/implot_items.cpp
)

target_link_libraries(imgui PUBLIC
    glfw
    OpenGL::GL
    ${GLEW_LIBRARIES}
)

target_include_directories(imgui PUBLIC
    ${CDIR}                      # project external root
    ${CDIR}/imgui
    ${CDIR}/imgui/misc/cpp
    ${CDIR}/imgui/backends
    ${CDIR}/implot
    ${GLEW_INCLUDE_DIRS}
)

add_library(tinyfiledialogs STATIC
    ${CDIR}/tinyfiledialogs/tinyfiledialogs.c
)

target_include_directories(tinyfiledialogs PUBLIC
    ${CDIR}/tinyfiledialogs
)

endif()
