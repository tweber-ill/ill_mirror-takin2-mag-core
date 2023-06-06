#!/bin/bash
#
# installs mingw files
# @author Tobias Weber <tweber@ill.fr>
# @date 2-feb-2020
# @license GPLv3
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


# directories
MINGW_DIR=/usr/x86_64-w64-mingw32/sys-root/mingw/bin/
MINGW_LIB_DIR=/usr/x86_64-w64-mingw32/sys-root/mingw/lib/
INSTALL_DIR=~/.wine/drive_c/takin2tools



# binaries
mkdir -vp ${INSTALL_DIR}

cp -v tools/bz/build/takin_bz.exe			${INSTALL_DIR}/
cp -v tools/magdyn/build/takin_magdyn.exe		${INSTALL_DIR}/
cp -v tools/moldyn/build/takin_moldyn.exe		${INSTALL_DIR}/
cp -v tools/pol/build/takin_pol.exe			${INSTALL_DIR}/
cp -v tools/structfact/build/takin_structfact.exe	${INSTALL_DIR}/
cp -v tools/magstructfact/build/takin_magstructfact.exe	${INSTALL_DIR}/
cp -v tools/cif2xml/build/*.exe				${INSTALL_DIR}/



# libraries
MINGW_LIBS="Qt5Core.dll Qt5Gui.dll Qt5Widgets.dll Qt5Svg.dll \
	libstdc++-6.dll libglib-2.0-0.dll libgcc_s_seh-1.dll libwinpthread-1.dll \
	libboost_program_options-mt-x64.dll libqhull_r.dll \
	libintl-8.dll iconv.dll libpcre2-16-0.dll libpcre2-8-0.dll libpcre-1.dll \
	zlib1.dll libbz2-1.dll libpng16-16.dll \
	libssp-0.dll libharfbuzz-0.dll libfreetype-6.dll"

for FILE in ${MINGW_LIBS}; do
	cp -v ${MINGW_DIR}/${FILE} ${INSTALL_DIR}/
done



# qt libraries
mkdir -vp ${INSTALL_DIR}/qtplugins/platforms/
mkdir -vp ${INSTALL_DIR}/qtplugins/styles/
cp -v ${MINGW_LIB_DIR}/qt5/plugins/platforms/*.dll ${INSTALL_DIR}/qtplugins/platforms/
cp -v ${MINGW_LIB_DIR}/qt5/plugins/styles/*.dll ${INSTALL_DIR}/qtplugins/styles/



# access rights
find ${INSTALL_DIR} -name "*.dll" -exec chmod a-x {} \; -print
find ${INSTALL_DIR} -name "*.exe" -exec chmod a+x {} \; -print



# strip
find ${INSTALL_DIR} \( -name "*.exe" -o -name "*.dll" \) -exec strip -v {} \; -print
