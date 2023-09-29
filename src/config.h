// Copyright (C) 2023, Coudert--Osmont Yoann
// SPDX-License-Identifier: AGPL-3.0-or-later
// See <https://www.gnu.org/licenses/>

#pragma once

namespace Config  {

struct ConfigData {
	char preferred_gpu[256];
	int window_x = 0, window_y = 0;
	int window_width = 800, window_height = 600;
	int style = 1;
	// TODO: Correct full screen bug
};

extern ConfigData data;

void load();
void save();

}
