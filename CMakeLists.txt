cmake_minimum_required(VERSION 3.24)

project(NexaTouchFramework VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(ENABLE_SKYRIM_SE ON CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_AE OFF CACHE BOOL "" FORCE)
set(ENABLE_SKYRIM_VR OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    CommonLibSSE
    GIT_REPOSITORY https://github.com/alandtse/CommonLibSSE-NG.git
    GIT_TAG ng
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(CommonLibSSE)

# CommonLibSSE-NG defines the plugin helper in this CMake module.
FetchContent_GetProperties(CommonLibSSE SOURCE_DIR COMMONLIBSSE_SOURCE_DIR)
include("${COMMONLIBSSE_SOURCE_DIR}/cmake/CommonLibSSE.cmake")

add_commonlibsse_plugin(${PROJECT_NAME}
    SOURCES src/main.cpp src/Config.cpp src/InteractionController.cpp src/SettingsMenu.cpp src/SkeletonProbe.cpp
    NAME "NexaTouchFramework"
    AUTHOR "NexaTouch"
    VERSION "${PROJECT_VERSION}"
)

# CommonLibSSE headers rely on a shared precompiled header of standard-library headers.
target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)

target_include_directories(${PROJECT_NAME} PRIVATE include)

# Expose the CMake project version to the plugin source as a string literal.
target_compile_definitions(${PROJECT_NAME} PRIVATE NEXATOUCH_VERSION="${PROJECT_VERSION}")
