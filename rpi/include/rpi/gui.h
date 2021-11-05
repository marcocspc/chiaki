#ifndef RPI_GUI_H
#define RPI_GUI_H

#include <SDL2/SDL.h>
//#include <SDL2/SDL_opengl.h>
//#include <SDL2/SDL_opengles2.h>


#include <sdlgui/screen.h>
#include <sdlgui/window.h>
#include <sdlgui/layout.h>
#include <sdlgui/label.h>
//#include <sdlgui/checkbox.h>
#include <sdlgui/button.h>
#include <sdlgui/toolbutton.h>
#include <sdlgui/popupbutton.h>
#include <sdlgui/combobox.h>
#include <sdlgui/dropdownbox.h>
//#include <sdlgui/progressbar.h>
#include <sdlgui/entypo.h>
#include <sdlgui/messagedialog.h>
#include <sdlgui/textbox.h>
//#include <sdlgui/slider.h>
#include <sdlgui/imagepanel.h>
#include <sdlgui/imageview.h>
//#include <sdlgui/vscrollpanel.h>
//#include <sdlgui/colorwheel.h>
//#include <sdlgui/graph.h>
//#include <sdlgui/tabwidget.h>
//#include <sdlgui/switchbox.h>
#include <sdlgui/formhelper.h>


#include <chiaki/log.h>
#include "chiaki/regist.h"
#include <chiaki/controller.h>

#include "rpi/settings.h"

/// instead of full std namespace
using std::cout;
using std::cerr;
using std::endl;

//using namespace sdlgui;
using sdlgui::Widget;
using sdlgui::Window;
using sdlgui::Button;
using sdlgui::BoxLayout;
using sdlgui::Vector2i;
using sdlgui::GridLayout;
using sdlgui::Alignment;
using sdlgui::Label;
using sdlgui::DropdownBox;
using sdlgui::Theme;
using sdlgui::Color;
using sdlgui::Orientation;
using sdlgui::TextBox;
using sdlgui::PopupButton;
using sdlgui::Popup;
using sdlgui::Cursor;	/// I edited in in sdlgui/common.h, clashes with /usr/include/X11/X.h  

using sdlgui::ToolButton;
using sdlgui::AdvancedGridLayout;
using sdlgui::GroupLayout;

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

class NanoSdlWindow;
class Host;
class IO;

class Interface
{
	public:
		Interface *NorthNeighbour;
		Interface *SouthNeighbour;
		Interface *WestNeighbour;
		Interface *EastNeighbour;
		virtual void Highlight() {};
		virtual void Reset() {};
		
	
	private:
	
};

struct CurrentWidget {

	Interface* widget=nullptr;
};




static void RegistEventCB(ChiakiRegistEvent *event, void *user);

class RpiHostIcon : public sdlgui::Button, public Interface
{
	public:
		RpiHostIcon(sdlgui::Widget *parent, const char* label);
		~RpiHostIcon();
			
	private:
};

class RpiSettingsWidget : public sdlgui::Button, public Interface
{
	public:
		RpiSettingsWidget(sdlgui::Widget *parent, std::vector<std::string> options, NanoSdlWindow* gui_object);
		~RpiSettingsWidget();
		
		void Highlight();
		void Reset();
		void OnClick();
		void OnChoiceClick(RpiSettingsWidget* choice);
		void SetNeighbours(Interface *N, Interface *S, Interface *W, Interface *E);
		
		NanoSdlWindow *gui_object;
		sdlgui::Popup *pop = nullptr;
	
	private:
		sdlgui::Theme *focused_theme;
		std::vector<std::string> allOptions;
};


class NanoSdlWindow : public sdlgui::Screen
{
	public:
		NanoSdlWindow(SDL_Window* pwindow, int rwidth, int rheight, SDL_Renderer* sdl_renderer);
		~NanoSdlWindow();
		bool start();
		int hostClick();
		void registClick();
		bool keyboardEvent(int key, int scancode, int action, int modifiers);
		virtual void draw(SDL_Renderer* renderer);
		virtual void drawContents();
		void moveFocus(int direction);  /// 1=North, 2=South, 3=West, 4=East
		virtual bool resizeEvent(int w, int h);
		void mouseClickEvent();
		void PressSelect();
		void setClientState(std::string client_state);
		void restoreGui();
		void RefreshScreenDraw();
		
		SDL_Window *sdl_window; 
		SDL_Renderer* sdl_renderer;
		bool takeInput=1;
		RpiSettings *settings = nullptr;
		//sdlgui::Widget *currentWidget = nullptr;
		sdlgui::Window *settings_win;
		Interface* current_widget;  // make this regular type, not struct
		
		RpiSettingsWidget *db1;
		RpiSettingsWidget *db2;
		RpiSettingsWidget *db3;
		RpiSettingsWidget *db4;
		RpiSettingsWidget *db5;
		

	private:
		Host *host = nullptr; 	/// controller state, session
		IO *io = nullptr;	    /// input, decode, rendering, audio
		ChiakiLog log;
		
		/// Widgets
		sdlgui::Window *top_win;
		sdlgui::BoxLayout *top_box_horizontal;
		sdlgui::BoxLayout *box_vertical;
		sdlgui::Button *host_icon;
		std::vector<sdlgui::Button*>  hosts_widgets;
		//std::vector<RpiSettingsWidget*>  settings_widgets;
		
		
		/// Register
		sdlgui::Button *regist_button;
		Popup *regist_popup = nullptr;
		TextBox *input_accId = nullptr;
		TextBox *input_pin = nullptr;
		

		
		sdlgui::Vector2i screenSize = sdlgui::Vector2i(1920, 1080);
		int running=1;		/// main gui loop
		int fullscreen=0;
		std::string client_state = "unknown";	/// unknown, notreg, standby, waiting, ready, playing. For the button.
		
		void updateHostIcon(std::string client_state);
};


#endif
