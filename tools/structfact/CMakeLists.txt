#
# @author Tobias Weber
# @date dec-2018
# @license GPLv3, see 'LICENSE' file
#

cmake_minimum_required(VERSION 3.0)
project(structfact)

set(CMAKE_VERBOSE_MAKEFILE TRUE)
option(BUILD_LIB "build as dynamic library" FALSE)

find_package(Boost REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)

set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)

set(CMAKE_CXX_STANDARD 20)
add_definitions(-std=c++2a -fconcepts)
add_definitions(${Boost_CXX_FLAGS})

include_directories(
	"${PROJECT_SOURCE_DIR}" "${Boost_INCLUDE_DIRS}/.." "../.."
	"ext" "ext/gemmi/include" "ext/gemmi/third_party"
	"../../ext/gemmi/include" "../../ext/gemmi/third_party"
)


if(BUILD_LIB)
	set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

	add_definitions(-DBUILD_LIB)
	add_library(structfact SHARED
		structfact.cpp structfact.h
		../../libs/_cxx20/glplot.cpp ../../libs/_cxx20/glplot.h)
else()
	add_executable(structfact
		structfact.cpp structfact.h
		../../libs/_cxx20/glplot.cpp ../../libs/_cxx20/glplot.h)
endif()

target_link_libraries(structfact ${Boost_LIBRARIES})
qt5_use_modules(structfact Core Gui Widgets OpenGL)
