#include <bitset> // test qbytestream
//#include <cstring>

#include <rpi/host.h>
#include "rpi/settings.h"

#include <chiaki/base64.h> //move to .h

/// Notes:
/// # sudo chmod a+rw /dev/dma_heap/*
/// Try:  gpu=96(config.txt), cma=64M(cmdline.txt)

#define PING_MS		500
#define HOSTS_MAX	3		// was 16
#define DROP_PINGS	3
#define CHIAKI_SESSION_AUTH_SIZE 0x10

// Can I make the ConnectionEventCB into this instead to save recursion?
static void EventCB(ChiakiEvent *event, void *user)
{	
	//printf("In EventCB\n");
	Host *host = (Host *)user;
	host->ConnectionEventCB(event);
}

static bool VideoCB(uint8_t *buf, size_t buf_size, void *user)
{
	//printf("Host VideoCB\n");
	IO *io = (IO *)user;
	return io->VideoCB(buf, buf_size);
}

static void InitAudioCB(unsigned int channels, unsigned int rate, void *user)
{
	IO *io = (IO *)user;
	io->InitAudioCB(channels, rate);
}

static void AudioCB(int16_t *buf, size_t samples_count, void *user)
{
	IO *io = (IO *)user;
	io->AudioCB(buf, samples_count);
}

void *play(void *user)
{
	//ORIGorig(void)user;
	Host *host = (Host *)user;

	while(1)
	{
		host->io->HandleJoyEvent();
		chiaki_session_set_controller_state(&host->session, &host->io->controller_state);
		SDL_Delay(4); ///ms
	}
}

Host::Host()
{
	chiaki_log_init(&log, 1, NULL, NULL);  // 1, 4, 14
	///chiaki_log_init(&log, CHIAKI_LOG_ALL & ~CHIAKI_LOG_VERBOSE, NULL, NULL);
	
	printf("Rpi Host created\n");
}

Host::~Host()
{	
	chiaki_session_stop(&session);
	chiaki_session_join(&session); //?
	
	printf("Rpi Host destroyed\n");
}

