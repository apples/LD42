cmake_minimum_required (VERSION 3.0)
project(Lua)

file(GLOB_RECURSE ginseng_SOURCES include/ginseng/*)
add_library(lua
    src/lapi.c src/lcode.c src/lctype.c src/ldebug.c src/ldo.c src/ldump.c src/lfunc.c src/lgc.c src/llex.c
    src/lmem.c src/lobject.c src/lopcodes.c src/lparser.c src/lstate.c src/lstring.c src/ltable.c src/ltm.c
    src/lundump.c src/lvm.c src/lzio.c src/lauxlib.c src/lbaselib.c src/lbitlib.c src/lcorolib.c src/ldblib.c
    src/liolib.c src/lmathlib.c src/loslib.c src/lstrlib.c src/ltablib.c src/lutf8lib.c src/loadlib.c src/linit.c)
set_property(TARGET lua PROPERTY C_STANDARD 99)
target_include_directories(lua INTERFACE src)
target_compile_definitions(lua PUBLIC "LUA_COMPAT_5_2")
