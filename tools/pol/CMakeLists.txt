#
# @author Tobias Weber
# @date Oct-2018
# @license GPLv3, see 'LICENSE' file
# @desc The present version was forked on 8-Nov-2018 from my privately developed "magtools" project (https://github.com/t-weber/magtools).
#

cmake_minimum_required(VERSION 3.0)
project(pol)

set(CMAKE_VERBOSE_MAKEFILE TRUE)
option(BUILD_LIB "build as dynamic library" FALSE)

find_package(Boost REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)

set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-std=c++20)
add_compile_options(${Boost_CXX_FLAGS})
add_compile_options(-Wall -Wextra)

include_directories("${PROJECT_SOURCE_DIR}" "${Boost_INCLUDE_DIRS}/.." "../..")

if(BUILD_LIB)
	set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

	add_definitions(-DBUILD_LIB)
	add_library(takin_pol SHARED pol.cpp
		../../tlibs2/libs/glplot.cpp ../../tlibs2/libs/glplot.h)
else()
	add_executable(takin_pol pol.cpp
		../../tlibs2/libs/glplot.cpp ../../tlibs2/libs/glplot.h)
endif()

target_link_libraries(takin_pol ${Boost_LIBRARIES})
qt5_use_modules(takin_pol Core Gui Widgets OpenGL)
