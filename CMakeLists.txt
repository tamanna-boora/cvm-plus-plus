cmake_minimum_required(VERSION 3.15)
project(cvm++ CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /WX /O2)
else()
    add_compile_options(-Wall -Wextra -O2)
endif()

add_executable(cvm src/main.cpp)
target_include_directories(cvm PRIVATE src)
