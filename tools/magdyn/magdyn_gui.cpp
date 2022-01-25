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

#include "magdyn_gui.h"

#include <QtCore/QDir>
#include <QtGui/QFontDatabase>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QTabWidget>
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

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace pt = boost::property_tree;

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
namespace algo = boost::algorithm;

#include "tlibs2/libs/phys.h"
#include "tlibs2/libs/algos.h"
#include "tlibs2/libs/helper.h"

#include "graph.h"
#include "../structfact/loadcif.h"

using namespace tl2_ops;

constexpr t_real g_eps = 1e-6;
constexpr int g_prec = 6;
constexpr int g_prec_gui = 3;


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

	auto tabs = new QTabWidget(this);

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

		tabs->addTab(m_sitespanel, "Atoms");
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

		tabs->addTab(m_termspanel, "Couplings");
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

		tabs->addTab(m_samplepanel, "Sample");
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

		tabs->addTab(m_disppanel, "Dispersion");
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

		tabs->addTab(m_hamiltonianpanel, "Hamiltonian");
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

		tabs->addTab(infopanel, "Infos");
	}

	// status
	m_status = new QLabel(this);
	m_status->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	m_status->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	// main grid
	auto pmainGrid = new QGridLayout(this);
	pmainGrid->setSpacing(4);
	pmainGrid->setContentsMargins(6, 6, 6, 6);
	pmainGrid->addWidget(tabs, 0,0, 1,1);
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
 * load settings
 */
void MagDynDlg::Load()
{
	m_ignoreCalc = 1;

	try
	{
		QString dirLast = m_sett->value("dir", "").toString();
		QString filename = QFileDialog::getOpenFileName(
			this, "Load File", dirLast, "XML Files (*.xml *.XML)");
		if(filename=="" || !QFile::exists(filename))
			return;
		m_sett->setValue("dir", QFileInfo(filename).path());

		// properties tree
		pt::ptree node;

		// load from file
		std::ifstream ifstr{filename.toStdString()};
		pt::read_xml(ifstr, node);

		// check signature
		if(auto optInfo = node.get_optional<std::string>("magdyn.meta.info");
			!optInfo || !(*optInfo==std::string{"magdyn_tool"}))
		{
			QMessageBox::critical(this, "Magnon Dynamics", "Unrecognised file format.");
			return;
		}

		const auto &magdyn = node.get_child("magdyn");

		// settings
		if(auto optVal = magdyn.get_optional<t_real>("config.h_start"))
			m_q_start[0]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.k_start"))
			m_q_start[1]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.l_start"))
			m_q_start[2]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.h_end"))
			m_q_end[0]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.k_end"))
			m_q_end[1]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.l_end"))
			m_q_end[2]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.h"))
			m_q[0]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.k"))
			m_q[1]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.l"))
			m_q[2]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_size>("config.num_Q_points"))
			m_num_points->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.weight_scale"))
			m_weight_scale->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<bool>("config.use_DMI"))
			m_use_dmi->setChecked(*optVal);
		if(auto optVal = magdyn.get_optional<bool>("config.use_field"))
			m_use_field->setChecked(*optVal);
		if(auto optVal = magdyn.get_optional<bool>("config.use_temperature"))
			m_use_temperature->setChecked(*optVal);
		if(auto optVal = magdyn.get_optional<bool>("config.use_weights"))
			m_use_weights->setChecked(*optVal);
		if(auto optVal = magdyn.get_optional<bool>("config.use_projector"))
			m_use_projector->setChecked(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.field_axis_h"))
			m_rot_axis[0]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.field_axis_k"))
			m_rot_axis[1]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.field_axis_l"))
			m_rot_axis[2]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.field_angle"))
			m_rot_angle->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<int>("config.spacegroup_index"))
			m_comboSG->setCurrentIndex(*optVal);

		m_dyn.Load(magdyn);

		// external field
		m_field_dir[0]->setValue(m_dyn.GetExternalField().dir[0].real());
		m_field_dir[1]->setValue(m_dyn.GetExternalField().dir[1].real());
		m_field_dir[2]->setValue(m_dyn.GetExternalField().dir[2].real());
		m_field_mag->setValue(m_dyn.GetExternalField().mag);
		m_align_spins->setChecked(m_dyn.GetExternalField().align_spins);

		// bragg peak
		const t_vec& bragg = m_dyn.GetBraggPeak();
		if(bragg.size() == 3)
		{
			m_bragg[0]->setValue(bragg[0].real());
			m_bragg[1]->setValue(bragg[1].real());
			m_bragg[2]->setValue(bragg[2].real());
		}

		// temperature
		t_real temp = m_dyn.GetTemperature();
		if(temp >= 0.)
			m_temperature->setValue(temp);

		// clear old tables
		DelTabItem(m_sitestab, -1);
		DelTabItem(m_termstab, -1);

		// atom sites
		for(const auto &site : m_dyn.GetAtomSites())
		{
			AddSiteTabItem(-1,
				site.name,
				site.pos[0].real(), site.pos[1].real(), site.pos[2].real(),
				site.spin_dir[0].real(), site.spin_dir[1].real(), site.spin_dir[2].real(),
				site.spin_mag);
		}

		// exchange terms
		for(const auto& term : m_dyn.GetExchangeTerms())
		{
			AddTermTabItem(-1,
				term.name, term.atom1, term.atom2,
				term.dist[0].real(), term.dist[1].real(), term.dist[2].real(),
				term.J.real(),
				term.dmi[0].real(), term.dmi[1].real(), term.dmi[2].real());
		}
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Magnon Dynamics", ex.what());
	}

	m_ignoreCalc = 0;
	CalcDispersion();
	CalcHamiltonian();

	StructPlotSync();
}


