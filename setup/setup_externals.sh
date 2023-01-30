#!/bin/bash
#
# downloads external libraries
# @author Tobias Weber <tweber@ill.fr>
# @date 6-apr-18
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

# -----------------------------------------------------------------------------
# tools
WGET=wget
TAR=tar
UZIP=unzip
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# URLs for external libs
QCP=https://www.qcustomplot.com/release/2.0.1/QCustomPlot-source.tar.gz
GEMMI=https://github.com/project-gemmi/gemmi/archive/master.zip
MAGDATA=https://stokes.byu.edu/iso/magnetic_data.txt
PATHSLIB=https://code.ill.fr/scientific-software/takin/paths/-/archive/master/paths-master.tar.bz2
#TLIBS=https://forge.frm2.tum.de/cgit/cgit.cgi/frm2/mira/tlibs.git/snapshot/tlibs-master.tar.bz2

# local file names
QCP_LOCAL=${QCP##*[/\\]}
GEMMI_LOCAL=${GEMMI##*[/\\]}
MAGDATA_LOCAL=${MAGDATA##*[/\\]}
PATHSLIB_LOCAL=${PATHSLIB##*[/\\]}
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# cleans externals
function clean_dirs()
{
	rm -rf qcp
	rm -rf gemmi
	rm -rf paths
}

function clean_files()
{
	rm -f ${QCP_LOCAL}
	rm -f ${GEMMI_LOCAL}
	rm -f ${PATHSLIB_LOCAL}
}
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
function dl_qcp()
{
	if ! ${WGET} ${QCP}
	then
		echo -e "Error downloading qcp.";
		exit -1;
	fi


	if ! ${TAR} xzvf ${QCP_LOCAL}
	then
		echo -e "Error extracting qcp.";
		exit -1;
	fi

	mv qcustomplot-source qcp
}


function dl_gemmi()
{
	if ! ${WGET} ${GEMMI}
	then
		echo -e "Error downloading gemmi.";
		exit -1;
	fi


	if ! ${UZIP} ${GEMMI_LOCAL}
	then
		echo -e "Error extracting gemmi.";
		exit -1;
	fi

	mv gemmi-master gemmi
}


function dl_pathslib()
{
	if ! ${WGET} ${PATHSLIB}
	then
		echo -e "Error downloading TAS-Paths.";
		exit -1;
	fi


	if ! ${TAR} xjvf ${PATHSLIB_LOCAL}
	then
		echo -e "Error extracting TAS-Paths.";
		exit -1;
	fi

	mv paths-master paths
}


function dl_magdata()
{
	if ! ${WGET} ${MAGDATA}
	then
		echo -e "Error downloading magnetic space group data.";
		exit -1;
	fi

	mv ${MAGDATA_LOCAL} magsg.dat
}
# -----------------------------------------------------------------------------


mkdir -pv data
mkdir -pv ext
cd ext

echo -e "\n--------------------------------------------------------------------------------"
echo -e "Removing old libs...\n"
clean_dirs
clean_files
echo -e "--------------------------------------------------------------------------------\n"

#echo -e "\n--------------------------------------------------------------------------------"
#echo -e "Installing external qcustomplot library...\n"
#dl_qcp
#echo -e "--------------------------------------------------------------------------------\n"

echo -e "\n--------------------------------------------------------------------------------"
echo -e "Installing external Gemmi library...\n"
dl_gemmi
echo -e "--------------------------------------------------------------------------------\n"

echo -e "\n--------------------------------------------------------------------------------"
echo -e "Installing external TAS-Paths library...\n"
dl_pathslib
echo -e "--------------------------------------------------------------------------------\n"

#echo -e "\n--------------------------------------------------------------------------------"
#echo -e "Downloading magnetic space group data...\n"
#dl_magdata
#echo -e "--------------------------------------------------------------------------------\n"

echo -e "\n--------------------------------------------------------------------------------"
echo -e "Removing temporary files...\n"
clean_files
echo -e "--------------------------------------------------------------------------------\n"

cd ..

echo -e "\n--------------------------------------------------------------------------------"
echo -e "Setting up links...\n"
if [ ! -L pathslib ]; then
	ln -sfv ext/paths/src pathslib
fi
echo -e "--------------------------------------------------------------------------------\n"
