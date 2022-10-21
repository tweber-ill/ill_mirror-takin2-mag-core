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

#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>

#include <iostream>
#include <boost/scope_exit.hpp>


t_real g_eps = 1e-6;
int g_prec = 6;
int g_prec_gui = 3;



MagDynDlg::MagDynDlg(QWidget* pParent) : QDialog{pParent},
	m_sett{new QSettings{"takin", "magdyn"}}
{
	m_dyn.SetEpsilon(g_eps);
	m_dyn.SetPrecision(g_prec);

	setWindowTitle("Magnon Dynamics");
	setSizeGripEnabled(true);

	m_tabs = new QTabWidget(this);

	// status
	m_status = new QLabel(this);
	m_status->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	m_status->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	// progress bar
	m_progress = new QProgressBar(this);
	m_progress->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	// stop button
	QPushButton* btnStop = new QPushButton(QIcon::fromTheme("process-stop"), "Stop", this);
	btnStop->setToolTip("Request stop to ongoing calculation.");
	btnStop->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	// main grid
	m_maingrid = new QGridLayout(this);
	m_maingrid->setSpacing(4);
	m_maingrid->setContentsMargins(6, 6, 6, 6);
	m_maingrid->addWidget(m_tabs, 0,0, 1,3);
	m_maingrid->addWidget(m_status, 1,0, 1,1);
	m_maingrid->addWidget(m_progress, 1,1, 1,1);
	m_maingrid->addWidget(btnStop, 1,2, 1,1);

	// create tab panels
	CreateSitesPanel();
	CreateExchangeTermsPanel();
	CreateVariablesPanel();
	CreateSampleEnvPanel();
	CreateDispersionPanel();
	CreateHamiltonPanel();
	CreateExportPanel();
	CreateInfoDlg();
	CreateMenuBar();

	// signals
	connect(btnStop, &QAbstractButton::clicked, [this]() { m_stopRequested = true; });

	if(m_sett)
	{
		// restory window size and position
		if(m_sett->contains("geo"))
			restoreGeometry(m_sett->value("geo").toByteArray());
		else
			resize(600, 600);

		if(m_sett->contains("recent_files"))
			m_recent.SetRecentFiles(m_sett->value("recent_files").toStringList());
	}

	m_ignoreTableChanges = false;
}


MagDynDlg::~MagDynDlg()
{
	Clear();

	if(m_structplot_dlg)
	{
		delete m_structplot_dlg;
		m_structplot_dlg = nullptr;
	}

	if(m_info_dlg)
	{
		delete m_info_dlg;
		m_info_dlg = nullptr;
	}
}


void MagDynDlg::Clear()
{
	m_ignoreCalc = true;

	// clear old tables
	DelTabItem(m_sitestab, -1);
	DelTabItem(m_termstab, -1);
	DelTabItem(m_varstab, -1);
	DelTabItem(m_fieldstab, -1);

	m_plot->clearPlottables();
	m_plot->replot();
	m_hamiltonian->clear();

	m_dyn.Clear();

	StructPlotSync();

	// set some defaults
	m_comboSG->setCurrentIndex(0);

	m_ordering[0]->setValue(0.);
	m_ordering[1]->setValue(0.);
	m_ordering[2]->setValue(0.);

	m_normaxis[0]->setValue(1.);
	m_normaxis[1]->setValue(0.);
	m_normaxis[2]->setValue(0.);

	m_weight_scale->setValue(1.);
	m_weight_min->setValue(0.);
	m_weight_max->setValue(9999.);

	m_ignoreCalc = false;
}


/**
 * rotate the direction of the magnetic field
 */
void MagDynDlg::RotateField(bool ccw)
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
	if(!ccw)
		angle = -angle;

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

	if(m_autocalc->isChecked())
		SyncSitesAndTerms();
};


/**
 * generate atom sites form the space group symmetries
 */
