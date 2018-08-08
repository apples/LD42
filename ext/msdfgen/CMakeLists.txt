cmake_minimum_required(VERSION 3.5)
project(msdfgen)

function(msdfgen_link_pkgconfig TARGETNAME LIBNAME)
    string(TOUPPER ${LIBNAME} LIBPREFIX)
    pkg_search_module(${LIBPREFIX} REQUIRED ${LIBNAME})
    target_link_libraries(${TARGETNAME} ${${LIBPREFIX}_LIBRARIES})
    target_include_directories(${TARGETNAME} PUBLIC ${${LIBPREFIX}_INCLUDE_DIRS})
    target_compile_options(${TARGETNAME} PUBLIC ${${LIBPREFIX}_CFLAGS_OTHER})
endfunction()

file(GLOB_RECURSE msdfgen_HEADERS
    "core/*.h"
    "lib/*.h"
    "ext/*.h"
    "include/*.h"
)

file(GLOB_RECURSE msdfgen_SOURCES
    "core/*.cpp"
    "lib/*.cpp"
    "ext/*.cpp"
)

add_library(msdfgen ${msdfgen_SOURCES} ${msdfgen_HEADERS})
target_include_directories(msdfgen PUBLIC "include")
target_include_directories(msdfgen INTERFACE ".")

if(EMSCRIPTEN)
    set_target_properties(msdfgen PROPERTIES
        COMPILE_FLAGS "-s USE_FREETYPE=1"
        LINK_FLAGS "-s USE_FREETYPE=1")
else()
    find_package(PkgConfig)
    msdfgen_link_pkgconfig(msdfgen freetype2)
    msdfgen_link_pkgconfig(msdfgen libpng)
    msdfgen_link_pkgconfig(msdfgen harfbuzz)
    msdfgen_link_pkgconfig(msdfgen bzip2)
    msdfgen_link_pkgconfig(msdfgen graphite2)
    target_link_libraries(msdfgen ${FREETYPE2_LIBRARIES}) #Cyclic dependency for harfbuzz
endif()

