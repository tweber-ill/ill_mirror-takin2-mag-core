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


include_directories(
	"${PROJECT_SOURCE_DIR}" 
	"${PROJECT_SOURCE_DIR}/ext"
)


# -----------------------------------------------------------------------------
find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5PrintSupport REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
# -----------------------------------------------------------------------------



add_executable(in20
	tools/in20/main.cpp tools/in20/mainwnd.cpp tools/in20/mainwnd.h
	ext/qcp/qcustomplot.cpp ext/qcp/qcustomplot.h
)


target_link_libraries(in20
	Qt5::Core Qt5::Gui Qt5::Widgets Qt5::PrintSupport
)