void MagDynDlg::GenerateSitesFromSG()
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
			t_real, t_real, t_real,                // position
			std::string, std::string, std::string, // spin direction
			t_real                                 // spin magnitude
			>> generatedsites;

		// iterate existing sites
		int orgRowCnt = m_sitestab->rowCount();
		for(int row=0; row<orgRowCnt; ++row)
		{
			std::string ident = m_sitestab->item(row, COL_SITE_NAME)
				->text().toStdString();
			t_real x = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
				m_sitestab->item(row, COL_SITE_POS_X))->GetValue();
			t_real y = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
				m_sitestab->item(row, COL_SITE_POS_Y))->GetValue();
			t_real z = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
				m_sitestab->item(row, COL_SITE_POS_Z))->GetValue();
			std::string sx = m_sitestab->item(row, COL_SITE_SPIN_X)->text().toStdString();
			std::string sy = m_sitestab->item(row, COL_SITE_SPIN_Y)->text().toStdString();
			std::string sz = m_sitestab->item(row, COL_SITE_SPIN_Z)->text().toStdString();
			t_real S = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
				m_sitestab->item(row, COL_SITE_SPIN_MAG))->GetValue();

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
		QMessageBox::critical(this, "Magnon Dynamics", ex.what());
	}

	m_ignoreCalc = 0;
	SyncSitesAndTerms();
	CalcAll();
}


/**
 * generate exchange terms from space group symmetries
 */
void MagDynDlg::GenerateCouplingsFromSG()
{
	m_ignoreCalc = 1;

	try
	{
		// symops of current space group
		auto sgidx = m_comboSG2->itemData(m_comboSG2->currentIndex()).toInt();
		if(sgidx < 0 || std::size_t(sgidx) >= m_SGops.size())
		{
			QMessageBox::critical(this, "Magnon Dynamics",
				"Invalid space group selected.");
			m_ignoreCalc = 0;
			return;
		}

		std::vector<std::tuple<
			std::string,                           // ident
			t_size, t_size,                        // uc atom indices
			t_real, t_real, t_real,                // supercell vector
			std::string,                           // exchange term (not modified)
			std::string, std::string, std::string  // dmi vector
			>> generatedcouplings;


		const auto& sites = m_dyn.GetAtomSites();
		const auto& ops = m_SGops[sgidx];

		// get all site positions
		std::vector<t_vec_real> allsites;
		allsites.reserve(sites.size());
		for(const auto& site: sites)
			allsites.push_back(tl2::create<t_vec_real>({
				site.pos[0], site.pos[1], site.pos[2], 1. }));

		// iterate existing coupling terms
		for(int row=0; row<m_termstab->rowCount(); ++row)
		{
			std::string ident = m_sitestab->item(row, COL_XCH_NAME)->text().toStdString();
			t_size atom_1_idx = static_cast<tl2::NumericTableWidgetItem<t_size>*>(
				m_termstab->item(row, COL_XCH_ATOM1_IDX))->GetValue();
			t_size atom_2_idx = static_cast<tl2::NumericTableWidgetItem<t_size>*>(
				m_termstab->item(row, COL_XCH_ATOM2_IDX))->GetValue();
			t_real sc_x = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
				m_termstab->item(row, COL_XCH_DIST_X))->GetValue();
			t_real sc_y = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
				m_termstab->item(row, COL_XCH_DIST_Y))->GetValue();
			t_real sc_z = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
				m_termstab->item(row, COL_XCH_DIST_Z))->GetValue();
			t_real dmi_x = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
				m_termstab->item(row, COL_XCH_DMI_X))->GetValue();
			t_real dmi_y = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
				m_termstab->item(row, COL_XCH_DMI_Y))->GetValue();
			t_real dmi_z = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
				m_termstab->item(row, COL_XCH_DMI_Z))->GetValue();
			std::string oldJ = m_termstab->item(row, COL_XCH_INTERACTION)->text().toStdString();

			// atom positions in unit cell
			const t_vec_real& site1 = allsites[atom_1_idx];
			t_vec_real site2 = allsites[atom_2_idx];

			// position in super cell
			site2 += tl2::create<t_vec_real>({ sc_x, sc_y, sc_z, 0. });

			// generate sites
			auto newsites1 = tl2::apply_ops_hom<t_vec_real, t_mat_real, t_real>(
				site1, ops, g_eps, false, true, true);
			auto newsites2 = tl2::apply_ops_hom<t_vec_real, t_mat_real, t_real>(
				site2, ops, g_eps, false, true, true);

			// generate dmi vectors
			t_vec_real dmi = tl2::create<t_vec_real>({dmi_x, dmi_y, dmi_z, 0});
			auto newdmis = tl2::apply_ops_hom<t_vec_real, t_mat_real, t_real>(
				dmi, ops, g_eps, false, true);

			for(std::size_t op_idx=0; op_idx<newsites1.size(); ++op_idx)
			{
				const t_vec_real& newsite1 = newsites1[op_idx];
				const t_vec_real& newsite2 = newsites2[op_idx];
				const t_vec_real& newdmi = newdmis[op_idx];

				auto [sc1_ok, newsite1_idx, sc1] = tl2::get_supercell(newsite1, allsites, 3, g_eps);
				auto [sc2_ok, newsite2_idx, sc2] = tl2::get_supercell(newsite2, allsites, 3, g_eps);
				t_vec_real sc_dist = sc2 - sc1;

				if(!sc1_ok || !sc2_ok)
				{
					std::cerr << "Could not find supercell for position generated from symop "
						<< op_idx << "." << std::endl;
				}

				generatedcouplings.emplace_back(std::make_tuple(
					ident, newsite1_idx, newsite2_idx,
					sc_dist[0], sc_dist[1], sc_dist[2], oldJ,
					tl2::var_to_str(newdmi[0]), tl2::var_to_str(newdmi[1]), tl2::var_to_str(newdmi[2])));
			}
		}

		// remove original couplings
		DelTabItem(m_termstab, -1);

		// add new couplings
		for(const auto& coupling : generatedcouplings)
		{
			std::apply(&MagDynDlg::AddTermTabItem,
				std::tuple_cat(std::make_tuple(this, -1), coupling));
		}
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Magnon Dynamics", ex.what());
	}

	m_ignoreCalc = 0;
	SyncSitesAndTerms();
	CalcAll();
}


