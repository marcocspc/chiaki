#include "rpi/settings.h"

#define SETTINGS_VERSION 1

using namespace std;



//char key_bytes_out[16] = {};
std::vector<char> KeyStr2ByteArray(std::string in_str) {

	std::vector<char> key_bytes_out;
	int n;

	for (auto it = in_str.cbegin() ; it != in_str.cend(); ++it) {
		
		///std::cout << *it << ' ';
		
		if(*it == 92) { ///92==backslash
			it++;
			if(*it == 120) {/// 120==x - so Case1, a hex will follow
				char hex[2] = {};
				it++;
				hex[0] = *it;
				it++;
				if(*it == 92) { /// oops backslash again, so single symbol hex
					///leaving hex[1] unset bad but works?
					std::istringstream(hex) >> std::hex >> n;
					//std::cout << std::dec << "Hex gives " << n << '\n';
					key_bytes_out.push_back(n);
					it--; /// rewind one step
				} else {
					hex[1] = *it;
					int n;
					std::istringstream(hex) >> std::hex >> n;
					//std::cout << std::dec << "Hex gives " << n << '\n';
					key_bytes_out.push_back(n);
				}
			} /// end Case1
			else { /// we assume regular escape char - Case2

				//printf("It:%d ", *it); /// prints correctly
				int n;
				
				/// \'=39
				if(*it==39) n=39;
				/// \"=34
				if(*it==34) n=34;
				/// \?=63
				if(*it==63) n=63;
				/// \\=92
				if(*it==92) n=92;
				/// \a=7 but 'a'=97
				if(*it==97) n=7;
				/// \b=8 but 'b'=98
				if(*it==98) n=8;
				/// \f=12 but 'f'=102
				if(*it==102) n=12;
				/// \n=10 but 'n'=110
				if(*it==110) n=10;
				/// \r=13 but 'r'=114
				if(*it==114) n=13;
				/// \t=9 but 't'=116
				if(*it==116) n=9;
				/// \v=11 but 'v'=118
				if(*it==118) n=11;
				
				//std::cout << std::dec << "Esc char is " << n << '\n';
				key_bytes_out.push_back(n);			
			} ///end Case2		
		} else { /// regular single symbol - Case3
			
			int n = *it;
			//printf("Std Char: %d \n", n);
			key_bytes_out.push_back(n);
		} /// end Case 3
	} /// End str iterator

	return key_bytes_out;
}

std::string GetValueForToken(std::string line, std::string token)
{	
	std::string tmps;
	int found = -1;
	
	if((found = line.find(token)) > -1)
	{	
		tmps = line.substr(line.find(":")+1, -1); /// all text after ":"
		tmps.erase( remove_if( tmps.begin(), tmps.end(), ::isspace ), tmps.end() );  /// strip white space
	}
	
	return tmps;
}

RpiSettings::RpiSettings()
{
}

RpiSettings::~RpiSettings()
{
}

void RpiSettings::PrintHostSettings(rpi_settings_host host1)
{
	/// Yaml uses spaces as tabs
	const char *tab1="  ";
	const char *tab2="    ";
	const char *tab3="      ";

	/// hosts
	int hostN=1;
	cout << "regist_hosts:" << endl;
		cout << tab1; cout << "- host: " 		<< hostN << endl;
			cout << tab2; cout << "nick: " 		<< host1.nick_name << endl;
			cout << tab2; cout << "id: " 		<< host1.id << endl;
			cout << tab2; cout << "rp_key: " 	<< host1.rp_key << endl;
			cout << tab2; cout << "regist: " 	<< host1.regist << endl;
			cout << tab2; cout << "session: " 	<< endl;
				cout << tab3; cout << "decoder: " 	<< host1.sess.decoder << endl;
				cout << tab3; cout << "codec: " 	<< host1.sess.codec << endl;
				cout << tab3; cout << "resolution: "<< host1.sess.resolution << endl;
				cout << tab3; cout << "fps: " 		<< host1.sess.fps << endl;
				cout << tab3; cout << "audio: " 	<< host1.sess.audio_device << endl;

}

