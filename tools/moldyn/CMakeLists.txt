#
# @author Tobias Weber
# @date dec-2019
# @license GPLv3, see 'LICENSE' file
#

cmake_minimum_required(VERSION 3.0)
project(moldyn)

set(CMAKE_VERBOSE_MAKEFILE TRUE)

find_package(Boost REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)

set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)

set(CMAKE_CXX_STANDARD 20)
add_definitions(-std=c++2a -fconcepts)
add_definitions(${Boost_CXX_FLAGS})

include_directories(
	"${PROJECT_SOURCE_DIR}" "${Boost_INCLUDE_DIRS}/.." "../.."
	"ext"
)


add_executable(moldyn
	moldyn.cpp moldyn.h
	../../libs/_cxx20/glplot.cpp ../../libs/_cxx20/glplot.h)

target_link_libraries(moldyn ${Boost_LIBRARIES})
qt5_use_modules(moldyn Core Gui Widgets OpenGL)
