cmake_minimum_required(VERSION 3.16)
project(trr_qt_trainer LANGUAGES CXX)

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
    src/ui/runtime_dialog.ui
    src/ui/value_writes_dialog.ui
)

target_link_libraries(trr_qt_trainer PRIVATE Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets)

if (WIN32)
    target_link_libraries(trr_qt_trainer PRIVATE dwmapi wininet)
endif()

if (MSVC)
    target_compile_options(trr_qt_trainer PRIVATE /W4)
else()
    target_compile_options(trr_qt_trainer PRIVATE -Wall -Wextra -Wpedantic)
endif()
