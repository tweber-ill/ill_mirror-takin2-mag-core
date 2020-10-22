/**
 * space group browser
 * @author Tobias Weber <tweber@ill.fr>
 * @date Apr-2018
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 8-Nov-2018 from my privately developed "magtools" project (https://github.com/t-weber/magtools).
 */

#include "browser.h"

#include <QtCore/QSettings>
#include <QtWidgets/QApplication>

#include <locale>
#include <memory>


// ----------------------------------------------------------------------------


static inline void set_locales()
{
	std::ios_base::sync_with_stdio(false);

	::setlocale(LC_ALL, "C");
	std::locale::global(std::locale("C"));
	QLocale::setDefault(QLocale::C);
}



int main(int argc, char** argv)
{
	QSettings sett("tobis_stuff", "sgbrowser", nullptr);

	auto app = std::make_unique<QApplication>(argc, argv);
	set_locales();

	auto dlg = std::make_unique<SgBrowserDlg>(nullptr, &sett);
	dlg->show();

	return app->exec();
}

// ----------------------------------------------------------------------------
