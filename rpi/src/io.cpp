#include <rpi/io.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "rpi/stb_image_write.h"

#define SETSU_UPDATE_INTERVAL_MS 4

AVFramesList::AVFramesList()
{
	for(auto i=0; i<size; i++)
	{
		frameslist[i] = new AVFrame;
		frameslist[i] = av_frame_alloc();
	}
}

AVFramesList::~AVFramesList()
{
}

void AVFramesList::Increment()///current
{	
	/// current == next-1
	// is already min?
	//~ if(next == 0) current = size-1;
	//~ else current = next-1;
	current=next;
	
	///printf("Curr:  %d    Next:  %d\n", current, next);
}

AVFrame* AVFramesList::GetNextFreed()
{
	//is already max?
	if(next == size-1) next=0;
	else next++;
	
	//av_frame_free(&frameslist[next]);
	frameslist[next] = av_frame_alloc();//should check for success
	return frameslist[next];
}

AVFrame* AVFramesList::GetCurrentFrame()
{
	return frameslist[current];
}

void AVFramesList::FreeLatest()
{
	printf("FreeLatest()\n");
	av_frame_free(&frameslist[current]);
}



static void SessionSetsuCb(SetsuEvent *event, void *user);
static void SessionSetsuCb(SetsuEvent *event, void *user)
{	
	IO *io = (IO *)user;
	
	io->HandleSetsuEvent(event);
}

IO::IO()
{
	// maybe Ill have to do some timer thing to poll every now and then
	audio_out_devices.clear();
	int i, count = SDL_GetNumAudioDevices(0);
	for (i = 0; i < count; ++i) {
		audio_out_devices.push_back(SDL_GetAudioDeviceName(i, 0));
		SDL_Log("Audio device %d: %s", i, SDL_GetAudioDeviceName(i, 0));
	}
	
	/// Setsu - touchpad, rumble
	setsu_motion_device = nullptr;
	chiaki_controller_state_set_idle(&setsu_state);
	orient_dirty = true;
	chiaki_orientation_tracker_init(&orient_tracker);
	setsu = setsu_new();

	printf("Rpi IO created\n");
}

int IO::Init(Host *host)
{	
	this->host = host;
	
	return 0;
}

IO::~IO()
{	
	for(int i = 0; gamecontrollers[i] != nullptr && i < MAX_NUM_JOYSTICKS; i++)
	{
		if(gamecontrollers[i] != nullptr)
		{
			SDL_GameControllerClose(gamecontrollers[i]);
			gamecontrollers[i] = nullptr;
			fprintf(stdout, "Closed joystick:\t#%i\n", i);
		}
		if(haptics[i] != nullptr)
		{
			SDL_HapticClose(haptics[i]);
			haptics[i] = nullptr;
			fprintf(stdout, "Closed haptic:\t\t#%i\n", i);
		}
	}
	
	setsu_free(setsu);
	
	SDL_Quit();

	printf("Rpi IO destroyed\n");
}

int IO::InitGamepads()
{		
	/// Initializing SDL with Joystick support (Joystick, game controller and haptic subsystems)
	if(SDL_Init( SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC ) < 0)
	{
		fprintf(stderr, "Error: Couldn't initialize SDL. %s\n", SDL_GetError());
		return 1;
	}
	
	fprintf(stdout, "Gamepads currently attached: %i\n", SDL_NumJoysticks());
	if(SDL_NumJoysticks() == 0)
		fprintf(stdout, "Check the gamepad is properly connected\n"); 
	
	/// Also actually open - add more checks later
	for(int i=0; i<SDL_NumJoysticks(); i++)
	{
		gamecontrollers[i] = SDL_GameControllerOpen(i);
		printf("Controller is:  %s\n", SDL_GameControllerName(gamecontrollers[i]));
	}
	
	SDL_JoystickEventState(SDL_ENABLE);

	return 0;
}

int IO::RefreshGamepads()
{
	for(int i=0; i<SDL_NumJoysticks(); i++)
	{	
		gamecontrollers[i] = nullptr;
		gamecontrollers[i] = SDL_GameControllerOpen(i);
		printf("Controller is:  %s\n", SDL_GameControllerName(gamecontrollers[i]));
	}
	
	return 0;
}

