#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <unordered_map>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

namespace Lua {

extern lua_State *L;
inline void new_state() {
	L = luaL_newstate();
	luaL_openlibs(L);
}
inline void close() { lua_close(L); }

inline bool call() {
	if(lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) {
		const int top = lua_gettop(L);
		std::cerr << "Total on stack " << top << "\n";
		for(int i = 1; i <= top; ++i) {
			std::cerr << "[" << i << "] -> (" << lua_typename(L, lua_type(L, i)) << ") ";
			if(lua_type(L, i) == LUA_TSTRING) std::cerr << lua_tostring(Lua::L, i);
			std::cerr << "\n";
		}
		return true;
	}
	return false;
}
inline bool callFile(const char* filename) { 
	luaL_loadfile(L, filename);
	return call();
}
inline bool callString(const char* code) { 
	luaL_loadstring(L, code);
	return call();
}


template<typename T> struct Class;

///////////
// STACK //
///////////

template<typename T> struct Stack { 
	static T& get(int ind) { return **(T**)luaL_checkudata(L, ind, Class<T>::getName()); }
	template<typename G> static void add(const G &x) {
		#ifndef NDEBUG
		if(!std::strcmp(Class<T>::getName(), "Lua Class not defined"))
			luaL_error(L, "tried to push a non-defined class [%s] on the stack...\n", typeid(T).name());
		#endif
		T** data = reinterpret_cast<T**>(lua_newuserdata(L, sizeof(T*) + sizeof(T)));
		new(data + 1) T(x());
		*data = reinterpret_cast<T*>(data + 1);
		luaL_getmetatable(L, Class<T>::getName());
		lua_setmetatable(L, -2);
	}
};
template<typename T> struct Stack<T&> {
	inline static T& get(int ind) { return Stack<T>::get(ind); }
	template<typename G> static void add(const G &x) requires std::is_fundamental_v<T> { Stack<T>::add(x); }
	template<typename G> static void add(const G &x) requires std::is_compound_v<T> {
		#ifndef NDEBUG
		if(!std::strcmp(Class<T>::getName(), "Lua Class not defined"))
			luaL_error(L, "tried to push a non-defined class [%s] on the stack...\n", typeid(T).name());
		#endif
		new(lua_newuserdata(L, sizeof(T*))) T*(&x());
		luaL_getmetatable(L, Class<T>::getName());
		lua_setmetatable(L, -2);
	}
};
template<> struct Stack<int> {
	static int get(int ind) { return lua_tointeger(L, ind); }
	template<typename G> static void add(const G &x) { lua_pushinteger(L, x()); }
};
template<> struct Stack<double> {
	static double get(int ind) { return lua_tonumber(L, ind); }
	template<typename G> static void add(const G &x) { lua_pushnumber(L, x()); }
};
template<> struct Stack<std::string> {
	static std::string get(int ind) { return lua_tostring(L, ind); }
};

////////////
// GLOBAL //
////////////

template<typename T>
void setGlobal(const char* name, T& x) {
	Stack<T&>::add([&]()->T& { return x; });
	lua_setglobal(L, name);
}

///////////
// CLASS //
///////////

template<typename T, typename... Args, int... inds>
int cons2(lua_State *L, const std::string &name, std::integer_sequence<int, inds...>) {
	T** data = reinterpret_cast<T**>(lua_newuserdata(L, sizeof(T*) + sizeof(T)));
	new(data + 1) T(Stack<Args>::get(L, inds + 1)...);
	*data = reinterpret_cast<T*>(data + 1);
	luaL_getmetatable(L, name.c_str());
	lua_setmetatable(L, -2);
	return 1;
}

template<typename U, typename Ret, typename... Args, int... inds>
int callMetClosure0(Ret(U::**g)(Args...), std::integer_sequence<int, inds...>) {
	if constexpr (std::is_same_v<Ret, void>) {
		(Stack<U>::get(1).**g)(Stack<Args>::get(inds + 2)...);
		return 0;
	} else {
		Stack<Ret>::add([&]()->Ret{ return (Stack<U>::get(1).**g)(Stack<Args>::get(inds + 2)...); });
		return 1;
	}
}

template<typename U, typename Ret, typename... Args>
int callMetClosure(lua_State *L) {
	auto *g = (Ret(U::**)(Args...)) lua_touserdata(L, lua_upvalueindex(1));
	assert(g);
	return callMetClosure0<U, Ret, Args...>(g, std::make_integer_sequence<int, sizeof...(Args)>{});
}

template<typename V>
void callGetter(int v) {
	// Stack<V&>::add([&]() { return Stack<T>::get(1).*(V T::*)v; });
	Stack<V&>::add([&]()->V& { return * (V*)(*(char**)lua_touserdata(L, 1) + v); }); 
}

template<typename V>
void callSetter(int v) {
	* (V*)(*(char**)lua_touserdata(L, 1) + v) = Stack<V>::get(3);
}

struct HashName {
	std::size_t operator()(const char* s) const {
		std::size_t h = *s;
		while(*(++s)) h = 257*h + *s;
		return h;
	}
};

struct PredName {
	bool operator()(const char *s, const char *t) const {
		return !strcmp(s, t);
	}
};

template<typename T>
struct Class {

