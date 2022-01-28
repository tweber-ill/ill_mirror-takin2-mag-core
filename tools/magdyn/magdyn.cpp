/**
 * magnon dynamics
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

#include <QtGui/QFontDatabase>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/version.hpp>
#include <boost/config.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
namespace algo = boost::algorithm;

#include "tlibs2/libs/phys.h"
#include "tlibs2/libs/algos.h"

#include "graph.h"
#include "../structfact/loadcif.h"

using namespace tl2_ops;


t_real g_eps = 1e-6;
int g_prec = 6;
int g_prec_gui = 3;


/**
 * columns of sites table
 */
enum : int
{
	COL_SITE_NAME = 0,
	COL_SITE_POS_X, COL_SITE_POS_Y, COL_SITE_POS_Z,
	COL_SITE_SPIN_X, COL_SITE_SPIN_Y, COL_SITE_SPIN_Z,
	COL_SITE_SPIN_MAG,

	NUM_SITE_COLS
};


/**
 * columns of exchange terms table
 */
enum : int
{
	COL_XCH_NAME = 0,
	COL_XCH_ATOM1_IDX, COL_XCH_ATOM2_IDX,
	COL_XCH_DIST_X, COL_XCH_DIST_Y, COL_XCH_DIST_Z,
	COL_XCH_INTERACTION,
	COL_XCH_DMI_X, COL_XCH_DMI_Y, COL_XCH_DMI_Z,

	NUM_XCH_COLS
};



