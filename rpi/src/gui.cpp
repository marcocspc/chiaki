#include <iostream>
#include <stdio.h>
#include <dirent.h>		/// list files
#include <stdint.h>
#include <assert.h>
#include <unistd.h>		/// sleep()
#include <algorithm>	/// min,max
#include <vector>

#include "rpi/gui.h"
#include "rpi/host.h"
#include "rpi/io.h"
#include "rpi/settings.h"
#define STB_IMAGE_IMPLEMENTATION
#include "rpi/stb_image.h"	/// has to be here

#include <drm_fourcc.h>


int RemapChi(int x, int in_min, int in_max, int out_min, int out_max)
{	
	int out=0;
	out =  (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
	/// need to clamp as above over shoots
	out = std::max(out, out_min);
	out = std::min(out, out_max);
	
	return out;
}

std::vector<std::string> GetFiles(const char* folderPath, const char* match)
{	
	std::vector<std::string>  outFileNames;
	struct dirent *de;  // Pointer for directory entry
  
    /// opendir() returns a pointer of DIR type. 
    DIR *dr = opendir(folderPath);
  
    if (dr == NULL)
    {
        printf("Could not open directory: %s\n", folderPath );
        return outFileNames;
    }
  
    while ((de = readdir(dr)) != NULL)
    {
            /// check if matches
            int found = -1;
            std::string tmp_s;
            std::string s; s=de->d_name;
            if((found = s.find(std::string(match))) > -1)
			{
				tmp_s = s.substr(0, s.find(".")); 
				///printf("%s\n", tmp_s.c_str());
				outFileNames.push_back(tmp_s);
			}
            
	}
  
    closedir(dr);
    return outFileNames;
}


/// The text blurbs for the info panel.
const char* ChiakiGetHelpText(int n)
{
	const char* text = "\nWelcome to Chiaki!\n\n\nSpecial Combo's:\n  R3 + Circle = Quit Play Session\n  R3 + Square = Screen grab\n\nHotkeys:\n  F11 = Toggle Fullscreen";
	if(n==1)
		text = "VIDEO DECODER:\n\nThe software that decodes the video stream. The actual decode happens on the Pi's hardware but this decides which API to use.\n\nCurrently only v4l2 is available and this is used through ffmpeg";
	if(n==2)
		text = "VIDEO CODEC:\n\nChoose which video compression format to request from your Playstation.\n\nh264 is older and a very common standard which works on PS4 upwards.\n\nh265 is a newer format that you can use with the PS5. It is more efficient when streaming high resolution video and also hopefully will support HDR video at some point in the future.\n\n\n\"Automatic\" will request h265 for PS5s.";
	if(n==3)
		text = "VIDEO RESOLUTION:\n\nYou can probably guess what this one does!\n\n1080p is only available for PS4-Pro and upwards.\n\nIf 1080 is selected and you have a PS4 it will automatically step down to 720p for the session.\n\nIf you have limited network bandwidth try a lower resolution.";
	if(n==4)
		text = "FRAME RATE:\n\nYou can request your playstation to send a 30 or 60 fps video stream.\n\nIf you have limited network bandwidth you can try the lower fps.";
	if(n==5)
		text = "AUDIO OUT:\n\nThis popup should list all the Audio Out devices that the program believes that your Raspberry currently has available.\n\nThe names might not look like you are used to but that's how the library I'm using for this sees them.";
		
	return text;
}


/// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
    /// Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    /// Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    /// Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    /// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}

/// EGL error check
void checkEgl() {
	int err;        
	if((err = eglGetError()) != EGL_SUCCESS){
		printf("EGL error! error = %x\n", err);
	} 
}

/// GL Error Check
void checkGl(){
	GLenum err;
	while ( ( err = glGetError() ) != GL_NO_ERROR) {
		std::cerr << err;        
	}
}

/// Test "texture", Use GL_LUMINANCE for type
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
    { {0.0, 1.0}, {1.0, 1.0}, {0.0, 0.0}, {1.0, 0.0} }
};

// OpenGL shader setup
#define DECLARE_YUV2RGB_MATRIX_GLSL \
	"const mat4 yuv2rgb = mat4(\n" \
	"    vec4(  1.1644,  1.1644,  1.1644,  0.0000 ),\n" \
	"    vec4(  0.0000, -0.2132,  2.1124,  0.0000 ),\n" \
	"    vec4(  1.7927, -0.5329,  0.0000,  0.0000 ),\n" \
	"    vec4( -0.9729,  0.3015, -1.1334,  1.0000 ));"

static const GLchar* vertex_shader_source =
	"#version 100\n"
	"attribute vec3 position;\n"
	"attribute vec2 tx_coords;\n"
	"varying vec2 v_texCoord;\n"
	"void main() {  \n"
	" gl_Position = vec4(position, 1.0);\n"
	" v_texCoord = tx_coords;\n"
	"}\n";
	
static const GLchar* fragment_shader_source =
	"#version 100\n"
	"#extension GL_OES_EGL_image_external : require\n"
	"precision mediump float;\n"
	"uniform samplerExternalOES texture;\n"
	"//uniform sampler2D texture;\n"
	"varying vec2 v_texCoord;\n"
	"void main() {	\n"
	"	gl_FragColor = texture2D( texture, v_texCoord );\n"
	"	//out_color = vec4(v_texCoord.x,v_texCoord.y,0,1);\n"
	"}\n";

static const GLchar* bg_shader_source =
	"#version 100\n"
	"precision mediump float;\n"
	"uniform sampler2D texture;\n"
	"varying vec2 v_texCoord;\n"
	"void main() {	\n"
	"	gl_FragColor = texture2D( texture, v_texCoord );\n"
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

