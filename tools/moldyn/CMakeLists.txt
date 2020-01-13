#
# @author Tobias Weber
# @date dec-2019
# @license GPLv3, see 'LICENSE' file
#
# cmake -DCMAKE_BUILD_TYPE=Release ..
# mingw64-cmake -DCMAKE_BUILD_TYPE=Release ..
#

cmake_minimum_required(VERSION 3.0)
project(moldyn)

set(CMAKE_VERBOSE_MAKEFILE TRUE)


#find_package(Boost REQUIRED COMPONENTS system REQUIRED)
find_package(Boost REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)


set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)

set(CMAKE_CXX_STANDARD 17)
add_definitions(-std=c++2a -fconcepts)
add_definitions(${Boost_CXX_FLAGS})
add_definitions(-DBOOST_SYSTEM_NO_DEPRECATED -DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions(-D_GL_MAJ_VER=3 -D_GL_MIN_VER=2)


include_directories(
	"${PROJECT_SOURCE_DIR}" "${Boost_INCLUDE_DIRS}/.." "../.."
	"ext"
)


add_executable(moldyn
	moldyn.cpp moldyn.h
	../../libs/_cxx20/glplot.cpp ../../libs/_cxx20/glplot.h)

target_link_libraries(moldyn ${Boost_LIBRARIES})
qt5_use_modules(moldyn Core Gui Widgets OpenGL)
