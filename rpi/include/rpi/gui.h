#ifndef RPI_GUI_H
#define RPI_GUI_H

#include <SDL2/SDL.h>
///#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengles2.h>
#include <SDL2/SDL_image.h>

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

class ImguiSdlWindow
{
	public:
		ImguiSdlWindow(char pathbuf[], SDL_Window* pwindow, int rwidth, int rheight, SDL_Renderer* sdl_renderer, SDL_GLContext* gl_context);
		~ImguiSdlWindow();
		Host *host = nullptr; 	/// controller state, session
		IO *io = nullptr;	    /// input, decode, rendering, audio
		RpiSettings *settings = nullptr;
		std::string home_dir;
		
		
		bool start();
		int hostClick();
		void registClick(std::string acc_id, std::string pin);
		virtual bool resizeEvent(int w, int h);
		void mouseClickEvent();
		void PressSelect();
		void setClientState(std::string client_state);
		void CreateImguiWidgets();
		void restoreGui();
		void RefreshScreenDraw();
		void UpdateSettingsGui();
		void ChangeSettingAction(int widgetID, std::string choice);
		void SwitchHelpText(const char* label);
		void HandleSDLEvents();
		void ToggleFullscreen();
		
		/// Imgui things
		void SettingsDraw(int widgetID, const char* label, std::vector<std::string> list, std::string &select);
		void SwitchHostImage(int which);
		
		std::vector<std::string>  remoteHostFiles;
		std::string sel_remote;
		
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
		
		ImVec4  clear_color = ImColor(14, 36, 43);
		ImColor button_col = ImColor(128, 128, 128);
		ImVec4  info_col = ImColor(5, 5, 5, 128);
		///ImColor neon_yell = ImColor(200, 255, 0, 255);
		ImFont* imgui_font;
		ImFont* regist_font;
		int help_text_n = 0;
		int psBtnSz = 256;
		int dspszX = 0;
		int dspszY = 0;
		int settingIndent = 0;
		
		bool open_regist = false;
		std::string regist_acc_id;
		std::string regist_pin;
		int remoteIp[4];
		
		SDL_GLContext *gl_ctx;
		SDL_Window *sdl_window = nullptr; 
		SDL_Renderer *sdl_renderer = nullptr;
		EGLDisplay egl_display;
		bool planes_init_done = false;
		EGLint attribs[50];
		bool IsX11 = false;
		bool takeInput=1;
		int clear_counter = 0;
		
		
		/// textures for gui
		GLuint gui_textures[16] = {(GLuint)0};
		GLuint current_host_texture = 0;
		GLuint bg_texture;
		GLuint logo_texture;
		int logo_width = 0;
		int logo_height = 0;
		
		/// v4l2-gl/ffmpeg things
		GLuint program;
		GLuint bg_program;
		GLuint texture;
		GLuint new_texture;
		bool hasTexInit=false;
		
		AVFrame *frame = nullptr;
		int InitVideoGl();
		bool UpdateAVFrame();
		bool stream_video_started = false;//remove?
		unsigned long int prevFrameCount=0;
		int glViewGeom[4];  /// x,y,w,h 
		
		

	private:
		ChiakiLog log;
		bool done = false;		/// A Major on/off state
		int guiActive=1;		/// main gui loop
		int fullscreen=0;
		std::string client_state = "unknown";	/// unknown, notreg, standby, waiting, ready, playing. For the button.
};


#endif
