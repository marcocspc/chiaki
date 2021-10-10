#include "rpi/settings.h"

using namespace std;

///	registkey: 677f884f
///	rpKey_morning: \xf1`\xf8\a\\\x9cT\x82z\x18\x5\xb9\xbc\x80\xca\x15

//char key_bytes_out[16] = {};
std::vector<char> KeyStr2ByteArray(std::string in_str) {
	
	/// handling:  "\xf1`\xf8\a\\\x9cT\x82z\x18\x5\xb9\xbc\x80\xca\x15"
	std::vector<char> key_bytes_out;
	int n;

	for (auto it = in_str.cbegin() ; it != in_str.cend(); ++it) {
		
		std::cout << *it << ' ';
		
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

std::vector<std::string> ReadRpiSettings()
{	
		
	char *homedir;
	if ((homedir = getenv("HOME")) == NULL) {
		homedir = getpwuid(getuid())->pw_dir;
	}
	
	const char* filename = strcat(homedir, "/.config/Chiaki/rpi_settings.conf");
	std::vector<std::string> in_strings;
	std::vector<std::string> out_strings;
	
	
	printf("Will try to read TEMP rpi settingfs file: %s\n", filename);
	
	/// OPen the file and read the data
	fstream file;
	file.open(filename, std::fstream::in);
	if (file.is_open()) {
		
      string tp;
      while(getline(file, tp)) {
         in_strings.push_back(tp);
      }
      
      file.close();
    } else printf("Couldn't open the file\n");
    
    
    /// Parsing the settings lines
    for(auto s : in_strings)
    {
		//cout << s << endl;
		int found = -1;
		
		if((found = s.find("registkey"))>-1)
		{
			//printf("Found registkey\n");
			string tmps = s.substr(s.find(":")+1, -1); /// all text after ":"
			tmps.erase( remove_if( tmps.begin(), tmps.end(), ::isspace ), tmps.end() );  /// strip white space
			out_strings.push_back(tmps);
			//cout << tmps << endl;
		}
		
		if((found = s.find("rpKey_morning"))>-1)
		{
			//printf("Found rpKey_morning\n");
			string tmps = s.substr(s.find(":")+1, -1); /// all text after ":"
			tmps.erase( remove_if( tmps.begin(), tmps.end(), ::isspace ), tmps.end() );  /// strip white space
			out_strings.push_back(tmps);
			//cout << tmps << endl;
		}
		
	}
    
    //printf("INSIDE[1]:  %s\n", out_strings[1].c_str());
    
    return out_strings;
}