MagDynDlg::MagDynDlg(QWidget* pParent) : QDialog{pParent},
	m_sett{new QSettings{"takin", "magdyn"}}
{
	m_dyn.SetEpsilon(g_eps);
	m_dyn.SetPrecision(g_prec);

	setWindowTitle("Magnon Dynamics");
	setSizeGripEnabled(true);
	setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));

	m_tabs = new QTabWidget(this);

	// atom sites panel
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
		m_sitestab->verticalHeader()->setVisible(false);

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

		QPushButton *pTabBtnAdd = new QPushButton("Add Atom", m_sitespanel);
		QPushButton *pTabBtnDel = new QPushButton("Delete Atom", m_sitespanel);
		QPushButton *pTabBtnUp = new QPushButton("Move Atom Up", m_sitespanel);
		QPushButton *pTabBtnDown = new QPushButton("Move Atom Down", m_sitespanel);

		m_comboSG = new QComboBox(m_sitespanel);
		QPushButton *pTabBtnSG = new QPushButton("Generate", m_sitespanel);

		pTabBtnAdd->setFocusPolicy(Qt::StrongFocus);
		pTabBtnDel->setFocusPolicy(Qt::StrongFocus);
		pTabBtnUp->setFocusPolicy(Qt::StrongFocus);
		pTabBtnDown->setFocusPolicy(Qt::StrongFocus);
		m_comboSG->setFocusPolicy(Qt::StrongFocus);
		pTabBtnSG->setFocusPolicy(Qt::StrongFocus);

		pTabBtnAdd->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDel->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnUp->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDown->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnSG->setSizePolicy(QSizePolicy{
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

		int y = 0;
		grid->addWidget(m_sitestab, y,0,1,4);
		grid->addWidget(pTabBtnAdd, ++y,0,1,1);
		grid->addWidget(pTabBtnDel, y,1,1,1);
		grid->addWidget(pTabBtnUp, y,2,1,1);
		grid->addWidget(pTabBtnDown, y,3,1,1);
		grid->addWidget(new QLabel("Space Group:"), ++y,0,1,1);
		grid->addWidget(m_comboSG, y,1,1,2);
		grid->addWidget(pTabBtnSG, y,3,1,1);


		// table CustomContextMenu
		QMenu *pTabContextMenu = new QMenu(m_sitestab);
		pTabContextMenu->addAction("Add Atom Before", this, [this]()
		{
			this->AddSiteTabItem(-2);
		});
		pTabContextMenu->addAction("Add Atom After", this, [this]()
		{
			this->AddSiteTabItem(-3);
		});
		pTabContextMenu->addAction("Clone Atom", this, [this]()
		{
			this->AddSiteTabItem(-4);
		});
		pTabContextMenu->addAction("Delete Atom", this, [this]()
		{
			this->DelTabItem(m_sitestab);
		});


		// table CustomContextMenu in case nothing is selected
		QMenu *pTabContextMenuNoItem = new QMenu(m_sitestab);
		pTabContextMenuNoItem->addAction("Add Atom", this, [this]()
		{
			this->AddSiteTabItem();
		});
		pTabContextMenuNoItem->addAction("Delete Atom", this, [this]()
		{
			this->DelTabItem(m_sitestab);
		});
		//pTabContextMenuNoItem->addSeparator();


		// signals
		connect(pTabBtnAdd, &QAbstractButton::clicked, this,
			[this]() { this->AddSiteTabItem(-1); });
		connect(pTabBtnDel, &QAbstractButton::clicked, this,
			[this]() { this->DelTabItem(m_sitestab); });
		connect(pTabBtnUp, &QAbstractButton::clicked, this,
			[this]() { this->MoveTabItemUp(m_sitestab); });
		connect(pTabBtnDown, &QAbstractButton::clicked, this,
			[this]() { this->MoveTabItemDown(m_sitestab); });
		connect(pTabBtnSG, &QAbstractButton::clicked,
			this, &MagDynDlg::GenerateFromSG);

		connect(m_sitestab, &QTableWidget::itemChanged,
			this, &MagDynDlg::SitesTableItemChanged);
		connect(m_sitestab, &QTableWidget::customContextMenuRequested, this,
			[this, pTabContextMenu, pTabContextMenuNoItem](const QPoint& pt)
		{
			this->ShowTableContextMenu(
				m_sitestab, pTabContextMenu, pTabContextMenuNoItem, pt);
		});

		m_tabs->addTab(m_sitespanel, "Atoms");
	}


	// exchange terms panel
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
		m_termstab->verticalHeader()->setVisible(false);

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

		QPushButton *pTabBtnAdd = new QPushButton("Add Term", m_termspanel);
		QPushButton *pTabBtnDel = new QPushButton("Delete Term", m_termspanel);
		QPushButton *pTabBtnUp = new QPushButton("Move Term Up", m_termspanel);
		QPushButton *pTabBtnDown = new QPushButton("Move Term Down", m_termspanel);
		pTabBtnAdd->setFocusPolicy(Qt::StrongFocus);
		pTabBtnDel->setFocusPolicy(Qt::StrongFocus);
		pTabBtnUp->setFocusPolicy(Qt::StrongFocus);
		pTabBtnDown->setFocusPolicy(Qt::StrongFocus);

		pTabBtnAdd->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDel->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnUp->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDown->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});


		auto grid = new QGridLayout(m_termspanel);
		grid->setSpacing(4);
		grid->setContentsMargins(6, 6, 6, 6);

		int y = 0;
		grid->addWidget(m_termstab, y,0,1,4);
		grid->addWidget(pTabBtnAdd, ++y,0,1,1);
		grid->addWidget(pTabBtnDel, y,1,1,1);
		grid->addWidget(pTabBtnUp, y,2,1,1);
		grid->addWidget(pTabBtnDown, y,3,1,1);


		// table CustomContextMenu
		QMenu *pTabContextMenu = new QMenu(m_termstab);
		pTabContextMenu->addAction("Add Term Before", this,
			[this]() { this->AddTermTabItem(-2); });
		pTabContextMenu->addAction("Add Term After", this,
			[this]() { this->AddTermTabItem(-3); });
		pTabContextMenu->addAction("Clone Term", this,
			[this]() { this->AddTermTabItem(-4); });
		pTabContextMenu->addAction("Delete Term", this,
			[this]() { this->DelTabItem(m_termstab); });


		// table CustomContextMenu in case nothing is selected
		QMenu *pTabContextMenuNoItem = new QMenu(m_termstab);
		pTabContextMenuNoItem->addAction("Add Term", this,
			[this]() { this->AddTermTabItem(); });
		pTabContextMenuNoItem->addAction("Delete Term", this,
			[this]() { this->DelTabItem(m_termstab); });
		//pTabContextMenuNoItem->addSeparator();


		// signals
		connect(pTabBtnAdd, &QAbstractButton::clicked, this,
			[this]() { this->AddTermTabItem(-1); });
		connect(pTabBtnDel, &QAbstractButton::clicked, this,
			[this]() { this->DelTabItem(m_termstab); });
		connect(pTabBtnUp, &QAbstractButton::clicked, this,
			[this]() { this->MoveTabItemUp(m_termstab); });
		connect(pTabBtnDown, &QAbstractButton::clicked, this,
			[this]() { this->MoveTabItemDown(m_termstab); });

		connect(m_termstab, &QTableWidget::itemChanged,
			this, &MagDynDlg::TermsTableItemChanged);
		connect(m_termstab, &QTableWidget::customContextMenuRequested, this,
			[this, pTabContextMenu, pTabContextMenuNoItem](const QPoint& pt)
			{ this->ShowTableContextMenu(m_termstab, pTabContextMenu, pTabContextMenuNoItem, pt); });

		m_tabs->addTab(m_termspanel, "Couplings");
	}


	// sample environment panel
	{
		m_samplepanel = new QWidget(this);

		// field magnitude
		m_field_mag = new QDoubleSpinBox(m_samplepanel);
		m_field_mag->setDecimals(2);
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
			"Align Spins along Field Direction", m_samplepanel);
		m_align_spins->setChecked(false);
		m_align_spins->setFocusPolicy(Qt::StrongFocus);

		// rotation axis
		m_rot_axis[0] = new QDoubleSpinBox(m_samplepanel);
		m_rot_axis[1] = new QDoubleSpinBox(m_samplepanel);
		m_rot_axis[2] = new QDoubleSpinBox(m_samplepanel);

		// rotation angle
		m_rot_angle = new QDoubleSpinBox(m_samplepanel);
		m_rot_angle->setDecimals(2);
		m_rot_angle->setMinimum(-360);
		m_rot_angle->setMaximum(+360);
		m_rot_angle->setSingleStep(0.1);
		m_rot_angle->setValue(90.);
		m_rot_angle->setSuffix("°");
		m_rot_angle->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});

		QPushButton *btn_rotate = new QPushButton(
			"Rotate Field", m_samplepanel);
		btn_rotate->setFocusPolicy(Qt::StrongFocus);

		// bragg peak
		m_bragg[0] = new QDoubleSpinBox(m_samplepanel);
		m_bragg[1] = new QDoubleSpinBox(m_samplepanel);
		m_bragg[2] = new QDoubleSpinBox(m_samplepanel);

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
			m_field_dir[i]->setDecimals(2);
			m_field_dir[i]->setMinimum(-99);
			m_field_dir[i]->setMaximum(+99);
			m_field_dir[i]->setSingleStep(0.1);
			m_field_dir[i]->setValue(i == 2 ? 1. : 0.);
			m_field_dir[i]->setSizePolicy(QSizePolicy{
				QSizePolicy::Expanding, QSizePolicy::Fixed});

			m_rot_axis[i]->setDecimals(2);
			m_rot_axis[i]->setMinimum(-99);
			m_rot_axis[i]->setMaximum(+99);
			m_rot_axis[i]->setSingleStep(0.1);
			m_rot_axis[i]->setValue(i == 2 ? 1. : 0.);
			m_rot_axis[i]->setSizePolicy(QSizePolicy{
				QSizePolicy::Expanding, QSizePolicy::Fixed});

			m_bragg[i]->setDecimals(2);
			m_bragg[i]->setMinimum(-99);
			m_bragg[i]->setMaximum(+99);
			m_bragg[i]->setSingleStep(0.1);
			m_bragg[i]->setValue(i == 0 ? 1. : 0.);
			m_bragg[i]->setSizePolicy(QSizePolicy{
				QSizePolicy::Expanding, QSizePolicy::Fixed});
		}

		m_field_dir[0]->setPrefix("Bh = ");
		m_field_dir[1]->setPrefix("Bk = ");
		m_field_dir[2]->setPrefix("Bl = ");

		auto grid = new QGridLayout(m_samplepanel);
		grid->setSpacing(4);
		grid->setContentsMargins(6, 6, 6, 6);

		int y = 0;
		grid->addWidget(new QLabel(QString("Field Magnitude:"),
			m_samplepanel), y,0,1,1);
		grid->addWidget(m_field_mag, y++,1,1,1);
		grid->addWidget(new QLabel(QString("Field Direction:"),
			m_samplepanel), y,0,1,1);
		grid->addWidget(m_field_dir[0], y,1,1,1);
		grid->addWidget(m_field_dir[1], y,2,1,1);
		grid->addWidget(m_field_dir[2], y++,3,1,1);
		grid->addWidget(m_align_spins, y++,0,1,2);

		grid->addItem(new QSpacerItem(8, 8,
			QSizePolicy::Minimum, QSizePolicy::Fixed),
			y++,0, 1,1);
		auto sep1 = new QFrame(m_samplepanel);
		sep1->setFrameStyle(QFrame::HLine);
		grid->addWidget(sep1, y++,0, 1,4);
		grid->addItem(new QSpacerItem(8, 8,
			QSizePolicy::Minimum, QSizePolicy::Fixed),
			y++,0, 1,1);

		grid->addWidget(new QLabel(QString("Rotation Axis:"),
			m_samplepanel), y,0,1,1);
		grid->addWidget(m_rot_axis[0], y,1,1,1);
		grid->addWidget(m_rot_axis[1], y,2,1,1);
		grid->addWidget(m_rot_axis[2], y++,3,1,1);
		grid->addWidget(new QLabel(QString("Rotation Angle:"),
			m_samplepanel), y,0,1,1);
		grid->addWidget(m_rot_angle, y,1,1,1);
		grid->addWidget(btn_rotate, y++,3,1,1);

		grid->addItem(new QSpacerItem(16, 16,
			QSizePolicy::Minimum, QSizePolicy::Fixed),
			y++,0, 1,1);
		auto sep2 = new QFrame(m_samplepanel);
		sep2->setFrameStyle(QFrame::HLine);
		grid->addWidget(sep2, y++,0, 1,4);
		grid->addItem(new QSpacerItem(16, 16,
			QSizePolicy::Minimum, QSizePolicy::Fixed),
			y++,0, 1,1);

		grid->addWidget(new QLabel(QString("Bragg Peak:"),
			m_samplepanel), y,0,1,1);
		grid->addWidget(m_bragg[0], y,1,1,1);
		grid->addWidget(m_bragg[1], y,2,1,1);
		grid->addWidget(m_bragg[2], y++,3,1,1);

		grid->addItem(new QSpacerItem(16, 16,
			QSizePolicy::Minimum, QSizePolicy::Fixed),
			y++,0, 1,1);
		auto sep3 = new QFrame(m_samplepanel);
		sep3->setFrameStyle(QFrame::HLine);
		grid->addWidget(sep3, y++,0, 1,4);
		grid->addItem(new QSpacerItem(16, 16,
			QSizePolicy::Minimum, QSizePolicy::Fixed),
			y++,0, 1,1);

		grid->addWidget(new QLabel(QString("Temperature:"),
			m_samplepanel), y,0,1,1);
		grid->addWidget(m_temperature, y++,1,1,1);

		grid->addItem(new QSpacerItem(16, 16,
			QSizePolicy::Minimum, QSizePolicy::Expanding),
			y++,0,1,4);


		// signals
		connect(m_field_mag,
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			[this]() { this->SyncSitesAndTerms(); });

		for(int i=0; i<3; ++i)
		{
			connect(m_field_dir[i],
				static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
				[this]() { this->SyncSitesAndTerms(); });

			connect(m_bragg[i],
				static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
				[this]() { this->SyncSitesAndTerms(); });
		}

		connect(m_temperature,
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			[this]() { this->SyncSitesAndTerms(); });

		connect(m_align_spins, &QCheckBox::toggled,
			[this]() { this->SyncSitesAndTerms(); });

		connect(btn_rotate, &QAbstractButton::clicked, [this]()
		{
			t_vec_real axis = tl2::create<t_vec_real>(
			{
				m_rot_axis[0]->value(),
				m_rot_axis[1]->value(),
				m_rot_axis[2]->value(),
			});

			t_vec_real B = tl2::create<t_vec_real>(
			{
				m_field_dir[0]->value(),
				m_field_dir[1]->value(),
				m_field_dir[2]->value(),
			});

			t_real angle = m_rot_angle->value() / 180.*tl2::pi<t_real>;

			t_mat_real R = tl2::rotation<t_mat_real, t_vec_real>(
				axis, angle, false);
			B = R*B;
			tl2::set_eps_0(B, g_eps);

			for(int i=0; i<3; ++i)
			{
				m_field_dir[i]->blockSignals(true);
				m_field_dir[i]->setValue(B[i]);
				m_field_dir[i]->blockSignals(false);
			}

			this->SyncSitesAndTerms();
		});

		m_tabs->addTab(m_samplepanel, "Sample");
	}


	// dispersion panel
	{
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
		m_weight_scale = new QDoubleSpinBox(m_disppanel);
		m_weight_scale->setDecimals(2);
		m_weight_scale->setMinimum(0);
		m_weight_scale->setMaximum(+99);
		m_weight_scale->setSingleStep(0.1);
		m_weight_scale->setValue(1.);
		m_weight_scale->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Fixed});

		for(int i=0; i<3; ++i)
		{
			m_q_start[i]->setDecimals(2);
			m_q_start[i]->setMinimum(-99);
			m_q_start[i]->setMaximum(+99);
			m_q_start[i]->setSingleStep(0.1);
			m_q_start[i]->setValue(0.);
			m_q_start[i]->setSuffix(" rlu");
			m_q_start[i]->setSizePolicy(QSizePolicy{
				QSizePolicy::Expanding, QSizePolicy::Fixed});

			m_q_end[i]->setDecimals(2);
			m_q_end[i]->setMinimum(-99);
			m_q_end[i]->setMaximum(+99);
			m_q_end[i]->setSingleStep(0.1);
			m_q_end[i]->setValue(0.);
			m_q_end[i]->setSuffix(" rlu");
			m_q_end[i]->setSizePolicy(QSizePolicy{
				QSizePolicy::Expanding, QSizePolicy::Fixed});
		}

		m_q_start[0]->setPrefix("h = ");
		m_q_start[1]->setPrefix("k = ");
		m_q_start[2]->setPrefix("l = ");
		m_q_end[0]->setPrefix("h = ");
		m_q_end[1]->setPrefix("k = ");
		m_q_end[2]->setPrefix("l = ");

		m_q_start[0]->setValue(-1.);
		m_q_end[0]->setValue(+1.);

		auto grid = new QGridLayout(m_disppanel);
		grid->setSpacing(4);
		grid->setContentsMargins(6, 6, 6, 6);

		int y = 0;
		grid->addWidget(m_plot, y++,0,1,4);
		grid->addWidget(
			new QLabel(QString("Starting Q:"), m_disppanel), y,0,1,1);
		grid->addWidget(m_q_start[0], y,1,1,1);
		grid->addWidget(m_q_start[1], y,2,1,1);
		grid->addWidget(m_q_start[2], y++,3,1,1);
		grid->addWidget(
			new QLabel(QString("Ending Q:"), m_disppanel), y,0,1,1);
		grid->addWidget(m_q_end[0], y,1,1,1);
		grid->addWidget(m_q_end[1], y,2,1,1);
		grid->addWidget(m_q_end[2], y++,3,1,1);
		grid->addWidget(
			new QLabel(QString("Number of Qs:"), m_disppanel), y,0,1,1);
		grid->addWidget(m_num_points, y,1,1,1);
		grid->addWidget(
			new QLabel(QString("Weight Scale:"), m_disppanel), y,2,1,1);
		grid->addWidget(m_weight_scale, y++,3,1,1);

		// signals
		for(int i=0; i<3; ++i)
		{
			connect(m_q_start[i],
				static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
				[this]() { this->CalcDispersion(); });
			connect(m_q_end[i],
				static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
				[this]() { this->CalcDispersion(); });
		}

		connect(m_num_points,
			static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			[this]() { this->CalcDispersion(); });

		connect(m_weight_scale,
			static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
			[this]() { this->CalcDispersion(); });

		connect(m_plot, &QCustomPlot::mouseMove,
			this, &MagDynDlg::PlotMouseMove);

		m_tabs->addTab(m_disppanel, "Dispersion");
	}


	// hamiltonian panel
	{
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
			m_q[i]->setDecimals(2);
			m_q[i]->setMinimum(-99);
			m_q[i]->setMaximum(+99);
			m_q[i]->setSingleStep(0.1);
			m_q[i]->setValue(0.);
			m_q[i]->setSuffix(" rlu");
			m_q[i]->setSizePolicy(QSizePolicy{
				QSizePolicy::Expanding, QSizePolicy::Fixed});
		}

		m_q[0]->setPrefix("h = ");
		m_q[1]->setPrefix("k = ");
		m_q[2]->setPrefix("l = ");

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
				[this]() { this->CalcHamiltonian(); });
		}

		m_tabs->addTab(m_hamiltonianpanel, "Hamiltonian");
	}


	// info panel
	{
		auto infopanel = new QWidget(this);
		auto grid = new QGridLayout(infopanel);
		grid->setSpacing(4);
		grid->setContentsMargins(6, 6, 6, 6);

		auto sep1 = new QFrame(infopanel);
		sep1->setFrameStyle(QFrame::HLine);
		auto sep2 = new QFrame(infopanel);
		sep2->setFrameStyle(QFrame::HLine);
		auto sep3 = new QFrame(infopanel);
		sep3->setFrameStyle(QFrame::HLine);
		auto sep4 = new QFrame(infopanel);
		sep4->setFrameStyle(QFrame::HLine);

		std::string strBoost = BOOST_LIB_VERSION;
		algo::replace_all(strBoost, "_", ".");

		auto labelTitle = new QLabel("Magnon Dynamics Calculator", infopanel);
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

		grid->addItem(new QSpacerItem(16,16,
			QSizePolicy::Minimum, QSizePolicy::Expanding),
			y++,0, 1,1);

		m_tabs->addTab(infopanel, "Infos");
	}

	// status
	m_status = new QLabel(this);
	m_status->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	m_status->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	// main grid
	auto pmainGrid = new QGridLayout(this);
	pmainGrid->setSpacing(4);
	pmainGrid->setContentsMargins(6, 6, 6, 6);
	pmainGrid->addWidget(m_tabs, 0,0, 1,1);
	pmainGrid->addWidget(m_status, 1,0,1,1);

	// menu bar
	{
		m_menu = new QMenuBar(this);
		m_menu->setNativeMenuBar(m_sett ? m_sett->value("native_gui", false).toBool() : false);

		auto menuFile = new QMenu("File", m_menu);
		auto acNew = new QAction("New", menuFile);
		auto acLoad = new QAction("Open...", menuFile);
		auto acSave = new QAction("Save...", menuFile);
		auto acExit = new QAction("Quit", menuFile);

		auto menuView = new QMenu("View", m_menu);
		auto acStructView = new QAction("Show Structure...", menuView);
		auto acSaveFigure = new QAction("Save Dispersion Figure...", menuView);
		auto acRescalePlot = new QAction("Rescale Dispersion Axes", menuView);

		acNew->setShortcut(QKeySequence::New);
		acLoad->setShortcut(QKeySequence::Open);
		acSave->setShortcut(QKeySequence::Save);
		acExit->setShortcut(QKeySequence::Quit);

		acExit->setMenuRole(QAction::QuitRole);

		auto menuOptions = new QMenu("Options", m_menu);
		m_use_dmi = new QAction("Use DMI", menuOptions);
		m_use_dmi->setToolTip("Enables the Dzyaloshinskij-Moriya interaction.");
		m_use_dmi->setCheckable(true);
		m_use_dmi->setChecked(true);
		m_use_field = new QAction("Use External Field", menuOptions);
		m_use_field->setToolTip("Enables an external field.");
		m_use_field->setCheckable(true);
		m_use_field->setChecked(true);
		m_use_temperature = new QAction("Use Bose Factor", menuOptions);
		m_use_temperature->setToolTip("Enables the Bose factor.");
		m_use_temperature->setCheckable(true);
		m_use_temperature->setChecked(true);
		m_use_weights = new QAction("Use Spectral Weights", menuOptions);
		m_use_weights->setToolTip("Enables calculation of the spin correlation function.");
		m_use_weights->setCheckable(true);
		m_use_weights->setChecked(false);
		m_use_projector = new QAction("Use Neutron Weights", menuOptions);
		m_use_projector->setToolTip("Enables the neutron orthogonal projector.");
		m_use_projector->setCheckable(true);
		m_use_projector->setChecked(true);

		menuFile->addAction(acNew);
		menuFile->addSeparator();
		menuFile->addAction(acLoad);
		menuFile->addAction(acSave);
		menuFile->addSeparator();
		menuFile->addAction(acExit);

		menuView->addAction(acStructView);
		menuView->addSeparator();
		menuView->addAction(acRescalePlot);
		menuView->addAction(acSaveFigure);

		menuOptions->addAction(m_use_dmi);
		menuOptions->addAction(m_use_field);
		menuOptions->addAction(m_use_temperature);
		menuOptions->addAction(m_use_weights);
		menuOptions->addAction(m_use_projector);

		// connections
		connect(acNew, &QAction::triggered, this, &MagDynDlg::Clear);
		connect(acLoad, &QAction::triggered, this, &MagDynDlg::Load);
		connect(acSave, &QAction::triggered, this, &MagDynDlg::Save);
		connect(acExit, &QAction::triggered, this, &QDialog::close);

		connect(acSaveFigure, &QAction::triggered,
			this, &MagDynDlg::SavePlotFigure);
		connect(acRescalePlot, &QAction::triggered,
			[this]()
		{
			if(!m_plot)
				return;

			m_plot->rescaleAxes();
			m_plot->replot();
		});

		connect(acStructView, &QAction::triggered,
			this, &MagDynDlg::ShowStructurePlot);

		connect(m_use_dmi, &QAction::toggled,
			[this]() { this->SyncSitesAndTerms(); });
		connect(m_use_field, &QAction::toggled,
			[this]() { this->SyncSitesAndTerms(); });
		connect(m_use_temperature, &QAction::toggled,
			[this]() { this->SyncSitesAndTerms(); });
		connect(m_use_weights, &QAction::toggled,
			[this]() { this->CalcDispersion(); this->CalcHamiltonian(); });
		connect(m_use_projector, &QAction::toggled,
			[this]() { this->CalcDispersion(); this->CalcHamiltonian(); });

		m_menu->addMenu(menuFile);
		m_menu->addMenu(menuView);
		m_menu->addMenu(menuOptions);
		pmainGrid->setMenuBar(m_menu);
	}


	// restory window size and position
	if(m_sett && m_sett->contains("geo"))
		restoreGeometry(m_sett->value("geo").toByteArray());
	else
		resize(600, 500);

	m_ignoreTableChanges = 0;
}


