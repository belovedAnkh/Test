cmake_minimum_required(VERSION 3.21)

project(
	SexLabSceneFreeLook
	VERSION 1.0.0
	LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
	message(FATAL_ERROR "in-source builds are not allowed")
endif()

find_package(CommonLibSSE CONFIG REQUIRED)

add_commonlibsse_plugin(
	${PROJECT_NAME}
	NAME "SexLabSceneFreeLook"
	AUTHOR "belovedAnkh"
	VERSION ${PROJECT_VERSION}
	USE_ADDRESS_LIBRARY
	SOURCES
		src/Main.cpp
		src/Hooks.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /Zc:preprocessor /W4 /wd4100)
endif()
