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

#include <QtCore/QMimeData>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
namespace algo = boost::algorithm;

#include "loadcif.h"
#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/phys.h"
#include "tlibs2/libs/algos.h"
#include "tlibs2/libs/qt/helper.h"

using namespace tl2_ops;


t_real g_eps = 1e-6;
int g_prec = 6;


// ----------------------------------------------------------------------------
StructFactDlg::StructFactDlg(QWidget* pParent) : QDialog{pParent},
	m_sett{new QSettings{"takin", "structfact"}}
{
	setWindowTitle("Nuclear Structure Factors");
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


	{ // nuclei panel
		QWidget *nucleipanel = new QWidget(this);

		m_nuclei = new QTableWidget(nucleipanel);
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
		m_nuclei->setHorizontalHeaderItem(COL_SCATLEN_RE, new QTableWidgetItem{"Re{b} (fm)"});
		m_nuclei->setHorizontalHeaderItem(COL_SCATLEN_IM, new QTableWidgetItem{"Im{b} (fm)"});
		m_nuclei->setHorizontalHeaderItem(COL_X, new QTableWidgetItem{"x (frac.)"});
		m_nuclei->setHorizontalHeaderItem(COL_Y, new QTableWidgetItem{"y (frac.)"});
		m_nuclei->setHorizontalHeaderItem(COL_Z, new QTableWidgetItem{"z (frac.)"});
		m_nuclei->setHorizontalHeaderItem(COL_RAD, new QTableWidgetItem{"Radius"});
		m_nuclei->setHorizontalHeaderItem(COL_COL, new QTableWidgetItem{"Colour"});

		m_nuclei->setColumnWidth(COL_NAME, 90);
		m_nuclei->setColumnWidth(COL_SCATLEN_RE, 75);
		m_nuclei->setColumnWidth(COL_SCATLEN_IM, 75);
		m_nuclei->setColumnWidth(COL_X, 75);
		m_nuclei->setColumnWidth(COL_Y, 75);
		m_nuclei->setColumnWidth(COL_Z, 75);
		m_nuclei->setColumnWidth(COL_RAD, 75);
		m_nuclei->setColumnWidth(COL_COL, 75);

		QToolButton *pTabBtnAdd = new QToolButton(nucleipanel);
		QToolButton *pTabBtnDel = new QToolButton(nucleipanel);
		QToolButton *pTabBtnUp = new QToolButton(nucleipanel);
		QToolButton *pTabBtnDown = new QToolButton(nucleipanel);
		QToolButton *pTabBtnSG = new QToolButton(nucleipanel);

		m_nuclei->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});
		pTabBtnAdd->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDel->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnUp->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDown->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnSG->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});

		pTabBtnAdd->setText("Add Nucleus");
		pTabBtnDel->setText("Delete Nuclei");
		pTabBtnUp->setText("Move Nuclei Up");
		pTabBtnDown->setText("Move Nuclei Down");
		pTabBtnSG->setText("Generate");

		m_editA = new QLineEdit("5", nucleipanel);
		m_editB = new QLineEdit("5", nucleipanel);
		m_editC = new QLineEdit("5", nucleipanel);
		m_editAlpha = new QLineEdit("90", nucleipanel);
		m_editBeta = new QLineEdit("90", nucleipanel);
		m_editGamma = new QLineEdit("90", nucleipanel);

		m_comboSG = new QComboBox(nucleipanel);


		// get space groups and symops
		auto spacegroups = get_sgs<t_mat>();
		m_SGops.reserve(spacegroups.size());
		m_SGops_centr.reserve(spacegroups.size());
		for(auto [sgnum, descr, ops] : spacegroups)
		{
			m_comboSG->addItem(descr.c_str(), m_comboSG->count());
			m_SGops.emplace_back(std::move(ops));

			// determine centring ops
			std::vector<t_mat> ops_centr;
			for(const t_mat& op : ops)
			{
				if(tl2::hom_is_centring<t_mat>(op, g_eps))
					ops_centr.push_back(op);
			}
			m_SGops_centr.emplace_back(std::move(ops_centr));
		}


		auto pTabGrid = new QGridLayout(nucleipanel);
		pTabGrid->setSpacing(2);
		pTabGrid->setContentsMargins(4,4,4,4);
		int y=0;
		//pTabGrid->addWidget(m_plot.get(), y,0,1,4);
		pTabGrid->addWidget(m_nuclei, y,0,1,4);
		pTabGrid->addWidget(pTabBtnAdd, ++y,0,1,1);
		pTabGrid->addWidget(pTabBtnDel, y,1,1,1);
		pTabGrid->addWidget(pTabBtnUp, y,2,1,1);
		pTabGrid->addWidget(pTabBtnDown, y,3,1,1);

		pTabGrid->addWidget(new QLabel("Space Group:"), ++y,0,1,1);
		pTabGrid->addWidget(m_comboSG, y,1,1,2);
		pTabGrid->addWidget(pTabBtnSG, y,3,1,1);


		auto sep1 = new QFrame(nucleipanel); sep1->setFrameStyle(QFrame::HLine);
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
		m_pTabContextMenu = new QMenu(m_nuclei);
		m_pTabContextMenu->addAction("Add Nucleus Before", this, [this]() { this->AddTabItem(-2); });
		m_pTabContextMenu->addAction("Add Nucleus After", this, [this]() { this->AddTabItem(-3); });
		m_pTabContextMenu->addAction("Clone Nucleus", this, [this]() { this->AddTabItem(-4); });
		m_pTabContextMenu->addAction("Delete Nucleus", this, [this]() { StructFactDlg::DelTabItem(); });


		// table CustomContextMenu in case nothing is selected
		m_pTabContextMenuNoItem = new QMenu(m_nuclei);
		m_pTabContextMenuNoItem->addAction("Add Nucleus", this, [this]() { this->AddTabItem(); });
		m_pTabContextMenuNoItem->addAction("Delete Nucleus", this, [this]() { StructFactDlg::DelTabItem(); });
		//m_pTabContextMenuNoItem->addSeparator();


		// signals
		for(auto* edit : std::vector<QLineEdit*>{{ m_editA, m_editB, m_editC, m_editAlpha, m_editBeta, m_editGamma }})
			connect(edit, &QLineEdit::textEdited, this, [this]() { this->CalcB(); });

		connect(pTabBtnAdd, &QToolButton::clicked, this, [this]() { this->AddTabItem(-1); });
		connect(pTabBtnDel, &QToolButton::clicked, this, [this]() { StructFactDlg::DelTabItem(); });
		connect(pTabBtnUp, &QToolButton::clicked, this, &StructFactDlg::MoveTabItemUp);
		connect(pTabBtnDown, &QToolButton::clicked, this, &StructFactDlg::MoveTabItemDown);
		connect(pTabBtnSG, &QToolButton::clicked, this, &StructFactDlg::GenerateFromSG);

		connect(m_nuclei, &QTableWidget::currentCellChanged, this, &StructFactDlg::TableCurCellChanged);
		connect(m_nuclei, &QTableWidget::entered, this, &StructFactDlg::TableCellEntered);
		connect(m_nuclei, &QTableWidget::itemChanged, this, &StructFactDlg::TableItemChanged);
		connect(m_nuclei, &QTableWidget::customContextMenuRequested, this, &StructFactDlg::ShowTableContextMenu);

		tabs->addTab(nucleipanel, "Nuclei");
	}


	{	// structure factors panel
		auto sfactpanel = new QWidget(this);
		auto pGrid = new QGridLayout(sfactpanel);
		pGrid->setSpacing(4);
		pGrid->setContentsMargins(4,4,4,4);

		m_structfacts = new QPlainTextEdit(sfactpanel);
		m_structfacts->setReadOnly(true);
		m_structfacts->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

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


	{ // find space group panel
		auto findsgpanel = new QWidget(this);

		m_nuclei_FindSG = new QTableWidget(findsgpanel);
		m_nuclei_FindSG->setShowGrid(true);
		m_nuclei_FindSG->setSortingEnabled(true);
		m_nuclei_FindSG->setMouseTracking(true);
		m_nuclei_FindSG->setSelectionBehavior(QTableWidget::SelectRows);
		m_nuclei_FindSG->setSelectionMode(QTableWidget::ContiguousSelection);
		m_nuclei_FindSG->setContextMenuPolicy(Qt::CustomContextMenu);

		m_nuclei_FindSG->verticalHeader()->setDefaultSectionSize(fontMetrics().lineSpacing() + 4);
		m_nuclei_FindSG->verticalHeader()->setVisible(false);

		m_nuclei_FindSG->setColumnCount(3);
		m_nuclei_FindSG->setHorizontalHeaderItem(0, new QTableWidgetItem{"x (frac.)"});
		m_nuclei_FindSG->setHorizontalHeaderItem(1, new QTableWidgetItem{"y (frac.)"});
		m_nuclei_FindSG->setHorizontalHeaderItem(2, new QTableWidgetItem{"z (frac.)"});

		m_nuclei_FindSG->setColumnWidth(0, 200);
		m_nuclei_FindSG->setColumnWidth(1, 200);
		m_nuclei_FindSG->setColumnWidth(2, 200);

		m_sgmatches = new QPlainTextEdit(findsgpanel);
		m_sgmatches->setReadOnly(true);
		m_sgmatches->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));


		QToolButton *pTabBtnAdd = new QToolButton(findsgpanel);
		QToolButton *pTabBtnDel = new QToolButton(findsgpanel);
		QToolButton *pTabBtnUp = new QToolButton(findsgpanel);
		QToolButton *pTabBtnDown = new QToolButton(findsgpanel);
		QToolButton *pBtnCalc = new QToolButton(findsgpanel);

		m_nuclei_FindSG->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});
		pTabBtnAdd->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDel->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnUp->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDown->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pBtnCalc->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});

		pTabBtnAdd->setText("Add Nucleus");
		pTabBtnDel->setText("Delete Nuclei");
		pTabBtnUp->setText("Move Nuclei Up");
		pTabBtnDown->setText("Move Nuclei Down");
		pBtnCalc->setText("Find Matching Space Groups");


		auto pTabGrid = new QGridLayout(findsgpanel);
		pTabGrid->setSpacing(2);
		pTabGrid->setContentsMargins(4,4,4,4);
		int y=0;
		pTabGrid->addWidget(m_nuclei_FindSG, y,0,1,4);
		pTabGrid->addWidget(m_sgmatches, ++y,0,1,4);
		pTabGrid->addWidget(pTabBtnAdd, ++y,0,1,1);
		pTabGrid->addWidget(pTabBtnDel, y,1,1,1);
		pTabGrid->addWidget(pTabBtnUp, y,2,1,1);
		pTabGrid->addWidget(pTabBtnDown, y,3,1,1);
		pTabGrid->addWidget(pBtnCalc, ++y,2,1,2);


		// table CustomContextMenu
		m_pTabContextMenu_FindSG = new QMenu(m_nuclei_FindSG);
		m_pTabContextMenu_FindSG->addAction("Add Nucleus Before", this, [this]() { this->AddTabItem_FindSG(-2); });
		m_pTabContextMenu_FindSG->addAction("Add Nucleus After", this, [this]() { this->AddTabItem_FindSG(-3); });
		m_pTabContextMenu_FindSG->addAction("Clone Nucleus", this, [this]() { this->AddTabItem_FindSG(-4); });
		m_pTabContextMenu_FindSG->addAction("Delete Nucleus", this, [this]() { StructFactDlg::DelTabItem_FindSG(); });


		// table CustomContextMenu in case nothing is selected
		m_pTabContextMenuNoItem_FindSG = new QMenu(m_nuclei_FindSG);
		m_pTabContextMenuNoItem_FindSG->addAction("Add Nucleus", this, [this]() { this->AddTabItem_FindSG(); });
		m_pTabContextMenuNoItem_FindSG->addAction("Delete Nucleus", this, [this]() { StructFactDlg::DelTabItem_FindSG(); });


		// signals
		connect(pTabBtnAdd, &QToolButton::clicked, this, [this]() { this->AddTabItem_FindSG(-1); });
		connect(pTabBtnDel, &QToolButton::clicked, this, [this]() { StructFactDlg::DelTabItem_FindSG(); });
		connect(pTabBtnUp, &QToolButton::clicked, this, &StructFactDlg::MoveTabItemUp_FindSG);
		connect(pTabBtnDown, &QToolButton::clicked, this, &StructFactDlg::MoveTabItemDown_FindSG);
		connect(pBtnCalc, &QToolButton::clicked, this, &StructFactDlg::FindSG);

		connect(m_nuclei_FindSG, &QTableWidget::itemChanged, this, &StructFactDlg::TableItemChanged_FindSG);
		connect(m_nuclei_FindSG, &QTableWidget::customContextMenuRequested, this, &StructFactDlg::ShowTableContextMenu_FindSG);

		tabs->addTab(findsgpanel, "Match Space Group");
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

		auto labelTitle = new QLabel("Takin / Nuclear Structure Factor Calculator", infopanel);
		auto fontTitle = labelTitle->font();
		fontTitle.setBold(true);
		labelTitle->setFont(fontTitle);
		labelTitle->setAlignment(Qt::AlignHCenter);

		auto labelAuthor = new QLabel("Written by Tobias Weber <tweber@ill.fr>.", infopanel);
		labelAuthor->setAlignment(Qt::AlignHCenter);

		auto labelDate = new QLabel("December 2018.", infopanel);
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
		auto menuView = new QMenu("View", m_menu);

		auto acNew = new QAction("New", menuFile);
		auto acLoad = new QAction("Load...", menuFile);
		auto acSave = new QAction("Save...", menuFile);
		auto acImportCIF = new QAction("Import CIF...", menuFile);
		auto acImportTAZ = new QAction("Import TAZ...", menuFile);
		auto acExportTAZ = new QAction("Export TAZ...", menuFile);
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
		menuFile->addAction(acImportTAZ);
		menuFile->addAction(acExportTAZ);
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
		connect(acLoad, &QAction::triggered, this, static_cast<void(StructFactDlg::*)()>(&StructFactDlg::Load));
		connect(acSave, &QAction::triggered, this, static_cast<void(StructFactDlg::*)()>(&StructFactDlg::Save));
		connect(acImportCIF, &QAction::triggered, this, &StructFactDlg::ImportCIF);
		connect(acImportTAZ, &QAction::triggered, this, &StructFactDlg::ImportTAZ);
		connect(acExportTAZ, &QAction::triggered, this, &StructFactDlg::ExportTAZ);
		connect(acExit, &QAction::triggered, this, &QDialog::close);
		connect(ac3DView, &QAction::triggered, this, &StructFactDlg::ShowStructPlot);

		m_menu->addMenu(menuFile);
		m_menu->addMenu(menuView);
		pmainGrid->setMenuBar(m_menu);
	}


	// restore window size and position
	if(m_sett && m_sett->contains("geo"))
		restoreGeometry(m_sett->value("geo").toByteArray());
	else
		resize(600, 500);


	setAcceptDrops(true);
	m_ignoreChanges = 0;
}


/**
 * a file is being dragged over the window
 */
void StructFactDlg::dragEnterEvent(QDragEnterEvent *evt)
{
	if(evt)
		evt->accept();
}


/**
 * a file is being dropped onto the window
 */
void StructFactDlg::dropEvent(QDropEvent *evt)
{
	const QMimeData *mime = evt->mimeData();
	if(!mime)
		return;

	for(const QUrl& url : mime->urls())
	{
		if(!url.isLocalFile())
			continue;

		Load(url.toLocalFile());
		evt->accept();
		break;
        }
}


void StructFactDlg::closeEvent(QCloseEvent *)
{
	if(m_sett)
	{
		m_sett->setValue("geo", saveGeometry());
		if(m_dlgPlot)
			m_sett->setValue("geo_3dview", m_dlgPlot->saveGeometry());
	}
}


