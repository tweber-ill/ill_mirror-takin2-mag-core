/**
 * structure factor tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date Dec-2018
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 28-Dec-2018 from my privately developed "misc" project (https://github.com/t-weber/misc).
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2021  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 * "misc" project
 * Copyright (C) 2017-2021  Tobias WEBER (privately developed).
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

#include "structfact.h"

#include <QtCore/QDir>
#include <QtWidgets/QApplication>

#include <iostream>

#include "tlibs2/libs/qt/helper.h"


#ifndef BUILD_LIB	// build application


int main(int argc, char** argv)
{
	tl2::set_gl_format(1, _GL_MAJ_VER, _GL_MIN_VER, 8);
	tl2::set_locales();

	QApplication::addLibraryPath(QString(".") + QDir::separator() + "qtplugins");
	auto app = std::make_unique<QApplication>(argc, argv);
	auto dlg = std::make_unique<StructFactDlg>(nullptr);
	dlg->show();

	return app->exec();
}


#else	// build library


#include <boost/dll/alias.hpp>


/**
 * initialise plugin
 */
bool init()
{
	tl2::set_gl_format(1, _GL_MAJ_VER, _GL_MIN_VER, 8);
	tl2::set_locales();

	return true;
}


/**
 * plugin descriptor
 * type; title; description
 */
const char* descr()
{
	return "dlg;Structure Factors;Calculates nuclear structure factors.";
}


/**
 * create the plugin main dialog
 */
//std::shared_ptr<QDialog> create(QWidget *pParent)
QDialog* create(QWidget *pParent)
{
	//std::cout << "In " << __FUNCTION__ << std::endl;
	//return std::make_shared<StructFactDlg>(pParent);
	return new StructFactDlg(pParent);
}


/**
 * destroy the plugin main dialog
 */
void destroy(QDialog* dlg)
{
	//std::cout << "In " << __FUNCTION__ << std::endl;
	if(dlg) delete dlg;
}


BOOST_DLL_ALIAS(init, tl_init);
BOOST_DLL_ALIAS(descr, tl_descr);
BOOST_DLL_ALIAS(create, tl_create);
BOOST_DLL_ALIAS(destroy, tl_destroy);


#endif
