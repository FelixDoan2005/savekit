cmake_minimum_required(VERSION 3.20)

project(savekit
    VERSION 0.1.0
    DESCRIPTION "A small versioned binary serialization library"
    LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(SAVEKIT_BUILD_TESTS        "Build the unit tests"                       ON)
option(SAVEKIT_BUILD_TOOLS        "Build the command-line tools (savekit-dump)" ON)
option(SAVEKIT_ENABLE_SANITIZERS  "Build with AddressSanitizer + UBSan (GCC/Clang, not MinGW)" OFF)
option(SAVEKIT_WARNINGS_AS_ERRORS "Treat compiler warnings as errors"          OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# Shared compile settings, applied to every target via savekit_options
# ---------------------------------------------------------------------------
add_library(savekit_options INTERFACE)

if(MSVC)
    target_compile_options(savekit_options INTERFACE /W4 /permissive- /utf-8)
    if(SAVEKIT_WARNINGS_AS_ERRORS)
        target_compile_options(savekit_options INTERFACE /WX)
    endif()
else()
    target_compile_options(savekit_options INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
    if(SAVEKIT_WARNINGS_AS_ERRORS)
        target_compile_options(savekit_options INTERFACE -Werror)
    endif()
endif()

if(SAVEKIT_ENABLE_SANITIZERS)
    if(MSVC OR MINGW)
        message(WARNING "Sanitizers are not supported on this toolchain; ignoring SAVEKIT_ENABLE_SANITIZERS")
    else()
        set(_san -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
        target_compile_options(savekit_options INTERFACE ${_san})
        target_link_options(savekit_options INTERFACE ${_san})
    endif()
endif()

# ---------------------------------------------------------------------------
# Library
# ---------------------------------------------------------------------------
add_library(savekit
    src/version.cpp
    src/binary_writer.cpp
    # Add your .cpp files here as you write them, e.g.:
    # src/binary_reader.cpp
    # src/crc32.cpp
)
add_library(savekit::savekit ALIAS savekit)

target_include_directories(savekit PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(savekit PUBLIC savekit_options)

# ---------------------------------------------------------------------------
# Tools and tests
# ---------------------------------------------------------------------------
if(SAVEKIT_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(SAVEKIT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
