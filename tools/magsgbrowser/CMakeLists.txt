#
# @author Tobias Weber
# @date Apr-2018
# @license GPLv3, see 'LICENSE' file
# @desc The present version was forked on 8-Nov-2018 from the privately developed "magtools" project (https://github.com/t-weber/magtools).
#
project(magsgbrowser)
cmake_minimum_required(VERSION 3.0)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 17)
add_definitions(-std=c++17 -fconcepts)
include_directories("${PROJECT_SOURCE_DIR}" "../..")


# -----------------------------------------------------------------------------
# main program
add_executable(magsgbrowser
	main.cpp
	browser.cpp browser.h
)

target_link_libraries(magsgbrowser
	Qt5::Core Qt5::Gui Qt5::Widgets
)
# -----------------------------------------------------------------------------

