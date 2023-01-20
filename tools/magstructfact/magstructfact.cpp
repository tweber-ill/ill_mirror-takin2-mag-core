/**
 * magnetic structure factor tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date Jan-2019
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

#include "magstructfact.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <iostream>
#include <fstream>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
namespace algo = boost::algorithm;

#include "../structfact/loadcif.h"
#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/phys.h"
#include "tlibs2/libs/algos.h"
#include "tlibs2/libs/qt/helper.h"

using namespace tl2_ops;


t_real g_eps = 1e-6;
int g_prec = 6;


MagStructFactDlg::MagStructFactDlg(QWidget* pParent) : QDialog{pParent},
	m_sett{new QSettings{"takin", "magstructfact"}}
{
	setWindowTitle("Magnetic Structure Factors");
	setSizeGripEnabled(true);
	setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));


	// restore settings done from takin main settings dialog
	QSettings sett_core("takin", "core");
	if(sett_core.contains("main/font_gen"))
	{
		QString font_str = sett_core.value("main/font_gen").toString();
		QFont font = this->font();
		if(font.fromString(font_str))
			setFont(font);
	}
	if(sett_core.contains("main/prec"))
	{
		g_prec = sett_core.value("main/prec").toInt();
		g_eps = std::pow(t_real(10), -t_real(g_prec));
	}


	auto tabs = new QTabWidget(this);


	{	// fourier components panel
		m_nucleipanel = new QWidget(this);

		m_nuclei = new QTableWidget(m_nucleipanel);
		m_nuclei->setShowGrid(true);
		m_nuclei->setSortingEnabled(true);
		m_nuclei->setMouseTracking(true);
		m_nuclei->setSelectionBehavior(QTableWidget::SelectRows);
		m_nuclei->setSelectionMode(QTableWidget::ContiguousSelection);
		m_nuclei->setContextMenuPolicy(Qt::CustomContextMenu);

		m_nuclei->verticalHeader()->setDefaultSectionSize(fontMetrics().lineSpacing() + 4);
		m_nuclei->verticalHeader()->setVisible(false);

		m_nuclei->setColumnCount(NUM_COLS);
		m_nuclei->setHorizontalHeaderItem(COL_NAME, new QTableWidgetItem{"Name"});
		m_nuclei->setHorizontalHeaderItem(COL_X, new QTableWidgetItem{"x (frac.)"});
		m_nuclei->setHorizontalHeaderItem(COL_Y, new QTableWidgetItem{"y (frac.)"});
		m_nuclei->setHorizontalHeaderItem(COL_Z, new QTableWidgetItem{"z (frac.)"});
		m_nuclei->setHorizontalHeaderItem(COL_M_MAG, new QTableWidgetItem{"|M|"});
		m_nuclei->setHorizontalHeaderItem(COL_ReM_X, new QTableWidgetItem{"Re{FC_x}"});
		m_nuclei->setHorizontalHeaderItem(COL_ReM_Y, new QTableWidgetItem{"Re{FC_y}"});
		m_nuclei->setHorizontalHeaderItem(COL_ReM_Z, new QTableWidgetItem{"Re{FC_z}"});
		m_nuclei->setHorizontalHeaderItem(COL_ImM_X, new QTableWidgetItem{"Im{FC_x}"});
		m_nuclei->setHorizontalHeaderItem(COL_ImM_Y, new QTableWidgetItem{"Im{FC_y}"});
		m_nuclei->setHorizontalHeaderItem(COL_ImM_Z, new QTableWidgetItem{"Im{FC_z}"});
		m_nuclei->setHorizontalHeaderItem(COL_RAD, new QTableWidgetItem{"Scale"});
		m_nuclei->setHorizontalHeaderItem(COL_COL, new QTableWidgetItem{"Colour"});

		m_nuclei->setColumnWidth(COL_NAME, 90);
		m_nuclei->setColumnWidth(COL_X, 75);
		m_nuclei->setColumnWidth(COL_Y, 75);
		m_nuclei->setColumnWidth(COL_Z, 75);
		m_nuclei->setColumnWidth(COL_M_MAG, 75);
		m_nuclei->setColumnWidth(COL_ReM_X, 75);
		m_nuclei->setColumnWidth(COL_ReM_Y, 75);
		m_nuclei->setColumnWidth(COL_ReM_Z, 75);
		m_nuclei->setColumnWidth(COL_ImM_X, 75);
		m_nuclei->setColumnWidth(COL_ImM_Y, 75);
		m_nuclei->setColumnWidth(COL_ImM_Z, 75);
		m_nuclei->setColumnWidth(COL_RAD, 75);
		m_nuclei->setColumnWidth(COL_COL, 75);

		QToolButton *pTabBtnAdd = new QToolButton(m_nucleipanel);
		QToolButton *pTabBtnDel = new QToolButton(m_nucleipanel);
		QToolButton *pTabBtnUp = new QToolButton(m_nucleipanel);
		QToolButton *pTabBtnDown = new QToolButton(m_nucleipanel);
		QToolButton *pTabBtnSG = new QToolButton(m_nucleipanel);

		m_nuclei->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});
		pTabBtnAdd->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDel->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnUp->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDown->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnSG->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});

		pTabBtnAdd->setText("Add Fourier Comp.");
		pTabBtnDel->setText("Delete Fourier Comp.");
		pTabBtnUp->setText("Move Fourier Comp. Up");
		pTabBtnDown->setText("Move Fourier Comp. Down");
		pTabBtnSG->setText("Generate");

		m_editA = new QLineEdit("5", m_nucleipanel);
		m_editB = new QLineEdit("5", m_nucleipanel);
		m_editC = new QLineEdit("5", m_nucleipanel);
		m_editAlpha = new QLineEdit("90", m_nucleipanel);
		m_editBeta = new QLineEdit("90", m_nucleipanel);
		m_editGamma = new QLineEdit("90", m_nucleipanel);

		m_comboSG = new QComboBox(m_nucleipanel);


		// get space groups and symops
		auto spacegroups = get_sgs<t_mat>();
		m_SGops.reserve(spacegroups.size());
		for(auto [sgnum, descr, ops] : spacegroups)
		{
			m_comboSG->addItem(descr.c_str(), m_comboSG->count());
			m_SGops.emplace_back(std::move(ops));
		}


		auto pTabGrid = new QGridLayout(m_nucleipanel);
		pTabGrid->setSpacing(2);
		pTabGrid->setContentsMargins(4,4,4,4);
		int y = 0;
		//pTabGrid->addWidget(m_plot.get(), y,0,1,4);
		pTabGrid->addWidget(m_nuclei, y,0,1,4);
		pTabGrid->addWidget(pTabBtnAdd, ++y,0,1,1);
		pTabGrid->addWidget(pTabBtnDel, y,1,1,1);
		pTabGrid->addWidget(pTabBtnUp, y,2,1,1);
		pTabGrid->addWidget(pTabBtnDown, y,3,1,1);

		pTabGrid->addWidget(new QLabel("Space Group:"), ++y,0,1,1);
		pTabGrid->addWidget(m_comboSG, y,1,1,2);
		pTabGrid->addWidget(pTabBtnSG, y,3,1,1);


		auto sep1 = new QFrame(m_nucleipanel); sep1->setFrameStyle(QFrame::HLine);
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
		QMenu *pTabContextMenu = new QMenu(m_nuclei);
		pTabContextMenu->addAction("Add Fourier Component Before", this, [this]() { this->AddTabItem(-2); });
		pTabContextMenu->addAction("Add Fourier Component After", this, [this]() { this->AddTabItem(-3); });
		pTabContextMenu->addAction("Clone Fourier Component", this, [this]() { this->AddTabItem(-4); });
		pTabContextMenu->addAction("Delete Fourier Component", this, [this]() { this->DelTabItem(); });


		// table CustomContextMenu in case nothing is selected
		QMenu *pTabContextMenuNoItem = new QMenu(m_nuclei);
		pTabContextMenuNoItem->addAction("Add Fourier Component", this, [this]() { this->AddTabItem(); });
		pTabContextMenuNoItem->addAction("Delete Fourier Component", this, [this]() { this->DelTabItem(); });
		//pTabContextMenuNoItem->addSeparator();


		// signals
		for(auto* edit : std::vector<QLineEdit*>{{ m_editA, m_editB, m_editC, m_editAlpha, m_editBeta, m_editGamma }})
			connect(edit, &QLineEdit::textEdited, this, [this]() { this->CalcB(); });

		connect(pTabBtnAdd, &QToolButton::clicked, this, [this]() { this->AddTabItem(-1); });
		connect(pTabBtnDel, &QToolButton::clicked, this, [this]() { this->DelTabItem(); });
		connect(pTabBtnUp, &QToolButton::clicked, this, [this]() { this->MoveTabItemUp(m_nuclei); });
		connect(pTabBtnDown, &QToolButton::clicked, this, [this]() { this->MoveTabItemDown(m_nuclei); });
		connect(pTabBtnSG, &QToolButton::clicked, this, &MagStructFactDlg::GenerateFromSG);

		connect(m_nuclei, &QTableWidget::currentCellChanged, this, &MagStructFactDlg::TableCurCellChanged);
		connect(m_nuclei, &QTableWidget::entered, this, &MagStructFactDlg::TableCellEntered);
		connect(m_nuclei, &QTableWidget::itemChanged, this, &MagStructFactDlg::TableItemChanged);
		connect(m_nuclei, &QTableWidget::customContextMenuRequested, this,
			[this, pTabContextMenu, pTabContextMenuNoItem](const QPoint& pt)
			{ this->ShowTableContextMenu(m_nuclei, pTabContextMenu, pTabContextMenuNoItem, pt); });

		tabs->addTab(m_nucleipanel, "Fourier Components");
	}


	{	// propagation vectors panel
		m_propvecpanel = new QWidget(this);

		m_propvecs = new QTableWidget(m_propvecpanel);
		m_propvecs->setShowGrid(true);
		m_propvecs->setSortingEnabled(true);
		m_propvecs->setMouseTracking(true);
		m_propvecs->setSelectionBehavior(QTableWidget::SelectRows);
		m_propvecs->setSelectionMode(QTableWidget::ContiguousSelection);
		m_propvecs->setContextMenuPolicy(Qt::CustomContextMenu);

		m_propvecs->verticalHeader()->setDefaultSectionSize(fontMetrics().lineSpacing() + 4);
		m_propvecs->verticalHeader()->setVisible(false);

		m_propvecs->setColumnCount(PROP_NUM_COLS);
		m_propvecs->setHorizontalHeaderItem(PROP_COL_NAME, new QTableWidgetItem{"Name"});
		m_propvecs->setHorizontalHeaderItem(PROP_COL_X, new QTableWidgetItem{"x (frac.)"});
		m_propvecs->setHorizontalHeaderItem(PROP_COL_Y, new QTableWidgetItem{"y (frac.)"});
		m_propvecs->setHorizontalHeaderItem(PROP_COL_Z, new QTableWidgetItem{"z (frac.)"});
		m_propvecs->setHorizontalHeaderItem(PROP_COL_CONJ, new QTableWidgetItem{"FC*"});

		m_propvecs->setColumnWidth(PROP_COL_NAME, 90);
		m_propvecs->setColumnWidth(PROP_COL_X, 90);
		m_propvecs->setColumnWidth(PROP_COL_Y, 90);
		m_propvecs->setColumnWidth(PROP_COL_Z, 90);
		m_propvecs->setColumnWidth(PROP_COL_CONJ, 75);

		QToolButton *pTabBtnAdd = new QToolButton(m_propvecpanel);
		QToolButton *pTabBtnDel = new QToolButton(m_propvecpanel);
		QToolButton *pTabBtnUp = new QToolButton(m_propvecpanel);
		QToolButton *pTabBtnDown = new QToolButton(m_propvecpanel);

		m_propvecs->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});
		pTabBtnAdd->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDel->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnUp->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDown->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});

		pTabBtnAdd->setText("Add Vector");
		pTabBtnDel->setText("Delete Vector");
		pTabBtnUp->setText("Move Vector Up");
		pTabBtnDown->setText("Move Vector Down");


		auto pTabGrid = new QGridLayout(m_propvecpanel);
		pTabGrid->setSpacing(2);
		pTabGrid->setContentsMargins(4,4,4,4);
		int y = 0;
		pTabGrid->addWidget(m_propvecs, y,0,1,4);
		pTabGrid->addWidget(pTabBtnAdd, ++y,0,1,1);
		pTabGrid->addWidget(pTabBtnDel, y,1,1,1);
		pTabGrid->addWidget(pTabBtnUp, y,2,1,1);
		pTabGrid->addWidget(pTabBtnDown, y,3,1,1);


		// table CustomContextMenu
		QMenu *pPropContextMenu = new QMenu(m_propvecs);
		pPropContextMenu->addAction("Add Vector Before", this, [this]() { this->AddPropItem(-2); });
		pPropContextMenu->addAction("Add Vector After", this, [this]() { this->AddPropItem(-3); });
		pPropContextMenu->addAction("Clone Vector", this, [this]() { this->AddPropItem(-4); });
		pPropContextMenu->addAction("Delete Vector", this, [this]() { this->DelPropItem(); });


		// table CustomContextMenu in case nothing is selected
		QMenu *pPropContextMenuNoItem = new QMenu(m_propvecs);
		pPropContextMenuNoItem->addAction("Add Vector", this, [this]() { this->AddPropItem(); });
		pPropContextMenuNoItem->addAction("Delete Vector", this, [this]() { this->DelPropItem(); });



		connect(pTabBtnAdd, &QToolButton::clicked, this, [this]() { this->AddPropItem(-1); });
		connect(pTabBtnDel, &QToolButton::clicked, this, [this]() { this->DelPropItem(); });
		connect(pTabBtnUp, &QToolButton::clicked, this,  [this]() { this->MoveTabItemUp(m_propvecs); });
		connect(pTabBtnDown, &QToolButton::clicked, this,  [this]() { this->MoveTabItemUp(m_propvecs); });

		//connect(m_propvecs, &QTableWidget::currentCellChanged, this, &MagStructFactDlg::PropCurCellChanged);
		//connect(m_propvecs, &QTableWidget::entered, this, &MagStructFactDlg::PropCellEntered);
		connect(m_propvecs, &QTableWidget::itemChanged, this, &MagStructFactDlg::PropItemChanged);
		connect(m_propvecs, &QTableWidget::customContextMenuRequested, this,
			[this, pPropContextMenu, pPropContextMenuNoItem](const QPoint& pt)
			{ this->ShowTableContextMenu(m_propvecs, pPropContextMenu, pPropContextMenuNoItem, pt); });

		tabs->addTab(m_propvecpanel, "Propagation Vectors");
	}


	{	// structure factors panel
		auto sfactpanel = new QWidget(this);
		auto pGrid = new QGridLayout(sfactpanel);
		pGrid->setSpacing(4);
		pGrid->setContentsMargins(4,4,4,4);

		m_structfacts = new QPlainTextEdit(sfactpanel);
		m_structfacts->setReadOnly(true);
		m_structfacts->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
		m_structfacts->setLineWrapMode(QPlainTextEdit::NoWrap);

		m_maxBZ = new QSpinBox(sfactpanel);
		m_maxBZ->setMinimum(0);
		m_maxBZ->setMaximum(99);
		m_maxBZ->setValue(4);

		m_RemoveZeroes = new QCheckBox("Remove Zeroes", sfactpanel);
		m_RemoveZeroes->setChecked(true);


		pGrid->addWidget(m_structfacts, 0,0, 1,4);
		pGrid->addWidget(new QLabel("Max. Order:"), 1,0,1,1);
		pGrid->addWidget(m_maxBZ, 1,1, 1,1);
		pGrid->addWidget(m_RemoveZeroes, 1,2, 1,2);


		// signals
		connect(m_maxBZ, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this]() { this->Calc(); });
		connect(m_RemoveZeroes, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged), this, [this]() { this->Calc(); });

		tabs->addTab(sfactpanel, "Structure Factors");
	}


	{	// powder lines panel
		auto powderpanel = new QWidget(this);
		auto pGrid = new QGridLayout(powderpanel);
		pGrid->setSpacing(4);
		pGrid->setContentsMargins(4,4,4,4);

		m_powderlines = new QPlainTextEdit(powderpanel);
		m_powderlines->setReadOnly(true);
		m_powderlines->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
		m_powderlines->setLineWrapMode(QPlainTextEdit::NoWrap);

		pGrid->addWidget(m_powderlines, 0,0, 1,4);

		tabs->addTab(powderpanel, "Powder Lines");
	}


	{	// real magnetic moments
		auto mmpanel = new QWidget(this);
		auto pGrid = new QGridLayout(mmpanel);
		pGrid->setSpacing(4);
		pGrid->setContentsMargins(4,4,4,4);

		m_moments = new QPlainTextEdit(mmpanel);
		m_moments->setReadOnly(true);
		m_moments->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
		m_moments->setLineWrapMode(QPlainTextEdit::NoWrap);

		for(auto*& spin : m_maxSC)
		{
			spin = new QSpinBox(mmpanel);
			spin->setMinimum(0);
			spin->setMaximum(999);
			spin->setValue(4);
		}

		m_maxSC[0]->setPrefix("x = ");
		m_maxSC[1]->setPrefix("y = ");
		m_maxSC[2]->setPrefix("z = ");

		pGrid->addWidget(m_moments, 0,0, 1,4);
		pGrid->addWidget(new QLabel("Max. Supercell Order:"), 1,0,1,1);
		pGrid->addWidget(m_maxSC[0], 1,1, 1,1);
		pGrid->addWidget(m_maxSC[1], 1,2, 1,1);
		pGrid->addWidget(m_maxSC[2], 1,3, 1,1);


		// signals
		for(auto* spin : m_maxSC)
			connect(spin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this]() { this->Calc(); });

		tabs->addTab(mmpanel, "Magnetic Moments");
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
			m_labelGlInfos[i]->setSizePolicy(QSizePolicy::Ignored, m_labelGlInfos[i]->sizePolicy().verticalPolicy());
		}

		auto sep1 = new QFrame(infopanel); sep1->setFrameStyle(QFrame::HLine);
		auto sep2 = new QFrame(infopanel); sep2->setFrameStyle(QFrame::HLine);
		auto sep3 = new QFrame(infopanel); sep3->setFrameStyle(QFrame::HLine);

		std::string strBoost = BOOST_LIB_VERSION;
		algo::replace_all(strBoost, "_", ".");

		auto labelTitle = new QLabel("Takin / Magnetic Structure Factor Calculator", infopanel);
		auto fontTitle = labelTitle->font();
		fontTitle.setBold(true);
		labelTitle->setFont(fontTitle);
		labelTitle->setAlignment(Qt::AlignHCenter);

		auto labelAuthor = new QLabel("Written by Tobias Weber <tweber@ill.fr>.", infopanel);
		labelAuthor->setAlignment(Qt::AlignHCenter);

		auto labelDate = new QLabel("January 2019.", infopanel);
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
		m_menu->setNativeMenuBar(m_sett ? m_sett->value("native_gui", false).toBool() : false);

		auto menuFile = new QMenu("File", m_menu);
		auto menuView = new QMenu("3D View", m_menu);

		auto acNew = new QAction("New", menuFile);
		auto acLoad = new QAction("Load...", menuFile);
		auto acSave = new QAction("Save...", menuFile);
		auto acImportCIF = new QAction("Import CIF...", menuFile);
		auto acExit = new QAction("Quit", menuFile);
		auto ac3DView = new QAction("Unit Cell / Fourier Components...", menuFile);
		auto ac3DViewSC = new QAction("Super Cell / Magnetic Moments...", menuFile);

		acNew->setShortcut(QKeySequence::New);
		acLoad->setShortcut(QKeySequence::Open);
		acSave->setShortcut(QKeySequence::Save);
		acExit->setShortcut(QKeySequence::Quit);

		acExit->setMenuRole(QAction::QuitRole);

		// not yet implemented
		acImportCIF->setEnabled(false);

		menuFile->addAction(acNew);
		menuFile->addSeparator();
		menuFile->addAction(acLoad);
		menuFile->addAction(acSave);
		menuFile->addSeparator();
		menuFile->addAction(acImportCIF);
		menuFile->addSeparator();
		menuFile->addAction(acExit);
		menuView->addAction(ac3DView);
		menuView->addAction(ac3DViewSC);

		connect(acNew, &QAction::triggered, this,  [this]()
		{
			// clear old tables
			DelTabItem(-1);
			DelPropItem(-1);

			// set some defaults
			m_comboSG->setCurrentIndex(0);
			m_editA->setText("5");
			m_editB->setText("5");
			m_editC->setText("5");
			m_editAlpha->setText("90");
			m_editBeta->setText("90");
			m_editGamma->setText("90");
		});
		connect(acLoad, &QAction::triggered, this, &MagStructFactDlg::Load);
		connect(acSave, &QAction::triggered, this, &MagStructFactDlg::Save);
		connect(acImportCIF, &QAction::triggered, this, &MagStructFactDlg::ImportCIF);
		connect(acExit, &QAction::triggered, this, &QDialog::close);


		// unit cell view
		connect(ac3DView, &QAction::triggered, this, [this]()
		{
			// plot widget
			if(!m_dlgPlot)
			{
				m_dlgPlot = new QDialog(this);
				m_dlgPlot->setWindowTitle("Unit Cell");
				m_dlgPlot->setFont(this->font());

				m_plot = std::make_shared<tl2::GlPlot>(this);
				m_plot->setFormat(tl2::gl_format(1, _GL_MAJ_VER, _GL_MIN_VER, 8, m_plot->format()));

				m_plot->GetRenderer()->SetRestrictCamTheta(false);
				m_plot->GetRenderer()->SetLight(0, tl2::create<t_vec3_gl>({ 5, 5, 5 }));
				m_plot->GetRenderer()->SetLight(1, tl2::create<t_vec3_gl>({ -5, -5, -5 }));
				m_plot->GetRenderer()->SetCoordMax(1.);
				m_plot->GetRenderer()->GetCamera().SetDist(1.5);
				m_plot->GetRenderer()->GetCamera().UpdateTransformation();

				auto labCoordSys = new QLabel("Coordinate System:", /*m_dlgPlot*/ this);
				auto comboCoordSys = new QComboBox(/*m_dlgPlot*/ this);
				m_status3D = new QLabel(/*m_dlgPlot*/ this);

				comboCoordSys->addItem("Fractional Units (rlu)");
				comboCoordSys->addItem("Lab Units (A)");


				m_plot->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});
				labCoordSys->setSizePolicy(QSizePolicy{QSizePolicy::Fixed, QSizePolicy::Fixed});

				auto grid = new QGridLayout(m_dlgPlot);
				grid->setSpacing(2);
				grid->setContentsMargins(4,4,4,4);
				grid->addWidget(m_plot.get(), 0,0,1,2);
				grid->addWidget(labCoordSys, 1,0,1,1);
				grid->addWidget(comboCoordSys, 1,1,1,1);
				grid->addWidget(m_status3D, 2,0,1,2);


				connect(m_plot.get(), &tl2::GlPlot::AfterGLInitialisation, this, &MagStructFactDlg::AfterGLInitialisation);
				connect(m_plot->GetRenderer(), &tl2::GlPlotRenderer::PickerIntersection, this, &MagStructFactDlg::PickerIntersection);
				connect(m_plot.get(), &tl2::GlPlot::MouseDown, this, &MagStructFactDlg::PlotMouseDown);
				//connect(m_plot.get(), &tl2::GlPlot::MouseUp, this, [this](bool left, bool mid, bool right) {});
				connect(comboCoordSys, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int val)
				{
					if(this->m_plot)
						this->m_plot->GetRenderer()->SetCoordSys(val);
				});


				if(m_sett && m_sett->contains("geo_3dview"))
					m_dlgPlot->restoreGeometry(m_sett->value("geo_3dview").toByteArray());
				else
					m_dlgPlot->resize(500,500);
			}

			m_dlgPlot->show();
			m_dlgPlot->raise();
			m_dlgPlot->focusWidget();
		});


		// super cell view
		connect(ac3DViewSC, &QAction::triggered, this, [this]()
		{
			// plot widget
			if(!m_dlgPlotSC)
			{
				m_dlgPlotSC = new QDialog(this);
				m_dlgPlotSC->setWindowTitle("Super Cell");
				m_dlgPlotSC->setFont(this->font());

				m_plotSC = std::make_shared<tl2::GlPlot>(this);
				m_plotSC->setFormat(tl2::gl_format(1, _GL_MAJ_VER, _GL_MIN_VER, 8, m_plotSC->format()));

				m_plotSC->GetRenderer()->SetRestrictCamTheta(false);
				m_plotSC->GetRenderer()->SetLight(0, tl2::create<t_vec3_gl>({ 5, 5, 5 }));
				m_plotSC->GetRenderer()->SetLight(1, tl2::create<t_vec3_gl>({ -5, -5, -5 }));
				m_plotSC->GetRenderer()->SetCoordMax(1.);
				m_plotSC->GetRenderer()->GetCamera().SetDist(5.);
				m_plotSC->GetRenderer()->GetCamera().UpdateTransformation();

				auto labCoordSys = new QLabel("Coordinate System:", /*m_dlgPlotSC*/ this);
				auto comboCoordSys = new QComboBox(/*m_dlgPlotSC*/ this);
				m_status3DSC = new QLabel(/*m_dlgPlotSC*/ this);

				comboCoordSys->addItem("Fractional Units (rlu)");
				comboCoordSys->addItem("Lab Units (A)");


				m_plotSC->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});
				labCoordSys->setSizePolicy(QSizePolicy{QSizePolicy::Fixed, QSizePolicy::Fixed});

				auto grid = new QGridLayout(m_dlgPlotSC);
				grid->setSpacing(2);
				grid->setContentsMargins(4,4,4,4);
				grid->addWidget(m_plotSC.get(), 0,0,1,2);
				grid->addWidget(labCoordSys, 1,0,1,1);
				grid->addWidget(comboCoordSys, 1,1,1,1);
				grid->addWidget(m_status3DSC, 2,0,1,2);


				connect(m_plotSC.get(), &tl2::GlPlot::AfterGLInitialisation, this, &MagStructFactDlg::AfterGLInitialisationSC);
				connect(m_plotSC->GetRenderer(), &tl2::GlPlotRenderer::PickerIntersection, this, &MagStructFactDlg::PickerIntersectionSC);
				//connect(m_plotSC.get(), &tl2::GlPlot::MouseDown, this, [this](bool left, bool mid, bool right) {});
				//connect(m_plotSC.get(), &tl2::GlPlot::MouseUp, this, [this](bool left, bool mid, bool right) {});
				connect(comboCoordSys, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int val)
				{
					if(this->m_plotSC)
						this->m_plotSC->GetRenderer()->SetCoordSys(val);
				});


				if(m_sett && m_sett->contains("geo_3dview_sc"))
					m_dlgPlotSC->restoreGeometry(m_sett->value("geo_3dview_sc").toByteArray());
				else
					m_dlgPlotSC->resize(500,500);
			}

			m_dlgPlotSC->show();
			m_dlgPlotSC->raise();
			m_dlgPlotSC->focusWidget();
		});

		m_menu->addMenu(menuFile);
		m_menu->addMenu(menuView);
		pmainGrid->setMenuBar(m_menu);
	}


	// restory window size and position
	if(m_sett && m_sett->contains("geo"))
		restoreGeometry(m_sett->value("geo").toByteArray());
	else
		resize(600, 500);


	m_ignoreChanges = 0;
}



void MagStructFactDlg::closeEvent(QCloseEvent *evt)
{
	if(m_sett)
	{
		m_sett->setValue("geo", saveGeometry());
		if(m_dlgPlot)
			m_sett->setValue("geo_3dview", m_dlgPlot->saveGeometry());
		if(m_dlgPlotSC)
			m_sett->setValue("geo_3dview_sc", m_dlgPlotSC->saveGeometry());
	}
}
