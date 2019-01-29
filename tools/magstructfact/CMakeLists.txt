#
# @author Tobias Weber
# @date jan-2019
# @license GPLv3, see 'LICENSE' file
#

cmake_minimum_required(VERSION 3.0)
project(magstructfact)

set(CMAKE_VERBOSE_MAKEFILE TRUE)
set(BUILD_LIB FALSE)

find_package(Boost REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)

set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)

set(CMAKE_CXX_STANDARD 17)
add_definitions(-std=c++17 -fconcepts)
add_definitions(${Boost_CXX_FLAGS})

include_directories("${PROJECT_SOURCE_DIR}" "${Boost_INCLUDE_DIRS}/.." "../.."
	"../../ext/gemmi/include" "../../ext/gemmi/third_party")

if(BUILD_LIB)
	set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

	add_definitions(-DBUILD_LIB)
	add_library(magstructfact SHARED
		magstructfact.cpp magstructfact.h
		../../libs/_cxx20/glplot.cpp ../../libs/_cxx20/glplot.h)
else()
	add_executable(magstructfact
		magstructfact.cpp magstructfact.h
		../../libs/_cxx20/glplot.cpp ../../libs/_cxx20/glplot.h)
endif()

target_link_libraries(magstructfact ${Boost_LIBRARIES})
qt5_use_modules(magstructfact Core Gui Widgets OpenGL)
