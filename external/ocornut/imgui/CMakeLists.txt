cmake_minimum_required(VERSION 3.19)

project(imgui)

set(imgui_CPP_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(glfw_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../glfw/glfw")

set(imgui_BUILD_SOURCES
    "${imgui_CPP_SRC_DIR}/imgui.cpp"
    "${imgui_CPP_SRC_DIR}/imgui_demo.cpp"
    "${imgui_CPP_SRC_DIR}/imgui_draw.cpp"
    "${imgui_CPP_SRC_DIR}/imgui_tables.cpp"
    "${imgui_CPP_SRC_DIR}/imgui_widgets.cpp"
    "${imgui_CPP_SRC_DIR}/backends/imgui_impl_glfw.cpp"
    "${imgui_CPP_SRC_DIR}/misc/cpp/imgui_stdlib.cpp"
    )

if(APPLE)
  find_library(SDL2Fwk SDL2 REQUIRED)
  list(APPEND imgui_BUILD_SOURCES "${imgui_CPP_SRC_DIR}/backends/imgui_impl_metal.mm")
endif()

add_library(imgui STATIC "${imgui_BUILD_SOURCES}")
target_include_directories(imgui PUBLIC "${imgui_CPP_SRC_DIR}")
target_include_directories(imgui PUBLIC "${glfw_ROOT_DIR}/include")

if(APPLE)
  target_include_directories(imgui PUBLIC "metal-cpp")
  target_link_directories(imgui PUBLIC "${glfw_ROOT_DIR}/macos-lib-universal")
  target_link_libraries(imgui PUBLIC glfw3 "-framework QuartzCore" "-framework Cocoa" "-framework Metal" "-framework MetalKit" "-framework IOKit" "-framework AppKit")
endif()