ImguiSdlWindow::ImguiSdlWindow(char pathbuf[], SDL_Window* pwindow, int rwidth, int rheight, SDL_Renderer* renderer, SDL_GLContext* gl_context)
{
	sdl_window = pwindow;
	sdl_renderer = renderer;
	gl_ctx = gl_context;
	
	if(std::strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0 ||  std::strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0 )///0 is match
		 IsX11 = true;
	else IsX11 = false;
	printf("Running under X11: %d\n", IsX11);
	
	/// establish user home dir
	home_dir = std::string("/home/");
	home_dir.append(getenv("USER"));

			
	/// not sure where the best place for this is?
	host = new Host;
	io = new IO;
	host->gui = this;
	io->Init(host);
	host->Init(io);
	io->InitGamepads();
	
	main_config = home_dir;
	main_config.append("/.config/Chiaki/Chiaki_rpi.conf");
	settings = new RpiSettings();
	settings->all_read_settings = settings->ReadSettingsYaml(main_config);

	host->StartDiscoveryService();

	/// Imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	
	
	// settings options - THESE ARE THE ACTUAL SETTINGS SO DON'T CHANGE THE STRINGS.
	// -----------------------------------------------------------------
	decoder_options = {"automatic", "v4l2"};  /// doesn't like "-" symbols.
	sel_decoder = decoder_options[0];
	
	vcodec_options =  {"automatic", "h264", "h265"};
	sel_vcodec = vcodec_options[0];
	
	resolution_options = {"1080", "720", "540"};
	sel_resolution = resolution_options[1];
	
	framerate_options = {"60", "30"};
	sel_framerate = framerate_options[0];
	
	///audio_options = {"hdmi" ,"jack"};
	///audio_options = {"default"};
	// Audio logic is pretty rough, needs fixing up
	for(std::string out_dev : io->audio_out_devices){
		audio_options.push_back(out_dev);
	}
	sel_audio = audio_options[0];
	// -----------------------------------------------------------------

	/// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(sdl_window, *gl_ctx);
	///const char* glsl_version = "#version 300 es"; //was: 100
	const char* glsl_version = "#version 100";
	ImGui_ImplOpenGL3_Init(glsl_version);
	printf("GL_VERSION  : %s\n", glGetString(GL_VERSION) );
	printf("GL_RENDERER : %s\n", glGetString(GL_RENDERER) );

	resizeEvent(rwidth, rheight); ///kick a refresh

	/// Load Assets - Linux only path handling
	/// Linux Path eg:  /home/pi/dev/chiaki_testbuild/build/rpi/chiaki-rpi
	///
	std::string fullpath(pathbuf);
	std::vector<std::string> fullpathvec;
	std::string mainpath("");
	const char* p;
	for(p = strtok( pathbuf, "/" );  p;  p = strtok( NULL, "/" ))
	{
		///printf( "SPLIT:  %s\n", p );
		fullpathvec.push_back(p);
	}
	for(uint8_t i=0; i<(fullpathvec.size()-3); i++)
	{
		mainpath += "/";
		mainpath += fullpathvec.at(i);
	}
	mainpath += "/rpi/assets/";
	///printf("MAIN PATH:  %s\n", mainpath.c_str());

	ImGuiIO &imio = ImGui::GetIO();
	float size_pixels = 18;
	imgui_font = imio.Fonts->AddFontFromFileTTF(std::string(mainpath+"komika.slick.ttf").c_str(), size_pixels);
	bigger_font = imio.Fonts->AddFontFromFileTTF(std::string(mainpath+"komika.slick.ttf").c_str(), size_pixels*1.5);
	regist_font = imio.Fonts->AddFontFromFileTTF(std::string(mainpath+"Cousine-Regular.ttf").c_str(), size_pixels);

	/// Textures for buttons, bg etc
	int my_image_width = 0;
	int my_image_height = 0;
	std::string tx0 = mainpath + "unknown_01.png";
	bool imret = LoadTextureFromFile(tx0.c_str(), &gui_textures[0], &my_image_width, &my_image_height);
	IM_ASSERT(imret);

	std::string tx1 = mainpath + "ps4_01.png";
	imret = LoadTextureFromFile(tx1.c_str(), &gui_textures[1], &my_image_width, &my_image_height);
	IM_ASSERT(imret);

	std::string tx2 = mainpath + "ps4_orange_01.png";
	imret = LoadTextureFromFile(tx2.c_str(), &gui_textures[2], &my_image_width, &my_image_height);
	IM_ASSERT(imret);

	std::string tx3 = mainpath + "ps4_blue_01.png";
	imret = LoadTextureFromFile(tx3.c_str(), &gui_textures[3], &my_image_width, &my_image_height);
	IM_ASSERT(imret);

	std::string tx4 = mainpath + "ps5_01.png";
	imret = LoadTextureFromFile(tx4.c_str(), &gui_textures[4], &my_image_width, &my_image_height);
	IM_ASSERT(imret);

	std::string tx5 = mainpath + "ps5_orange_01.png";
	imret = LoadTextureFromFile(tx5.c_str(), &gui_textures[5], &my_image_width, &my_image_height);
	IM_ASSERT(imret);

	std::string tx6 = mainpath + "ps5_blue_01.png";
	imret = LoadTextureFromFile(tx6.c_str(), &gui_textures[6], &my_image_width, &my_image_height);
	IM_ASSERT(imret);


	SwitchHostImage(0); /// to 'unknown' as default start
	remote_texture = gui_textures[0];

	std::string tx7 = mainpath + "logo_01.png";
	LoadTextureFromFile(tx7.c_str(), &logo_texture, &logo_width, &logo_height);
	IM_ASSERT(imret);

	bg_program = common_get_shader_program(vertex_shader_source, bg_shader_source);
	std::string screengrab_name = home_dir;
	screengrab_name.append("/.config/Chiaki/screengrab.jpg");
	int ww, hh;
	LoadTextureFromFile(screengrab_name.c_str(), &bg_texture, &ww, &hh);
	IM_ASSERT(imret);

	/// use the first avalable remote in gui
	std::string path = std::string(home_dir + "/.config/Chiaki");
	const char* match = ".remote";
	remoteHostFiles = GetFiles(path.c_str(), match);
	if(remoteHostFiles.size() > 0) {
		sel_remote = remoteHostFiles.at(0);
		std::string remoteFile = home_dir;
		remoteFile.append(std::string("/.config/Chiaki/") + sel_remote + std::string(".remote"));
		RpiSettings tmpsettings;
		std::vector<rpi_settings_host> tmpRemoteSettings;
		tmpRemoteSettings = tmpsettings.ReadSettingsYaml(remoteFile);
		host->current_remote_settings = tmpRemoteSettings.at(0);
	} else
		sel_remote = std::string("no remotes");
	
	
	remoteIp[0]=0;
	remoteIp[1]=0;
	remoteIp[2]=0;
	remoteIp[3]=0;
	/// protect for old settings files
	std::vector<std::string> ipvec = StringIpToVector(host->current_remote_settings.remote_ip);
	if(ipvec.size() != 0 && ipvec.at(0) != "0" && ipvec.at(1) != "0") {
		remoteIp[0]=stoi(ipvec.at(0));
		remoteIp[1]=stoi(ipvec.at(1));
		remoteIp[2]=stoi(ipvec.at(2));
		remoteIp[3]=stoi(ipvec.at(3));
	}
	
	time(&timestamp_sec_a); 
	timestamp_sec_a -= 11; /// substract 11 sec to enable at startup
	
	//InitVideoGl(); // not sure if I should keep here
	SwitchSettings(20);		/// Set to Local settings first instance [0]
	

	printf("Gui init done\n");
}


