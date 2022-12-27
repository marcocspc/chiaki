#include <bitset> 	// test qbytestream
#include <string>
#include <iostream>	/// to_string()

#include <netdb.h>
//#include <stdio.h>
#include <netinet/in.h>

#include <rpi/host.h>
#include "rpi/settings.h"

#include <chiaki/base64.h> //move to .h

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
			new_host.sess.resolution = "720";
			new_host.sess.fps = "60";
			new_host.sess.audio_device = "";
			// oldbroke int ps5 = chiaki_target_is_ps5(chiaki_discovery_host_system_version_target(dh));//ChiakiDiscoveryHost
			int ps5 = chiaki_discovery_host_is_ps5(dh);
			new_host.isPS5 = std::to_string(ps5);
			new_host.nick_name = dh->host_name;
			new_host.id = dh->host_id; /// mac
			new_host.remote_ip = "0.0.0.0"; /// for remotes only
			new_host.rp_key = rp_key_str;
			new_host.regist = regist_key;
			
			host->gui->settings->all_validated_settings.push_back(new_host);
			host->gui->settings->WriteYaml(host->gui->settings->all_validated_settings, std::string(host->gui->home_dir + "/.config/Chiaki/Chiaki_rpi.conf"));
			host->gui->settings->all_read_settings = host->gui->settings->ReadSettingsYaml(host->gui->main_config);
			
			// Not correct! Needs to be Discovered's state
			host->gui->setClientState(std::string("ready"));
			host->gui->currentSettingsId=0;/// workaround for below
			host->gui->SwitchSettings(20);
			/// update host settings gui
			host->gui->UpdateSettingsGui();
			printf("Registration Success!!\n");
			break;
	}
	
	//chiaki_regist_stop(&gui->regist);
	//chiaki_regist_fini(&gui->regist);
}


