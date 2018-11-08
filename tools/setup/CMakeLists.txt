#
# @author Tobias Weber
# @date Apr-2018
# @license GPLv3, see 'LICENSE' file
# @desc The present version was forked on 8-Nov-2018 from the privately developed "magtools" project (https://github.com/t-weber/magtools).
#
project(setup)
cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
add_definitions(-std=c++17 -fconcepts)
include_directories("${PROJECT_SOURCE_DIR}" "../..")


# -----------------------------------------------------------------------------
# setup tool
add_executable(convmag
	convmag.cpp
)

target_link_libraries(convmag)
# -----------------------------------------------------------------------------