ImguiSdlWindow::~ImguiSdlWindow()
{
	printf("ImguiSdlWindow destroyed\n");
}

/// Also switched the Help text
void ImguiSdlWindow::SettingsDraw(int widgetID, const char* label, std::vector<std::string> list, std::string &select)
{
	
	ImGui::Dummy(ImVec2(settingIndent, 10));
	ImGui::SameLine();
	ImGui::Text(label);
	ImGui::SameLine(settingIndent+130);
	ImVec2 button_sz(200, 30);
	
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(button_col));
	int nCols = 1;
	
	ImGui::PushID(widgetID); /// needed to differentiate same label buttons("automatic")
		/// try popup modal instead for bigger popup?
		if(ImGui::Button(select.c_str(), button_sz)){
			
			ImGui::OpenPopup(label);
		}
		
		if (ImGui::BeginPopup(label))
		{   
			for (uint8_t i = 0; i < list.size(); i++)
				if (ImGui::Selectable(list.at(i).c_str() )){
					select = list.at(i);
					// need to refresh settings in memory (which also writes to file)
					ChangeSettingAction(widgetID, select);
				}
			ImGui::EndPopup();
		}
	ImGui::PopID();
	ImGui::PopStyleColor(nCols);
	
	if (ImGui::IsItemHovered()) SwitchHelpText(label);
}

///
void ImguiSdlWindow::SwitchHostImage(int which)
{	
	current_host_texture = gui_textures[which];
}


void ImguiSdlWindow::SwitchRemoteImage(std::string state, int isPS5)
{
	if(isPS5) {
		
		if(state == std::string("standby"))
			remote_texture = gui_textures[5];
		if(state == std::string("ready"))
			remote_texture = gui_textures[6];
		
	} else {
		
		if(state == std::string("standby"))
			remote_texture = gui_textures[2];
		if(state == std::string("ready"))
			remote_texture = gui_textures[3];
	}	
}


/// Maybe use the new id variable instead?
void ImguiSdlWindow::SwitchHelpText(const char* label)
{
	if(strcmp(label, "Video Decoder")==0){
		help_text_n = 1;
		return;
	}
	if(strcmp(label, "Video Codec")==0){
		help_text_n = 2;
		return;
	}
	if(strcmp(label, "Resolution")==0){
		help_text_n = 3;
		return;
	}
	if(strcmp(label, "Framerate")==0){
		help_text_n = 4;
		return;
	}
	if(strcmp(label, "Audio Out")==0){
		help_text_n = 5;
		return;
	}	
}


void ImguiSdlWindow::RefreshReadBg()
{
	std::string screengrab_name = home_dir;
	screengrab_name.append("/.config/Chiaki/screengrab.jpg");
	int ww, hh;
	bool imret = LoadTextureFromFile(screengrab_name.c_str(), &bg_texture, &ww, &hh);
	IM_ASSERT(imret);
}


