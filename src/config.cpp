#include "config.h"

#include <cstring>
#include <fstream>
#include <string>

namespace Config  {

ConfigData data = {};

#define __config_load_str(line, key, val)\
	if(line.starts_with(key "=")) {\
		const std::size_t n = std::min(sizeof(val)-1, line.size() - sizeof(key));\
		strncpy(val, line.data() + sizeof(key), n);\
		val[n] = 0;\
	}

void load() {
	std::ifstream f(BUILD_DIR "/visu.ini");
	std::string line;
	while(std::getline(f, line)) {
		__config_load_str(line, "gfx:preferred_gpu", data.preferred_gpu);
	}
	f.close();
}

void save() {
	std::ofstream f(BUILD_DIR "/visu.ini");
	f << "gfx:preferred_gpu=" << data.preferred_gpu << '\n';
	f.close();
}

}