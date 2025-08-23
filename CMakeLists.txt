cmake_minimum_required(VERSION 3.31.6)
project(airbud C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/include/SDL3-3.2.18/cmake")

find_package(SDL3 REQUIRED CONFIG)

add_executable(airbud src/main.c
        src/read_file.c
        src/init.c
        src/frame_queue.c
        src/decode.c
        src/decode.h
        src/render.c
        src/render.h
        src/game_states.h
        src/game_states.c
        src/game_logic.c
        src/game_logic.h
)

target_include_directories(airbud PRIVATE
        "${CMAKE_SOURCE_DIR}/include/ffmpeg/include"
        "${CMAKE_SOURCE_DIR}/src"
)

# Don't use link_directories; specify full paths below instead!

target_link_libraries(airbud PRIVATE
        SDL3::SDL3
        "${CMAKE_SOURCE_DIR}/include/ffmpeg/lib/libavcodec.dll.a"
        "${CMAKE_SOURCE_DIR}/include/ffmpeg/lib/libavformat.dll.a"
        "${CMAKE_SOURCE_DIR}/include/ffmpeg/lib/libavutil.dll.a"
        "${CMAKE_SOURCE_DIR}/include/ffmpeg/lib/libswresample.dll.a"
)

add_custom_command(TARGET airbud POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/include/SDL3-3.2.18/x86_64-w64-mingw32/bin/SDL3.dll"
        "$<TARGET_FILE_DIR:airbud>"
)

set(FFMPEG_DLLS
        avdevice-61.dll
        postproc-58.dll
        swresample-5.dll
        avcodec-61.dll
        avformat-61.dll
        avutil-59.dll
        swscale-8.dll
)

foreach(_dll IN LISTS FFMPEG_DLLS)
    add_custom_command(TARGET airbud POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/include/ffmpeg/bin/${_dll}"
            "$<TARGET_FILE_DIR:airbud>"
    )
endforeach()