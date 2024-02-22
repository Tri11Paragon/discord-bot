cmake_minimum_required(VERSION 3.27)
project(discord_bot)

option(ENABLE_ADDRSAN "Enable the address sanitizer" OFF)
option(ENABLE_UBSAN "Enable the ub sanitizer" OFF)
option(ENABLE_TSAN "Enable the thread data race sanitizer" OFF)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(libs/blt)
add_subdirectory(libs/DPP-10.0.29)

include_directories(include/)
file(GLOB_RECURSE PROJECT_BUILD_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(discord_bot ${PROJECT_BUILD_FILES})

target_compile_options(discord_bot PRIVATE -Wall -Werror -Wpedantic -Wno-comment)
target_link_options(discord_bot PRIVATE -Wall -Werror -Wpedantic -Wno-comment)

target_link_libraries(discord_bot PUBLIC BLT)
target_link_libraries(discord_bot PUBLIC dpp)

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