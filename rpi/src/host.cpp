#include <bitset> 	// test qbytestream
#include <string>
#include <iostream>	/// to_string()

#include <rpi/host.h>
#include "rpi/settings.h"

#include <chiaki/base64.h> //move to .h

/// Notes:
/// # sudo chmod a+rw /dev/dma_heap/*
/// Try:  gpu=96(config.txt), cma=64M(cmdline.txt)
///  or   gpu=256, cma=128

#define PING_MS		500
#define DROP_PINGS	3
#define CHIAKI_SESSION_AUTH_SIZE 0x10

static void RegistEventCB(ChiakiRegistEvent *event, void *user)
{
	printf("In  host RegistEventCB\n");
	Host *host = (Host *)user;
	
	switch(event->type)
	{
		case CHIAKI_REGIST_EVENT_TYPE_FINISHED_CANCELED:
			printf("Registration Cancelled\n");
			break;
		case CHIAKI_REGIST_EVENT_TYPE_FINISHED_FAILED:
			printf("Registration fail\n");
			break;
		case CHIAKI_REGIST_EVENT_TYPE_FINISHED_SUCCESS:

			ChiakiRegisteredHost *r_host = event->registered_host;
			
			char regist_key[128];
			memcpy(regist_key, r_host->rp_regist_key, sizeof(regist_key));
			std::string regist_key_str(regist_key);
			printf("Regist Key:  %s\n", regist_key);

			std::string rp_key_str = host->GetHostRPKey(r_host->rp_key);
			printf("GetHostRPKey: %s\n", rp_key_str.c_str());
			
			/// Save out defaults to settings file 
			// Only host[0] for now
			int i=0;
			ChiakiDiscoveryHost *dh = host->discoveredHosts+i;
			rpi_settings_host new_host;
			new_host.sess.decoder = "automatic";
			new_host.sess.codec = "automatic";
			new_host.sess.resolution = "1080";
			new_host.sess.fps = "60";
			new_host.sess.audio_device = "hdmi";
			int ps5 = chiaki_target_is_ps5(chiaki_discovery_host_system_version_target(dh));//ChiakiDiscoveryHost
			new_host.isPS5 = std::to_string(ps5);
			new_host.nick_name = dh->host_name;
			new_host.id = dh->host_id; /// mac
			new_host.rp_key = rp_key_str;
			new_host.regist = regist_key;
			
			host->gui->settings->all_validated_settings.push_back(new_host);
			host->gui->settings->WriteYaml(host->gui->settings->all_validated_settings);
			
			// Not correct! Needs to be Discovered's state
			host->gui->setClientState(std::string("ready"));
			printf("Registration Success!!\n");
			break;
	}
	
	//chiaki_regist_stop(&gui->regist);
	//chiaki_regist_fini(&gui->regist);
}

// Can I make the ConnectionEventCB into this instead to save recursion?
static void EventCB(ChiakiEvent *event, void *user);
static void EventCB(ChiakiEvent *event, void *user)
{	
	//printf("In EventCB\n");
	Host *host = (Host *)user;
	host->ConnectionEventCB(event);
}

// Not used?
//static void DiscoveryServiceHostsCallback(ChiakiDiscoveryHost *hosts, size_t hosts_count, void *user);


