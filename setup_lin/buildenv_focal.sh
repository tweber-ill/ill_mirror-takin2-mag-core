#!/bin/bash
#
# install additional packages needed for building
# @author Tobias Weber <tobias.weber@tum.de>
# @date 30-jul-20
# @license GPLv3, see 'LICENSE' file
#

if [[ $(id -u) -gt 0 ]]; then
	echo -e "Please run this script as root."
	exit -1
fi



# -----------------------------------------------------------------------------
# install packages
# -----------------------------------------------------------------------------
if ! apt-get install g++-10 liblapacke-dev llvm llvm-dev
then
	echo -e "Error: Could not install packages necessary for building."
	exit -1
fi
# -----------------------------------------------------------------------------
