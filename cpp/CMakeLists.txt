cmake_minimum_required(VERSION 3.20)
project(EchoLabyrinth_SFML VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for clangd / Visual Studio IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Try to find a local installation of SFML 2.6+ first; fallback to FetchContent if not installed
find_package(SFML 2.6 COMPONENTS graphics window audio system QUIET)

if(NOT SFML_FOUND)
    message(STATUS "SFML 2.6+ not found locally. Fetching SFML 2.6.1 via FetchContent...")
    include(FetchContent)
    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG        2.6.1
        GIT_SHALLOW    ON
    )
    set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SFML)
endif()

set(ECHO_SOURCES
    src/main.cpp
    src/AcousticSystem.cpp
    src/Labyrinth.cpp
    src/Player.cpp
    src/EnemyAI.cpp
    src/SaveSystem.cpp
)

set(ECHO_HEADERS
    include/Config.hpp
    include/AcousticSystem.hpp
    include/Labyrinth.hpp
    include/Player.hpp
    include/EnemyAI.hpp
    include/SaveSystem.hpp
)

add_executable(EchoLabyrinth_SFML ${ECHO_SOURCES} ${ECHO_HEADERS})

target_include_directories(EchoLabyrinth_SFML PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(EchoLabyrinth_SFML PRIVATE
    sfml-graphics
    sfml-window
    sfml-audio
    sfml-system
)

if(MSVC)
    target_compile_options(EchoLabyrinth_SFML PRIVATE /W4 /permissive-)
else()
    target_compile_options(EchoLabyrinth_SFML PRIVATE -Wall -Wextra -Wpedantic)
endif()

# On Windows, copy OpenAL / SFML runtime DLLs to target output directory automatically
if(WIN32)
    add_custom_command(
        TARGET EchoLabyrinth_SFML
        COMMENT "Copy OpenAL and SFML DLLs to output directory"
        PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:EchoLabyrinth_SFML>
            $<TARGET_FILE_DIR:EchoLabyrinth_SFML>
        COMMAND_EXPAND_LISTS
    )
endif()