int Host::Init(IO *io)
{	
	ChiakiErrorCode err;

	this->io = io;
	
	
	/// Starting a Discovery Service - helps with IP
	/// Temp to get PS type version
	ChiakiDiscoveryServiceOptions *options = new ChiakiDiscoveryServiceOptions;
	options->ping_ms = PING_MS;
	options->hosts_max = HOSTS_MAX;
	options->host_drop_pings = DROP_PINGS;
	//options.cb = Discovery;   // type  ChiakiDiscoveryServiceCb
	//options.cb_user = this;
	
	ChiakiDiscoveryService *service = new ChiakiDiscoveryService;
	///from discoverymanager
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = 0xffffffff; /// 255.255.255.255
	options->send_addr = reinterpret_cast<sockaddr *>(&addr);
	options->send_addr_size = sizeof(addr);
	
	ChiakiConnectInfo connect_info;
	int foundHost=0;
	std::string state;  /// standby, etc
	
	err = chiaki_discovery_service_init(service, options, &log);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&log, "DiscoveryManager failed to init Discovery Service");
		return -1;
	}
	
	ChiakiTarget target = CHIAKI_TARGET_PS4_UNKNOWN;  ///will be filled in. console+firmware, I think. An Enum

	/// check for discoveries
	for(int i=0; i<10; i++)
	{
		printf("Searching for Playstation Host [%d/10]\n", i+1);
		
		foundHost = service->hosts_count;
		state = chiaki_discovery_host_state_string(service->hosts->state);
		printf("State:  %s\n", state.c_str());

		if(foundHost==1)
		{	
			target = chiaki_discovery_host_system_version_target(service->hosts);
			printf("Target:  %d\n", target);  //  Target:  1000, CHIAKI_TARGET_PS4_10
			if(target == CHIAKI_TARGET_PS4_UNKNOWN)
				printf("Oh No target still Unknown!\n");\
				
			/// HOST IP
			connect_info.host = service->hosts->host_addr;
			printf("Host Addr:  %s\n", connect_info.host);
			i=10;
		}
		sleep(1);
	}
	////////////   END Discovery Service
		
	chiaki_opus_decoder_init(&opus_decoder, &log);
	ChiakiAudioSink audio_sink;

	///	676f894f
	///	\xf1`\xf8\a\\\x9cT\x52z\x18\x5\x59\xbc\x20\xca\x15
	std::vector<std::string> settings_in = ReadRpiSettings();
	
	ChiakiVideoResolutionPreset resolution_preset = CHIAKI_VIDEO_RESOLUTION_PRESET_720p;
	ChiakiVideoFPSPreset fps_preset = CHIAKI_VIDEO_FPS_PRESET_60;
		
	strcpy(connect_info.regist_key, settings_in.at(0).c_str());  //from settings file
	printf("Regist Key:  %s\n", connect_info.regist_key);

	/// https://en.cppreference.com/w/cpp/language/ascii
	/// char byte_aray2[16] = {241, 96, 248, 7, 92, 156, 84, 130, 122, 24, 5, 185, 188, 128, 202, 21};
	std::vector<char> key_bytes = KeyStr2ByteArray(settings_in[1].c_str());
	memcpy(&connect_info.morning, key_bytes.data(), sizeof(connect_info.morning)); //from settings file
	printf("RPKey_morning:  %s\n", connect_info.morning);
	
	/// If Sleeping (orange light)
	if(foundHost==1 && (state == "standby" || state == "unknown"))
	{
		printf("Sending Wakeup!\n");
		
		uint64_t credential = (uint64_t)strtoull(connect_info.regist_key, NULL, 16);
		chiaki_discovery_wakeup(&log, NULL,	connect_info.host, credential, 0);// Last is this->IsPS5()
		
		sleep(1);
	}
	while(state == "standby" || state == "unknown")
	{
		state = chiaki_discovery_host_state_string(service->hosts->state);
		printf("Waiting for \"ready\", now:  \"%s\"\n", state.c_str());
		sleep(2);
		if(state == "ready") {
			printf("\nPlaystation now Awake! Please re-run chiaki-rpi!\n\n");
			exit(1);
		}
	}

	
	
	
	
	connect_info.ps5 = false;
	connect_info.video_profile_auto_downgrade = false;
	connect_info.enable_keyboard = false;
	//connect_info.video_profile.bitrate = 15000;
	session.connect_info.video_profile.max_fps = fps_preset;
	//session.connect_info.video_profile.width = 1280;
	//session.connect_info.video_profile.height = 720;
	//session.connect_info.video_profile.bitrate = 15000;
	session.connect_info.video_profile_auto_downgrade = false;
	
	//connect_info.video_profile.codec = CHIAKI_CODEC_H264;
	//session.connect_info.video_profile.codec = CHIAKI_CODEC_H264;
	
	//connect_info.video_profile.bitrate = 15000;
	
//~ unsigned int width;
//~ unsigned int height;
//~ unsigned int max_fps;
//~ unsigned int bitrate;
//~ ChiakiCodec codec;
//~ } ChiakiConnectVideoProfile;

	chiaki_connect_video_profile_preset(&connect_info.video_profile, resolution_preset, fps_preset);
	//printf("E\n");
	
	err = chiaki_session_init(&session, &connect_info, &log);
	//free((void *)connect_info.host);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		printf("ailed to init session\n");
		return 1;
	}
	
	video_profile = connect_info.video_profile; /// store it. Why actually?
	
	chiaki_opus_decoder_set_cb(&opus_decoder, InitAudioCB, AudioCB, io);
	chiaki_opus_decoder_get_sink(&opus_decoder, &audio_sink);
	chiaki_session_set_audio_sink(&session, &audio_sink);
	chiaki_session_set_video_sample_cb(&session, VideoCB, io); /// send to IO to execute
	//chiaki_session_set_event_cb(&this->session, EventCB, this);
	

	return 0;
}


