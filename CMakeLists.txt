cmake_minimum_required(VERSION 3.17)

project(re-edit LANGUAGES CXX)

set(re-edit_VERSION_MAJOR 1)
set(re-edit_VERSION_MINOR 0)
set(re-edit_VERSION_PATCH 0)
set(re-edit_VERSION "${re-edit_VERSION_MAJOR}.${re-edit_VERSION_MINOR}.${re-edit_VERSION_PATCH}")

set(CMAKE_CXX_STANDARD 17)

#if(re-edit_DEV_BUILD)
#  enable_testing()
#
#  # Using RE SDK version 4.3.0
#  set(RE_SDK_VERSION 4.3.0)
#
#  # Location of RE SDK: can be set when invoking cmake => cmake -D "RE_SDK_ROOT:PATH=/path/to/re_sdk"
#  # or via -p option in configure.py script or in cmake-gui
#  if(APPLE)
#    set(RE_SDK_ROOT "/Users/Shared/ReasonStudios/JukeboxSDK_${RE_SDK_VERSION}/SDK" CACHE PATH "Location of RE SDK")
#  else()
#    set(RE_SDK_ROOT "C:/Users/Public/Documents/ReasonStudios/JukeboxSDK_${RE_SDK_VERSION}/SDK" CACHE PATH "Location of RE SDK")
#  endif()
#else()
#  if(NOT RE_SDK_ROOT)
#    message(ERROR "RE_SDK_ROOT must be defined")
#  endif()
#endif()
#
set(target "re-edit")

# Including Dear ImGui
add_subdirectory(external/ocornut/imgui EXCLUDE_FROM_ALL)

# Include stb
add_subdirectory(external/nothings/stb EXCLUDE_FROM_ALL)

# Using RE SDK version 4.3.0
set(RE_SDK_VERSION 4.3.0)

# Location of RE SDK: can be set when invoking cmake => cmake -D "RE_SDK_ROOT:PATH=/path/to/re_sdk"
# or via -p option in configure.py script or in cmake-gui
if(APPLE)
  set(RE_SDK_ROOT "/Users/Shared/ReasonStudios/JukeboxSDK_${RE_SDK_VERSION}/SDK" CACHE PATH "Location of RE SDK")
else()
  set(RE_SDK_ROOT "C:/Users/Public/Documents/ReasonStudios/JukeboxSDK_${RE_SDK_VERSION}/SDK" CACHE PATH "Location of RE SDK")
endif()

# Adding dependency on re-mock
add_subdirectory(../../pongasoft/re-mock re-mock EXCLUDE_FROM_ALL)

# Defines the location of the sources
set(re-edit_CPP_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src/cpp")

# Defines all the common sources
set(re-edit_BUILD_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/logging/logging.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/logging/loguru.hpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/logging/loguru.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Application.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Application.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Constants.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/DrawContext.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/DrawContext.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/FilmStrip.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/FilmStrip.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/PanelView.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/PanelView.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/TextureManager.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/TextureManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/UserPreferences.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/View.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Widget.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Widget.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/WidgetView.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/WidgetView.cpp"
    )

# Defines the macos main
if(APPLE)
  list(APPEND re-edit_BUILD_SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/MTLTextureManager.cpp")
  set(re-edit_MAIN_SRC "${re-edit_CPP_SRC_DIR}/re/edit/macos/main.cpp" "${re-edit_CPP_SRC_DIR}/re/edit/macos/metal.cpp")
#  set(re-edit_MAIN_SRC "${re-edit_CPP_SRC_DIR}/re/edit/macos/main.mm")
endif()

# Build the executable
add_executable(${target} "${re-edit_BUILD_SOURCES}" "${re-edit_MAIN_SRC}")
target_link_libraries(${target} PRIVATE imgui stb re-mock)
