#
# @author Tobias Weber <tweber@ill.fr>
# @date 17-jun-2019
# @license see 'LICENSE' file
#

project(magtools)
cmake_minimum_required(VERSION 3.0)


# setup tools
add_subdirectory(tools/setup)


# magnetic and nuclear structure tools
add_subdirectory(tools/structfact)
add_subdirectory(tools/magstructfact)
add_subdirectory(tools/magsgbrowser)
add_subdirectory(tools/cif2xml)


# polarisation tools
add_subdirectory(tools/pol)


# misc tools
add_subdirectory(tools/moldyn)


# new scan browser tool
add_subdirectory(tools/scanbrowser)
