cmake_minimum_required(VERSION 3.5)
project(MetaStuff)

file(GLOB_RECURSE METASTUFF_SOURCES include/*)
add_library(metastuff INTERFACE)
target_sources(metastuff INTERFACE ${METASTUFF_SOURCES})
target_include_directories(metastuff INTERFACE include)