/// triggers when host an icon is hovered/focused
// must trigger gui refresh
void ImguiSdlWindow::SwitchSettings(int id)
{	
	/// do something only on change
	if(currentSettingsId != id) {
		currentSettingsId = id;
		
		if(currentSettingsId == 20) {	/// local host 0
			if(settings->all_validated_settings.size() > 0 ) {
				gui_settings_ptr = &settings->all_validated_settings.at(0);
				UpdateSettingsGui();
			} else gui_settings_ptr = nullptr;
			///printf("Switched to Local Settings\n");
		}
		
		if(currentSettingsId == 21) {	/// remote host 0
			gui_settings_ptr = &host->current_remote_settings;
			UpdateSettingsGui();
			///printf("Switched to Remote Settings\n");
		}		
	}	
	///printf("apa");
}


/// changes what's seen in the gui
void ImguiSdlWindow::UpdateSettingsGui()
{	
	if(gui_settings_ptr == nullptr){
		printf("Warning: empty gui_settings_ptr\n");
		return;
	}
		
	sel_decoder = gui_settings_ptr->sess.decoder;
	sel_vcodec = gui_settings_ptr->sess.codec;
	sel_resolution = gui_settings_ptr->sess.resolution;
	sel_framerate = gui_settings_ptr->sess.fps;
	sel_audio = gui_settings_ptr->sess.audio_device;
}

/// triggers on each settings gui change - Writes to settings file
///decoder, codec, resolution, fps, audio 
void ImguiSdlWindow::ChangeSettingAction(int widgetID, std::string choice)
{	
	std::string setting;
	
	if(gui_settings_ptr == nullptr){
		printf("Warning: empty gui_settings_ptr\n");
		return;
	}
	
	if(widgetID == 1)
	for(std::string ch : decoder_options) {
		if(choice == ch) {
			setting = "decoder";
			gui_settings_ptr->sess.decoder = choice;
			goto refresh;
		}
	}

	if(widgetID == 2)
	for(std::string ch : vcodec_options) {
		if(choice == ch) {
			setting = "codec";
			gui_settings_ptr->sess.codec = choice;
			goto refresh;
		}
	}
	
	if(widgetID == 3)
	for(std::string ch : resolution_options) {
		if(choice == ch) {
			setting = "resolution";
			gui_settings_ptr->sess.resolution = choice;
			goto refresh;
		}
	}
	
	if(widgetID == 4)
	for(std::string ch : framerate_options) {
		if(choice == ch) {
			setting = "fps";
			gui_settings_ptr->sess.fps = choice;
			goto refresh;
		}
	}
	
	if(widgetID == 5)
	for(std::string ch : audio_options) {
		if(choice == ch) {
			setting = "audio";
			gui_settings_ptr->sess.audio_device = choice;
			goto refresh;
		}
	}	
	printf("No matching settings found\n");
	return;
	

refresh:
	std::string config_file;
	if(currentSettingsId == 20) {	/// is Local
		//settings->all_validated_settings.at(0) = *gui_settings_ptr;//what bug was caused?
		config_file = main_config;
	} else {
		//host->current_remote_settings = *gui_settings_ptr;//
		config_file = home_dir;
		config_file.append(std::string("/.config/Chiaki/") + sel_remote + std::string(".remote"));
	}
	
	settings->RefreshSettings(*gui_settings_ptr, setting, choice, config_file);
}


std::string ImguiSdlWindow::GetIpAddr()
{
	std::string out_ip("0.0.0.0");
	out_ip = (std::to_string(remoteIp[0]) +"."+ std::to_string(remoteIp[1]) +"."+std::to_string(remoteIp[2]) +"."+std::to_string(remoteIp[3]));
	
	return out_ip;
}



/// This is the GUI main loop
///
// Need to fix this weirdo logic overlap with session.
bool ImguiSdlWindow::start()
{	
	ImGuiStyle &style = ImGui::GetStyle();
	style.WindowRounding = 0.0f;///8
	style.FrameRounding = 0.0f;///2
	style.GrabRounding = style.FrameRounding;
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(button_col));

	// Main loop
	while (!done)
	{	
		
		/// Wait for a fresh decoded frame
		bool got_frame = false;
		while(!got_frame && !guiActive)
		{	
			usleep(2000);  /// 2000 should be 2ms
			/// maybe generate fresh frame texture for GL
			if(io->nextFrameCount > prevFrameCount && !guiActive){
				frame = io->frames_list.GetCurrentFrame();
				UpdateAVFrame();
				prevFrameCount = io->nextFrameCount;
				got_frame = true;
			}
		}
		
		
		if(guiActive)
			HandleSDLEvents();

		if(guiActive)
			CreateImguiWidgets();

		/// GL render
		if(guiActive || IsX11) {
			RefreshScreenDraw();
		} else if(clear_counter < 2) {
			/// flushed out the last gui frame from kmsdrm framebuffer for black bg
			RefreshScreenDraw();
			clear_counter++;
		}
		
	}
	
	return true;
}



