cmake_minimum_required(VERSION 3.17)

set(re-edit_VERSION_MAJOR 0)
set(re-edit_VERSION_MINOR 9)
set(re-edit_VERSION_PATCH 5)
set(re-edit_VERSION "${re-edit_VERSION_MAJOR}.${re-edit_VERSION_MINOR}.${re-edit_VERSION_PATCH}")

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "" FORCE)
  # on macOS "uname -m" returns the architecture (x86_64 or arm64)
  execute_process(
      COMMAND uname -m
      RESULT_VARIABLE result
      OUTPUT_VARIABLE RE_EDIT_OSX_NATIVE_ARCHITECTURE
      OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if((CMAKE_GENERATOR STREQUAL "Xcode")                # works only with Xcode
      AND (RE_EDIT_OSX_NATIVE_ARCHITECTURE STREQUAL "arm64")) # and only when running on arm64
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    set(re-edit_ARCHIVE_ARCHITECTURE "macOS_universal")
    set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST) # fixes gtest crash
    message(STATUS "macOS universal (x86_64 / arm64) build")
  else()
    option(re-edit_UNIVERSAL_BUILD "Build universal binary (x86_64 & arm64)" OFF)
    set(re-edit_ARCHIVE_ARCHITECTURE "macOS_${RE_EDIT_OSX_NATIVE_ARCHITECTURE}")
    message(STATUS "macOS native ${RE_EDIT_OSX_NATIVE_ARCHITECTURE} build")
  endif()
endif()

if(WIN32)
  set(re-edit_ARCHIVE_ARCHITECTURE "win_64bits")
endif()

project(re-edit VERSION "${re-edit_VERSION}" LANGUAGES CXX)

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

# Uncomment to run with sanitizer
#add_compile_options("-fsanitize=address")
#add_link_options("-fsanitize=address")

if(WIN32)
  # This gets rid of the missing dlls from the C++ runtime
  add_compile_options("$<$<CONFIG:DEBUG>:/MTd>")
  add_compile_options("$<$<CONFIG:RELEASE>:/MT>")
endif()

# Including glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw/glfw-3.3.8 EXCLUDE_FROM_ALL)

# Including Dear ImGui
add_subdirectory(external/ocornut/imgui EXCLUDE_FROM_ALL)

# Include stb
add_subdirectory(external/nothings/stb EXCLUDE_FROM_ALL)

# Include nativefiledialog
add_subdirectory(external/btzy/nativefiledialog-extended EXCLUDE_FROM_ALL)

# Using RE SDK version 4.3.0
set(RE_SDK_VERSION 4.3.0)

# Location of RE SDK: can be set when invoking cmake => cmake -D "RE_SDK_ROOT:PATH=/path/to/re_sdk"
# or via -p option in configure.py script or in cmake-gui
if(APPLE)
  set(RE_SDK_ROOT "/Users/Shared/ReasonStudios/JukeboxSDK_${RE_SDK_VERSION}/SDK" CACHE PATH "Location of RE SDK")
elseif(WIN32)
  set(RE_SDK_ROOT "C:/Users/Public/Documents/ReasonStudios/JukeboxSDK_${RE_SDK_VERSION}/SDK" CACHE PATH "Location of RE SDK")
endif()

# Adding dependency on re-mock
option(RE_MOCK_SUPPORT_FOR_AUDIO_FILE "" ON)
# using re-mock v1.1.0
add_subdirectory(../../pongasoft/re-mock re-mock EXCLUDE_FROM_ALL)