/**
 * save current settings
 */
void MagDynDlg::Save()
{
	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getSaveFileName(this, "Save File", dirLast, "XML Files (*.xml *.XML)");
	if(filename=="")
		return;
	m_sett->setValue("dir", QFileInfo(filename).path());

	// properties tree
	pt::ptree magdyn;

	magdyn.put<std::string>("meta.info", "magdyn_tool");
	magdyn.put<std::string>("meta.date", tl2::epoch_to_str<t_real>(tl2::epoch<t_real>()));

	// settings
	magdyn.put<t_real>("config.h_start", m_q_start[0]->value());
	magdyn.put<t_real>("config.k_start", m_q_start[1]->value());
	magdyn.put<t_real>("config.l_start", m_q_start[2]->value());
	magdyn.put<t_real>("config.h_end", m_q_end[0]->value());
	magdyn.put<t_real>("config.k_end", m_q_end[1]->value());
	magdyn.put<t_real>("config.l_end", m_q_end[2]->value());
	magdyn.put<t_real>("config.h", m_q[0]->value());
	magdyn.put<t_real>("config.k", m_q[1]->value());
	magdyn.put<t_real>("config.l", m_q[2]->value());
	magdyn.put<t_size>("config.num_Q_points", m_num_points->value());
	magdyn.put<t_size>("config.weight_scale", m_weight_scale->value());
	magdyn.put<bool>("config.use_DMI", m_use_dmi->isChecked());
	magdyn.put<bool>("config.use_field", m_use_field->isChecked());
	magdyn.put<bool>("config.use_temperature", m_use_temperature->isChecked());
	magdyn.put<bool>("config.use_weights", m_use_weights->isChecked());
	magdyn.put<bool>("config.use_projector", m_use_projector->isChecked());
	magdyn.put<t_real>("config.field_axis_h", m_rot_axis[0]->value());
	magdyn.put<t_real>("config.field_axis_k", m_rot_axis[1]->value());
	magdyn.put<t_real>("config.field_axis_l", m_rot_axis[2]->value());
	magdyn.put<t_real>("config.field_angle", m_rot_angle->value());
	magdyn.put<t_real>("config.spacegroup_index", m_comboSG->currentIndex());
	m_dyn.Save(magdyn);

	pt::ptree node;
	node.put_child("magdyn", magdyn);

	// save to file
	std::ofstream ofstr{filename.toStdString()};
	if(!ofstr)
	{
		QMessageBox::critical(this, "Magnon Dynamics",
			"Cannot open file for writing.");
		return;
	}
	ofstr.precision(g_prec);
	pt::write_xml(ofstr, node,
		pt::xml_writer_make_settings('\t', 1, std::string{"utf-8"}));
}


