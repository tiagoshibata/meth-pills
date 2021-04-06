cmake_minimum_required(VERSION 3.19)
project(meth-pills C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
include(CheckIPOSupported)
check_ipo_supported()
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if (BUILD_TESTING)
    set(CMAKE_C_INCLUDE_WHAT_YOU_USE include-what-you-use)
    set(CMAKE_C_CLANG_TIDY clang-tidy)
endif()

if (CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-Wall -Wextra -Wno-missing-field-initializers)
    string(REPLACE -O3 -Ofast CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
endif()

add_executable(meth-pills
        main.c
)
