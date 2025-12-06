cmake_minimum_required(VERSION 3.14)
project(ParseHttpTests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 启用CTest
include(CTest)

# 拉取GoogleTest（使用官方FetchContent方式）
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# Windows上避免覆盖父项目的编译选项
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 被测代码源文件路径
# 假设ParseHttp.cpp位于src/common，且依赖头文件在包含路径中
add_library(ParseHttpLib STATIC
  ${CMAKE_CURRENT_LIST_DIR}/../src/common/ParseHttp.cpp
)

# 头文件包含路径
# 顶层包含src/common和lib目录（含json.hpp、LogM.h）
target_include_directories(ParseHttpLib PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/../src/common
  ${CMAKE_CURRENT_LIST_DIR}/../lib
)

# 测试可执行文件
add_executable(ParseHttpTests
  main.cpp
  HttpRequestTest.cpp
)

# 链接日志库，如果是.so在Windows不可用，示例仅在Linux使用；这里仅包含头文件即可
# 如需在Linux链接，请改为：target_link_libraries(ParseHttpLib PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../lib/libLogM.so)

# 测试可执行文件链接
target_link_libraries(ParseHttpTests
  PRIVATE
    ParseHttpLib
    GTest::gtest
    GTest::gtest_main
)

# 注册CTest测试
add_test(NAME ParseHttpTests COMMAND ParseHttpTests)
