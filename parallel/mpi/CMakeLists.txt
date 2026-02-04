cmake_minimum_required(VERSION 3.10)
project(rrap_mpi C)
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

add_executable(rrap_mpi
        ${MAIN_SRC}
        ${IMPL_SRCS}
)

target_include_directories(rrap_mpi PUBLIC include ${MPI_C_INCLUDE_DIRS})
target_link_libraries(rrap_mpi PRIVATE m ${MPI_C_LIBRARIES})
set_target_properties(rrap_mpi PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")