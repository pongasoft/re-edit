cmake_minimum_required(VERSION 3.24)

set(re-edit_VERSION_MAJOR 1)
set(re-edit_VERSION_MINOR 6)
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

#######################################################
# Including raylib
#######################################################
option(CUSTOMIZE_BUILD "" ON)
option(USE_AUDIO "" OFF)
option(SUPPORT_MODULE_RSHAPES "" ON)
option(SUPPORT_MODULE_RTEXT "" OFF)
option(SUPPORT_MODULE_RMODELS "" OFF)
option(SUPPORT_MODULE_RAUDIO "" OFF)
option(SUPPORT_CAMERA_SYSTEM "" OFF)
option(SUPPORT_GESTURES_SYSTEM "" OFF)
option(SUPPORT_MOUSE_GESTURES "" OFF)
option(SUPPORT_DEFAULT_FONT "" OFF)
option(SUPPORT_SCREEN_CAPTURE "" OFF)
option(SUPPORT_GIF_RECORDING "" OFF)
option(SUPPORT_QUADS_DRAW_MODE "" OFF)
option(SUPPORT_IMAGE_EXPORT "" ON)
option(SUPPORT_IMAGE_GENERATION "" OFF)
option(SUPPORT_IMAGE_MANIPULATION "" ON)
option(SUPPORT_FILEFORMAT_DDS "" OFF)
option(SUPPORT_FILEFORMAT_HDR "" OFF)
option(SUPPORT_FILEFORMAT_PNM "" OFF)
option(SUPPORT_FILEFORMAT_KTX "" OFF)
option(SUPPORT_FILEFORMAT_ASTC "" OFF)
option(SUPPORT_FILEFORMAT_FNT "" OFF)
option(SUPPORT_FILEFORMAT_TTF "" OFF)
option(SUPPORT_TEXT_MANIPULATION "" OFF)
option(SUPPORT_MESH_GENERATION "" OFF)
option(SUPPORT_FILEFORMAT_OBJ "" OFF)
option(SUPPORT_FILEFORMAT_MTL "" OFF)
option(SUPPORT_FILEFORMAT_IQM "" OFF)
option(SUPPORT_FILEFORMAT_GLTF "" OFF)
option(SUPPORT_FILEFORMAT_VOX "" OFF)
option(SUPPORT_FILEFORMAT_WAV "" OFF)
option(SUPPORT_FILEFORMAT_OGG "" OFF)
option(SUPPORT_FILEFORMAT_XM "" OFF)
option(SUPPORT_FILEFORMAT_MOD "" OFF)
option(SUPPORT_FILEFORMAT_MP3 "" OFF)
add_subdirectory("${CMAKE_SOURCE_DIR}/external/raysan5/raylib" raylib EXCLUDE_FROM_ALL)
# code assumes it is an embedded device with low memory...
target_compile_definitions(raylib PRIVATE "-DRL_DEFAULT_BATCH_BUFFER_ELEMENTS=8192")

#######################################################
# Including Dear ImGui
#######################################################
add_subdirectory(external/ocornut/imgui EXCLUDE_FROM_ALL)

#######################################################
# Including rlImGui (ImGui backend)
#######################################################
add_subdirectory("${CMAKE_SOURCE_DIR}/external/raylib-extras/rlImGui" rlImGui EXCLUDE_FROM_ALL)

#######################################################
# Include nativefiledialog (open native file dialog)
#######################################################
add_subdirectory(external/btzy/nativefiledialog-extended EXCLUDE_FROM_ALL)

#######################################################
# Include efsw (cross platform filesystem watcher)
#######################################################
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
    "${re-edit_CPP_SRC_DIR}/re/edit/lua/Base.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/lua/Base.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/lua/ConfigParser.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/lua/ConfigParser.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/lua/Device2D.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/lua/Device2D.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/lua/HDGui2D.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/lua/HDGui2D.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/platform/NativeApplication.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/platform/RLContext.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/platform/RLContext.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/AppContext.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/AppContext.hpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/AppContext.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/Application.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/Application.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/BuiltIns.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/Canvas.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/Canvas.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/Clipboard.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/Color.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/Color.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/Config.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/Constants.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/Dialog.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/Dialog.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/Errors.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/FilmStrip.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/FilmStrip.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/FontManager.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/FontManager.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/Grid.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/LoggingManager.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/LoggingManager.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/Graphics.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/Graphics.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/NetworkManager.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/NetworkManager.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/Panel.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/Panel.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/PanelActions.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/PanelState.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/PanelState.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/PreferencesManager.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/PreferencesManager.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/Property.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/PropertyManager.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/ReGui.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/ReGui.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/String.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/String.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/TextureManager.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/TextureManager.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/UIContext.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/UIContext.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/UndoManager.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/UndoManager.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/Views.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/Views.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/Widget.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/Widget.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/WidgetActions.cpp"
    "${re-edit_CPP_SRC_DIR}/re/edit/WidgetAttribute.h"
    "${re-edit_CPP_SRC_DIR}/re/edit/WidgetAttribute.cpp")

