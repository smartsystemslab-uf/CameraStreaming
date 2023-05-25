cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)
#set(PROJECT_NAME ImmersionViewer)
#project( ${PROJECT_NAME} )
find_package( OpenCV REQUIRED )
include_directories(${OpenCV_INCLUDE_DIRS})

# CMakeLists.txt
find_package(nlohmann_json 3.2.0 REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)
set(target_dir "bin")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -D__LIBAVRESAMPLE__ -std=c++11")
#SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-std=c++11 -pthread")

find_package(OpenMP)
if (NOT OPENMP_FOUND)
        message(WARNING "OpenMP is not found. Will be slow.")
else()
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        message("OpenMP is found. Version: ${OpenMP_VERSION}.")
        message("OpenMP is found. Spec date: ${OpenMP_CXX_SPEC_DATE}.")
        message("OpenMP is found. Libs: ${OpenMP_CXX_LIBRARIES}.")
endif()


find_package(JPEG)
if(JPEG_FOUND)
        include_directories(${JPEG_INCLUDE_DIR})
endif()

add_executable(${PROJECT_NAME}
    main.cpp
    queue.h
    config.h
    showmayimages.h
)

include_directories(".")
find_package(FFmpeg REQUIRED)

if (FFMPEG_FOUND)
  message("Found FFMPEG/LibAV libraries")
  include_directories(${FFMPEG_INCLUDE_DIR})
  target_link_libraries (${PROJECT_NAME} ${FFMPEG_LIBRARIES})
else (FFMPEG_FOUND)
  message(ERROR "Can't find libavcodec, libavformat or libavutil. Add them!")
endif (FFMPEG_FOUND)

target_link_libraries (${PROJECT_NAME} "-lm")
target_link_libraries(${PROJECT_NAME} stdc++fs)

find_package (Threads)

include_directories(${PROJ_INCLUDE_DIR})
target_link_libraries( ${PROJECT_NAME} ${OpenCV_LIBS} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)


