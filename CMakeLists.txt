cmake_minimum_required(VERSION 3.24)

set(re-edit_VERSION_MAJOR 1)
set(re-edit_VERSION_MINOR 1)
set(re-edit_VERSION_PATCH 0)
set(re-edit_VERSION "${re-edit_VERSION_MAJOR}.${re-edit_VERSION_MINOR}.${re-edit_VERSION_PATCH}")

execute_process(COMMAND git describe --long --dirty --abbrev=10 --tags
    RESULT_VARIABLE result
    OUTPUT_VARIABLE re-edit_GIT_VERSION
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git describe --tags --abbrev=0
    RESULT_VARIABLE result
    OUTPUT_VARIABLE re-edit_GIT_TAG
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_STRIP_TRAILING_WHITESPACE)
message(STATUS "re-edit git version - ${re-edit_GIT_VERSION} | re-edit git tag - ${re-edit_GIT_TAG}")

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

# Include stb (for loading png images)
add_subdirectory(external/nothings/stb EXCLUDE_FROM_ALL)

# Including Dear ImGui
add_subdirectory(external/ocornut/imgui EXCLUDE_FROM_ALL)

# Include nativefiledialog (open native file dialog)
add_subdirectory(external/btzy/nativefiledialog-extended EXCLUDE_FROM_ALL)

# Include efsw (cross platform filesystem watcher)
add_subdirectory(external/SpartanJ/efsw EXCLUDE_FROM_ALL)

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
#set(re-mock_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../pongasoft/re-mock")
include(cmake/fetch_re-mock.cmake)

