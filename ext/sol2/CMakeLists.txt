cmake_minimum_required (VERSION 3.0)
project(Sol2)

file(GLOB_RECURSE sol2_SOURCES include/*)
add_library(sol2 INTERFACE)
set_property(TARGET sol2 PROPERTY INTERFACE_SOURCES ${sol2_SOURCES})
target_include_directories(sol2 INTERFACE include)
target_link_libraries(sol2 INTERFACE lua)
