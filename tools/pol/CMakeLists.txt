#
# @author Tobias Weber
# @date Oct-2018
# @license GPLv3, see 'LICENSE' file
# @desc The present version was forked on 8-Nov-2018 from the privately developed "magtools" project (https://github.com/t-weber/magtools).
#

cmake_minimum_required(VERSION 3.0)
project(pol)

set(CMAKE_VERBOSE_MAKEFILE TRUE)
set(BUILD_LIB TRUE)

find_package(Boost REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)

set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)

set(CMAKE_CXX_STANDARD 17)
add_definitions(-std=c++17 -fconcepts)
add_definitions(${Boost_CXX_FLAGS})

include_directories("${PROJECT_SOURCE_DIR}" "${Boost_INCLUDE_DIRS}/.." "../..")

if(BUILD_LIB)
	set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

	add_definitions(-DBUILD_LIB)
	add_library(pol SHARED pol.cpp
		../../libs/_cxx20/glplot.cpp ../../libs/_cxx20/glplot.h)
else()
	add_executable(pol pol.cpp
		../../libs/_cxx20/glplot.cpp ../../libs/_cxx20/glplot.h)
endif()

target_link_libraries(pol ${Boost_LIBRARIES})
qt5_use_modules(pol Core Gui Widgets OpenGL)
