cmake_minimum_required(VERSION 3.15)
project(utilities)

set(CMAKE_CXX_STANDARD 17)

add_executable(utilities main.cpp file/EndianChanger.hpp audio/WavReader.hpp file/file_size_tools.hpp)