int Host::StartSession()
{	
	ChiakiErrorCode err;
			
	err = chiaki_session_start(&session);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		printf("ERROR chiaki session start\n");
		CHIAKI_LOGE(&log, "Session start failed: %s\n", chiaki_error_string(err));
		return 1;
	}
	printf("After chiaki session start\n");
	printf("Session Bitrate:  %d\n", session.connect_info.video_profile.bitrate);
	printf("Session target:  %d\n", session.target);

	chiaki_session_set_event_cb(&session, EventCB, this);

	return 0;
}



void Host::ConnectionEventCB(ChiakiEvent *event)
{
	switch(event->type)
	{
		case CHIAKI_EVENT_CONNECTED:
			printf("EventCB CHIAKI_EVENT_CONNECTED\n");
			chiaki_thread_create(&play_th, play, this);
			//session, chiaki_thread_create(&session->session_thread, session_thread_func, session);
			//if(this->chiaki_event_connected_cb != nullptr)
			//	this->chiaki_event_connected_cb();
			break;
		//~ case CHIAKI_EVENT_LOGIN_PIN_REQUEST:
			//~ CHIAKI_LOGI(this->log, "EventCB CHIAKI_EVENT_LOGIN_PIN_REQUEST");
			//~ if(this->chiaki_even_login_pin_request_cb != nullptr)
				//~ this->chiaki_even_login_pin_request_cb(event->login_pin_request.pin_incorrect);
			//~ break;
		//~ case CHIAKI_EVENT_RUMBLE:
			//~ CHIAKI_LOGD(this->log, "EventCB CHIAKI_EVENT_RUMBLE");
			//~ if(this->chiaki_event_rumble_cb != nullptr)
				//~ this->chiaki_event_rumble_cb(event->rumble.left, event->rumble.right);
			//~ break;
		//~ case CHIAKI_EVENT_QUIT:
			//~ CHIAKI_LOGI(this->log, "EventCB CHIAKI_EVENT_QUIT");
			//~ if(this->chiaki_event_quit_cb != nullptr)
				//~ this->chiaki_event_quit_cb(&event->quit);
			//~ break;
	}
}

bool Host::SetHostRPKey(Host *host, std::string rp_key_b64)
{
	if(host != nullptr)
	{
		size_t rp_key_sz = sizeof(host->rp_key);
		ChiakiErrorCode err = chiaki_base64_decode(
			rp_key_b64.c_str(), rp_key_b64.length(),
			host->rp_key, &rp_key_sz);
		if(CHIAKI_ERR_SUCCESS != err)
			CHIAKI_LOGE(&this->log, "Failed to parse RP_KEY %s (it must be a base64 encoded)", rp_key_b64.c_str());
		else
			return true;
	}
	else
		CHIAKI_LOGE(&this->log, "Cannot SetHostRPKey from nullptr host");

	return false;
}

std::string Host::GetHostRPKey(Host *host)
{
	if(host != nullptr)
	{
		if(host->rp_key_data || host->registered)
		{

			size_t B64EncSize = ((4 * 0x10 / 3) + 3) & ~3;
			size_t rp_key_b64_sz = B64EncSize;
			//size_t rp_key_b64_sz = this->GetB64encodeSize(0x10);
			char rp_key_b64[rp_key_b64_sz + 1] = { 0 };
			ChiakiErrorCode err;
			err = chiaki_base64_encode(
				host->rp_key, 0x10,
				rp_key_b64, sizeof(rp_key_b64));

			if(CHIAKI_ERR_SUCCESS == err)
				return rp_key_b64;
			else
				CHIAKI_LOGE(&this->log, "Failed to encode rp_key to base64");
		}
	}
	else
		CHIAKI_LOGE(&this->log, "Cannot GetHostRPKey from nullptr host");

	return "";
}
