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


// TO-DOs:
// Rumble. Motion Control.
// multiple hosts.

/// #include "gperftools/profiler.h"
/// export CPUPROFILE_FREQUENCY=50
/// env CPUPROFILE=out.prof rpi/chiaki-rpi
/// > google-pprof --text rpi/chiaki-rpi out.prof


int main()
{
	///ChiakiLog log;
	///chiaki_log_init(&log, 4, NULL, NULL);  // 1 is the log level, 4 also good, try 14
	///chiaki_log_init(&log, CHIAKI_LOG_ALL & ~CHIAKI_LOG_VERBOSE, NULL, NULL);
	
	int bufsize = 256;
	char pathbuf[bufsize];
	readlink("/proc/self/exe", pathbuf, bufsize);
	printf ("Linux base Path:  %s\n", pathbuf);
	
	/// Test for existing h265/hevc decoder device
	FILE *f;
	f = fopen("/dev/video19", "r");
	if(f != NULL) {
		fclose(f);
	} else {
		printf("WARNING:  h265 Decoder Not Detected\n");
	}
	
	SDL_version linked;
	SDL_GetVersion(&linked);
	printf("SDL version linked: %d.%d.%d\n", linked.major, linked.minor, linked.patch);

	///SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0); // TEST
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);//2,3 use 2
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);//0,1 use 0
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);//was: 24, try 0
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);//was: 8, try 0
	///SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);//test
	
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	
	int screen_width = 1280;
	int screen_height = 720;
	
	bool IsX11 = true;
    if(std::strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0 || std::strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0)///0 is match
		 IsX11 = true;
	else IsX11 = false;
	
	/// Seems Mode 0 is the best one in my case.
	/// More here:  https://wiki.libsdl.org/CategoryVideo
	/// First one X11:   INFO: Mode 0	bpp 24	SDL_PIXELFORMAT_RGB888		1920 x 1200
	/// First one CLI:   INFO: Mode 0	bpp 32	SDL_PIXELFORMAT_ARGB8888	1920 x 1200
	SDL_DisplayMode sdl_top_mode;
	///if( SDL_GetDisplayMode(int displayIndex, int modeIndex, SDL_DisplayMode * mode) == 0)
	if( SDL_GetDisplayMode(0, 0, &sdl_top_mode) == 0)  /// success
	{	
		if(!IsX11) {
			screen_width = sdl_top_mode.w;
			screen_height = sdl_top_mode.h;
		}
		
		printf("Disp Mode: \t%dx%dpx @ %dhz \n", sdl_top_mode.w, sdl_top_mode.h, sdl_top_mode.refresh_rate);
	}
	
    /// Create an application window with the following settings:
    SDL_Window *sdl_window;
	sdl_window = SDL_CreateWindow(
		"Chiaki",         			///    const char* title
		SDL_WINDOWPOS_UNDEFINED,  	///    int x: initial x position
		SDL_WINDOWPOS_UNDEFINED,  	///    int y: initial y position
		screen_width,                   ///    int w: width, in pixels
		screen_height,                  ///    int h: height, in pixels
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	);

	/// Check that the window was successfully made
    if(sdl_window==NULL){
      std::cout << "SDL Could not create window: " << SDL_GetError() << '\n';
      SDL_Quit();
      return 1;
    }

    /// Affects fullscreen toggle under X11
    SDL_SetWindowDisplayMode(sdl_window, &sdl_top_mode);

    SDL_GLContext gl_context;    
	SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1"); /// Seems to work!
	SDL_SetHint(SDL_HINT_VIDEO_X11_FORCE_EGL, "1"); // 0=glx, "By default SDL will use GLX when both are present."
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2"); /// Seems to work and be enough!
	
	///SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR
	///SDL_HINT_VIDEO_DOUBLE_BUFFER
	///SDL_HINT_KMSDRM_REQUIRE_DRM_MASTER
	///SDL_HINT_AUTO_UPDATE_JOYSTICKS
	///test for micro stutter - no improvement, RaspiOS tearing issue?
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
	SDL_SetHint(SDL_HINT_VIDEO_DOUBLE_BUFFER, "1");
	
    SDL_GL_SetSwapInterval(1); /// Enable vsync, also in gui.cpp
    
	printf("SDL_VIDEO_DRIVER selected: %s\n",  SDL_GetCurrentVideoDriver());


	//~ SDL_Log("Available renderers:\n");
	//~ int renderer_index = 0;
	//~ for(int it = 0; it < SDL_GetNumRenderDrivers(); it++) {
		//~ SDL_RendererInfo info;
		//~ SDL_GetRenderDriverInfo(it,&info);

		//~ SDL_Log("%s\n", info.name);

		//~ if(strcmp("opengles2", info.name) == 0) {
			//~ printf("Got hold of GLES2, at %d\n", it);
			//~ renderer_index = it;
		//~ }
	//~ }
	
	/// Having this here makes work ok in X11...
	//~ if(IsX11) {
		//~ gl_context = SDL_GL_CreateContext(sdl_window);
		//~ SDL_GL_MakeCurrent(sdl_window, gl_context);
	//~ }

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
	SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_BLEND);
	
	
	/// ...but needs to be here for CLI
	if(!IsX11) {
		gl_context = SDL_GL_CreateContext(sdl_window);
		SDL_GL_MakeCurrent(sdl_window, gl_context);
	}
	
			
	/// GUI start
	ImguiSdlWindow *screen = new ImguiSdlWindow(pathbuf, sdl_window, screen_width, screen_height, sdl_renderer, &gl_context);
	screen->start();
	//while(1) sleep(0.1);
	
	/// Finish
	printf("END chiaki-rpi\n");
	return 0;
}
