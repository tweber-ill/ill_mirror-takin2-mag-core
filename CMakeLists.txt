#
# @author Tobias Weber <tweber@ill.fr>
# @date 17-jun-2019
# @license see 'LICENSE' file
#

project(magtools)
cmake_minimum_required(VERSION 3.0)

option(ONLY_BUILD_FINISHED "only build finished tools" TRUE)


# cif converter
add_subdirectory(tools/cif2xml)

# polarisation tools
add_subdirectory(tools/pol)

# nuclear and magnetic structure factors
add_subdirectory(tools/structfact)
add_subdirectory(tools/magstructfact)

# molecular dynamics tools
add_subdirectory(tools/moldyn)

# new scan browser tool
add_subdirectory(tools/scanbrowser)


if(NOT ONLY_BUILD_FINISHED)
	# setup tools
	add_subdirectory(tools/setup)

	# magnetic structure factor browser
	add_subdirectory(tools/magsgbrowser)
endif()
