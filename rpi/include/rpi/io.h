#ifndef RPI_IO_H
#define RPI_IO_H

#include <chiaki/controller.h>
#include <chiaki/discovery.h>
#include <chiaki/log.h>
#include <chiaki/opusdecoder.h>
#include <chiaki/regist.h>
#include <chiaki/session.h>

#include <chiaki/ffmpegdecoder.h>

#include <SDL2/SDL.h>
#include <mutex>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#include "rpi/drmprime_out.h"
}

#define MAX_NUM_JOYSTICKS 2

#include "rpi/host.h"

class Host;
class Settings;

class IO
{
	private:
		Host *host =nullptr;
		
		/// controllers
		
		SDL_Haptic *haptics[MAX_NUM_JOYSTICKS];
		int rumble[MAX_NUM_JOYSTICKS]; 	/// 2 -> play rumble
		int connected; 
							/// connected joysticks number
		/// video
		std::mutex mtx;
		AVCodec *codec =nullptr;
		AVCodecContext *codec_context;
		AVFrame *frame =nullptr;
		drmprime_out_env_t *dpo = NULL;
		
		/// Audio
		SDL_AudioDeviceID sdl_audio_device_id = 0;


	public:
		ChiakiLog log;
		//Settings *settings = nullptr;
		bool session_init = false;
		//ChiakiSession session; // session in host
		SDL_GameController *gamecontrollers[MAX_NUM_JOYSTICKS];
		ChiakiControllerState controller_state;
		
		
		IO();
		~IO();
		int Init(Host *host);
		int InitGamepads();
		int RefreshGamepads();
		void HandleJoyEvent(void);
		void SpecialControlHandler();
		ChiakiControllerState GetState();
		
		int InitFFmpeg();
		int FiniFFmpeg();
		int drm_fd;
		bool takeInput=0;
		void ShutdownStreamDrm();
		void SwitchInputReceiver(std::string target);	/// gui/session
		
		void InitAudioCB(unsigned int channels, unsigned int rate);
		bool VideoCB(uint8_t *buf, size_t buf_size);
		void AudioCB(int16_t *buf, size_t samples_count);
		

};



#endif
