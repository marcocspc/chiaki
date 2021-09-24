#include <iostream>
#include <stdio.h>
#include <string.h>

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

///LIB
#include <chiaki/common.h> 				/// ChiakiTarget
#include <chiaki/discovery.h>
#include <chiaki/discoveryservice.h>
#include <chiaki/session.h> 
#include <chiaki/opusdecoder.h>


#define PING_MS		500
#define HOSTS_MAX	3		// was 16
#define DROP_PINGS	3

///  Will need to read config prefs for registered hosts.
///
///



//~ static void Discovery(ChiakiDiscoveryHost *discovered_hosts, size_t hosts_count, void *user)
//~ {
	//~ DiscoveryManager *dm = (DiscoveryManager *)user;
	//~ for(size_t i = 0; i < hosts_count; i++)
	//~ {
		//~ dm->DiscoveryCB(discovered_hosts + i);
	//~ }
//~ }


static void MyCB()
{
	printf("Hi from Callback!\n");
}



// Change this to plain C code?
//
int main()
{
	printf("Hello!\n");
	
	ChiakiLog log;
	chiaki_log_init(&log, 4, NULL, NULL);  // 1 is the log level, 4 also good, try 14
	
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
	addr.sin_addr.s_addr = 0xffffffff; // 255.255.255.255
	options->send_addr = reinterpret_cast<sockaddr *>(&addr);
	options->send_addr_size = sizeof(addr);
	
	// segfault caused here
	ChiakiErrorCode err = chiaki_discovery_service_init(service, options, &log);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		//service_active = false;
		CHIAKI_LOGE(&log, "DiscoveryManager failed to init Discovery Service");
		return -1;
	}
	
	/// check for discoveries - doesn't find if sleeping
	for(int i=0; i<10; i++)
	{
		printf("host count:  %d\n", service->hosts_count);
		
		if(service->hosts_count > 0)
		{	
			const char* state = chiaki_discovery_host_state_string(service->hosts->state);
			printf("State:  %s\n", state );
			
			/// Get MAC
			if(service->hosts->host_id)
			{	
				/// MAC 70:9E:29:C0:51:9B
				printf("Host ID:  %s\n", service->hosts->host_id);
			}
			
			printf("Host Addr:  %s\n", service->hosts->host_addr);
			
			
			/// Try start a Session  (we can monitor the PS4 console)

			ChiakiTarget target = CHIAKI_TARGET_PS4_UNKNOWN;  ///will be filled in. console+firmware, I think. An Enum
			target = chiaki_discovery_host_system_version_target(service->hosts);
			if(target == CHIAKI_TARGET_PS4_UNKNOWN)
				printf("Oh No target still Unknown!\n");
		
			ChiakiSession *session = new ChiakiSession;
			ChiakiConnectInfo *connect_info = new ChiakiConnectInfo;  ///from session.h
			ChiakiOpusDecoder opus_decoder;
			
			ChiakiConnectVideoProfile videoprofile;
			videoprofile.width = 1280;
			videoprofile.width = 720;
			videoprofile.max_fps = 60;
			videoprofile.bitrate = 15000;
			videoprofile.codec = CHIAKI_CODEC_H264;   //CHIAKI_CODEC_H264
			printf("Video profile set\n");
			
			//~ typedef struct chiaki_connect_info_t
			//~ {
				//~ bool ps5;
				//~ const char *host; // null terminated, mutable pointer to an immutable character/string
				//~ char regist_key[CHIAKI_SESSION_AUTH_SIZE]; // must be completely filled (pad with \0)
				//~ uint8_t morning[0x10];
				//~ ChiakiConnectVideoProfile video_profile;
				//~ bool video_profile_auto_downgrade; // Downgrade video_profile if server does not seem to support it.
				//~ bool enable_keyboard;
			//~ } ChiakiConnectInfo;
			
			//~ const char* is a mutable pointer to an immutable character/string.
			//~ You cannot change the contents of the location(s) this pointer points to. 
			//~ Also, compilers are required to give error messages when you try to do so. 
			//~ For the same reason, conversion from const char * to char* is deprecated.
			

			//char registkey[CHIAKI_SESSION_AUTH_SIZE] = "677f884f";  ///an array is not a modifyable value
			std::string registkey = "677f884f\0\0\0\0\0\0\0";  //from settings file
			//std::string hostid = "709E29C0519B";  //from settings file
			char *rpKey_morning = "\xf1`\xf8\a\\\x9cT\x82z\x18\x5\xb9\xbc\x80\xca\x15";
			
			connect_info->ps5 = false;

			connect_info->host = service->hosts->host_addr;	// PS4 IP addr
			printf("HostID used:  %s\n", connect_info->host);
				
			strcpy(connect_info->regist_key, registkey.c_str());  //from settings file, char[16]
			
			memcpy(connect_info->morning, rpKey_morning, sizeof(connect_info->morning)); //from settings file,

			connect_info->video_profile = videoprofile;		// CHIAKI_CODEC_H264 = 0,
			connect_info->video_profile_auto_downgrade = true;
			connect_info->enable_keyboard = true;
			////////////////////////////////////////////////////////
			//streamsession.cpp start goes like:
			//1. ffmeg_decode_init()
			//2. chiaki_opus_decoder_init()
			//3. chiaki_session_init()
			//4. more opus init things
			//5. chiaki_session_set_video_sample_cb()
			//6. chiaki_session_set_event_cb()
			//7. enable setsu
			
			/// from switc main.cpp - well it runs not sure what it does
			ChiakiErrorCode err = chiaki_lib_init();
			if(err != CHIAKI_ERR_SUCCESS)
			{	
				printf("ERROR Chiaki lib init\n");
				CHIAKI_LOGE(&log, "Chiaki lib init failed: %s\n", chiaki_error_string(err));
				return 1;
			}
				
			// init controller states
			chiaki_controller_state_set_idle(&session->controller_state);
				
			err = chiaki_session_init(session, connect_info, &log);
			if(err != CHIAKI_ERR_SUCCESS)
			{
				printf("ERROR chiaki session init\n");
				CHIAKI_LOGE(&log, "Chiaki session init failed: %s\n", chiaki_error_string(err));
				return 1;
			}
			
			chiaki_opus_decoder_init(&opus_decoder, &log);
			
			
			//~ chiaki_opus_decoder_set_cb(&this->opus_decoder, InitAudioCB, AudioCB, user);
			//~ chiaki_opus_decoder_get_sink(&this->opus_decoder, &audio_sink);
			//~ chiaki_session_set_audio_sink(&this->session, &audio_sink);
			//~ chiaki_session_set_video_sample_cb(&this->session, VideoCB, user);
			//~ chiaki_session_set_event_cb(&this->session, EventCB, this);

			//chiaki_session_set_event_cb(session, EventCb, this);
			
			
			/// //////////////////  END INITS and SETUPS ???
			
			
			err = chiaki_session_start(session);
			if(err != CHIAKI_ERR_SUCCESS)
			{
				printf("ERROR chiaki session start\n");
				CHIAKI_LOGE(&log, "Session start failed: %s\n", chiaki_error_string(err));
				return 1;
			}
			printf("After chiaki session start\n");
			// Now doing chiaki_stream_connection_run
			// error in streamconnection.c ln440
			//  [I] Remote disconnected from StreamConnection with reason "Nagare did not init!"
			//~ [E] StreamConnection didn't receive streaminfo
			//~ [E] Remote disconnected from StreamConnectio
			
			// next in stream SHOULD be    received: 0, lost: 0  in first is ok.
			//~ [V] Sending Congestion Control Packet, received: 0, lost: 0
			//~ [D] StreamConnection received audio header:
			//~ [D] offset 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f  0123456789abcdef
			//~ [D]      0 02 10 00 00 bb 80 00 00 01 e0 00 00 00 01       ..............  
			//~ [I] Audio Header:
			//~ [I]   channels = 2
			//~ [I]   bits = 16
			//~ [I]   rate = 48000
			//~ [I]   frame size = 480
			//~ [I]   unknown = 1
			//~ [I] ChiakiOpusDecoder initialized
			
			
			
			
			
			
			
			
			
			break;
		}
		

		sleep(1);
	}/// END for 10 loop
	
	// try wakeup - from discovery.h
	//CHIAKI_EXPORT ChiakiErrorCode chiaki_discovery_wakeup(ChiakiLog *log, ChiakiDiscovery *discovery, const char *host, uint64_t user_credential, bool ps5);

	
	
	while(1) sleep(1);
	
	
	/// Finish
	chiaki_discovery_service_fini(service);
	//hosts = {};
	//emit HostsUpdated();
	
	
	printf("END rpidrm\n");
	return 0;
}
