cmake_minimum_required(VERSION 3.2)

project(libswf)

if (WIN32)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_subdirectory(gameswf)
add_subdirectory(player)
add_subdirectory(zlib)