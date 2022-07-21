#
# @author Tobias Weber
# @date dec-2019
# @license GPLv3, see 'LICENSE' file
#
# cmake -DCMAKE_BUILD_TYPE=Release ..
# mingw64-cmake -DCMAKE_BUILD_TYPE=Release ..
#
# ----------------------------------------------------------------------------
# mag-core (part of the Takin software suite)
# Copyright (C) 2018-2021  Tobias WEBER (Institut Laue-Langevin (ILL),
#                          Grenoble, France).
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# ----------------------------------------------------------------------------
#

project(moldyn)
cmake_minimum_required(VERSION 3.0)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/../../tlibs2/cmake")

set(CMAKE_VERBOSE_MAKEFILE TRUE)


#find_package(Boost REQUIRED COMPONENTS system REQUIRED)
find_package(Boost REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)
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
	"${Boost_INCLUDE_DIRS}" "${Boost_INCLUDE_DIRS}/.." "ext"
)


add_executable(takin_moldyn
	moldyn.cpp moldyn.h
	../../tlibs2/libs/qt/gl.cpp ../../tlibs2/libs/qt/gl.h
	../../tlibs2/libs/qt/glplot.cpp ../../tlibs2/libs/qt/glplot.h)

target_link_libraries(takin_moldyn ${Boost_LIBRARIES} ${Qhull_LIBRARIES} Qt5::Core Qt5::Gui Qt5::Widgets)