MagDynDlg::~MagDynDlg()
{
	Clear();
}


void MagDynDlg::Clear()
{
	m_ignoreCalc = true;

	// clear old tables
	DelTabItem(m_sitestab, -1);
	DelTabItem(m_termstab, -1);

	m_plot->clearPlottables();
	m_plot->replot();
	m_hamiltonian->clear();

	m_dyn.Clear();

	// set some defaults
	m_comboSG->setCurrentIndex(0);

	m_ignoreCalc = false;
}


/**
 * generate atom sites form the space group symmetries
 */
void MagDynDlg::GenerateFromSG()
{
	m_ignoreCalc = 1;

	try
	{
		// symops of current space group
		auto sgidx = m_comboSG->itemData(m_comboSG->currentIndex()).toInt();
		if(sgidx < 0 || std::size_t(sgidx) >= m_SGops.size())
		{
			QMessageBox::critical(this, "Magnon Dynamics",
				"Invalid space group selected.");
			m_ignoreCalc = 0;
			return;
		}

		const auto& ops = m_SGops[sgidx];
		std::vector<std::tuple<
			std::string,
			t_real, t_real, t_real, // position
			t_real, t_real, t_real, // spin direction
			t_real                  // spin magnitude
			>> generatedsites;

		// iterate sites
		int orgRowCnt = m_sitestab->rowCount();
		for(int row=0; row<orgRowCnt; ++row)
		{
			auto *name = m_sitestab->item(row, COL_SITE_NAME);
			auto *pos_x = m_sitestab->item(row, COL_SITE_POS_X);
			auto *pos_y = m_sitestab->item(row, COL_SITE_POS_Y);
			auto *pos_z = m_sitestab->item(row, COL_SITE_POS_Z);
			auto *spin_x = m_sitestab->item(row, COL_SITE_SPIN_X);
			auto *spin_y = m_sitestab->item(row, COL_SITE_SPIN_Y);
			auto *spin_z = m_sitestab->item(row, COL_SITE_SPIN_Z);
			auto *spin_mag = m_sitestab->item(row, COL_SITE_SPIN_MAG);

			t_real x{}, y{}, z{}, sx{}, sy{}, sz{}, S;
			std::string ident = name->text().toStdString();
			std::istringstream{pos_x->text().toStdString()} >> x;
			std::istringstream{pos_y->text().toStdString()} >> y;
			std::istringstream{pos_z->text().toStdString()} >> z;
			std::istringstream{spin_x->text().toStdString()} >> sx;
			std::istringstream{spin_y->text().toStdString()} >> sy;
			std::istringstream{spin_z->text().toStdString()} >> sz;
			std::istringstream{spin_mag->text().toStdString()} >> S;

			t_vec_real sitepos = tl2::create<t_vec_real>({x, y, z, 1});
			auto newsitepos = tl2::apply_ops_hom<t_vec_real, t_mat_real, t_real>(
				sitepos, ops, g_eps);

			for(const auto& newsite : newsitepos)
			{
				generatedsites.emplace_back(std::make_tuple(
					ident, newsite[0], newsite[1], newsite[2],
					sx, sy, sz, S));
			}
		}

		// remove original sites
		DelTabItem(m_sitestab, -1);

		// add new sites
		for(const auto& site : generatedsites)
		{
			std::apply(&MagDynDlg::AddSiteTabItem,
				std::tuple_cat(std::make_tuple(this, -1), site));
		}
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "MagnonDynamics", ex.what());
	}

	m_ignoreCalc = 0;
	CalcDispersion();
	CalcHamiltonian();
}


