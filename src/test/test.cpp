#include <iostream>

#include <ultimaille/io/by_extension.h>

#include "luabinder.h"

// #include <LuaBridge/LuaBridge.h>

using namespace std;

void check(lua_State *L, int err) {
	if(err != LUA_OK) {
		switch(err) {
		case LUA_ERRMEM:
			throw std::runtime_error("Out of memory");
			break;
		case LUA_ERRSYNTAX:
			throw std::logic_error(lua_tostring(L, 1));
			break;
		default:
			throw std::runtime_error("Unknown error code " + std::to_string(err) + ": " + lua_tostring(L, 1));
		}
	}
}

struct Foo {
	int y=0;
	~Foo() { cerr << "DESTRUCTOR " << this << endl; }
	Foo(const Foo &a) { cerr << "COPY CONST REF" << endl; }
	Foo(Foo &&a) { cerr << "COPY RVALUE" << endl; }
	Foo() { cerr << "CONSTRUCTION" << endl; }
	Foo& operator=(const Foo &a) { cerr << "OPERATOR= CONST REF" << endl; return *this; }
	void bar() {
		cout << "Foobar" << endl;
	}
	int f(int x) {
		return 2*x+y;
	}
};

Foo gen() { Foo a; a.y = 100; return a; }

int main() {
	Lua::new_state();

	Lua::Class<Foo> myluafoo("Foo");
	myluafoo.cons()
		.fun("f", &Foo::f)
		.fun("bar", &Foo::bar)
		.var("y", &Foo::y);
	Lua::addFunction("gen", gen);

	Lua::Class<UM::SurfaceAttributes> l_SurfaceAttributes("SurfaceAttributes");
	Lua::Class<UM::Triangles> l_Triangles("Triangles");
	l_Triangles.cons()
		.fun("nfacets", &UM::Triangles::nfacets);
	Lua::addFunction("read_by_extension_triangles", UM::read_by_extension<UM::Triangles>);

	// luabridge::getGlobalNamespace(Lua::L)
	// 	.beginClass<Foo>("Foo2")
	// 		.addFunction("bar", &Foo::bar)
	// 		.addFunction("f", &Foo::f)
	// 		.addProperty("y", &Foo::y)
	// 	.endClass()
	// 	.addFunction("gen2", gen);

	luaL_loadfile(Lua::L, PROJECT_DIR "/src/test/test.lua");
	if(lua_pcall(Lua::L, 0, LUA_MULTRET, 0) != LUA_OK) {
		const int top = lua_gettop(Lua::L);
		cerr << "Total on stack " << top << "\n";
		for(int i = 1; i <= top; ++i) {
			cerr << "[" << i << "] -> (" << lua_typename(Lua::L, lua_type(Lua::L, i)) << ") ";
			if(lua_type(Lua::L, i) == LUA_TSTRING) cerr << lua_tostring(Lua::L, i);
			cerr << "\n";
		}
		throw std::runtime_error(lua_tostring(Lua::L, 1));
	}

	lua_close(Lua::L);
	return 0;
}