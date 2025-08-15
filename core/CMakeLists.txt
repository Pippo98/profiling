set(CDIR ${CMAKE_CURRENT_LIST_DIR})

configure_file(${CDIR}/src/defines.hpp.in ${CDIR}/src/defines.hpp)

set(
	DIRS_TO_INCLUDE
	${DIR}
	${CDIR}/src

	# thirdparty
	${DIR}/external/imgui
	${DIR}/external/imgui/misc/cpp
	${DIR}/external/imgui/backends
	${DIR}/external/implot

	# GLEW
	${GLEW_INCLUDE_DIRS}
	${EXTERNAL_DIRS}
)

add_library(profiler
    ${CDIR}/src/profiler/profiler.cpp
)
add_library(window_utils
    ${CDIR}/src/app_utils/implementation.cpp
)
target_link_libraries(window_utils
    PUBLIC
    imgui
    implot
)

target_include_directories(
		profiler
		PUBLIC
		${DIRS_TO_INCLUDE}
)
target_include_directories(
		window_utils
		PUBLIC
		${DIRS_TO_INCLUDE}
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

add_executable(plotter
    ${CDIR}/executables/plotter/plotter.cpp
    ${CDIR}/executables/plotter/csv.cpp
    ${CDIR}/executables/plotter/kvp.cpp
)
target_link_libraries(plotter window_utils)

add_executable(test_profiler ${CDIR}/executables/test_profiler.cpp)
target_link_libraries(test_profiler profiler)