/**
 * add an atom site
 */
void MagDynDlg::AddSiteTabItem(int row,
	const std::string& name,
	t_real x, t_real y, t_real z,
	t_real sx, t_real sy, t_real sz,
	t_real S)
{
	bool bclone = 0;
	m_ignoreTableChanges = 1;

	if(row == -1)	// append to end of table
		row = m_sitestab->rowCount();
	else if(row == -2 && m_sites_cursor_row >= 0)	// use row from member variable
		row = m_sites_cursor_row;
	else if(row == -3 && m_sites_cursor_row >= 0)	// use row from member variable +1
		row = m_sites_cursor_row + 1;
	else if(row == -4 && m_sites_cursor_row >= 0)	// use row from member variable +1
	{
		row = m_sites_cursor_row + 1;
		bclone = 1;
	}

	m_sitestab->setSortingEnabled(false);
	m_sitestab->insertRow(row);

	if(bclone)
	{
		for(int thecol=0; thecol<NUM_SITE_COLS; ++thecol)
		{
			m_sitestab->setItem(row, thecol,
				m_sitestab->item(m_sites_cursor_row, thecol)->clone());
		}
	}
	else
	{
		m_sitestab->setItem(row, COL_SITE_NAME,
			new QTableWidgetItem(name.c_str()));
		m_sitestab->setItem(row, COL_SITE_POS_X,
			new NumericTableWidgetItem<t_real>(x));
		m_sitestab->setItem(row, COL_SITE_POS_Y,
			new NumericTableWidgetItem<t_real>(y));
		m_sitestab->setItem(row, COL_SITE_POS_Z,
			new NumericTableWidgetItem<t_real>(z));
		m_sitestab->setItem(row, COL_SITE_SPIN_X,
			new NumericTableWidgetItem<t_real>(sx));
		m_sitestab->setItem(row, COL_SITE_SPIN_Y,
			new NumericTableWidgetItem<t_real>(sy));
		m_sitestab->setItem(row, COL_SITE_SPIN_Z,
			new NumericTableWidgetItem<t_real>(sz));
		m_sitestab->setItem(row, COL_SITE_SPIN_MAG,
			new NumericTableWidgetItem<t_real>(S));
	}

	m_sitestab->scrollToItem(m_sitestab->item(row, 0));
	m_sitestab->setCurrentCell(row, 0);

	m_sitestab->setSortingEnabled(/*sorting*/ true);

	m_ignoreTableChanges = 0;
	SyncSitesAndTerms();
}


