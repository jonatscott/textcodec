# need at least ver 3.16 to understand all syntax
cmake_minimum_required(VERSION 3.16)

project(textcodec
        VERSION 1.0.2
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

# -- tests
# allow disable tests - default is ON
option(TEXTCODEC_BUILD_TESTS "Build test suite" ON)

if (TEXTCODEC_BUILD_TESTS)
        include(CTest)
        enable_testing()

        # pull in doctest at config time
        include(FetchContent)
        FetchContent_Declare(doctest
                GIT_REPOSITORY https://github.com/doctest/doctest.git
                GIT_TAG v2.4.11)

        set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
        FetchContent_MakeAvailable(doctest)
        unset(CMAKE_POLICY_VERSION_MINIMUM)

        # unit test link plus doctest
        add_executable(textcodec_tests tests/test_base64.cpp)
        target_link_libraries(textcodec_tests PRIVATE textcodec_core doctest::doctest)

        # same warnings as rest of project
        target_compile_options(textcodec_tests PRIVATE
                $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)

        # register each test case in bin as its own CTest test
        include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
        doctest_discover_tests(textcodec_tests)

        # CLI black-box tests 
        find_package(Python3 COMPONENTS Interpreter)
        if(Python3_Interpreter_FOUND)
                add_test(NAME cli_tests
                        COMMAND ${Python3_Executable}
                        ${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_test.py
                        $<TARGET_FILE:textcodec>)
        else()
                message(WARNING
                        "Python3 interpreter not found - skipping CLI tests")
        endif()
endif()

# -- install (so sudo make install puts it on PATH)

include(GNUInstallDirs)

# install into system bin dir (/usr/local/bin)
install(TARGETS textcodec
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})