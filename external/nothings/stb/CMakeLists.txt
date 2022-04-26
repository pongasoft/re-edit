cmake_minimum_required(VERSION 3.19)

project(stb)

set(stb_CPP_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}")

set(stb_BUILD_SOURCES
    "${stb_CPP_SRC_DIR}/stb.cpp"
    )

add_library(stb STATIC "${stb_BUILD_SOURCES}")
target_include_directories(stb PUBLIC "${stb_CPP_SRC_DIR}")
