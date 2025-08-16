set(CDIR ${CMAKE_CURRENT_LIST_DIR})

if(PROFILER_BUILD_GUI)

FILE(GLOB imgui_srcs ${CDIR}/imgui/*)
FILE(GLOB implot_srcs ${CDIR}/implot/*)

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


add_library(
    imgui
    STATIC
    # GUI
    ${imgui_srcs}
    ${CDIR}/imgui/backends/imgui_impl_glfw.cpp
    ${CDIR}/imgui/backends/imgui_impl_opengl2.cpp
    ${CDIR}/imgui/misc/cpp/imgui_stdlib.cpp
    # PLOT
    ${implot_srcs}
    ${CDIR}/implot/implot.cpp
    ${CDIR}/implot/implot_items.cpp
)
target_link_libraries(
    imgui
    PUBLIC
    glfw
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
)
target_include_directories(
	imgui
	PUBLIC
	${CDIR}
	# thirdparty
	${DIR}/external/imgui
	${DIR}/external/imgui/misc/cpp
	${DIR}/external/imgui/backends
	${DIR}/external/implot
	# GLEW
	${GLEW_INCLUDE_DIRS}
)

endif()