void IO::SwitchInputReceiver(std::string target)	/// gui/session
{
	if(target == std::string("gui")){
		host->gui->takeInput=1;
		takeInput=0;
	} else {
		host->gui->takeInput=0;
		takeInput=1;
	}
}

/// The input for the play session
void IO::HandleJoyEvent(void)
{	
	SDL_Event event;
	
	if(takeInput) // Warning this flushes the SDL_USEREVENTs
	{
	/// reading events from the event queue
		while (SDL_PollEvent(&event))
		{	

			switch (event.type)
			{
				case SDL_QUIT:
					exit(0);
					break;
				
				case SDL_KEYDOWN:
				{
					case SDLK_F11:
						host->gui->ToggleFullscreen();
						break;
				}
					
				case SDL_CONTROLLERBUTTONUP:
					controller_state = GetState();
					break;
				case SDL_CONTROLLERBUTTONDOWN:
					//printf("Buttonpress\n");
					controller_state = GetState();
					SpecialControlHandler();
					break;			
				case SDL_CONTROLLERAXISMOTION:
					controller_state = GetState();
					break;
				
				case SDL_JOYDEVICEADDED:
					printf("Gamepad added\n");
					RefreshGamepads();
					break;
				case SDL_JOYDEVICEREMOVED:
					printf("Gamepad removed\n");
					RefreshGamepads();
					break;
					
				case SDL_USEREVENT:
					SDL_PushEvent(&event); // pass back to queue, might cause inf loop!
					break;
					
				case SDL_WINDOWEVENT:
				{
					if (event.window.event == SDL_WINDOWEVENT_RESIZED)
					{	
						host->gui->resizeEvent(event.window.data1, event.window.data2);
					}
					break;
				}
				
				default:
					break;
			}
		}
	}
}

/// Supports just one controller atm.
ChiakiControllerState IO::GetState()
{
	ChiakiControllerState state;
	chiaki_controller_state_set_idle(&state);
	chiaki_controller_state_set_idle(&controller_state);

	SDL_GameController *controller = gamecontrollers[0];

	if(!controller)
		return state;

	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A) ? CHIAKI_CONTROLLER_BUTTON_CROSS : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B) ? CHIAKI_CONTROLLER_BUTTON_MOON : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_X) ? CHIAKI_CONTROLLER_BUTTON_BOX : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_Y) ? CHIAKI_CONTROLLER_BUTTON_PYRAMID : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT) ? CHIAKI_CONTROLLER_BUTTON_DPAD_LEFT : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ? CHIAKI_CONTROLLER_BUTTON_DPAD_RIGHT : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP) ? CHIAKI_CONTROLLER_BUTTON_DPAD_UP : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN) ? CHIAKI_CONTROLLER_BUTTON_DPAD_DOWN : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) ? CHIAKI_CONTROLLER_BUTTON_L1 : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) ? CHIAKI_CONTROLLER_BUTTON_R1 : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_LEFTSTICK) ? CHIAKI_CONTROLLER_BUTTON_L3 : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSTICK) ? CHIAKI_CONTROLLER_BUTTON_R3 : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START) ? CHIAKI_CONTROLLER_BUTTON_OPTIONS : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_BACK) ? CHIAKI_CONTROLLER_BUTTON_SHARE : 0;
	state.buttons |= SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_GUIDE) ? CHIAKI_CONTROLLER_BUTTON_PS : 0;
	state.l2_state = (uint8_t)(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) >> 7);
	state.r2_state = (uint8_t)(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) >> 7);
	state.left_x = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
	state.left_y = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);
	state.right_x = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX);
	state.right_y = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY);

	return state;
}

