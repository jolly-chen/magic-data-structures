cmake_minimum_required(VERSION 3.26)

project(
    mds
    LANGUAGES CXX
)

include_directories(include)

add_subdirectory(manual)
add_subdirectory(edg)
