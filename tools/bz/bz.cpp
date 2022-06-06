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

#include <QtWidgets/QApplication>
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
		m_symops->verticalHeader()->setDefaultSectionSize(
			fontMetrics().lineSpacing()*4 + 4);
		m_symops->verticalHeader()->setVisible(false);
		m_symops->setAlternatingRowColors(true);
		m_symops->setColumnCount(NUM_COLS);
		m_symops->setHorizontalHeaderItem(COL_OP,
			new QTableWidgetItem{"Symmetry Operation"});
		m_symops->setHorizontalHeaderItem(COL_PROP,
			new QTableWidgetItem{"Properties"});
		m_symops->setColumnWidth(COL_OP, 300);
		m_symops->setColumnWidth(COL_PROP, 100);

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
		m_sg_ops.reserve(spacegroups.size());
		for(auto [sgnum, descr, ops] : spacegroups)
		{
			m_comboSG->addItem(descr.c_str(), m_comboSG->count());
			m_sg_ops.emplace_back(std::move(ops));
		}


		auto tabGrid = new QGridLayout(symopspanel);
		tabGrid->setSpacing(2);
		tabGrid->setContentsMargins(4,4,4,4);
		int y=0;
		//tabGrid->addWidget(m_plot.get(), y,0,1,4);
		tabGrid->addWidget(m_symops, y,0,1,4);
		tabGrid->addWidget(btnAdd, ++y,0,1,1);
		tabGrid->addWidget(btnDel, y,1,1,1);
		tabGrid->addWidget(btnUp, y,2,1,1);
		tabGrid->addWidget(btnDown, y,3,1,1);

		tabGrid->addWidget(new QLabel("Space Group:"), ++y,0,1,1);
		tabGrid->addWidget(m_comboSG, y,1,1,2);
		tabGrid->addWidget(btnSG, y,3,1,1);


		auto sep1 = new QFrame(symopspanel); sep1->setFrameStyle(QFrame::HLine);
		tabGrid->addWidget(sep1, ++y,0, 1,4);

		tabGrid->addWidget(new QLabel("Lattice (A):"), ++y,0,1,1);
		tabGrid->addWidget(m_editA, y,1,1,1);
		tabGrid->addWidget(m_editB, y,2,1,1);
		tabGrid->addWidget(m_editC, y,3,1,1);
		tabGrid->addWidget(new QLabel("Angles (deg):"), ++y,0,1,1);
		tabGrid->addWidget(m_editAlpha, y,1,1,1);
		tabGrid->addWidget(m_editBeta, y,2,1,1);
		tabGrid->addWidget(m_editGamma, y,3,1,1);


		// table CustomContextMenu
		m_tabContextMenu = new QMenu(m_symops);
		m_tabContextMenu->addAction("Add SymOp Before", this, [this]() { this->AddTabItem(-2); });
		m_tabContextMenu->addAction("Add SymOp After", this, [this]() { this->AddTabItem(-3); });
		m_tabContextMenu->addAction("Clone SymOp", this, [this]() { this->AddTabItem(-4); });
		m_tabContextMenu->addAction("Delete SymOp", this, [this]() { BZDlg::DelTabItem(); });


		// table CustomContextMenu in case nothing is selected
		m_tabContextMenuNoItem = new QMenu(m_symops);
		m_tabContextMenuNoItem->addAction("Add SymOp", this, [this]() { this->AddTabItem(); });
		m_tabContextMenuNoItem->addAction("Delete SymOp", this, [this]() { BZDlg::DelTabItem(); });
		//m_tabContextMenuNoItem->addSeparator();


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

		tabs->addTab(symopspanel, "Crystal");
	}


	{	// brillouin zone and cuts panel
		auto bzpanel = new QWidget(this);
		auto grid = new QGridLayout(bzpanel);
		grid->setSpacing(4);
		grid->setContentsMargins(4,4,4,4);

		m_bzscene = new BZCutScene(bzpanel);
		m_bzview = new BZCutView(m_bzscene);

		for(QDoubleSpinBox** const cut :
			{ &m_cutX, &m_cutY, &m_cutZ,
			&m_cutNX, &m_cutNY, &m_cutNZ, &m_cutD })
		{
			*cut = new QDoubleSpinBox(bzpanel);
			(*cut)->setMinimum(-99);
			(*cut)->setMaximum(99);
			(*cut)->setDecimals(2);
			(*cut)->setSingleStep(0.1);
			(*cut)->setValue(0);

			// signals
			connect(*cut, static_cast<void (QDoubleSpinBox::*)(double)>
					(&QDoubleSpinBox::valueChanged),
				this, &BZDlg::CalcBZCut);
		}
		m_cutX->setValue(1);
		m_cutNZ->setValue(1);

		m_BZDrawOrder = new QSpinBox(bzpanel);
		m_BZDrawOrder->setMinimum(0);
		m_BZDrawOrder->setMaximum(99);
		m_BZDrawOrder->setValue(4);

		m_BZCalcOrder = new QSpinBox(bzpanel);
		m_BZCalcOrder->setMinimum(1);
		m_BZCalcOrder->setMaximum(99);
		m_BZCalcOrder->setValue(4);

		QPushButton *btnShowBZ = new QPushButton("3D View...", bzpanel);

		// cuts
		grid->addWidget(m_bzview, 0,0, 1,4);
		grid->addWidget(new QLabel("In-Plane Vector:"), 1,0, 1,1);
		grid->addWidget(m_cutX, 1,1, 1,1);
		grid->addWidget(m_cutY, 1,2, 1,1);
		grid->addWidget(m_cutZ, 1,3, 1,1);
		grid->addWidget(new QLabel("Plane Normal:"), 2,0, 1,1);
		grid->addWidget(m_cutNX, 2,1, 1,1);
		grid->addWidget(m_cutNY, 2,2, 1,1);
		grid->addWidget(m_cutNZ, 2,3, 1,1);
		grid->addWidget(new QLabel("Plane Offset:"), 3,0, 1,1);
		grid->addWidget(m_cutD, 3,1, 1,1);
		grid->addWidget(new QLabel("Draw Order:"), 3,2,1,1);
		grid->addWidget(m_BZDrawOrder, 3,3, 1,1);

		// bz
		auto sep1 = new QFrame(bzpanel); sep1->setFrameStyle(QFrame::HLine);
		grid->addWidget(sep1, 4,0,1,4);
		grid->addWidget(new QLabel("Calc. Order:"), 5,0,1,1);
		grid->addWidget(m_BZCalcOrder, 5,1, 1,1);
		grid->addWidget(btnShowBZ, 5,3, 1,1);

		// signals
		connect(m_BZDrawOrder,
			static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
				this, [this]() { this->CalcBZCut(); });
		connect(m_bzview, &BZCutView::SignalMouseCoordinates,
			this, &BZDlg::BZCutMouseMoved);
		connect(m_BZCalcOrder,
			static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
				this, [this]() { this->CalcBZ(); });
		connect(btnShowBZ, &QPushButton::clicked, this, &BZDlg::ShowBZPlot);

		tabs->addTab(bzpanel, "Brillouin Zone");
	}


	{	// brillouin zone calculation results panel
		auto resultspanel = new QWidget(this);
		auto grid = new QGridLayout(resultspanel);
		grid->setSpacing(4);
		grid->setContentsMargins(4,4,4,4);

		m_bzresults = new QPlainTextEdit(resultspanel);
		m_bzresults->setReadOnly(true);
		m_bzresults->setFont(
			QFontDatabase::systemFont(QFontDatabase::FixedFont));

		grid->addWidget(m_bzresults, 0,0, 1,4);

		tabs->addTab(resultspanel, "Results");
	}


	// status bar
	m_status = new QLabel(this);
	m_status->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	m_status->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

	// main grid
	auto main_grid = new QGridLayout(this);
	main_grid->setSpacing(4);
	main_grid->setContentsMargins(4,4,4,4);
	main_grid->addWidget(tabs, 0,0, 1,1);
	main_grid->addWidget(m_status, 1,0, 1,1);


	// info dialog
	QDialog *dlgInfo = nullptr;
	{
		auto infopanel = new QWidget(this);
		auto grid = new QGridLayout(infopanel);
		grid->setSpacing(4);
		grid->setContentsMargins(6, 6, 6, 6);

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

		// renderer infos
		for(int i=0; i<4; ++i)
		{
			m_labelGlInfos[i] = new QLabel("", infopanel);
			m_labelGlInfos[i]->setSizePolicy(
				QSizePolicy::Ignored,
				m_labelGlInfos[i]->sizePolicy().verticalPolicy());
		}

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

		auto labelGemmi = new QLabel(QString(
			"<a href=\"https://github.com/project-gemmi/gemmi\">Gemmi</a>"
			" Version: %1.").arg(get_gemmi_version<QString>()),
			infopanel);
		labelGemmi->setOpenExternalLinks(true);
		grid->addWidget(labelGemmi, y++,0, 1,1);

		grid->addWidget(sep3, y++,0, 1,1);

		for(int i=0; i<4; ++i)
			grid->addWidget(m_labelGlInfos[i], y++,0, 1,1);

		grid->addItem(new QSpacerItem(16,16,
			QSizePolicy::Minimum, QSizePolicy::Expanding),
			y++,0, 1,1);

		// add info panel as a tab
		//m_tabs->addTab(infopanel, "Infos");

		// show info panel as a dialog
		dlgInfo = new QDialog(this);
		dlgInfo->setWindowTitle("About");
		dlgInfo->setSizeGripEnabled(true);

		QPushButton *infoDlgOk = new QPushButton("OK", dlgInfo);
		connect(infoDlgOk, &QAbstractButton::clicked,
			dlgInfo, &QDialog::accept);

		auto dlgGrid = new QGridLayout(dlgInfo);
		dlgGrid->setSpacing(8);
		dlgGrid->setContentsMargins(8, 8, 8, 8);
		dlgGrid->addWidget(infopanel, 0,0, 1,4);
		dlgGrid->addWidget(infoDlgOk, 1,3, 1,1);
	}


	// menu bar
	{
		m_menu = new QMenuBar(this);
		m_menu->setNativeMenuBar(
			m_sett ? m_sett->value("native_gui", false).toBool() : false);

		auto menuFile = new QMenu("File", m_menu);
		auto menuBZ = new QMenu("Brillouin Zone", m_menu);

		// recent files menu
		m_menuOpenRecent = new QMenu("Open Recent", menuFile);

		m_recent.SetRecentFilesMenu(m_menuOpenRecent);
		m_recent.SetMaxRecentFiles(16);
		m_recent.SetOpenFunc(&m_open_func);

		// file menu
		auto acNew = new QAction("New", menuFile);
		auto acLoad = new QAction("Open...", menuFile);
		auto acSave = new QAction("Save...", menuFile);
		auto acImportCIF = new QAction("Import CIF...", menuFile);
		auto acExit = new QAction("Quit", menuFile);

		// bz menu
		auto ac3DView = new QAction("3D View...", menuFile);
		auto acCutSVG = new QAction("Save Cut to SVG...", menuFile);
		m_acCutHull = new QAction("Calculate Convex Hull for Cut", menuFile);

		// help menu
		auto menuHelp = new QMenu("Help", m_menu);
		auto *acAboutQt = new QAction("About Qt...", menuHelp);
		auto *acAbout = new QAction("About...", menuHelp);

		acNew->setIcon(QIcon::fromTheme("document-new"));
		acLoad->setIcon(QIcon::fromTheme("document-open"));
		acSave->setIcon(QIcon::fromTheme("document-save"));
		acExit->setIcon(QIcon::fromTheme("application-exit"));
		acAboutQt->setIcon(QIcon::fromTheme("help-about"));
		acAbout->setIcon(QIcon::fromTheme("help-about"));
		m_menuOpenRecent->setIcon(QIcon::fromTheme("document-open-recent"));

		acNew->setShortcut(QKeySequence::New);
		acLoad->setShortcut(QKeySequence::Open);
		acSave->setShortcut(QKeySequence::Save);
		acExit->setShortcut(QKeySequence::Quit);

		acExit->setMenuRole(QAction::QuitRole);
		acAboutQt->setMenuRole(QAction::AboutQtRole);
		acAbout->setMenuRole(QAction::AboutRole);

		m_acCutHull->setCheckable(true);
		m_acCutHull->setChecked(true);

		menuFile->addAction(acNew);
		menuFile->addSeparator();
		menuFile->addAction(acLoad);
		menuFile->addMenu(m_menuOpenRecent);
		menuFile->addSeparator();
		menuFile->addAction(acSave);
		menuFile->addSeparator();
		menuFile->addAction(acImportCIF);
		menuFile->addSeparator();
		menuFile->addAction(acExit);

		menuBZ->addAction(m_acCutHull);
		menuBZ->addAction(acCutSVG);
		menuBZ->addSeparator();
		menuBZ->addAction(ac3DView);

		menuHelp->addAction(acAboutQt);
		menuHelp->addAction(acAbout);

		connect(acNew, &QAction::triggered, this, &BZDlg::NewFile);
		connect(acLoad, &QAction::triggered, this, static_cast<void (BZDlg::*)()>(&BZDlg::Load));
		connect(acSave, &QAction::triggered, this, static_cast<void (BZDlg::*)()>(&BZDlg::Save));
		connect(acImportCIF, &QAction::triggered, this, &BZDlg::ImportCIF);
		connect(acExit, &QAction::triggered, this, &QDialog::close);
		connect(ac3DView, &QAction::triggered, this, &BZDlg::ShowBZPlot);
		connect(acCutSVG, &QAction::triggered, this, &BZDlg::SaveCutSVG);
		connect(m_acCutHull, &QAction::triggered, this, &BZDlg::CalcBZCut);
		connect(acAboutQt, &QAction::triggered, this, []()
		{
			qApp->aboutQt();
		});
		connect(acAbout, &QAction::triggered, this, [dlgInfo]()
		{
			if(!dlgInfo)
				return;

			dlgInfo->show();
			dlgInfo->raise();
			dlgInfo->activateWindow();
		});

		m_menu->addMenu(menuFile);
		m_menu->addMenu(menuBZ);
		m_menu->addMenu(menuHelp);
		main_grid->setMenuBar(m_menu);
	}


	if(m_sett)
	{
		// restore window size and position
		if(m_sett->contains("geo"))
			restoreGeometry(m_sett->value("geo").toByteArray());
		else
			resize(600, 500);

		if(m_sett->contains("recent_files"))
			m_recent.SetRecentFiles(m_sett->value("recent_files").toStringList());
	}

	m_ignoreChanges = 0;
}


void BZDlg::closeEvent(QCloseEvent *)
{
	if(m_sett)
	{
		m_recent.TrimEntries();
		m_sett->setValue("recent_files", m_recent.GetRecentFiles());

		m_sett->setValue("geo", saveGeometry());
		if(m_dlgPlot)
			m_sett->setValue("geo_3dview", m_dlgPlot->saveGeometry());
	}
}


void BZDlg::UpdateBZDescription()
{
	// brillouin zone description
	std::string descr = m_descrBZ + "\n" + m_descrBZCut;
	m_bzresults->setPlainText(descr.c_str());
}
