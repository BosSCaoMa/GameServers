cmake_minimum_required(VERSION 3.26)
project(MyGameServer)

set(CMAKE_CXX_STANDARD 17)

# ==== 新增：通过 pkg-config 找 libsodium ====
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSODIUM REQUIRED libsodium)

# 1. 包含头文件目录（库的头文件 + 项目自身头文件）
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include  # 项目自身头文件
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/include  # 动态库的头文件（若有）
    ${LIBSODIUM_INCLUDE_DIRS}
)

# 2. 指定动态库所在目录（若库在系统路径如 /usr/lib 可省略）
link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    ${LIBSODIUM_LIBRARY_DIRS} # <-- 新增：libsodium 库目录
)

# 3. 收集源文件
file(GLOB MAIN_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)


add_executable(MyGameServer ${MAIN_SRC})

target_link_libraries(MyGameServer
    LogM
    ${LIBSODIUM_LIBRARIES}                       # <-- 新增：链接 libsodium
)
