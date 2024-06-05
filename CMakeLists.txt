cmake_minimum_required(VERSION 3.14)
project(annotate-snippets)

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message("Setting build type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE
        PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

option(ANNOTATE_SNIPPETS_ENABLE_TESTING "Enable testing of the annotate-snippets project." ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (ANNOTATE_SNIPPETS_ENABLE_TESTING)
    enable_testing()
    add_subdirectory(test)
endif ()
