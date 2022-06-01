#
# @author Tobias Weber
# @date may-2022
# @license GPLv3, see 'LICENSE' file
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

cmake_minimum_required(VERSION 3.0)
project(bz)
enable_language(CXX)

list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}"
	"${PROJECT_SOURCE_DIR}/../../tlibs2/cmake"
)

set(CMAKE_VERBOSE_MAKEFILE TRUE)
option(BUILD_LIB "build as dynamic library" FALSE)

# system specific settings
message("Building for ${CMAKE_SYSTEM_NAME} systems.")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# pass linker --subsystem option
	add_compile_options(-Wl,--subsystem,windows)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	add_compile_options(-mmacosx-version-min=10.10)
endif()

find_package(Boost REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Svg)
find_package(Qhull)

if(Qhull_FOUND)
	add_definitions(-DUSE_QHULL)
	include_directories("${Qhull_INCLUDE_DIRS}")
endif()

set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-std=c++20)
add_compile_options(${Boost_CXX_FLAGS})
add_compile_options(-Wall -Wextra)

include_directories(
	"${PROJECT_SOURCE_DIR}" "${Boost_INCLUDE_DIRS}/.." "../.."
	"ext" "ext/gemmi/include" "ext/gemmi/third_party"
	"../../ext/gemmi/include" "../../ext/gemmi/third_party"
)


add_executable(takin_bz
	bz.cpp bz_file.cpp bz_calc.cpp
	bz_plot.cpp bz_main.cpp
	bz_ops.cpp bz.h globals.h
	plot_cut.cpp plot_cut.h
	../../tlibs2/libs/qt/gl.cpp ../../tlibs2/libs/qt/gl.h
	../../tlibs2/libs/qt/glplot.cpp ../../tlibs2/libs/qt/glplot.h)

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_link_options(takin_bz
		# create an __info_plist section in the binary
		PRIVATE LINKER:-sectcreate,__TEXT,__info_plist,${PROJECT_SOURCE_DIR}/../../setup_mac/InfoBZ.plist
	)
endif()

target_link_libraries(takin_bz
	${Boost_LIBRARIES}
	${Qhull_LIBRARIES}
	Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Svg
)
