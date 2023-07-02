if(CMAKE_CXX_COMPILER_ID MATCHES GNU OR CMAKE_CXX_COMPILER_ID MATCHES Clang)
	add_compile_options(
		-w
	)
endif()
add_subdirectory(ultimaille)


###########
## IMGUI ##
###########

include_directories(
	imgui
)

add_library(imgui STATIC
	imgui/imgui.cpp 
	imgui/imgui_demo.cpp 
	imgui/imgui_draw.cpp 
	imgui/imgui_tables.cpp
	imgui/imgui_widgets.cpp
	imgui/backends/imgui_impl_glfw.cpp 
	imgui/backends/imgui_impl_vulkan.cpp
)
