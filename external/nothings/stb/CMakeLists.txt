cmake_minimum_required(VERSION 3.19)

project(stb-image)

set(stb-image_CPP_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}")

set(stb-image_BUILD_SOURCES
    "${stb-image_CPP_SRC_DIR}/stb.cpp"
    )

add_library(stb-image STATIC "${stb-image_BUILD_SOURCES}")
target_include_directories(stb-image PUBLIC "${stb-image_CPP_SRC_DIR}")
