/**
 * in20 data analysis tool
 * @author Tobias Weber
 * @date 6-Apr-2018
 * @license see 'LICENSE' file
 */

#include <locale>
#include <iostream>
#include <QApplication>

#include "mainwnd.h"


int main(int argc, char** argv)
{
	std::ios_base::sync_with_stdio(false);

	setlocale(LC_ALL, "C");
	std::locale::global(std::locale("C"));
	QLocale::setDefault(QLocale::C);


	QApplication app(argc, argv);
	QSettings sett("tobis_stuff", "in20tool");

	MainWnd wnd(&sett);
	wnd.show();

	return app.exec();
}
