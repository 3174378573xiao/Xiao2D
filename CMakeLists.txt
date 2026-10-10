# threelib/Xiao2D/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ========= SDL预编译包(find_package，二进制包) =========
set(SDL3_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SDL/lib/cmake/SDL3")
set(SDL3_ttf_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SDL_ttf/lib/cmake/SDL3_ttf")

find_package(SDL3 REQUIRED CONFIG)
find_package(SDL3_ttf REQUIRED CONFIG)

# 编译Xiao2D为静态库 STATIC
add_library(Xiao2D STATIC
    Xiao2D.cpp
    Xiao2D.h
)

# PUBLIC：链接Xiao2D的目标自动获得头文件搜索路径
# ${CMAKE_CURRENT_SOURCE_DIR} = threelib/Xiao2D
target_include_directories(Xiao2D
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Xiao2D内部依赖SDL3/SDL3_ttf，在这里写上PRIVATE
target_link_libraries(Xiao2D
    PUBLIC
        SDL3::SDL3
        SDL3_ttf::SDL3_ttf
)