void IO::SpecialControlHandler()
{
	///printf("SpecialControlHandler()\n");

	/// R3+Moon/Circle to Quit session
	ChiakiControllerState kill_combo;
	chiaki_controller_state_set_idle(&kill_combo);
	kill_combo.buttons |= CHIAKI_CONTROLLER_BUTTON_R3;
	kill_combo.buttons |= CHIAKI_CONTROLLER_BUTTON_MOON;
	
	/// R3+Box/Square to take screen grab
	ChiakiControllerState screengrab_combo;
	chiaki_controller_state_set_idle(&screengrab_combo);
	screengrab_combo.buttons |= CHIAKI_CONTROLLER_BUTTON_R3;
	screengrab_combo.buttons |= CHIAKI_CONTROLLER_BUTTON_BOX;
	
	if(controller_state.buttons == kill_combo.buttons) {
		///printf("Kill Session - Restore Gui\n");
		chiaki_session_stop(&this->host->session);
		host->gui->restoreGui();
		return;
	}
	if(controller_state.buttons == screengrab_combo.buttons) {
		ScreenGrab();
		return;
	}
	
	
}

void IO::SetsuPoll()
{
	setsu_poll(setsu, SessionSetsuCb, this);
	//chiaki_orientation_tracker_apply_to_controller_state(&orient_tracker, &setsu_state);
}

void IO::HandleSetsuEvent(SetsuEvent *event)
{
	if(!setsu)
		return;
	switch(event->type)
	{
		case SETSU_EVENT_DEVICE_ADDED:
			switch(event->dev_type)
			{
				case SETSU_DEVICE_TYPE_TOUCHPAD:
					// connect all the touchpads!
					if(setsu_connect(setsu, event->path, event->dev_type))
						printf("Connected Setsu Touchpad Device %s\n", event->path);
					else
						printf("Failed to connect to Setsu Touchpad Device %s\n", event->path);
					break;
				case SETSU_DEVICE_TYPE_MOTION:
					// connect only one motion since multiple make no sense
					if(setsu_motion_device)
					{
						printf("Setsu Motion Device %s detected there is already one connected\n",
								event->path);
						break;
					}
					setsu_motion_device = setsu_connect(setsu, event->path, event->dev_type);
					if(setsu_motion_device)
						printf("Connected Setsu Motion Device %s\n", event->path);
					else
						printf("Failed to connect to Setsu Motion Device %s\n", event->path);
					break;
			}
			break;
		
		case SETSU_EVENT_DEVICE_REMOVED:
			switch(event->dev_type)
			{
				case SETSU_DEVICE_TYPE_TOUCHPAD:
					printf("Setsu Touchpad Device %s disconnected", event->path);
					for(auto it=setsu_ids.begin(); it!=setsu_ids.end();)
					{
						///if(it.key().first == event->path)
						if(it->first.first == event->path)
						{
							chiaki_controller_state_stop_touch(&setsu_state, it->second);
							setsu_ids.erase(it++);
						}
						else
							it++;
					}
					//SendFeedbackState();
					break;
				case SETSU_DEVICE_TYPE_MOTION:
					if(!setsu_motion_device || strcmp(setsu_device_get_path(setsu_motion_device), event->path))
						break;
					printf("Setsu Motion Device %s disconnected", event->path);
					setsu_motion_device = nullptr;
					chiaki_orientation_tracker_init(&orient_tracker);
					orient_dirty = true;
					break;
			}
			break;
		case SETSU_EVENT_TOUCH_DOWN:
			break;
		case SETSU_EVENT_TOUCH_UP:
			for(auto it=setsu_ids.begin(); it!=setsu_ids.end(); it++)
			{
				if(it->first.first == setsu_device_get_path(event->dev) && it->first.second == event->touch.tracking_id)
				{
					chiaki_controller_state_stop_touch(&setsu_state, it->second);
					setsu_ids.erase(it);
					break;
				}
			}
			// SendFeedbackState();
			break;
		case SETSU_EVENT_TOUCH_POSITION: {
			std::pair<std::string, SetsuTrackingId> k =  { setsu_device_get_path(event->dev), event->touch.tracking_id };
			auto it = setsu_ids.find(k);
			if(it == setsu_ids.end())
			{
				int8_t cid = chiaki_controller_state_start_touch(&setsu_state, event->touch.x, event->touch.y);
				if(cid >= 0)
					setsu_ids[k] = (uint8_t)cid;
				else
					break;
			}
			else
				chiaki_controller_state_set_touch_pos(&setsu_state, it->second, event->touch.x, event->touch.y);
			// SendFeedbackState();
			break;
		}
		case SETSU_EVENT_BUTTON_DOWN:
			setsu_state.buttons |= CHIAKI_CONTROLLER_BUTTON_TOUCHPAD;
			break;
		case SETSU_EVENT_BUTTON_UP:
			setsu_state.buttons &= ~CHIAKI_CONTROLLER_BUTTON_TOUCHPAD;
			break;
		case SETSU_EVENT_MOTION:
			chiaki_orientation_tracker_update(&orient_tracker,
					event->motion.gyro_x, event->motion.gyro_y, event->motion.gyro_z,
					event->motion.accel_x, event->motion.accel_y, event->motion.accel_z,
					event->motion.timestamp);
			orient_dirty = true;
			break;
	}
}

