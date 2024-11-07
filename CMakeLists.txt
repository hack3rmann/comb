cmake_minimum_required(VERSION 3.28.2)

project(comb)

include(FetchContent)
set(FETCHCONTENT_QUIET NO)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
execute_process(COMMAND ln -sfT ${BUILD_DIR}/compile_commands.json
    ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json)

add_executable(comb_tests
    tests/main.cpp
    tests/parse/basic.cpp
    tests/parse/parser.cpp)

target_include_directories(comb_tests PRIVATE include)

set(FMT_VERSION 11.0.2)
find_package(fmt ${FMT_VERSION} QUIET)

if (NOT fmt_FOUND)
    FetchContent_Declare(
        fmt
        DOWNLOAD_EXTRACT_TIMESTAMP OFF
        URL https://github.com/fmtlib/fmt/archive/refs/tags/${FMT_VERSION}.tar.gz)

    FetchContent_MakeAvailable(fmt)

    target_link_libraries(comb_tests fmt)
endif()