/**
 * add an atom site
 */
void MagDynDlg::AddSiteTabItem(int row,
	const std::string& name,
	t_real x, t_real y, t_real z,
	const std::string& sx,
	const std::string& sy,
	const std::string& sz,
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
			new tl2::NumericTableWidgetItem<t_real>(x));
		m_sitestab->setItem(row, COL_SITE_POS_Y,
			new tl2::NumericTableWidgetItem<t_real>(y));
		m_sitestab->setItem(row, COL_SITE_POS_Z,
			new tl2::NumericTableWidgetItem<t_real>(z));
		m_sitestab->setItem(row, COL_SITE_SPIN_X,
			new tl2::NumericTableWidgetItem<t_real>(sx));
		m_sitestab->setItem(row, COL_SITE_SPIN_Y,
			new tl2::NumericTableWidgetItem<t_real>(sy));
		m_sitestab->setItem(row, COL_SITE_SPIN_Z,
			new tl2::NumericTableWidgetItem<t_real>(sz));
		m_sitestab->setItem(row, COL_SITE_SPIN_MAG,
			new tl2::NumericTableWidgetItem<t_real>(S));
	}

	m_sitestab->scrollToItem(m_sitestab->item(row, 0));
	m_sitestab->setCurrentCell(row, 0);

	m_sitestab->setSortingEnabled(/*sorting*/ true);

	UpdateVerticalHeader(m_sitestab);

	m_ignoreTableChanges = 0;
	if(m_autocalc->isChecked())
		SyncSitesAndTerms();
}


/**
 * add an exchange term
 */