/**
 * add an exchange term
 */
void MagDynDlg::AddTermTabItem(int row,
	const std::string& name,
	t_size atom_1, t_size atom_2,
	t_real dist_x, t_real dist_y, t_real dist_z,
	t_real J,
	t_real dmi_x, t_real dmi_y, t_real dmi_z)
{
	bool bclone = 0;
	m_ignoreTableChanges = 1;

	if(row == -1)	// append to end of table
		row = m_termstab->rowCount();
	else if(row == -2 && m_terms_cursor_row >= 0)	// use row from member variable
		row = m_terms_cursor_row;
	else if(row == -3 && m_terms_cursor_row >= 0)	// use row from member variable +1
		row = m_terms_cursor_row + 1;
	else if(row == -4 && m_terms_cursor_row >= 0)	// use row from member variable +1
	{
		row = m_terms_cursor_row + 1;
		bclone = 1;
	}

	m_termstab->setSortingEnabled(false);
	m_termstab->insertRow(row);

	if(bclone)
	{
		for(int thecol=0; thecol<NUM_XCH_COLS; ++thecol)
		{
			m_termstab->setItem(row, thecol,
				m_termstab->item(m_terms_cursor_row, thecol)->clone());
		}
	}
	else
	{
		m_termstab->setItem(row, COL_XCH_NAME,
			new QTableWidgetItem(name.c_str()));
		m_termstab->setItem(row, COL_XCH_ATOM1_IDX,
			new NumericTableWidgetItem<t_size>(atom_1));
		m_termstab->setItem(row, COL_XCH_ATOM2_IDX,
			new NumericTableWidgetItem<t_size>(atom_2));
		m_termstab->setItem(row, COL_XCH_DIST_X,
			new NumericTableWidgetItem<t_real>(dist_x));
		m_termstab->setItem(row, COL_XCH_DIST_Y,
			new NumericTableWidgetItem<t_real>(dist_y));
		m_termstab->setItem(row, COL_XCH_DIST_Z,
			new NumericTableWidgetItem<t_real>(dist_z));
		m_termstab->setItem(row, COL_XCH_INTERACTION,
			new NumericTableWidgetItem<t_real>(J));
		m_termstab->setItem(row, COL_XCH_DMI_X,
			new NumericTableWidgetItem<t_real>(dmi_x));
		m_termstab->setItem(row, COL_XCH_DMI_Y,
			new NumericTableWidgetItem<t_real>(dmi_y));
		m_termstab->setItem(row, COL_XCH_DMI_Z,
			new NumericTableWidgetItem<t_real>(dmi_z));
	}

	m_termstab->scrollToItem(m_termstab->item(row, 0));
	m_termstab->setCurrentCell(row, 0);

	m_termstab->setSortingEnabled(/*sorting*/ true);

	m_ignoreTableChanges = 0;
	SyncSitesAndTerms();
}


