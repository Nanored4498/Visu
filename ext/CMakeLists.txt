if(CMAKE_CXX_COMPILER_ID MATCHES GNU OR CMAKE_CXX_COMPILER_ID MATCHES Clang)
	add_compile_options(
		-w
	)
endif()
add_subdirectory(ultimaille)