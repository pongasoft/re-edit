cmake_minimum_required(VERSION 3.20)

include(FetchContent)

if(re-mock_ROOT_DIR)
  # instructs FetchContent to not download or update but use the location instead
  set(FETCHCONTENT_SOURCE_DIR_RE-MOCK "${re-mock_ROOT_DIR}")
else()
  set(FETCHCONTENT_SOURCE_DIR_RE-MOCK "")
endif()

set(re-mock_GIT_REPO "https://github.com/pongasoft/re-mock" CACHE STRING "re-mock git repository url" FORCE)
set(re-mock_GIT_TAG "v1.3.1" CACHE STRING "re-mock git tag" FORCE)
set(re-mock_DOWNLOAD_URL "${re-mock_GIT_REPO}/archive/refs/tags/${re-mock_GIT_TAG}.zip" CACHE STRING "re-mock download url" FORCE)

FetchContent_Declare(re-mock
      URL                        "${re-mock_DOWNLOAD_URL}"
      SOURCE_DIR                 "${CMAKE_BINARY_DIR}/re-mock"
      BINARY_DIR                 "${CMAKE_BINARY_DIR}/re-mock-build"
      DOWNLOAD_EXTRACT_TIMESTAMP true
      CONFIGURE_COMMAND          ""
      BUILD_COMMAND              ""
      INSTALL_COMMAND            ""
      TEST_COMMAND               ""
      )

FetchContent_GetProperties(re-mock)

if(NOT re-mock_POPULATED)

  if(FETCHCONTENT_SOURCE_DIR_RE-MOCK)
    message(STATUS "Using re-mock from local ${FETCHCONTENT_SOURCE_DIR_RE-MOCK}")
  else()
    message(STATUS "Fetching re-mock from ${re-mock_DOWNLOAD_URL}")
  endif()

  FetchContent_Populate(re-mock)

endif()

set(re-mock_ROOT_DIR ${re-mock_SOURCE_DIR})

add_subdirectory("${re-mock_SOURCE_DIR}" "${re-mock_BINARY_DIR}" EXCLUDE_FROM_ALL)
