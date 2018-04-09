#!/bin/bash
#
# downloads external libraries
# @author Tobias Weber <tweber@ill.fr>
# @date 6-apr-18
# @license see 'LICENSE' file
#

# -----------------------------------------------------------------------------
# tools
WGET=wget
TAR=tar
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# URLs for external libs
TLIBS=https://forge.frm2.tum.de/cgit/cgit.cgi/frm2/mira/tlibs.git/snapshot/tlibs-master.tar.bz2
QCP=http://www.qcustomplot.com/release/2.0.0/QCustomPlot-source.tar.gz

# local file names
TLIBS_LOCAL=${TLIBS##*[/\\]}
QCP_LOCAL=${QCP##*[/\\]}
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# cleans externals
function clean_dirs()
{
	rm -rf tlibs
	rm -rf qcp
}

function clean_files()
{
	rm -f ${TLIBS_LOCAL}
	rm -f ${QCP_LOCAL}
}
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
function dl_tlibs()
{
	if ! ${WGET} ${TLIBS}
	then
		echo -e "Error downloading tlibs.";
		exit -1;
	fi

	if ! ${TAR} xjvf ${TLIBS_LOCAL}
	then
		echo -e "Error extracting tlibs.";
		exit -1;
	fi

	mv tlibs-master tlibs
}


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
# -----------------------------------------------------------------------------


cd ext

echo -e "\n--------------------------------------------------------------------------------"
echo -e "Removing old libs...\n"
clean_dirs
clean_files
echo -e "--------------------------------------------------------------------------------\n"

echo -e "\n--------------------------------------------------------------------------------"
echo -e "Installing external tlibs library...\n"
dl_tlibs
echo -e "--------------------------------------------------------------------------------\n"

echo -e "\n--------------------------------------------------------------------------------"
echo -e "Installing external qcustomplot library...\n"
dl_qcp
echo -e "--------------------------------------------------------------------------------\n"

echo -e "\n--------------------------------------------------------------------------------"
echo -e "Removing temporary files...\n"
clean_files
echo -e "--------------------------------------------------------------------------------\n"