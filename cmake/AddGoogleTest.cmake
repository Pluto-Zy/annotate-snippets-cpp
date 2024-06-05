if (NOT DEFINED GOOGLETEST_PATH)
    set(GOOGLETEST_PATH "${CMAKE_SOURCE_DIR}/googletest")
endif()

message(STATUS "Looking for googletest sources")
message(STATUS "Looking for googletest sources in ${GOOGLETEST_PATH}")

if (EXISTS "${GOOGLETEST_PATH}"            AND IS_DIRECTORY "${GOOGLETEST_PATH}"            AND EXISTS "${GOOGLETEST_PATH}/CMakeLists.txt" AND
    EXISTS "${GOOGLETEST_PATH}/googletest" AND IS_DIRECTORY "${GOOGLETEST_PATH}/googletest" AND EXISTS "${GOOGLETEST_PATH}/googletest/CMakeLists.txt" AND
    EXISTS "${GOOGLETEST_PATH}/googlemock" AND IS_DIRECTORY "${GOOGLETEST_PATH}/googlemock" AND EXISTS "${GOOGLETEST_PATH}/googlemock/CMakeLists.txt")
    message(STATUS "Looking for googletest sources - Found")

    add_subdirectory(${GOOGLETEST_PATH})
else()
    message(STATUS "Looking for googletest sources - Not Found")
    message(STATUS "Did not find googletest sources. Fetching from GitHub...")

    include(FetchContent)
    FetchContent_Declare(
        googletest
        # version 1.14.0
        URL https://github.com/google/googletest/archive/f8d7d77c06936315286eb55f8de22cd23c188571.zip
        URL_HASH MD5=215bb9e5da4985c3ea28a0bfc6e883c1
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    include(GoogleTest)
endif()
