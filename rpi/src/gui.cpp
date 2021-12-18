#include <iostream>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>		/// sleep()

///#include <X11/Xlib.h>   /// also need to link :(

#include "rpi/gui.h"
#include "rpi/host.h"
#include "rpi/io.h"
#include "rpi/settings.h"

//#include <rpi/glhelp.h>
#include <drm_fourcc.h>

#define WinWidth 1920
#define WinHeight 1080

typedef int32_t i32;
typedef uint32_t u32;
typedef int32_t b32;


//~ inline void MessageCallback( GLenum source,
                 //~ GLenum type,
                 //~ GLuint id,
                 //~ GLenum severity,
                 //~ GLsizei length,
                 //~ const GLchar* message,
                 //~ const void* userParam )
//~ {
  //~ fprintf( stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
           //~ ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            //~ type, severity, message );
//~ }

/// EGL error check
void checkEgl() {
	int err;        
	if((err = eglGetError()) != EGL_SUCCESS){
		printf("Error after peglCreateImageKHR!, error = %x\n", err);
	} else {
		printf("eglCreateImageKHR Success! \n");
	}
}

/// GL Error Check
void checkGl(){
	GLenum err;
	while ( ( err = glGetError() ) != GL_NO_ERROR) {
		std::cerr << err;        
	}
}

/// Use GL_LUMINANCE for type
static const unsigned char texture_data[] =
{
	0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
	0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
	0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
	0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
	0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
	0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
	0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
	0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF
};

/// negative x,y is bottom left and first vertex
static const GLfloat vertices[][4][3] =
{
    { {-1.0, -1.0, 0.0}, { 1.0, -1.0, 0.0}, {-1.0, 1.0, 0.0}, {1.0, 1.0, 0.0} }
};
static const GLfloat uv_coords[][4][2] =
{
    { {0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0} }
};

// OpenGL shader setup
#define DECLARE_YUV2RGB_MATRIX_GLSL \
	"const mat4 yuv2rgb = mat4(\n" \
	"    vec4(  1.1644,  1.1644,  1.1644,  0.0000 ),\n" \
	"    vec4(  0.0000, -0.2132,  2.1124,  0.0000 ),\n" \
	"    vec4(  1.7927, -0.5329,  0.0000,  0.0000 ),\n" \
	"    vec4( -0.9729,  0.3015, -1.1334,  1.0000 ));"

static const GLchar* vertex_shader_source =
	"#version 300 es\n"
	"in vec3 position;\n"
	"in vec2 tx_coords;\n"
	"out vec2 v_texCoord;\n"
	"void main() {  \n"
	"	gl_Position = vec4(position, 1.0);\n"
	"	v_texCoord = tx_coords;\n"
	"}\n";
	
static const GLchar* fragment_shader_source =
	"#version 300 es\n"
	"#extension GL_OES_EGL_image_external : require\n"
	"precision mediump float;\n"
	"//uniform samplerExternalOES texture;\n"
	"uniform sampler2D texture;\n"
	"in vec2 v_texCoord;\n"
	"out vec4 out_color;\n"
	"void main() {	\n"
	"	//out_color = texture2D( texture, v_texCoord ) + vec4(v_texCoord.x*0.25,v_texCoord.y*0.25, 0, 1);\n"
	"	out_color = texture2D( texture, v_texCoord ) + vec4(v_texCoord.x*0.25,v_texCoord.y*0.25, 0, 1);\n"
	"	//out_color = vec4(1,1,0,1);\n"
	"	//out_color = vec4(v_texCoord.x,v_texCoord.y,0,1);\n"
	"}\n";
	
GLint common_get_shader_program(const char *vertex_shader_source, const char *fragment_shader_source) {
	enum Consts {INFOLOG_LEN = 512};
	GLchar infoLog[INFOLOG_LEN];
	GLint fragment_shader;
	GLint shader_program;
	GLint success;
	GLint vertex_shader;

	/* Vertex shader */
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertex_shader, INFOLOG_LEN, NULL, infoLog);
		printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
	}

	/* Fragment shader */
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragment_shader, INFOLOG_LEN, NULL, infoLog);
		printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
	}

	/* Link shaders */
	shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);
	glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shader_program, INFOLOG_LEN, NULL, infoLog);
		printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return shader_program;
}

