# ============================================================
# 《黄埔江上又一人》· 校园浮生录 — CMake 构建脚本（组长 A）
# 课程要求：C++17，不使用游戏引擎，仅 C++ 标准库 + 内置 nlohmann/json。
# 本机若无 CMake，用 build.bat（MSVC 直接编译）即可。
# ============================================================
cmake_minimum_required(VERSION 3.15)
project(HuangpuSim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# 收集源码
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
list(FILTER LIB_SOURCES EXCLUDE_REGEX ".*/main/.*")

# 公共库（游戏规则层：core/economy/player/events/story/time/ui/save/simulate）
add_library(huangpu_core STATIC ${LIB_SOURCES})
if (MSVC)
    target_compile_options(huangpu_core PRIVATE /utf-8 /W3 /permissive-)
else()
    target_compile_options(huangpu_core PRIVATE -Wall -Wextra)
endif()

# 目标 1：交互版垂直切片（控制台 demo）
add_executable(huangpu_sim ${CMAKE_CURRENT_SOURCE_DIR}/main/main_console.cpp)
target_link_libraries(huangpu_sim PRIVATE huangpu_core)

# 目标 2：headless 模拟器（报告"结果分析"的数据来源）
add_executable(huangpu_simulate ${CMAKE_CURRENT_SOURCE_DIR}/main/main_simulate.cpp)
target_link_libraries(huangpu_simulate PRIVATE huangpu_core)

# 目标 3：规则测试
add_executable(huangpu_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_main.cpp)
target_link_libraries(huangpu_tests PRIVATE huangpu_core)

# 把 data/ 拷到运行目录，保证任意目录构建后可直接运行
add_custom_command(TARGET huangpu_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data $<TARGET_FILE_DIR:huangpu_sim>/data)