static void rem_discovery_cb(ChiakiDiscoveryHost *discovery_host, void *user)
{
	Host* host = (Host*)user;
	
	host->remote_state = chiaki_discovery_host_state_string(discovery_host->state);
	/// printf("Discovered remote is:  %s\n", host->remote_state.c_str());
	
	//~ if(host->system_version)
		//~ CHIAKI_LOGI(log, "System Version:                    %s", host->system_version);

	//~ if(host->device_discovery_protocol_version)
		//~ CHIAKI_LOGI(log, "Device Discovery Protocol Version: %s", host->device_discovery_protocol_version);

	//~ if(host->host_request_port)
		//~ CHIAKI_LOGI(log, "Request Port:                      %hu", (unsigned short)host->host_request_port);

	host->discovered_remote.nick_name = std::string(discovery_host->host_name);
	
	std::string isPS5("0");
	if(discovery_host->host_type == std::string("PS5")) isPS5 = "1";
	host->discovered_remote.isPS5 = isPS5;
	int isPS5_ = stoi(isPS5);

	host->discovered_remote.id = discovery_host->host_id;

	//~ if(host->running_app_titleid)
		//~ CHIAKI_LOGI(log, "Running App Title ID:              %s", host->running_app_titleid);

	//~ if(host->running_app_name)
		//~ CHIAKI_LOGI(log, "Running App Name:                  %s%s", host->running_app_name, (strcmp(host->running_app_name, "Persona 5") == 0 ? " (best game ever)" : ""));

	/// Compare ID with all .remote files
	if(host->gui->remoteHostFiles.size() > 0)
	{
		for (uint8_t i = 0; i < host->gui->remoteHostFiles.size(); i++) 
		{
			std::string sel_remote = host->gui->remoteHostFiles.at(i);
			std::string remoteFile = host->gui->home_dir;
			remoteFile.append(std::string("/.config/Chiaki/") + sel_remote + std::string(".remote"));
			RpiSettings tmpsettings;
			std::vector<rpi_settings_host> tmpRemoteSettings;
			tmpRemoteSettings = tmpsettings.ReadSettingsYaml(remoteFile);
			
			/// now compare with the discovered remote from above
			if(host->discovered_remote.id == tmpRemoteSettings.at(0).id)  /// found/matched!
			{
				host->discoveredRemoteMatched = true;
				host->gui->SwitchRemoteImage(host->remote_state, isPS5_);
				host->gui->currentSettingsId=0;/// workaround for below
				host->gui->SwitchSettings(21);
			}
		}
	}
	
	printf("Remote Discovery Callback\n");
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


// NEEDS FIXING UP FOR MULTIPLE HOSTS
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
	
	// fails, not used in master 'gui' version
	//host->ps5 = chiaki_target_is_ps5(chiaki_discovery_host_system_version_target(discovered_hosts));
	//printf("Is PS5:  %d\n", host->ps5);
	
	// tmp fix
	host->ps5 = chiaki_discovery_host_is_ps5(discovered_hosts);
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
						host->gui->currentSettingsId=0;/// workaround for below
						host->gui->SwitchSettings(20);
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
	///typedef void (*ChiakiDiscoveryServiceCb)(ChiakiDiscoveryHost *hosts, size_t hosts_count, void *user);
	///discovery_cb = DiscoveryCB(void*);

	ChiakiDiscoveryServiceOptions options;
	options.ping_ms = PING_MS;
	options.hosts_max = HOSTS_MAX;
	options.host_drop_pings = DROP_PINGS;
	options.cb = Discovery;		/// type  ChiakiDiscoveryServiceCb
	options.cb_user = this;		/// the Host
	
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


int Host::DiscoverRemote()
{
	/// const char *host;
	const char *host = gui->GetIpAddr().c_str();

	ChiakiDiscovery discovery;
	ChiakiErrorCode err = chiaki_discovery_init(&discovery, &log, AF_INET); // TODO: IPv6
	if(err != CHIAKI_ERR_SUCCESS)
	{
		printf("Discovery init failed\n");
		return 1;
	}

	ChiakiDiscoveryThread thread;
	err = chiaki_discovery_thread_start(&thread, &discovery, rem_discovery_cb, this);//last was NULL
	if(err != CHIAKI_ERR_SUCCESS)
	{
		printf("Discovery thread init failed\n");
		chiaki_discovery_fini(&discovery);
		return 1;
	}

	struct addrinfo *host_addrinfos;
	///int r = getaddrinfo(arguments.host, NULL, NULL, &host_addrinfos);
	int r = getaddrinfo(host, NULL, NULL, &host_addrinfos);
	if(r != 0)
	{
		printf("getaddrinfo failed\n");
		return 1;
	}

	struct sockaddr *host_addr = NULL;
	socklen_t host_addr_len = 0;
	for(struct addrinfo *ai=host_addrinfos; ai; ai=ai->ai_next)
	{
		if(ai->ai_protocol != IPPROTO_UDP)
			continue;
		if(ai->ai_family != AF_INET) // TODO: IPv6
			continue;

		host_addr_len = ai->ai_addrlen;
		host_addr = (struct sockaddr *)malloc(host_addr_len);
		if(!host_addr)
			break;
		memcpy(host_addr, ai->ai_addr, host_addr_len);
	}
	freeaddrinfo(host_addrinfos);

	if(!host_addr)
	{
		printf("Failed to get addr for hostname\n");
		return 1;
	}

	ChiakiDiscoveryPacket packet;
	memset(&packet, 0, sizeof(packet));
	packet.cmd = CHIAKI_DISCOVERY_CMD_SRCH;
	packet.protocol_version = CHIAKI_DISCOVERY_PROTOCOL_VERSION_PS4;
	((struct sockaddr_in *)host_addr)->sin_port = htons(CHIAKI_DISCOVERY_PORT_PS4);
	err = chiaki_discovery_send(&discovery, &packet, host_addr, host_addr_len);
	if(err != CHIAKI_ERR_SUCCESS)
		printf("Failed to send discovery packet for PS4\n");
		
	packet.protocol_version = CHIAKI_DISCOVERY_PROTOCOL_VERSION_PS5;
	((struct sockaddr_in *)host_addr)->sin_port = htons(CHIAKI_DISCOVERY_PORT_PS5);
	err = chiaki_discovery_send(&discovery, &packet, host_addr, host_addr_len);
	if(err != CHIAKI_ERR_SUCCESS)
		printf("Failed to send discovery packet for PS5\n");
		
	sleep(1); /// work around is threading I guess
	
	printf("DiscoverRemote ran\n");
	return 0;
}


int Host::WakeupRemote()
{
	const char *host_ip = gui->GetIpAddr().c_str();
	int isPS5 = stoi(discovered_remote.isPS5);
	uint64_t credential = (uint64_t)strtoull(current_remote_settings.regist.c_str(), NULL, 16);
	
	chiaki_discovery_wakeup(&log, NULL, host_ip, credential, isPS5);
	discoveredRemoteMatched = false;

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
	
	//This will fail on PS5, not used in master 'gui' version
	//regist_info.target = chiaki_discovery_host_system_version_target(discoveredHosts);
	
	// tmp fix - won't work with older PS4s
	// florians version lets user set target system in the dialog.
	if(ps5) {
		regist_info.target = CHIAKI_TARGET_PS5_1;
		printf("Attempt for PS5\n");
	}
	else {
		regist_info.target = CHIAKI_TARGET_PS4_10;
		printf("Attempt for PS4\n");
	}
	
	regist_info.psn_online_id = nullptr; /// using regist_info.psn_account_id for higher targets
	
	sscanf(pin.c_str(), "%" SCNu32, &regist_info.pin);
		printf("PIN int:  %d\n", regist_info.pin);
		
	regist_info.host = discoveredHosts->host_addr; /// ip
		printf("Host Addr:  %s\n", regist_info.host);
		
	regist_info.broadcast = false;
	
	chiaki_regist_start(&regist, &log, &regist_info, RegistEventCB, this);
	
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
	int stringlen = session_settings.regist.length();
	strncpy(out2, session_settings.regist.c_str(), stringlen);
	
	memcpy(&connect_info.regist_key, out2, CHIAKI_SESSION_AUTH_SIZE);
	/// printf("Regist for session:%sEND\n", connect_info.regist_key);

	uint8_t rp_key[0x10] = {0};
	size_t rp_key_sz = sizeof(rp_key);
	
	chiaki_base64_decode(
		session_settings.rp_key.c_str(), session_settings.rp_key.length(),
		rp_key, &rp_key_sz);
	memcpy(&connect_info.morning, rp_key, rp_key_sz);
	///printf("Rp Key:%sEND\n", connect_info.morning);

	connect_info.host = session_settings.remote_ip.c_str();
	///printf("PS IP addr:  %s\n", connect_info.host);
	
	bool isPS5=false;
	if(session_settings.isPS5 == "1") isPS5 = true;
	
	connect_info.ps5 = isPS5;
	connect_info.video_profile_auto_downgrade = false;
	connect_info.enable_keyboard = false;
	ChiakiVideoResolutionPreset resolution_preset = gui->settings->GetChiakiResolution(session_settings.sess.resolution, stoi(session_settings.isPS5));
	ChiakiVideoFPSPreset fps_preset = gui->settings->GetChiakiFps(session_settings.sess.fps);
	chiaki_connect_video_profile_preset(&connect_info.video_profile, resolution_preset, fps_preset);
	connect_info.video_profile.codec = gui->settings->GetChiakiCodec(session_settings.sess.codec, stoi(session_settings.isPS5));
	if(connect_info.video_profile.codec == CHIAKI_CODEC_H264) printf("Requesting codec: h264\n");
	if(connect_info.video_profile.codec == CHIAKI_CODEC_H265) printf("Requesting codec: h265\n");
	if(connect_info.video_profile.codec == CHIAKI_CODEC_H265_HDR) printf("Requesting codec: h265HDR\n");
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
	///printf("Session Bitrate:  %d\n", session.connect_info.video_profile.bitrate);
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
