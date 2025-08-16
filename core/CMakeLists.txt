set(CDIR ${CMAKE_CURRENT_LIST_DIR})

configure_file(${CDIR}/src/defines.hpp.in ${CDIR}/src/defines.hpp)

set(
	DIRS_TO_INCLUDE
	${CDIR}/src
)

add_library(profiler
    ${CDIR}/src/profiler/profiler.cpp
)
target_include_directories(
		profiler
		PUBLIC
		${DIRS_TO_INCLUDE}
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

if(PROFILER_BUILD_GUI)
	add_executable(plotter
			${CDIR}/src/app_utils/implementation.cpp
			${CDIR}/executables/plotter/plotter.cpp
			${CDIR}/executables/plotter/csv.cpp
			${CDIR}/executables/plotter/kvp.cpp
	)
	target_link_libraries(plotter
		PUBLIC
		imgui
	)
	target_include_directories(plotter
		PRIVATE
		${DIRS_TO_INCLUDE}
	)
endif()


if(PROFILER_BUILD_TEST)
	add_executable(test_profiler ${CDIR}/executables/test_profiler.cpp)
	target_link_libraries(test_profiler profiler)
endif()
