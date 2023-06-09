cmake_minimum_required(VERSION 3.5.1)
project(Visu)

set(CMAKE_CXX_STANDARD 20)

if(CMAKE_CXX_COMPILER_ID MATCHES GNU OR CMAKE_CXX_COMPILER_ID MATCHES Clang)
	add_compile_options(
		-fdiagnostics-color
	)
endif()

find_package(Lua REQUIRED)

add_subdirectory(ext/ultimaille)

if(CMAKE_CXX_COMPILER_ID MATCHES GNU OR CMAKE_CXX_COMPILER_ID MATCHES Clang)
	add_compile_options(
		-Wall
		-Wextra
	)
endif()

add_definitions(-DPROJECT_DIR=\"${CMAKE_SOURCE_DIR}\")

file(GLOB SOURCES *.cpp)
include_directories(
	${LUA_INCLUDE_DIR}
	ext/ultimaille
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME}
	${LUA_LIBRARIES}
	ultimaille
)
