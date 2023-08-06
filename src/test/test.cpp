#include <iostream>

#include <ultimaille/io/by_extension.h>

#define LUA_BINDER_IMPL
#include <lua/luabinder.h>
#include <lua/std.h>

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
Foo A;
Foo& getA() { return A; }

int main() {
	Lua::new_state();

	Lua::addClass<Foo>("Foo")
		.cons()
		.fun("f", &Foo::f)
		.fun("bar", &Foo::bar)
		.var("y", &Foo::y);
	Lua::addFunction("gen", gen);
	Lua::addFunction("getA", getA);

	Lua::callFile(PROJECT_DIR "/src/test/test.lua");
	std::cerr << A.y << std::endl;

	Lua::close();
	return 0;
}