void MagDynDlg::AddTermTabItem(int row,
	const std::string& name,
	t_size atom_1, t_size atom_2,
	t_real dist_x, t_real dist_y, t_real dist_z,
	const std::string& J,
	const std::string& dmi_x,
	const std::string& dmi_y,
	const std::string& dmi_z)
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
			new tl2::NumericTableWidgetItem<t_size>(atom_1));
		m_termstab->setItem(row, COL_XCH_ATOM2_IDX,
			new tl2::NumericTableWidgetItem<t_size>(atom_2));
		m_termstab->setItem(row, COL_XCH_DIST_X,
			new tl2::NumericTableWidgetItem<t_real>(dist_x));
		m_termstab->setItem(row, COL_XCH_DIST_Y,
			new tl2::NumericTableWidgetItem<t_real>(dist_y));
		m_termstab->setItem(row, COL_XCH_DIST_Z,
			new tl2::NumericTableWidgetItem<t_real>(dist_z));
		m_termstab->setItem(row, COL_XCH_INTERACTION,
			new tl2::NumericTableWidgetItem<t_real>(J));
		m_termstab->setItem(row, COL_XCH_DMI_X,
			new tl2::NumericTableWidgetItem<t_real>(dmi_x));
		m_termstab->setItem(row, COL_XCH_DMI_Y,
			new tl2::NumericTableWidgetItem<t_real>(dmi_y));
		m_termstab->setItem(row, COL_XCH_DMI_Z,
			new tl2::NumericTableWidgetItem<t_real>(dmi_z));
	}

	m_termstab->scrollToItem(m_termstab->item(row, 0));
	m_termstab->setCurrentCell(row, 0);

	m_termstab->setSortingEnabled(/*sorting*/ true);

	UpdateVerticalHeader(m_termstab);

	m_ignoreTableChanges = 0;
	if(m_autocalc->isChecked())
		SyncSitesAndTerms();
}


/**
 * add a variable
 */
void MagDynDlg::AddVariableTabItem(int row,
	const std::string& name, const t_cplx& value)
{
	bool bclone = 0;
	m_ignoreTableChanges = 1;

	if(row == -1)	// append to end of table
		row = m_varstab->rowCount();
	else if(row == -2 && m_variables_cursor_row >= 0)	// use row from member variable
		row = m_variables_cursor_row;
	else if(row == -3 && m_variables_cursor_row >= 0)	// use row from member variable +1
		row = m_variables_cursor_row + 1;
	else if(row == -4 && m_variables_cursor_row >= 0)	// use row from member variable +1
	{
		row = m_variables_cursor_row + 1;
		bclone = 1;
	}

	m_varstab->setSortingEnabled(false);
	m_varstab->insertRow(row);

	if(bclone)
	{
		for(int thecol=0; thecol<NUM_VARS_COLS; ++thecol)
		{
			m_varstab->setItem(row, thecol,
				m_varstab->item(m_variables_cursor_row, thecol)->clone());
		}
	}
	else
	{
		m_varstab->setItem(row, COL_VARS_NAME,
			new QTableWidgetItem(name.c_str()));
		m_varstab->setItem(row, COL_VARS_VALUE_REAL,
			new tl2::NumericTableWidgetItem<t_real>(value.real()));
		m_varstab->setItem(row, COL_VARS_VALUE_IMAG,
			new tl2::NumericTableWidgetItem<t_real>(value.imag()));
	}

	m_varstab->scrollToItem(m_varstab->item(row, 0));
	m_varstab->setCurrentCell(row, 0);

	m_varstab->setSortingEnabled(/*sorting*/ true);

	UpdateVerticalHeader(m_varstab);

	m_ignoreTableChanges = 0;
	if(m_autocalc->isChecked())
		SyncSitesAndTerms();
}


/**
 * add a magnetic field
 */