set(VERSION_DIR "${CMAKE_BINARY_DIR}/generated")
configure_file("${CMAKE_CURRENT_LIST_DIR}/src/cpp/re/edit/version.h.in" "${VERSION_DIR}/version.h")

# Fonts
set(RE_EDIT_FONTS_DIR "${CMAKE_CURRENT_LIST_DIR}/external/fonts")

set(re-edit_MAIN_SRC "${re-edit_CPP_SRC_DIR}/re/edit/platform/main.cpp")

# Defines the macos main
if(APPLE)
  list(APPEND re-edit_BUILD_SOURCES
      "${re-edit_CPP_SRC_DIR}/re/edit/platform/macos/MacOSNetworkManager.mm"
      "${re-edit_CPP_SRC_DIR}/re/edit/platform/macos/MacOSMultipleInstanceManager.mm"
      "${re-edit_CPP_SRC_DIR}/re/edit/platform/macos/MacOSNativeApplication.h"
      "${re-edit_CPP_SRC_DIR}/re/edit/platform/macos/MacOSNativeApplication.cpp"
      "${re-edit_CPP_SRC_DIR}/re/edit/platform/macos/NSUserDefaultsManager.mm"
      )
  list(APPEND re-edit_MAIN_SRC "${re-edit_CPP_SRC_DIR}/re/edit/platform/macos/main.cpp")
elseif(WIN32)
  list(APPEND re-edit_BUILD_SOURCES
      "${re-edit_CPP_SRC_DIR}/re/edit/platform/windows/LocalSettingsManager.cpp"
      "${re-edit_CPP_SRC_DIR}/re/edit/platform/windows/WindowsNativeApplication.h"
      "${re-edit_CPP_SRC_DIR}/re/edit/platform/windows/WindowsNativeApplication.cpp"
      "${re-edit_CPP_SRC_DIR}/re/edit/platform/windows/WindowsMultipleInstanceManager.cpp"
      "${re-edit_CPP_SRC_DIR}/re/edit/platform/windows/WindowsNetworkManager.cpp"
      )
  list(APPEND re-edit_MAIN_SRC
      "${re-edit_CPP_SRC_DIR}/re/edit/platform/windows/main.cpp"
      "${re-edit_RESOURCES_DIR}/re-edit.rc"
      )
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
target_link_libraries("${target}_lib" PUBLIC raylib imgui rlImGui re-mock nfd efsw "${re-edit_LINK_LIBRARIES}")
target_compile_definitions("${target}_lib" PUBLIC ENABLE_RE_EDIT_INTERNAL_ASSERT=1)

# Build the executable
add_executable(${target} WIN32 MACOSX_BUNDLE "${re-edit_MAIN_SRC}")
set_target_properties(${target} PROPERTIES OUTPUT_NAME "RE Edit")
target_link_libraries(${target} PRIVATE "${target}_lib")
if(APPLE)
  set_target_properties(${target} PROPERTIES
      BUNDLE True
      MACOSX_BUNDLE_GUI_IDENTIFIER com.pongasoft.re-edit
      MACOSX_BUNDLE_BUNDLE_NAME "RE Edit"
      MACOSX_BUNDLE_BUNDLE_VERSION "${re-edit_VERSION}"
      MACOSX_BUNDLE_SHORT_VERSION_STRING "${re-edit_VERSION}"
      MACOSX_BUNDLE_ICON_NAME "AppIcon"
      MACOSX_BUNDLE_COPYRIGHT "© 2023 pongasoft"
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/MacOSXBundleInfo.plist.in"
      )
  target_compile_definitions("${target}_lib" PUBLIC GL_SILENCE_DEPRECATION)

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
    "${re-edit_CPP_TST_DIR}/re/edit/lua/TestMisc.cpp"
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