static EGLint texgen_attrs[] = {
   EGL_DMA_BUF_PLANE0_FD_EXT,
   EGL_DMA_BUF_PLANE0_OFFSET_EXT,
   EGL_DMA_BUF_PLANE0_PITCH_EXT,
   EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT,
   EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT,
   EGL_DMA_BUF_PLANE1_FD_EXT,
   EGL_DMA_BUF_PLANE1_OFFSET_EXT,
   EGL_DMA_BUF_PLANE1_PITCH_EXT,
   EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT,
   EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT,
   EGL_DMA_BUF_PLANE2_FD_EXT,
   EGL_DMA_BUF_PLANE2_OFFSET_EXT,
   EGL_DMA_BUF_PLANE2_PITCH_EXT,
   EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT,
   EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT,
};

NanoSdlWindow::NanoSdlWindow( SDL_Window* pwindow, int rwidth, int rheight, SDL_Renderer* renderer)
{
	/// SDL2 Setup
	sdl_window = pwindow;
	sdl_renderer = renderer;
	u32 WindowFlags = SDL_WINDOW_OPENGL; /// added  | SDL_WINDOW_FULLSCREEN
	
	if(std::strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0)///0 is match
		IsX11 = true;
	else IsX11 = false;
	printf("Running under X11: %d\n", IsX11);
	
	//~ if(XOpenDisplay(NULL))
		//~ printf("Running under X11\n");
		
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
	
	//  T E M P
	return;


	/// Imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	/// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	
	
	/// settings options - THESE ARE THE ACTUAL SETTINGS SO DON'T CHANGE THE STRINGS.
	decoder_options = {"automatic", "v4l2-drm"};///"v4l2-drm", "v4l2-gl", "mmal"
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
	
	// try popup modal instead for bigger popup?
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
	sleep(2);
	io->InitFFmpeg();
	sleep(3);
	InitVideoGl();
	sleep(1);	/// some time is needed here, was 1 
	host->StartSession();
	io->SwitchInputReceiver(std::string("session"));
	setClientState("playing");
	printf("Starting test run\n");
	while(1)
	{	
		// Probably something to do with running between different threads?
		
		//HandleSDLEvents();
		usleep(30000);
		//glViewport(0, 0, 1280, 720); // should set in Resize() callback
		glBindTexture(GL_TEXTURE_EXTERNAL_OES, new_texture);
		//glBindTexture(GL_TEXTURE_2D, texture);
		//glGetUniformLocation(program, "texture");
		//glUseProgram(program);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		SDL_GL_SwapWindow(sdl_window);
	}
	return false;
	
	
	/// start gui'ing
	bool show_demo_window = true;
	bool show_another_window = false;
	clear_color = ImVec4(0.45f, 0.55f, 0.60f, 0.1f);

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

// Now using glDmaTexture
int NanoSdlWindow::InitVideoGl()
{
	GLuint vbo;
	GLint pos;
	GLint uvs;
	
	// Use v-sync=(1)
	SDL_GL_SetSwapInterval(0);
	///SDL_HINT_RENDER_VSYNC 0
	// Disable depth test and face culling.
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	printf("GL_VERSION  : %s\n", glGetString(GL_VERSION) );
	printf("GL_RENDERER : %s\n", glGetString(GL_RENDERER) );
	
	/// Shader
	program = common_get_shader_program(vertex_shader_source, fragment_shader_source);
	pos = glGetAttribLocation(program, "position");
	uvs = glGetAttribLocation(program, "tx_coords");
	
	/// Geometry
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices)+sizeof(uv_coords), 0, GL_STATIC_DRAW);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), sizeof(uv_coords), uv_coords);
	glEnableVertexAttribArray(pos);
	glEnableVertexAttribArray(uvs);
	glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glVertexAttribPointer(uvs, 2, GL_FLOAT, GL_FALSE, 0,  (void*)sizeof(vertices) ); /// last is offset to loc in buf memory
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	/// Texture init
	    
    /// Test with static data texture (from above) works. (edit shader too)
    //~ glBindTexture(GL_TEXTURE_2D, texture);
	//~ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);//or GL_LINEAR
    //~ glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //~ glTexSubImage2D(GL_TEXTURE_2D,
                    //~ 0,
                    //~ 0, 0,
                    //~ 8, 8,
                    //~ GL_LUMINANCE, GL_UNSIGNED_BYTE,
                    //~ texture_data);
	//~ glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 8, 8, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, texture_data);
    /// End Test texture
    
	//~ glGenTextures(1, &texture);
	//~ glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
	//~ //checkGl();  //ok
	//~ glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //~ glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//~ //checkGl(); //ok
	//~ glTexImage2D(GL_TEXTURE_EXTERNAL_OES, 0, GL_RGBA, 1280, 720, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	//~ //checkGl();//err code 1280
	
	glHint(GL_GENERATE_MIPMAP_HINT, GL_FASTEST);
	glUseProgram(program);
	//glGetUniformLocation(program, "texture");
	
	// below gets err code 1280
	//glTexSubImage2D(GL_TEXTURE_EXTERNAL_OES, 0, 0,0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);///nullptr should work

	// Gl err code GL_INVALID_ENUM	1280	Set when an enumeration parameter is not legal 
	
	//glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
	//glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);
	//glTexSubImage2D(GL_TEXTURE_EXTERNAL_OES, 0, 0,0,1,1,GL_RGBA, GL_UNSIGNED_BYTE, nullptr); // last = uv_default ?? or nullptr
	//glTexImage2D(GL_TEXTURE_EXTERNAL_OES, 0, GL_RGBA, 1, 1, 0, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, nullptr);
	//glGetUniformLocation(program, "texture");
	//glUseProgram(program);
	//glGetUniformLocation(program, "texture");
	
	
	
	/// Texture Init from 'tearing'
	//~ GLuint targetTexture;
	//~ glGenTextures(1, &targetTexture);
	//~ glBindTexture(GL_TEXTURE_2D, targetTexture);
	//~ glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mode->width, mode->height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

	

	
	return 1;
}



