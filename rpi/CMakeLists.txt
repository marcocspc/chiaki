
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W -Wall -Wextra -g -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -Wextra -g -O2")

#message("${CMAKE_SOURCE_DIR}")

find_package(SDL2 MODULE REQUIRED)

#include_directories("${FFMPEG_INCLUDE_DIRS}") # needed for correct AV_PIX_FMT_DRM_PRIME==181

include_directories(
	"${FFMPEG_INCLUDE_DIRS}" 			# needed for correct AV_PIX_FMT_DRM_PRIME==181
	"${CMAKE_SOURCE_DIR}/rpi/include/"
	"${CMAKE_SOURCE_DIR}/lib/include/"
	"${SDL2_INCLUDE_DIRS}"
	/usr/include/libdrm)

add_executable(chiaki-rpi
				src/drmprime_out.c
				src/io.cpp
				src/host.cpp
				src/main.cpp
				src/settings.cpp)

# ${AVCODEC_LIBRARY} ${AVUTIL_LIBRARY} ${AVFORMAT_LIBRARY} already in chiaki-lib
target_link_libraries(chiaki-rpi chiaki-lib SDL2::SDL2 -ldrm)
#find DependInfo.cmake  for clues!