void MagDynDlg::DelTabItem(QTableWidget *pTab, int begin, int end)
{
	m_ignoreTableChanges = 1;

	// if nothing is selected, clear all items
	if(begin == -1 || pTab->selectedItems().count() == 0)
	{
		pTab->clearContents();
		pTab->setRowCount(0);
	}
	else if(begin == -2)	// clear selected
	{
		for(int row : GetSelectedRows(pTab, true))
		{
			pTab->removeRow(row);
		}
	}
	else if(begin >= 0 && end >= 0)		// clear given range
	{
		for(int row=end-1; row>=begin; --row)
		{
			pTab->removeRow(row);
		}
	}

	m_ignoreTableChanges = 0;
	SyncSitesAndTerms();
}


void MagDynDlg::MoveTabItemUp(QTableWidget *pTab)
{
	m_ignoreTableChanges = 1;
	pTab->setSortingEnabled(false);

	auto selected = GetSelectedRows(pTab, false);
	for(int row : selected)
	{
		if(row == 0)
			continue;

		auto *item = pTab->item(row, 0);
		if(!item || !item->isSelected())
			continue;

		pTab->insertRow(row-1);
		for(int col=0; col<pTab->columnCount(); ++col)
			pTab->setItem(row-1, col, pTab->item(row+1, col)->clone());
		pTab->removeRow(row+1);
	}

	for(int row=0; row<pTab->rowCount(); ++row)
	{
		if(auto *item = pTab->item(row, 0);
		   item && std::find(selected.begin(), selected.end(), row+1) != selected.end())
		   {
			   for(int col=0; col<pTab->columnCount(); ++col)
				   pTab->item(row, col)->setSelected(true);
		   }
	}

	m_ignoreTableChanges = 0;
	SyncSitesAndTerms();
}


