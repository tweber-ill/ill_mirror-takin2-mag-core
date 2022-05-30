#
# @author Tobias Weber <tweber@ill.fr>
# @date 17-jun-2019
# @license see 'LICENSE' file
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

project(magtools)
cmake_minimum_required(VERSION 3.0)

option(ONLY_BUILD_FINISHED "only build finished tools" TRUE)


# cif converter
add_subdirectory(tools/cif2xml)

# polarisation tool
add_subdirectory(tools/pol)

# nuclear and magnetic structure factor tools
add_subdirectory(tools/structfact)
add_subdirectory(tools/magstructfact)

# Brillouin zone tool
add_subdirectory(tools/bz)

# magnon dynamics tool
add_subdirectory(tools/magdyn)

# molecular dynamics tool
add_subdirectory(tools/moldyn)


if(NOT ONLY_BUILD_FINISHED)
	# setup tools
	add_subdirectory(tools/setup)

	# magnetic structure factor browser
	add_subdirectory(tools/magsgbrowser)

	# new scan browser tool
	add_subdirectory(tools/scanbrowser)
endif()
