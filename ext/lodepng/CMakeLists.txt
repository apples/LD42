cmake_minimum_required(VERSION 3.2)
project(lodepng)

add_library(lodepng EXCLUDE_FROM_ALL
    lodepng.h lodepng.cpp
)
set_property(TARGET lodepng PROPERTY CXX_STANDARD 14)
target_include_directories(lodepng PUBLIC ./)