void MagDynDlg::MoveTabItemDown(QTableWidget *pTab)
{
	m_ignoreTableChanges = 1;
	pTab->setSortingEnabled(false);

	auto selected = GetSelectedRows(pTab, true);
	for(int row : selected)
	{
		if(row == pTab->rowCount()-1)
			continue;

		auto *item = pTab->item(row, 0);
		if(!item || !item->isSelected())
			continue;

		pTab->insertRow(row+2);
		for(int col=0; col<pTab->columnCount(); ++col)
			pTab->setItem(row+2, col, pTab->item(row, col)->clone());
		pTab->removeRow(row);
	}

	for(int row=0; row<pTab->rowCount(); ++row)
	{
		if(auto *item = pTab->item(row, 0);
		   item && std::find(selected.begin(), selected.end(), row-1) != selected.end())
		   {
			   for(int col=0; col<pTab->columnCount(); ++col)
				   pTab->item(row, col)->setSelected(true);
		   }
	}

	m_ignoreTableChanges = 0;
	SyncSitesAndTerms();
}


std::vector<int> MagDynDlg::GetSelectedRows(
	QTableWidget *pTab, bool sort_reversed) const
{
	std::vector<int> vec;
	vec.reserve(pTab->selectedItems().size());

	for(int row=0; row<pTab->rowCount(); ++row)
	{
		if(auto *item = pTab->item(row, 0); item && item->isSelected())
			vec.push_back(row);
	}

	if(sort_reversed)
	{
		std::stable_sort(vec.begin(), vec.end(), [](int row1, int row2)
		{ return row1 > row2; });
	}

	return vec;
}


/**
 * item contents changed
 */
void MagDynDlg::SitesTableItemChanged(QTableWidgetItem * /*item*/)
{
	if(m_ignoreTableChanges)
		return;

	SyncSitesAndTerms();
}


/**
 * item contents changed
 */
void MagDynDlg::TermsTableItemChanged(QTableWidgetItem * /*item*/)
{
	if(m_ignoreTableChanges)
		return;

	SyncSitesAndTerms();
}


void MagDynDlg::ShowTableContextMenu(
	QTableWidget *pTab, QMenu *pMenu, QMenu *pMenuNoItem, const QPoint& pt)
{
	auto ptGlob = pTab->mapToGlobal(pt);

	if(const auto* item = pTab->itemAt(pt); item)
	{
		if(pTab == m_termstab)
			m_terms_cursor_row = item->row();
		else if(pTab == m_sitestab)
			m_sites_cursor_row = item->row();

		ptGlob.setY(ptGlob.y() + pMenu->sizeHint().height()/2);
		pMenu->popup(ptGlob);
	}
	else
	{
		ptGlob.setY(ptGlob.y() + pMenuNoItem->sizeHint().height()/2);
		pMenuNoItem->popup(ptGlob);
	}
}


/**
 * get the sites and exchange terms from the table
 * and transfer them to the dynamics calculator
 */
