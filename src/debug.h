#pragma once

#include <exception>
#include <string>
#include <iostream>

struct AppError : public std::exception {
	const std::string m_msg;
	AppError(const std::string &msg, const std::string &file, int line):
		m_msg(msg + "\n    at " + file + ":" + std::to_string(line)) {}
	const char* what() const throw() { return m_msg.c_str(); }
};
#define THROW_ERROR(msg) throw AppError((msg), __FILE__, __LINE__)

#ifdef NDEBUG

	#define DEBUG_HERE
	#define DEBUG_MSG(...) {}
	#define PRINT_INFO(...)
	#define ASSERT(x)

#else

	template <class A, class... Args>
	void debugMessage(const A& a, const Args&... args) {
		std::cerr << a;
		((std::cerr << ' ' << args), ...);
		std::cerr << std::endl;
	}
	#define DEBUG_HERE std::cerr << "[DEBUG] " << __FILE__ << ':' << __LINE__ << std::endl
	#define DEBUG_MSG(...) debugMessage(__VA_ARGS__, "\n    at", __FILE__ ":" + std::to_string(__LINE__))
	#define PRINT_INFO(...) debugMessage(__VA_ARGS__)
	#define ASSERT(x) if(!(x)) THROW_ERROR("Assertion failed '"#x"'")

#endif