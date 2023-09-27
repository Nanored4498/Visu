// Copyright (C) 2023, Coudert--Osmont Yoann
// SPDX-License-Identifier: AGPL-3.0-or-later
// See <https://www.gnu.org/licenses/>

#pragma once

#include <maths.h>

#include <cstdint>
#include <string>
#include <vector>

class Attribute {
public:
	std::string name;
	enum TYPE {
		INTEGER,
		SCALAR,
		VEC2
	} type;
	union {
		std::vector<int64_t> iu;
		std::vector<double> u;
		std::vector<vec2> uv;
	};

	Attribute(const std::string &name, TYPE type): name(name), type(type) {
		switch(type) {
			case INTEGER: new(&iu) decltype(iu)(); break;
			case SCALAR: new(&u) decltype(u)(); break;
			case VEC2: new(&uv) decltype(uv)(); break;
		}
	}

	Attribute(Attribute&& a): name(move(a.name)), type(a.type) {
		switch(type) {
			case INTEGER: new(&iu) decltype(iu)(move(a.iu)); break;
			case SCALAR: new(&u) decltype(u)(move(a.u)); break;
			case VEC2: new(&uv) decltype(uv)(move(a.uv)); break;
		}
	}

	~Attribute() {
		switch(type) {
			case INTEGER: iu.~vector(); break;
			case SCALAR: u.~vector(); break;
			case VEC2: uv.~vector(); break;
		}
	}
};

class Mesh {
public:
	// GEOMETRY
	std::vector<vec3> points;

	// TOPOLOGY
	std::vector<std::uint32_t> edge_vertices;
	std::vector<std::uint32_t> facet_vertices, facet_offset;

	// ATTRIBUTES
	std::vector<Attribute>
		point_attributes,
		edge_attributes,
		facet_attributes,
		facet_corner_attributes;
	
	Mesh(): facet_offset(1, 0u) {}

	inline std::size_t nverts() const { return points.size(); }
	inline std::size_t nfacets() const { return facet_offset.size()-1; }
	inline std::size_t nfacet_corners() const { return facet_vertices.size(); }

	inline std::uint32_t prev(const std::uint32_t f, const std::uint32_t fc) const {
		return fc == facet_offset[f] ? facet_offset[f+1]-1 : fc-1;
	}
	inline std::uint32_t next(const std::uint32_t f, const std::uint32_t fc) const {
		return fc+1 == facet_offset[f+1] ? facet_offset[f] : fc+1;
	}

	inline const vec3& corner_point(const std::uint32_t fc) const { return points[facet_vertices[fc]]; }
	vec3 corner_normal(const std::uint32_t f, const std::uint32_t fc) const {
		const std::uint32_t pfc = prev(f, fc);
		const std::uint32_t nfc = next(f, fc);
		const vec3 a = points[facet_vertices[pfc]] - points[facet_vertices[fc]];
		const vec3 b = points[facet_vertices[nfc]] - points[facet_vertices[fc]];
		return cross(a, b).normalize();
	}
};

Mesh readMesh(const char* filename);