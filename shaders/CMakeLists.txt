find_program(GLSL_COMPILER NAMES glslc HINTS Vulkan::glslc)

set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(SHADER_BINARY_DIR ${CMAKE_BINARY_DIR}/shaders)

file(GLOB SHADER_SOURCES
	${SHADER_SOURCE_DIR}/*.vert
	${SHADER_SOURCE_DIR}/*.frag)

add_custom_command(
	OUTPUT ${SHADER_BINARY_DIR}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
	COMMENT "Creating ${SHADER_BINARY_DIR}"
)

foreach(source IN LISTS SHADER_SOURCES)
	get_filename_component(FILENAME ${source} NAME)
	set(output ${SHADER_BINARY_DIR}/${FILENAME}.spv)
	add_custom_command(
		OUTPUT ${output}
    	VERBATIM
    	COMMAND ${GLSL_COMPILER} -o ${output} ${source}
    	DEPENDS ${source} ${SHADER_BINARY_DIR}
    	COMMENT "Compiling ${FILENAME}"
  	)
	list(APPEND SPV_SHADERS ${output})
endforeach()
message(${CMAKE_COMMAND})

add_custom_target(spv_shaders ALL DEPENDS ${SPV_SHADERS})