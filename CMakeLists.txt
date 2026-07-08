cmake_minimum_required(VERSION 3.15)
project(MineBot)

set(CMAKE_CXX_STANDARD 26)

# Поиск необходимых системных пакетов
find_package(CURL REQUIRED)

# Установка версии игры (1.21.4 использует протокол 769)
set(BOTCRAFT_GAME_VERSION "1.21.4" CACHE STRING "Minecraft game version" FORCE)
set(BOTCRAFT_BUILD_EXAMPLES OFF CACHE BOOL "Build Botcraft examples" FORCE)
set(BOTCRAFT_BUILD_DOC OFF CACHE BOOL "Build Botcraft documentation" FORCE)
set(BOTCRAFT_BUILD_TESTS OFF CACHE BOOL "Build Botcraft tests" FORCE)
set(BOTCRAFT_USE_OPENGL_GUI OFF CACHE BOOL "Build Botcraft OpenGL GUI" FORCE)
set(BOTCRAFT_USE_IMGUI OFF CACHE BOOL "Build Botcraft ImGui UI" FORCE)

include(FetchContent)

# Скачивание библиотеки BotCraft
FetchContent_Declare(
        botcraft
        GIT_REPOSITORY https://github.com/adepierre/Botcraft.git
        GIT_TAG        master
        GIT_SHALLOW    TRUE
        UPDATE_DISCONNECTED TRUE
)
FetchContent_MakeAvailable(botcraft)

# Определение исполняемого файла бота
add_executable(MineBot
        src/main.cpp
        src/ChatListener.cpp
        include/ChatListener.h
        src/ResourcePackHandler.cpp
        include/ResourcePackHandler.h
)

# Подключение заголовочных файлов
target_include_directories(MineBot PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${botcraft_SOURCE_DIR}/include
)

# Компоновка библиотек
target_link_libraries(MineBot PRIVATE
        CURL::libcurl
        botcraft
)
