#!/bin/bash
#
# install additional packages needed for building
# @author Tobias Weber <tobias.weber@tum.de>
# @date 30-jul-20
# @license GPLv3, see 'LICENSE' file
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

if [[ $(id -u) -gt 0 ]]; then
	echo -e "Please run this script as root."
	exit -1
fi



# -----------------------------------------------------------------------------
# install packages
# -----------------------------------------------------------------------------
if ! apt-get install g++-10 llvm llvm-dev \
	liblapacke-dev libqcustomplot-dev libqhull-dev
then
	echo -e "Error: Could not install packages necessary for building."
	exit -1
fi
# -----------------------------------------------------------------------------
