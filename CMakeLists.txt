cmake_minimum_required(VERSION 3.24)
project(ShortestCycleVisualizer VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(BUILD_VISUALIZER "Build the raylib desktop application" ON)
option(USE_SYSTEM_RAYLIB "Use an installed raylib package instead of fetching it" OFF)
include(CTest)

function(project_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /utf-8)
    elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

function(fetch_raylib)
    include(FetchContent)
    set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)
    set(CUSTOMIZE_BUILD ON CACHE BOOL "Customize raylib for the visualizer" FORCE)
    set(SUPPORT_MODULE_RAUDIO OFF CACHE BOOL "Omit the unused raylib audio module" FORCE)
    set(USE_AUDIO OFF CACHE BOOL "Omit raylib audio system dependencies" FORCE)
    # Keep raylib 5.5's older policy declaration compatible with new CMake releases.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.10)
    FetchContent_Declare(raylib
        URL https://codeload.github.com/raysan5/raylib/zip/refs/tags/5.5
        URL_HASH SHA256=00e8a7c85f7a5221e56ee8e4e1c6642b773eae73d29379a91c8ac5149e803f42
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(raylib)
endfunction()

add_library(graph_core STATIC src/graph.c src/bfs.c)
target_include_directories(graph_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
project_warnings(graph_core)

if(BUILD_VISUALIZER)
    if(USE_SYSTEM_RAYLIB)
        find_package(raylib 5.5 CONFIG REQUIRED)
    else()
        fetch_raylib()
    endif()

    add_executable(shortest_cycle src/main.c src/app.c src/gui.c)
    target_link_libraries(shortest_cycle PRIVATE graph_core raylib)
    if(WIN32)
        set_target_properties(shortest_cycle PROPERTIES WIN32_EXECUTABLE TRUE)
        if(USE_SYSTEM_RAYLIB)
            target_link_libraries(shortest_cycle PRIVATE opengl32 gdi32 winmm)
        endif()
        if(MSVC)
            target_link_options(shortest_cycle PRIVATE /ENTRY:mainCRTStartup)
        endif()
    elseif(NOT APPLE)
        target_link_libraries(shortest_cycle PRIVATE m)
    endif()
    project_warnings(shortest_cycle)
endif()

if(BUILD_TESTING)
    add_executable(test_bfs tests/test_bfs.c)
    target_link_libraries(test_bfs PRIVATE graph_core)
    project_warnings(test_bfs)
    add_test(NAME shortest_cycle_correctness COMMAND test_bfs)
endif()
