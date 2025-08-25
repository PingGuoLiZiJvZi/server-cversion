
get_filename_component(project_name ${CMAKE_CURRENT_SOURCE_DIR} NAME)

cmake_minimum_required(VERSION 3.10)

project(${project_name})

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB file_list "*.cpp" "*.c")

message(${file_list})

add_library(${project_name} STATIC ${file_list})