#include <rpi/io.h>


/// Gamepad special input actions:
///
/// R3+Circle to quit
///
///


static enum AVPixelFormat get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != -1; p++) {
        if (*p == AV_PIX_FMT_DRM_PRIME) {
            return *p;
	}
    }

    fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}

IO::IO()
{
	printf("Rpi IO created\n");

}

int IO::Init(Host *host)
{	
	ChiakiErrorCode err;
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
	
	SDL_Quit();

	printf("Rpi IO destroyed\n");
}

int IO::InitGamepads()
{		
	// Initializing SDL with Joystick support (Joystick, game controller and haptic subsystems)
	if(SDL_Init( SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC ) < 0)
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

/// The input for the session
void IO::HandleJoyEvent(void)
{	
	SDL_Event event;
	
	if(takeInput)
	{
	/// reading events from the event queue
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_QUIT:
					exit(0);
					break;
				case SDL_CONTROLLERBUTTONUP:
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
	ChiakiControllerState kill_combo;  // can be set in Init or create.
	chiaki_controller_state_set_idle(&kill_combo);
	kill_combo.buttons |= CHIAKI_CONTROLLER_BUTTON_R3;
	kill_combo.buttons |= CHIAKI_CONTROLLER_BUTTON_MOON;
	if(controller_state.buttons == kill_combo.buttons) {
		///printf("Kill Session - Restore Gui\n");
		chiaki_session_stop(&this->host->session);		
		host->gui->restoreGui();
	}
	
	
}

void IO::ShutdownStreamDrm()
{
	drmprime_out_delete(dpo);
}


/// Set h264 or hevc here
int IO::InitFFmpeg() // pass the drm_fd here maybe instead of back door
{	
	printf("Begin InitFFmpeg\n");
	
	///av_log_set_level(AV_LOG_DEBUG);
	
	AVBufferRef * hw_device_ctx = nullptr;
	enum AVPixelFormat hw_pix_fmt = AV_PIX_FMT_NONE;
	const char *hw_decoder_name = "drm";
	enum AVHWDeviceType type;
	AVCodec *av_codec;  //rename 'decoder' ?
	const char *codec_name = "h264_v4l2m2m";  /// or "hevc"
		
	type = av_hwdevice_find_type_by_name(hw_decoder_name);
	if(type == AV_HWDEVICE_TYPE_NONE)
	{
		CHIAKI_LOGE(&log, "Hardware decoder \"%s\" not found", hw_decoder_name);
		//goto error_codec_context;
		return 1;
	}
	
	dpo = drmprime_out_new(drm_fd);  // I think I need to nicely close this after!?
    if (dpo == NULL) {
        fprintf(stderr, "Failed to open drmprime output\n");
        //return 1;
    }
	
	av_codec = avcodec_find_decoder_by_name(codec_name);
	if(!av_codec)
	{
		CHIAKI_LOGE(&log, "%s Codec not available", codec_name);
		//goto error_mutex;
		return 1;
	}
	
	//hw_pix_fmt = AV_PIX_FMT_DRM_PRIME; /// new, but doesn't really matter

	codec_context = avcodec_alloc_context3(av_codec);
	if(!codec_context)
	{
		CHIAKI_LOGE(&log, "Failed to alloc codec context");
		//goto error_mutex;
		return 1;
	}

	//   EXPERIMENTAL - from the switch code- Can't tell if it does anything.
	//
	//~ codec_context->flags |= AV_CODEC_FLAG_LOW_DELAY;
	//~ codec_context->flags2 |= AV_CODEC_FLAG2_FAST;
	//~ codec_context->flags2 |= AV_CODEC_FLAG2_CHUNKS; // was off
	//~ codec_context->thread_type = FF_THREAD_SLICE;
	//~ codec_context->thread_count = 4;
	
	/// Negotiating pixel format. Must return drm_prime
	codec_context->get_format = get_hw_format;  /// was func: get_hw_format;
	///[h264_v4l2m2m @ 0x1165d10] Format drm_prime chosen by get_format().
	///[h264_v4l2m2m @ 0x1165d10] avctx requested=181 (drm_prime); get_format requested=181 (drm_prime)

	codec_context->pix_fmt = AV_PIX_FMT_DRM_PRIME;   /* request a DRM frame */
    codec_context->coded_height = 720;
	codec_context->coded_width = 1280;
	
	if (avcodec_open2(codec_context, av_codec, nullptr) < 0) {
		printf("Could not open codec\n");
		return 1;
	}
	
	/// If you get 182 or other you are sourcing the wrong header file
	printf("Actual Fmt: %d\n", codec_context->pix_fmt); // 181
	
	if(codec_context->pix_fmt != AV_PIX_FMT_DRM_PRIME) {
		printf("Couldn't negoitiate AV_PIX_FMT_DRM_PRIME\n");
		return 1;
	}
	
	printf("Finish InitFFmpeg\n");
	return 0;

error_codec_context:
	if(hw_device_ctx)
		av_buffer_unref(&hw_device_ctx);
	avcodec_free_context(&codec_context);
error_mutex:
	//chiaki_mutex_fini(&mtx);
	this->mtx.lock();
	return CHIAKI_ERR_UNKNOWN;
}

int IO::FiniFFmpeg()
{
	avcodec_close(codec_context);
	avcodec_free_context(&codec_context);
	//if(decoder->hw_device_ctx)
	//	av_buffer_unref(&decoder->hw_device_ctx);
}


bool IO::VideoCB(uint8_t *buf, size_t buf_size)
{
	///printf("In VideoCB\n");
	///printf("%d\n", buf_size);
	
	this->mtx.lock();
	
	AVPacket packet;
	av_init_packet(&packet);
	packet.data = buf;
	packet.size = buf_size;
	AVFrame *frame = NULL;//new
	int ret = 0;
		    
    if(&packet)
	{
		ret = avcodec_send_packet(codec_context, &packet);
		/// printf("Send Packet (sample_cb) return;  %d\n", r);
		/// In particular, we don't expect AVERROR(EAGAIN), because we read all
		/// decoded frames with avcodec_receive_frame() until done.
		if (ret < 0) {
		  printf("Error return in send_packet\n");
	    }
	}
	 
	//this->mtx.unlock();
	//this->mtx.lock();

    while (1) {
        if (!(frame = av_frame_alloc())) {
            fprintf(stderr, "Can not alloc frame\n");
            ret = AVERROR(ENOMEM);
            goto the_end;
        }
		
		ret = avcodec_receive_frame(codec_context, frame); //ret 0 ==success
		///printf("PIX Format after receive_frame:  %d\n", frame->format);//181
		if (ret == 0) {
			//printf("Frame Successfully Decoded\n");
			drmprime_out_display(dpo, frame);
			this->mtx.unlock();
		}
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			printf("AVERROR(EAGAIN)\n");
			av_frame_free(&frame);
			//return 0; /// goes sends/gets another packet.
		} else if (ret < 0) {
			fprintf(stderr, "Error while decoding\n");
		}

   

fail:	/// this happens even without fail
	av_frame_free(&frame);
	av_packet_unref(&packet);
	this->mtx.unlock();
	if (ret < 0)
		return 0;
the_end:
	this->mtx.unlock();
	return 1;
	} // END while(1)
	
	return 1;
}

void IO::InitAudioCB(unsigned int channels, unsigned int rate)
{
	SDL_AudioSpec want, have, test;
	SDL_memset(&want, 0, sizeof(want));
	
	//source
	//[I] Audio Header:
	//[I]   channels = 2
	//[I]   bits = 16
	//[I]   rate = 48000
	//[I]   frame size = 480
	//[I]   unknown = 1
	unsigned int channels_ = 2;
	unsigned int rate_ = 48000;
	
	want.freq = rate_;
	want.format = AUDIO_S16SYS;
	// 2 == stereo
	want.channels = channels_;
	want.samples = 1024;
	want.callback = NULL;

	if(this->sdl_audio_device_id <= 0)
	{
		// the chiaki session might be called many times
		// open the audio device only once
		this->sdl_audio_device_id = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
	}

	if(this->sdl_audio_device_id <= 0)
	{
		//CHIAKI_LOGE(this->log, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
		printf("ERROR SDL_OpenAudioDevice\n");
	}
	else
	{
		SDL_PauseAudioDevice(this->sdl_audio_device_id, 0);
	}
}


void IO::AudioCB(int16_t *buf, size_t samples_count)
{	
	///printf("In AudioCB\n");
	//printf("AudCB: %d\n", samples_count);
	
	for(int x = 0; x < samples_count * 2; x++)
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
}
