cmake_minimum_required(VERSION 3.5)
project(LD42)

add_subdirectory(ext/ginseng)
add_subdirectory(ext/lua)
add_subdirectory(ext/sol2)
add_subdirectory(ext/soloud)
add_subdirectory(ext/metastuff)
add_subdirectory(ext/glm)
add_subdirectory(ext/lodepng)
add_subdirectory(ext/sushi)
add_subdirectory(ext/msdfgen)

set(LD42_CXX_STANDARD 17)

add_custom_target(ld42)

file(GLOB LD42_CLIENT_SRCS CONFIGURE_DEPENDS src/*.[ch]pp src/emberjs/*.[ch]pp src/platform/*.[ch]pp)

set(LD42_CLIENT_DATA_DIR "${CMAKE_SOURCE_DIR}/data" CACHE PATH "Client data directory")

if(EMSCRIPTEN)
    option(LD42_USE_WASM "Use WASM" ON)
    set(LD42_WWW_DIR "${CMAKE_BINARY_DIR}/www" CACHE PATH "Client output directory")
    set(LD42_CLIENT_STATIC_DIR "${CMAKE_SOURCE_DIR}/static" CACHE PATH "Client static directory")
    set(LD42_TOTAL_MEMORY "33554432" CACHE STRING "Total memory to allocate in the browser")

    find_package(PythonInterp 2 REQUIRED)

    if(NOT PYTHONINTERP_FOUND)
        message(FATAL_ERROR "Python 2 not found.")
    endif()

    # Client Static
    file(GLOB_RECURSE LD42_CLIENT_STATIC CONFIGURE_DEPENDS CONFIGURE_DEPENDS ${LD42_CLIENT_STATIC_DIR}/*)
    add_custom_target(ld42_client_static
        COMMENT "Copying client static files"
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/static ${LD42_WWW_DIR}
        SOURCES ${LD42_CLIENT_STATIC})

    # Client JS
    set(LD42_CLIENT_JS
        src/emberjs/config.js
        src/emberjs/websocket.js)

    # Client Data
    file(GLOB_RECURSE LD42_CLIENT_DATA_FILES CONFIGURE_DEPENDS ${LD42_CLIENT_DATA_DIR}/*)
    set(FILE_PACKAGER $ENV{EMSCRIPTEN}/tools/file_packager.py)
    set(LD42_DATA_FILE ${LD42_WWW_DIR}/ld42_client.data)
    set(LD42_DATA_LOADER ${LD42_WWW_DIR}/ld42_client.data.js)
    add_custom_command(
        OUTPUT ${LD42_DATA_FILE} ${LD42_DATA_LOADER}
        COMMAND ${PYTHON_EXECUTABLE} ${FILE_PACKAGER} ${LD42_DATA_FILE} --preload ${LD42_CLIENT_DATA_DIR}@data --js-output=${LD42_DATA_LOADER}
        COMMENT "Packaging data files"
        DEPENDS ${LD42_CLIENT_DATA_FILES})
    add_custom_target(ld42_data
        SOURCES ${LD42_CLIENT_DATA_FILES}
        DEPENDS ${LD42_DATA_FILE} ${LD42_DATA_LOADER})

    # Client C++
    add_executable(ld42_client
        ${LD42_CLIENT_SRCS}
        ${LD42_CLIENT_JS}
        src/platform/webgl/platform.cpp)
    string(CONCAT LD42_CLIENT_LINK_FLAGS
        " -s TOTAL_MEMORY=${LD42_TOTAL_MEMORY}"
        " -s DISABLE_EXCEPTION_CATCHING=0"
        " -s ASSERTIONS=1"
        " -s FORCE_FILESYSTEM=1"
        " -s USE_SDL=2"
        " -s USE_SDL_IMAGE=2"
        " -s USE_SDL_NET=2"
        " -s USE_FREETYPE=1")
    if (LD42_USE_WASM)
        string(PREPEND LD42_CLIENT_LINK_FLAGS
            " -s BINARYEN=1"
            " -s \"BINARYEN_TRAP_MODE='js'\"")
    endif()
    string(CONCAT LD42_CLIENT_LINK_FLAGS_DEBUG
        " -g4"
        " --cpuprofiler"
        " --memoryprofiler")
    string(CONCAT LD42_CLIENT_LINK_FLAGS_RELWITHDEBINFO
        " --cpuprofiler"
        " --memoryprofiler")
    set_target_properties(ld42_client PROPERTIES
        CXX_STANDARD ${LD42_CXX_STANDARD}
        SUFFIX .js
        LINK_FLAGS "${LD42_CLIENT_LINK_FLAGS}"
        LINK_FLAGS_DEBUG "${LD42_CLIENT_LINK_FLAGS_DEBUG}"
        LINK_FLAGS_RELWITHDEBINFO "${LD42_CLIENT_LINK_FLAGS_RELWITHDEBINFO}"
        RUNTIME_OUTPUT_DIRECTORY "${LD42_WWW_DIR}")
    target_compile_definitions(ld42_client PUBLIC
        SOL_CHECK_ARGUMENTS
        SOL_PRINT_ERRORS)
    em_link_js_library(ld42_client ${LD42_CLIENT_JS})
    target_link_libraries(ld42_client
        ginseng
        sol2
        metastuff
        sushi
        msdfgen
        soloud)
    add_dependencies(ld42_client ld42_client_static ld42_data)

    add_dependencies(ld42 ld42_client)
else()
    set(LD42_DIST_DIR "${CMAKE_BINARY_DIR}/dist" CACHE PATH "Client Output Directory")

    find_package(sdl2 REQUIRED)
    add_subdirectory(ext/glad)

    # Client Data
    file(GLOB_RECURSE LD42_CLIENT_DATA_FILES ${LD42_CLIENT_DATA_DIR}/*)
    add_custom_target(ld42_data
        COMMENT "Packaging data files"
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${LD42_CLIENT_DATA_DIR} ${LD42_DIST_DIR}/data
        SOURCES ${LD42_CLIENT_DATA_FILES})

    # Emberjs Shim
    file(GLOB_RECURSE EMBERJS_SHIM_SRCS emberjs_shim_src/*.cpp emberjs_shim_src/*.hpp)
    add_library(emberjs_shim ${EMBERJS_SHIM_SRCS})
    set_target_properties(emberjs_shim PROPERTIES CXX_STANDARD 17)

    # Client C++
    add_executable(ld42_client
        ${LD42_CLIENT_SRCS}
        src/platform/native/platform.cpp)
    set_target_properties(ld42_client PROPERTIES
        CXX_STANDARD ${LD42_CXX_STANDARD}
        LINK_FLAGS "-static"
        RUNTIME_OUTPUT_DIRECTORY "${LD42_DIST_DIR}")
    target_compile_definitions(ld42_client PUBLIC
        GLM_ENABLE_EXPERIMENTAL
        SOL_CHECK_ARGUMENTS
        SOL_PRINT_ERRORS)
    target_include_directories(ld42_client PRIVATE
        ${SDL2_INCLUDE_DIRS})
    target_link_libraries(ld42_client
        emberjs_shim
        ginseng
        sol2
        metastuff
        sushi
        msdfgen
        soloud
        ${SDL2_LIBRARIES}
        glad
        png16
        z)
    if (WIN32)
        target_link_libraries(ld42_client
            ole32
            oleaut32
            imm32
            winmm
            version)
        target_compile_options(ld42_client PRIVATE
            "$<$<CONFIG:DEBUG>:-Wa,-mbig-obj>")
        set_source_files_properties(src/components.cpp PROPERTIES
            COMPILE_OPTIONS "$<$<CONFIG:DEBUG>:-Og>")
    endif()
    add_dependencies(ld42_client ld42_data)

    add_dependencies(ld42 ld42_client)
endif()
