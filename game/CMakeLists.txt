cmake_minimum_required(VERSION 3.31.1)
project(myfolder VERSION 0.1.0 LANGUAGES C CXX)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(raylib CONFIG REQUIRED)

add_executable(myfolder main.cpp)

target_link_libraries(myfolder PRIVATE raylib)