void IO::SendFeedbackState()
{
	ChiakiControllerState state;
	chiaki_controller_state_set_idle(&state);

	// setsu is the one that potentially has gyro/accel/orient so copy that directly first
	state = setsu_state;

	chiaki_controller_state_or(&state, &state, &controller_state);///merge
	chiaki_session_set_controller_state(&host->session, &state);
}


void IO::ShutdownStreamDrm()
{
	drmprime_out_delete(dpo);
}


int IO::InitFFmpeg() // pass the drm_fd here maybe instead of back door?
{	
	///printf("Begin InitFFmpeg\n");
	///av_log_set_level(AV_LOG_DEBUG);
	
	AVBufferRef * hw_device_ctx = nullptr;
	const char *hw_decoder_name = "drm";
	enum AVHWDeviceType type;
	AVCodec *av_codec;
	
	const char *codec_name = "h264_v4l2m2m";
	ChiakiCodec chi_codec = host->gui->settings->GetChiakiCodec(host->gui->settings->all_validated_settings.at(0).sess.codec);
	if(chi_codec == CHIAKI_CODEC_H265)
		codec_name = "hevc";

	type = av_hwdevice_find_type_by_name(hw_decoder_name);
	if(type == AV_HWDEVICE_TYPE_NONE)
	{
		printf("Hardware decoder \"%s\" not found", hw_decoder_name);
		return 1;
	}
	
	if(!host->gui->IsX11)
	{
		printf("Initiating new drm render device");
		dpo = drmprime_out_new(drm_fd);
		if (dpo == nullptr) {
			fprintf(stderr, "Failed to open drmprime output\n");
			return 1;
		}
	}
	
	av_codec = avcodec_find_decoder_by_name(codec_name);
	if(!av_codec)
	{
		CHIAKI_LOGE(&log, "%s Codec not available", codec_name);
		return 1;
	}

	codec_context = avcodec_alloc_context3(av_codec);
	if(!codec_context)
	{
		CHIAKI_LOGE(&log, "Failed to alloc codec context");
		return 1;
	}

	/// Needs to match actual session resolution
	int width_setting = 1280;
	int height_setting = 720;
	ChiakiVideoResolutionPreset resolution_preset = host->gui->settings->GetChiakiResolution(host->gui->settings->all_validated_settings.at(0).sess.resolution);
	if(resolution_preset == CHIAKI_VIDEO_RESOLUTION_PRESET_1080p) { width_setting=1920; height_setting=1080; }
	if(resolution_preset == CHIAKI_VIDEO_RESOLUTION_PRESET_720p)  { width_setting=1280; height_setting=720; }
	if(resolution_preset == CHIAKI_VIDEO_RESOLUTION_PRESET_540p)  { width_setting=960;  height_setting=540; }
    codec_context->coded_height = height_setting;
	codec_context->coded_width = width_setting;
	codec_context->pix_fmt = AV_PIX_FMT_DRM_PRIME;  /// request a DRM frame

	if (avcodec_open2(codec_context, av_codec, nullptr) < 0) {
		printf("Could not open codec\n");
		return 1;
	}
	
	/// Must have gotten drm_prime
	/// If you get 182 or other you are sourcing the wrong header file
	printf("Actual Fmt: %d\n", codec_context->pix_fmt); // 181
	
	if(codec_context->pix_fmt != AV_PIX_FMT_DRM_PRIME) {
		printf("Couldn't negoitiate AV_PIX_FMT_DRM_PRIME\n");
		return 1;
	}
	
	printf("Finish InitFFmpeg\n");
	return 0;

	if(hw_device_ctx)
		av_buffer_unref(&hw_device_ctx);
	avcodec_free_context(&codec_context);
//error_mutex:
	//chiaki_mutex_fini(&mtx);
//	this->mtx.lock();
//	return CHIAKI_ERR_UNKNOWN;
}