void MagDynDlg::AddFieldTabItem(int row,
	t_real Bh, t_real Bk, t_real Bl,
	t_real Bmag)
{
	bool bclone = 0;

	if(row == -1)	// append to end of table
		row = m_fieldstab->rowCount();
	else if(row == -2 && m_fields_cursor_row >= 0)	// use row from member variable
		row = m_fields_cursor_row;
	else if(row == -3 && m_fields_cursor_row >= 0)	// use row from member variable +1
		row = m_fields_cursor_row + 1;
	else if(row == -4 && m_fields_cursor_row >= 0)	// use row from member variable +1
	{
		row = m_fields_cursor_row + 1;
		bclone = 1;
	}

	m_fieldstab->setSortingEnabled(false);
	m_fieldstab->insertRow(row);

	if(bclone)
	{
		for(int thecol=0; thecol<NUM_FIELD_COLS; ++thecol)
		{
			m_fieldstab->setItem(row, thecol,
				m_fieldstab->item(m_fields_cursor_row, thecol)->clone());
		}
	}
	else
	{
		m_fieldstab->setItem(row, COL_FIELD_H,
			new tl2::NumericTableWidgetItem<t_real>(Bh));
		m_fieldstab->setItem(row, COL_FIELD_K,
			new tl2::NumericTableWidgetItem<t_real>(Bk));
		m_fieldstab->setItem(row, COL_FIELD_L,
			new tl2::NumericTableWidgetItem<t_real>(Bl));
		m_fieldstab->setItem(row, COL_FIELD_MAG,
			new tl2::NumericTableWidgetItem<t_real>(Bmag));
	}

	m_fieldstab->scrollToItem(m_fieldstab->item(row, 0));
	m_fieldstab->setCurrentCell(row, 0);

	m_fieldstab->setSortingEnabled(/*sorting*/ true);

	UpdateVerticalHeader(m_fieldstab);
}


/**
 * set selected field as current
 */
void MagDynDlg::SetCurrentField()
{
	if(m_fields_cursor_row < 0 || m_fields_cursor_row >= m_fieldstab->rowCount())
		return;

	const auto* Bh = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
		m_fieldstab->item(m_fields_cursor_row, COL_FIELD_H));
	const auto* Bk = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
		m_fieldstab->item(m_fields_cursor_row, COL_FIELD_K));
	const auto* Bl = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
		m_fieldstab->item(m_fields_cursor_row, COL_FIELD_L));
	const auto* Bmag = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
		m_fieldstab->item(m_fields_cursor_row, COL_FIELD_MAG));

	if(!Bh || !Bk || !Bl || !Bmag)
		return;

	m_field_dir[0]->setValue(Bh->GetValue());
	m_field_dir[1]->setValue(Bk->GetValue());
	m_field_dir[2]->setValue(Bl->GetValue());
	m_field_mag->setValue(Bmag->GetValue());
}


/**
 * delete table widget items
 */
void MagDynDlg::DelTabItem(QTableWidget *pTab, int begin, int end)
{
	bool needs_recalc = true;
	if(pTab == m_fieldstab)
		needs_recalc = false;

	if(needs_recalc)
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

	UpdateVerticalHeader(pTab);

	if(needs_recalc)
	{
		m_ignoreTableChanges = 0;
		if(m_autocalc->isChecked())
			SyncSitesAndTerms();
	}
}


void MagDynDlg::MoveTabItemUp(QTableWidget *pTab)
{
	bool needs_recalc = true;
	if(pTab == m_fieldstab)
		needs_recalc = false;

	if(needs_recalc)
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

	UpdateVerticalHeader(pTab);

	if(needs_recalc)
	{
		m_ignoreTableChanges = 0;
		if(m_autocalc->isChecked())
			SyncSitesAndTerms();
	}
}


void MagDynDlg::MoveTabItemDown(QTableWidget *pTab)
{
	bool needs_recalc = true;
	if(pTab == m_fieldstab)
		needs_recalc = false;

	if(needs_recalc)
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

	UpdateVerticalHeader(pTab);

	if(needs_recalc)
	{
		m_ignoreTableChanges = 0;
		if(m_autocalc->isChecked())
			SyncSitesAndTerms();
	}
}


/**
 * insert a vertical header column showing the row index
 */
void MagDynDlg::UpdateVerticalHeader(QTableWidget *pTab)
{
	for(int row=0; row<pTab->rowCount(); ++row)
	{
		QTableWidgetItem *item = pTab->verticalHeaderItem(row);
		if(!item)
			item = new QTableWidgetItem{};
		item->setText(QString::number(row));
		pTab->setVerticalHeaderItem(row, item);
	}
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

	if(m_autocalc->isChecked())
		SyncSitesAndTerms();
}


/**
 * item contents changed
 */
void MagDynDlg::TermsTableItemChanged(QTableWidgetItem * /*item*/)
{
	if(m_ignoreTableChanges)
		return;

	if(m_autocalc->isChecked())
		SyncSitesAndTerms();
}


/**
 * item contents changed
 */
