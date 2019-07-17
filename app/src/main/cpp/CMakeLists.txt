cmake_minimum_required(VERSION 3.4.1)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions -Wall")


if (${ANDROID_PLATFORM_LEVEL} LESS 11)
  message(FATAL_ERROR "OpenGL 2 is not supported before API level 11 (currently using ${ANDROID_PLATFORM_LEVEL}).")
  return()
elseif (${ANDROID_PLATFORM_LEVEL} LESS 18)
  add_definitions("-DDYNAMIC_ES3")
  set(OPENGL_LIB GLESv2)
else ()
  set(OPENGL_LIB GLESv3)
endif (${ANDROID_PLATFORM_LEVEL} LESS 11)

include_directories("${CMAKE_SOURCE_DIR}/engine/include" "${CMAKE_SOURCE_DIR}/engine/glm" "${CMAKE_SOURCE_DIR}/engine/lua" "${CMAKE_SOURCE_DIR}/android_app_glue" "${CMAKE_SOURCE_DIR}/engine/platforms/android" "${CMAKE_SOURCE_DIR}/engine/platforms")

file(GLOB_RECURSE nyan_SRC
    "*.h"
    "*.c"
    "*.inc"
    "*.inl"
    "*.hpp"
    "*.cpp"
)

add_library(coconuteng SHARED ${nyan_SRC})

add_definitions(-DGL_V_3
                    -DUSE_ANDROID
                    -DANDROID_NDK
                    -DFT2_BUILD_LIBRARY
                    -DDEBUG_SHADERS)

target_link_libraries(coconuteng
                      ${OPENGL_LIB}
                      android
                      EGL
                      log
                      dl
                      jnigraphics
                      m)


