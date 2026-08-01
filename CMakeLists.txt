cmake_minimum_required(VERSION 3.16)
project(trr_qt_trainer VERSION 0.0.0 LANGUAGES CXX)

set(TRR_VERSION "0.0.00" CACHE STRING "Trainer display version")
set(TRR_BUILD_NUMBER "" CACHE STRING "CI build number; defaults to the Git commit count")
set(TRR_COMMIT_SHA "" CACHE STRING "Source revision; defaults to the current short Git SHA")
set(TRR_BUILD_CHANNEL "local" CACHE STRING "Build channel, such as release or nightly")
set(TRR_BUILD_BRANCH "" CACHE STRING "Source branch; defaults to the current Git branch")

if (TRR_BUILD_NUMBER STREQUAL "")
    execute_process(
        COMMAND git rev-list --count HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE TRR_BUILD_NUMBER
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()
if (TRR_BUILD_NUMBER STREQUAL "")
    set(TRR_BUILD_NUMBER "0")
endif()

if (TRR_COMMIT_SHA STREQUAL "")
    execute_process(
        COMMAND git rev-parse --short=8 HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE TRR_COMMIT_SHA
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()
if (TRR_COMMIT_SHA STREQUAL "")
    set(TRR_COMMIT_SHA "00000000")
endif()

if (TRR_BUILD_BRANCH STREQUAL "")
    execute_process(
        COMMAND git branch --show-current
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE TRR_BUILD_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
endif()
if (TRR_BUILD_BRANCH STREQUAL "")
    set(TRR_BUILD_BRANCH "unknown")
endif()

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/build_info.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/build_info.h"
    @ONLY
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/src/ui")

if (DEFINED ENV{QTDIR})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{QTDIR}")
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Network Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network Widgets)

add_executable(trr_qt_trainer WIN32
    src/app_logger.cpp
    src/app_logger.h
    src/main.cpp
    src/rpcs3_session_controller.cpp
    src/rpcs3_session_controller.h
    src/ui/main_window.ui
    src/ui/runtime_dialog.ui
    src/ui/value_writes_dialog.ui
)

target_link_libraries(trr_qt_trainer PRIVATE Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets)
target_include_directories(trr_qt_trainer PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")

if (WIN32)
    target_link_libraries(trr_qt_trainer PRIVATE dwmapi wininet)
endif()

if (MSVC)
    target_compile_options(trr_qt_trainer PRIVATE /W4)
else()
    target_compile_options(trr_qt_trainer PRIVATE -Wall -Wextra -Wpedantic)
endif()
