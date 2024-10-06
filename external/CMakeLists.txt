set(CDIR ${CMAKE_CURRENT_LIST_DIR})

set(EXTERNAL_DIRS
  ${CDIR}
  )

FILE(GLOB imgui_srcs ${CDIR}/imgui/*)
FILE(GLOB implot_srcs ${CDIR}/implot/*)

add_library(
    imgui
    STATIC

    # GUI
    ${imgui_srcs}
    ${CDIR}/imgui/backends/imgui_impl_glfw.cpp
    ${CDIR}/imgui/backends/imgui_impl_opengl3.cpp
    ${CDIR}/imgui/misc/cpp/imgui_stdlib.cpp
)

add_library(
    implot
    STATIC

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
target_link_libraries(
    implot
    PUBLIC

    imgui
)

set(EXTERNAL_DIRS ${EXTERNAL_DIRS} PARENT_SCOPE)
