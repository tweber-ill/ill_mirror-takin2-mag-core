#
# @author Tobias Weber <tweber@ill.fr>
# @date may-2022
# @license GPLv3, see 'LICENSE' file
#
# ----------------------------------------------------------------------------
# mag-core (part of the Takin software suite)
# Copyright (C) 2018-2023  Tobias WEBER (Institut Laue-Langevin (ILL),
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

set(QT_VER 5)

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-std=c++20)
add_compile_options(-Wall -Wextra)

list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}"
	"${PROJECT_SOURCE_DIR}/../../tlibs2/cmake"
)

set(CMAKE_VERBOSE_MAKEFILE TRUE)
option(USE_SCRIPTING "use scripting" FALSE)

# system specific settings
message("Building for ${CMAKE_SYSTEM_NAME} systems.")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# pass linker --subsystem option
	add_compile_options(-Wl,--subsystem,windows)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	add_compile_options(-mmacosx-version-min=10.15)
endif()

find_package(Boost REQUIRED COMPONENTS program_options)
add_compile_options(${Boost_CXX_FLAGS})

find_package(Qhull)
if(Qhull_FOUND)
	add_definitions(-DUSE_QHULL)
	include_directories("${Qhull_INCLUDE_DIRS}")
endif()

message("Trying to select Qt version ${QT_VER}.")
if(QT_VER EQUAL 6)
	find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets Svg)
	include_directories("${Qt6Core_INCLUDE_DIRS}/..")
	list(APPEND QtLibraries Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGLWidgets Qt6::Svg)
elseif(QT_VER EQUAL 5)
	find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Svg)
	include_directories("${Qt5Core_INCLUDE_DIRS}/..")
	list(APPEND QtLibraries Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Svg)
else()
	message(FATAL_ERROR "Tried to select non-supported Qt version ${QT_VER}.")
endif()
set(CMAKE_AUTOUIC TRUE)
set(CMAKE_AUTOMOC TRUE)

include_directories(
	"${PROJECT_SOURCE_DIR}"
	"${Boost_INCLUDE_DIRS}" "${Boost_INCLUDE_DIRS}/.." "../.."
	"ext" "ext/gemmi/include" "ext/gemmi/third_party"
	"../../ext/gemmi/include" "../../ext/gemmi/third_party"
)


add_executable(takin_bz
	bz.cpp bz_file.cpp bz_calc.cpp
	bz_plot.cpp bz_main.cpp
	bz_ops.cpp bz.h
	globals.cpp globals.h
	bz_formulas.cpp
	plot_cut.cpp plot_cut.h
	bzlib.h
	../../tlibs2/libs/qt/recent.cpp ../../tlibs2/libs/qt/recent.h
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
	${QtLibraries}
)


if(USE_SCRIPTING)
	find_package(Python3 COMPONENTS Interpreter Development)
	find_package(SWIG COMPONENTS python)
	#find_package(SWIG COMPONENTS javascript)

	if(SWIG_FOUND AND SWIG_python_FOUND)
		message("Scripting using python version ${Python3_VERSION} enabled; packages: ${Python3_SITEARCH}.")

		cmake_policy(SET CMP0078 NEW)
		cmake_policy(SET CMP0086 NEW)

		set(UseSWIG_TARGET_NAME_PREFERENCE STANDARD)
		include(${SWIG_USE_FILE})

		set_source_files_properties(bzlib.i PROPERTIES CPLUSPLUS TRUE)
		set_source_files_properties(bzlib.i PROPERTIES SWIG_FLAGS "-I ${PROJECT_SOURCE_DIR}")
		#set_source_files_properties(bzlib.i PROPERTIES SWIG_FLAGS -node)
		set_source_files_properties(
			${PROJECT_BINARY_DIR}/CMakeFiles/bz_py.dir/bzlibPYTHON_wrap.cxx
			#${PROJECT_BINARY_DIR}/CMakeFiles/bz_py.dir/bzlibJAVASCRIPT_wrap.cxx
			PROPERTIES SKIP_AUTOMOC TRUE SKIP_AUTOUIC TRUE)

		swig_add_library(bz_py LANGUAGE python SOURCES bzlib.i)
		#swig_add_library(bz_py LANGUAGE javascript SOURCES bzlib.i)

		target_link_libraries(bz_py
			Python3::Python
			${Boost_LIBRARIES}
			${Qhull_LIBRARIES}
		)
	endif()
else()
	message("Scripting disabled.")
endif()
