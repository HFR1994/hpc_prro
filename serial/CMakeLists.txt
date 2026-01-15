cmake_minimum_required(VERSION 3.10)
project(rra_serial C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

file(GLOB_RECURSE MAIN_SRC CONFIGURE_DEPENDS
    "main.c"
)

file(GLOB_RECURSE IMPL_SRCS CONFIGURE_DEPENDS
    "src/*.c"
    "src/**/*.c"
)

add_executable(rra_serial
        ${MAIN_SRC}
        ${IMPL_SRCS}
)

target_include_directories(rra_serial PUBLIC  include)
target_link_libraries(rra_serial PRIVATE m)
set_target_properties(rra_serial PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")