#
# @author Tobias Weber
# @date dec-2019
# @license GPLv3, see 'LICENSE' file
#
# cmake -DCMAKE_BUILD_TYPE=Release ..
# mingw64-cmake -DCMAKE_BUILD_TYPE=Release ..
#

project(moldyn)
cmake_minimum_required(VERSION 3.0)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/../../tlibs2/cmake")

set(CMAKE_VERBOSE_MAKEFILE TRUE)


#find_package(Boost REQUIRED COMPONENTS system REQUIRED)
find_package(Boost REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)
#find_package(Qhull)	# TODO


set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)

set(CMAKE_CXX_STANDARD 20)
add_definitions(-std=c++20)
add_definitions(${Boost_CXX_FLAGS})
add_definitions(-DBOOST_SYSTEM_NO_DEPRECATED -DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions(-D_GL_MAJ_VER=3 -D_GL_MIN_VER=2)


if(Qhull_FOUND)
	add_definitions(-DUSE_QHULL)
	include_directories("${Qhull_INCLUDE_DIRS}")
endif()


include_directories(
	"${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/.." "${PROJECT_SOURCE_DIR}/../.."
	"${Boost_INCLUDE_DIRS}/.." "ext"
)


add_executable(takin_moldyn
	moldyn.cpp moldyn.h
	../../tlibs2/libs/glplot.cpp ../../tlibs2/libs/glplot.h)

target_link_libraries(takin_moldyn ${Boost_LIBRARIES} ${Qhull_LIBRARIES})
qt5_use_modules(takin_moldyn Core Gui Widgets OpenGL)
