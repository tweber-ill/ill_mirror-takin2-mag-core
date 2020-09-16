#!/bin/bash
#
# downloads external libraries for in20tools
# @author Tobias Weber <tweber@ill.fr>
# @date 6-apr-18
# @license GPLv3, see 'LICENSE' file
#

# -----------------------------------------------------------------------------
# tools
WGET=wget
TAR=tar
UZIP=unzip
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# URLs for external libs
TLIBS=https://forge.frm2.tum.de/cgit/cgit.cgi/frm2/mira/tlibs.git/snapshot/tlibs-master.tar.bz2
QCP=https://www.qcustomplot.com/release/2.0.1/QCustomPlot-source.tar.gz
GEMMI=https://github.com/project-gemmi/gemmi/archive/master.zip

# local file names
TLIBS_LOCAL=${TLIBS##*[/\\]}
QCP_LOCAL=${QCP##*[/\\]}
GEMMI_LOCAL=${GEMMI##*[/\\]}
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# cleans externals
function clean_dirs()
{
	rm -rf qcp
	rm -rf gemmi
}

function clean_files()
{
	rm -f ${QCP_LOCAL}
	rm -f ${GEMMI_LOCAL}
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
# -----------------------------------------------------------------------------


mkdir -p ext
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
echo -e "Removing temporary files...\n"
clean_files
echo -e "--------------------------------------------------------------------------------\n"