bool NanoSdlWindow::UpdateFromGUI(AVFrame *frame)
{	
	
	// perf test
	//return true; // Still 17-18% CPU!
	
	
	//printf("\nBEGIN AVOpenGLFrame::Update\n");
	//auto f = QOpenGLContext::currentContext()->extraFunctions();
	//printf("PIX Format in Update():  %d\n", frame->format); // 181 is the one!
	//printf("DRM_PRIME as int in Update(): %d\n", AV_PIX_FMT_DRM_PRIME); //prints 182
	
	if(frame->format != AV_PIX_FMT_DRM_PRIME)
	{	
		printf("Inside Check.  Ie not a AV_PIX_FMT_DRM_PRIME Frame\n");
		return false;
	}
	
	//needed for aspect ratio fitting
	int width = frame->width;
	int height = frame->height;

	
	
	/// My one
	const AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor*)frame->data[0];
	int fd = -1;
	fd = desc->objects[0].fd;
	//printf("FD: %d\n", fd);
	//ORIG GLuint texture = 1;

	EGLint attribs[50];
	EGLint *a = attribs;
	const EGLint *b = texgen_attrs;
	
	*a++ = EGL_WIDTH;
	*a++ = av_frame_cropped_width(frame);
	*a++ = EGL_HEIGHT;
	*a++ = av_frame_cropped_height(frame);
	*a++ = EGL_LINUX_DRM_FOURCC_EXT;
	*a++ = desc->layers[0].format;

	int i, j;
	for (i = 0; i < desc->nb_layers; ++i) {
            for (j = 0; j < desc->layers[i].nb_planes; ++j) {
                const AVDRMPlaneDescriptor * const p = desc->layers[i].planes + j;
                const AVDRMObjectDescriptor * const obj = desc->objects + p->object_index;
                *a++ = *b++;
                *a++ = obj->fd;
                *a++ = *b++;
                *a++ = p->offset;
                *a++ = *b++;
                *a++ = p->pitch;
                if (obj->format_modifier == 0) {
                   b += 2;
                }
                else {
                   *a++ = *b++;
                   *a++ = (EGLint)(obj->format_modifier & 0xFFFFFFFF);
                   *a++ = *b++;
                   *a++ = (EGLint)(obj->format_modifier >> 32);
                }
            }
     }
	
	 *a = EGL_NONE;
	
	
	SDL_SysWMinfo WMinfo;
	SDL_VERSION(&WMinfo.version);
	SDL_GetWindowWMInfo(sdl_window, &WMinfo);
	EGLNativeDisplayType hwnd = WMinfo.info.x11.display;  /// WMinfo.info.kmsdrm.drm_fd, WMinfo.info.x11.display
	EGLDisplay egl_display = eglGetDisplay(hwnd);
	
	///SDL_GL_GetCurrentContext   ORIG!
	//EGLDisplay egl_display = eglGetCurrentDisplay();
	
