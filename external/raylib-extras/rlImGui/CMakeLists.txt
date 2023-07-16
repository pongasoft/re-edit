cmake_minimum_required(VERSION 3.19)

project(rlImGui)

set(rlImGui_CPP_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}")

set(rlImGui_BUILD_SOURCES
    "${rlImGui_CPP_SRC_DIR}/rlImGui.cpp"
    )

add_library(rlImGui STATIC "${rlImGui_BUILD_SOURCES}")
target_compile_definitions(rlImGui PUBLIC NO_FONT_AWESOME)
target_include_directories(rlImGui PUBLIC "${rlImGui_CPP_SRC_DIR}")
target_link_libraries(rlImGui PUBLIC imgui raylib)