/**
 * save the plot as pdf
 */
void MagDynDlg::SavePlotFigure()
{
	if(!m_plot)
		return;

	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getSaveFileName(
		this, "Save Figure", dirLast, "PDf Files (*.pdf *.PDF)");
	if(filename=="")
		return;
	m_sett->setValue("dir", QFileInfo(filename).path());

	m_plot->savePdf(filename);
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


/**
 * calculate the dispersion branches
 */
void MagDynDlg::CalcDispersion()
{
	if(m_ignoreCalc)
		return;

	m_plot->clearPlottables();

	// nothing to calculate?
	if(m_dyn.GetAtomSites().size()==0 || m_dyn.GetExchangeTerms().size()==0)
	{
		m_plot->replot();
		return;
	}

	const t_real Q_start[]
	{
		m_q_start[0]->value(),
		m_q_start[1]->value(),
		m_q_start[2]->value(),
	};

	const t_real Q_end[]
	{
		m_q_end[0]->value(),
		m_q_end[1]->value(),
		m_q_end[2]->value(),
	};

	const t_real Q_range[]
	{
		std::abs(Q_end[0] - Q_start[0]),
		std::abs(Q_end[1] - Q_start[1]),
		std::abs(Q_end[2] - Q_start[2]),
	};

	// Q component with maximum range
	t_size Q_idx = 0;
	if(Q_range[1] > Q_range[Q_idx])
		Q_idx = 1;
	if(Q_range[2] > Q_range[Q_idx])
		Q_idx = 2;

	t_size num_pts = m_num_points->value();

	QVector<t_real> qs_data, Es_data, ws_data;
	qs_data.reserve(num_pts*10);
	Es_data.reserve(num_pts*10);
	ws_data.reserve(num_pts*10);

	t_real weight_scale = m_weight_scale->value();;

	bool use_goldstone = false;
	t_real E0 = use_goldstone ? m_dyn.GetGoldstoneEnergy() : 0.;

	bool only_energies = !m_use_weights->isChecked();
	bool use_projector = m_use_projector->isChecked();

	for(t_size i=0; i<num_pts; ++i)
	{
		t_real Q[]
		{
			std::lerp(Q_start[0], Q_end[0], t_real(i)/t_real(num_pts-1)),
			std::lerp(Q_start[1], Q_end[1], t_real(i)/t_real(num_pts-1)),
			std::lerp(Q_start[2], Q_end[2], t_real(i)/t_real(num_pts-1)),
		};

		auto energies_and_correlations = m_dyn.GetEnergies(Q[0], Q[1], Q[2],
			!m_use_weights->isChecked());

		for(const auto& E_and_S : energies_and_correlations)
		{
			t_real E = E_and_S.E - E0;
			if(std::isnan(E) || std::isinf(E))
				continue;

			qs_data.push_back(Q[Q_idx]);
			Es_data.push_back(E);

			// weights
			if(!only_energies)
			{
				const t_mat& S = E_and_S.S;
				t_real weight = E_and_S.weight;

				if(!use_projector)
					weight = tl2::trace<t_mat>(S).real();

				if(std::isnan(weight) || std::isinf(weight))
					weight = 0.;
				ws_data.push_back(weight * weight_scale);
			}
		}
	}

	//m_plot->addGraph();
	GraphWithWeights *graph = new GraphWithWeights(
		m_plot->xAxis, m_plot->yAxis);
	QPen pen = graph->pen();
	pen.setColor(QColor(0xff, 0x00, 0x00));
	pen.setWidthF(1.);
	graph->setPen(pen);
	graph->setBrush(QBrush(pen.color(), Qt::SolidPattern));
	graph->setLineStyle(QCPGraph::lsNone);
	graph->setScatterStyle(QCPScatterStyle(
		QCPScatterStyle::ssDisc, weight_scale));
	graph->setAntialiased(true);
	graph->setData(qs_data, Es_data, true /*already sorted*/);
	graph->SetWeights(ws_data);

	auto [min_E_iter, max_E_iter] =
		std::minmax_element(Es_data.begin(), Es_data.end());

	const char* Q_label[]{ "h (rlu)", "k (rlu)", "l (rlu)" };
	m_plot->xAxis->setLabel(Q_label[Q_idx]);
	m_plot->xAxis->setRange(Q_start[Q_idx], Q_end[Q_idx]);
	if(min_E_iter != Es_data.end() && max_E_iter != Es_data.end())
	{
		t_real E_range = *max_E_iter - *min_E_iter;
		m_plot->yAxis->setRange(*min_E_iter - E_range*0.05, *max_E_iter + E_range*0.05);
	}
	else
	{
		m_plot->yAxis->setRange(0., 1.);
	}

	m_plot->replot();
}


/**
 * calculate the hamiltonian for a single Q value
 */
void MagDynDlg::CalcHamiltonian()
{
	if(m_ignoreCalc)
		return;

	m_hamiltonian->clear();

	const t_real Q[]
	{
		m_q[0]->value(),
		m_q[1]->value(),
		m_q[2]->value(),
	};

	std::ostringstream ostr;
	ostr.precision(g_prec_gui);

	// get hamiltonian
	t_mat H = m_dyn.GetHamiltonian(Q[0], Q[1], Q[2]);

	ostr << "<p><h3>Hamiltonian</h3>";
	ostr << "<table style=\"border:0px\">";

	for(std::size_t i=0; i<H.size1(); ++i)
	{
		ostr << "<tr>";
		for(std::size_t j=0; j<H.size2(); ++j)
		{
			t_cplx elem = H(i, j);
			tl2::set_eps_0<t_cplx, t_real>(elem, g_eps);
			ostr << "<td style=\"padding-right:8px\">"
				<< elem << "</td>";
		}
		ostr << "</tr>";
	}
	ostr << "</table></p>";


	// get energies and correlation functions
	bool only_energies = !m_use_weights->isChecked();
	bool use_projector = m_use_projector->isChecked();

	auto energies_and_correlations = m_dyn.GetEnergies(H, Q[0], Q[1], Q[2], only_energies);
	using t_E_and_S = typename decltype(energies_and_correlations)::value_type;

	if(only_energies)
	{
		// split into positive and negative energies
		std::vector<EnergyAndWeight> Es_neg, Es_pos;
		for(const t_E_and_S& E_and_S : energies_and_correlations)
		{
			t_real E = E_and_S.E;

			if(E < 0.)
				Es_neg.push_back(E_and_S);
			else
				Es_pos.push_back(E_and_S);
		}

		std::stable_sort(Es_neg.begin(), Es_neg.end(),
			[](const t_E_and_S& E_and_S_1, const t_E_and_S& E_and_S_2) -> bool
		{
			t_real E1 = E_and_S_1.E;
			t_real E2 = E_and_S_2.E;
			return std::abs(E1) < std::abs(E2);
		});

		std::stable_sort(Es_pos.begin(), Es_pos.end(),
			[](const t_E_and_S& E_and_S_1, const t_E_and_S& E_and_S_2) -> bool
		{
			t_real E1 = E_and_S_1.E;
			t_real E2 = E_and_S_2.E;
			return std::abs(E1) < std::abs(E2);
		});

		ostr << "<p><h3>Energies</h3>";
		ostr << "<table style=\"border:0px\">";
		ostr << "<tr>";
		ostr << "<th style=\"padding-right:8px\">Creation</th>";
		for(const t_E_and_S& E_and_S : Es_pos)
		{
			t_real E = E_and_S.E;
			tl2::set_eps_0(E);

			ostr << "<td style=\"padding-right:8px\">"
				<< E << " meV" << "</td>";
		}
		ostr << "</tr>";

		ostr << "<tr>";
		ostr << "<th style=\"padding-right:8px\">Annihilation</th>";
		for(const t_E_and_S& E_and_S : Es_neg)
		{
			t_real E = E_and_S.E;
			tl2::set_eps_0(E);

			ostr << "<td style=\"padding-right:8px\">"
				<< E << " meV" << "</td>";
		}
		ostr << "</tr>";
		ostr << "</table></p>";
	}
	else
	{
		std::stable_sort(energies_and_correlations.begin(), energies_and_correlations.end(),
			[](const t_E_and_S& E_and_S_1, const t_E_and_S& E_and_S_2) -> bool
		{
			t_real E1 = E_and_S_1.E;
			t_real E2 = E_and_S_2.E;
			return E1 < E2;
		});

		ostr << "<p><h3>Spectrum</h3>";
		ostr << "<table style=\"border:0px\">";
		ostr << "<tr>";
		ostr << "<th style=\"padding-right:16px\">Energy E</td>";
		ostr << "<th style=\"padding-right:16px\">Correlation S(Q, E)</td>";
		ostr << "<th style=\"padding-right:16px\">Neutron S⟂(Q, E)</td>";
		ostr << "<th style=\"padding-right:16px\">Weight</td>";
		ostr << "</tr>";
		for(const t_E_and_S& E_and_S : energies_and_correlations)
		{
			ostr << "<tr>";
			t_real E = E_and_S.E;
			const t_mat& S = E_and_S.S;
			const t_mat& S_perp = E_and_S.S_perp;
			t_real weight = E_and_S.weight;
			if(!use_projector)
				weight = tl2::trace<t_mat>(S).real();

			tl2::set_eps_0(E);
			tl2::set_eps_0(weight);

			// E
			ostr << "<td style=\"padding-right:16px\">"
				<< E << " meV" << "</td>";

			// S(Q, E)
			ostr << "<td style=\"padding-right:16px\">";
			ostr << "<table style=\"border:0px\">";
			for(std::size_t i=0; i<S.size1(); ++i)
			{
				ostr << "<tr>";
				for(std::size_t j=0; j<S.size2(); ++j)
				{
					t_cplx elem = S(i, j);
					tl2::set_eps_0<t_cplx, t_real>(elem, g_eps);
					ostr << "<td style=\"padding-right:8px\">"
						<< elem << "</td>";
				}
				ostr << "</tr>";
			}
			ostr << "</table>";
			ostr << "</td>";

			// S_perp(Q, E)
			ostr << "<td style=\"padding-right:16px\">";
			ostr << "<table style=\"border:0px\">";
			for(std::size_t i=0; i<S_perp.size1(); ++i)
			{
				ostr << "<tr>";
				for(std::size_t j=0; j<S_perp.size2(); ++j)
				{
					t_cplx elem = S_perp(i, j);
					tl2::set_eps_0<t_cplx, t_real>(elem, g_eps);
					ostr << "<td style=\"padding-right:8px\">"
						<< elem << "</td>";
				}
				ostr << "</tr>";
			}
			ostr << "</table>";
			ostr << "</td>";

			// tr(S_perp(Q, E))
			ostr << "<td style=\"padding-right:16px\">" << weight << "</td>";

			ostr << "</tr>";
		}
		ostr << "</table></p>";
	}

	m_hamiltonian->setHtml(ostr.str().c_str());
}


/**
 * mouse move event of the plot
 */
void MagDynDlg::PlotMouseMove(QMouseEvent* evt)
{
	if(!m_status)
		return;

	t_real Q = m_plot->xAxis->pixelToCoord(evt->pos().x());
	t_real E = m_plot->yAxis->pixelToCoord(evt->pos().y());

	QString status("Q = %1 rlu, E = %2 meV.");
	status = status.arg(Q, 0, 'g', g_prec_gui).arg(E, 0, 'g', g_prec_gui);
	m_status->setText(status);
}


void MagDynDlg::closeEvent(QCloseEvent *)
{
	if(!m_sett)
		return;

	m_sett->setValue("geo", saveGeometry());

	if(m_structplot_dlg)
		m_sett->setValue("geo_struct_view", m_structplot_dlg->saveGeometry());
}


/**
 * show the 3d view of the atomic structure
 */
void MagDynDlg::ShowStructurePlot()
{
	// plot widget
	if(!m_structplot_dlg)
	{
		m_structplot_dlg = new QDialog(this);
		m_structplot_dlg->setWindowTitle("Unit Cell");

		m_structplot = new tl2::GlPlot(this);
		m_structplot->GetRenderer()->SetLight(
			0, tl2::create<t_vec3_gl>({ 5, 5, 5 }));
		m_structplot->GetRenderer()->SetLight(
			1, tl2::create<t_vec3_gl>({ -5, -5, -5 }));
		m_structplot->GetRenderer()->SetCoordMax(1.);
		m_structplot->GetRenderer()->GetCamera().SetDist(1.5);
		m_structplot->GetRenderer()->GetCamera().UpdateTransformation();
		m_structplot->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Expanding});

		m_structplot_status = new QLabel(this);

		auto grid = new QGridLayout(m_structplot_dlg);
		grid->setSpacing(2);
		grid->setContentsMargins(4,4,4,4);
		grid->addWidget(m_structplot, 0,0,1,2);
		grid->addWidget(m_structplot_status, 1,0,1,2);

		connect(m_structplot, &tl2::GlPlot::AfterGLInitialisation,
			this, &MagDynDlg::StructPlotAfterGLInitialisation);
		connect(m_structplot->GetRenderer(), &tl2::GlPlotRenderer::PickerIntersection,
			this, &MagDynDlg::StructPlotPickerIntersection);
		connect(m_structplot, &tl2::GlPlot::MouseDown,
			this, &MagDynDlg::StructPlotMouseDown);
		connect(m_structplot, &tl2::GlPlot::MouseUp,
			this, &MagDynDlg::StructPlotMouseUp);

		if(m_sett && m_sett->contains("geo_struct_view"))
			m_structplot_dlg->restoreGeometry(
				m_sett->value("geo_struct_view").toByteArray());
		else
			m_structplot_dlg->resize(500, 500);
	}

	m_structplot_dlg->show();
	m_structplot_dlg->raise();
	m_structplot_dlg->focusWidget();
}


