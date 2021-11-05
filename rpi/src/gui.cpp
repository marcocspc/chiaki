#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>		/// sleep()

#include "rpi/gui.h"
#include "rpi/host.h"
#include "rpi/io.h"
#include "rpi/settings.h"

#define WinWidth 1920
#define WinHeight 1080
#define NANOGUI_USE_GLES
typedef int32_t i32;
typedef uint32_t u32;
typedef int32_t b32;


RpiHostIcon::RpiHostIcon(Widget *parent, const char* label)
	: Button(parent, label, 0)
{	

}

RpiHostIcon::~RpiHostIcon()
{
}

RpiSettingsWidget::RpiSettingsWidget(sdlgui::Widget *parent, std::vector<std::string> options, NanoSdlWindow* gui_object)
	: Button(parent, options.at(0), [this] { OnClick(); })
{	
	this->gui_object = gui_object;
	allOptions = options;
	setFontSize(18);
	setTextColor(Color(200,200,200,255));
}

RpiSettingsWidget::~RpiSettingsWidget()
{	
}

void RpiSettingsWidget::SetNeighbours(Interface *N, Interface *S, Interface *W, Interface *E)
{
	NorthNeighbour = N;
	SouthNeighbour = S;
	WestNeighbour = W;
	EastNeighbour = E;
}

void RpiSettingsWidget::OnClick()
{
	printf("Settings Click\n");
	/// New popup window to the right of button.
	pop = new Popup(gui_object, gui_object->settings_win);///(Widget *parent, Window *parentWindow);
	Vector2i buttonPos = this->position();
	pop->setAnchorPos(buttonPos+Vector2i(220, 15));
	
	Theme *popup_theme = new Theme(gui_object->sdl_renderer);
    popup_theme->mWindowDropShadowSize = 0;
	popup_theme->mWindowHeaderHeight = 0;
	///popup_theme->mButtonGradientTopUnfocused = Color(150,150,150,255);
	///popup_theme->mButtonGradientBotUnfocused = Color(100,100,100,255);
    pop->setTheme(popup_theme);
	
	BoxLayout *box = new BoxLayout(Orientation::Vertical, Alignment::Middle, 5, 5);
	pop->setLayout(box);

	/// Create all option buttons
	std::vector<RpiSettingsWidget*> option_widgets;
		
	for(std::string l : allOptions)
	{
		std::vector<std::string> tmpStr = {l};
		RpiSettingsWidget *tmp = new RpiSettingsWidget(pop, tmpStr, gui_object);  /// parent is caller button
		tmp->setCallback([this, tmp] { OnChoiceClick(tmp); } ); /// seems always need 'this' as well
		tmp->setFixedSize(Vector2i(160, 30));
		option_widgets.push_back(tmp);
	}

	/// Link the widget list for traversal
	int i=0;
	for(RpiSettingsWidget* w : option_widgets)
	{
		if(i==0){
			w->SetNeighbours(NULL, option_widgets.at(i+1), NULL, NULL);
		} else
		if(i>0 && i<(option_widgets.size()-1)){
			w->SetNeighbours(option_widgets.at(i-1), option_widgets.at(i+1), NULL, NULL);
		} else
		if(i == option_widgets.size()-1){
			w->SetNeighbours(option_widgets.at(i-1), NULL, NULL, NULL);
		}
				
		i++;
	}

	gui_object->current_widget->Reset();
	gui_object->current_widget = option_widgets.at(0);
	gui_object->current_widget->Highlight(); // mouse position does not follow
	
	gui_object->performLayout(gui_object->sdl_renderer);
}

void RpiSettingsWidget::OnChoiceClick(RpiSettingsWidget* choice)
{
	printf("Choice was: %s\n", choice->caption().c_str() );
	this->setCaption(choice->caption());  		/// confusing but 'this' is the originator button
	choice->parent()->setVisible(false);  		/// hide choice menu
	this->gui_object->current_widget = this;	/// set current widget back to originator
	this->Highlight();					        /// highlight it
}

