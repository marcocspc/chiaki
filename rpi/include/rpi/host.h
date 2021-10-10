#ifndef RPI_HOST_H
#define RPI_HOST_H

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>

#include <chiaki/controller.h>
#include <chiaki/discovery.h>
#include <chiaki/log.h>
#include <chiaki/opusdecoder.h>
#include <chiaki/regist.h>
#include <chiaki/session.h>

#include <chiaki/discovery.h>
#include <chiaki/discoveryservice.h>

#include <rpi/io.h>

static void EventCB(ChiakiEvent *event, void *user);

class Settings;


class Host
{
	private:
		//friend class Settings;
		ChiakiConnectVideoProfile video_profile;
		// rp_key_data is true when rp_key, rp_regist_key, rp_key_type
		bool registered = false;
		bool rp_key_data = false;
		uint8_t rp_key[0x10] = {0};
		
		


	public:
		ChiakiLog log;
		//Settings *settings = nullptr;
		IO *io; ///should be private
		bool session_init = false;
		ChiakiSession session;
		ChiakiOpusDecoder opus_decoder;
		
		ChiakiThread play_th;  ///should be private? but harder to do
		
		Host();
		~Host();
		int Init(IO *io);
		int StartSession();
		void Play();
		bool SetHostRPKey(Host *host, std::string rp_key_b64);
		std::string GetHostRPKey(Host *host);
		
		void ConnectionEventCB(ChiakiEvent *event);

};
#endif
