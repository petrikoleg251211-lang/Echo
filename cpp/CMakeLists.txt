cmake_minimum_required(VERSION 3.20)
project(EchoLabyrinth_SFML VERSION 4.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Detect local SFML 2.5 / 2.6 installation first; fallback to FetchContent (SFML 2.6.1) if absent
find_package(SFML 2.5 COMPONENTS graphics window audio system QUIET)

if(NOT SFML_FOUND)
    message(STATUS "Local SFML 2.5+ not found. Fetching SFML 2.6.1 via CMake FetchContent...")
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
    src/Game.cpp
    src/AssetManager.cpp
    src/AcousticSystem.cpp
    src/AudioManager.cpp
    src/UiRenderer.cpp
    src/Labyrinth.cpp
    src/Player.cpp
    src/EnemyAI.cpp
    src/SaveSystem.cpp
)

set(ECHO_HEADERS
    include/Config.hpp
    include/Game.hpp
    include/AssetManager.hpp
    include/AcousticSystem.hpp
    include/AudioManager.hpp
    include/UiRenderer.hpp
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
    target_compile_options(EchoLabyrinth_SFML PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(EchoLabyrinth_SFML PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Copy assets/ directory next to the compiled executable so 3D .obj models & manifest are always available
add_custom_command(
    TARGET EchoLabyrinth_SFML POST_BUILD
    COMMENT "Copying EchoLabyrinth 3D models & asset manifest to output directory..."
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:EchoLabyrinth_SFML>/assets
)

# On Windows, copy runtime DLLs (openal32.dll, sfml-*.dll) next to the compiled executable
if(WIN32 AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
    add_custom_command(
        TARGET EchoLabyrinth_SFML POST_BUILD
        COMMENT "Copying runtime DLLs to output directory..."
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:EchoLabyrinth_SFML>
            $<TARGET_FILE_DIR:EchoLabyrinth_SFML>
        COMMAND_EXPAND_LISTS
    )
endif()
