get_directory_property(project_name ${CMAKE_CURRENT_SOURCE_DIR} NAME)

cmake_minimum_required(VERSION 3.10)

project(project_name)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SUBDIRECTORIES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/*")

set(subdirectories "")

foreach(path ${SUBDIRECTORIES})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${path})
        list(APPEND subdirectories ${path})
    endif()
endforeach()

list(REMOVE_ITEM subdirectories ".vscode" "build"  "bin" "lib" "source" ".git")

message("Subdirectories: ${subdirectories}")

include_directories(${subdirectories})

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/static)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/shared)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/static)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/shared)

if (UNIX)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_POSIX_C_SOURCE=200809L")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_POSIX_C_SOURCE=200809L")
endif()


message(${subdirectories})

 foreach(subdir IN LISTS subdirectories)
     add_subdirectory(${subdir})
 endforeach()
# add_subdirectory(reactor)