int IO::FiniFFmpeg()
{
	avcodec_close(codec_context);
	avcodec_free_context(&codec_context);
	//if(decoder->hw_device_ctx)
	//	av_buffer_unref(&decoder->hw_device_ctx);
	return 1;
}


/// For kmsdrm render and gl render
bool IO::VideoCB(uint8_t *buf, size_t buf_size)
{	
	//printf("In VideoCB\n");	
	AVPacket packet;
	av_init_packet(&packet); // Deprecated2021 NEW->  AVPacket* packet = av_packet_alloc();
	packet.data = buf;
	packet.size = buf_size;
	int ret = 0;
		    
   // if(&packet)
    if(1)
	{	
		ret = avcodec_send_packet(codec_context, &packet);
		/// printf("Send Packet (sample_cb) return;  %d\n", r);
		/// In particular, we don't expect AVERROR(EAGAIN), because we read all
		/// decoded frames with avcodec_receive_frame() until done.
		if (ret < 0) {
		  printf("Error return in send_packet\n");
	    }
	}
	
	///NEED TO FREE THE CLASS FRAME
	av_frame_free(&frame);
	
    while (1) {
        //~ if (!(frame = av_frame_alloc())) { //can this be removed with queue?
            //~ fprintf(stderr, "Can not alloc frame\n");
            //~ ret = AVERROR(ENOMEM);
            //~ goto the_end;
        //~ }
        
        frame = frames_list.GetNextFreed();/// get empty frame pointer, allocated
		ret = avcodec_receive_frame(codec_context, frame); //ret 0 ==success

		///printf("PIX Format after receive_frame:  %d\n", frame->format);//181
		if (ret == 0) {
			///printf("Frame Successfully Decoded\n");
						
			if(host->gui->IsX11)
			{	
				frames_list.Increment();/// increments 'current' for pull
				nextFrameCount++;
			} else {
				drmprime_out_display(dpo, frame); /// drm pipe
			}

		}
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			printf("AVERROR(EAGAIN)\n");
			av_frame_free(&frame);
		} else if (ret < 0) {
			fprintf(stderr, "Error while decoding\n");
		}


fail:	/// this happens even without fail
	///av_frame_free(&frame);
	av_packet_unref(&packet);
	return 1;
	} /// END while(1)
	
	return 1;
}


void IO::ScreenGrab()
{	
	int ret;
	AVFrame *sw_frame = nullptr;
	sw_frame = av_frame_alloc();
	AVFrame *tmp_frame;

	int outWidth = frame->width;
	int outHeight = frame->height;
	size_t size = av_image_get_buffer_size((AVPixelFormat)AV_PIX_FMT_RGB24, outWidth, outHeight, 1);

	if (frame->format == AV_PIX_FMT_DRM_PRIME) {
		/// retrieve data from GPU to CPU
		if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0) {
			fprintf(stderr, "Error transferring the data to system memory\n");
			return;
		}
		tmp_frame = sw_frame;
	} else
		tmp_frame = frame;

	/// https://newbedev.com/how-to-convert-rgb-from-yuv420p-for-ffmpeg-encoder 
	SwsContext * ctx = sws_getContext(outWidth, outHeight,
							  AV_PIX_FMT_YUV420P, outWidth, outHeight,
							  AV_PIX_FMT_RGB24, 0, 0, 0, 0);

	AVFrame * newOutFrame;
	newOutFrame = av_frame_alloc();
	uint8_t *out_buffer;  
	out_buffer=new uint8_t[3*outWidth*outHeight];
	av_image_fill_arrays (newOutFrame->data, newOutFrame->linesize, out_buffer, AV_PIX_FMT_RGB24, outWidth, outHeight, 1);
	sws_scale(ctx, (const uint8_t* const*)tmp_frame->data, tmp_frame->linesize, 0, outHeight, newOutFrame->data, newOutFrame->linesize);
	
	std::string filename("/home/");
	filename.append(getenv("USER"));
	filename.append("/.config/Chiaki/screengrab.jpg");
	///const char* file_name = "screengrab.jpg";
	//FILE* output_file = fopen(filename.c_str(), "w");

	///int stbi_write_jpg(char const *filename, int w, int h, int comp, const void *data, int quality);
	///stbi_write_jpg(file_name, outWidth, outHeight, 3, (const void*)newOutFrame->data[0], 90);
	if (stbi_write_jpg(filename.c_str(), outWidth, outHeight, 3, (const void*)newOutFrame->data[0], 90) < 1) {
		fprintf(stderr, "Failed to save screen grab\n");
		//fclose(output_file);
		return;
	}

	//fclose(output_file);
	printf("Saved a screen grab\n");
}


