#ifndef RPI_SETTINGS_H
#define RPI_SETTINGS_H

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>	/// remove_if()
#include <cstring>

#include <sstream>		/// stringstream
#include <iomanip>		/// std::hex
//#include <bitset>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <chiaki/base64.h>
#include <chiaki/session.h>

//~ using std::cout;
//~ using std::endl;
//~ using std::ofstream;
//~ using std::fstream;
//~ using std::string;
//~ using std::vector;
//~ using std::array;


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
	std::string isPS5;
	std::string nick_name;
	std::string id; //mac or id?
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
		void ReadYaml();	/// reads into 'all_host_settings'
		void WriteYaml(std::vector<rpi_settings_host> all_host_settings);
		void PrintHostSettings(rpi_settings_host host);
		void RefreshSettings(std::string setting, std::string choice);  /// both memory and file
		ChiakiCodec GetChiakiCodec(std::string choice);
		ChiakiVideoResolutionPreset GetChiakiResolution(std::string choice);
		ChiakiVideoFPSPreset GetChiakiFps(std::string choice);
		
		std::vector<std::string> sessionSettingsNames;
		std::vector<rpi_settings_host> all_host_settings;
	
	private:
	
	
};


#endif