/**
 * structure plot picker intersection
 */
void MagDynDlg::StructPlotPickerIntersection(
        const t_vec3_gl* pos, std::size_t objIdx,
        [[maybe_unused]] const t_vec3_gl* posSphere)
{
	m_structplot_status->setText("");
	if(!pos)
		return;

	auto iter_atoms = m_structplot_atoms.find(objIdx);
	if(iter_atoms == m_structplot_atoms.end())
		return;

	const std::string& ident = iter_atoms->second->name;

	// TODO
	m_structplot_status->setText(ident.c_str());
}


/**
 * structure plot mouse button pressed
 */
void MagDynDlg::StructPlotMouseDown(
	[[maybe_unused]] bool left,
	[[maybe_unused]] bool mid,
	[[maybe_unused]] bool right)
{
}


/**
 * structure plot mouse button released
 */
void MagDynDlg::StructPlotMouseUp(
	[[maybe_unused]] bool left,
	[[maybe_unused]] bool mid,
	[[maybe_unused]] bool right)
{
}


/**
 * after structure plot initialisation
 */
void MagDynDlg::StructPlotAfterGLInitialisation()
{
	if(!m_structplot)
		return;

	// reference sphere for linked objects
	m_structplot_sphere = m_structplot->GetRenderer()->AddSphere(
		0.05, 0.,0.,0., 1.,1.,1.,1.);
	m_structplot->GetRenderer()->SetObjectVisible(
		m_structplot_sphere, false);

	// reference arrow for linked objects
	m_structplot_arrow = m_structplot->GetRenderer()->AddArrow(
		0.015, 0.25, 0.,0.,0.5,  1.,1.,1.,1.);
	m_structplot->GetRenderer()->SetObjectVisible(
		m_structplot_arrow, false);

	// GL device info
	auto [strGlVer, strGlShaderVer, strGlVendor, strGlRenderer]
		= m_structplot->GetRenderer()->GetGlDescr();
	m_labelGlInfos[0]->setText(
		QString("GL Version: %1.").arg(strGlVer.c_str()));
	m_labelGlInfos[1]->setText(
		QString("GL Shader Version: %1.").arg(strGlShaderVer.c_str()));
	m_labelGlInfos[2]->setText(
		QString("GL Vendor: %1.").arg(strGlVendor.c_str()));
	m_labelGlInfos[3]->setText(
		QString("GL Device: %1.").arg(strGlRenderer.c_str()));

	StructPlotSync();
}


