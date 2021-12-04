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

typedef int32_t i32;
typedef uint32_t u32;
typedef int32_t b32;

NanoSdlWindow::NanoSdlWindow( SDL_Window* pwindow, int rwidth, int rheight, SDL_Renderer* renderer)
{
	/// SDL2 Setup
	sdl_window = pwindow;
	sdl_renderer = renderer;
	u32 WindowFlags = SDL_WINDOW_OPENGL; /// added  | SDL_WINDOW_FULLSCREEN
	
	
	/// not sure where the best place for this is?
	host = new Host;
	io = new IO;
	host->gui = this;	/// connect up
	int ret;
	ret = io->Init(host);
	ret = host->Init(io);
	ret = io->InitGamepads();

	settings = new RpiSettings();
	settings->ReadYaml();  /// before starting discovery service
	
	ret = host->StartDiscoveryService();
	
	
	/// Imgui

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	/// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	
	
	/// settings options - THESE ARE THE ACTUAL SETTINGS SO DON'T CHANGE THE STRINGS.
	decoder_options = {"v4l2-drm"};/// "v4l2-gl", "mmal"
	sel_decoder = decoder_options[0];
	
	vcodec_options =  {"automatic" ,"h264", "h265"};
	sel_vcodec = vcodec_options[0];
	
	resolution_options = {"1080" ,"720", "540"};
	sel_resolution = resolution_options[0];
	
	framerate_options = {"60" ,"30"};
	sel_framerate = framerate_options[0];
	
	audio_options = {"hdmi" ,"jack"};
	sel_audio = audio_options[0];
	
	
	
	/// Setup Platform/Renderer backends
	gl_context = SDL_GL_CreateContext(sdl_window);
	ImGui_ImplSDL2_InitForOpenGL(sdl_window, gl_context);
	const char* glsl_version = "#version 300 es"; //was: 100
	ImGui_ImplOpenGL3_Init(glsl_version);
	
	
	printf("END Gui init\n");
}


NanoSdlWindow::~NanoSdlWindow()
{
	printf("NanoSdlWindow destroyed\n");
}


void NanoSdlWindow::SettingsDraw(const char* label, std::vector<std::string> list, std::string &select)
{
	ImGui::Text(label); ImGui::SameLine(120);
	ImVec2 button_sz(200, 30);
	
	// try popup modal instead to setting its size?
	if(ImGui::Button(select.c_str(), button_sz)){
		ImGui::OpenPopup(label);
	}
	if (ImGui::BeginPopup(label))
	{   
		for (int i = 0; i < list.size(); i++)
			if (ImGui::Selectable(list.at(i).c_str() )){
				select = list.at(i);
				// need to refresh settings in memory (which also writes to file)
				// ChangeSettingAction(std::string choice)
				ChangeSettingAction(select);
			}
		ImGui::EndPopup();
	}
}

/// changes what's seen in the gui
void NanoSdlWindow::UpdateSettingsGui()
{
	//settings->all_validated_settings
	sel_decoder = settings->all_validated_settings.at(0).sess.decoder;
	sel_vcodec = settings->all_validated_settings.at(0).sess.codec;
	sel_resolution = settings->all_validated_settings.at(0).sess.resolution;
	sel_framerate = settings->all_validated_settings.at(0).sess.fps;
	sel_audio = settings->all_validated_settings.at(0).sess.audio_device;
}

/// triggers on each settings gui change
///decoder, codec, resolution, fps, audio 
void NanoSdlWindow::ChangeSettingAction(std::string choice)
{	
	std::string setting;
	
	for(std::string ch : decoder_options) {
		if(choice == ch) {
			setting = "decoder";
			goto refresh;
		}
	}
	for(std::string ch : vcodec_options) {
		if(choice == ch) {
			setting = "codec";
			goto refresh;
		}
	}	
	for(std::string ch : resolution_options) {
		if(choice == ch) {
			setting = "resolution";
			goto refresh;
		}
	}	
	for(std::string ch : framerate_options) {
		if(choice == ch) {
			setting = "fps";
			goto refresh;
		}
	}	
	for(std::string ch : audio_options) {
		if(choice == ch) {
			setting = "audio";
			goto refresh;
		}
	}	
	printf("No matching settings found\n");
	return;
	

refresh:
	///printf("goto refresh. Setting:  %s\n", setting.c_str());
	settings->RefreshSettings(setting, choice);
}