void ImguiSdlWindow::CreateImguiWidgets()
{			
			time_t timestamp_sec_b;
			time(&timestamp_sec_b);
			long unsigned int timelapsed = timestamp_sec_b - timestamp_sec_a;
			///printf("Lapsed:  %ld\n", timestamp_sec_a);
			
	
			// try move these out
			ImGuiIO &imio = ImGui::GetIO();
			imio.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     /// Enable Keyboard Controls
			imio.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      /// Enable Gamepad Controls
			
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();
			
			ImGuiWindowFlags window_flags = 0;
			window_flags |= ImGuiWindowFlags_NoTitleBar;
			window_flags |= ImGuiWindowFlags_NoCollapse;
			///window_flags |= ImGuiWindowFlags_NoBackground;
			window_flags |= ImGuiWindowFlags_NoResize;
			window_flags |= ImGuiWindowFlags_NoMouseInputs;
						
			dspszX = imio.DisplaySize.x;
			dspszY = imio.DisplaySize.y;
			float botHgt = 0.5;  /// fraction of total height
			int brdr = 20;
			int subPanelSzX = (dspszX*0.9) / 3;
			int subPanelSzYtop = (dspszY*0.9) * (1.0-botHgt) - 20;  /// the -20 is fudge
			int subPanelSzYbot = (dspszY*0.9) * botHgt;
			int center_win_alpha = 64;
			
			/// BG Window for textured backdrop
			if(bg_texture != 0) {
				center_win_alpha = 128;
				ImGui::SetNextWindowSize(ImVec2((int)dspszX+8, (int)dspszY+8), ImGuiCond_Always);
				ImGui::SetNextWindowPos(ImVec2((int)-4, (int)-4), ImGuiCond_Always);
				bool* b_open = new bool;
				ImGui::Begin("BG Window", b_open, window_flags|ImGuiWindowFlags_NoBringToFrontOnFocus|ImGuiWindowFlags_NoMouseInputs);
				ImDrawList* draw_listA = ImGui::GetWindowDrawList();
				draw_listA->AddImage((void*)(intptr_t)bg_texture, ImVec2(0, 0), ImVec2(dspszX, dspszY) );
				ImGui::End();
			}
			
			ImGui::SetNextWindowSize(ImVec2((int)imio.DisplaySize.x*0.9, (int)imio.DisplaySize.y*0.9), ImGuiCond_Always);
			ImGui::SetNextWindowPos(ImVec2((int)imio.DisplaySize.x*0.05, (int)imio.DisplaySize.y*0.05), ImGuiCond_Always);
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(ImColor((int)clear_color.x, (int)clear_color.y, (int)clear_color.z, center_win_alpha)));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(ImColor((int)clear_color.x, (int)clear_color.y, (int)clear_color.z, 0)));

			bool* p_open = new bool;
			if (ImGui::Begin("Parent Window", p_open, window_flags) )
			{
				ImGui::PopStyleColor(2); /// num colors pushed
					
				/// Panels For Playstation Hosts + Quit
				ImGui::BeginChild("UpperHalfPanel", ImVec2(subPanelSzX*3, subPanelSzYtop), false, ImGuiWindowFlags_NavFlattened);
				
					ImGui::BeginChild("A", ImVec2(subPanelSzX, subPanelSzYtop), false, ImGuiWindowFlags_NavFlattened);
						//ImGui::Dummy(ImVec2(0, (subPanelSzYtop/2)-(20/2)) );  /// add offset space at top
						
						/// Gui for Remote connections
						///	
						ImGui::BeginGroup();
						ImGui::Indent(40);
						ImGui::Dummy(ImVec2(0, subPanelSzYtop/3) );
						ImGui::PushItemWidth(160.0f);   
						ImGui::InputInt4(" ", remoteIp);
						ImGui::PopItemWidth();

						const char* lbl="RemoteHost";
						if(ImGui::Button(sel_remote.c_str(), ImVec2(160, 30) )){
			
							ImGui::OpenPopup(lbl);
						}
						if(remoteHostFiles.size() > 0)
						{
							if (ImGui::BeginPopup(lbl))
							{   
									for (uint8_t i = 0; i < remoteHostFiles.size(); i++)
										if (ImGui::Selectable(remoteHostFiles.at(i).c_str() )){
											sel_remote = remoteHostFiles.at(i);
											
											std::string remoteFile = home_dir;
											remoteFile.append(std::string("/.config/Chiaki/") + sel_remote + std::string(".remote"));
											RpiSettings tmpsettings;
											std::vector<rpi_settings_host> tmpRemoteSettings;
											tmpRemoteSettings = tmpsettings.ReadSettingsYaml(remoteFile);
											
											host->current_remote_settings = tmpRemoteSettings.at(0);
											
											/// Refresh IP int fields gui
											std::vector<std::string> ipvec = StringIpToVector(host->current_remote_settings.remote_ip);
											if(ipvec.size() != 0) {
												remoteIp[0]=stoi(ipvec.at(0));
												remoteIp[1]=stoi(ipvec.at(1));
												remoteIp[2]=stoi(ipvec.at(2));
												remoteIp[3]=stoi(ipvec.at(3));
											}
											/// revert
											host->discoveredRemoteMatched = false;
											//currentSettingsId = 20; /// back to local
											//UpdateSettingsGui();	/// back to local
										}
									ImGui::EndPopup();
							}
						}
						
						if(timelapsed > 10) {
							if (ImGui::Button("Find Remote", ImVec2(160, 30))) {
									
									host->discoveredRemoteMatched = false;
									
									if(sel_remote != "no remotes") {
										host->DiscoverRemote();
										
										/// all this to save current gui IP
										host->current_remote_settings.remote_ip = GetIpAddr();
										RpiSettings tmpsettings;
										std::vector<rpi_settings_host> tmpRemoteSettings;
										tmpRemoteSettings.push_back(host->current_remote_settings);
										std::string remoteFile = home_dir;
										remoteFile.append(std::string("/.config/Chiaki/") + sel_remote + std::string(".remote"));
										tmpsettings.WriteYaml(tmpRemoteSettings, remoteFile);
									}
							}
						} else {
							std::string waittxt("    Please Wait ");
							waittxt.append(std::to_string(10-timelapsed));
							ImGui::Text(waittxt.c_str());
						}
						ImGui::EndGroup();
						
						ImGui::SameLine();
						ImGui::BeginGroup();
						ImGui::Dummy(ImVec2(0, (subPanelSzYtop/3)-10) );
						if(host->discoveredRemoteMatched) {
							if (ImGui::ImageButton((void*)(intptr_t)remote_texture, ImVec2(120, 120), ImVec2(0, 0), ImVec2(1, 1), 2) )    // <0 frame_padding uses default frame padding settings. 0 for no padding
							{
								if(host->remote_state == std::string("standby"))
								{
									host->WakeupRemote();
									timestamp_sec_a = timestamp_sec_b;
								}
								if(host->remote_state == std::string("ready"))
								{
									SDL_SysWMinfo WMinfo;
									SDL_VERSION(&WMinfo.version);
									SDL_GetWindowWMInfo(sdl_window, &WMinfo);
									printf("SDL drm_fd:  %d\n", WMinfo.info.kmsdrm.drm_fd);
									io->drm_fd = WMinfo.info.kmsdrm.drm_fd;
									
									host->current_remote_settings.remote_ip = GetIpAddr();
									host->session_settings = host->current_remote_settings;
									
									/// session startup
									io->InitFFmpeg();
									if(IsX11) InitVideoGl();
									host->StartSession();
									guiActive = 0;
									io->SwitchInputReceiver(std::string("session"));
									setClientState("playing");
								}
							}
							if (ImGui::IsItemHovered()) SwitchSettings(21);
							
						}
						ImGui::EndGroup();
					ImGui::EndChild();
					
					/// Local Playstation host
					ImGui::SameLine();
					ImGui::BeginChild("B", ImVec2(subPanelSzX, subPanelSzYtop), false, ImGuiWindowFlags_NavFlattened);
						ImGui::Dummy(ImVec2(0, (subPanelSzYtop/2)-(psBtnSz/2)) );  /// add offset space at top
						ImGui::Indent(subPanelSzX/2 - (int)(psBtnSz/2));							
						///ImGui::ImageButton(ImTextureID my_image_texture, ImVec2(256, 256), const ImVec2& uv0 = ImVec2(0, 0),  const ImVec2& uv1 = ImVec2(1,1), int frame_padding = -1, const ImVec4& bg_col = ImVec4(0,0,0,0), const ImVec4& tint_col = ImVec4(1,1,1,1));    // <0 frame_padding uses default frame padding settings. 0 for no padding
						if (ImGui::ImageButton((void*)(intptr_t)current_host_texture, ImVec2(psBtnSz, psBtnSz), ImVec2(0, 0), ImVec2(1, 1), 2) )    // <0 frame_padding uses default frame padding settings. 0 for no padding
						{
							hostClick();
						}
						if (ImGui::IsItemHovered()) SwitchSettings(20);
						ImGui::Text(client_state.c_str());
					ImGui::EndChild();

					/// Quit button
					ImGui::SameLine();
					ImGui::BeginChild("C", ImVec2(subPanelSzX, subPanelSzYtop), false, ImGuiWindowFlags_NavFlattened);
						ImGui::Dummy(ImVec2(0, (subPanelSzYtop/2)-(20/2)) );  /// add offset space at top
						ImGui::Indent(subPanelSzX/2 - (int)(80/2));
						if (ImGui::Button("Quit", ImVec2(80, 40))) {
							guiActive = 0;
							SDL_VideoQuit();
							SDL_AudioQuit();
							SDL_Quit();
							exit(0);
						}
					ImGui::EndChild();

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
						
						ImGui::PushFont(regist_font);
						ImGui::InputText(" Account Id", c_regist_acc_id, IM_ARRAYSIZE(c_regist_acc_id));
						ImGui::Text(" ");
						ImGui::PopFont();
						
						ImGui::PushFont(regist_font);
						ImGui::InputText(" Pin", c_regist_pin, IM_ARRAYSIZE(c_regist_pin));
						ImGui::Text(" ");
						ImGui::PopFont();

						if (ImGui::Button("Register", ImVec2(120, 0))) {
							
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

				ImGui::BeginChild("LowerHalfPanel", ImVec2(ImGui::GetWindowSize().x, subPanelSzYbot), false, ImGuiWindowFlags_NavFlattened);
									
					
					ImDrawList* draw_list = ImGui::GetWindowDrawList();
					
					ImGui::BeginChild("LogoPanel", ImVec2(subPanelSzX, subPanelSzYbot), false, ImGuiWindowFlags_NavFlattened);
						draw_list->AddRectFilled(ImVec2(dspszX*0.05+brdr, dspszY*(1.0-botHgt)), ImVec2(dspszX*0.05+subPanelSzX-brdr, dspszY*0.05 + dspszY*0.9 - brdr), IM_COL32(200, 255, 64, 255), 8.0);
						float logo_scaled_height = ((float)logo_height/(float)logo_width) * subPanelSzX;
						draw_list->AddImage((void*)(intptr_t)logo_texture, ImVec2(dspszX*0.05+brdr+10, dspszY*0.05+subPanelSzYtop+(subPanelSzYbot-logo_scaled_height)), ImVec2(dspszX*0.05+subPanelSzX-brdr-10, dspszY*0.05+dspszY*0.9-brdr-10) );
					ImGui::EndChild();
					
					ImGui::SameLine();
					
					/// Settings popup buttons
					ImGui::BeginChild("SettingsPanel", ImVec2(subPanelSzX, subPanelSzYbot), false, ImGuiWindowFlags_NavFlattened);
						
						ImGui::Indent(subPanelSzX/4);
						ImGui::PushFont(bigger_font);
						if(currentSettingsId == 20)
							ImGui::Text("LOCAL Settings");
						else ImGui::Text("REMOTE Settings");
						ImGui::PopFont();
						ImGui::Indent(-subPanelSzX/4);
						
						ImGui::Dummy(ImVec2(100, 30));
						
						SettingsDraw(1, "Video Decoder", decoder_options, sel_decoder);

						SettingsDraw(2, "Video Codec", vcodec_options, sel_vcodec);

						SettingsDraw(3, "Resolution", resolution_options, sel_resolution);

						SettingsDraw(4, "Framerate", framerate_options, sel_framerate);

						SettingsDraw(5, "Audio Out", audio_options, sel_audio);
					ImGui::EndChild();
					
					ImGui::SameLine();
					
					/// Text Info Feedback
					ImGui::BeginChild("InfoPanel", ImVec2(subPanelSzX, subPanelSzYbot), false, ImGuiWindowFlags_NavFlattened);
						int wrap_width = (dspszX*0.95-brdr-10) - (dspszX*0.05+subPanelSzX*2+brdr); /// from coords below
						ImGui::PushTextWrapPos(wrap_width); /// is width, not coordinate
						ImGui::Dummy(ImVec2(100, 10));
						ImGui::Indent(10);
						ImGui::Text(ChiakiGetHelpText(help_text_n));
						ImGui::PopTextWrapPos();
						/// 2d point position corners, in window(?) space. Clipped by edges of parent.
						draw_list->AddRectFilled(ImVec2(dspszX*0.05+subPanelSzX*2+brdr, dspszY*(1.0-botHgt)), ImVec2(dspszX*0.95-brdr, dspszY*0.05 + dspszY*0.9 - brdr), IM_COL32(5, 5, 5, 85), 8.0);
					ImGui::EndChild();
					
				ImGui::EndChild();
			}/// END parent window
			ImGui::End();
}


