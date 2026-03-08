cmake_minimum_required(VERSION 3.24)

include(cmake/REEditFetchContent.cmake)

#------------------------------------------------------------------------
# Git repo/tag for re-mock
#------------------------------------------------------------------------
set(re-mock_GIT_REPO "https://codeberg.org/pongasoft/re-mock" CACHE STRING "re-mock git repository url")
set(re-mock_GIT_TAG "v1.8.1" CACHE STRING "re-mock git tag")
set(re-mock_DOWNLOAD_URL "${re-mock_GIT_REPO}/archive/${re-mock_GIT_TAG}.zip" CACHE STRING "re-mock download url")
set(re-mock_DOWNLOAD_URL_HASH "SHA256=929b64f5abbd1c2e80b8fdc2eb31eea12d8d9b6e9019165dee51eaf117ddeda1" CACHE STRING "re-mock download url hash")

re_edit_fetch_content(NAME re-mock)

set(re-mock_ROOT_DIR ${re-mock_SOURCE_DIR})

add_subdirectory("${re-mock_SOURCE_DIR}" "${re-mock_BINARY_DIR}" EXCLUDE_FROM_ALL)
