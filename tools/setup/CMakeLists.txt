#
# @author Tobias Weber
# @date Apr-2018
# @license GPLv3, see 'LICENSE' file
# @desc The present version was forked on 8-Nov-2018 from my privately developed "magtools" project (https://github.com/t-weber/magtools).
#
project(setup)
cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 20)
add_definitions(-std=c++20)
include_directories("${PROJECT_SOURCE_DIR}" "../..")


message("PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}.")
message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}.")
message("CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}.")

# -----------------------------------------------------------------------------
# setup tool
add_executable(setup convmag.cpp)
target_link_libraries(setup)


add_custom_command(TARGET setup POST_BUILD
	COMMAND cd ${CMAKE_SOURCE_DIR} && ${CMAKE_BINARY_DIR}/tools/setup/setup
	MAIN_DEPENDENCY setup)
# -----------------------------------------------------------------------------
