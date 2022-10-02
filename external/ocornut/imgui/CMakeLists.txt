cmake_minimum_required(VERSION 3.19)

project(imgui)

set(imgui_CPP_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}")

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
  list(APPEND imgui_BUILD_SOURCES
      "${imgui_CPP_SRC_DIR}/backends/imgui_impl_metal.mm"
      "${imgui_CPP_SRC_DIR}/backends/regui_impl_metal.mm"
      )
elseif(WIN32)
  list(APPEND imgui_BUILD_SOURCES "${imgui_CPP_SRC_DIR}/backends/imgui_impl_opengl3.cpp")
endif()

add_library(imgui STATIC "${imgui_BUILD_SOURCES}")
target_include_directories(imgui PUBLIC "${imgui_CPP_SRC_DIR}")

if(APPLE)
  target_include_directories(imgui PUBLIC "metal-cpp")
  target_link_libraries(imgui PUBLIC glfw "-framework QuartzCore" "-framework Cocoa" "-framework Metal" "-framework MetalKit" "-framework IOKit" "-framework AppKit")
elseif(WIN32)
  target_include_directories(imgui PUBLIC "${imgui_CPP_SRC_DIR}/backends")
  target_link_libraries(imgui PUBLIC glfw opengl32.lib gdi32.lib shell32.lib Shcore.lib)
endif()

add_executable("binary_to_compressed_c" "${imgui_CPP_SRC_DIR}/misc/fonts/binary_to_compressed_c.cpp")