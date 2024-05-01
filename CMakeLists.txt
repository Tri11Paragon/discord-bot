cmake_minimum_required(VERSION 3.25)
project(discord_bot)

option(ENABLE_ADDRSAN "Enable the address sanitizer" OFF)
option(ENABLE_UBSAN "Enable the ub sanitizer" OFF)
option(ENABLE_TSAN "Enable the thread data race sanitizer" OFF)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

#cmake_policy(SET CMP0069 NEW)
#include(CheckIPOSupported)
#check_ipo_supported(RESULT supported OUTPUT error)

#set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
#set(BUILD_SHARED_LIBS OFF)
#set(POSITION_INDEPENDENT_CODE ON)

add_compile_options(-march=native)

set(SQLITE_ORM_ENABLE_CXX_17 ON)

add_subdirectory(libs/blt)
add_subdirectory(libs/DPP-10.0.29)
add_subdirectory(libs/sqlite_orm-1.8.2)

find_package(CURL)

if( supported )
    message(STATUS "IPO / LTO enabled")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET BLT PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO / LTO not supported: <${error}>")
endif()

include_directories(include/)
file(GLOB_RECURSE PROJECT_BUILD_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(discord_bot ${PROJECT_BUILD_FILES})

target_compile_options(discord_bot PUBLIC -Wall -Wpedantic -Wno-comment -march=native)
target_link_options(discord_bot PUBLIC -Wall -Wpedantic -Wno-comment)

target_link_libraries(discord_bot PUBLIC curl)
target_link_libraries(discord_bot PUBLIC dpp)
target_link_libraries(discord_bot PUBLIC sqlite_orm)
target_link_libraries(discord_bot PUBLIC BLT)

if (${ENABLE_ADDRSAN} MATCHES ON)
    target_compile_options(discord_bot PRIVATE -fsanitize=address)
    target_link_options(discord_bot PRIVATE -fsanitize=address)
endif ()

if (${ENABLE_UBSAN} MATCHES ON)
    target_compile_options(discord_bot PRIVATE -fsanitize=undefined)
    target_link_options(discord_bot PRIVATE -fsanitize=undefined)
endif ()

if (${ENABLE_TSAN} MATCHES ON)
    target_compile_options(discord_bot PRIVATE -fsanitize=thread)
    target_link_options(discord_bot PRIVATE -fsanitize=thread)
endif ()