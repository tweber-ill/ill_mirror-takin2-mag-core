/**
 * brillouin zone tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date May-2022
 * @license GPLv3, see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2022  Tobias WEBER (Institut Laue-Langevin (ILL),
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

#include "bz.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
namespace algo = boost::algorithm;

#include "../structfact/loadcif.h"


BZDlg::BZDlg(QWidget* pParent) : QDialog{pParent},
	m_sett{new QSettings{"takin", "bz"}}
{
	setWindowTitle("Brillouin Zones");
	setSizeGripEnabled(true);
	setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));


	auto tabs = new QTabWidget(this);


	{ // symops panel
		QWidget *symopspanel = new QWidget(this);

		m_symops = new QTableWidget(symopspanel);
		m_symops->setShowGrid(true);
		m_symops->setSortingEnabled(true);
		m_symops->setMouseTracking(true);
		m_symops->setSelectionBehavior(QTableWidget::SelectRows);
		m_symops->setSelectionMode(QTableWidget::ContiguousSelection);
		m_symops->setContextMenuPolicy(Qt::CustomContextMenu);

		m_symops->verticalHeader()->setDefaultSectionSize(fontMetrics().lineSpacing() + 4);
		m_symops->verticalHeader()->setVisible(false);

		m_symops->setColumnCount(NUM_COLS);
		m_symops->setHorizontalHeaderItem(COL_OP, new QTableWidgetItem{"Symmetry Operations"});
		m_symops->setColumnWidth(COL_OP, 500);

		QToolButton *btnAdd = new QToolButton(symopspanel);
		QToolButton *btnDel = new QToolButton(symopspanel);
		QToolButton *btnUp = new QToolButton(symopspanel);
		QToolButton *btnDown = new QToolButton(symopspanel);
		QPushButton *btnSG = new QPushButton("Get SymOps", symopspanel);

		m_symops->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});
		btnAdd->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		btnDel->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		btnUp->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		btnDown->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		btnSG->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});

		btnAdd->setText("Add SymOp");
		btnDel->setText("Delete SymOp");
		btnUp->setText("Move SymOp Up");
		btnDown->setText("Move SymOp Down");

		m_editA = new QLineEdit("5", symopspanel);
		m_editB = new QLineEdit("5", symopspanel);
		m_editC = new QLineEdit("5", symopspanel);
		m_editAlpha = new QLineEdit("90", symopspanel);
		m_editBeta = new QLineEdit("90", symopspanel);
		m_editGamma = new QLineEdit("90", symopspanel);

		m_comboSG = new QComboBox(symopspanel);


		// get space groups and symops
		auto spacegroups = get_sgs<t_mat>();
		m_SGops.reserve(spacegroups.size());
		for(auto [sgnum, descr, ops] : spacegroups)
		{
			m_comboSG->addItem(descr.c_str(), m_comboSG->count());
			m_SGops.emplace_back(std::move(ops));
		}


		auto pTabGrid = new QGridLayout(symopspanel);
		pTabGrid->setSpacing(2);
		pTabGrid->setContentsMargins(4,4,4,4);
		int y=0;
		//pTabGrid->addWidget(m_plot.get(), y,0,1,4);
		pTabGrid->addWidget(m_symops, y,0,1,4);
		pTabGrid->addWidget(btnAdd, ++y,0,1,1);
		pTabGrid->addWidget(btnDel, y,1,1,1);
		pTabGrid->addWidget(btnUp, y,2,1,1);
		pTabGrid->addWidget(btnDown, y,3,1,1);

		pTabGrid->addWidget(new QLabel("Space Group:"), ++y,0,1,1);
		pTabGrid->addWidget(m_comboSG, y,1,1,2);
		pTabGrid->addWidget(btnSG, y,3,1,1);


		auto sep1 = new QFrame(symopspanel); sep1->setFrameStyle(QFrame::HLine);
		pTabGrid->addWidget(sep1, ++y,0, 1,4);

		pTabGrid->addWidget(new QLabel("Lattice (A):"), ++y,0,1,1);
		pTabGrid->addWidget(m_editA, y,1,1,1);
		pTabGrid->addWidget(m_editB, y,2,1,1);
		pTabGrid->addWidget(m_editC, y,3,1,1);
		pTabGrid->addWidget(new QLabel("Angles (deg):"), ++y,0,1,1);
		pTabGrid->addWidget(m_editAlpha, y,1,1,1);
		pTabGrid->addWidget(m_editBeta, y,2,1,1);
		pTabGrid->addWidget(m_editGamma, y,3,1,1);


		// table CustomContextMenu
		m_pTabContextMenu = new QMenu(m_symops);
		m_pTabContextMenu->addAction("Add SymOp Before", this, [this]() { this->AddTabItem(-2); });
		m_pTabContextMenu->addAction("Add SymOp After", this, [this]() { this->AddTabItem(-3); });
		m_pTabContextMenu->addAction("Clone SymOp", this, [this]() { this->AddTabItem(-4); });
		m_pTabContextMenu->addAction("Delete SymOp", this, [this]() { BZDlg::DelTabItem(); });


		// table CustomContextMenu in case nothing is selected
		m_pTabContextMenuNoItem = new QMenu(m_symops);
		m_pTabContextMenuNoItem->addAction("Add SymOp", this, [this]() { this->AddTabItem(); });
		m_pTabContextMenuNoItem->addAction("Delete SymOp", this, [this]() { BZDlg::DelTabItem(); });
		//m_pTabContextMenuNoItem->addSeparator();


		// signals
		for(auto* edit : std::vector<QLineEdit*>{{ m_editA, m_editB, m_editC, m_editAlpha, m_editBeta, m_editGamma }})
			connect(edit, &QLineEdit::textEdited, this, [this]() { this->CalcB(); });

		connect(btnAdd, &QToolButton::clicked, this, [this]() { this->AddTabItem(-1); });
		connect(btnDel, &QToolButton::clicked, this, [this]() { BZDlg::DelTabItem(); });
		connect(btnUp, &QToolButton::clicked, this, &BZDlg::MoveTabItemUp);
		connect(btnDown, &QToolButton::clicked, this, &BZDlg::MoveTabItemDown);
		connect(btnSG, &QPushButton::clicked, this, &BZDlg::GetSymOpsFromSG);

		connect(m_symops, &QTableWidget::currentCellChanged, this, &BZDlg::TableCurCellChanged);
		connect(m_symops, &QTableWidget::entered, this, &BZDlg::TableCellEntered);
		connect(m_symops, &QTableWidget::itemChanged, this, &BZDlg::TableItemChanged);
		connect(m_symops, &QTableWidget::customContextMenuRequested, this, &BZDlg::ShowTableContextMenu);

		tabs->addTab(symopspanel, "Space Group");
	}


	{	// brillouin zone cuts panel
		auto cutspanel = new QWidget(this);
		auto pGrid = new QGridLayout(cutspanel);
		pGrid->setSpacing(4);
		pGrid->setContentsMargins(4,4,4,4);

		m_bzscene = new BZCutScene(cutspanel);
		m_bzview = new BZCutView(m_bzscene);

		for(QDoubleSpinBox** const cut :
			{ &m_cutX, &m_cutY, &m_cutZ,
			&m_cutNX, &m_cutNY, &m_cutNZ, &m_cutD })
		{
			*cut = new QDoubleSpinBox(cutspanel);
			(*cut)->setMinimum(-99);
			(*cut)->setMaximum(99);
			(*cut)->setDecimals(2);
			(*cut)->setSingleStep(0.1);
			(*cut)->setValue(0);

			connect(*cut, static_cast<void (QDoubleSpinBox::*)(double)>
					(&QDoubleSpinBox::valueChanged),
				this, &BZDlg::CalcBZCut);
		}
		m_cutX->setValue(1);
		m_cutNZ->setValue(1);

		pGrid->addWidget(m_bzview, 0,0, 1,4);
		pGrid->addWidget(new QLabel("In-Plane Vector:"), 1,0, 1,1);
		pGrid->addWidget(m_cutX, 1,1, 1,1);
		pGrid->addWidget(m_cutY, 1,2, 1,1);
		pGrid->addWidget(m_cutZ, 1,3, 1,1);
		pGrid->addWidget(new QLabel("Plane Normal:"), 2,0, 1,1);
		pGrid->addWidget(m_cutNX, 2,1, 1,1);
		pGrid->addWidget(m_cutNY, 2,2, 1,1);
		pGrid->addWidget(m_cutNZ, 2,3, 1,1);
		pGrid->addWidget(new QLabel("Plane Offset:"), 3,0, 1,1);
		pGrid->addWidget(m_cutD, 3,1, 1,1);

		tabs->addTab(cutspanel, "Cut");
	}


	{	// brillouin zone panel
		auto bzpanel = new QWidget(this);
		auto pGrid = new QGridLayout(bzpanel);
		pGrid->setSpacing(4);
		pGrid->setContentsMargins(4,4,4,4);

		m_bz = new QPlainTextEdit(bzpanel);
		m_bz->setReadOnly(true);
		m_bz->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

		m_maxBZ = new QSpinBox(bzpanel);
		m_maxBZ->setMinimum(1);
		m_maxBZ->setMaximum(99);
		m_maxBZ->setValue(4);

		QPushButton *btnShowBZ = new QPushButton("3D View...", bzpanel);

		pGrid->addWidget(m_bz, 0,0, 1,4);
		pGrid->addWidget(new QLabel("Max. Order:"), 1,0,1,1);
		pGrid->addWidget(m_maxBZ, 1,1, 1,1);
		pGrid->addWidget(btnShowBZ, 1,3, 1,1);


		// signals
		connect(m_maxBZ,
			static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
				this, [this]() { this->CalcBZ(); });
		connect(btnShowBZ, &QPushButton::clicked, this, &BZDlg::ShowBZPlot);

		tabs->addTab(bzpanel, "Brillouin Zone");
	}


	{	// info panel
		auto infopanel = new QWidget(this);
		auto pGrid = new QGridLayout(infopanel);
		pGrid->setSpacing(4);
		pGrid->setContentsMargins(4,4,4,4);

		// table grid
		for(int i=0; i<4; ++i)
		{
			m_labelGlInfos[i] = new QLabel("", infopanel);
			m_labelGlInfos[i]->setSizePolicy(
				QSizePolicy::Ignored,
				m_labelGlInfos[i]->sizePolicy().verticalPolicy());
		}

		auto sep1 = new QFrame(infopanel); sep1->setFrameStyle(QFrame::HLine);
		auto sep2 = new QFrame(infopanel); sep2->setFrameStyle(QFrame::HLine);
		auto sep3 = new QFrame(infopanel); sep3->setFrameStyle(QFrame::HLine);

		std::string strBoost = BOOST_LIB_VERSION;
		algo::replace_all(strBoost, "_", ".");

		auto labelTitle = new QLabel("Brillouin Zone Calculator", infopanel);
		auto fontTitle = labelTitle->font();
		fontTitle.setBold(true);
		labelTitle->setFont(fontTitle);
		labelTitle->setAlignment(Qt::AlignHCenter);

		auto labelAuthor = new QLabel("Written by Tobias Weber <tweber@ill.fr>.", infopanel);
		labelAuthor->setAlignment(Qt::AlignHCenter);

		auto labelDate = new QLabel("May 2022.", infopanel);
		labelDate->setAlignment(Qt::AlignHCenter);

		int y = 0;
		pGrid->addWidget(labelTitle, y++,0, 1,1);
		pGrid->addWidget(labelAuthor, y++,0, 1,1);
		pGrid->addWidget(labelDate, y++,0, 1,1);
		pGrid->addItem(new QSpacerItem(16,16, QSizePolicy::Minimum, QSizePolicy::Fixed), y++,0, 1,1);
		pGrid->addWidget(sep1, y++,0, 1,1);
		pGrid->addWidget(new QLabel(QString("Compiler: ") + QString(BOOST_COMPILER) + ".", infopanel), y++,0, 1,1);
		pGrid->addWidget(new QLabel(QString("C++ Library: ") + QString(BOOST_STDLIB) + ".", infopanel), y++,0, 1,1);
		pGrid->addWidget(new QLabel(QString("Build Date: ") + QString(__DATE__) + ", " + QString(__TIME__) + ".", infopanel), y++,0, 1,1);
		pGrid->addWidget(sep2, y++,0, 1,1);
		pGrid->addWidget(new QLabel(QString("Qt Version: ") + QString(QT_VERSION_STR) + ".", infopanel), y++,0, 1,1);
		pGrid->addWidget(new QLabel(QString("Boost Version: ") + strBoost.c_str() + ".", infopanel), y++,0, 1,1);
		pGrid->addWidget(sep3, y++,0, 1,1);
		for(int i=0; i<4; ++i)
			pGrid->addWidget(m_labelGlInfos[i], y++,0, 1,1);
		pGrid->addItem(new QSpacerItem(16,16, QSizePolicy::Minimum, QSizePolicy::Expanding), y++,0, 1,1);

		tabs->addTab(infopanel, "Infos");
	}


	// main grid
	auto pmainGrid = new QGridLayout(this);
	pmainGrid->setSpacing(4);
	pmainGrid->setContentsMargins(4,4,4,4);
	pmainGrid->addWidget(tabs, 0,0, 1,1);


	// menu bar
	{
		m_menu = new QMenuBar(this);
		m_menu->setNativeMenuBar(
			m_sett ? m_sett->value("native_gui", false).toBool() : false);

		auto menuFile = new QMenu("File", m_menu);
		auto menuView = new QMenu("Brillouin Zone", m_menu);

		auto acNew = new QAction("New", menuFile);
		auto acLoad = new QAction("Load...", menuFile);
		auto acSave = new QAction("Save...", menuFile);
		auto acImportCIF = new QAction("Import CIF...", menuFile);
		auto acExit = new QAction("Quit", menuFile);
		auto ac3DView = new QAction("3D View...", menuFile);

		acNew->setShortcut(QKeySequence::New);
		acLoad->setShortcut(QKeySequence::Open);
		acSave->setShortcut(QKeySequence::Save);
		acExit->setShortcut(QKeySequence::Quit);

		acExit->setMenuRole(QAction::QuitRole);

		menuFile->addAction(acNew);
		menuFile->addSeparator();
		menuFile->addAction(acLoad);
		menuFile->addAction(acSave);
		menuFile->addSeparator();
		menuFile->addAction(acImportCIF);
		menuFile->addSeparator();
		menuFile->addAction(acExit);
		menuView->addAction(ac3DView);

		connect(acNew, &QAction::triggered, this,  [this]()
		{
			// clear old table
			DelTabItem(-1);

			// set some defaults
			m_comboSG->setCurrentIndex(0);
			m_editA->setText("5");
			m_editB->setText("5");
			m_editC->setText("5");
			m_editAlpha->setText("90");
			m_editBeta->setText("90");
			m_editGamma->setText("90");
		});
		connect(acLoad, &QAction::triggered, this, &BZDlg::Load);
		connect(acSave, &QAction::triggered, this, &BZDlg::Save);
		connect(acImportCIF, &QAction::triggered, this, &BZDlg::ImportCIF);
		connect(acExit, &QAction::triggered, this, &QDialog::close);
		connect(ac3DView, &QAction::triggered, this, &BZDlg::ShowBZPlot);

		m_menu->addMenu(menuFile);
		m_menu->addMenu(menuView);
		pmainGrid->setMenuBar(m_menu);
	}


	// restore window size and position
	if(m_sett && m_sett->contains("geo"))
		restoreGeometry(m_sett->value("geo").toByteArray());
	else
		resize(600, 500);


	m_ignoreChanges = 0;
}


void BZDlg::closeEvent(QCloseEvent *)
{
	if(m_sett)
	{
		m_sett->setValue("geo", saveGeometry());
		if(m_dlgPlot)
			m_sett->setValue("geo_3dview", m_dlgPlot->saveGeometry());
	}
}