void RpiSettingsWidget::Highlight()
{	
	Vector2i pos = absolutePosition();
	//SDL_WarpMouseGlobal(pos.x+5, pos.y+5);
	SDL_WarpMouseInWindow(gui_object->sdl_window, pos.x+5, pos.y+5);
	//setTextColor(Color(255,255,255,255));
	setTextColor(Color(255,255,150,255));
}

void RpiSettingsWidget::Reset()
{	
	setTextColor(Color(200,200,200,255));
}


/// 'this'  == Screen,  "the root element of a hierarchy of nanogui widgets"
NanoSdlWindow::NanoSdlWindow( SDL_Window* pwindow, int rwidth, int rheight, SDL_Renderer* renderer)
	: Screen( pwindow, Vector2i(rwidth, rheight), "Chiaki")
{
	/// SDL2 Setup
    sdl_window = pwindow;
    sdl_renderer = renderer;
    u32 WindowFlags = SDL_WINDOW_OPENGL; /// added  | SDL_WINDOW_FULLSCREEN

	/// Nanogui
	Theme *window_theme = new Theme(sdl_renderer);
	window_theme->mWindowDropShadowSize = 0;
	window_theme->mWindowHeaderHeight = 0;
	window_theme->mWindowFillUnfocused = Color(128,0,64,255);
    window_theme->mWindowFillFocused = Color(128,0,64,255);
    
    Theme *settings_theme = new Theme(sdl_renderer);
	settings_theme->mWindowDropShadowSize = 0;
	settings_theme->mWindowHeaderHeight = 0;
	settings_theme->mWindowFillUnfocused = Color(0,180,90,255);
    settings_theme->mWindowFillFocused = Color(0,180,90,255);
    
    //~ popup_theme = new Theme(sdl_renderer);
    //~ popup_theme->mWindowDropShadowSize = 0;
	//~ popup_theme->mWindowHeaderHeight = 0;
	//~ popup_theme->mWindowFillUnfocused = Color(128,128,128,0);
    //~ popup_theme->mWindowFillFocused = Color(128,128,128,0);
    
    
    
    /// H O S T S
    
    //~ std::vector<int> cols = {screenSize.x/3, screenSize.x/3, screenSize.x/3}; //cols.push_back(1920); /// list pf size in pixels
	//~ std::vector<int> rows = {screenSize.y/2}; //rows.push_back(540); /// list pf size in pixels
	//~ AdvancedGridLayout *hosts_grid = new AdvancedGridLayout(cols, rows, 0);
    
	/// The Top hosts window
	top_win = new Window(this, "");
	//top_win->setFixedSize(Vector2i(screenSize.x, screenSize.y*0.5));
	top_win->setSize(Vector2i(screenSize.x, screenSize.y*0.5));
	top_win->setTheme(window_theme);
	top_box_horizontal = new BoxLayout(Orientation::Horizontal, Alignment::Middle, 20, 20); /// Abusing the pad values for forcing placement 
	top_win->setLayout(top_box_horizontal);
	///settings_win->center();

	host_icon = new Button(top_win, " - - - ", [this] { hostClick(); });
	host_icon->setFixedSize(Vector2i(200, 200));
	hosts_widgets.push_back(host_icon);
	//host_icon->setPosition(Vector2i(800, 300));
	//hosts_grid->setAnchor(host_icon, AdvancedGridLayout::Anchor(1, 0, 1, 1, Alignment::Middle, Alignment::Middle));
	
	
	
	/// S E T T I N G S
	
	//std::vector<int> cols = {screenSize.x/3, screenSize.x/3, screenSize.x/3}; //cols.push_back(1920); /// list pf size in pixels
	//std::vector<int> rows = {screenSize.y/2}; //rows.push_back(540); /// list pf size in pixels
	//AdvancedGridLayout *settings_grid = new AdvancedGridLayout(cols, rows, 0);
	//AdvancedGridLayout::Anchor settings_anchor(1, 0, 1, 1, Alignment::Middle, Alignment::Middle);

	
	/// Lower Window for settings widgets
	settings_win = new Window(this, "");
	//settings_win->setFixedSize(Vector2i(screenSize.x, screenSize.y*0.5));
	settings_win->setPosition(Vector2i(0, screenSize.y*0.5));
	settings_win->setTheme(settings_theme);

	GridLayout *vert_group = new GridLayout(Orientation::Horizontal, 2, Alignment::Middle, 10, 10); /// defaults to two columns
	settings_win->setLayout(vert_group);

	settings_win->label("Decoder: ").setFontSize(20);
	db1 = new RpiSettingsWidget(settings_win, std::vector<std::string>{ "V4L2-DRM", "MMAL" }, this);
	db1->setFixedWidth(200);
	//settings_grid->setAnchor(db1, settings_anchor);


	settings_win->label("Video Codec: ").setFontSize(20);
	db2 = new RpiSettingsWidget(settings_win, std::vector<std::string>{ "Automatic" ,"h264", "h265" }, this);
	db2->setFixedWidth(200);
	//settings_grid->setAnchor(db2, settings_anchor);


	settings_win->label("Resolution: ").setFontSize(20);
	db3 = new RpiSettingsWidget(settings_win, std::vector<std::string>{ "1080" ,"720", "540"}, this);
	db3->setFixedWidth(200);
	//settings_grid->setAnchor(db3, settings_anchor);


	settings_win->label("Framerate: ").setFontSize(20);
	db4 = new RpiSettingsWidget(settings_win, std::vector<std::string>{ "60" ,"30"}, this);
	db4->setFixedWidth(200);
	//settings_grid->setAnchor(db4, settings_anchor);


	settings_win->label("Audio Device: ").setFontSize(20);
	db5 = new RpiSettingsWidget(settings_win, std::vector<std::string>{ "HDMI" ,"Jack", "aoaoao"}, this);	
	db5->setFixedWidth(200);
	//settings_grid->setAnchor(db5, settings_anchor);


	
	/// 1=North, 2=South, 3=West, 4=East
	db1->SetNeighbours(NULL, db2, NULL, NULL);
	db2->SetNeighbours(db1, db3, NULL, NULL);
	db3->SetNeighbours(db2, db4, NULL, NULL);
	db4->SetNeighbours(db3, db5, NULL, NULL);
	db5->SetNeighbours(db4, NULL, NULL, NULL);
	
	performLayout(sdl_renderer);
	
	db1->Highlight();
	current_widget = db1;
	//SDL_ShowCursor(0);

	/// not sure where the best place for this is?
	host = new Host;
	io = new IO;
	host->gui = this;	/// connect up

	int ret;
	ret = io->Init(host);
	ret = host->Init(io);
	ret = io->InitGamepads();

	settings = new RpiSettings();
	settings->ReadYaml();

	ret = host->StartDiscoveryService();
}