/// For during Gui only - see IO for session SDL event handling
void ImguiSdlWindow::HandleSDLEvents()
{
	SDL_Event event;

	if(takeInput)///takeInput
	{
		/// pulling events from the event queue
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			
			switch (event.type)
			{
				case SDL_QUIT:
					guiActive = 0;
					SDL_VideoQuit();
					SDL_AudioQuit();
					SDL_Quit();
					exit(0);
					break;
					
				///case SDL_CONTROLLERBUTTONUP:
				case SDL_CONTROLLERBUTTONDOWN:
				{	
					// Not needed here ChiakiControllerState press = io->GetState();	
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
						guiActive = 0;
						SDL_VideoQuit();
						SDL_AudioQuit();
						SDL_Quit();
						exit(0);
						break;
					  case SDLK_F11:
						fullscreen = !fullscreen;
						if (fullscreen)
						{
						  SDL_SetWindowFullscreen(sdl_window, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP);
						  //SDL_SetWindowSize(sdl_window, 1920, 1080);
						}
						else
						{
						  SDL_SetWindowFullscreen(sdl_window, SDL_WINDOW_OPENGL);
						}
						break;
						
					  default:
						break;
					}
					break;
				}
				
				case SDL_WINDOWEVENT:
				{
					if (event.window.event == SDL_WINDOWEVENT_RESIZED)
					{
						resizeEvent(event.window.data1, event.window.data2);
					}
					break;
				}

				default:
					break;
			}/// END switch:
		}/// END while (SDL_PollEvent
	}/// END  if(takeInput)

}


