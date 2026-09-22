# need at least ver 3.16 to understand all syntax
cmake_minimum_required(VERSION 3.16)

project(textcodec
        VERSION 1.0.1
        DESCRIPTION "Headless Base64 encode/decode tool driver on SSH"
        LANGUAGES CXX)

# -- standard build settings
set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# if not select build type, default to Release
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# -- library with reusable logic
# build static library
add_library(textcodec_core
        src/base64.cpp
        src/io.cpp)

# expose the include/ dir to lib 
target_include_directories(textcodec_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

# turn on strict warnings, but only for GCC/Clang
target_compile_options(textcodec_core PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)


# -- executable

add_executable(textcodec src/main.cpp)

# link exe to main libraries 
target_link_libraries(textcodec PRIVATE textcodec_core)

# apply strict warnings to executable's translation unit 
target_compile_options(textcodec PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)

# -- install (so sudo make install puts it on PATH)

include(GNUInstallDirs)

# install into system bin dir (/usr/local/bin)
install(TARGETS textcodec
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})