/**
 * about dialog
 * @author Tobias Weber <tweber@ill.fr>
 * @date 2-Feb-2023
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

#include "about.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QFrame>
#include <QtWidgets/QPushButton>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
namespace algo = boost::algorithm;


QDialog* create_about_dialog(QWidget* parent)
{
	auto infopanel = new QWidget(parent);
	auto grid = new QGridLayout(infopanel);
	grid->setSpacing(4);
	grid->setContentsMargins(6, 6, 6, 6);

	auto sep1 = new QFrame(infopanel); sep1->setFrameStyle(QFrame::HLine);
	auto sep2 = new QFrame(infopanel); sep2->setFrameStyle(QFrame::HLine);
	auto sep3 = new QFrame(infopanel); sep3->setFrameStyle(QFrame::HLine);

	std::string strBoost = BOOST_LIB_VERSION;
	algo::replace_all(strBoost, "_", ".");

	auto labelTitle = new QLabel("Takin / Scan Browser", infopanel);
	auto fontTitle = labelTitle->font();
	fontTitle.setBold(true);
	labelTitle->setFont(fontTitle);
	labelTitle->setAlignment(Qt::AlignHCenter);

	auto labelAuthor = new QLabel("Written by Tobias Weber <tweber@ill.fr>.", infopanel);
	labelAuthor->setAlignment(Qt::AlignHCenter);

	auto labelDate = new QLabel("2018.", infopanel);
	labelDate->setAlignment(Qt::AlignHCenter);

	int y = 0;
	grid->addWidget(labelTitle, y++,0, 1,1);
	grid->addWidget(labelAuthor, y++,0, 1,1);
	grid->addWidget(labelDate, y++,0, 1,1);

	grid->addItem(new QSpacerItem(16,16,
		QSizePolicy::Minimum, QSizePolicy::Fixed),
		y++,0, 1,1);
	grid->addWidget(sep1, y++,0, 1,1);

	grid->addWidget(new QLabel(
		QString("Compiler: ") +
		QString(BOOST_COMPILER) + ".",
		infopanel), y++,0, 1,1);
	grid->addWidget(new QLabel(
		QString("C++ Library: ") +
		QString(BOOST_STDLIB) + ".",
		infopanel), y++,0, 1,1);
	grid->addWidget(new QLabel(
		QString("Build Date: ") +
		QString(__DATE__) + ", " +
		QString(__TIME__) + ".",
		infopanel), y++,0, 1,1);

	grid->addWidget(sep2, y++,0, 1,1);

	auto labelQt = new QLabel(QString(
		"<a href=\"http://code.qt.io/cgit/\">Qt</a>"
		" Version: %1.").arg(QT_VERSION_STR),
		infopanel);
	labelQt->setOpenExternalLinks(true);
	grid->addWidget(labelQt, y++,0, 1,1);

	auto labelBoost = new QLabel(QString(
		"<a href=\"http://www.boost.org\">Boost</a>"
		" Version: %1.").arg(strBoost.c_str()),
		infopanel);
	labelBoost->setOpenExternalLinks(true);
	grid->addWidget(labelBoost, y++,0, 1,1);

	grid->addWidget(sep3, y++,0, 1,1);

	grid->addItem(new QSpacerItem(16,16,
	QSizePolicy::Minimum, QSizePolicy::Expanding),
		y++,0, 1,1);

	QDialog* dlgInfo = new QDialog(parent);
	dlgInfo->setWindowTitle("About");
	dlgInfo->setSizeGripEnabled(true);
	dlgInfo->setFont(parent->font());

	QPushButton *infoDlgOk = new QPushButton("OK", dlgInfo);
	QObject::connect(infoDlgOk, &QAbstractButton::clicked,
		dlgInfo, &QDialog::accept);

	auto dlgGrid = new QGridLayout(dlgInfo);
	dlgGrid->setSpacing(8);
	dlgGrid->setContentsMargins(8, 8, 8, 8);
	dlgGrid->addWidget(infopanel, 0,0, 1,4);
	dlgGrid->addWidget(infoDlgOk, 1,3, 1,1);

	return dlgInfo;
}