///printf("EGLDisplay:  %d\n", egl_display);///6062456, 28434872, 8860088
	const EGLImage image = eglCreateImageKHR( egl_display,
                                              EGL_NO_CONTEXT,
                                              EGL_LINUX_DMA_BUF_EXT,
                                              NULL, attribs);
	if (!image) {
		printf("Failed to create EGLImage\n");
		return -1;
	}

	/// his
	glGenTextures(1, &texture);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, nullptr);

	new_texture = texture;

	eglDestroyImageKHR(&egl_display, image);
	
	//printf("END AVOpenGLFrame::Update\n\n");
	return true;
}



bool NanoSdlWindow::UpdateAVFrame(AVFrame *frame)
{	
	
	// TEMP TEST
	UpdateFromGUI(frame);
	return true;
	
	
	
	if(frame->format != AV_PIX_FMT_DRM_PRIME)
	{	
		printf("UpdateAVFrame: Not a AV_PIX_FMT_DRM_PRIME Frame, %d\n", frame->format);
		return false;
	}
	
	//needed for aspect ratio fitting
	int width = frame->width;
	int height = frame->height;

	
	
	/// My one
	const AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor*)frame->data[0];
	int fd = -1;
	fd = desc->objects[0].fd;
	//printf("FD: %d\n", fd);
	//GLuint texture = 1;
	texture = 1;
	
	int TEST_fd=-1;

	EGLint attribs[50];
	EGLint *a = attribs;
	const EGLint *b = texgen_attrs;
	
	*a++ = EGL_WIDTH;
	*a++ = av_frame_cropped_width(frame);
	*a++ = EGL_HEIGHT;
	*a++ = av_frame_cropped_height(frame);
	*a++ = EGL_LINUX_DRM_FOURCC_EXT;
	*a++ = desc->layers[0].format;

	int i, j;
	for (i = 0; i < desc->nb_layers; ++i) {
            for (j = 0; j < desc->layers[i].nb_planes; ++j) {
                const AVDRMPlaneDescriptor * const p = desc->layers[i].planes + j;
                const AVDRMObjectDescriptor * const obj = desc->objects + p->object_index;
                *a++ = *b++;
                *a++ = obj->fd; TEST_fd = obj->fd;
                *a++ = *b++;
                *a++ = p->offset;
                *a++ = *b++;
                *a++ = p->pitch;
                if (obj->format_modifier == 0) {
                   b += 2;
                }
                else {
                   *a++ = *b++;
                   *a++ = (EGLint)(obj->format_modifier & 0xFFFFFFFF);
                   *a++ = *b++;
                   *a++ = (EGLint)(obj->format_modifier >> 32);
                }
            }
     }
	
	 *a = EGL_NONE;
	
	SDL_SysWMinfo WMinfo;
	SDL_VERSION(&WMinfo.version);
	SDL_GetWindowWMInfo(sdl_window, &WMinfo);
	EGLNativeDisplayType hwnd = WMinfo.info.x11.display;  /// WMinfo.info.kmsdrm.drm_fd, WMinfo.info.x11.display
	EGLDisplay egl_display = eglGetDisplay(hwnd);
	//printf("EGLDisplay A:  %d\n", egl_display);// should not be 0 or negative
	
	//EGLint *major=nullptr, *minor=nullptr;
	//eglInitialize(egl_display, major, minor);
	//segfaults printf("EGL Init: v%d.%d\n", *major, *minor);
	
	//EGLNativeDisplayType hhh = EGLNativeDisplayType(EGL_DEFAULT_DISPLAY);
	//EGLDisplay _egl_display = eglGetDisplay(hhh);
	
	///SDL_GL_GetCurrentDisplay() but only SDL2 2.0.20
	/// None of these seem to work. Error is NOT the egl_display !?!!
	//EGLDisplay egl_display = eglGetCurrentDisplay();///old orig, no work in X11 or CLI. Fine in Chiaki-gui
	//EGLDisplay egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);/// ok but eglCreateImageKHR no work in either
	//printf("EGLDisplay B:  %d\n", _egl_display);// should not be 0 or negative	
	if(egl_display == EGL_NO_DISPLAY){
		printf("UpdateAVFrame:  No EGL Display found\n");
		return -1;
	}

	//SDL_GLContext glcontext = SDL_GL_CreateContext(sdl_window);


	/// Should test with the egl_vout code? No its using lots of custom code.
	/// Need a test with SDL + eglCreateImageKHR
	
	const EGLImage image = eglCreateImageKHR( egl_display,
                                              EGL_NO_CONTEXT,
                                              EGL_LINUX_DMA_BUF_EXT,
                                              NULL,
                                              attribs);
                         
    ///
    //~ const EGLint pbuf_attribs[] = {
                    //~ EGL_WIDTH, 1280,
                    //~ EGL_HEIGHT, 720,
                    //~ EGL_LINUX_DRM_FOURCC_EXT, DRM_FORMAT_XRGB8888,  /// takes 16 or 32 bits per pixel (or 8 probably)
					//~ EGL_DMA_BUF_PLANE0_FD_EXT, TEST_fd,
					//~ EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
					//~ EGL_DMA_BUF_PLANE0_PITCH_EXT, 1280*4,
                    //~ EGL_NONE};
   	//~ const EGLImage image = eglCreateImageKHR( egl_display,
                                              //~ EGL_NO_CONTEXT,
                                              //~ EGL_LINUX_DMA_BUF_EXT,
                                              //~ NULL,
                                              //~ pbuf_attribs);                 
                    
	/// Success reported here!

	if (!image) {	///or EGL_NO_IMAGE_KHR
		printf("Failed to create EGLImage\n");
		return -1;
	}
	return 1;
	glGenTextures(1, &texture);
	glEnable(GL_TEXTURE_EXTERNAL_OES);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);
	checkGl();
	new_texture = texture;
	eglDestroyImageKHR(&egl_display, image);
	return 1;

	/// his
	//~ glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);
	//~ uint8_t uv_default[] = {0x7f, 0x7f};
	//~ glTexImage2D(GL_TEXTURE_EXTERNAL_OES, 0, GL_RGBA, 1280, 720, 0, GL_RGBA, GL_UNSIGNED_BYTE, uv_default);
	//~ glGetUniformLocation(program, "texture");
	
	//glEnable(GL_TEXTURE_EXTERNAL_OES);
	//glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
	//glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexImage2D(GL_TEXTURE_EXTERNAL_OES, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);
	/// Success reported here!
	
	
	
	
	//glTexImage2D(GL_TEXTURE_EXTERNAL_OES, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	new_texture = texture;
	
	glGetUniformLocation(program, "texture");
	eglDestroyImageKHR(&egl_display, image);
	

	/*
	glGenTextures(1, &texture);
	glEnable(GL_TEXTURE_EXTERNAL_OES);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	uint8_t uv_default[] = {0x7f, 0x7f};
	glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);
	//NOT glTexSubImage2D(GL_TEXTURE_EXTERNAL_OES, 0, 0,0,1,1,GL_RGBA, GL_UNSIGNED_BYTE, nullptr); // last = uv_default ?? or nullptr
	//NOT glTexImage2D(GL_TEXTURE_EXTERNAL_OES, 0, GL_RGBA, 1, 1, 0, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, nullptr);
	
	glGetUniformLocation(program, "texture");
	
	// This one needed?
	eglDestroyImageKHR(&egl_display, image);
	*/
	//printf("Ran through NanoSdlWindow::UpdateAVFrame\n");
}


