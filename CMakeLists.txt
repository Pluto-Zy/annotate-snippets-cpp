cmake_minimum_required(VERSION 3.14)
project(annotate-snippets VERSION 1.0.0)

include(GNUInstallDirs)

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif ()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message("Setting build type to 'Debug' as none was specified.")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE
        PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Determine if the current project is the main project.
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(ANNOTATE_SNIPPETS_MAIN_PROJECT ON)
else ()
    set(ANNOTATE_SNIPPETS_MAIN_PROJECT OFF)
endif ()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Enable testing by default only if this is the main project.
option(
    ANNOTATE_SNIPPETS_ENABLE_TESTING
    "Enable testing of the annotate-snippets project."
    ${ANNOTATE_SNIPPETS_MAIN_PROJECT}
)
option(INSTALL_ANNOTATE_SNIPPETS "Enable installation of the annotate-snippets project." ON)

include(CPM)
CPMAddPackage(URI "gh:agauniyal/rang#v3.2" DOWNLOAD_ONLY ON)

add_library(annotate_snippets
    src/annotated_source.cpp
    src/source_file.cpp
    src/style_spec.cpp
    src/detail/styled_string_impl.cpp
    src/renderer/human_renderer.cpp
)

target_include_directories(annotate_snippets
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        $<BUILD_INTERFACE:${rang_SOURCE_DIR}/include>
)

set(
    ANTS_WARNING_OPTIONS
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Werror>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
)
target_compile_options(annotate_snippets PRIVATE ${ANTS_WARNING_OPTIONS})

target_compile_features(annotate_snippets PUBLIC cxx_std_17)

add_library(ants::annotate_snippets ALIAS annotate_snippets)

if (ANNOTATE_SNIPPETS_ENABLE_TESTING)
    enable_testing()
    add_subdirectory(test)
endif ()

if (INSTALL_ANNOTATE_SNIPPETS)
    set(ANNOTATE_SNIPPETS_CMAKE_INSTALL_PATH "${CMAKE_INSTALL_LIBDIR}/cmake/annotate-snippets")

    # Installation setup
    install(TARGETS annotate_snippets
        EXPORT annotate-snippets-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(DIRECTORY include/annotate_snippets
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT annotate-snippets-targets
        FILE annotate-snippets-targets.cmake
        NAMESPACE ants::
        DESTINATION ${ANNOTATE_SNIPPETS_CMAKE_INSTALL_PATH}
    )

    include(CMakePackageConfigHelpers)

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/annotate-snippets-config-version.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    configure_package_config_file(
        cmake/annotate-snippets-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/annotate-snippets-config.cmake
        INSTALL_DESTINATION ${ANNOTATE_SNIPPETS_CMAKE_INSTALL_PATH}
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/annotate-snippets-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/annotate-snippets-config-version.cmake
        DESTINATION ${ANNOTATE_SNIPPETS_CMAKE_INSTALL_PATH}
    )
endif ()
