cmake_minimum_required(VERSION 3.5.1)
project(Visu)

set(CMAKE_CXX_STANDARD 20)

file(GLOB SOURCES *.cpp)

find_package(Lua REQUIRED)

include_directories(${LUA_INCLUDE_DIR})

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${LUA_LIBRARIES})
