cmake_minimum_required(VERSION 3.30)
project(qt_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets Network Test REQUIRED)

add_executable(qt_client src/main.cpp src/main_window.cpp src/main_window.h)
target_link_libraries(qt_client PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)

add_executable(qt_client_test tests/main_window_test.cpp src/main_window.cpp src/main_window.h)
target_link_libraries(qt_client_test PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Test)