NanoSdlWindow::~NanoSdlWindow()
{
}

/// This is the GUI main loop
bool NanoSdlWindow::start()
{	
	SDL_Event event;
	while (running)
	{
		SDL_Delay(8);///ms
		
		if(takeInput)
		{
		/// reading events from the event queue
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
					case SDL_QUIT:
						running = 0;
						exit(0);
						break;
						
					///case SDL_CONTROLLERBUTTONUP:
					case SDL_CONTROLLERBUTTONDOWN:
					{
						///printf("Joy Button!\n");
						ChiakiControllerState press = io->GetState();
						
						/// 1=North, 2=South, 3=West, 4=East
						if(press.buttons == CHIAKI_CONTROLLER_BUTTON_DPAD_UP )
						{
							moveFocus(1);
							break;
						}
						if(press.buttons == CHIAKI_CONTROLLER_BUTTON_DPAD_DOWN )
						{
							moveFocus(2);
							break;
						}
						if(press.buttons == CHIAKI_CONTROLLER_BUTTON_DPAD_LEFT )
						{
							moveFocus(3);
							break;
						}
						if(press.buttons == CHIAKI_CONTROLLER_BUTTON_DPAD_RIGHT )
						{
							moveFocus(4);
							break;
						}
						if(press.buttons == CHIAKI_CONTROLLER_BUTTON_CROSS )
						{
							///printf("Select Pressed\n");
							SDL_Event user_event;
							user_event.button.type = SDL_MOUSEBUTTONDOWN;
							user_event.button.button = SDL_BUTTON_LEFT;
							user_event.button.clicks = 1;
							SDL_PushEvent(&user_event);
							SDL_Delay(250);///ms
							SDL_Event user_event2;
							user_event2.button.type = SDL_MOUSEBUTTONUP;
							user_event2.button.button = SDL_BUTTON_LEFT;
							user_event2.button.clicks = 1;
							SDL_PushEvent(&user_event2);
							break;
						}
						
						
						break;
					}
					case SDL_JOYDEVICEADDED:
						printf("Gamepad added\n");
						io->RefreshGamepads();
						break;
					case SDL_JOYDEVICEREMOVED:
						printf("Gamepad removed\n");
						io->RefreshGamepads();
						break;
						
					case SDL_KEYDOWN:
					{	
						printf("KEY DOWN!!!\n");
						switch (event.key.keysym.sym)
						{
						  case SDLK_ESCAPE:
							running = 0;
							exit(0);
							break;
						  case 'f':
							fullscreen = !fullscreen;
							if (fullscreen)
							{
							  SDL_SetWindowFullscreen(sdl_window, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP);
							  SDL_SetWindowSize(sdl_window, 1920, 1080);
							}
							else
							{
							  SDL_SetWindowFullscreen(sdl_window, SDL_WINDOW_OPENGL);
							}
							break;
							
						  default:
							break;
						}
					}
					case SDL_MOUSEBUTTONDOWN:
					{
					  mouseClickEvent();
					  //break; let pass through
					}
					
					//default:
					//	break;
				}
				
				this->onEvent(event);
			}
		}
		
		// Render should not really happen here every 8ms
		// Should have its own thread or something
		RefreshScreenDraw();
	}
}


