
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W -Wall -Wextra -g") #-pg added for gprof, sends breaking signals.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -Wextra -g")

#message("${CMAKE_SOURCE_DIR}")

#find_package(SDL2 MODULE REQUIRED)
find_package(SDL2 2.0.17 REQUIRED)
#find_package(SDL2IMAGE REQUIRED)
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
	"${SDL2_INCLUDE_DIRS}"
	/usr/include/libdrm
     /usr/include
	/home/pi/dev/nanogui-sdl/
	/home/pi/dev/nanogui-sdl/sdlgui)
	
	
# Nano Gui source files
list(APPEND NNGUI_BASIC_SOURCE
     /home/pi/dev/nanogui-sdl/sdlgui/button.h
     /home/pi/dev/nanogui-sdl/sdlgui/checkbox.h
     /home/pi/dev/nanogui-sdl/sdlgui/colorpicker.h
     /home/pi/dev/nanogui-sdl/sdlgui/colorwheel.h
     /home/pi/dev/nanogui-sdl/sdlgui/combobox.h
     /home/pi/dev/nanogui-sdl/sdlgui/common.h
     /home/pi/dev/nanogui-sdl/sdlgui/graph.h
     /home/pi/dev/nanogui-sdl/sdlgui/imagepanel.h
     /home/pi/dev/nanogui-sdl/sdlgui/imageview.h
     /home/pi/dev/nanogui-sdl/sdlgui/label.h
     /home/pi/dev/nanogui-sdl/sdlgui/layout.h
     /home/pi/dev/nanogui-sdl/sdlgui/messagedialog.h
     /home/pi/dev/nanogui-sdl/sdlgui/popup.h
     /home/pi/dev/nanogui-sdl/sdlgui/popupbutton.h
     /home/pi/dev/nanogui-sdl/sdlgui/progressbar.h
     /home/pi/dev/nanogui-sdl/sdlgui/screen.h
     /home/pi/dev/nanogui-sdl/sdlgui/slider.h
     /home/pi/dev/nanogui-sdl/sdlgui/stackedwidget.h
     /home/pi/dev/nanogui-sdl/sdlgui/tabheader.h
     /home/pi/dev/nanogui-sdl/sdlgui/tabwidget.h
     /home/pi/dev/nanogui-sdl/sdlgui/textbox.h
     /home/pi/dev/nanogui-sdl/sdlgui/theme.h
     /home/pi/dev/nanogui-sdl/sdlgui/vscrollpanel.h
     /home/pi/dev/nanogui-sdl/sdlgui/switchbox.h
     /home/pi/dev/nanogui-sdl/sdlgui/dropdownbox.h
     /home/pi/dev/nanogui-sdl/sdlgui/widget.h
     /home/pi/dev/nanogui-sdl/sdlgui/window.h
     /home/pi/dev/nanogui-sdl/sdlgui/nanovg.h
     /home/pi/dev/nanogui-sdl/sdlgui/nanovg_rt.h
     
     /home/pi/dev/nanogui-sdl/sdlgui/button.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/checkbox.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/colorpicker.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/colorwheel.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/combobox.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/common.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/graph.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/imagepanel.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/imageview.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/label.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/layout.cpp
     #/home/pi/dev/nanogui-sdl/sdlgui/loadimages.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/messagedialog.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/resources.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/popup.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/popupbutton.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/progressbar.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/screen.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/slider.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/stackedwidget.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/tabheader.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/tabwidget.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/textbox.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/switchbox.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/dropdownbox.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/theme.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/vscrollpanel.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/widget.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/window.cpp
     /home/pi/dev/nanogui-sdl/sdlgui/nanovg.c
)
	
	

add_executable(chiaki-rpi
				src/drmprime_out.c
				src/io.cpp
				src/host.cpp
				src/main.cpp
				src/settings.cpp
                    src/gui.cpp
				${NNGUI_BASIC_SOURCE})

# ${AVCODEC_LIBRARY} ${AVUTIL_LIBRARY} ${AVFORMAT_LIBRARY} already in chiaki-lib
# 
target_link_libraries(chiaki-rpi chiaki-lib ${SDL2_LIBRARIES} ${SDL2TTF_LIBRARY} -ldrm -lGLESv2 )
#find DependInfo.cmake  for clues!
