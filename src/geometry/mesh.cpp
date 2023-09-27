// Copyright (C) 2023, Coudert--Osmont Yoann
// SPDX-License-Identifier: AGPL-3.0-or-later
// See <https://www.gnu.org/licenses/>

#include "mesh.h"
#include "debug.h"

#include <cstring>
#include <fstream>
#include <sstream>

using namespace std;

Mesh readOBJ(const char* filename) {
	Mesh m;
	vector<vec2> VT;
	Attribute& polyline_id = m.edge_attributes.emplace_back("polyline_id", Attribute::INTEGER);
	Attribute& tex_coords = m.facet_corner_attributes.emplace_back("tex_coords", Attribute::VEC2);
	ifstream in(filename);
	if(in.fail()) THROW_ERROR(string("Failed to open ") + filename);
	string line;
	uint32_t line_index = 0;
	int64_t polyline_index = 0;
	while(!in.eof()) {
		++ line_index;
		getline(in, line);
		istringstream iss(line);
		string type_token;
		iss >> type_token;
		if(type_token == "v") {
			m.points.emplace_back();
			for(ptrdiff_t i=0; i < 3; ++i) iss >> m.points.back()[i];
		} else if(type_token == "vt") {
			VT.emplace_back();
			for(ptrdiff_t i=0; i < 2; ++i) iss >> VT.back()[i];
		} else if(type_token == "f") {
			while(true) {
				while(!iss.eof() && !isdigit(iss.peek())) iss.get();
				if(iss.eof()) break;
				uint32_t tmp; iss >> tmp;
				if(!tmp || tmp > m.points.size())
					THROW_ERROR("Invalid vertex index at line " + to_string(line_index) + " of file '" + filename + "'");
				m.facet_vertices.push_back(tmp - 1);
				tex_coords.uv.emplace_back();
				if(iss.peek() == '/') {
					iss.get();
					if(iss.peek() != '/') {
						iss >> tmp;
						if(!tmp || tmp > VT.size())
							THROW_ERROR("Invalid texture coordinate index index at line " + to_string(line_index) + " of file '" + filename + "'");
						tex_coords.uv.back() = VT[tmp-1];
					}
				}
			}
			m.facet_offset.push_back(m.facet_vertices.size());
		} else if(type_token == "l") {
			int i, j;
			for(iss >> i; iss >> j; i = j) {
				m.edge_vertices.push_back(i);
				m.edge_vertices.push_back(j);
				polyline_id.iu.push_back(polyline_index);
			}
			++ polyline_index;
		}
	}
	in.close();
	if(VT.empty()) m.facet_corner_attributes.clear();
	if(m.edge_vertices.empty()) m.edge_attributes.clear();
	return m;
}

Mesh readMesh(const char* filename) {
	size_t filename_len = strlen(filename);
	if(!strcmp(filename+filename_len-4, ".obj")) return readOBJ(filename);
	THROW_ERROR(string("mesh filename extension not recognized: ") + filename);
}