// NEEDS FIXING UP FOR MULTIPLE HOSTS, PS5's
//
/// triggers both on initial discovery and comeback after wakeup signal.
static void Discovery(ChiakiDiscoveryHost *discovered_hosts, size_t hosts_count, void *);
static void Discovery(ChiakiDiscoveryHost *discovered_hosts, size_t hosts_count, void *user)
{
	///printf("In Discovery()\n");
	Host *host = (Host *)user;
	/// This triggers again after first time need to filter
	host->discoveredHosts = discovered_hosts;

	// Needs to be expanded to multi host
	host->state = chiaki_discovery_host_state_string(discovered_hosts->state);
	printf("Discovered Host State:  %s\n", host->state.c_str());

	host->ps5 = chiaki_target_is_ps5(chiaki_discovery_host_system_version_target(discovered_hosts));
	printf("Is PS5:  %d\n", host->ps5);

	/// If read settings are empty (so nothing was read from file earlier)
	/// [N]
	if(host->gui->settings->all_read_settings.size() == 0)
	{	
		printf("No Hosts found in file\n");
		host->gui->setClientState(std::string("notreg"));
	}
	else /// [Y] - some hosts were found in settings (but are they this?)
	{
		for(uint8_t i=0; i<hosts_count; i++) /// for each Discovered host
		{
			ChiakiDiscoveryHost *dh = host->discoveredHosts+i;
			
			/// iterate read-hosts
			for(rpi_settings_host sh : host->gui->settings->all_read_settings)
			{
				/// Look for the 'id' in file, compare
				if(sh.id == dh->host_id)
				{
					printf("Discovery ID match\n");
					/// if id's match, check if theres a 'regist' entry?
					if(sh.regist == "") { /// NO not registered
						host->gui->setClientState(std::string("notreg"));
						return;
					} else { /// YES, found+registered
						/// set Status to Ready or Orange
						std::string state = chiaki_discovery_host_state_string(dh->state);
						host->gui->setClientState(state);
						/// copy this read-host to the active host list
						// lets do this with a checking function to not double up or append existing data
						host->gui->settings->all_validated_settings.push_back(sh);
						/// update host settings gui
						host->gui->UpdateSettingsGui();
						
					}
				}///END id match
			}

		}

	}

	/// gui pointer should be filled now so we can update the gui.
	/// host->gui->setClientState(host->state);
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
 
/// Session Main loop
void *play(void *user)
{	
	printf("Starting Play Thread\n");
	Host *host = (Host *)user;
	
	while(1)
	{	
		/// Setsu first (touchpad, rumble)
		host->io->SetsuPoll();
		
		/// SDL Gamepad (could do rumble instead?)
		host->io->HandleJoyEvent();
		
		/// Merge Setsu with Joy and send
		host->io->SendFeedbackState();

		SDL_Delay(4); ///ms
	}
}


Host::Host()
{
	//chiaki_log_init(&log, 14, NULL, NULL);  // 1, 4, 14
	//chiaki_log_init(&log, CHIAKI_LOG_ALL & ~CHIAKI_LOG_VERBOSE, NULL, NULL);
	
	///printf("Rpi Host created\n");
}

Host::~Host()
{	
	chiaki_session_stop(&session);
	chiaki_session_join(&session); //?
	//chiaki_opus_decoder_fini(&opus_decoder);
	
	printf("Rpi Host destroyed\n");
}

int Host::Init(IO *io)
{	
	this->io = io;
	chiaki_opus_decoder_init(&opus_decoder, &log);
	
	printf("END HostInit\n");
	return 0;
}


int Host::StartDiscoveryService()
{
	//typedef void (*ChiakiDiscoveryServiceCb)(ChiakiDiscoveryHost *hosts, size_t hosts_count, void *user);
	//discovery_cb = DiscoveryCB(void*);

	ChiakiDiscoveryServiceOptions options;
	options.ping_ms = PING_MS;
	options.hosts_max = HOSTS_MAX;
	options.host_drop_pings = DROP_PINGS;
	options.cb = Discovery;		// type  ChiakiDiscoveryServiceCb
	options.cb_user = this;		/// the Host
	
	
	
	//ORIG ChiakiDiscoveryService *service = new ChiakiDiscoveryService;
	service = new ChiakiDiscoveryService;
	///from discoverymanager
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = 0xffffffff; /// 255.255.255.255
	options.send_addr = reinterpret_cast<sockaddr *>(&addr);
	options.send_addr_size = sizeof(addr);
	
	///ChiakiConnectInfo connect_info;
	///int foundHost=0;
	///std::string state;  /// standby, etc
	
	int err = chiaki_discovery_service_init(service, &options, &log);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(&log, "DiscoveryManager failed to init Discovery Service");
		return -1;
	}
	
	printf("Discovery Service started\n");
	return 0;
}

