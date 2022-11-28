#
# @author Tobias Weber <tweber@ill.fr>
# @date Oct-2018
# @license GPLv3, see 'LICENSE' file
# @desc The present version was forked on 8-Nov-2018 from my privately developed "magtools" project (https://github.com/t-weber/magtools).
#
# ----------------------------------------------------------------------------
# mag-core (part of the Takin software suite)
# Copyright (C) 2018-2021  Tobias WEBER (Institut Laue-Langevin (ILL),
#                          Grenoble, France).
# "magtools" project
# Copyright (C) 2017-2018  Tobias WEBER (privately developed).
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

cmake_minimum_required(VERSION 3.0)
project(pol)
enable_language(CXX)

set(QT_VER 5)

set(CMAKE_VERBOSE_MAKEFILE TRUE)
option(BUILD_LIB "build as dynamic library" FALSE)

# -----------------------------------------------------------------------------
# compiler settings
set(CMAKE_CXX_STANDARD 20)
add_compile_options(-std=c++20)
add_compile_options(-Wall -Wextra)
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# system specific settings
message("Building for ${CMAKE_SYSTEM_NAME} systems.")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# pass linker --subsystem option
	add_compile_options(-Wl,--subsystem,windows)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	add_compile_options(-mmacosx-version-min=10.15)
endif()
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# packages
find_package(Boost REQUIRED)
add_compile_options(${Boost_CXX_FLAGS})

message("Trying to select Qt version ${QT_VER}.")
if(QT_VER EQUAL 6)
	find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets)
	include_directories("${Qt6Core_INCLUDE_DIRS}/..")
	list(APPEND QtLibraries Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGLWidgets)
elseif(QT_VER EQUAL 5)
	find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)
	include_directories("${Qt5Core_INCLUDE_DIRS}/..")
	list(APPEND QtLibraries Qt5::Core Qt5::Gui Qt5::Widgets)
else()
	message(FATAL_ERROR "Tried to select a non-supported Qt version ${QT_VER}.")
endif()
set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)

include_directories("${PROJECT_SOURCE_DIR}"
	"${Boost_INCLUDE_DIRS}" "${Boost_INCLUDE_DIRS}/.." "../.."
)
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# linker settings
if(BUILD_LIB)
	set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

	add_definitions(-DBUILD_LIB)
	add_library(takin_pol SHARED pol.cpp
		../../tlibs2/libs/qt/gl.cpp ../../tlibs2/libs/qt/gl.h
		../../tlibs2/libs/qt/glplot.cpp ../../tlibs2/libs/qt/glplot.h)
else()
	add_executable(takin_pol pol.cpp
		../../tlibs2/libs/qt/gl.cpp ../../tlibs2/libs/qt/gl.h
		../../tlibs2/libs/qt/glplot.cpp ../../tlibs2/libs/qt/glplot.h)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_link_options(takin_pol
		# create an __info_plist section in the binary
		PRIVATE LINKER:-sectcreate,__TEXT,__info_plist,${PROJECT_SOURCE_DIR}/../../setup_mac/InfoPol.plist
	)
endif()

target_link_libraries(takin_pol
	${Boost_LIBRARIES}
	${QtLibraries}
)
# -----------------------------------------------------------------------------
