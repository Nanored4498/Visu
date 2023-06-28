cmake_minimum_required(VERSION 3.5.1)
project(Visu)

set(CMAKE_CXX_STANDARD 20)

find_package(Lua REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

if(CMAKE_CXX_COMPILER_ID MATCHES GNU OR CMAKE_CXX_COMPILER_ID MATCHES Clang)
	add_compile_options(
		-fdiagnostics-color
	)
	set(CMAKE_CXX_FLAGS_RELEASE "-O3 -Wall -DNDEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
endif()

add_subdirectory(ext)
add_subdirectory(shaders)

if(CMAKE_CXX_COMPILER_ID MATCHES GNU OR CMAKE_CXX_COMPILER_ID MATCHES Clang)
	add_compile_options(
		-Wall
		-Wextra
		-Wpedantic
		-pedantic-errors
	)
endif()

add_definitions(-DPROJECT_DIR=\"${CMAKE_SOURCE_DIR}\")
add_definitions(-DBUILD_DIR=\"${CMAKE_BINARY_DIR}\")
add_definitions(-DSHADER_DIR=\"${CMAKE_BINARY_DIR}/shaders\")
include_directories(
	${LUA_INCLUDE_DIR}
	ext/ultimaille
	src
)

file(GLOB SOURCES src/*.cpp src/graphics/*.cpp src/lua/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME}
	${LUA_LIBRARIES}
	ultimaille
	glfw
	vulkan
)


######## TEST #########
file(GLOB TEST_SOURCES src/test/*.cpp src/lua/*.cpp)
add_executable(Test ${TEST_SOURCES})
target_link_libraries(Test
	${LUA_LIBRARIES}
	ultimaille
)
########################