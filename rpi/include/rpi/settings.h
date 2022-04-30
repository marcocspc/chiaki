#ifndef RPI_SETTINGS_H
#define RPI_SETTINGS_H

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>	/// remove_if()
#include <cstring>
#include <sys/stat.h>	/// check/make dir

#include <sstream>		/// stringstream
#include <iomanip>		/// std::hex

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <chiaki/base64.h>
#include <chiaki/session.h>

std::string GetValueForToken(std::string line, std::string token);
//std::vector<char> KeyStr2ByteArray(std::string in_str);

struct rpi_settings_session {
	std::string decoder;
	std::string codec;
	std::string resolution;
	std::string fps;
	std::string audio_device;
};

struct rpi_settings_host{
	std::string isPS5; /// 0,1
	std::string nick_name;
	std::string id; ///mac
	std::string rp_key;
	std::string regist;
	rpi_settings_session sess;
};

class RpiSettings
{
	public:
		RpiSettings();
		~RpiSettings();
		size_t GetB64encodeSize(size_t in);
		std::vector<rpi_settings_host>  ReadSettingsYaml(std::string filename);	// reads into 'all_host_settings'
		void WriteYaml(std::vector<rpi_settings_host> all_host_settings);
		void PrintHostSettings(rpi_settings_host host);
		void RefreshSettings(std::string setting, std::string choice);  /// both memory and file
		
		std::vector<rpi_settings_host> all_read_settings; 		// all host settings as read from .conf file
		std::vector<rpi_settings_host> all_validated_settings; 	// Read settings checked against Discovered hosts
		
		ChiakiCodec GetChiakiCodec(std::string choice);
		ChiakiVideoResolutionPreset GetChiakiResolution(std::string choice);
		ChiakiVideoFPSPreset GetChiakiFps(std::string choice);

	private:

};

#endif