/**
 * get the sites and exchange terms and
 * transfer them to the structure plotter
 */
void MagDynDlg::StructPlotSync()
{
	if(!m_structplot)
		return;

	const auto& sites = m_dyn.GetAtomSites();


	// clear old atoms
	for(const auto& [atom_idx, atom_site] : m_structplot_atoms)
	{
		m_structplot->GetRenderer()->RemoveObject(atom_idx);
	}

	m_structplot_atoms.clear();


	// add atoms
	for(const auto& site : sites)
	{
		t_real_gl rgb[3]{1., 0., 0.};
		t_real_gl scale = 1.;

		std::size_t obj = m_structplot->GetRenderer()->AddLinkedObject(
			m_structplot_sphere, 0,0,0, rgb[0], rgb[1], rgb[2], 1);

		std::size_t arrow = m_structplot->GetRenderer()->AddLinkedObject(
			m_structplot_arrow, 0,0,0, rgb[0], rgb[1], rgb[2], 1);

		m_structplot_atoms.insert(std::make_pair(obj, &site));
		m_structplot_atoms.insert(std::make_pair(arrow, &site));

		t_vec_gl pos_vec = tl2::create<t_vec_gl>({
			t_real_gl(site.pos[0].real()),
			t_real_gl(site.pos[1].real()),
			t_real_gl(site.pos[2].real()),
		});

		t_vec_gl spin_vec = tl2::create<t_vec_gl>({
			t_real_gl(site.spin_dir[0].real() * site.spin_mag),
			t_real_gl(site.spin_dir[1].real() * site.spin_mag),
			t_real_gl(site.spin_dir[2].real() * site.spin_mag),
		});

		m_structplot->GetRenderer()->SetObjectMatrix(obj,
			tl2::hom_translation<t_mat_gl>(
				pos_vec[0], pos_vec[1], pos_vec[2]) *
			tl2::hom_scaling<t_mat_gl>(scale, scale, scale));

		m_structplot->GetRenderer()->SetObjectMatrix(arrow,
			tl2::get_arrow_matrix<t_vec_gl, t_mat_gl, t_real_gl>(
				spin_vec,                          // to
				1,                                 // post-scale
				tl2::create<t_vec_gl>({0, 0, 0}),  // post-translate
				tl2::create<t_vec_gl>({0, 0, 1}),  // from
				scale,                             // pre-scale
				pos_vec));                         // pre-translate

		m_structplot->GetRenderer()->SetObjectLabel(obj, site.name);
		m_structplot->GetRenderer()->SetObjectLabel(arrow, site.name);
	}

	m_structplot->update();
}
