#ifndef RPI_SETTINGS_H
#define RPI_SETTINGS_H

#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>	/// remove_if()
#include <cstring>

#include <sstream>		/// stringstream
#include <iomanip>		/// std::hex
//#include <bitset>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

std::vector<std::string> ReadRpiSettings();
std::vector<char> KeyStr2ByteArray(std::string in_str);

#endif
