#pragma once

namespace Config  {

struct ConfigData {
	char preferred_gpu[256];
};

extern ConfigData data;

void load();
void save();

}
