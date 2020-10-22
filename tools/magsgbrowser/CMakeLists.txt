#
# @author Tobias Weber
# @date Apr-2018
# @license GPLv3, see 'LICENSE' file
# @desc The present version was forked on 8-Nov-2018 from my privately developed "magtools" project (https://github.com/t-weber/magtools).
#
project(magsgbrowser)
cmake_minimum_required(VERSION 3.0)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 20)
add_definitions(-std=c++20)
include_directories("${PROJECT_SOURCE_DIR}" "../.."
	"../../ext/gemmi/include" "../../ext/gemmi/third_party")


# -----------------------------------------------------------------------------
# main program
add_executable(takin_magsgbrowser
	main.cpp
	browser.cpp browser.h
)

target_link_libraries(takin_magsgbrowser
	Qt5::Core Qt5::Gui Qt5::Widgets
)
# -----------------------------------------------------------------------------