void NanoSdlWindow::RefreshScreenDraw()
{
	//orig
    //glViewport(0, 0, WinWidth, WinHeight);
    //glClearColor(0.1, 0.0, 0.1, 0.0);
    //glClear(GL_COLOR_BUFFER_BIT);
    
    //nano
    SDL_RenderClear(sdl_renderer);
        
    this->drawAll();
    // Render the rect to the screen
    SDL_RenderPresent(sdl_renderer);
  
    //orig
    SDL_GL_SwapWindow(sdl_window);
	
}

void NanoSdlWindow::registClick()
{	
	std::string pin = input_pin->value();
	std::string accountID = input_accId->value();
	
	host->RegistStart(accountID, pin);
	
	input_accId->setValue("");
	input_pin->setValue("");
	regist_popup->setVisible(false);
}

int NanoSdlWindow::hostClick()
{
	printf("Host Click\n");
	int ret;
	
	/// gui:  unknown, notreg, standby, waiting, ready, playing   (this is 'client_state')

	if(client_state == "notreg") /// first click on icon
	{	
		/// This is the Register New Host GUI
		if(regist_popup != nullptr) { /// already created

			if(regist_popup->visible()) regist_popup->setVisible(false);
			else {
				input_accId->setValue("");
				input_pin->setValue("");
				regist_popup->setVisible(true);
			}
			return 1;
		}
		
		regist_popup = new Popup(top_win, top_win);
		Vector2i registPos = host_icon->position();
		regist_popup->setAnchorPos(registPos+Vector2i(220, 20));
		regist_popup->setFixedSize(Vector2i(300, 240));
		
		regist_popup->withLayout<GroupLayout>();
		regist_popup->label("REGISTER NEW");
		
		regist_popup->label("Account ID");
		input_accId = new TextBox(regist_popup,"");
		input_accId->withAlignment(TextBox::Alignment::Left);
		input_accId->setEditable(true);
		
		regist_popup->label("PIN");
		input_pin = new TextBox(regist_popup,"");
		input_pin->withAlignment(TextBox::Alignment::Left);
		input_pin->setEditable(true);
		
		regist_popup->label("   ");
		
		regist_button = new Button(regist_popup, "Register", [this] { registClick(); });
        
        performLayout(sdl_renderer);
        
		return 0;
	}

	if(client_state == "unknown")
	{	
		printf("State Unknown\n");
		return 0;
	}
	
	if(client_state == "standby") /// orange, sleeping  (blue is Off ?)
	{
		printf("Sending Wakeup!\n");
		uint64_t credential = (uint64_t)strtoull(host->connect_info.regist_key, NULL, 16);
		chiaki_discovery_wakeup(&log, NULL,	host->connect_info.host, credential, 0);// Last is this->IsPS5()		
		setClientState("waiting");
		return 0;
	}
	
	if(client_state == "ready") /// white, good to go
	{
		printf("Starting Play session!\n");
		
		SDL_MinimizeWindow(sdl_window);

		/// transfer drm_fd
		SDL_SysWMinfo WMinfo;
		SDL_VERSION(&WMinfo.version);
		SDL_GetWindowWMInfo(sdl_window, &WMinfo);
		printf("SDL drm_fd:  %d\n", WMinfo.info.kmsdrm.drm_fd);
		io->drm_fd = WMinfo.info.kmsdrm.drm_fd;
				
		ret = io->InitFFmpeg();
		sleep(1);	/// some time is needed here, was 1 
		host->StartSession();
		io->SwitchInputReceiver(std::string("session"));
		setClientState("playing");
		return 0;
	}
	
	//~ if(ret==0) {  /// 0 is good
		//~ printf("Will try starting the session\n");
		//~ host.StartSession();
	//~ }
	
	return 0;
}

