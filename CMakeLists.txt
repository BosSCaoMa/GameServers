cmake_minimum_required(VERSION 3.16)
project(MyGameServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==== libsodium，通过 pkg-config 查 ====
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSODIUM REQUIRED libsodium)

# ==== 线程库 ====
find_package(Threads REQUIRED)

# ==== 头文件目录 ====
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DataServer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Login/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Connect/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Login/axis

    # libsodium 头文件
    ${LIBSODIUM_INCLUDE_DIRS}

    # 如果你的 mysql-connector-c++ 头文件不在系统默认 include 目录，
    # 比如在 /usr/include/mysql-cppconn，就在这里再加一行：
    # /usr/include/mysql-cppconn
)

# ==== 源文件收集 ====
file(GLOB_RECURSE COMMON_SRC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DataServer/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Login/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Login/axis/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Connect/*.cpp
)

file(GLOB MAIN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

add_executable(MyGameServer ${MAIN_SRC} ${COMMON_SRC})

# ==== 链接库 ====
target_link_libraries(MyGameServer
    ${LIBSODIUM_LIBRARIES}  # libsodium

    mysqlcppconn            # MySQL Connector/C++ 主库
    # 某些发行版库名可能是 mysqlcppconn8，如果链接报错就改成这个

    Threads::Threads        # CMake 官方推荐的线程库写法
)
