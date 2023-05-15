cmake_minimum_required(VERSION 3.24)

include(cmake/REEditFetchContent.cmake)

#------------------------------------------------------------------------
# Git repo/tag for re-mock
#------------------------------------------------------------------------
set(re-mock_GIT_REPO "https://github.com/pongasoft/re-mock" CACHE STRING "re-mock git repository url")
set(re-mock_GIT_TAG "v1.4.3" CACHE STRING "re-mock git tag")
set(re-mock_DOWNLOAD_URL "${re-mock_GIT_REPO}/archive/refs/tags/${re-mock_GIT_TAG}.zip" CACHE STRING "re-mock download url" FORCE)
set(re-mock_DOWNLOAD_URL_HASH "SHA256=251340fdc1ddb4a44b55ab4b55e9dad3f3b1d4de70a46af183577523ca87e27a" CACHE STRING "re-mock download url hash" FORCE)

re_edit_fetch_content(NAME re-mock)

set(re-mock_ROOT_DIR ${re-mock_SOURCE_DIR})

add_subdirectory("${re-mock_SOURCE_DIR}" "${re-mock_BINARY_DIR}" EXCLUDE_FROM_ALL)
