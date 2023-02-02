/**
 * scan browser / analysis tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date 6-Apr-2018
 * @license see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2023  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ----------------------------------------------------------------------------
 */

#include <iostream>
#include <QtWidgets/QApplication>

#include "tlibs2/libs/qt/helper.h"
#include "mainwnd.h"


int main(int argc, char** argv)
{
	tl2::set_locales();

	QApplication app(argc, argv);
	QSettings sett("tobis_stuff", "scanbrowser");

	// set GUI style
	//sett.setValue("mainwnd/theme", "fusion");
	if(sett.contains("mainwnd/theme"))
	{
		if(auto style = QStyleFactory::create(sett.value("mainwnd/theme").toString()); style)
			app.setStyle(style);
	}

	// main dialog
	MainWnd wnd(&sett);
	wnd.show();

	return app.exec();
}
