set(CDIR ${CMAKE_CURRENT_LIST_DIR})

include_directories(
    ${EXTERNAL_DIRS}
    ${CDIR}/src
)

add_library(window_utils
    ${CDIR}/src/app_utils/implementation.cpp
)
target_link_libraries(window_utils
    PUBLIC
    imgui
    implot
    )


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

add_executable(example
${CDIR}/executables/example/example.cpp
)
target_link_libraries(example
    window_utils
    )