/// This is the GUI main loop
///
bool NanoSdlWindow::start()
{	
	/// start gui'ing
	bool show_demo_window = true;
	bool show_another_window = false;
	clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	ImGuiStyle &style = ImGui::GetStyle();
	style.WindowRounding = 8.0f;///8
	style.FrameRounding = 0.0f;///4
	style.GrabRounding = style.FrameRounding;
	
	ImGuiIO &imio = ImGui::GetIO(); //(void)_imio;
	imio.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     /// Enable Keyboard Controls
	imio.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      /// Enable Gamepad Controls
	
	// Main loop
	bool done = false;
	while (!done)
	{
		/// POLL SDL EVENT QUEUE HERE
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		HandleSDLEvents();
		
		//~ SDL_Event event;
        //~ while (SDL_PollEvent(&event))
        //~ {
            //~ ImGui_ImplSDL2_ProcessEvent(&event);
            //~ if (event.type == SDL_QUIT)
                //~ done = true;
            //~ if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(sdl_window))
                //~ done = true;
        //~ }
		
		
		ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
		
		ImGuiWindowFlags window_flags = 0;
		window_flags |= ImGuiWindowFlags_NoTitleBar;
		window_flags |= ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoBackground;
        window_flags |= ImGuiWindowFlags_NoResize;
		
		const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
		
		ImGui::SetNextWindowSize(ImVec2(1600, 800), ImGuiCond_Once);
		bool* p_open = new bool;
		if (ImGui::Begin("Parent Window", p_open, window_flags) )
		{


			static ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_None | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_NoTreePushOnOpen;
			
			ImGuiWindowFlags child_flags = 0;
			
			
			// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
			{
				static float f = 0.0f;
				static int counter = 0;
			
				ImGui::SetNextWindowSize(ImVec2(1400, 300), ImGuiCond_Once);
				//printf("WinX:  %d\n", ImGui::GetContentRegionAvail().x);

				//ImGui::Begin("~~~ Playstation Hosts ~~~", p_open, window_flags);              // Create a window called "Hello, world!" and append into it.
				//ImGui::BeginChild("ChildL", ImVec2(ImGui::GetContentRegionAvail().x * 0.9f,ImGui::GetContentRegionAvail().y * 0.5), false, ImGuiWindowFlags_NavFlattened);
				ImGui::BeginChild("ChildA", ImVec2(1000,300), false, ImGuiWindowFlags_NavFlattened);
					ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

					if (ImGui::Button(client_state.c_str()))                            // Buttons return true when clicked (most widgets return true when edited/activated)
					{
						hostClick();
					}

					ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
				ImGui::EndChild();
				
				
				/// Register Dialog Popup
				
				if(open_regist)
				{	
					ImGui::OpenPopup("Register");
					ImGui::BeginPopupModal("Register", NULL, ImGuiWindowFlags_AlwaysAutoResize);
					{	
						static char c_regist_acc_id[128] = "";
						static char c_regist_pin[128] = "";
						
						ImGui::Text("Add Register info from your Playstation\n\n");
						ImGui::Text(" ");
						
						ImGui::InputText(" Account Id", c_regist_acc_id, IM_ARRAYSIZE(c_regist_acc_id));
						ImGui::Text(" ");
						
						ImGui::InputText(" Pin", c_regist_pin, IM_ARRAYSIZE(c_regist_pin));
						ImGui::Text(" ");

						if (ImGui::Button("Send", ImVec2(120, 0))) {
							
							std::string acc_id(c_regist_acc_id);
							std::string pin(c_regist_pin);
							registClick(acc_id, pin);
	
							ImGui::CloseCurrentPopup();
							open_regist=false;
						}
						ImGui::SetItemDefaultFocus();
						ImGui::SameLine();
						if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); open_regist=false; }
						ImGui::EndPopup();
					}
				}
				
				
	
				
			}
			
			if(1)
			{   
				ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 100, main_viewport->WorkPos.y + 300), ImGuiCond_Once);
				ImGui::SetNextWindowSize(ImVec2(600, 300), ImGuiCond_Once);

				ImGui::BeginChild("ChildB", ImVec2(ImGui::GetContentRegionAvail().x * 0.9f,ImGui::GetContentRegionAvail().y * 0.5), false, ImGuiWindowFlags_NavFlattened);
								
				/// Settings popup buttons
				SettingsDraw("Video Decoder", decoder_options, sel_decoder);
				
				SettingsDraw("Video Codec", vcodec_options, sel_vcodec);
				
				SettingsDraw("Resolution", resolution_options, sel_resolution);

				SettingsDraw("Framerate", framerate_options, sel_framerate);

				SettingsDraw("Audio Out", audio_options, sel_audio);		

				if (ImGui::Button("Quit")) {
					done = true;
					SDL_Event user_event;
					user_event.button.type = SDL_QUIT;
					SDL_PushEvent(&user_event);
					SDL_PushEvent(&user_event);
				}
					
				ImGui::EndChild();
			}
			
			
			
						
	}/// END parent window
	ImGui::End();

	/// Rendering
	RefreshScreenDraw();
		
		
	} // END while(!done)
	

}


