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


list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}"
	"${PROJECT_SOURCE_DIR}/../../tlibs2/cmake"
)


set(CMAKE_CXX_STANDARD 20)
add_compile_options(-std=c++20)
add_compile_options(-Wall -Wextra -Weffc++)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_compile_options(-g -ggdb)
	set(CMAKE_VERBOSE_MAKEFILE TRUE)

	message("Debug building for ${CMAKE_SYSTEM_NAME} systems.")

elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
	add_compile_options("-DNDEBUG")
	add_compile_options("-Wno-#pragma-messages")

	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		# see: https://gcc.gnu.org/onlinedocs/gcc/Developer-Options.html
		add_compile_options(-fcompare-debug-second)
	endif()

	message("Release building for ${CMAKE_SYSTEM_NAME} systems.")
endif()


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

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)
set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)


find_package(QCP)
if(QCP_FOUND)
	include_directories("${QCP_INCLUDE_DIRS}")
endif()



include_directories("${PROJECT_SOURCE_DIR}")

include_directories(
	SYSTEM "${Boost_INCLUDE_DIRS}/.." "../.."
	"ext" "ext/gemmi/include" "ext/gemmi/third_party"
	"../../ext/gemmi/include" "../../ext/gemmi/third_party"
)


add_executable(takin_magdyn
	main.cpp graph.h
	magdyn.cpp magdyn.h
	magdyn_file.cpp
	magdyn_plot.cpp magdyn_structplot.cpp
	../../tlibs2/libs/magdyn.h
	../../tlibs2/libs/qt/gl.cpp ../../tlibs2/libs/qt/gl.h
	../../tlibs2/libs/qt/glplot.cpp ../../tlibs2/libs/qt/glplot.h
)


if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_link_options(takin_magdyn
		# create an __info_plist section in the binary
		PRIVATE LINKER:-sectcreate,__TEXT,__info_plist,${PROJECT_SOURCE_DIR}/../../setup_mac/InfoMagDyn.plist
	)
endif()


target_link_libraries(takin_magdyn
	Qt5::Core Qt5::Gui Qt5::Widgets Qt5::OpenGL
	${QCP_LIBRARIES}
	${Boost_LIBRARIES}
	"${Lapacke_LIBRARIES}")
