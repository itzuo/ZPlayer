
cmake_minimum_required(VERSION 3.4.1)

set(FFMPEG ${CMAKE_SOURCE_DIR}/ffmpeg) # ffmpeg的路径
set(RTMP ${CMAKE_SOURCE_DIR}/rtmp) # rtmp的路径

include_directories(${FFMPEG}/include) # 导入ffmpeg的头文件

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -L${FFMPEG}/libs/${CMAKE_ANDROID_ARCH_ABI}") # ffmpeg库指定

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -L${RTMP}/libs/${CMAKE_ANDROID_ARCH_ABI}") # rtmp库指定

file(GLOB source_file *.cpp)


add_library(
        z-player

        SHARED

        ${source_file})

message("CMAKE_ANDROID_ARCH_ABI====${CMAKE_ANDROID_ARCH_ABI}")

#include_directories(${CMAKE_SOURCE_DIR}/include)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/libs/${CMAKE_ANDROID_ARCH_ABI}")
#set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/libs/${ANDROID_API}")

find_library(
        log-lib

        log)

target_link_libraries(
        z-player
        # 方法一：使用-Wl 忽略顺序
        #        -Wl,--start-group #忽略顺序引发的错误
        #        avcodec avfilter avformat avutil swresample swscale
        #        -Wl,--end-group
        # 方法二：调整顺序
        avformat avcodec avfilter avutil swresample swscale #必须要把avformat放在avcodec的前面
        ${log-lib}
        rtmp
        z
        OpenSLES
        android)