void NanoSdlWindow::PressSelect()
{
	
}

void NanoSdlWindow::setClientState(std::string state)
{	
	/// host: unknown, standby, ready					  (this is host 'state')
	/// gui:  unknown, notreg, standby, waiting, ready, playing   (this is 'client_state')
	
	//~ if(settings->all_host_settings.size() == 0) { // just first one for now
		//~ host_icon->setCaption("notreg");
		//~ client_state = "notreg";
		//~ return;
	//~ }

	if(state == "unknown") {
			host_icon->setCaption(" - ? - ");
			client_state = state;
	} else {
			host_icon->setCaption(state);
			client_state = state;
	}
	
}

void NanoSdlWindow::updateHostIcon(std::string client_state)
{
	host_icon->setCaption(client_state);
}

bool NanoSdlWindow::keyboardEvent(int key, int scancode, int action, int modifiers)
{
	//printf("Key press\n");	

	//performLayout(mSDL_Renderer);

	return false;
}

/// Think I need an Interface here
/// 1=North, 2=South, 3=West, 4=East
void NanoSdlWindow::moveFocus(int direction)
{
	if(direction == 1)  /// up
	{	
		/// Move focus/mouse to next widget (Northwards)
		if(current_widget->NorthNeighbour) {
			current_widget->NorthNeighbour->Highlight();
			current_widget->Reset();
			current_widget = current_widget->NorthNeighbour;
		}
	}
	
	if(direction == 2)  /// down
	{	
		/// Move focus/mouse to next widget (Southwards)
		if(current_widget->SouthNeighbour) {
			current_widget->SouthNeighbour->Highlight();
			current_widget->Reset();
			current_widget = current_widget->SouthNeighbour;
		}
	}
		
		
}

void NanoSdlWindow::restoreGui()
{
	io->FiniFFmpeg();
	io->ShutdownStreamDrm(); /// good. clears stream frame buffer
	
	SDL_MaximizeWindow(sdl_window);
	host->StartDiscoveryService();
	io->SwitchInputReceiver(std::string("gui"));

}

bool NanoSdlWindow::resizeEvent(int w, int h)
{	
	screenSize = Vector2i(w, h);
	printf("New Window Size: %dx%d\n", w, h);
	
	
	//~ top_win->setFixedSize(Vector2i(screenSize.x, screenSize.y*0.5));

	//~ settings_win->setFixedSize(Vector2i(screenSize.x, screenSize.y*0.5));
	//~ settings_win->setPosition(Vector2i(0, screenSize.y*0.5));
		
	performLayout(mSDL_Renderer);  // <-- cause of slowdown!? 
	
	return false;
}

void NanoSdlWindow::mouseClickEvent()
{
	//printf("mouseClickEvent\n");
	
	performLayout(mSDL_Renderer);
}

void NanoSdlWindow::draw(SDL_Renderer* renderer)
{	
	Screen::draw(renderer);
}

void NanoSdlWindow::drawContents()
{
}


