#pragma once

#include <cassert>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

namespace Lua {

template<typename T> struct Class;

///////////
// STACK //
///////////

template<typename T> struct Stack {
	static T& get(lua_State *L, int ind) { return *(T*)luaL_checkudata(L, ind, Class<T>::getName()); }
	template<typename G> static void add(lua_State *L, const G &x) {
		T* obj = (T*) lua_newuserdata(L, sizeof(T));
		new(obj) T(x());
		luaL_getmetatable(L, Class<T>::getName());
		lua_setmetatable(L, -2);
	}
};
template<> struct Stack<int> {
	static int get(lua_State *L, int ind) { return lua_tointeger(L, ind); }
	template<typename G> static void add(lua_State *L, const G &x) { lua_pushinteger(L, x()); }
};
template<> struct Stack<double> {
	static double get(lua_State *L, int ind) { return lua_tonumber(L, ind); }
	template<typename G> static void add(lua_State *L, const G &x) { lua_pushnumber(L, x()); }
};
template<> struct Stack<std::string> {
	static std::string get(lua_State *L, int ind) { return lua_tostring(L, ind); }
};

///////////
// CLASS //
///////////

template<typename T, typename... Args, int... inds>
int cons2(lua_State *L, const std::string &name, std::integer_sequence<int, inds...>) {
	T* obj = (T*) lua_newuserdata(L, sizeof(T));
	assert(obj);
	new(obj) T(Stack<Args>::get(L, inds + 1)...);
	luaL_getmetatable(L, name.c_str());
	lua_setmetatable(L, -2);
	return 1;
}

template<typename F, typename U, typename Ret, typename... Args, int... inds>
int callMetClosure(lua_State *L, F *g, std::integer_sequence<int, inds...>) {
	if constexpr (std::is_same_v<Ret, void>) {
		(Stack<U>::get(L, 1).**g)(Stack<Args>::get(L, inds + 2)...);
		return 0;
	} else {
		Stack<Ret>::add(L, [&](){ return (Stack<U>::get(L, 1).**g)(Stack<Args>::get(L, inds + 2)...); });
		return 1;
	}
}

template<typename T>
struct Class {
	Class(lua_State *L, const char* name) {
		Class<T>::L = L;
		Class<T>::name = name;
		luaL_newmetatable(L, name);
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, gc);
		lua_setfield(L, -2, "__gc");
		lua_setglobal(L, name);
	}

	template<typename... Args>
	Class<T>& cons(const char *name = "new") {
		struct Temp {
			static int cons(lua_State *L) { return cons2<T, Args...>(L, Class<T>::name, std::make_integer_sequence<int, sizeof...(Args)>{}); }
		};
		luaL_getmetatable(L, Class<T>::name.c_str());
		assert(lua_istable(L, -1));
		lua_pushcfunction(L, Temp::cons);
		lua_setfield(L, -2, name);
		lua_pop(L, 1);
		return *this;
	}

	template<typename U, typename Ret, typename... Args>
	Class<T>& fun(const char* name, Ret(U::*f)(Args...)) {
		using F = decltype(f);
		struct Temp {
			static int f(lua_State *L) {
				F *g = (F*) lua_touserdata(L, lua_upvalueindex(1));
				assert(g);
				return callMetClosure<F, U, Ret, Args...>(L, g, std::make_integer_sequence<int, sizeof...(Args)>{});
			}
		};
		luaL_getmetatable(L, Class<T>::name.c_str());
		F* g = (F*)lua_newuserdata(L, sizeof(F));
		*g = f;
		lua_pushcclosure(L, Temp::f, 1);
		lua_setfield(L, -2, name);
		lua_pop(L, 1);
		return *this;
	}

	static const char* getName() { return name.c_str(); }

private:
	static lua_State *L;
	static std::string name;

	static int gc(lua_State *L) {
		T* obj = (T*) luaL_checkudata(L, 1, name.c_str());
		obj->~T();
		return 0;
	}
};

template<typename T> lua_State* Class<T>::L = nullptr;
template<typename T> std::string Class<T>::name = "Lua Class not defined";

///////////////
// FUNCTIONS //
///////////////

template<typename F, typename Ret, typename... Args, int... inds>
int callFunClosure(lua_State *L, F *g, std::integer_sequence<int, inds...>) {
	if constexpr (std::is_same_v<Ret, void>) {
		(**g)(Stack<Args>::get(L, inds + 1)...);
		return 0;
	} else {
		Stack<Ret>::add(L, [&](){ return (**g)(Stack<Args>::get(L, inds + 1)...); });
		return 1;
	}
}

template<typename Ret, typename... Args>
void addFunction(lua_State *L, const char *name, Ret f(Args...)) {
	using F = decltype(f);
	struct Temp {
		static int f(lua_State *L) {
			F *g = (F*) lua_touserdata(L, lua_upvalueindex(1));
			assert(g);
			return callFunClosure<F, Ret, Args...>(L, g, std::make_integer_sequence<int, sizeof...(Args)>{});
		}
	};
	F* g = (F*)lua_newuserdata(L, sizeof(F));
	*g = f;
	lua_pushcclosure(L, Temp::f, 1);
	lua_setglobal(L, name);
}

}