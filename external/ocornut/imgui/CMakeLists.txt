cmake_minimum_required(VERSION 3.19)

project(imgui)

# For macOS: Using SDL2 installed as a framework (see https://www.libsdl.org/download-2.0.php)
#            version 2.0.20

set(imgui_CPP_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}")

set(imgui_BUILD_SOURCES
    "${imgui_CPP_SRC_DIR}/imgui.cpp"
    "${imgui_CPP_SRC_DIR}/imgui_demo.cpp"
    "${imgui_CPP_SRC_DIR}/imgui_draw.cpp"
    "${imgui_CPP_SRC_DIR}/imgui_tables.cpp"
    "${imgui_CPP_SRC_DIR}/imgui_widgets.cpp"
    "${imgui_CPP_SRC_DIR}/backends/imgui_impl_sdl.cpp"
    )

if(APPLE)
  find_library(SDL2Fwk SDL2 REQUIRED)
  list(APPEND imgui_BUILD_SOURCES "${imgui_CPP_SRC_DIR}/backends/imgui_impl_metal.mm")
endif()

add_library(imgui STATIC "${imgui_BUILD_SOURCES}")
target_include_directories(imgui PUBLIC "${imgui_CPP_SRC_DIR}")

if(APPLE)
  target_include_directories(imgui PUBLIC "metal-cpp")
  target_link_libraries(imgui PUBLIC "${SDL2Fwk}" "-framework QuartzCore" "-framework Cocoa" "-framework Metal")
endif()
