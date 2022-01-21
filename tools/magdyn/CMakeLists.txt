#
# @author Tobias Weber
# @date jan-2022
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
project(magdyn)
enable_language(CXX)

set(CMAKE_VERBOSE_MAKEFILE TRUE)

list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}"
	"${PROJECT_SOURCE_DIR}/../../tlibs2/cmake"
)


set(CMAKE_CXX_STANDARD 20)
add_compile_options(-std=c++20)
add_compile_options(-Wall -Wextra -Weffc++)


message("Building for ${CMAKE_SYSTEM_NAME} systems.")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# pass linker --subsystem option
	add_compile_options(-Wl,--subsystem,windows)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	add_compile_options(-mmacosx-version-min=10.10)
endif()


find_package(Boost REQUIRED)
add_definitions(${Boost_CXX_FLAGS})

find_package(Lapacke)
if(Lapacke_FOUND)
	message("Lapacke enabled.")
	add_definitions(-DUSE_LAPACK)
	include_directories("${Lapacke_INCLUDE_DIRS}")
else()
	message("Lapacke disabled.")
endif()

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)
set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)


find_package(QCP)
if(QCP_FOUND)
	include_directories("${QCP_INCLUDE_DIRS}")
endif()



include_directories(
	"${PROJECT_SOURCE_DIR}" "${Boost_INCLUDE_DIRS}/.." "../.."
)


add_executable(takin_magdyn
	main.cpp
	magdyn_gui.cpp magdyn_gui.h
	../../tlibs2/libs/magdyn.h
)


if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_link_options(takin_magdyn
		# create an __info_plist section in the binary
		PRIVATE LINKER:-sectcreate,__TEXT,__info_plist,${PROJECT_SOURCE_DIR}/../../setup_mac/InfoMagDyn.plist
	)
endif()


target_link_libraries(takin_magdyn
	Qt5::Core Qt5::Gui Qt5::Widgets
	${QCP_LIBRARIES}
	${Boost_LIBRARIES}
	"${Lapacke_LIBRARIES}")