# Defines the location of the sources
set(re-edit_CPP_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src/cpp")

# Defines all the common sources
set(re-edit_BUILD_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/lua/Base.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/lua/Base.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/lua/Device2D.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/lua/Device2D.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/lua/HDGui2D.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/lua/HDGui2D.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/lua/ReEdit.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/lua/ReEdit.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/AppContext.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/AppContext.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Application.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Application.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Color.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Color.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Constants.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Errors.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/FilmStrip.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/FilmStrip.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/FontManager.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/FontManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/LoggingManager.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/LoggingManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Graphics.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Graphics.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Panel.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Panel.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/PanelState.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/PanelState.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Property.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/PropertyManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/ReGui.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/TextureManager.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/TextureManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/UndoManager.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/UndoManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/UserPreferences.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Views.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Views.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Widget.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Widget.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/WidgetAttribute.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/WidgetAttribute.cpp")

# Fonts
set(RE_EDIT_FONTS_DIR "${CMAKE_CURRENT_LIST_DIR}/external/fonts")

# Defines the macos main
if(APPLE)
  list(APPEND re-edit_BUILD_SOURCES "${re-edit_CPP_SRC_DIR}/re/edit/macos/MTLTextureManager.cpp")
  set(re-edit_MAIN_SRC "${re-edit_CPP_SRC_DIR}/re/edit/macos/main.cpp" "${re-edit_CPP_SRC_DIR}/re/edit/macos/metal.cpp")
elseif(WIN32)
  list(APPEND re-edit_BUILD_SOURCES "${re-edit_CPP_SRC_DIR}/re/edit/windows/OGL3TextureManager.cpp")
  set(re-edit_MAIN_SRC "${re-edit_CPP_SRC_DIR}/re/edit/windows/main.cpp")
endif()

# Build the static library
add_library("${target}_lib" "${re-edit_BUILD_SOURCES}")
target_include_directories("${target}_lib" PUBLIC "external/misc" "${RE_EDIT_FONTS_DIR}/includes" "${RE_EDIT_FONTS_DIR}/src")
target_link_libraries("${target}_lib" PUBLIC imgui stb re-mock nfd)
target_compile_definitions("${target}_lib" PUBLIC ENABLE_RE_EDIT_INTERNAL_ASSERT=1)

# Build the executable
add_executable(${target} "${re-edit_MAIN_SRC}")
target_link_libraries(${target} PRIVATE "${target}_lib")

#if(APPLE AND ${re-edit_UNIVERSAL_BUILD})
#  message(STATUS "setting up target for univeral build")
#  function(target_setup_universal_binary target)
#    set_target_properties(${target}
#        PROPERTIES
#        XCODE_ATTRIBUTE_OSX_ARCHITECTURES   "x86_64;arm64"
#        XCODE_ATTRIBUTE_ARCHS               "$(ARCHS_STANDARD_64_BIT)"
#        )
#    set_target_properties(${target}
#        PROPERTIES
#        XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH        "NO"
#        )
#  endfunction()
#  target_setup_universal_binary("${target}")
#  target_setup_universal_binary(glfw)
#  target_setup_universal_binary(imgui)
#  target_setup_universal_binary(re-mock)
#  target_setup_universal_binary(stb)
#endif()

#######################################################
# Install
#######################################################
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install")

install(
    TARGETS "${target}"
    COMPONENT "exe"
    DESTINATION "."
)

#######################################################
# Archive
#######################################################
set(CPACK_PACKAGE_NAME                "${target}")
set(CPACK_PACKAGE_VERSION_MAJOR       "${re-edit_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR       "${re-edit_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH       "${re-edit_VERSION_PATCH}-dev")
set(CPACK_SYSTEM_NAME                 "${re-edit_ARCHIVE_ARCHITECTURE}")

set(CPACK_GENERATOR ZIP)
set(CPACK_VERBATIM_VARIABLES TRUE)
set(CPACK_ARCHIVE_COMPONENT_INSTALL OFF)
set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE ON)
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)

include(CPack)

cpack_add_component("exe")

add_custom_target("create_archive"
    COMMAND ${CMAKE_COMMAND} -E echo "Creating archive..."
    COMMAND "${CMAKE_CPACK_COMMAND}" --verbose -G ZIP -C $<CONFIG>
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}" # CPackConfig.cmake is created there
    DEPENDS "${target}"
    )

#######################################################
# Testing
#######################################################
# Download and unpack googletest at configure time
include(fetch-GoogleTest.cmake)
include(GoogleTest)

set(target_test "${target}_test")

set(re-edit_CPP_TST_DIR "${CMAKE_CURRENT_LIST_DIR}/test/cpp")

set(TEST_CASE_SOURCES
    "${re-edit_CPP_TST_DIR}/re/edit/lua/TestDevice2D.cpp"
    "${re-edit_CPP_TST_DIR}/re/edit/lua/TestHDGui2D.cpp"
    )

add_executable("${target_test}" "${TEST_CASE_SOURCES}")
target_compile_definitions("${target_test}" PUBLIC RE_EDIT_PROJECT_DIR="${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries("${target_test}" gtest_main gmock "${target}_lib")
target_include_directories("${target_test}" PUBLIC "${re-edit_CPP_SRC_DIR}")

gtest_discover_tests("${target_test}")
