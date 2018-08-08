cmake_minimum_required (VERSION 3.0)
project(GLAD)

add_library(glad src/glad.c include/glad/glad.h include/KHR/khrplatform.h)
target_include_directories(glad PUBLIC include)