void MagDynDlg::VariablesTableItemChanged(QTableWidgetItem * /*item*/)
{
	if(m_ignoreTableChanges)
		return;

	if(m_autocalc->isChecked())
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
		else if(pTab == m_varstab)
			m_variables_cursor_row = item->row();
		else if(pTab == m_fieldstab)
			m_fields_cursor_row = item->row();

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
 * get the sitesm exchange terms, and variables from the table
 * and transfer them to the dynamics calculator
 */
void MagDynDlg::SyncSitesAndTerms()
{
	if(m_ignoreCalc)
		return;

	m_dyn.Clear();

	BOOST_SCOPE_EXIT(this_)
	{
		this_->m_sitestab->blockSignals(false);
		this_->m_termstab->blockSignals(false);
		this_->m_varstab->blockSignals(false);
	} BOOST_SCOPE_EXIT_END
	m_sitestab->blockSignals(true);
	m_termstab->blockSignals(true);
	m_varstab->blockSignals(true);

	// get ordering vector and rotation axis
	{
		t_vec_real ordering = tl2::create<t_vec_real>(
		{
			m_ordering[0]->value(),
			m_ordering[1]->value(),
			m_ordering[2]->value(),
		});

		t_vec_real rotaxis = tl2::create<t_vec_real>(
		{
			m_normaxis[0]->value(),
			m_normaxis[1]->value(),
			m_normaxis[2]->value(),
		});

		m_dyn.SetOrderingWavevector(ordering);
		m_dyn.SetRotationAxis(rotaxis);
	}

	// dmi
	bool use_dmi = m_use_dmi->isChecked();

	// get external field
	if(m_use_field->isChecked())
	{
		ExternalField field;
		field.dir = tl2::create<t_vec_real>(
		{
			m_field_dir[0]->value(),
			m_field_dir[1]->value(),
			m_field_dir[2]->value(),
		});

		field.mag = m_field_mag->value();
		field.align_spins = m_align_spins->isChecked();

		m_dyn.SetExternalField(field);
	}

	// get temperature
	if(m_use_temperature->isChecked())
	{
		t_real temp = m_temperature->value();
		m_dyn.SetTemperature(temp);
	}

	// get variables
	for(int row=0; row<m_varstab->rowCount(); ++row)
	{
		auto *name = m_varstab->item(row, COL_VARS_NAME);
		auto *val_re = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
			m_varstab->item(row, COL_VARS_VALUE_REAL));
		auto *val_im = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
			m_varstab->item(row, COL_VARS_VALUE_IMAG));

		if(!name || !val_re || !val_im)
		{
			std::cerr << "Invalid entry in variables table row "
				<< row << "." << std::endl;
			continue;
		}

		Variable var;
		var.name = name->text().toStdString();
		var.value = val_re->GetValue() + val_im->GetValue() * t_cplx(0, 1);

		m_dyn.AddVariable(std::move(var));
	}

	// get atom sites
	for(int row=0; row<m_sitestab->rowCount(); ++row)
	{
		auto *name = m_sitestab->item(row, COL_SITE_NAME);
		auto *pos_x = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
			m_sitestab->item(row, COL_SITE_POS_X));
		auto *pos_y = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
			m_sitestab->item(row, COL_SITE_POS_Y));
		auto *pos_z = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
			m_sitestab->item(row, COL_SITE_POS_Z));
		auto *spin_x = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
			m_sitestab->item(row, COL_SITE_SPIN_X));
		auto *spin_y = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
			m_sitestab->item(row, COL_SITE_SPIN_Y));
		auto *spin_z = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
			m_sitestab->item(row, COL_SITE_SPIN_Z));
		auto *spin_mag = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
			m_sitestab->item(row, COL_SITE_SPIN_MAG));

		if(!name || !pos_x || !pos_y || !pos_z ||
			!spin_x || !spin_y || !spin_z || !spin_mag)
		{
			std::cerr << "Invalid entry in sites table row "
				<< row << "." << std::endl;
			continue;
		}

		AtomSite site;
		site.name = name->text().toStdString();
		site.g = -2. * tl2::unit<t_mat>(3);

		site.pos = tl2::create<t_vec_real>(
		{
			pos_x->GetValue(),
			pos_y->GetValue(),
			pos_z->GetValue(),
		});

		site.spin_mag = spin_mag->GetValue();
		site.spin_dir[0] = spin_x->text().toStdString();
		site.spin_dir[1] = spin_y->text().toStdString();
		site.spin_dir[2] = spin_z->text().toStdString();

		m_dyn.AddAtomSite(std::move(site));
	}

	m_dyn.CalcAtomSites();
	const auto& sites = m_dyn.GetAtomSites();

	// get exchange terms
	for(int row=0; row<m_termstab->rowCount(); ++row)
	{
		auto *name = m_termstab->item(row, COL_XCH_NAME);
		auto *atom_1_idx = static_cast<tl2::NumericTableWidgetItem<t_size>*>(
			m_termstab->item(row, COL_XCH_ATOM1_IDX));
		auto *atom_2_idx = static_cast<tl2::NumericTableWidgetItem<t_size>*>(
			m_termstab->item(row, COL_XCH_ATOM2_IDX));
		auto *dist_x = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
			m_termstab->item(row, COL_XCH_DIST_X));
		auto *dist_y = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
			m_termstab->item(row, COL_XCH_DIST_Y));
		auto *dist_z = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
			m_termstab->item(row, COL_XCH_DIST_Z));
		auto *interaction = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
			m_termstab->item(row, COL_XCH_INTERACTION));
		auto *dmi_x = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
			m_termstab->item(row, COL_XCH_DMI_X));
		auto *dmi_y = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
			m_termstab->item(row, COL_XCH_DMI_Y));
		auto *dmi_z = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
			m_termstab->item(row, COL_XCH_DMI_Z));

		if(!name || !atom_1_idx || !atom_2_idx ||
			!dist_x || !dist_y || !dist_z ||
			!interaction || !dmi_x || !dmi_y || !dmi_z)
		{
			std::cerr << "Invalid entry in terms table row "
				<< row << "." << std::endl;
			continue;
		}

		ExchangeTerm term;
		term.name = name->text().toStdString();
		term.atom1 = atom_1_idx->GetValue();
		term.atom2 = atom_2_idx->GetValue();
		term.dist = tl2::create<t_vec_real>(
		{
			dist_x->GetValue(),
			dist_y->GetValue(),
			dist_z->GetValue(),
		});

		//term.J = interaction->GetValue();
		term.J = interaction->text().toStdString();

		// atom 1 index out of bounds?
		if(term.atom1 >= sites.size())
		{
			atom_1_idx->setBackground(QBrush(QColor(0xff, 0x00, 0x00)));
			continue;
		}
		else
		{
			QBrush brush = name->background();
			atom_1_idx->setBackground(brush);
		}

		// atom 2 index out of bounds?
		if(term.atom2 >= sites.size())
		{
			atom_2_idx->setBackground(QBrush(QColor(0xff, 0x00, 0x00)));
			continue;
		}
		else
		{
			QBrush brush = name->background();
			atom_2_idx->setBackground(brush);
		}

		if(use_dmi)
		{
			term.dmi[0] = dmi_x->text().toStdString();
			term.dmi[1] = dmi_y->text().toStdString();
			term.dmi[2] = dmi_z->text().toStdString();
		}

		m_dyn.AddExchangeTerm(std::move(term));
	}

	m_dyn.CalcExchangeTerms();
	//m_dyn.CalcIndices();

	CalcAll();
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

	m_recent.TrimEntries();
	m_sett->setValue("recent_files", m_recent.GetRecentFiles());

	m_sett->setValue("geo", saveGeometry());

	if(m_structplot_dlg)
		m_sett->setValue("geo_struct_view", m_structplot_dlg->saveGeometry());
}


/**
 * enable GUI inputs after calculation threads have finished
 */
void MagDynDlg::EnableInput()
{
	m_tabs->setEnabled(true);
	m_menu->setEnabled(true);
}


/**
 * disable GUI inputs for calculation threads
 */
void MagDynDlg::DisableInput()
{
	m_menu->setEnabled(false);
	m_tabs->setEnabled(false);
}
