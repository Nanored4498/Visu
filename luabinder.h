#pragma once

#include <cassert>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

template<typename T> struct LuaClass;

// STACK
template<typename T> struct LuaStack {
	static T& get(lua_State *L, int ind) { return *(T*)luaL_checkudata(L, ind, LuaClass<T>::getName()); }
	static void add(lua_State *L, T x) { assert(false); }
};
template<> struct LuaStack<int> {
	static int get(lua_State *L, int ind) { return lua_tointeger(L, ind); }
	static void add(lua_State *L, int x) { lua_pushinteger(L, x); }
};
template<> struct LuaStack<double> {
	static double get(lua_State *L, int ind) { return lua_tonumber(L, ind); }
	static void add(lua_State *L, double x) { lua_pushnumber(L, x); }
};
template<> struct LuaStack<std::string> {
	static std::string get(lua_State *L, int ind) { return lua_tostring(L, ind); }
	static void add(lua_State *L, const std::string &x) { assert(false); }
};

template<typename T, typename... Args, int... inds>
int cons2(lua_State *L, const std::string &name, std::integer_sequence<int, inds...>) {
	T* obj = (T*) lua_newuserdata(L, sizeof(T));
	assert(obj);
	*obj = T(LuaStack<Args>::get(L, inds + 1)...);
	luaL_getmetatable(L, name.c_str());
	lua_setmetatable(L, -2);
	return 1;
}

template<typename F, typename U, typename Ret, typename... Args, int... inds>
Ret callClosure(lua_State *L, F *g, std::integer_sequence<int, inds...>) {
	return (LuaStack<U>::get(L, 1).**g)(LuaStack<Args>::get(L, inds + 2)...);
}

template<typename T>
struct LuaClass {
	LuaClass(lua_State *L, const std::string &name) {
		LuaClass<T>::L = L;
		LuaClass<T>::name = name;
		luaL_newmetatable(L, name.c_str());
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, gc);
		lua_setfield(L, -2, "__gc");
		lua_setglobal(L, name.c_str());
	}

	template<typename... Args>
	LuaClass<T>& cons(const std::string &name = "new") {
		struct Temp {
			static int cons(lua_State *L) { return cons2<T, Args...>(L, LuaClass<T>::name, std::make_integer_sequence<int, sizeof...(Args)>{}); }
		};
		luaL_getmetatable(L, LuaClass<T>::name.c_str());
		assert(lua_istable(L, -1));
		lua_pushcfunction(L, Temp::cons);
		lua_setfield(L, -2, name.c_str());
		lua_pop(L, 1);
		return *this;
	}

	template<typename U, typename Ret, typename... Args>
	LuaClass<T>& fun(const std::string &name, Ret(U::*f)(Args...)) {
		using F = decltype(f);
		struct Temp {
			static int f(lua_State *L) {
				F *g = (F*) lua_touserdata(L, lua_upvalueindex(1));
				assert(g);
				if constexpr (std::is_same_v<Ret, void>) {
					callClosure<F, U, Ret, Args...>(L, g, std::make_integer_sequence<int, sizeof...(Args)>{});
					return 0;
				} else {
					LuaStack<Ret>::add(L, callClosure<F, U, Ret, Args...>(L, g, std::make_integer_sequence<int, sizeof...(Args)>{}));
					return 1;
				}
			}
		};
		luaL_getmetatable(L, LuaClass<T>::name.c_str());
		F* g = (F*)lua_newuserdata(L, sizeof(F));
		*g = f;
		lua_pushcclosure(L, Temp::f, 1);
		lua_setfield(L, -2, name.c_str());
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

template<typename T> lua_State* LuaClass<T>::L = nullptr;
template<typename T> std::string LuaClass<T>::name = "Lua Class not defined";
