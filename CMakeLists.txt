cmake_minimum_required(VERSION 3.23)
set(CMAKE_C_COMPILER "C:\\msys64\\mingw64\\bin\\gcc.exe")
set(CMAKE_CXX_COMPILER "C:\\msys64\\mingw64\\bin\\g++.exe")

project(threadpool)

include_directories(./src)

add_executable(pool ./main.cpp)