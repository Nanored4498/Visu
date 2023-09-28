#pragma once

namespace Config  {

struct ConfigData {
	char preferred_gpu[256];
	int window_x = 0, window_y = 0;
	int window_width = 800, window_height = 600;
};

extern ConfigData data;

void load();
void save();

}
