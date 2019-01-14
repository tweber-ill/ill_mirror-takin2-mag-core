#
# @author Tobias Weber
# @date dec-2018
# @license GPLv3, see 'LICENSE' file
#

cmake_minimum_required(VERSION 3.0)
project(structfact)

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
	add_library(structfact SHARED
	structfact.cpp structfact.h
	../glplot/glplot.cpp ../glplot/glplot.h)
else()
	add_executable(structfact
		structfact.cpp structfact.h
		../glplot/glplot.cpp ../glplot/glplot.h)
endif()

target_link_libraries(structfact ${Boost_LIBRARIES})
qt5_use_modules(structfact Core Gui Widgets OpenGL)