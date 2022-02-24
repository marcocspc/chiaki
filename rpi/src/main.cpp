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
// aspect ratio for kmsdrm
// Publish!?
// multiple hosts.
// Rumble. Motion Control.

///#include "gperftools/profiler.h"
/// export CPUPROFILE_FREQUENCY=50
/// env CPUPROFILE=out.prof rpi/chiaki-rpi
/// > google-pprof --text rpi/chiaki-rpi out.prof


int main()
{
	//ChiakiLog log;
	//chiaki_log_init(&log, 4, NULL, NULL);  // 1 is the log level, 4 also good, try 14
	//chiaki_log_init(&log, CHIAKI_LOG_ALL & ~CHIAKI_LOG_VERBOSE, NULL, NULL);
	
	SDL_version linked;
	SDL_GetVersion(&linked);
	printf("SDL version linked: %d.%d.%d\n", linked.major, linked.minor, linked.patch);

	///SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0); // TEST
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);//2,3 use 2
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);//0,1 use 0
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);//try 0
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);// try 0
	///SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);//test
	
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    /// Create an application window with the following settings:
    SDL_Window *sdl_window;
	sdl_window = SDL_CreateWindow(
		"Chiaki",         			///    const char* title
		SDL_WINDOWPOS_UNDEFINED,  	///    int x: initial x position
		SDL_WINDOWPOS_UNDEFINED,  	///    int y: initial y position
		WinWidth,                   ///    int w: width, in pixels
		WinHeight,                  ///    int h: height, in pixels
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	);
	
	/// Check that the window was successfully made
    if(sdl_window==NULL){
      std::cout << "SDL Could not create window: " << SDL_GetError() << '\n';
      SDL_Quit();
      return 1;
    }
    
	SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1"); /// Seems to work!
	SDL_SetHint(SDL_HINT_VIDEO_X11_FORCE_EGL, "1"); // NEW Testing, 0=glx, "By default SDL will use GLX when both are present."
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2"); /// Seems to work and be enough!
	
	///SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR
	///SDL_HINT_VIDEO_DOUBLE_BUFFER
	///SDL_HINT_KMSDRM_REQUIRE_DRM_MASTER
	///SDL_HINT_AUTO_UPDATE_JOYSTICKS
	///test for micro stutter - no improvement, RaspiOS tearing issue?
	///SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	///SDL_SetHint(SDL_HINT_VIDEO_DOUBLE_BUFFER, "1");
	
	SDL_GLContext gl_context = SDL_GL_CreateContext(sdl_window);
    SDL_GL_MakeCurrent(sdl_window, gl_context);
    SDL_GL_SetSwapInterval(1); /// Enable vsync, also in gui.cpp
    
	printf("SDL_VIDEO_DRIVER selected: %s\n",  SDL_GetCurrentVideoDriver());

	SDL_Renderer* sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
	if(sdl_renderer==NULL){
		/// In the event that the window could not be made...
		printf("Could not create renderer: %s\n", SDL_GetError() );
		SDL_Quit();
		return 1;
	}
	
	SDL_RendererInfo info;
	SDL_GetRendererInfo( sdl_renderer, &info );
	printf("SDL_RENDER_DRIVER selected: %s\n", info.name);
	///SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_BLEND);
			
	/// GUI start
	ImguiSdlWindow *screen = new ImguiSdlWindow(sdl_window, WinWidth, WinHeight, sdl_renderer, &gl_context);
	screen->start();
	
	/// Finish
	printf("END chiaki-rpi\n");
	return 0;
}