/// happens on Session start
void IO::InitAudioCB(unsigned int channels, unsigned int rate)
{
	
	printf("Init Audio CB\n");
	
	SDL_AudioSpec want;
	SDL_memset(&want, 0, sizeof(want));
	
	//source
	//[I] Audio Header:
	//[I]   channels = 2
	//[I]   bits = 16
	//[I]   rate = 48000
	//[I]   frame size = 480
	//[I]   unknown = 1
	//unsigned int channels_ = 2;
	//unsigned int rate_ = 48000;

	
	want.freq = rate;
	want.format = AUDIO_S16SYS;
	want.channels = channels;  /// 2 == stereo
	want.samples = 1024;
	want.callback = NULL;
	
	std::string current_out_choice = host->gui->settings->all_validated_settings.at(0).sess.audio_device;
	printf("Session Audio OUT:  %s\n", current_out_choice.c_str());
	
	if(audio_out_devices.size() > 0)
	{
		sdl_audio_device_id = SDL_OpenAudioDevice(current_out_choice.c_str() , 0, &want, NULL, 0);
	}
	
	if(this->sdl_audio_device_id <= 0)
	{	
		// the chiaki session might be called many times
		// open the audio device only once
		sdl_audio_device_id = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
	}

	if(this->sdl_audio_device_id <= 0)
	{
		///CHIAKI_LOGE(this->log, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
		printf("ERROR SDL_OpenAudioDevice\n");
	}
	else
	{
		SDL_PauseAudioDevice(this->sdl_audio_device_id, 0);
	}
}

void IO::AudioCB(int16_t *buf, size_t samples_count)
{	
	//printf("In AudioCB\n");
	//printf("AudCB: %d\n", samples_count);
	
	for(uint32_t x = 0; x < samples_count * 2; x++)
	{
		// boost audio volume
		int sample = buf[x] * 1.80;
		// Hard clipping (audio compression)
		// truncate value that overflow/underflow int16
		if(sample > INT16_MAX)
		{
			buf[x] = INT16_MAX;
			CHIAKI_LOGD(&log, "Audio Hard clipping INT16_MAX < %d", sample);
		}
		else if(sample < INT16_MIN)
		{
			buf[x] = INT16_MIN;
			CHIAKI_LOGD(&log, "Audio Hard clipping INT16_MIN > %d", sample);
		}
		else
			buf[x] = (int16_t)sample;
	}
	
	int audio_queued_size = SDL_GetQueuedAudioSize(this->sdl_audio_device_id);
	if(audio_queued_size > 16000)
	{
		// clear audio queue to avoid big audio delay
		// average values are close to 13000 bytes
		CHIAKI_LOGW(&log, "Triggering SDL_ClearQueuedAudio with queue size = %d", audio_queued_size);
		SDL_ClearQueuedAudio(this->sdl_audio_device_id);
	}

	int success = SDL_QueueAudio(this->sdl_audio_device_id, buf, sizeof(int16_t) * samples_count * 2);
	if(success != 0)
		CHIAKI_LOGE(&log, "SDL_QueueAudio failed: %s\n", SDL_GetError());
		
	//printf("END IO::AudioCB\n");
	return;
}
