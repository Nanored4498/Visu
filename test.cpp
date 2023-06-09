#include <iostream>

#include "luabinder.h"

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
	int y;
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

Foo gen() { return Foo(); }

int main(int argc, char **argv) {
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);

	cerr << "========" << endl;
	Foo a = gen();
	cerr << "========" << endl;
	Foo *b = new Foo(gen());
	cerr << "========" << endl;

	LuaClass<Foo> myluafoo(L, "Foo");
	myluafoo.cons()
		.fun("f", &Foo::f)
		.fun("bar", &Foo::bar);
;
	luaL_loadstring(L, "x = Foo.new();\
		x:bar();\
		print(x:f(10));");
	if(lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) {
		const int top = lua_gettop(L);
		cerr << "Total on stack " << top << "\n";
		for(int i = 1; i <= top; ++i) {
			cerr << "[" << i << "] -> (" << lua_typename(L, lua_type(L, i)) << ") ";
			if(lua_type(L, i) == LUA_TSTRING) cerr << lua_tostring(L, i);
			cerr << "\n";
		}
		throw std::runtime_error(lua_tostring(L, 1));
	}

	lua_close(L);
	return 0;
}