cmake_minimum_required(VERSION 3.16)
project(EchoLabyrinth_SFML VERSION 5.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for IDEs (VS Code, CLion, Visual Studio)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ECHO_AUTO_FETCH_SFML "Automatically download and build SFML 2.6.1 if not installed on the system" ON)
option(ECHO_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer (GCC/Clang)" OFF)

# 1. Try to locate a pre-installed SFML 2.5+ package first
find_package(SFML 2.5 COMPONENTS graphics window audio system QUIET)

# 2. If SFML is not found locally (e.g. fresh Windows machine), automatically fetch SFML 2.6.1 from GitHub
if(NOT SFML_FOUND)
    if(ECHO_AUTO_FETCH_SFML)
        message(STATUS "SFML not found locally. Fetching SFML 2.6.1 automatically via CMake FetchContent...")
        include(FetchContent)
        set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared SFML libraries" FORCE)
        FetchContent_Declare(
            SFML
            GIT_REPOSITORY https://github.com/SFML/SFML.git
            GIT_TAG 2.6.1
            GIT_SHALLOW ON
        )
        FetchContent_MakeAvailable(SFML)
    else()
        message(FATAL_ERROR "SFML 2.5+ was not found and ECHO_AUTO_FETCH_SFML is OFF.")
    endif()
endif()

set(ECHO_HEADERS
    include/Config.hpp
    include/AcousticSystem.hpp
    include/AssetManager.hpp
    include/AudioManager.hpp
    include/PostProcessor.hpp
    include/UiRenderer.hpp
    include/SaveSystem.hpp
    include/Labyrinth.hpp
    include/Player.hpp
    include/EnemyAI.hpp
    include/Game.hpp
)

set(ECHO_SOURCES
    src/main.cpp
    src/AcousticSystem.cpp
    src/AssetManager.cpp
    src/AudioManager.cpp
    src/SoundtrackComposer.cpp
    src/PostProcessor.cpp
    src/UiRenderer.cpp
    src/SaveSystem.cpp
    src/LabyrinthGen.cpp
    src/LabyrinthRooms.cpp
    src/Relics.cpp
    src/Codex.cpp
    src/Player.cpp
    src/EnemyAI.cpp
    src/EnemyVariants.cpp
    src/BossAI.cpp
    src/Game.cpp
    src/GameRun.cpp
    src/GameNoise.cpp
    src/GameInput.cpp
    src/GameWorldRender.cpp
    src/GameHudRender.cpp
    src/GameBalance.cpp
    src/GameSelfTest.cpp
)

if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/echolabyrinth.rc")
    list(APPEND ECHO_SOURCES resources/echolabyrinth.rc)
endif()

add_executable(EchoLabyrinth_SFML ${ECHO_SOURCES} ${ECHO_HEADERS})

target_include_directories(EchoLabyrinth_SFML PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(EchoLabyrinth_SFML PRIVATE /W4 /permissive- /utf-8 /EHsc)
    target_compile_definitions(EchoLabyrinth_SFML PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
else()
    target_compile_options(EchoLabyrinth_SFML PRIVATE -Wall -Wextra -Wpedantic)
    if(ECHO_ENABLE_SANITIZERS)
        target_compile_options(EchoLabyrinth_SFML PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -g)
        target_link_options(EchoLabyrinth_SFML PRIVATE -fsanitize=address,undefined)
    endif()
endif()

target_link_libraries(EchoLabyrinth_SFML PRIVATE
    sfml-graphics
    sfml-window
    sfml-audio
    sfml-system
)

# Copy 3D Wavefront OBJ models, GLSL shaders, icon, and ASSET_MANIFEST.md next to the built executable
add_custom_command(TARGET EchoLabyrinth_SFML POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:EchoLabyrinth_SFML>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:EchoLabyrinth_SFML>/assets"
    COMMENT "Copying EchoLabyrinth 3D models, GLSL shaders & asset manifest to output directory..."
)

# On Windows, copy runtime DLLs (SFML + OpenAL32) next to the .exe so it runs standalone
if(WIN32)
    add_custom_command(TARGET EchoLabyrinth_SFML POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:EchoLabyrinth_SFML>
            $<TARGET_FILE_DIR:EchoLabyrinth_SFML>
        COMMAND_EXPAND_LISTS
    )

    if(DEFINED sfml_SOURCE_DIR)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(OPENAL_DLL_PATH "${sfml_SOURCE_DIR}/extlibs/bin/x64/openal32.dll")
        else()
            set(OPENAL_DLL_PATH "${sfml_SOURCE_DIR}/extlibs/bin/x86/openal32.dll")
        endif()
        if(EXISTS "${OPENAL_DLL_PATH}")
            add_custom_command(TARGET EchoLabyrinth_SFML POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${OPENAL_DLL_PATH}"
                    "$<TARGET_FILE_DIR:EchoLabyrinth_SFML>"
            )
            install(FILES "${OPENAL_DLL_PATH}" DESTINATION .)
        endif()
    endif()
endif()

# Stage 4: CPack Portable Release & Installer Packaging (ZIP / NSIS / TGZ)
install(TARGETS EchoLabyrinth_SFML RUNTIME DESTINATION .)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION .)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    DESTINATION .
)

set(CPACK_PACKAGE_NAME "EchoLabyrinth")
set(CPACK_PACKAGE_VENDOR "Subterranean Survey Corps")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "EchoLabyrinth 5.4 'Resonance' — Acoustic Roguelike Horror (C++17 / SFML)")
set(CPACK_PACKAGE_VERSION_MAJOR 5)
set(CPACK_PACKAGE_VERSION_MINOR 4)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
else()
    set(CPACK_GENERATOR "ZIP;TGZ")
endif()
include(CPack)

