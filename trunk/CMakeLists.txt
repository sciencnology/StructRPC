cmake_minimum_required(VERSION 3.10)

# 项目名称和版本
project(ExampleStructRpc VERSION 1.0)
set(CMAKE_CXX_COMPILER "/home/uranus/gcc_13.2.0/bin/g++")
# 指定 C 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-g)
# 添加 include 目录
include_directories(/usr/local/include include sdk logic utils ./ ./StructBuuffer/trunk)

# 设置源文件目录
set(SRC_DIRS logic src)

# 查找所有源文件
file(GLOB_RECURSE SOURCES 
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/logic/*.cpp
    ${PROJECT_SOURCE_DIR}/thirdparty/**/*.cpp
)
# 添加可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 指定生成目录
# set_target_properties(${PROJECT_NAME} PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build
# )

link_directories(/usr/local/lib)

target_link_libraries(${PROJECT_NAME} boost_system boost_thread pthread)
