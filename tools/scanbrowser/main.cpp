/**
 * scan browser / analysis tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date 6-Apr-2018
 * @license see 'LICENSE' file
 */

#include <iostream>
#include <QtWidgets/QApplication>

#include "tlibs2/libs/helper.h"
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
