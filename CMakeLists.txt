cmake_minimum_required(VERSION 3.16)
project(FileTree LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()


find_package(Threads REQUIRED)
find_package(spdlog CONFIG REQUIRED)


add_executable(ft main.cc)
target_link_libraries(ft PRIVATE Threads::Threads spdlog::spdlog)