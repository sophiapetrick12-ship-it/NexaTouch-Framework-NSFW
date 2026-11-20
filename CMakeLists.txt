cmake_minimum_required(VERSION 3.24)

project(NexaTouchFramework VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(CommonLibSSE CONFIG REQUIRED)

set(NEXATOUCH_SOURCES
    src/main.cpp
)

add_commonlibsse_plugin(${PROJECT_NAME}
    SOURCES ${NEXATOUCH_SOURCES}
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PRIVATE NEXATOUCH_VERSION="${PROJECT_VERSION}")
