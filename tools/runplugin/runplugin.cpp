/**
 * directly executes a plugin module
 * @author Tobias Weber <tweber@ill.fr>
 * @date Nov-2018
 * @license GPLv3, see 'LICENSE' file
 *
 * g++-8 -std=c++17 -I/usr/local/include -L/usr/local/lib -o runplugin runplugin.cpp -F/usr/local/opt/qt5/lib -framework QtCore -framework QtWidgets -lboost_filesystem -lboost_system
 * g++ -std=c++17 -I/usr/local/include -I/usr/include/qt5 -L/usr/local/lib -fPIC -o runplugin runplugin.cpp -lQt5Core -lQt5Widgets -lboost_filesystem -lboost_system -ldl
 */

#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <iostream>
#include <memory>
#include <boost/dll/shared_library.hpp>


static inline void set_locales()
{
	std::ios_base::sync_with_stdio(false);

	::setlocale(LC_ALL, "C");
	std::locale::global(std::locale("C"));
	QLocale::setDefault(QLocale::C);
}


int main(int argc, char** argv)
{
	set_locales();

	if(argc <= 1)
	{
		std::cerr << "Specify a plugin library." << std::endl;
		return -1;
	}

	const char *dllfile = argv[1];
	auto dll = std::make_shared<boost::dll::shared_library>(dllfile);
	if(!dll || !dll->is_loaded())
	{
		std::cerr << "Could not load plugin." << std::endl;
		return -1;
	}
	std::cerr << "Plugin " << dll->location() << " loaded." << std::endl;


	if(!dll->has("tl_init") || !dll->has("tl_create"))
	{
		std::cerr << "Plugin does not have the \"tl_init\" or \"tl_create\" functions." << std::endl;
		return -1;
	}


	if(auto initDlg = dll->get<bool(*)()>("tl_init"); initDlg)
		initDlg();

	auto app = std::make_unique<QApplication>(argc, argv);

	if(auto createDlg = dll->get<std::shared_ptr<QDialog>(*)(QWidget*)>("tl_create"); createDlg)
	{
		if(auto dlg = createDlg(nullptr); dlg)
		{
			dlg->show();
			dlg->activateWindow();
		}
	}

	return app->exec();
}
