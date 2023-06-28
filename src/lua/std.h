#pragma once

#include "luabinder.h"

#include <vector>

namespace Lua {

template<typename A, typename B>
void bindPair(const char* name) {
	using P = std::pair<A, B>;
	addClass<P>(name)
		.cons()
		.var("first", &P::first)
		.var("second", &P::second);
}

template<typename T>
void bindVector(const char* name) {
	using V = std::vector<T>;
	addClass<V>(name)
		.cons()
		.fun("push_back", static_cast<void (V::*)(T const&)>(&V::push_back));
}

}