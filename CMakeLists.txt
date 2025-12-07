cmake_minimum_required(VERSION 3.16)
project(MyGameServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==== 新增：通过 pkg-config 找 libsodium ====
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSODIUM REQUIRED libsodium)
find_package(MySQL REQUIRED)
find_package(Threads REQUIRED)

# 1. 包含头文件目录（库的头文件 + 项目自身头文件）
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DataServer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Login/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Connect/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Login/axis
    ${LIBSODIUM_INCLUDE_DIRS}
    ${MYSQL_INCLUDE_DIRS}
)

# Collect all source files
file(GLOB_RECURSE COMMON_SRC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DataServer/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Login/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Login/axis/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Connect/*.cpp
)

file(GLOB MAIN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

add_executable(MyGameServer ${MAIN_SRC} ${COMMON_SRC})

# Link libraries
target_link_libraries(MyGameServer
    ${LIBSODIUM_LIBRARIES}
    ${MYSQL_LIBRARIES}
    mysqlcppconn
    Threads::Threads
)
