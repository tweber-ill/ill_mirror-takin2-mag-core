#
# @author Tobias Weber <tweber@ill.fr>
# @date dec-2018
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
project(cif2xml)


set(CMAKE_VERBOSE_MAKEFILE TRUE)

find_package(Boost REQUIRED)


set(CMAKE_CXX_STANDARD 20)
add_definitions(-std=c++20)
add_definitions(${Boost_CXX_FLAGS})


include_directories(
	"${PROJECT_SOURCE_DIR}"
	"${Boost_INCLUDE_DIRS}" "${Boost_INCLUDE_DIRS}/.." "../.."
	"../../ext/gemmi/include" "../../ext/gemmi/third_party"
	"ext" "ext/gemmi/include" "ext/gemmi/third_party"
)


add_executable(takin_cif2xml
	cif2xml.cpp ../structfact/loadcif.h)
target_link_libraries(takin_cif2xml
#	-static-libstdc++ -static-libgcc
)


add_executable(takin_findsg
	findsg.cpp ../structfact/loadcif.h)
target_link_libraries(takin_findsg)


if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_link_options(takin_cif2xml
		# create an __info_plist section in the binary
		PRIVATE LINKER:-sectcreate,__TEXT,__info_plist,${PROJECT_SOURCE_DIR}/../../setup_mac/InfoCif2Xml.plist
	)

	target_link_options(takin_findsg
		# create an __info_plist section in the binary
		PRIVATE LINKER:-sectcreate,__TEXT,__info_plist,${PROJECT_SOURCE_DIR}/../../setup_mac/InfoFindSG.plist
	)
endif()