// will need number for which discoveredHost
void Host::RegistStart(std::string accountID, std::string pin)
{	

	RpiSettings settings;
	accId_len =  settings.GetB64encodeSize(accountID.length());
	
	printf("AccID entered:  %s\n", accountID.c_str());
	
	/// Need to strip any trailing '=' signs
	if(accountID.back() == '=')
		accountID = accountID.substr(0, accountID.size()-1);
	
	///printf("NEW: %s\n", accountID.c_str());

	ChiakiErrorCode err = chiaki_base64_decode(
			accountID.c_str(), accountID.length(),
			regist_info.psn_account_id, &accId_len);
	if(err != CHIAKI_ERR_SUCCESS)
		printf("Base64 decode of AccountID failed\n");

	regist_info.target = chiaki_discovery_host_system_version_target(discoveredHosts);
	printf("Regist Target:  %d\n", regist_info.target);
	
	regist_info.psn_online_id = nullptr; /// using regist_info.psn_account_id for higher targets
	
	sscanf(pin.c_str(), "%" SCNu32, &regist_info.pin);
		printf("PIN int:  %d\n", regist_info.pin);
		
	regist_info.host = discoveredHosts->host_addr; /// ip
		printf("Host Addr:  %s\n", regist_info.host);
		
	regist_info.broadcast = false;
	
	chiaki_regist_start(&regist, &log, &regist_info, RegistEventCB, this);

	//sleep(3); // Need 3 here. But shouldn't it have it's own thread??
	
	printf("END RegistClick\n");
}


void Host::SendWakeup()
{	
	int isPS5=0;
	if(gui->settings->all_validated_settings.at(0).isPS5 == "1") isPS5 = 1;
	
	char out2[CHIAKI_SESSION_AUTH_SIZE] = {'0'};
	int stringlen = gui->settings->all_validated_settings.at(0).regist.length();
	strncpy(out2, gui->settings->all_validated_settings.at(0).regist.c_str(), stringlen);
	
	memcpy(&connect_info.regist_key, out2, CHIAKI_SESSION_AUTH_SIZE);
	///printf("Regist for wakeup:%s\n", connect_info.regist_key);
	
	uint64_t credential = (uint64_t)strtoull(connect_info.regist_key, NULL, 16);
	
	/// Currently only stored here
	printf("Sending Wakeup to:  %s\n", discoveredHosts->host_addr);
	chiaki_discovery_wakeup(&log, NULL,	discoveredHosts->host_addr, credential, isPS5);
}


int Host::StartSession()
{	
	ChiakiErrorCode err;
	
	/// Take Host[0] data from settings!

	char out2[CHIAKI_SESSION_AUTH_SIZE] = {'0'};
	int stringlen = gui->settings->all_validated_settings.at(0).regist.length();
	strncpy(out2, gui->settings->all_validated_settings.at(0).regist.c_str(), stringlen);
	
	memcpy(&connect_info.regist_key, out2, CHIAKI_SESSION_AUTH_SIZE);
	printf("Regist for session:%sEND\n", connect_info.regist_key);

	uint8_t rp_key[0x10] = {0};
	size_t rp_key_sz = sizeof(rp_key);
	
	chiaki_base64_decode(
		gui->settings->all_validated_settings.at(0).rp_key.c_str(), gui->settings->all_validated_settings.at(0).rp_key.length(),
		rp_key, &rp_key_sz);
	memcpy(&connect_info.morning, rp_key, rp_key_sz);
	///printf("Rp Key:%sEND\n", connect_info.morning);
	
	connect_info.host = discoveredHosts->host_addr;
	///printf("PS IP addr:  %s\n", connect_info.host);
	
	bool isPS5=false;
	if(gui->settings->all_validated_settings.at(0).isPS5 == "1") isPS5 = true;
	
	connect_info.ps5 = isPS5;
	connect_info.video_profile_auto_downgrade = false;
	connect_info.enable_keyboard = false;
	connect_info.video_profile.codec = gui->settings->GetChiakiCodec(gui->settings->all_validated_settings.at(0).sess.codec);
	ChiakiVideoResolutionPreset resolution_preset = gui->settings->GetChiakiResolution(gui->settings->all_validated_settings.at(0).sess.resolution);
	ChiakiVideoFPSPreset fps_preset = gui->settings->GetChiakiFps(gui->settings->all_validated_settings.at(0).sess.fps);
	chiaki_connect_video_profile_preset(&connect_info.video_profile, resolution_preset, fps_preset);
	connect_info.video_profile.bitrate = 15000;

	err = chiaki_session_init(&session, &connect_info, &log);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		printf("Failed to init session\n");
		return 1;
	}
	
	chiaki_opus_decoder_set_cb(&opus_decoder, InitAudioCB, AudioCB, io);
	chiaki_opus_decoder_get_sink(&opus_decoder, &audio_sink);
	chiaki_session_set_audio_sink(&session, &audio_sink);
	chiaki_session_set_video_sample_cb(&session, VideoCB, io); /// send to IO to execute

	/// Actually start		
	err = chiaki_session_start(&session);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		printf("ERROR chiaki session start\n");
		CHIAKI_LOGE(&log, "Session start failed: %s\n", chiaki_error_string(err));
		return 1;
	}
	printf("Session Bitrate:  %d\n", session.connect_info.video_profile.bitrate);
	///printf("Session target:  %d\n", session.target);

	chiaki_session_set_event_cb(&session, EventCB, this);
	
	chiaki_discovery_service_fini(service); /// service is not a pointer in 'Gui'
	
	printf("END chiaki session start\n");
	return 0;
}

