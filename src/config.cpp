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
#define __config_load_int(line, key, val)\
	if(line.starts_with(key "=")) {\
		line.push_back(0);\
		val = atoi(line.c_str() + sizeof(key));\
	}

void load() {
	std::ifstream f(BUILD_DIR "/visu.ini");
	std::string line;
	while(std::getline(f, line)) {
		__config_load_str(line, "gfx:preferred_gpu", data.preferred_gpu)
		else __config_load_int(line, "window:x", data.window_x)
		else __config_load_int(line, "window:y", data.window_y)
		else __config_load_int(line, "window:width", data.window_width)
		else __config_load_int(line, "window:height", data.window_height)
	}
	f.close();
}

void save() {
	std::ofstream f(BUILD_DIR "/visu.ini");
	f << "gfx:preferred_gpu=" << data.preferred_gpu << '\n';
	f << "window:x=" << data.window_x << '\n';
	f << "window:y=" << data.window_y << '\n';
	f << "window:width=" << data.window_width << '\n';
	f << "window:height=" << data.window_height << '\n';
	f.close();
}

}