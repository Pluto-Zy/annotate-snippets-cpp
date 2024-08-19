if (NOT DEFINED RANG_PATH)
    set(RANG_PATH "${CMAKE_SOURCE_DIR}/rang")
endif()

message(STATUS "Looking for rang sources in ${RANG_PATH}")

if (EXISTS "${RANG_PATH}" AND IS_DIRECTORY "${RANG_PATH}" AND EXISTS "${RANG_PATH}/CMakeLists.txt")
    message(STATUS "Found rang sources in ${RANG_PATH}")
    add_subdirectory(${RANG_PATH})
else()
    message(STATUS "Did not find rang sources. Fetching from GitHub...")

    include(FetchContent)
    FetchContent_Declare(
        rang
        # rang release 3.2
        URL https://github.com/agauniyal/rang/archive/refs/tags/v3.2.zip
        URL_HASH MD5=AC7B6EAC8E42F0E434F4B60C8FD7C22C
    )

    FetchContent_MakeAvailable(rang)
endif()

if (TARGET rang AND NOT TARGET rang::rang)
    message(STATUS "Creating alias target rang::rang")
    add_library(rang::rang ALIAS rang)
endif()
