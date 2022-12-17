cmake_minimum_required(VERSION 3.13)
project(kaaris-vs-booba C)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# source files
file(GLOB_RECURSE SRC "src/*.c" "src/*.h")

# keep folder tree for Visual Studio
if (MSVC)
    foreach (FILE ${SRC})
        # Get the directory of the source file
        get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)

        # Remove common directory prefix to make the group
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")

        # Make sure we are using windows slashes
        string(REPLACE "/" "\\" GROUP "${GROUP}")

        # Group into "Source Files" and "Header Files"
        if ("${FILE}" MATCHES ".*\\.c")
            set(GROUP "Source Files${GROUP}")
        elseif ("${FILE}" MATCHES ".*\\.h")
            set(GROUP "Header Files${GROUP}")
        endif ()

        source_group("${GROUP}" FILES "${FILE}")
    endforeach ()
endif ()

# Warning: put this before find_package as they use DEPS_DIR on Windows
set(DEPS_DIR ${PROJECT_SOURCE_DIR}/dep/win64-msvc)

if (${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(USE_FLAGS "-O3 -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='[\"png\", \"jpg\"]' -s USE_SDL_MIXER=2 -s SDL2_MIXER_FORMATS='[\"mp3\"]' -s USE_SDL_TTF=2 -s USE_SDL_GFX=2 -s USE_SDL_NET=2 -s ALLOW_MEMORY_GROWTH=1 -s ASYNCIFY --preload-file data -s WASM=1 --shell-file template.html --post-js postfix.js")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS}")
    set(CMAKE_EXECUTABLE_SUFFIX .html)
else ()
    # dependencies headers
    set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

    find_package(SDL2 REQUIRED)
    find_package(SDL2_IMAGE REQUIRED)
    find_package(SDL2_TTF REQUIRED)
    find_package(SDL2_MIXER REQUIRED)
    find_package(SDL2_GFX REQUIRED)
    find_package(SDL2_NET REQUIRED)
    find_package(FFmpeg REQUIRED)

    include_directories(SRC ${SDL2_INCLUDE_DIR} ${SDL2_TTF_INCLUDE_DIR} ${SDL2_IMAGE_INCLUDE_DIR} ${SDL2_MIXER_INCLUDE_DIR} ${SDL2_GFX_INCLUDE_DIR} ${SDL2_NET_INCLUDE_DIR} ${FFMPEG_INCLUDE_DIRS})

    # Warnings
    add_compile_options(/W3)
endif ()
    

# compiadd_executable(${PROJECT_NAME} WIN32 ${SRC} icon/res.rc)
add_executable(${PROJECT_NAME} WIN32 ${SRC} src/res.rc)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${DEPS_DIR}/bin $<TARGET_FILE_DIR:${PROJECT_NAME}>/)

# local includes
target_include_directories(${PROJECT_NAME} PRIVATE "src")

# dependencies libraries
target_link_libraries(${PROJECT_NAME} ${SDL2MAIN_LIBRARY} ${SDL2_LIBRARY} ${SDL2_TTF_LIBRARY} ${SDL2_IMAGE_LIBRARY} ${SDL2_MIXER_LIBRARY} ${SDL2_GFX_LIBRARY} ${SDL2_NET_LIBRARY} ${FFMPEG_LIBRARIES})

# Other
if (MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS /SUBSYSTEM:WINDOWS)
else ()
    target_link_libraries(${PROJECT_NAME} m)
endif ()

