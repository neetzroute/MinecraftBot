cmake_minimum_required(VERSION 3.15)
project(MinecraftBot)

set(CMAKE_CXX_STANDARD 26)

set(BOTCRAFT_GAME_VERSION "1.21.4" CACHE STRING "Minecraft game version" FORCE)
set(BOTCRAFT_BUILD_EXAMPLES OFF CACHE BOOL "Build Botcraft examples" FORCE)
set(BOTCRAFT_BUILD_DOC OFF CACHE BOOL "Build Botcraft documentation" FORCE)
set(BOTCRAFT_BUILD_TESTS OFF CACHE BOOL "Build Botcraft tests" FORCE)
set(BOTCRAFT_USE_OPENGL_GUI OFF CACHE BOOL "Build Botcraft OpenGL GUI" FORCE)
set(BOTCRAFT_USE_IMGUI OFF CACHE BOOL "Build Botcraft ImGui UI" FORCE)

include(FetchContent)

FetchContent_Declare(
        botcraft
        GIT_REPOSITORY https://github.com/adepierre/Botcraft.git
        GIT_TAG        master
        GIT_SHALLOW    TRUE
        # Для headless-клиента нужен только Asio. Без этого ограничения CMake
        # также скачивает необязательные зависимости GUI и тестов.
        GIT_SUBMODULES 3rdparty/asio
        GIT_SUBMODULES_RECURSE FALSE
        UPDATE_DISCONNECTED TRUE
)
FetchContent_MakeAvailable(botcraft)

add_executable(MinecraftBot
        src/main.cpp
        src/ChatListener.cpp
        include/ChatListener.h
        src/ResourcePackHandler.cpp
        include/ResourcePackHandler.h
)

target_include_directories(MinecraftBot PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${botcraft_SOURCE_DIR}/include
)

target_link_libraries(MinecraftBot PRIVATE
        botcraft
)
