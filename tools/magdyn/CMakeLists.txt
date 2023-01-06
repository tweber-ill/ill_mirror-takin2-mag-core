#
# @author Tobias Weber <tweber@ill.fr>
# @date jan-2022
# @license GPLv3, see 'LICENSE' file
#
# ----------------------------------------------------------------------------
# mag-core (part of the Takin software suite)
# Copyright (C) 2018-2022  Tobias WEBER (Institut Laue-Langevin (ILL),
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

set(QT_VER 5)

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-std=c++20)
add_compile_options(-Wall -Wextra -Weffc++)

list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}"
	"${PROJECT_SOURCE_DIR}/../../tlibs2/cmake"
)

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
	add_compile_options(-mmacosx-version-min=10.15)
endif()


find_package(Threads REQUIRED)

find_package(Boost REQUIRED)
set(Boost_USE_MULTITHREADED TRUE)
add_definitions(${Boost_CXX_FLAGS})

find_package(Lapacke REQUIRED)
add_definitions(-DUSE_LAPACK=1)
include_directories("${Lapacke_INCLUDE_DIRS}")

message("Trying to select Qt version ${QT_VER}.")
if(QT_VER EQUAL 6)
	find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets PrintSupport)
	include_directories("${Qt6Core_INCLUDE_DIRS}/..")
	list(APPEND QtLibraries Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGLWidgets Qt6::PrintSupport)
elseif(QT_VER EQUAL 5)
	find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets PrintSupport)
	include_directories("${Qt5Core_INCLUDE_DIRS}/..")
	list(APPEND QtLibraries Qt5::Core Qt5::Gui Qt5::Widgets Qt5::PrintSupport)
else()
	message(FATAL_ERROR "Tried to select a non-supported Qt version ${QT_VER}.")
endif()
set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)

find_package(QCP)
include_directories("${QCP_INCLUDE_DIRS}")

find_package(HDF5 COMPONENTS CXX)

if(HDF5_FOUND)
	message("Using HDF5 version ${HDF5_VERSION}.")
	include_directories("${HDF5_CXX_INCLUDE_DIRS}")
	add_definitions(-DUSE_HDF5)
else()
	message("Disabling HDF5 file support.")
	set(HDF5_CXX_LIBRARIES "")
endif()



include_directories("${PROJECT_SOURCE_DIR}")

include_directories(
	SYSTEM "${Boost_INCLUDE_DIRS}" "${Boost_INCLUDE_DIRS}/.." "../.."
	"ext" "ext/gemmi/include" "ext/gemmi/third_party"
	"../../ext/gemmi/include" "../../ext/gemmi/third_party"
)


add_executable(takin_magdyn
	main.cpp graph.h
	magdyn.cpp magdyn.h
	magdyn_gui.cpp magdyn_struct.cpp magdyn_file.cpp
	magdyn_disp.cpp magdyn_structplot.cpp
	defs.cpp defs.h
	table_import.cpp table_import.h

	../../tlibs2/libs/magdyn.h
	../../tlibs2/libs/qt/gl.cpp ../../tlibs2/libs/qt/gl.h
	../../tlibs2/libs/qt/glplot.cpp ../../tlibs2/libs/qt/glplot.h
	../../tlibs2/libs/qt/recent.cpp ../../tlibs2/libs/qt/recent.h

#	../../../paths/externals/qcustomplot/qcustomplot.cpp
)


if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_link_options(takin_magdyn
		# create an __info_plist section in the binary
		PRIVATE LINKER:-sectcreate,__TEXT,__info_plist,${PROJECT_SOURCE_DIR}/../../setup_mac/InfoMagDyn.plist
	)
endif()

target_link_libraries(takin_magdyn
	Threads::Threads
	${QtLibraries}
	${QCP_LIBRARIES}
	${Boost_LIBRARIES}
	${Lapacke_LIBRARIES}
	${HDF5_CXX_LIBRARIES}
)
