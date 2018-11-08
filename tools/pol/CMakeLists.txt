#
# @author Tobias Weber
# @date Oct-2018
# @license GPLv3, see 'LICENSE' file
# @desc The present version was forked on 8-Nov-2018 from the privately developed "magtools" project (https://github.com/t-weber/magtools).
#

project(pol)
cmake_minimum_required(VERSION 3.0)

find_package(Boost REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)

set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)

set(CMAKE_CXX_STANDARD 17)
add_definitions(-std=c++17 -fconcepts)
add_definitions(${Boost_CXX_FLAGS})
include_directories("${PROJECT_SOURCE_DIR}" "${Boost_INCLUDE_DIRS}/.." "../..")

add_executable(pol pol.cpp
	../glplot/glplot.cpp ../glplot/glplot.h)
target_link_libraries(pol ${Boost_LIBRARIES})
qt5_use_modules(pol Core Gui Widgets OpenGL)