void ImguiSdlWindow::RefreshScreenDraw()
{
	glViewport(glViewGeom[0], glViewGeom[1], glViewGeom[2], glViewGeom[3]);

	if(guiActive)
		ImGui::Render();
	
	if(guiActive)
		glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
	else
		glClearColor(0,0,0,0);

	glClear(GL_COLOR_BUFFER_BIT);

	///glUseProgram(program);//not needed
	if(!guiActive)
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	if(guiActive)
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	///SDL_GL_SwapWindow(sdl_window);	// use below one?
	SDL_RenderPresent(sdl_renderer);	///Waits for vsync, use for gl video 
}

// Now using glDmaTexture
int ImguiSdlWindow::InitVideoGl()
{
	GLuint vbo;
	GLint pos;
	GLint uvs;
	
	/// Use v-sync=(1)
	SDL_GL_SetSwapInterval(1);
	
	/// Disable depth test and face culling.
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

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

    /// Texture
	glEnable(GL_TEXTURE_EXTERNAL_OES);
	glHint(GL_GENERATE_MIPMAP_HINT, GL_FASTEST);
	glGenTextures(1, &texture);
	glUseProgram(program);
	///glGetUniformLocation(program, "texture");
	
	SDL_SysWMinfo WMinfo;
	SDL_VERSION(&WMinfo.version);
	SDL_GetWindowWMInfo(sdl_window, &WMinfo);
	EGLNativeDisplayType hwnd = WMinfo.info.x11.display;  /// WMinfo.info.kmsdrm.drm_fd, WMinfo.info.x11.display
	egl_display = eglGetDisplay(hwnd);

	return 1;
}

/// Has to be run from the 'main' thread, currently here from ImguiSdlWindow
/// Running from play thread causes Gl context problems
bool ImguiSdlWindow::UpdateAVFrame()
{	
	
	// perf test
	//return true; // Still 17-18% CPU!
					
	if(frame->format != AV_PIX_FMT_DRM_PRIME)
	{	
		printf("UpdateFromGUI: Not a AV_PIX_FMT_DRM_PRIME AVFrame, %d\n", frame->format);
		return false;
	}
	
	
	// TRY do only on first frame - but needs valid frame->data
	//if(!planes_init_done)
	//{
		const AVDRMFrameDescriptor *desc = (AVDRMFrameDescriptor*)frame->data[0];
		//~ EGLint attribs[50];
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
		 
		 //planes_init_done = true;
	 //}
	
	

	
	const EGLImage image = eglCreateImageKHR( egl_display,
                                              EGL_NO_CONTEXT,
                                              EGL_LINUX_DMA_BUF_EXT,
                                              NULL, attribs);
	if (!image) {
		printf("Failed to create EGLImage\n");
		return -1;
	}

	glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, nullptr);
		
	return true;
}