	struct VarAccess {
		VarAccess(int v, void (*g)(int), void (*s)(int)): v(v), g(g), s(s) {}
		inline void get() const { g(v); }
		inline void set() const { s(v); }
	private:
		const int v;
		void (*g)(int);
		void (*s)(int);
	};
	
	Class(const char* name) {
		Class<T>::name = name;
		luaL_newmetatable(L, name);
		lua_pushcfunction(L, index);
		lua_setfield(L, -2, "__index");
		lua_pushcfunction(L, newindex);
		lua_setfield(L, -2, "__newindex");
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
		luaL_getmetatable(L, Class<T>::name.c_str());
		new(lua_newuserdata(L, sizeof(f))) decltype(f)(f);
		lua_pushcclosure(L, callMetClosure<U, Ret, Args...>, 1);
		lua_setfield(L, -2, name);
		lua_pop(L, 1);
		return *this;
	}

	template<typename U, typename Ret, typename... Args>
	inline Class<T>& fun(const char* name, Ret(U::*f)(Args...) const) { return fun(name, reinterpret_cast<Ret(U::*)(Args...)>(f)); }

	template<typename V>
	Class<T>& var(const char* name, V T::*v) {
		vars.emplace(name, VarAccess(*reinterpret_cast<int*>(&v), callGetter<V>, callSetter<V>));
		return *this;
	}

	static const char* getName() { return name.c_str(); }

private:
	static std::string name;
	static std::unordered_map<const char*, VarAccess, HashName, PredName> vars;

	static int gc(lua_State *L) {
		T** data = reinterpret_cast<T**>(luaL_checkudata(L, 1, name.c_str()));
		if(*data == reinterpret_cast<T*>(data+1)) (*data)->~T();
		return 0;
	}

	static int index(lua_State *L) {
		lua_getmetatable(L, 1);
		lua_pushvalue(L, 2);
		lua_rawget(L, 3);
		lua_remove(L, 3);
		if(lua_isnil(L, 3)) {
			const auto it = vars.find(lua_tostring(L, 2));
			if(it != vars.end()) {
				lua_pop(L, 1);
				it->second.get();
			}
		}
		return 1;
	}

	static int newindex(lua_State *L) {
		const auto it = vars.find(lua_tostring(L, 2));
		if(it == vars.end()) {
			lua_getmetatable(L, 1);
			lua_pushstring(L, "__name");
			lua_rawget(L, -2);
			luaL_error(L, "tried to set a non-defined variable %s of class %s...\n", lua_tostring(L, 2), lua_tostring(L, -1));
		}
		it->second.set();
		return 0;
	}
};

template<typename T> std::string Class<T>::name = "Lua Class not defined";
template<typename T> std::unordered_map<const char*, typename Class<T>::VarAccess, HashName, PredName> Class<T>::vars;

template<typename T>
inline Class<T> addClass(const char *name) { return Class<T>(name); }

///////////////
// FUNCTIONS //
///////////////

template<typename F, typename Ret, typename... Args, int... inds>
int callFunClosure(F *g, std::integer_sequence<int, inds...>) {
	if constexpr (std::is_same_v<Ret, void>) {
		(**g)(Stack<Args>::get(inds + 1)...);
		return 0;
	} else {
		Stack<Ret>::add([&]()->Ret{ return (**g)(Stack<Args>::get(inds + 1)...); });
		return 1;
	}
}

template<typename Ret, typename... Args>
void addFunction(const char *name, Ret f(Args...)) {
	using F = decltype(f);
	struct Temp {
		static int f(lua_State *L) {
			F *g = (F*) lua_touserdata(L, lua_upvalueindex(1));
			assert(g);
			return callFunClosure<F, Ret, Args...>(g, std::make_integer_sequence<int, sizeof...(Args)>{});
		}
	};
	new(lua_newuserdata(L, sizeof(F))) F(f);
	lua_pushcclosure(L, Temp::f, 1);
	lua_setglobal(L, name);
}

}

#ifdef LUA_BINDER_IMPL
lua_State *Lua::L = nullptr;
#endif