void MagDynDlg::SyncSitesAndTerms()
{
	if(m_ignoreCalc)
		return;

	m_dyn.Clear();

	// dmi
	bool use_dmi = m_use_dmi->isChecked();

	// get external field
	if(m_use_field->isChecked())
	{
		ExternalField field;
		field.dir = tl2::create<t_vec>(
		{
			m_field_dir[0]->value(),
			m_field_dir[1]->value(),
			m_field_dir[2]->value(),
		});

		field.mag = m_field_mag->value();
		field.align_spins = m_align_spins->isChecked();

		m_dyn.SetExternalField(field);
	}

	// get bragg peak
	{
		const t_real bragg[]
		{
			m_bragg[0]->value(),
			m_bragg[1]->value(),
			m_bragg[2]->value(),
		};

		m_dyn.SetBraggPeak(bragg[0], bragg[1], bragg[2]);
	}

	// get temperature
	if(m_use_temperature->isChecked())
	{
		t_real temp = m_temperature->value();
		m_dyn.SetTemperature(temp);
	}

	// get atom sites
	for(int row=0; row<m_sitestab->rowCount(); ++row)
	{
		auto *name = m_sitestab->item(row, COL_SITE_NAME);
		auto *pos_x = m_sitestab->item(row, COL_SITE_POS_X);
		auto *pos_y = m_sitestab->item(row, COL_SITE_POS_Y);
		auto *pos_z = m_sitestab->item(row, COL_SITE_POS_Z);
		auto *spin_x = m_sitestab->item(row, COL_SITE_SPIN_X);
		auto *spin_y = m_sitestab->item(row, COL_SITE_SPIN_Y);
		auto *spin_z = m_sitestab->item(row, COL_SITE_SPIN_Z);
		auto *spin_mag = m_sitestab->item(row, COL_SITE_SPIN_MAG);

		if(!name || !pos_x || !pos_y || !pos_z || 
			!spin_x || !spin_y || !spin_z || !spin_mag)
		{
			std::cerr << "Invalid entry in sites table row "
				<< row << "." << std::endl;
			continue;
		}

		AtomSite site;
		site.name = name->text().toStdString();
		site.pos = tl2::zero<t_vec>(3);
		site.spin_dir = tl2::zero<t_vec>(3);
		site.g = -2. * tl2::unit<t_mat>(3);
		std::istringstream{pos_x->text().toStdString()} >> site.pos[0];
		std::istringstream{pos_y->text().toStdString()} >> site.pos[1];
		std::istringstream{pos_z->text().toStdString()} >> site.pos[2];
		std::istringstream{spin_mag->text().toStdString()} >> site.spin_mag;

		// align spins along external field?
		if(m_align_spins->isChecked() && m_use_field->isChecked())
		{
			site.spin_dir[0] = m_field_dir[0]->value();
			site.spin_dir[1] = m_field_dir[1]->value();
			site.spin_dir[2] = m_field_dir[2]->value();
		}
		else
		{
			std::istringstream{spin_x->text().toStdString()}
				>> site.spin_dir[0];
			std::istringstream{spin_y->text().toStdString()}
				>> site.spin_dir[1];
			std::istringstream{spin_z->text().toStdString()}
				>> site.spin_dir[2];
		}

		m_dyn.AddAtomSite(std::move(site));
	}

	m_dyn.CalcIndices();
	m_dyn.CalcSpinRotation();

	// get exchange terms
	for(int row=0; row<m_termstab->rowCount(); ++row)
	{
		auto *name = m_termstab->item(row, COL_XCH_NAME);
		auto *atom_1_idx = m_termstab->item(row, COL_XCH_ATOM1_IDX);
		auto *atom_2_idx = m_termstab->item(row, COL_XCH_ATOM2_IDX);
		auto *dist_x = m_termstab->item(row, COL_XCH_DIST_X);
		auto *dist_y = m_termstab->item(row, COL_XCH_DIST_Y);
		auto *dist_z = m_termstab->item(row, COL_XCH_DIST_Z);
		auto *interaction = m_termstab->item(row, COL_XCH_INTERACTION);
		auto *dmi_x = m_termstab->item(row, COL_XCH_DMI_X);
		auto *dmi_y = m_termstab->item(row, COL_XCH_DMI_Y);
		auto *dmi_z = m_termstab->item(row, COL_XCH_DMI_Z);

		if(!name || !atom_1_idx || !atom_2_idx ||
			!dist_x || !dist_y || !dist_z ||
			!interaction || !dmi_x || !dmi_y || !dmi_z)
		{
			std::cerr << "Invalid entry in terms table row " << row << "." << std::endl;
			continue;
		}

		ExchangeTerm term;
		term.name = name->text().toStdString();
		term.dist = tl2::zero<t_vec>(3);

		std::istringstream{atom_1_idx->text().toStdString()} >> term.atom1;
		std::istringstream{atom_2_idx->text().toStdString()} >> term.atom2;
		std::istringstream{dist_x->text().toStdString()} >> term.dist[0];
		std::istringstream{dist_y->text().toStdString()} >> term.dist[1];
		std::istringstream{dist_z->text().toStdString()} >> term.dist[2];
		std::istringstream{interaction->text().toStdString()} >> term.J;

		if(use_dmi)
		{
			term.dmi = tl2::zero<t_vec>(3);

			std::istringstream{dmi_x->text().toStdString()} >> term.dmi[0];
			std::istringstream{dmi_y->text().toStdString()} >> term.dmi[1];
			std::istringstream{dmi_z->text().toStdString()} >> term.dmi[2];
		}

		m_dyn.AddExchangeTerm(std::move(term));
	}

	CalcDispersion();
	CalcHamiltonian();

	StructPlotSync();
}


void MagDynDlg::mousePressEvent(QMouseEvent *evt)
{
	QDialog::mousePressEvent(evt);
}


/**
 * dialog is closing
 */
void MagDynDlg::closeEvent(QCloseEvent *)
{
	if(!m_sett)
		return;

	m_sett->setValue("geo", saveGeometry());

	if(m_structplot_dlg)
		m_sett->setValue("geo_struct_view", m_structplot_dlg->saveGeometry());
}