int Host::RemoteStartSession()
{	
	ChiakiErrorCode err;
	
	// Need:
	// .regist
	// rp key
	// host addr / ip
	// isPS5
	// codec
	// resolution
	// fps

	//printf("REMOTE isPS5 from setting:  %s\n", settings.all_read_settings.at(0).isPS5.c_str() );
	ChiakiConnectInfo rem_connect_info;

	///  ORIG REGIST PART  ---------------------------------------------
	char out2[CHIAKI_SESSION_AUTH_SIZE] = {'0'};
	int stringlen = current_remote_settings.regist.length();
	strncpy(out2, current_remote_settings.regist.c_str(), stringlen);

	memcpy(&rem_connect_info.regist_key, out2, CHIAKI_SESSION_AUTH_SIZE);
	///printf("Regist for session:%sEND\n", rem_connect_info.regist_key);

	uint8_t rp_key[0x10] = {0};
	size_t rp_key_sz = sizeof(rp_key);
	
	chiaki_base64_decode(
		current_remote_settings.rp_key.c_str(), current_remote_settings.rp_key.length(),
		rp_key, &rp_key_sz);
	memcpy(&rem_connect_info.morning, rp_key, rp_key_sz);
	///printf("Rp Key:%sEND\n", connect_info.morning);

	std::string remoteExternalIp(std::to_string(gui->remoteIp[0])+"."+std::to_string(gui->remoteIp[1])+"."+std::to_string(gui->remoteIp[2])+"."+std::to_string(gui->remoteIp[3]));
	printf("IP FORM GUI:  %s\n", remoteExternalIp.c_str());
	rem_connect_info.host = remoteExternalIp.c_str();
	///printf("PS IP addr:  %s\n", connect_info.host); /// fred PS4:  192.168.1.65
	
	bool isPS5=false;
	if(current_remote_settings.isPS5 == "1") isPS5 = true;

	rem_connect_info.ps5 = isPS5;
	rem_connect_info.video_profile_auto_downgrade = false;
	rem_connect_info.enable_keyboard = false;
	
	RpiSettings tmpSettings;
	//rem_connect_info.video_profile.codec = settings.GetChiakiCodec(settings.all_read_settings.at(0).sess.codec);
	rem_connect_info.video_profile.codec = CHIAKI_CODEC_H264;
	//ChiakiVideoResolutionPreset resolution_preset = settings.GetChiakiResolution(settings.all_read_settings.at(0).sess.resolution);
	ChiakiVideoResolutionPreset resolution_preset = CHIAKI_VIDEO_RESOLUTION_PRESET_720p;
	ChiakiVideoFPSPreset fps_preset = tmpSettings.GetChiakiFps(current_remote_settings.sess.fps);
	chiaki_connect_video_profile_preset(&rem_connect_info.video_profile, resolution_preset, fps_preset);
	rem_connect_info.video_profile.bitrate = 15000;

	///session.connect_info = rem_connect_info;
	session.connect_info.ps5 = rem_connect_info.ps5;
	session.connect_info.video_profile_auto_downgrade = rem_connect_info.video_profile_auto_downgrade;
	session.connect_info.video_profile.codec = rem_connect_info.video_profile.codec;
	session.connect_info.enable_keyboard = rem_connect_info.enable_keyboard;
	session.connect_info.video_profile = rem_connect_info.video_profile;
	session.connect_info.video_profile.bitrate = rem_connect_info.video_profile.bitrate;

	err = chiaki_session_init(&session, &rem_connect_info, &log);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		printf("Failed to init session\n");
		return 1;
	}

	chiaki_opus_decoder_set_cb(&opus_decoder, InitAudioCB, AudioCB, io);
	chiaki_opus_decoder_get_sink(&opus_decoder, &audio_sink);
	chiaki_session_set_audio_sink(&session, &audio_sink);
	chiaki_session_set_video_sample_cb(&session, VideoCB, io); /// send to IO to execute

	/// Actually start		
	err = chiaki_session_start(&session);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		printf("ERROR chiaki session start\n");
		CHIAKI_LOGE(&log, "Session start failed: %s\n", chiaki_error_string(err));
		return 1;
	}
	printf("Session Bitrate:  %d\n", session.connect_info.video_profile.bitrate);
	///printf("Session target:  %d\n", session.target);

	chiaki_session_set_event_cb(&session, EventCB, this);
	
	chiaki_discovery_service_fini(service); /// service is not a pointer in 'Gui'
	
	printf("END chiaki session start\n");
	return 0;
}

