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
