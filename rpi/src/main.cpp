#include <iostream>
#include <stdio.h>
#include <string.h>

///RPI
#include "rpi/gui.h"	/// has SDL2 includes
#include "rpi/host.h"
#include "rpi/io.h"

/// for discovery service
#define PING_MS		500
#define HOSTS_MAX	4		// was 16
#define DROP_PINGS	3

#define WinWidth 1920
#define WinHeight 1080

// TO-DOs:
// Gui for registration.
// Figure out Settings store/load. Yaml?
// PS5.  Publish!?
// multiple hosts. drm stream render aspect ratio.
// 
// See serveritemwidget.cpp for ident Unregistered

/// export CPUPROFILE_FREQUENCY=50
/// env CPUPROFILE=out.prof rpi/chiaki-rpi
/// > google-pprof --text rpi/chiaki-rpi out.prof


///#include "gperftools/profiler.h"

int main()
{
	int ret;
	
	//ChiakiLog log;
	//chiaki_log_init(&log, 4, NULL, NULL);  // 1 is the log level, 4 also good, try 14
	//chiaki_log_init(&log, CHIAKI_LOG_ALL & ~CHIAKI_LOG_VERBOSE, NULL, NULL);
	
	/// SDL Setup
	///
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	
	SDL_Init(SDL_INIT_VIDEO);
	//SDL_VideoInit(NULL);
	//SDL_InitSubSystem(SDL_INIT_VIDEO);
	SDL_Window *sdl_window;
	
	//u32 WindowFlags = SDL_WINDOW_OPENGL; /// added  | SDL_WINDOW_FULLSCREEN
    /// Create an application window with the following settings:
	sdl_window = SDL_CreateWindow(
		"Chiaki",         			///    const char* title
		SDL_WINDOWPOS_UNDEFINED,  	///    int x: initial x position
		SDL_WINDOWPOS_UNDEFINED,  	///    int y: initial y position
		WinWidth,                   ///    int w: width, in pixels
		WinHeight,                  ///    int h: height, in pixels
		//SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN  | SDL_WINDOW_ALLOW_HIGHDPI        //    Uint32 flags: window options, see docs
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
	);
	
	/// Check that the window was successfully made
    if(sdl_window==NULL){
      std::cout << "Could not create window: " << SDL_GetError() << '\n';
      SDL_Quit();
      return 1;
    }
    
	printf("SDL_VIDEODRIVER selected: %s\n",  SDL_GetCurrentVideoDriver());

	SDL_Renderer* sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);
	if(sdl_renderer==NULL){
		// In the event that the window could not be made...
		printf("Could not create renderer: %s\n", SDL_GetError() );
		SDL_Quit();
		return 1;
	}
	SDL_RendererInfo info;
	SDL_GetRendererInfo( sdl_renderer, &info );
	printf("SDL_RENDER_DRIVER selected: %s\n", info.name);
	SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_BLEND);
	
	//ProfilerStart();
	/// GUI start
	NanoSdlWindow *screen = new NanoSdlWindow(sdl_window, WinWidth, WinHeight, sdl_renderer);
	screen->start();
	//while(1)SDL_Delay(1000);
	
	/// Finish
	//ProfilerStop();

	printf("END rpidrm\n");
	return 0;
}