void Host::ConnectionEventCB(ChiakiEvent *event)
{
	switch(event->type)
	{
		case CHIAKI_EVENT_CONNECTED:
			printf("ConnectionEventCB() CHIAKI_EVENT_CONNECTED\n");
			chiaki_thread_create(&play_th, play, this);
			break;
		
		case CHIAKI_EVENT_LOGIN_PIN_REQUEST:
			printf("ConnectionEventCB() CHIAKI_EVENT_LOGIN_PIN_REQUEST\n");
			//~ CHIAKI_LOGI(this->log, "EventCB CHIAKI_EVENT_LOGIN_PIN_REQUEST");
			//~ if(this->chiaki_even_login_pin_request_cb != nullptr)
				//~ this->chiaki_even_login_pin_request_cb(event->login_pin_request.pin_incorrect);
			break;
		case CHIAKI_EVENT_RUMBLE:
			printf("ConnectionEventCB() CHIAKI_EVENT_RUMBLE\n");
			//~ if(this->chiaki_event_rumble_cb != nullptr)
			//~ this->chiaki_event_rumble_cb(event->rumble.left, event->rumble.right);
			break;
		case CHIAKI_EVENT_QUIT:
			CHIAKI_LOGI(&log, "EventCB CHIAKI_EVENT_QUIT");
			//if(this->chiaki_event_quit_cb != nullptr)
			//	this->chiaki_event_quit_cb(&event->quit);
			break;
		case CHIAKI_EVENT_KEYBOARD_OPEN:
		case CHIAKI_EVENT_KEYBOARD_TEXT_CHANGE:
		case CHIAKI_EVENT_KEYBOARD_REMOTE_CLOSE:
			break;

	}
}

// Not used!?
//~ void Host::DiscoveryCB(ChiakiDiscoveryHost *discovered_host)
//~ {
	//~ printf("In DiscoveryCB()========================================================\n");
//~ }

std::string Host::GetHostRPKey(uint8_t rp_key[])
{
	//if(rp_key != nullptr)
	{
		size_t B64EncSize = ((4 * 0x10 / 3) + 3) & ~3;
		size_t rp_key_b64_sz = B64EncSize;
		//size_t rp_key_b64_sz = this->GetB64encodeSize(0x10);
		char rp_key_b64[rp_key_b64_sz + 1] = { 0 };
		ChiakiErrorCode err;
		err = chiaki_base64_encode(
			rp_key, 0x10,
			rp_key_b64, sizeof(rp_key_b64));

		if(CHIAKI_ERR_SUCCESS == err)
			return rp_key_b64;
		else
			CHIAKI_LOGE(&this->log, "Failed to encode rp_key to base64");

	}
	//else
	//	CHIAKI_LOGE(&this->log, "Cannot GetHostRPKey from nullptr host");

	return "";
}
