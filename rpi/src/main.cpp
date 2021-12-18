#include <iostream>
#include <stdio.h>
#include <string.h>

///RPI
#include "rpi/gui.h"	/// has SDL2 includes
#include "rpi/host.h"
#include "rpi/io.h"

///#define GL_GLEXT_PROTOTYPES 1

/// for discovery service
#define PING_MS		500
#define HOSTS_MAX	4		// was 16
#define DROP_PINGS	3

#define WinWidth 1920
#define WinHeight 1080

// TO-DOs:
// Hook up settings with session stream.
// x11-openGL. PS5. Basic UI layout + widgets.
// aspect ratio.
// Publish!?
// multiple hosts. Nicer GUI. Find Audio devices.
// Touchpad. Rumble. Motion Control.

///#include "gperftools/profiler.h"
/// export CPUPROFILE_FREQUENCY=50
/// env CPUPROFILE=out.prof rpi/chiaki-rpi
/// > google-pprof --text rpi/chiaki-rpi out.prof

/// grep -r -i "eglCreateImageKHR" *

int main()
{
	int ret;
	
	//ChiakiLog log;
	//chiaki_log_init(&log, 4, NULL, NULL);  // 1 is the log level, 4 also good, try 14
	//chiaki_log_init(&log, CHIAKI_LOG_ALL & ~CHIAKI_LOG_VERBOSE, NULL, NULL);
	
	SDL_version linked;
	SDL_GetVersion(&linked);
	printf("SDL version linked: %d.%d.%d\n", linked.major, linked.minor, linked.patch);
	
	/// SDL Setup
	///
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);//2,3
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);//0,1
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	//SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);//try 0
	//SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);// try 0
	
	SDL_Init(SDL_INIT_VIDEO);
	///SDL_VideoInit(NULL);
	///SDL_InitSubSystem(SDL_INIT_VIDEO);
	SDL_Window *sdl_window;

    /// Create an application window with the following settings:
	sdl_window = SDL_CreateWindow(
		"Chiaki",         			///    const char* title
		SDL_WINDOWPOS_UNDEFINED,  	///    int x: initial x position
		SDL_WINDOWPOS_UNDEFINED,  	///    int y: initial y position
		WinWidth,                   ///    int w: width, in pixels
		WinHeight,                  ///    int h: height, in pixels
		///SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN  | SDL_WINDOW_ALLOW_HIGHDPI        //    Uint32 flags: window options, see docs
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
	);
		
	/// Check that the window was successfully made
    if(sdl_window==NULL){
      std::cout << "Could not create window: " << SDL_GetError() << '\n';
      SDL_Quit();
      return 1;
    }
    

	
	SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1"); /// Seems to work!
	///SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR
	SDL_SetHint(SDL_HINT_VIDEO_X11_FORCE_EGL, "1"); // NEW Testing, 0=glx, "By default SDL will use GLX when both are present."
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2"); /// Seems to work!
	///SDL_HINT_VIDEO_DOUBLE_BUFFER
	///SDL_HINT_KMSDRM_REQUIRE_DRM_MASTER
	///SDL_HINT_AUTO_UPDATE_JOYSTICKS
	
	SDL_GLContext ctx = SDL_GL_CreateContext(sdl_window);
	SDL_GL_MakeCurrent(sdl_window, ctx);
	
	//~ SDL_Log("Available renderers:\n");
	//~ SDL_Log("\n");
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
    
    
	printf("SDL_VIDEODRIVER selected: %s\n",  SDL_GetCurrentVideoDriver());

	SDL_Renderer* sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED);
	//SDL_Renderer* sdl_renderer = SDL_CreateRenderer(sdl_window, renderer_index, SDL_RENDERER_ACCELERATED);
	if(sdl_renderer==NULL){
		/// In the event that the window could not be made...
		printf("Could not create renderer: %s\n", SDL_GetError() );
		SDL_Quit();
		return 1;
	}
	SDL_GL_SetSwapInterval(1); /// Enable vsync
	
	SDL_RendererInfo info;
	SDL_GetRendererInfo( sdl_renderer, &info );
	printf("SDL_RENDER_DRIVER selected: %s\n", info.name);
	SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_BLEND);
	
	//EGLDisplay egl_display = eglGetCurrentDisplay();
	
	//~ SDL_SysWMinfo WMinfo;
	//~ SDL_VERSION(&WMinfo.version);
	//~ SDL_GetWindowWMInfo(sdl_window, &WMinfo);
	
	//~ EGLNativeDisplayType hwnd = WMinfo.info.x11.display;  /// WMinfo.info.kmsdrm.drm_fd, WMinfo.info.x11.display
	//~ EGLDisplay egl_display = eglGetDisplay(hwnd); // Might also work
	
	//~ works'ish EGLDisplay egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	//~ printf("EGLDisplay: %d\n", egl_display);
	//~ if(egl_display == EGL_NO_DISPLAY){
		//~ printf("No EGL Display found\n");
		//~ return -1;
	//~ }

	/// GUI start
	NanoSdlWindow *screen = new NanoSdlWindow(sdl_window, WinWidth, WinHeight, sdl_renderer);
	screen->start();
	//while(1)SDL_Delay(1000);
	
	
	/// Finish
	printf("END rpidrm\n");
	return 0;
}
