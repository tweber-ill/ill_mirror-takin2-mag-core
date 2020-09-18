#
# @author Tobias Weber
# @date dec-2018
# @license GPLv3, see 'LICENSE' file
#

cmake_minimum_required(VERSION 3.0)
project(cif2xml)


set(CMAKE_VERBOSE_MAKEFILE TRUE)

find_package(Boost REQUIRED)


set(CMAKE_CXX_STANDARD 20)
add_definitions(-std=c++20)
add_definitions(${Boost_CXX_FLAGS})


include_directories(
	"${PROJECT_SOURCE_DIR}" "${Boost_INCLUDE_DIRS}/.." "../.."
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