# Defines the location of the sources
set(re-edit_CPP_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src/cpp")

# Defines the location of the resources
set(re-edit_RESOURCES_DIR "${CMAKE_CURRENT_LIST_DIR}/src/resources")

# Defines the icon file
if(APPLE)
  set(re-edit_ICON_FILE "${re-edit_RESOURCES_DIR}/re-edit.icns")
elseif(WIN32)
  set(re-edit_ICON_FILE "${re-edit_RESOURCES_DIR}/re-edit.ico")
endif()

# Convert to native path
cmake_path(NATIVE_PATH re-edit_ICON_FILE re-edit_ICON_FILE)
string(REPLACE "\\" "\\\\" re-edit_ICON_FILE "${re-edit_ICON_FILE}")

# Defines all the common sources
set(re-edit_BUILD_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/lua/Base.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/lua/Base.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/lua/ConfigParser.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/lua/ConfigParser.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/lua/Device2D.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/lua/Device2D.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/lua/HDGui2D.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/lua/HDGui2D.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/platform/GLFWContext.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/platform/GLFWContext.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/AppContext.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/AppContext.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Application.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Application.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/BuiltIns.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Color.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Color.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Config.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Constants.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Dialog.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Dialog.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Errors.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/FilmStrip.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/FilmStrip.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/FontManager.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/FontManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/LoggingManager.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/LoggingManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Graphics.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Graphics.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/NetworkManager.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/NetworkManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Panel.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Panel.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/PanelState.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/PanelState.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/PreferencesManager.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/PreferencesManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Property.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/PropertyManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/ReGui.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/ReGui.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/String.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/String.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/TextureManager.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/TextureManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/UndoManager.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/UndoManager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Views.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Views.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Widget.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/Widget.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/WidgetAttribute.h"
    "${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/WidgetAttribute.cpp")

set(VERSION_DIR "${CMAKE_BINARY_DIR}/generated")
configure_file("${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/version.h.in" "${VERSION_DIR}/version.h")

# Fonts
set(RE_EDIT_FONTS_DIR "${CMAKE_CURRENT_LIST_DIR}/external/fonts")

# Defines the macos main
if(APPLE)
  list(APPEND re-edit_BUILD_SOURCES
      "${re-edit_CPP_SRC_DIR}/re/edit/platform/macos/MTLManagers.cpp"
      "${re-edit_CPP_SRC_DIR}/re/edit/platform/macos/MacOSNetworkManager.mm"
      "${re-edit_CPP_SRC_DIR}/re/edit/platform/macos/NSUserDefaultsManager.mm"
      )
  set(re-edit_MAIN_SRC "${re-edit_CPP_SRC_DIR}/re/edit/platform/macos/main.cpp" "${re-edit_CPP_SRC_DIR}/re/edit/platform/macos/metal.cpp")
elseif(WIN32)
  list(APPEND re-edit_BUILD_SOURCES
      "${re-edit_CPP_SRC_DIR}/re/edit/platform/windows/OGL3Managers.cpp"
      "${re-edit_CPP_SRC_DIR}/re/edit/platform/windows/LocalSettingsManager.cpp"
      "${re-edit_CPP_SRC_DIR}/re/edit/platform/windows/WindowsNetworkManager.cpp"
      )
  set(re-edit_MAIN_SRC "${re-edit_CPP_SRC_DIR}/re/edit/platform/windows/main.cpp" "${re-edit_RESOURCES_DIR}/re-edit.rc")
  set(re-edit_LINK_LIBRARIES "winhttp")
endif()

# Build the static library
add_library("${target}_lib" "${re-edit_BUILD_SOURCES}")
target_include_directories("${target}_lib" PUBLIC
    "${VERSION_DIR}"
    "external/misc"
    "external/nlohmann/json/single_include"
    "${RE_EDIT_FONTS_DIR}/includes"
    "${RE_EDIT_FONTS_DIR}/src"
    )
target_link_libraries("${target}_lib" PUBLIC imgui stb-image re-mock nfd efsw "${re-edit_LINK_LIBRARIES}")
target_compile_definitions("${target}_lib" PUBLIC ENABLE_RE_EDIT_INTERNAL_ASSERT=1)

# Build the executable
add_executable(${target} WIN32 MACOSX_BUNDLE "${re-edit_MAIN_SRC}")
set_target_properties(${target} PROPERTIES OUTPUT_NAME "RE Edit")
target_link_libraries(${target} PRIVATE "${target}_lib")
if(APPLE)
  set_target_properties(${target} PROPERTIES
      BUNDLE True
      MACOSX_BUNDLE_GUI_IDENTIFIER com.pongasoft.re-edit
      MACOSX_BUNDLE_BUNDLE_NAME re-edit
      MACOSX_BUNDLE_BUNDLE_VERSION "${re-edit_VERSION}"
      MACOSX_BUNDLE_SHORT_VERSION_STRING "${re-edit_VERSION}"
      MACOSX_BUNDLE_ICON_NAME "AppIcon"
      MACOSX_BUNDLE_COPYRIGHT "© 2023 pongasoft"
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/MacOSXBundleInfo.plist.in"
      )
  target_sources(${target} PRIVATE "src/resources/re-edit.icns")
  set_source_files_properties("src/resources/re-edit.icns" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()

macro(cmake_option_to_python_bool cmake_opt python_opt)
  if ("${cmake_opt}")
    set("python_${python_opt}" "True")
  else ()
    set("python_${python_opt}" "False")
  endif ()
endmacro()

get_property(GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
cmake_option_to_python_bool(${GENERATOR_IS_MULTI_CONFIG} GENERATOR_IS_MULTI_CONFIG)

configure_file(${CMAKE_CURRENT_LIST_DIR}/re-edit.py.in "${CMAKE_BINARY_DIR}/re-edit.py" @ONLY)

#######################################################
# Install
#######################################################
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install")

install(
    TARGETS "${target}"
    DESTINATION "."
)

#######################################################
# Archive
#######################################################
set(CPACK_PACKAGE_NAME                "${target}")
set(CPACK_PACKAGE_VENDOR              "pongasoft")
set(CPACK_PACKAGE_VERSION             "${re-edit_VERSION}")
set(CPACK_SYSTEM_NAME                 "${re-edit_ARCHIVE_ARCHITECTURE}")
set(CPACK_RESOURCE_FILE_LICENSE       "${CMAKE_CURRENT_LIST_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_FILE_NAME           "${target}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME}")

set(CPACK_VERBATIM_VARIABLES TRUE)
set(CPACK_ARCHIVE_COMPONENT_INSTALL OFF)
set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE ON)
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
set(CPACK_PACKAGE_CHECKSUM "SHA256")

if(APPLE)
  # Generates a dmg
  set(RE_EDIT_CPACK_GENERATOR "DragNDrop")
elseif(WIN32)
  set_property(INSTALL "$<TARGET_FILE_NAME:${target}>" PROPERTY CPACK_START_MENU_SHORTCUTS "RE Edit")

  set(CPACK_PACKAGE_NAME              "RE Edit")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "pongasoft/RE Edit")
  set(CPACK_WIX_PROGRAM_MENU_FOLDER   "pongasoft")
  set(CPACK_WIX_UI_BANNER             "${CMAKE_CURRENT_LIST_DIR}/src/resources/wix_ui_banner-493x58.png")
  set(CPACK_WIX_UI_DIALOG             "${CMAKE_CURRENT_LIST_DIR}/src/resources/wix_ui_dialog-493x312.png")
  set(CPACK_WIX_PRODUCT_ICON          "${CMAKE_CURRENT_LIST_DIR}/src/resources/re-edit.ico")
  set(CPACK_WIX_UPGRADE_GUID          "3FFD49F8-82AE-4B57-BBEC-7AB995C24B8F")
  set(CPACK_WIX_ARCHITECTURE          "x64")

  # For the add/remove program section
  set(CPACK_WIX_PROPERTY_ARPCOMMENTS      "RE Edit is a free and open source WYSIWYG editor for the UI of a Rack Extension")
  set(CPACK_WIX_PROPERTY_ARPHELPLINK      "https://pongasoft.com/contact.html")
  set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT  "https://pongasoft.com/re-edit")
  set(CPACK_WIX_PROPERTY_ARPURLUPDATEINFO "https://github.com/pongasoft/re-edit/releases")

  set(RE_EDIT_CPACK_GENERATOR         "WIX")
