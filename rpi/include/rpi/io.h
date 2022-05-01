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
#include <map>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#include "rpi/drmprime_out.h"
}

#define MAX_NUM_JOYSTICKS 2

#include "rpi/host.h"
#include "rpi/settings.h"

#include <setsu.h>
#include <chiaki/orientation.h>

class Host;
class Settings;

class AVFramesList
{
	public:
		AVFramesList();
		~AVFramesList();
		void	 Increment();
		AVFrame* GetNextFreed();
		AVFrame* GetCurrentFrame();
		void FreeLatest();
		
	private:
		int size=3;
		int current=0;
		int next=1;
		
		AVFrame* frameslist[3];
};

//static void SessionSetsuCb(SetsuEvent *event, void *user);

class IO
{
	private:
		Host *host =nullptr;
		
		/// controllers
		SDL_Haptic *haptics[MAX_NUM_JOYSTICKS];
		int rumble[MAX_NUM_JOYSTICKS]; 	/// 2 -> play rumble
		int connected;   /// connected joysticks number

		/// video
		std::mutex mtx;
		AVCodec *codec =nullptr;
		AVCodecContext *codec_context = nullptr;
		AVFrame *frame = nullptr;

		drmprime_out_env_t *dpo = nullptr;
		
		/// Audio
		SDL_AudioDeviceID sdl_audio_device_id = 0;
		
	public:
		ChiakiLog log;
		bool session_init = false;
		SDL_GameController *gamecontrollers[MAX_NUM_JOYSTICKS];
		ChiakiControllerState controller_state;
		
		/// Setsu
		Setsu *setsu;
		std::map<std::pair<std::string, SetsuTrackingId>, uint8_t> setsu_ids; // needs c++17
		ChiakiControllerState setsu_state;
		SetsuDevice *setsu_motion_device;
		ChiakiOrientationTracker orient_tracker;
		bool orient_dirty;
		void HandleSetsuEvent(SetsuEvent *event);
		void SetsuPoll();
		
		IO();
		~IO();
		int Init(Host *host);
		int InitGamepads();
		int RefreshGamepads();
		void HandleJoyEvent(void);
		void SpecialControlHandler();
		ChiakiControllerState GetState();
		void SendFeedbackState();
		void ScreenGrab();
		
		int InitFFmpeg();
		int FiniFFmpeg();
		AVFrame *frame_for_deletion = nullptr;
		AVFramesList frames_list;
		int drm_fd;
		bool takeInput=0;
		void ShutdownStreamDrm();
		void SwitchInputReceiver(std::string target);	/// gui/session
		uint32_t myEventType1;
		SDL_Event frameDecoded_event;
		unsigned long int nextFrameCount=0;
		
		/// Audio
		std::vector<std::string> audio_out_devices;
		
		/// Callbacks
		void InitAudioCB(unsigned int channels, unsigned int rate);
		bool VideoCB(uint8_t *buf, size_t buf_size);
		void AudioCB(int16_t *buf, size_t samples_count);
		
		// TEST ONLY PLESAE REMOVE
		int tmpCount=0;
		

};



#endif
