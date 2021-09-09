/**
 * directly executes a plugin module
 * @author Tobias Weber <tweber@ill.fr>
 * @date Nov-2018
 * @license GPLv3, see 'LICENSE' file
 *
 * clang++ -std=c++17 -I/usr/local/include -L/usr/local/lib -I../.. -o runplugin runplugin.cpp -F/usr/local/opt/qt5/lib -framework QtCore -framework QtWidgets -lboost_filesystem -lboost_system
 * g++ -std=c++17 -I/usr/local/include -I/usr/include/qt5 -L/usr/local/lib -fPIC -o runplugin runplugin.cpp -lQt5Core -lQt5Widgets -lboost_filesystem -lboost_system -ldl
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2021  Tobias WEBER (Institut Laue-Langevin (ILL),
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

#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <iostream>
#include <memory>
#include <boost/dll/shared_library.hpp>
#include "tlibs2/libs/helper.h"
#include "tlibs2/libs/str.h"


int main(int argc, char** argv)
{
	try
	{
		tl2::set_locales();

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


		if(!dll->has("tl_descr") || !dll->has("tl_init") || !dll->has("tl_create") || !dll->has("tl_destroy"))
		{
			std::cerr << "Not a valid plugin" << std::endl;
			return -1;
		}


		if(auto descr = dll->get<const char*(*)()>("tl_descr"); descr)
		{
			std::vector<std::string> vecdescr;
			tl2::get_tokens<std::string, std::string>(descr(), ";", vecdescr);
			std::cout << "Module type: \"" << vecdescr[0] << "\", "
				<< "Name: \"" << vecdescr[1] << "\", "
				<< "Descr: \"" << vecdescr[2] << "\"" << std::endl;
		}


		if(auto initDlg = dll->get<bool(*)()>("tl_init"); initDlg)
			initDlg();


		auto app = std::make_unique<QApplication>(argc, argv);
		QDialog *dlg = nullptr;

		if(auto createDlg = dll->get<QDialog*(*)(QWidget*)>("tl_create"); createDlg)
		//if(auto createDlg = dll->get<std::shared_ptr<QDialog>(*)(QWidget*)>("tl_create"); createDlg)
		{
			if(dlg = createDlg(nullptr); dlg)
			{
				dlg->show();
				dlg->activateWindow();
			}
		}


		int ret = app->exec();
		if(auto destroyDlg = dll->get<void(*)(QDialog*)>("tl_destroy"); destroyDlg)
			destroyDlg(dlg);

		return ret;
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
	}

	return -1;
}