endif()

include(CPack)

add_custom_target("create_archive"
    COMMAND ${CMAKE_COMMAND} -E echo "Creating archive..."
    COMMAND "${CMAKE_CPACK_COMMAND}" --verbose -G ${RE_EDIT_CPACK_GENERATOR} -C $<CONFIG>
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}" # CPackConfig.cmake is created there
    DEPENDS "${target}"
    )

#######################################################
# Testing
#######################################################
# Download and unpack googletest at configure time
include(cmake/fetch-GoogleTest.cmake)
include(GoogleTest)

set(target_test "${target}_test")

set(re-edit_CPP_TST_DIR "${CMAKE_CURRENT_LIST_DIR}/test/cpp")

set(TEST_CASE_SOURCES
    "${re-edit_CPP_TST_DIR}/re/edit/lua/TestConfigParser.cpp"
    "${re-edit_CPP_TST_DIR}/re/edit/lua/TestDevice2D.cpp"
    "${re-edit_CPP_TST_DIR}/re/edit/lua/TestHDGui2D.cpp"
    )

add_executable("${target_test}" "${TEST_CASE_SOURCES}")
target_compile_definitions("${target_test}" PUBLIC RE_EDIT_PROJECT_DIR="${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries("${target_test}" gtest_main gmock "${target}_lib")
target_include_directories("${target_test}" PUBLIC "${re-edit_CPP_SRC_DIR}")

gtest_discover_tests("${target_test}")

add_custom_target("run_tests"
    COMMAND ${CMAKE_COMMAND} -E echo "Running tests using $<TARGET_FILE:${target_test}>"
    COMMAND "$<TARGET_FILE:${target_test}>"
    DEPENDS "${target_test}"
    )
