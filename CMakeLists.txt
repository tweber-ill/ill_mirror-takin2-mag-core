#
# @author Tobias Weber <tweber@ill.fr>
# @date 6-apr-2018
# @license see 'LICENSE' file
#

project(in20tools)
cmake_minimum_required(VERSION 3.0)


message("Build type: ${CMAKE_BUILD_TYPE}")

if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	set(CMAKE_VERBOSE_MAKEFILE TRUE)
endif()


set(CMAKE_CXX_STANDARD 17)
add_definitions(-std=c++17)



# -----------------------------------------------------------------------------
# Boost
find_package(Boost REQUIRED COMPONENTS system filesystem iostreams REQUIRED)
add_definitions(${Boost_CXX_FLAGS})

# Qt
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
# -----------------------------------------------------------------------------


include_directories(
	"${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/ext"
	"${Boost_INCLUDE_DIRS}" "${Boost_INCLUDE_DIRS}/.."
)


add_executable(in20
	tools/in20/main.cpp tools/in20/mainwnd.cpp tools/in20/mainwnd.h
	tools/in20/filebrowser.cpp tools/in20/filebrowser.h
	tools/in20/workspace.cpp tools/in20/workspace.h

	ext/tlibs/log/log.cpp ext/tlibs/file/loadinstr.cpp

	ext/qcp/qcustomplot.cpp ext/qcp/qcustomplot.h
)


target_link_libraries(in20
	${Boost_LIBRARIES}
	Qt5::Core Qt5::Gui Qt5::Widgets Qt5::PrintSupport
)