void NanoSdlWindow::RefreshScreenDraw()
{	
	ImGuiIO &imio = ImGui::GetIO(); //(void)_imio;
	imio.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     /// Enable Keyboard Controls
	imio.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      /// Enable Gamepad Controls
	
	ImGui::Render();
	glViewport(0, 0, (int)imio.DisplaySize.x, (int)imio.DisplaySize.y);
	
	glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
	//glClear(GL_COLOR_BUFFER_BIT);// remove when playing video!
	
	/// Draw video frame
	//~ glBindTexture(GL_TEXTURE_EXTERNAL_OES, da->texture);
    //~ glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    //~ eglSwapBuffers(de->setup.egl_dpy, de->setup.surf);
	//glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
	glUseProgram(program);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	//glDeleteTextures(1, &texture); // needed? slows down mebbe
	
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
		
		//SDL_MinimizeWindow(sdl_window);

		/// transfer drm_fd
		SDL_SysWMinfo WMinfo;
		SDL_VERSION(&WMinfo.version);
		SDL_GetWindowWMInfo(sdl_window, &WMinfo);
		printf("SDL drm_fd:  %d\n", WMinfo.info.kmsdrm.drm_fd);
		io->drm_fd = WMinfo.info.kmsdrm.drm_fd;
		
		
		ret = io->InitFFmpeg();
		InitVideoGl();
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


