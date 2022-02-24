
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W -Wall -Wextra -g") #-pg added for gprof, sends breaking signals.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -Wextra -g -DIMGUI_IMPL_OPENGL_ES2")

#CXXFLAGS += -DIMGUI_IMPL_OPENGL_ES2
#LINUX_GL_LIBS = -lGLESv2

message("${CMAKE_SOURCE_DIR}")

#/home/pi/dev/chiaki_v4l2/third-party

#find_package(SDL2 MODULE REQUIRED)
find_package(SDL2 2.0.17 REQUIRED)
find_package(SDL2IMAGE REQUIRED)
find_package(SDL2TTF REQUIRED)
include_directories(${SDL2TTF_INCLUDE_DIR})
#include_directories(${SDL2_IMAGE_INCLUDE_DIR})

# ./usr/lib/arm-linux-gnueabihf/libyaml-0.so.2
# apt install libyaml-dev

#include_directories("${FFMPEG_INCLUDE_DIRS}") # needed for correct AV_PIX_FMT_DRM_PRIME==181

include_directories(
	"${FFMPEG_INCLUDE_DIRS}" 			# needed for correct AV_PIX_FMT_DRM_PRIME==181
	"${CMAKE_SOURCE_DIR}/rpi/include/"
	"${CMAKE_SOURCE_DIR}/lib/include/"
     "${CMAKE_SOURCE_DIR}/third-party/imgui/"
     "${CMAKE_SOURCE_DIR}/third-party/imgui/backends/"
     "${CMAKE_SOURCE_DIR}/setsu/include/"
	"${SDL2_INCLUDE_DIRS}"
	/usr/include/libdrm
     /usr/include)
     
     
# Imgui source files
list(APPEND IMGUI_SOURCE_FILES
     ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui.cpp
     ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui_draw.cpp
     ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui_tables.cpp
     ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui_widgets.cpp
     ${CMAKE_SOURCE_DIR}/third-party/imgui/backends/imgui_impl_sdl.cpp
     ${CMAKE_SOURCE_DIR}/third-party/imgui/backends/imgui_impl_opengl3.cpp
     # ${CMAKE_SOURCE_DIR}/third-party/imgui/imgui_demo.cpp
     )

add_executable(chiaki-rpi
				src/drmprime_out.c
				src/io.cpp
				src/host.cpp
				src/main.cpp
				src/settings.cpp
                    src/gui.cpp
                    ${IMGUI_SOURCE_FILES})
                    
# ${AVCODEC_LIBRARY} ${AVUTIL_LIBRARY} ${AVFORMAT_LIBRARY} already in chiaki-lib
# -lEGS makes it build, but is that the right one or can SDL provide?
target_link_libraries(chiaki-rpi chiaki-lib ${SDL2_LIBRARIES} ${SDL2TTF_LIBRARY} ${SDL2IMAGE_LIBRARY} setsu -ldrm -lGLESv2 -lEGL)
#find DependInfo.cmake  for clues!
