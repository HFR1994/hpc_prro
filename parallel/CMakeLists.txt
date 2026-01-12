cmake_minimum_required(VERSION 3.10)
project(rra_parallel C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)


find_package(MPI REQUIRED)

file(GLOB_RECURSE MAIN_SRC CONFIGURE_DEPENDS
        "main.c"
)

file(GLOB_RECURSE IMPL_SRCS CONFIGURE_DEPENDS
        "src/*.c"
        "src/**/*.c"
)

add_executable(rra_parallel
        ${MAIN_SRC}
        ${IMPL_SRCS}
)

target_include_directories(rra_parallel PUBLIC include ${MPI_C_INCLUDE_DIRS})
target_link_libraries(rra_parallel PRIVATE m ${MPI_C_LIBRARIES})
set_target_properties(rra_parallel PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")