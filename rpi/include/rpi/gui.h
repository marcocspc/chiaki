#ifndef RPI_GUI_H
#define RPI_GUI_H

#include <SDL2/SDL.h>
///#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengles2.h>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

// version error
// /home/pi/dev/chiaki_v4l2/third-party/imgui/imgui_internal.h:267:25: error: missing binary operator before token "defined"
//  #elif defined(__GNUC__) defined(__arm__) && !defined(__thumb__)
// should be
//  #elif defined(__GNUC__) && defined(__arm__) && !defined(__thumb__)

#include <string>
#include <stdio.h>

#include <libavcodec/avcodec.h> ///AVFrame

#include <chiaki/log.h>
#include "chiaki/regist.h"
#include <chiaki/controller.h>

#include "rpi/settings.h"
#include "rpi/glhelp.h"

using std::cout;
using std::cerr;
using std::endl;

/// ----------------------------------------------------------------------------------------
//#include <SDL_syswm.h> /// Below section is instead of this
#ifdef __cplusplus
extern "C" {
#endif

#include "SDL_stdinc.h"
#include "SDL_error.h"
#include "SDL_video.h"
#include "SDL_version.h"
typedef enum
{
	SDL_SYSWM_UNKNOWN,
	SDL_SYSWM_WINDOWS,
	SDL_SYSWM_X11,
	SDL_SYSWM_DIRECTFB,
	SDL_SYSWM_COCOA,
	SDL_SYSWM_UIKIT,
	SDL_SYSWM_WAYLAND,
	SDL_SYSWM_MIR,  /* no longer available, left for API/ABI compatibility. Remove in 2.1! */
	SDL_SYSWM_WINRT,
	SDL_SYSWM_ANDROID,
	SDL_SYSWM_VIVANTE,
	SDL_SYSWM_OS2,
	SDL_SYSWM_HAIKU,
	SDL_SYSWM_KMSDRM
} SDL_SYSWM_TYPE;

struct SDL_SysWMinfo
{
	SDL_version version;
	SDL_SYSWM_TYPE subsystem;
	union
	{
		
//#if defined(SDL_VIDEO_DRIVER_X11)
        struct
        {
            Display *display;           /**< The X11 display */
            Window window;              /**< The X11 window */
        } x11;
//#endif
		
//#if defined(SDL_VIDEO_DRIVER_KMSDRM)
		struct
		{
			int dev_index;               /**< Device index (ex: the X in /dev/dri/cardX) */
			int drm_fd;                  /**< DRM FD (unavailable on Vulkan windows) */
			struct gbm_device *gbm_dev;  /**< GBM device (unavailable on Vulkan windows) */
		} kmsdrm;
//#endif

		/* Make sure this union is always 64 bytes (8 64-bit pointers). */
		/* Be careful not to overflow this if you add a new target! */
		Uint8 dummy[64];
	} info;
};
typedef struct SDL_SysWMinfo SDL_SysWMinfo;

extern DECLSPEC SDL_bool SDLCALL SDL_GetWindowWMInfo(SDL_Window * window,
													 SDL_SysWMinfo * info);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

///-------------------------------------------------------------------------

class Host;
class IO;

class NanoSdlWindow
{
	public:
		NanoSdlWindow(SDL_Window* pwindow, int rwidth, int rheight, SDL_Renderer* sdl_renderer);
		~NanoSdlWindow();
		Host *host = nullptr; 	/// controller state, session
		IO *io = nullptr;	    /// input, decode, rendering, audio
		
		bool start();
		int hostClick();
		void registClick(std::string acc_id, std::string pin);
		//bool keyboardEvent(int key, int scancode, int action, int modifiers);
		//virtual void draw(SDL_Renderer* renderer);
		//virtual void drawContents();
		virtual bool resizeEvent(int w, int h);
		void mouseClickEvent();
		void PressSelect();
		void setClientState(std::string client_state);
		void restoreGui();
		void RefreshScreenDraw();
		
	
		/// Imgui things
		SDL_GLContext gl_context;
		void SettingsDraw(const char* label, std::vector<std::string> list, std::string &select);
		
		std::vector<std::string> decoder_options;
		std::string sel_decoder;
		std::vector<std::string> vcodec_options;
		std::string sel_vcodec;
		std::vector<std::string> resolution_options;
		std::string sel_resolution;
		std::vector<std::string> framerate_options;
		std::string sel_framerate;
		std::vector<std::string> audio_options;
		std::string sel_audio;
		
		void UpdateSettingsGui();
		void ChangeSettingAction(std::string choice);
		
		void HandleSDLEvents();
		ImVec4 clear_color;
		bool open_regist = false;
		std::string regist_acc_id;
		std::string regist_pin;
		
		
		SDL_Window *sdl_window; 
		SDL_Renderer* sdl_renderer;
		bool IsX11 = false;
		bool takeInput=1;
		RpiSettings *settings = nullptr;
		
		/// v4l2-gl/ffmpeg things
		GLuint program;
		GLuint texture;
		GLuint new_texture;
		bool hasTexInit=false;
		
		int InitVideoGl();
		bool UpdateAVFrame(AVFrame *frame);
		bool UpdateFromGUI(AVFrame *frame);  // TEST ONLY
		
		

	private:
		ChiakiLog log;
		int running=1;		/// main gui loop
		int fullscreen=0;
		std::string client_state = "unknown";	/// unknown, notreg, standby, waiting, ready, playing. For the button.
};


#endif