void NanoSdlWindow::HandleSDLEvents()
{
	SDL_Event event;
	if(takeInput)
	{
	/// reading events from the event queue
		while (SDL_PollEvent(&event))
		{	
			ImGui_ImplSDL2_ProcessEvent(&event);
			switch (event.type)
			{
				case SDL_QUIT:
					running = 0;
					exit(0);
					break;
					
				///case SDL_CONTROLLERBUTTONUP:
				case SDL_CONTROLLERBUTTONDOWN:
				{
					//printf("Joy Button!\n");
					ChiakiControllerState press = io->GetState();
					
					//~ if(press.buttons == CHIAKI_CONTROLLER_BUTTON_DPAD_UP )
					//~ {
						//~ break;
					//~ }
					//~ if(press.buttons == CHIAKI_CONTROLLER_BUTTON_DPAD_DOWN )
					//~ {
						//~ break;
					//~ }
					//~ if(press.buttons == CHIAKI_CONTROLLER_BUTTON_DPAD_LEFT )
					//~ {
						//~ break;
					//~ }
					//~ if(press.buttons == CHIAKI_CONTROLLER_BUTTON_DPAD_RIGHT )
					//~ {
						//~ break;
					//~ }
					//~ if(press.buttons == CHIAKI_CONTROLLER_BUTTON_CROSS )
					//~ {
						//~ ///printf("Select Pressed\n");
						//~ SDL_Event user_event;
						//~ user_event.button.type = SDL_MOUSEBUTTONDOWN;
						//~ user_event.button.button = SDL_BUTTON_LEFT;
						//~ user_event.button.clicks = 1;
						//~ SDL_PushEvent(&user_event);
						//~ SDL_Delay(250);///ms
						//~ SDL_Event user_event2;
						//~ user_event2.button.type = SDL_MOUSEBUTTONUP;
						//~ user_event2.button.button = SDL_BUTTON_LEFT;
						//~ user_event2.button.clicks = 1;
						//~ SDL_PushEvent(&user_event2);
						//~ break;
					//~ }
					
					
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
					//printf("KEY DOWN!!!\n");
					switch (event.key.keysym.sym)
					{
					  case SDLK_ESCAPE:
						running = 0;
						exit(0);
						break;
					  case SDLK_F11:
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
						
					 // default:
						//break;
					}
				}
				case SDL_MOUSEBUTTONDOWN:
				{
				  //mouseClickEvent();
				  //break; let pass through
				}
				
				//default:
				//	break;
			}
			
			//this->onEvent(event);
		}
	}
	
}


void NanoSdlWindow::RefreshScreenDraw()
{	
	ImGuiIO &imio = ImGui::GetIO(); //(void)_imio;
	imio.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     /// Enable Keyboard Controls
	imio.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      /// Enable Gamepad Controls
	
	ImGui::Render();
	glViewport(0, 0, (int)imio.DisplaySize.x, (int)imio.DisplaySize.y);
	glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	SDL_GL_SwapWindow(sdl_window);	
}

void NanoSdlWindow::registClick(std::string acc_id, std::string pin)
{	
	//~ std::string pin = input_pin->value();
	//~ std::string accountID = input_accId->value();
	
	host->RegistStart(acc_id, pin);
}

int NanoSdlWindow::hostClick()
{
	printf("Host Click\n");
	int ret;
	
	/// gui:  unknown, notreg, standby, waiting, ready, playing   (this is 'client_state')

	if(client_state == "notreg") /// first click on icon
	{	
		/// This is the Register New Host GUI
		printf("Opening Register Dialog\n");
		open_regist = true;
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
		//sleep(1);	/// some time is needed here, was 1 
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
			//host_icon->setCaption(" - ? - ");
			client_state = state;
	} else {
			//host_icon->setCaption(state);
			client_state = state;
	}
	
}



//~ bool NanoSdlWindow::keyboardEvent(int key, int scancode, int action, int modifiers)
//~ {
	//~ //printf("Key press\n");	

	//~ //performLayout(mSDL_Renderer);

	//~ return false;
//~ }

void NanoSdlWindow::restoreGui()
{
	io->FiniFFmpeg();
	io->ShutdownStreamDrm(); /// good. clears stream frame buffer
	
	SDL_MaximizeWindow(sdl_window);
	host->StartDiscoveryService();  /// Did I ever stop it?
	io->SwitchInputReceiver(std::string("gui"));

}

bool NanoSdlWindow::resizeEvent(int w, int h)
{	
	printf("New Window Size: %dx%d\n", w, h);
	
	return false;
}

//~ void NanoSdlWindow::mouseClickEvent()
//~ {
	//~ printf("mouseClickEvent\n");
//~ }

//~ void NanoSdlWindow::draw(SDL_Renderer* renderer)
//~ {	
//~ }

//~ void NanoSdlWindow::drawContents()
//~ {
//~ }