/// not using actual libyaml to write. Seemed too complicated.
/// re-writing the full file at once from current settings in memory.
void RpiSettings::WriteYaml(std::vector<rpi_settings_host> all_host_settings)
{
	//const char* filename = "testyaml.yaml";
	
	std::string filename("/home/");
	filename.append(getenv("USER"));
	filename.append("/.config/Chiaki/Chiaki_rpi.conf");

	printf("Will try to read rpi settings file: %s\n", filename.c_str());
		
	ofstream out(filename);
	if (!out.is_open()) {
		printf("Could not open config file for writing: %s\n", filename.c_str());
		return;
	}
	
	/// Yaml needs spaces as tabs
	const char *tab1="  ";
	const char *tab2="    ";
	const char *tab3="      ";
	
	/// Current Settings Data
	int version = SETTINGS_VERSION;
	
	
	rpi_settings_host host1 = all_host_settings.at(0);
	int hostN=1;
	
	/// Start Writing it all at once
	out << "version: " << version << endl;
	out << endl;

	/// hosts
	out << "regist_hosts:" << endl;
		out << tab1; out << "- host: " 		<< hostN << endl;
			out << tab2; out << "nick: "	<< host1.nick_name << endl;
			out << tab2; out << "id: " 		<< host1.id << endl;
			out << tab2; out << "rp_key: "  << host1.rp_key << endl;
			out << tab2; out << "regist: "  << host1.regist << endl;
			out << tab2; out << "session: " << endl;
				out << tab3; out << "decoder: " 	<< host1.sess.decoder << endl;
				out << tab3; out << "codec: " 		<< host1.sess.codec << endl;
				out << tab3; out << "resolution: "  << host1.sess.resolution << endl;
				out << tab3; out << "fps: " 		<< host1.sess.fps << endl;
				out << tab3; out << "audio: " 		<< host1.sess.audio_device << endl;


	/// Finish
	out.flush();
	out.close();
	printf("Wrote yaml file\n");
}

/// Currently only supports one host in file
/// Reads straight into RpiSettings variable
/// Does not yet handle multiple hosts
void RpiSettings::ReadYaml()
{	
	std::string filename("/home/");
	filename.append(getenv("USER"));

	filename.append("/.config/Chiaki/Chiaki_rpi.conf");
	std::vector<std::string> in_strings;
	
	printf("Will try to read rpi settings file: %s\n", filename.c_str());
	
	/// OPen the file and read all the data
	fstream file;
	file.open(filename, std::fstream::in);
	if (file.is_open()) {
		
      string tp;
      while(getline(file, tp)) {
         in_strings.push_back(tp);
      }
      
      file.close();
    } else {
		printf("Couldn't open the file\n");
		return;
	}


	rpi_settings_host bufHost;
	int n_hosts=0;
    /// Parsing the settings lines
    for(auto s : in_strings)
    {
		std::string ret;
		
		ret = GetValueForToken(s, "host");
		if(ret != "") n_hosts++;
		
		/// Host settings
		ret = GetValueForToken(s, "nick");
		if(ret != "") bufHost.nick_name = ret;
		
		ret = GetValueForToken(s, "id");
		if(ret != "") bufHost.id = ret;
		
		ret = GetValueForToken(s, "rp_key");
		if(ret != "") bufHost.rp_key = ret;
		
		ret = GetValueForToken(s, "regist");
		if(ret != "") bufHost.regist = ret;
		
		/// Host Session settings
		ret = GetValueForToken(s, "decoder");
		if(ret != "") bufHost.sess.decoder = ret;
		
		ret = GetValueForToken(s, "codec");
		if(ret != "") bufHost.sess.codec = ret;
		
		ret = GetValueForToken(s, "resolution");
		if(ret != "") bufHost.sess.resolution = ret;
		
		ret = GetValueForToken(s, "fps");
		if(ret != "") bufHost.sess.fps = ret;
		
		ret = GetValueForToken(s, "audio");
		if(ret != "") bufHost.sess.audio_device = ret;
	}
	
	if(n_hosts > 0) {
		all_host_settings.push_back(bufHost);
		printf("Found settings for %d host(s)\n", n_hosts);
	}
	
	PrintHostSettings(bufHost);
	
	/// Finish
	
  return ;
}


size_t RpiSettings::GetB64encodeSize(size_t in)
{
	// calculate base64 buffer size after encode
	return ((4 * in / 3) + 3) & ~3;
}


