/**
 * magnon dynamics -- gui setup
 * @author Tobias Weber <tweber@ill.fr>
 * @date Jan-2022
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 28-Dec-2018 from my privately developed "misc" project (https://github.com/t-weber/misc).
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2022  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 * "misc" project
 * Copyright (C) 2017-2022  Tobias WEBER (privately developed).
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

#include "magdyn.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>

#include <boost/version.hpp>
#include <boost/config.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
namespace algo = boost::algorithm;

#include "../structfact/loadcif.h"


void MagDynDlg::CreateMainWindow()
{
	SetCurrentFile("");
	setSizeGripEnabled(true);

	m_tabs_in = new QTabWidget(this);
	m_tabs_out = new QTabWidget(this);

	// status
	m_status = new QLabel(this);
	m_status->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	m_status->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	// progress bar
	m_progress = new QProgressBar(this);
	m_progress->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	// start button
	m_btnStart = new QPushButton(QIcon::fromTheme("media-playback-start"), "Calculate", this);
	m_btnStart->setToolTip("Start calculation.");
	m_btnStart->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	// stop button
	QPushButton* btnStop = new QPushButton(QIcon::fromTheme("media-playback-stop"), "Stop", this);
	btnStop->setToolTip("Request stop to ongoing calculation.");
	btnStop->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	// splitter for input and output tabs
	m_split_inout = new QSplitter(this);
	m_split_inout->setOrientation(Qt::Horizontal);
	m_split_inout->setChildrenCollapsible(true);
	m_split_inout->addWidget(m_tabs_in);
	m_split_inout->addWidget(m_tabs_out);

	// main grid
	m_maingrid = new QGridLayout(this);
	m_maingrid->setSpacing(4);
	m_maingrid->setContentsMargins(6, 6, 6, 6);
	//m_maingrid->addWidget(m_tabs_in, 0,0, 1,3);
	//m_maingrid->addWidget(m_tabs_out, 0,3, 1,3);
	m_maingrid->addWidget(m_split_inout, 0,0, 1,7);
	m_maingrid->addWidget(m_status, 1,0, 1,3);
	m_maingrid->addWidget(m_progress, 1,3, 1,2);
	m_maingrid->addWidget(m_btnStart, 1,5, 1,1);
	m_maingrid->addWidget(btnStop, 1,6, 1,1);

	// signals
	connect(m_btnStart, &QAbstractButton::clicked, [this]() { this->CalcAll(); });
	connect(btnStop, &QAbstractButton::clicked, [this]() { m_stopRequested = true; });
}



void MagDynDlg::CreateSitesPanel()
{
	m_sitespanel = new QWidget(this);

	m_sitestab = new QTableWidget(m_sitespanel);
	m_sitestab->setShowGrid(true);
	m_sitestab->setSortingEnabled(true);
	m_sitestab->setMouseTracking(true);
	m_sitestab->setSelectionBehavior(QTableWidget::SelectRows);
	m_sitestab->setSelectionMode(QTableWidget::ContiguousSelection);
	m_sitestab->setContextMenuPolicy(Qt::CustomContextMenu);

	m_sitestab->verticalHeader()->setDefaultSectionSize(fontMetrics().lineSpacing() + 4);
	m_sitestab->verticalHeader()->setVisible(true);

	m_sitestab->setColumnCount(NUM_SITE_COLS);
	m_sitestab->setHorizontalHeaderItem(COL_SITE_NAME,
		new QTableWidgetItem{"Name"});
	m_sitestab->setHorizontalHeaderItem(COL_SITE_POS_X,
		new QTableWidgetItem{"x"});
	m_sitestab->setHorizontalHeaderItem(COL_SITE_POS_Y,
		new QTableWidgetItem{"y"});
	m_sitestab->setHorizontalHeaderItem(COL_SITE_POS_Z,
		new QTableWidgetItem{"z"});
	m_sitestab->setHorizontalHeaderItem(COL_SITE_SPIN_X,
		new QTableWidgetItem{"Spin x"});
	m_sitestab->setHorizontalHeaderItem(COL_SITE_SPIN_Y,
		new QTableWidgetItem{"Spin y"});
	m_sitestab->setHorizontalHeaderItem(COL_SITE_SPIN_Z,
		new QTableWidgetItem{"Spin z"});
	m_sitestab->setHorizontalHeaderItem(COL_SITE_SPIN_MAG,
		new QTableWidgetItem{"Spin |S|"});

	m_sitestab->setColumnWidth(COL_SITE_NAME, 90);
	m_sitestab->setColumnWidth(COL_SITE_POS_X, 80);
	m_sitestab->setColumnWidth(COL_SITE_POS_Y, 80);
	m_sitestab->setColumnWidth(COL_SITE_POS_Z, 80);
	m_sitestab->setColumnWidth(COL_SITE_SPIN_X, 80);
	m_sitestab->setColumnWidth(COL_SITE_SPIN_Y, 80);
	m_sitestab->setColumnWidth(COL_SITE_SPIN_Z, 80);
	m_sitestab->setColumnWidth(COL_SITE_SPIN_MAG, 80);
	m_sitestab->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Expanding});

	QPushButton *btnAdd = new QPushButton(
		QIcon::fromTheme("list-add"),
		"Add", m_sitespanel);
	QPushButton *btnDel = new QPushButton(
		QIcon::fromTheme("list-remove"),
		"Delete", m_sitespanel);
	QPushButton *btnUp = new QPushButton(
		QIcon::fromTheme("go-up"),
		"Up", m_sitespanel);
	QPushButton *btnDown = new QPushButton(
		QIcon::fromTheme("go-down"),
		"Down", m_sitespanel);

	QPushButton *btnMirrorAtoms = new QPushButton("Mirror", m_sitespanel);
	QPushButton *btnShowStruct = new QPushButton("View...", m_sitespanel);

	btnMirrorAtoms->setToolTip("Flip the coordinates of the atom positions.");
	btnShowStruct->setToolTip("Show a 3D view of the atom positions and couplings.");

	m_comboSG = new QComboBox(m_sitespanel);
	QPushButton *btnSG = new QPushButton(
		QIcon::fromTheme("insert-object"),
		"Generate", m_sitespanel);
	btnSG->setToolTip("Create atom site positions from space group symmetry operators.");

	btnAdd->setFocusPolicy(Qt::StrongFocus);
	btnDel->setFocusPolicy(Qt::StrongFocus);
	btnUp->setFocusPolicy(Qt::StrongFocus);
	btnDown->setFocusPolicy(Qt::StrongFocus);
	m_comboSG->setFocusPolicy(Qt::StrongFocus);
	btnSG->setFocusPolicy(Qt::StrongFocus);

	btnAdd->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});
	btnDel->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});
	btnUp->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});
	btnDown->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});
	btnSG->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});

	// get space groups and symops
	auto spacegroups = get_sgs<t_mat_real>();
	m_SGops.reserve(spacegroups.size());
	for(auto [sgnum, descr, ops] : spacegroups)
	{
		m_comboSG->addItem(descr.c_str(), m_comboSG->count());
		m_SGops.emplace_back(std::move(ops));
	}


	auto grid = new QGridLayout(m_sitespanel);
	grid->setSpacing(4);
	grid->setContentsMargins(6, 6, 6, 6);

	auto sep = new QFrame(m_samplepanel); sep->setFrameStyle(QFrame::HLine);

	int y = 0;
	grid->addWidget(m_sitestab, y,0,1,4);
	grid->addWidget(btnAdd, ++y,0,1,1);
	grid->addWidget(btnDel, y,1,1,1);
	grid->addWidget(btnUp, y,2,1,1);
	grid->addWidget(btnDown, y++,3,1,1);
	grid->addWidget(btnMirrorAtoms, y,0,1,1);
	grid->addWidget(btnShowStruct, y++,3,1,1);

	grid->addItem(new QSpacerItem(8, 8,
		QSizePolicy::Minimum, QSizePolicy::Fixed),
		y++,0, 1,1);
	grid->addWidget(sep, y++,0, 1,4);
	grid->addItem(new QSpacerItem(8, 8,
		QSizePolicy::Minimum, QSizePolicy::Fixed),
		y++,0, 1,1);

	grid->addWidget(new QLabel("Generate Atom Sites From Space Group:"), y++,0,1,4);
	grid->addWidget(m_comboSG, y,0,1,3);
	grid->addWidget(btnSG, y,3,1,1);


	// table CustomContextMenu
	QMenu *menuTableContext = new QMenu(m_sitestab);
	menuTableContext->addAction(
		QIcon::fromTheme("list-add"),
		"Add Atom Before", this,
		[this]() { this->AddSiteTabItem(-2); });
	menuTableContext->addAction(
		QIcon::fromTheme("list-add"),
		"Add Atom After", this,
		[this]() { this->AddSiteTabItem(-3); });
	menuTableContext->addAction(
		QIcon::fromTheme("edit-copy"),
		"Clone Atom", this,
		[this]() { this->AddSiteTabItem(-4); });
	menuTableContext->addAction(
		QIcon::fromTheme("list-remove"),
		"Delete Atom",this,
		[this]() { this->DelTabItem(m_sitestab); });


	// table CustomContextMenu in case nothing is selected
	QMenu *menuTableContextNoItem = new QMenu(m_sitestab);
	menuTableContextNoItem->addAction(
		QIcon::fromTheme("list-add"),
		"Add Atom", this,
		[this]() { this->AddSiteTabItem(); });
	menuTableContextNoItem->addAction(
		QIcon::fromTheme("list-remove"),
		"Delete Atom", this,
		[this]() { this->DelTabItem(m_sitestab); });


	// signals
	connect(btnAdd, &QAbstractButton::clicked,
		[this]() { this->AddSiteTabItem(-1); });
	connect(btnDel, &QAbstractButton::clicked,
		[this]() { this->DelTabItem(m_sitestab); });
	connect(btnUp, &QAbstractButton::clicked,
		[this]() { this->MoveTabItemUp(m_sitestab); });
	connect(btnDown, &QAbstractButton::clicked,
		[this]() { this->MoveTabItemDown(m_sitestab); });
	connect(btnSG, &QAbstractButton::clicked,
		this, &MagDynDlg::GenerateSitesFromSG);

	connect(btnMirrorAtoms, &QAbstractButton::clicked, this, &MagDynDlg::MirrorAtoms);
	connect(btnShowStruct, &QAbstractButton::clicked, this, &MagDynDlg::ShowStructurePlot);

	connect(m_comboSG, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		[this](int idx)
	{
		// synchronise with other sg combobox
		if(m_comboSG2)
		{
			m_comboSG2->blockSignals(true);
			m_comboSG2->setCurrentIndex(idx);
			m_comboSG2->blockSignals(false);
		}
	});

	connect(m_sitestab, &QTableWidget::itemSelectionChanged, [this]()
	{
		QList<QTableWidgetItem*> selected = m_sitestab->selectedItems();
		if(selected.size())
		{
			const QTableWidgetItem* item = *selected.begin();
			m_sites_cursor_row = item->row();
		}
	});
	connect(m_sitestab, &QTableWidget::itemChanged,
		this, &MagDynDlg::SitesTableItemChanged);
	connect(m_sitestab, &QTableWidget::customContextMenuRequested,
		[this, menuTableContext, menuTableContextNoItem](const QPoint& pt)
	{
		this->ShowTableContextMenu(
			m_sitestab, menuTableContext, menuTableContextNoItem, pt);
	});

	m_tabs_in->addTab(m_sitespanel, "Atoms");
}



void MagDynDlg::CreateExchangeTermsPanel()
{
	m_termspanel = new QWidget(this);

	m_termstab = new QTableWidget(m_termspanel);
	m_termstab->setShowGrid(true);
	m_termstab->setSortingEnabled(true);
	m_termstab->setMouseTracking(true);
	m_termstab->setSelectionBehavior(QTableWidget::SelectRows);
	m_termstab->setSelectionMode(QTableWidget::ContiguousSelection);
	m_termstab->setContextMenuPolicy(Qt::CustomContextMenu);

	m_termstab->verticalHeader()->setDefaultSectionSize(
		fontMetrics().lineSpacing() + 4);
	m_termstab->verticalHeader()->setVisible(true);

	m_termstab->setColumnCount(NUM_XCH_COLS);
	m_termstab->setHorizontalHeaderItem(
		COL_XCH_NAME, new QTableWidgetItem{"Name"});
	m_termstab->setHorizontalHeaderItem(
		COL_XCH_ATOM1_IDX, new QTableWidgetItem{"Atom 1"});
	m_termstab->setHorizontalHeaderItem(
		COL_XCH_ATOM2_IDX, new QTableWidgetItem{"Atom 2"});
	m_termstab->setHorizontalHeaderItem(
		COL_XCH_DIST_X, new QTableWidgetItem{"Cell Δx"});
	m_termstab->setHorizontalHeaderItem(
		COL_XCH_DIST_Y, new QTableWidgetItem{"Cell Δy"});
	m_termstab->setHorizontalHeaderItem(
		COL_XCH_DIST_Z, new QTableWidgetItem{"Cell Δz"});
	m_termstab->setHorizontalHeaderItem(
		COL_XCH_INTERACTION, new QTableWidgetItem{"Bond J"});
	m_termstab->setHorizontalHeaderItem(
		COL_XCH_DMI_X, new QTableWidgetItem{"DMI x"});
	m_termstab->setHorizontalHeaderItem(
		COL_XCH_DMI_Y, new QTableWidgetItem{"DMI y"});
	m_termstab->setHorizontalHeaderItem(
		COL_XCH_DMI_Z, new QTableWidgetItem{"DMI z"});

	m_termstab->setColumnWidth(COL_XCH_NAME, 90);
	m_termstab->setColumnWidth(COL_XCH_ATOM1_IDX, 80);
	m_termstab->setColumnWidth(COL_XCH_ATOM2_IDX, 80);
	m_termstab->setColumnWidth(COL_XCH_DIST_X, 80);
	m_termstab->setColumnWidth(COL_XCH_DIST_Y, 80);
	m_termstab->setColumnWidth(COL_XCH_DIST_Z, 80);
	m_termstab->setColumnWidth(COL_XCH_INTERACTION, 80);
	m_termstab->setColumnWidth(COL_XCH_DMI_X, 80);
	m_termstab->setColumnWidth(COL_XCH_DMI_Y, 80);
	m_termstab->setColumnWidth(COL_XCH_DMI_Z, 80);
	m_termstab->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Expanding});

	QPushButton *btnAdd = new QPushButton(
		QIcon::fromTheme("list-add"),
		"Add", m_termspanel);
	QPushButton *btnDel = new QPushButton(
		QIcon::fromTheme("list-remove"),
		"Delete", m_termspanel);
	QPushButton *btnUp = new QPushButton(
		QIcon::fromTheme("go-up"),
		"Up", m_termspanel);
	QPushButton *btnDown = new QPushButton(
		QIcon::fromTheme("go-down"),
		"Down", m_termspanel);

	btnAdd->setToolTip("Add an exchange term.");
	btnDel->setToolTip("Delete selected exchange term(s).");
	btnUp->setToolTip("Move selected exchange term(s) up.");
	btnDown->setToolTip("Move selected exchange term(s) down.");

	QPushButton *btnShowStruct = new QPushButton("View...", m_termspanel);
	btnShowStruct->setToolTip("Show a 3D view of the atom positions and couplings.");

	btnAdd->setFocusPolicy(Qt::StrongFocus);
	btnDel->setFocusPolicy(Qt::StrongFocus);
	btnUp->setFocusPolicy(Qt::StrongFocus);
	btnDown->setFocusPolicy(Qt::StrongFocus);

	btnAdd->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});
	btnDel->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});
	btnUp->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});
	btnDown->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});

	m_comboSG2 = new QComboBox(m_termspanel);
	QPushButton *btnSG = new QPushButton(
		QIcon::fromTheme("insert-object"),
		"Generate", m_sitespanel);
	btnSG->setToolTip("Create couplings from space group symmetry operators.");

	m_comboSG2->setFocusPolicy(Qt::StrongFocus);
	btnSG->setFocusPolicy(Qt::StrongFocus);
	btnSG->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});

	// copy space groups from other combobox
	for(int item_idx=0; item_idx<m_comboSG->count(); ++item_idx)
		m_comboSG2->addItem(m_comboSG->itemText(item_idx), m_comboSG2->count());

	// ordering vector
	m_ordering[0] = new QDoubleSpinBox(m_termspanel);
	m_ordering[1] = new QDoubleSpinBox(m_termspanel);
	m_ordering[2] = new QDoubleSpinBox(m_termspanel);

	// normal axis
	m_normaxis[0] = new QDoubleSpinBox(m_termspanel);
	m_normaxis[1] = new QDoubleSpinBox(m_termspanel);
	m_normaxis[2] = new QDoubleSpinBox(m_termspanel);

	for(int i=0; i<3; ++i)
	{
		m_ordering[i]->setDecimals(4);
		m_ordering[i]->setMinimum(-1);
		m_ordering[i]->setMaximum(+1);
		m_ordering[i]->setSingleStep(0.01);
		m_ordering[i]->setValue(0.);
		m_ordering[i]->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});

		m_normaxis[i]->setDecimals(4);
		m_normaxis[i]->setMinimum(-1);
		m_normaxis[i]->setMaximum(+1);
		m_normaxis[i]->setSingleStep(0.01);
		m_normaxis[i]->setValue(i==0 ? 1. : 0.);
		m_normaxis[i]->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});
	}

	m_ordering[0]->setPrefix("Oh = ");
	m_ordering[1]->setPrefix("Ok = ");
	m_ordering[2]->setPrefix("Ol = ");

	m_normaxis[0]->setPrefix("Nh = ");
	m_normaxis[1]->setPrefix("Nk = ");
	m_normaxis[2]->setPrefix("Nl = ");

	auto sep1 = new QFrame(m_samplepanel); sep1->setFrameStyle(QFrame::HLine);
	auto sep2 = new QFrame(m_samplepanel); sep2->setFrameStyle(QFrame::HLine);

	// grid
	auto grid = new QGridLayout(m_termspanel);
	grid->setSpacing(4);
	grid->setContentsMargins(6, 6, 6, 6);

	int y = 0;
	grid->addWidget(m_termstab, y++,0,1,4);
	grid->addWidget(btnAdd, y,0,1,1);
	grid->addWidget(btnDel, y,1,1,1);
	grid->addWidget(btnUp, y,2,1,1);
	grid->addWidget(btnDown, y++,3,1,1);
	grid->addWidget(btnShowStruct, y++,3,1,1);

	grid->addItem(new QSpacerItem(8, 8,
		QSizePolicy::Minimum, QSizePolicy::Fixed),
		y++,0, 1,1);
	grid->addWidget(sep1, y++,0, 1,4);
	grid->addItem(new QSpacerItem(8, 8,
		QSizePolicy::Minimum, QSizePolicy::Fixed),
		y++,0, 1,1);

	grid->addWidget(new QLabel("Generate Coupling Terms From Space Group:"), y++,0,1,4);
	grid->addWidget(m_comboSG2, y,0,1,3);
	grid->addWidget(btnSG, y++,3,1,1);

	grid->addItem(new QSpacerItem(8, 8,
		QSizePolicy::Minimum, QSizePolicy::Fixed),
		y++,0, 1,1);
	grid->addWidget(sep2, y++,0, 1,4);
	grid->addItem(new QSpacerItem(8, 8,
		QSizePolicy::Minimum, QSizePolicy::Fixed),
		y++,0, 1,1);

	grid->addWidget(new QLabel(QString("Ordering Vector:"),
		m_termspanel), y,0,1,1);
	grid->addWidget(m_ordering[0], y,1,1,1);
	grid->addWidget(m_ordering[1], y,2,1,1);
	grid->addWidget(m_ordering[2], y++,3,1,1);
	grid->addWidget(new QLabel(QString("Rotation Axis:"),
		m_termspanel), y,0,1,1);
	grid->addWidget(m_normaxis[0], y,1,1,1);
	grid->addWidget(m_normaxis[1], y,2,1,1);
	grid->addWidget(m_normaxis[2], y++,3,1,1);

	// table CustomContextMenu
	QMenu *menuTableContext = new QMenu(m_termstab);
	menuTableContext->addAction(
		QIcon::fromTheme("list-add"),
		"Add Term Before", this,
		[this]() { this->AddTermTabItem(-2); });
	menuTableContext->addAction(
		QIcon::fromTheme("list-add"),
		"Add Term After", this,
		[this]() { this->AddTermTabItem(-3); });
	menuTableContext->addAction(
		QIcon::fromTheme("edit-copy"),
		"Clone Term", this,
		[this]() { this->AddTermTabItem(-4); });
	menuTableContext->addAction(
		QIcon::fromTheme("list-remove"),
		"Delete Term", this,
		[this]() { this->DelTabItem(m_termstab); });


	// table CustomContextMenu in case nothing is selected
	QMenu *menuTableContextNoItem = new QMenu(m_termstab);
	menuTableContextNoItem->addAction(
		QIcon::fromTheme("list-add"),
		"Add Term", this,
		[this]() { this->AddTermTabItem(); });
	menuTableContextNoItem->addAction(
		QIcon::fromTheme("list-remove"),
		"Delete Term", this,
		[this]() { this->DelTabItem(m_termstab); });


	// signals
	connect(btnAdd, &QAbstractButton::clicked,
		[this]() { this->AddTermTabItem(-1); });
	connect(btnDel, &QAbstractButton::clicked,
		[this]() { this->DelTabItem(m_termstab); });
	connect(btnUp, &QAbstractButton::clicked,
		[this]() { this->MoveTabItemUp(m_termstab); });
	connect(btnDown, &QAbstractButton::clicked,
		[this]() { this->MoveTabItemDown(m_termstab); });
	connect(btnSG, &QAbstractButton::clicked,
		this, &MagDynDlg::GenerateCouplingsFromSG);

	connect(btnShowStruct, &QAbstractButton::clicked, this, &MagDynDlg::ShowStructurePlot);

	connect(m_comboSG2, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		[this](int idx)
	{
		// synchronise with other sg combobox
		if(m_comboSG)
		{
			m_comboSG->blockSignals(true);
			m_comboSG->setCurrentIndex(idx);
			m_comboSG->blockSignals(false);
		}
	});

	connect(m_termstab, &QTableWidget::itemSelectionChanged, [this]()
	{
		QList<QTableWidgetItem*> selected = m_termstab->selectedItems();
		if(selected.size())
		{
			const QTableWidgetItem* item = *selected.begin();
			m_terms_cursor_row = item->row();
		}
	});
	connect(m_termstab, &QTableWidget::itemChanged,
		this, &MagDynDlg::TermsTableItemChanged);
	connect(m_termstab, &QTableWidget::customContextMenuRequested,
		[this, menuTableContext, menuTableContextNoItem](const QPoint& pt)
		{ this->ShowTableContextMenu(m_termstab, menuTableContext, menuTableContextNoItem, pt); });

	auto calc_all = [this]()
	{
		if(this->m_autocalc->isChecked())
			this->CalcAll();
	};

	for(int i=0; i<3; ++i)
	{
		connect(m_ordering[i],
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), calc_all);

		connect(m_normaxis[i],
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), calc_all);
	}

	m_tabs_in->addTab(m_termspanel, "Couplings");
}



void MagDynDlg::CreateVariablesPanel()
{
	m_varspanel = new QWidget(this);

	m_varstab = new QTableWidget(m_varspanel);
	m_varstab->setShowGrid(true);
	m_varstab->setSortingEnabled(true);
	m_varstab->setMouseTracking(true);
	m_varstab->setSelectionBehavior(QTableWidget::SelectRows);
	m_varstab->setSelectionMode(QTableWidget::ContiguousSelection);
	m_varstab->setContextMenuPolicy(Qt::CustomContextMenu);

	m_varstab->verticalHeader()->setDefaultSectionSize(
		fontMetrics().lineSpacing() + 4);
	m_varstab->verticalHeader()->setVisible(false);

	m_varstab->setColumnCount(NUM_VARS_COLS);
	m_varstab->setHorizontalHeaderItem(
		COL_VARS_NAME, new QTableWidgetItem{"Name"});
	m_varstab->setHorizontalHeaderItem(
		COL_VARS_VALUE_REAL, new QTableWidgetItem{"Value (Re)"});
	m_varstab->setHorizontalHeaderItem(
		COL_VARS_VALUE_IMAG, new QTableWidgetItem{"Value (Im)"});

	m_varstab->setColumnWidth(COL_VARS_NAME, 150);
	m_varstab->setColumnWidth(COL_VARS_VALUE_REAL, 150);
	m_varstab->setColumnWidth(COL_VARS_VALUE_IMAG, 150);
	m_varstab->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Expanding});

	QPushButton *btnAdd = new QPushButton(
		QIcon::fromTheme("list-add"),
		"Add", m_varspanel);
	QPushButton *btnDel = new QPushButton(
		QIcon::fromTheme("list-remove"),
		"Delete", m_varspanel);
	QPushButton *btnUp = new QPushButton(
		QIcon::fromTheme("go-up"),
		"Up", m_varspanel);
	QPushButton *btnDown = new QPushButton(
		QIcon::fromTheme("go-down"),
		"Down", m_varspanel);

	btnAdd->setToolTip("Add a variable.");
	btnDel->setToolTip("Delete selected variables(s).");
	btnUp->setToolTip("Move selected variable(s) up.");
	btnDown->setToolTip("Move selected variable(s) down.");

	btnAdd->setFocusPolicy(Qt::StrongFocus);
	btnDel->setFocusPolicy(Qt::StrongFocus);
	btnUp->setFocusPolicy(Qt::StrongFocus);
	btnDown->setFocusPolicy(Qt::StrongFocus);

	btnAdd->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});
	btnDel->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});
	btnUp->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});
	btnDown->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});


	// grid
	auto grid = new QGridLayout(m_varspanel);
	grid->setSpacing(4);
	grid->setContentsMargins(6, 6, 6, 6);

	int y = 0;
	grid->addWidget(m_varstab, y++,0,1,4);
	grid->addWidget(btnAdd, y,0,1,1);
	grid->addWidget(btnDel, y,1,1,1);
	grid->addWidget(btnUp, y,2,1,1);
	grid->addWidget(btnDown, y++,3,1,1);


	// table CustomContextMenu
	QMenu *menuTableContext = new QMenu(m_varstab);
	menuTableContext->addAction(
		QIcon::fromTheme("list-add"),
		"Add Variable Before", this,
		[this]() { this->AddVariableTabItem(-2); });
	menuTableContext->addAction(
		QIcon::fromTheme("list-add"),
		"Add Variable After", this,
		[this]() { this->AddVariableTabItem(-3); });
	menuTableContext->addAction(
		QIcon::fromTheme("edit-copy"),
		"Clone Variable", this,
		[this]() { this->AddVariableTabItem(-4); });
	menuTableContext->addAction(
		QIcon::fromTheme("list-remove"),
		"Delete Variable", this,
		[this]() { this->DelTabItem(m_varstab); });


	// table CustomContextMenu in case nothing is selected
	QMenu *menuTableContextNoItem = new QMenu(m_varstab);
	menuTableContextNoItem->addAction(
		QIcon::fromTheme("list-add"),
		"Add Variable", this,
		[this]() { this->AddVariableTabItem(); });
	menuTableContextNoItem->addAction(
		QIcon::fromTheme("list-remove"),
		"Delete Variable", this,
		[this]() { this->DelTabItem(m_varstab); });


	// signals
	connect(btnAdd, &QAbstractButton::clicked,
		[this]() { this->AddVariableTabItem(-1); });
	connect(btnDel, &QAbstractButton::clicked,
		[this]() { this->DelTabItem(m_varstab); });
	connect(btnUp, &QAbstractButton::clicked,
		[this]() { this->MoveTabItemUp(m_varstab); });
	connect(btnDown, &QAbstractButton::clicked,
		[this]() { this->MoveTabItemDown(m_varstab); });

	connect(m_varstab, &QTableWidget::itemSelectionChanged, [this]()
	{
		QList<QTableWidgetItem*> selected = m_varstab->selectedItems();
		if(selected.size())
		{
			const QTableWidgetItem* item = *selected.begin();
			m_variables_cursor_row = item->row();
		}
	});
	connect(m_varstab, &QTableWidget::itemChanged,
		this, &MagDynDlg::VariablesTableItemChanged);
	connect(m_varstab, &QTableWidget::customContextMenuRequested,
		[this, menuTableContext, menuTableContextNoItem](const QPoint& pt)
		{ this->ShowTableContextMenu(m_varstab, menuTableContext, menuTableContextNoItem, pt); });


	m_tabs_in->addTab(m_varspanel, "Variables");
}



void MagDynDlg::CreateSampleEnvPanel()
{
	m_samplepanel = new QWidget(this);

	// field magnitude
	m_field_mag = new QDoubleSpinBox(m_samplepanel);
	m_field_mag->setDecimals(3);
	m_field_mag->setMinimum(0);
	m_field_mag->setMaximum(+99);
	m_field_mag->setSingleStep(0.1);
	m_field_mag->setValue(0.);
	m_field_mag->setPrefix("|B| = ");
	m_field_mag->setSuffix(" T");
	m_field_mag->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});

	// field direction
	m_field_dir[0] = new QDoubleSpinBox(m_samplepanel);
	m_field_dir[1] = new QDoubleSpinBox(m_samplepanel);
	m_field_dir[2] = new QDoubleSpinBox(m_samplepanel);

	// align spins along field (field-polarised state)
	m_align_spins = new QCheckBox(
		"Align Spins Along Field Direction", m_samplepanel);
	m_align_spins->setChecked(false);
	m_align_spins->setFocusPolicy(Qt::StrongFocus);

	// rotation axis
	m_rot_axis[0] = new QDoubleSpinBox(m_samplepanel);
	m_rot_axis[1] = new QDoubleSpinBox(m_samplepanel);
	m_rot_axis[2] = new QDoubleSpinBox(m_samplepanel);

	// rotation angle
	m_rot_angle = new QDoubleSpinBox(m_samplepanel);
	m_rot_angle->setDecimals(3);
	m_rot_angle->setMinimum(-360);
	m_rot_angle->setMaximum(+360);
	m_rot_angle->setSingleStep(0.1);
	m_rot_angle->setValue(90.);
	m_rot_angle->setSuffix("°");
	m_rot_angle->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});

	QPushButton *btn_rotate_ccw = new QPushButton(
		QIcon::fromTheme("object-rotate-left"),
		"Rotate CCW", m_samplepanel);
	QPushButton *btn_rotate_cw = new QPushButton(
		QIcon::fromTheme("object-rotate-right"),
		"Rotate CW", m_samplepanel);
	btn_rotate_ccw->setToolTip("Rotate the magnetic field in the counter-clockwise direction.");
	btn_rotate_cw->setToolTip("Rotate the magnetic field in the clockwise direction.");
	btn_rotate_ccw->setFocusPolicy(Qt::StrongFocus);
	btn_rotate_cw->setFocusPolicy(Qt::StrongFocus);


	// table with saved fields
	m_fieldstab = new QTableWidget(m_samplepanel);
	m_fieldstab->setShowGrid(true);
	m_fieldstab->setSortingEnabled(true);
	m_fieldstab->setMouseTracking(true);
	m_fieldstab->setSelectionBehavior(QTableWidget::SelectRows);
	m_fieldstab->setSelectionMode(QTableWidget::ContiguousSelection);
	m_fieldstab->setContextMenuPolicy(Qt::CustomContextMenu);

	m_fieldstab->verticalHeader()->setDefaultSectionSize(fontMetrics().lineSpacing() + 4);
	m_fieldstab->verticalHeader()->setVisible(true);

	m_fieldstab->setColumnCount(NUM_FIELD_COLS);
	m_fieldstab->setHorizontalHeaderItem(COL_FIELD_H,
		new QTableWidgetItem{"Bh"});
	m_fieldstab->setHorizontalHeaderItem(COL_FIELD_K,
		new QTableWidgetItem{"Bk"});
	m_fieldstab->setHorizontalHeaderItem(COL_FIELD_L,
		new QTableWidgetItem{"Bl"});
	m_fieldstab->setHorizontalHeaderItem(COL_FIELD_MAG,
		new QTableWidgetItem{"|B|"});

	m_fieldstab->setColumnWidth(COL_FIELD_H, 150);
	m_fieldstab->setColumnWidth(COL_FIELD_K, 150);
	m_fieldstab->setColumnWidth(COL_FIELD_L, 150);
	m_fieldstab->setColumnWidth(COL_FIELD_MAG, 150);
	m_fieldstab->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Expanding});

	QPushButton *btnAddField = new QPushButton(
		QIcon::fromTheme("list-add"),
		"Add", m_samplepanel);
	QPushButton *btnDelField = new QPushButton(
		QIcon::fromTheme("list-remove"),
		"Delete", m_samplepanel);
	QPushButton *btnFieldUp = new QPushButton(
		QIcon::fromTheme("go-up"),
		"Up", m_samplepanel);
	QPushButton *btnFieldDown = new QPushButton(
		QIcon::fromTheme("go-down"),
		"Down", m_samplepanel);

	btnAddField->setToolTip("Add a magnetic field.");
	btnDelField->setToolTip("Delete selected magnetic field(s).");
	btnFieldUp->setToolTip("Move selected magnetic field(s) up.");
	btnFieldDown->setToolTip("Move selected magnetic field(s) down.");

	QPushButton *btnSetField = new QPushButton("Set Field", m_samplepanel);
	btnSetField->setToolTip("Set the selected field as the currently active one.");

	btnAddField->setFocusPolicy(Qt::StrongFocus);
	btnDelField->setFocusPolicy(Qt::StrongFocus);
	btnFieldUp->setFocusPolicy(Qt::StrongFocus);
	btnFieldDown->setFocusPolicy(Qt::StrongFocus);

	btnAddField->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});
	btnDelField->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});
	btnFieldUp->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});
	btnFieldDown->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});


	// table CustomContextMenu
	QMenu *menuTableContext = new QMenu(m_fieldstab);
	menuTableContext->addAction(
		QIcon::fromTheme("list-add"),
		"Add Field Before", this,
		[this]() { this->AddFieldTabItem(-2); });
	menuTableContext->addAction(
		QIcon::fromTheme("list-add"),
		"Add Field After", this,
		[this]() { this->AddFieldTabItem(-3); });
	menuTableContext->addAction(
		QIcon::fromTheme("edit-copy"),
		"Clone Field", this,
		[this]() { this->AddFieldTabItem(-4); });
	menuTableContext->addAction(
		QIcon::fromTheme("list-remove"),
		"Delete Field", this,
		[this]() { this->DelTabItem(m_fieldstab); });
	menuTableContext->addSeparator();
	menuTableContext->addAction(
		QIcon::fromTheme("go-home"),
		"Set As Current Field", this,
		[this]() { this->SetCurrentField(); });


	// table CustomContextMenu in case nothing is selected
	QMenu *menuTableContextNoItem = new QMenu(m_fieldstab);
	menuTableContextNoItem->addAction(
		QIcon::fromTheme("list-add"),
		"Add Field", this,
		[this]() { this->AddFieldTabItem(-1,
			m_field_dir[0]->value(),
			m_field_dir[1]->value(),
			m_field_dir[2]->value(),
			m_field_mag->value()); });
	menuTableContextNoItem->addAction(
		QIcon::fromTheme("list-remove"),
		"Delete Field", this,
		[this]() { this->DelTabItem(m_fieldstab); });


	// temperature
	m_temperature = new QDoubleSpinBox(m_samplepanel);
	m_temperature->setDecimals(2);
	m_temperature->setMinimum(0);
	m_temperature->setMaximum(+999);
	m_temperature->setSingleStep(0.1);
	m_temperature->setValue(300.);
	m_temperature->setPrefix("T = ");
	m_temperature->setSuffix(" K");
	m_temperature->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});

	for(int i=0; i<3; ++i)
	{
		m_field_dir[i]->setDecimals(4);
		m_field_dir[i]->setMinimum(-99);
		m_field_dir[i]->setMaximum(+99);
		m_field_dir[i]->setSingleStep(0.1);
		m_field_dir[i]->setValue(i == 2 ? 1. : 0.);
		m_field_dir[i]->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});

		m_rot_axis[i]->setDecimals(4);
		m_rot_axis[i]->setMinimum(-99);
		m_rot_axis[i]->setMaximum(+99);
		m_rot_axis[i]->setSingleStep(0.1);
		m_rot_axis[i]->setValue(i == 2 ? 1. : 0.);
		m_rot_axis[i]->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});
	}

	m_field_dir[0]->setPrefix("Bh = ");
	m_field_dir[1]->setPrefix("Bk = ");
	m_field_dir[2]->setPrefix("Bl = ");

	auto grid = new QGridLayout(m_samplepanel);
	grid->setSpacing(4);
	grid->setContentsMargins(6, 6, 6, 6);

	int y = 0;
	grid->addWidget(new QLabel(QString("Magnetic Field:"),
		m_samplepanel), y++,0,1,2);
	grid->addWidget(new QLabel(QString("Magnitude:"),
		m_samplepanel), y,0,1,1);
	grid->addWidget(m_field_mag, y++,1,1,1);
	grid->addWidget(new QLabel(QString("Direction:"),
		m_samplepanel), y,0,1,1);
	grid->addWidget(m_field_dir[0], y,1,1,1);
	grid->addWidget(m_field_dir[1], y,2,1,1);
	grid->addWidget(m_field_dir[2], y++,3,1,1);
	grid->addWidget(m_align_spins, y++,0,1,4);

	auto sep1 = new QFrame(m_samplepanel); sep1->setFrameStyle(QFrame::HLine);
	auto sep2 = new QFrame(m_samplepanel); sep2->setFrameStyle(QFrame::HLine);
	auto sep3 = new QFrame(m_samplepanel); sep3->setFrameStyle(QFrame::HLine);

	grid->addItem(new QSpacerItem(8, 8,
		QSizePolicy::Minimum, QSizePolicy::Fixed),
		y++,0, 1,1);
	grid->addWidget(sep1, y++,0, 1,4);
	grid->addItem(new QSpacerItem(8, 8,
		QSizePolicy::Minimum, QSizePolicy::Fixed),
		y++,0, 1,1);

	grid->addWidget(new QLabel(QString("Rotate Magnetic Field:"),
		m_samplepanel), y++,0,1,2);
	grid->addWidget(new QLabel(QString("Axis:"),
		m_samplepanel), y,0,1,1);
	grid->addWidget(m_rot_axis[0], y,1,1,1);
	grid->addWidget(m_rot_axis[1], y,2,1,1);
	grid->addWidget(m_rot_axis[2], y++,3,1,1);
	grid->addWidget(new QLabel(QString("Angle:"),
		m_samplepanel), y,0,1,1);
	grid->addWidget(m_rot_angle, y,1,1,1);
	grid->addWidget(btn_rotate_ccw, y,2,1,1);
	grid->addWidget(btn_rotate_cw, y++,3,1,1);

	grid->addItem(new QSpacerItem(8, 8,
		QSizePolicy::Minimum, QSizePolicy::Fixed),
		y++,0, 1,1);
	grid->addWidget(sep2, y++,0, 1,4);
	grid->addItem(new QSpacerItem(8, 8,
		QSizePolicy::Minimum, QSizePolicy::Fixed),
		y++,0, 1,1);

	grid->addWidget(new QLabel(QString("Saved Fields:"),
		m_samplepanel), y++,0,1,4);
	grid->addWidget(m_fieldstab, y,0,1,4);
	grid->addWidget(btnAddField, ++y,0,1,1);
	grid->addWidget(btnDelField, y,1,1,1);
	grid->addWidget(btnFieldUp, y,2,1,1);
	grid->addWidget(btnFieldDown, y++,3,1,1);
	grid->addWidget(btnSetField, y++,3,1,1);

	grid->addItem(new QSpacerItem(8, 8,
		QSizePolicy::Minimum, QSizePolicy::Fixed),
		y++,0, 1,1);
	grid->addWidget(sep3, y++,0, 1,4);
	grid->addItem(new QSpacerItem(8, 8,
		QSizePolicy::Minimum, QSizePolicy::Fixed),
		y++,0, 1,1);

	grid->addWidget(new QLabel(QString("Temperature:"),
		m_samplepanel), y,0,1,1);
	grid->addWidget(m_temperature, y++,1,1,1);

	auto calc_all = [this]()
	{
		if(this->m_autocalc->isChecked())
			this->CalcAll();
	};

	// signals
	connect(m_field_mag,
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), calc_all);

	for(int i=0; i<3; ++i)
	{
		connect(m_field_dir[i],
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), calc_all);
	}

	connect(m_temperature,
		static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), calc_all);

	connect(m_align_spins, &QCheckBox::toggled, calc_all);

	connect(btn_rotate_ccw, &QAbstractButton::clicked, [this]()
	{
		RotateField(true);
	});

	connect(btn_rotate_cw, &QAbstractButton::clicked, [this]()
	{
		RotateField(false);
	});

	connect(btnAddField, &QAbstractButton::clicked,
		[this]() { this->AddFieldTabItem(-1,
			m_field_dir[0]->value(),
			m_field_dir[1]->value(),
			m_field_dir[2]->value(),
			m_field_mag->value()); });
	connect(btnDelField, &QAbstractButton::clicked,
		[this]() { this->DelTabItem(m_fieldstab); });
	connect(btnFieldUp, &QAbstractButton::clicked,
		[this]() { this->MoveTabItemUp(m_fieldstab); });
	connect(btnFieldDown, &QAbstractButton::clicked,
		[this]() { this->MoveTabItemDown(m_fieldstab); });

	connect(btnSetField, &QAbstractButton::clicked,
		[this]() { this->SetCurrentField(); });

	connect(m_fieldstab, &QTableWidget::itemSelectionChanged, [this]()
	{
		QList<QTableWidgetItem*> selected = m_fieldstab->selectedItems();
		if(selected.size())
		{
			const QTableWidgetItem* item = *selected.begin();
			m_fields_cursor_row = item->row();
		}
	});
	connect(m_fieldstab, &QTableWidget::customContextMenuRequested,
		[this, menuTableContext, menuTableContextNoItem](const QPoint& pt)
	{
		this->ShowTableContextMenu(
			m_fieldstab, menuTableContext, menuTableContextNoItem, pt);
	});

	m_tabs_in->addTab(m_samplepanel, "Sample");
}



void MagDynDlg::CreateDispersionPanel()
{
	const char* hklPrefix[] = { "h = ", "k = ","l = ", };
	m_disppanel = new QWidget(this);

	// plotter
	m_plot = new QCustomPlot(m_disppanel);
	m_plot->xAxis->setLabel("Q (rlu)");
	m_plot->yAxis->setLabel("E (meV)");
	m_plot->setInteraction(QCP::iRangeDrag, true);
	m_plot->setInteraction(QCP::iRangeZoom, true);
	m_plot->setSelectionRectMode(QCP::srmZoom);
	m_plot->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Expanding});

	// start and stop coordinates
	m_q_start[0] = new QDoubleSpinBox(m_disppanel);
	m_q_start[1] = new QDoubleSpinBox(m_disppanel);
	m_q_start[2] = new QDoubleSpinBox(m_disppanel);
	m_q_end[0] = new QDoubleSpinBox(m_disppanel);
	m_q_end[1] = new QDoubleSpinBox(m_disppanel);
	m_q_end[2] = new QDoubleSpinBox(m_disppanel);

	// number of points in plot
	m_num_points = new QSpinBox(m_disppanel);
	m_num_points->setMinimum(1);
	m_num_points->setMaximum(9999);
	m_num_points->setValue(512);
	m_num_points->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Fixed});

	// scaling factor for weights
	for(auto** comp : {&m_weight_scale, &m_weight_min, &m_weight_max})
	{
		*comp = new QDoubleSpinBox(m_disppanel);
		(*comp)->setDecimals(4);
		(*comp)->setMinimum(0.);
		(*comp)->setMaximum(+9999.);
		(*comp)->setSingleStep(0.1);
		(*comp)->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});
	}

	m_weight_scale->setValue(1.);
	m_weight_min->setValue(0.);
	m_weight_max->setValue(9999);
	m_weight_min->setMinimum(-1.);	// -1: disable clamping
	m_weight_max->setMinimum(-1.);	// -1: disable clamping

	for(int i=0; i<3; ++i)
	{
		m_q_start[i]->setDecimals(4);
		m_q_start[i]->setMinimum(-99);
		m_q_start[i]->setMaximum(+99);
		m_q_start[i]->setSingleStep(0.01);
		m_q_start[i]->setValue(0.);
		m_q_start[i]->setSuffix(" rlu");
		m_q_start[i]->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});
		m_q_start[i]->setPrefix(hklPrefix[i]);

		m_q_end[i]->setDecimals(4);
		m_q_end[i]->setMinimum(-99);
		m_q_end[i]->setMaximum(+99);
		m_q_end[i]->setSingleStep(0.01);
		m_q_end[i]->setValue(0.);
		m_q_end[i]->setSuffix(" rlu");
		m_q_end[i]->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});
		m_q_end[i]->setPrefix(hklPrefix[i]);
	}

	m_q_start[0]->setValue(-1.);
	m_q_end[0]->setValue(+1.);

	auto grid = new QGridLayout(m_disppanel);
	grid->setSpacing(4);
	grid->setContentsMargins(6, 6, 6, 6);

	int y = 0;
	grid->addWidget(m_plot, y++,0,1,4);
	grid->addWidget(
		new QLabel(QString("Start Q:"), m_disppanel), y,0,1,1);
	grid->addWidget(m_q_start[0], y,1,1,1);
	grid->addWidget(m_q_start[1], y,2,1,1);
	grid->addWidget(m_q_start[2], y++,3,1,1);
	grid->addWidget(
		new QLabel(QString("End Q:"), m_disppanel), y,0,1,1);
	grid->addWidget(m_q_end[0], y,1,1,1);
	grid->addWidget(m_q_end[1], y,2,1,1);
	grid->addWidget(m_q_end[2], y++,3,1,1);
	grid->addWidget(
		new QLabel(QString("Q Count:"), m_disppanel), y,0,1,1);
	grid->addWidget(m_num_points, y,1,1,1);
	grid->addWidget(
		new QLabel(QString("Weight Scale:"), m_disppanel), y,2,1,1);
	grid->addWidget(m_weight_scale, y++,3,1,1);
	grid->addWidget(
		new QLabel(QString("Min. Weight:"), m_disppanel), y,0,1,1);
	grid->addWidget(m_weight_min, y,1,1,1);
	grid->addWidget(
		new QLabel(QString("Max. Weight:"), m_disppanel), y,2,1,1);
	grid->addWidget(m_weight_max, y++,3,1,1);

	// signals
	for(int i=0; i<3; ++i)
	{
		connect(m_q_start[i],
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			[this]()
		{
			if(this->m_autocalc->isChecked())
				this->CalcDispersion();
		});
		connect(m_q_end[i],
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			[this]()
		{
			if(this->m_autocalc->isChecked())
				this->CalcDispersion();
		});
	}

	connect(m_num_points,
		static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
		[this]()
	{
		if(this->m_autocalc->isChecked())
			this->CalcDispersion();
	});

	for(auto* comp : {m_weight_scale, m_weight_min, m_weight_max})
	{
		connect(comp,
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			[this]()
		{
			// update graph weights
			for(GraphWithWeights* graph : m_graphs)
				graph->SetWeightScale(m_weight_scale->value(), m_weight_min->value(), m_weight_max->value());
			if(m_plot)
				m_plot->replot();
		});
	}

	connect(m_plot, &QCustomPlot::mouseMove, this, &MagDynDlg::PlotMouseMove);
	connect(m_plot, &QCustomPlot::mousePress, this, &MagDynDlg::PlotMousePress);

	m_tabs_out->addTab(m_disppanel, "Dispersion");
}



void MagDynDlg::CreateHamiltonPanel()
{
	const char* hklPrefix[] = { "h = ", "k = ","l = ", };
	m_hamiltonianpanel = new QWidget(this);

	// hamiltonian
	m_hamiltonian = new QTextEdit(m_hamiltonianpanel);
	m_hamiltonian->setReadOnly(true);
	m_hamiltonian->setWordWrapMode(QTextOption::NoWrap);
	m_hamiltonian->setLineWrapMode(QTextEdit::NoWrap);
	m_hamiltonian->setSizePolicy(QSizePolicy{
		QSizePolicy::Expanding, QSizePolicy::Expanding});

	// Q coordinates
	m_q[0] = new QDoubleSpinBox(m_hamiltonianpanel);
	m_q[1] = new QDoubleSpinBox(m_hamiltonianpanel);
	m_q[2] = new QDoubleSpinBox(m_hamiltonianpanel);

	for(int i=0; i<3; ++i)
	{
		m_q[i]->setDecimals(4);
		m_q[i]->setMinimum(-99);
		m_q[i]->setMaximum(+99);
		m_q[i]->setSingleStep(0.01);
		m_q[i]->setValue(0.);
		m_q[i]->setSuffix(" rlu");
		m_q[i]->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});
		m_q[i]->setPrefix(hklPrefix[i]);
	}

	auto grid = new QGridLayout(m_hamiltonianpanel);
	grid->setSpacing(4);
	grid->setContentsMargins(6, 6, 6, 6);

	int y = 0;
	grid->addWidget(m_hamiltonian, y++,0,1,4);
	grid->addWidget(new QLabel(QString("Q:"),
		m_hamiltonianpanel), y,0,1,1);
	grid->addWidget(m_q[0], y,1,1,1);
	grid->addWidget(m_q[1], y,2,1,1);
	grid->addWidget(m_q[2], y++,3,1,1);

	// signals
	for(int i=0; i<3; ++i)
	{
		connect(m_q[i],
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			[this]()
		{
			if(this->m_autocalc->isChecked())
				this->CalcHamiltonian();
		});
	}

	m_tabs_out->addTab(m_hamiltonianpanel, "Hamiltonian");
}



void MagDynDlg::CreateExportPanel()
{
	const char* hklPrefix[] = { "h = ", "k = ","l = ", };
	m_exportpanel = new QWidget(this);

	// Q coordinates
	m_exportStartQ[0] = new QDoubleSpinBox(m_exportpanel);
	m_exportStartQ[1] = new QDoubleSpinBox(m_exportpanel);
	m_exportStartQ[2] = new QDoubleSpinBox(m_exportpanel);
	m_exportEndQ[0] = new QDoubleSpinBox(m_exportpanel);
	m_exportEndQ[1] = new QDoubleSpinBox(m_exportpanel);
	m_exportEndQ[2] = new QDoubleSpinBox(m_exportpanel);

	// number of grid points
	for(int i=0; i<3; ++i)
	{
		m_exportNumPoints[i] = new QSpinBox(m_exportpanel);
		m_exportNumPoints[i]->setMinimum(1);
		m_exportNumPoints[i]->setMaximum(99999);
		m_exportNumPoints[i]->setValue(128);
		m_exportNumPoints[i]->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});
	}

	// export
	m_exportFormat = new QComboBox(m_exportpanel);
	m_exportFormat->addItem("Takin Grid File", EXPORT_GRID);
#ifdef USE_HDF5
	m_exportFormat->addItem("HDF5 File", EXPORT_HDF5);
#endif
	m_exportFormat->addItem("Text File", EXPORT_TEXT);

	QPushButton *btn_export = new QPushButton(
		QIcon::fromTheme("document-save-as"),
		"Export...", m_exportpanel);
	btn_export->setFocusPolicy(Qt::StrongFocus);

	for(int i=0; i<3; ++i)
	{
		m_exportStartQ[i]->setDecimals(4);
		m_exportStartQ[i]->setMinimum(-99);
		m_exportStartQ[i]->setMaximum(+99);
		m_exportStartQ[i]->setSingleStep(0.01);
		m_exportStartQ[i]->setValue(-1.);
		m_exportStartQ[i]->setSuffix(" rlu");
		m_exportStartQ[i]->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});
		m_exportStartQ[i]->setPrefix(hklPrefix[i]);

		m_exportEndQ[i]->setDecimals(4);
		m_exportEndQ[i]->setMinimum(-99);
		m_exportEndQ[i]->setMaximum(+99);
		m_exportEndQ[i]->setSingleStep(0.01);
		m_exportEndQ[i]->setValue(1.);
		m_exportEndQ[i]->setSuffix(" rlu");
		m_exportEndQ[i]->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});
		m_exportEndQ[i]->setPrefix(hklPrefix[i]);
	}


	auto grid = new QGridLayout(m_exportpanel);
	grid->setSpacing(4);
	grid->setContentsMargins(6, 6, 6, 6);

	int y = 0;
	grid->addWidget(new QLabel(QString("Export Ranges:"),
		m_exportpanel), y++,0,1,4);
	grid->addWidget(new QLabel(QString("Start Q:"),
		m_exportpanel), y,0,1,1);
	grid->addWidget(m_exportStartQ[0], y,1,1,1);
	grid->addWidget(m_exportStartQ[1], y,2,1,1);
	grid->addWidget(m_exportStartQ[2], y++,3,1,1);
	grid->addWidget(new QLabel(QString("End Q:"),
		m_exportpanel), y,0,1,1);
	grid->addWidget(m_exportEndQ[0], y,1,1,1);
	grid->addWidget(m_exportEndQ[1], y,2,1,1);
	grid->addWidget(m_exportEndQ[2], y++,3,1,1);

	grid->addItem(new QSpacerItem(8, 8,
		QSizePolicy::Minimum, QSizePolicy::Fixed),
		y++,0, 1,1);
	auto sep1 = new QFrame(m_samplepanel); sep1->setFrameStyle(QFrame::HLine);
	grid->addWidget(sep1, y++, 0,1,4);
	grid->addItem(new QSpacerItem(8, 8,
		QSizePolicy::Minimum, QSizePolicy::Fixed),
		y++,0, 1,1);

	grid->addWidget(new QLabel(QString("Number of Grid Points per Q Direction:"),
		m_exportpanel), y++,0,1,4);
	grid->addWidget(new QLabel(QString("Points:"),
		m_exportpanel), y,0,1,1);
	grid->addWidget(m_exportNumPoints[0], y,1,1,1);
	grid->addWidget(m_exportNumPoints[1], y,2,1,1);
	grid->addWidget(m_exportNumPoints[2], y++,3,1,1);

	grid->addItem(new QSpacerItem(8, 8,
		QSizePolicy::Minimum, QSizePolicy::Fixed),
		y++,0, 1,1);
	auto sep2 = new QFrame(m_samplepanel); sep1->setFrameStyle(QFrame::HLine);
	grid->addWidget(sep2, y++, 0,1,4);
	grid->addItem(new QSpacerItem(8, 8,
		QSizePolicy::Minimum, QSizePolicy::Fixed),
		y++,0, 1,1);

	grid->addItem(new QSpacerItem(16, 16,
		QSizePolicy::Minimum, QSizePolicy::Expanding),
		y++,0,1,4);

	grid->addWidget(new QLabel(QString("Export Format:"),
		m_exportpanel), y,0,1,1);
	grid->addWidget(m_exportFormat, y,1,1,1);
	grid->addWidget(btn_export, y++,3,1,1);

	// signals
	connect(btn_export, &QAbstractButton::clicked, this,
		static_cast<void (MagDynDlg::*)()>(&MagDynDlg::ExportSQE));

	m_tabs_out->addTab(m_exportpanel, "Export");
}



void MagDynDlg::CreateInfoDlg()
{
	auto infopanel = new QWidget(this);
	auto grid = new QGridLayout(infopanel);
	grid->setSpacing(4);
	grid->setContentsMargins(6, 6, 6, 6);

	auto sep1 = new QFrame(infopanel); sep1->setFrameStyle(QFrame::HLine);
	auto sep2 = new QFrame(infopanel); sep2->setFrameStyle(QFrame::HLine);
	auto sep3 = new QFrame(infopanel); sep3->setFrameStyle(QFrame::HLine);
	auto sep4 = new QFrame(infopanel); sep4->setFrameStyle(QFrame::HLine);

	std::string strBoost = BOOST_LIB_VERSION;
	algo::replace_all(strBoost, "_", ".");

	auto labelTitle = new QLabel("Takin / Magnon Dynamics Calculator", infopanel);
	auto fontTitle = labelTitle->font();
	fontTitle.setBold(true);
	labelTitle->setFont(fontTitle);
	labelTitle->setAlignment(Qt::AlignHCenter);

	auto labelAuthor = new QLabel("Written by Tobias Weber <tweber@ill.fr>.", infopanel);
	labelAuthor->setAlignment(Qt::AlignHCenter);

	auto labelDate = new QLabel("January 2022.", infopanel);
	labelDate->setAlignment(Qt::AlignHCenter);

	auto labelPaper = new QLabel(
		"This program implements the formalism given in "
		"<a href=\"https://doi.org/10.1088/0953-8984/27/16/166002\">this paper</a>.",
		infopanel);
	labelPaper->setOpenExternalLinks(true);

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

	grid->addItem(new QSpacerItem(16, 16,
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

	grid->addWidget(new QLabel(
		QString("Qt Version: ") +
		QString(QT_VERSION_STR) + ".",
		infopanel), y++,0, 1,1);
	grid->addWidget(new QLabel(
		QString("Boost Version: ") +
		strBoost.c_str() + ".",
		infopanel), y++,0, 1,1);

	grid->addWidget(sep3, y++,0, 1,1);
	grid->addWidget(labelPaper, y++,0, 1,1);
	grid->addWidget(sep4, y++,0, 1,1);

	for(int i=0; i<4; ++i)
		grid->addWidget(m_labelGlInfos[i], y++,0, 1,1);

	grid->addItem(new QSpacerItem(16, 16,
		QSizePolicy::Minimum, QSizePolicy::Expanding),
		y++,0, 1,1);

	// add info panel as a tab
	//m_tabs_out->addTab(infopanel, "Infos");

	// show info panel as a dialog
	m_info_dlg = new QDialog(this);
	m_info_dlg->setWindowTitle("About");
	m_info_dlg->setSizeGripEnabled(true);

	QPushButton *infoDlgOk = new QPushButton("OK", m_info_dlg);
	connect(infoDlgOk, &QAbstractButton::clicked,
		m_info_dlg, &QDialog::accept);

	auto dlgGrid = new QGridLayout(m_info_dlg);
	dlgGrid->setSpacing(8);
	dlgGrid->setContentsMargins(8, 8, 8, 8);
	dlgGrid->addWidget(infopanel, 0,0, 1,4);
	dlgGrid->addWidget(infoDlgOk, 1,3, 1,1);
}



void MagDynDlg::CreateMenuBar()
{
	m_menu = new QMenuBar(this);
	m_menu->setNativeMenuBar(m_sett ? m_sett->value("native_gui", false).toBool() : false);

	// file menu
	auto menuFile = new QMenu("File", m_menu);
	auto acNew = new QAction("New", menuFile);
	auto acLoad = new QAction("Open...", menuFile);
	auto acSave = new QAction("Save", menuFile);
	auto acSaveAs = new QAction("Save As...", menuFile);
	auto acExit = new QAction("Quit", menuFile);

	// structure menu
	auto menuStruct = new QMenu("Structure", m_menu);
	auto acStructImport = new QAction("Import From Table...", menuStruct);
	auto acStructView = new QAction("View...", menuStruct);

	// dispersion menu
	m_menuDisp = new QMenu("Dispersion", m_menu);
	auto acRescalePlot = new QAction("Rescale Axes", m_menuDisp);
	auto acSaveFigure = new QAction("Save Figure...", m_menuDisp);
	auto acSaveDisp = new QAction("Save Data...", m_menuDisp);

	// recent files menu
	m_menuOpenRecent = new QMenu("Open Recent", menuFile);

	m_recent.SetRecentFilesMenu(m_menuOpenRecent);
	m_recent.SetMaxRecentFiles(16);
	m_recent.SetOpenFunc(&m_open_func);

	// shortcuts
	acNew->setShortcut(QKeySequence::New);
	acLoad->setShortcut(QKeySequence::Open);
	acSave->setShortcut(QKeySequence::Save);
	acSaveAs->setShortcut(QKeySequence::SaveAs);
	acExit->setShortcut(QKeySequence::Quit);
	acExit->setMenuRole(QAction::QuitRole);

	// icons
	acNew->setIcon(QIcon::fromTheme("document-new"));
	acLoad->setIcon(QIcon::fromTheme("document-open"));
	acSave->setIcon(QIcon::fromTheme("document-save"));
	acSaveAs->setIcon(QIcon::fromTheme("document-save-as"));
	acExit->setIcon(QIcon::fromTheme("application-exit"));
	m_menuOpenRecent->setIcon(QIcon::fromTheme("document-open-recent"));
	acSaveFigure->setIcon(QIcon::fromTheme("image-x-generic"));
	acSaveDisp->setIcon(QIcon::fromTheme("text-x-generic"));

	// calculation menu
	auto menuCalc = new QMenu("Calculation", m_menu);
	m_autocalc = new QAction("Automatically Calculate", menuCalc);
	m_autocalc->setToolTip("Automatically calculate the results.");
	m_autocalc->setCheckable(true);
	m_autocalc->setChecked(false);
	QAction *acCalc = new QAction("Start Calculation", menuCalc);
	acCalc->setToolTip("Calculate all results.");
	//acCalc->setIcon(QIcon::fromTheme("accessories-calculator"));
	m_use_dmi = new QAction("Use DMI", menuCalc);
	m_use_dmi->setToolTip("Enables the Dzyaloshinskij-Moriya interaction.");
	m_use_dmi->setCheckable(true);
	m_use_dmi->setChecked(true);
	m_use_field = new QAction("Use External Field", menuCalc);
	m_use_field->setToolTip("Enables an external field.");
	m_use_field->setCheckable(true);
	m_use_field->setChecked(true);
	m_use_temperature = new QAction("Use Bose Factor", menuCalc);
	m_use_temperature->setToolTip("Enables the Bose factor.");
	m_use_temperature->setCheckable(true);
	m_use_temperature->setChecked(true);
	m_use_weights = new QAction("Use Spectral Weights", menuCalc);
	m_use_weights->setToolTip("Enables calculation of the spin correlation function.");
	m_use_weights->setCheckable(true);
	m_use_weights->setChecked(false);
	m_use_projector = new QAction("Use Neutron Weights", menuCalc);
	m_use_projector->setToolTip("Enables the neutron orthogonal projector.");
	m_use_projector->setCheckable(true);
	m_use_projector->setChecked(true);
	m_unite_degeneracies = new QAction("Unite Degenerate Energies", menuCalc);
	m_unite_degeneracies->setToolTip("Unites the weight factors corresponding to degenerate eigenenergies.");
	m_unite_degeneracies->setCheckable(true);
	m_unite_degeneracies->setChecked(true);
	m_ignore_annihilation = new QAction("Ignore Magnon Annihilation", menuCalc);
	m_ignore_annihilation->setToolTip("Calculate only magnon creation..");
	m_ignore_annihilation->setCheckable(true);
	m_ignore_annihilation->setChecked(false);

	// help menu
	auto menuHelp = new QMenu("Help", m_menu);
	QAction *acAboutQt = new QAction(
		QIcon::fromTheme("help-about"),
		"About Qt...", menuHelp);
	QAction *acAbout = new QAction(
		QIcon::fromTheme("help-about"),
		"About...", menuHelp);

	acAboutQt->setMenuRole(QAction::AboutQtRole);
	acAbout->setMenuRole(QAction::AboutRole);

	// actions
	menuFile->addAction(acNew);
	menuFile->addSeparator();
	menuFile->addAction(acLoad);
	menuFile->addMenu(m_menuOpenRecent);
	menuFile->addSeparator();
	menuFile->addAction(acSave);
	menuFile->addAction(acSaveAs);
	menuFile->addSeparator();
	menuFile->addAction(acExit);

	menuStruct->addAction(acStructImport);
	menuStruct->addSeparator();
	menuStruct->addAction(acStructView);

	m_menuDisp->addAction(acRescalePlot);
	m_menuDisp->addSeparator();
	m_menuDisp->addAction(acSaveFigure);
	m_menuDisp->addAction(acSaveDisp);

	menuCalc->addAction(m_autocalc);
	menuCalc->addAction(acCalc);
	menuCalc->addSeparator();
	menuCalc->addAction(m_use_dmi);
	menuCalc->addAction(m_use_field);
	menuCalc->addAction(m_use_temperature);
	menuCalc->addSeparator();
	menuCalc->addAction(m_use_weights);
	menuCalc->addAction(m_use_projector);
	menuCalc->addSeparator();
	menuCalc->addAction(m_unite_degeneracies);
	menuCalc->addAction(m_ignore_annihilation);

	menuHelp->addAction(acAboutQt);
	menuHelp->addAction(acAbout);

	// signals
	connect(acNew, &QAction::triggered, this, &MagDynDlg::Clear);
	connect(acLoad, &QAction::triggered,
		this, static_cast<void (MagDynDlg::*)()>(&MagDynDlg::Load));
	connect(acSave, &QAction::triggered,
			this, static_cast<void (MagDynDlg::*)()>(&MagDynDlg::Save));
	connect(acSaveAs, &QAction::triggered,
		this, static_cast<void (MagDynDlg::*)()>(&MagDynDlg::SaveAs));
	connect(acExit, &QAction::triggered, this, &QDialog::close);

	connect(acSaveFigure, &QAction::triggered,
		this, &MagDynDlg::SavePlotFigure);
	connect(acSaveDisp, &QAction::triggered,
		this, &MagDynDlg::SaveDispersion);

	connect(acRescalePlot, &QAction::triggered, [this]()
	{
		if(!m_plot)
			return;

		m_plot->rescaleAxes();
		m_plot->replot();
	});

	auto calc_all = [this]()
	{
		if(this->m_autocalc->isChecked())
			this->CalcAll();
	};

	auto calc_all_dyn = [this]()
	{
		if(this->m_autocalc->isChecked())
			this->CalcAllDynamics();
	};

	connect(acStructView, &QAction::triggered, this, &MagDynDlg::ShowStructurePlot);
	connect(acStructImport, &QAction::triggered, this, &MagDynDlg::ShowTableImporter);
	connect(m_use_dmi, &QAction::toggled, calc_all);
	connect(m_use_field, &QAction::toggled, calc_all);
	connect(m_use_temperature, &QAction::toggled, calc_all);
	connect(m_use_weights, &QAction::toggled, calc_all_dyn);
	connect(m_use_projector, &QAction::toggled, calc_all_dyn);
	connect(m_unite_degeneracies, &QAction::toggled, calc_all_dyn);
	connect(m_ignore_annihilation, &QAction::toggled, calc_all_dyn);
	connect(m_autocalc, &QAction::toggled, [this](bool checked)
	{
		if(checked)
			this->CalcAll();
	});

	connect(acCalc, &QAction::triggered, [this]()
	{
		this->CalcAll();
	});

	connect(acAboutQt, &QAction::triggered, []()
	{
		qApp->aboutQt();
	});
	connect(acAbout, &QAction::triggered, [this]()
	{
		if(!m_info_dlg)
			return;

		m_info_dlg->show();
		m_info_dlg->raise();
		m_info_dlg->activateWindow();
	});

	// menu bar
	m_menu->addMenu(menuFile);
	m_menu->addMenu(menuStruct);
	m_menu->addMenu(m_menuDisp);
	m_menu->addMenu(menuCalc);
	m_menu->addMenu(menuHelp);
	m_maingrid->setMenuBar(m_menu);
}
