#!/bin/bash
#
# @author Tobias Weber <tweber@ill.fr>
# @date 18-dec-19
# @license GPLv3
#

PROGDIR=~/.wine/drive_c/moldyn
MINGWDIR=/usr/x86_64-w64-mingw32/sys-root/mingw/bin
MINGWQTDIR=/usr/x86_64-w64-mingw32/sys-root/mingw/lib/qt5

mkdir ${PROGDIR}
mkdir -p ${PROGDIR}/qtplugins/platforms

cp -v build/moldyn.exe ${PROGDIR}/

cp -v ${MINGWDIR}/Qt5Core.dll		${PROGDIR}/
cp -v ${MINGWDIR}/Qt5Gui.dll		${PROGDIR}/
cp -v ${MINGWDIR}/Qt5Widgets.dll		${PROGDIR}/
cp -v ${MINGWQTDIR}/plugins/platforms/*.dll	${PROGDIR}/qtplugins/platforms/

cp -v ${MINGWDIR}/libgcc*.dll		${PROGDIR}/
cp -v ${MINGWDIR}/libstdc++-*.dll	${PROGDIR}/
cp -v ${MINGWDIR}/libglib*.dll		${PROGDIR}/
cp -v ${MINGWDIR}/libwinpthread*.dll	${PROGDIR}/

cp -v ${MINGWDIR}/zlib*.dll		${PROGDIR}/
cp -v ${MINGWDIR}/libbz2*.dll		${PROGDIR}/

cp -v ${MINGWDIR}/libfreetype*.dll	${PROGDIR}/
cp -v ${MINGWDIR}/libpng*.dll		${PROGDIR}/

cp -v ${MINGWDIR}/iconv.dll		${PROGDIR}/
cp -v ${MINGWDIR}/libpcre*.dll		${PROGDIR}/
cp -v ${MINGWDIR}/libharfbuzz*.dll	${PROGDIR}/
cp -v ${MINGWDIR}/libintl*.dll		${PROGDIR}/

#echo -e "[paths]\nplugins=qtplugins\n" > ${PROGDIR}/qt.conf

find ${PROGDIR} -name "*.exe" -or -name "*.dll" -exec strip {} \; -print