void ImguiSdlWindow::registClick(std::string acc_id, std::string pin)
{	
	host->RegistStart(acc_id, pin);
}

int ImguiSdlWindow::hostClick()
{
	printf("Host Click\n");
	
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
	
	if(client_state == "standby") /// orange
	{
		host->SendWakeup();
		setClientState("waiting");
		return 0;
	}
	
	if(client_state == "ready") /// blue
	{
		printf("Starting Play session!\n");
		
		/// copy over settings
		host->session_settings = settings->all_validated_settings.at(0);
		host->session_settings.remote_ip = host->discoveredHosts->host_addr;
		
		/// transfer drm_fd, only used for CLI/non-x11
		SDL_SysWMinfo WMinfo;
		SDL_VERSION(&WMinfo.version);
		SDL_GetWindowWMInfo(sdl_window, &WMinfo);
		printf("SDL drm_fd:  %d\n", WMinfo.info.kmsdrm.drm_fd);
		io->drm_fd = WMinfo.info.kmsdrm.drm_fd;
		
		io->InitFFmpeg();
		if(IsX11)
			InitVideoGl();
		//sleep(1);	/// some time is needed here, was 1 
		
		host->StartSession();
		guiActive = 0;
		io->SwitchInputReceiver(std::string("session"));
		setClientState("playing");
		return 0;
	}
		
	return 0;
}

void ImguiSdlWindow::PressSelect()
{
	
}

/// Can't be called too early, now unsafely using 'all_read_settings'
void ImguiSdlWindow::setClientState(std::string state)
{	
	/// host: unknown, standby, ready					 		 (this is host 'state')
	/// gui:  unknown, notreg, standby, waiting, ready, playing   (this is 'client_state')
	/// printf("ImguiSdlWindow::setClientState now:  %s\n", state.c_str());
	
	if(state.empty())
		client_state = std::string("unknown");
	else
		client_state = state;
	
	
	if(state == "unknown" || state == "notreg") {
		SwitchHostImage(0);
	} else {
		
		bool settings_ps5 = false;
		if(settings->all_read_settings.size() > 0) {
			if(settings->all_read_settings.at(0).isPS5 == "1")
				settings_ps5 = true;
		}

		if(settings_ps5 || host->ps5) // ouch we should not have two different ones. Needs merging
		{
			if(state == "standby")	SwitchHostImage(5);
			if(state == "waiting")	SwitchHostImage(4);
			if(state == "ready")	SwitchHostImage(6);
		}

		if(!settings_ps5 || !host->ps5)
		{
			if(state == "standby")	SwitchHostImage(2);
			if(state == "waiting")	SwitchHostImage(1);
			if(state == "ready")	SwitchHostImage(3);
		}
	}
	
}

void ImguiSdlWindow::restoreGui()
{	
	io->FiniFFmpeg();
	if(!IsX11)
		io->ShutdownStreamDrm(); /// good. clears stream frame buffer

	ImGui::Render();  /// Needed for some flush thing
	guiActive = 1;
	host->StartDiscoveryService();  /// Did I ever stop it?
	io->SwitchInputReceiver(std::string("gui"));
	clear_counter = 0;
	planes_init_done = false;
}

void ImguiSdlWindow::ToggleFullscreen()
{
	fullscreen = !fullscreen;
	if (fullscreen)
	{
	  SDL_SetWindowFullscreen(sdl_window, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	else
	{
	  SDL_SetWindowFullscreen(sdl_window, SDL_WINDOW_OPENGL);
	}
}

/// Needs to trigger in session
bool ImguiSdlWindow::resizeEvent(int w, int h)
{	
	//printf("New Window Size: %dx%d\n", w, h);
	
	/// Calc values to use for GL render portal
	GLsizei vp_width = 64;
	GLsizei vp_height = 48;
	GLsizei vp_xoffs = 0;
	GLsizei vp_yoffs = 0;
	float aspect = 1280.0 / 720.0;  /// Assuming always 1.77777
	
	if(aspect < w/h)  /// landscape
	{
		vp_width = (GLsizei)(h * aspect);
		vp_height = h;
		vp_xoffs = (w - vp_width)/2;
	}
	else   /// portrait
	{
		vp_width = w;
		vp_height = (GLsizei)(w / aspect);
		vp_yoffs = (h - vp_height)/2;
	}
	
	glViewGeom[0] = vp_xoffs;
	glViewGeom[1] = vp_yoffs;
	glViewGeom[2] = vp_width;
	glViewGeom[3] = vp_height;
	/// for use in glViewport(x, y, w, h)

	/// Calculate some dynamic widget sizes
	int btnMax=256;
	int btnMin=172;
	int a = RemapChi(w, 1280, 1920, btnMin, btnMax);
	int b = RemapChi(h, 720, 1080, btnMin, btnMax);
	psBtnSz = std::min(a,b);
	settingIndent = RemapChi(w, 1280, 1900, 0, 100);
	///printf("Indent:  %d\n", settingIndent);
	
	
	return true;
}
