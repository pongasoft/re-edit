cmake_minimum_required(VERSION 3.24)

include(cmake/REEditFetchContent.cmake)

#------------------------------------------------------------------------
# Git repo/tag for re-mock
#------------------------------------------------------------------------
set(re-mock_GIT_REPO "https://codeberg.org/pongasoft/re-mock" CACHE STRING "re-mock git repository url")
set(re-mock_GIT_TAG "v4.6.0.20260703" CACHE STRING "re-mock git tag")
set(re-mock_DOWNLOAD_URL "${re-mock_GIT_REPO}/archive/${re-mock_GIT_TAG}.zip" CACHE STRING "re-mock download url")

re_edit_fetch_content(NAME re-mock)

set(re-mock_ROOT_DIR ${re-mock_SOURCE_DIR})

add_subdirectory("${re-mock_SOURCE_DIR}" "${re-mock_BINARY_DIR}" EXCLUDE_FROM_ALL)
