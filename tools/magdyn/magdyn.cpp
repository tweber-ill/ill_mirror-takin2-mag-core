/**
 * magnon dynamics -- main dialog handler functions
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

#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>

#include <iostream>
#include <boost/scope_exit.hpp>



MagDynDlg::MagDynDlg(QWidget* pParent) : QDialog{pParent},
	m_sett{new QSettings{"takin", "magdyn"}}
{
	m_dyn.SetEpsilon(g_eps);
	m_dyn.SetPrecision(g_prec);

	// create gui
	CreateMainWindow();
	CreateSitesPanel();
	CreateExchangeTermsPanel();
	CreateVariablesPanel();
	CreateSampleEnvPanel();
	CreateDispersionPanel();
	CreateHamiltonPanel();
	CreateExportPanel();
	CreateInfoDlg();
	CreateMenuBar();

	if(m_sett)
	{
		// restore window size and position
		if(m_sett->contains("geo"))
			restoreGeometry(m_sett->value("geo").toByteArray());
		else
			resize(800, 600);

		if(m_sett->contains("recent_files"))
			m_recent.SetRecentFiles(m_sett->value("recent_files").toStringList());

		if(m_sett->contains("splitter"))
			m_split_inout->restoreState(m_sett->value("splitter").toByteArray());
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

	if(m_table_import_dlg)
	{
		delete m_table_import_dlg;
		m_table_import_dlg = nullptr;
	}

	if(m_info_dlg)
	{
		delete m_info_dlg;
		m_info_dlg = nullptr;
	}
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
	bool bclone = false;
	m_ignoreTableChanges = true;
	BOOST_SCOPE_EXIT(this_)
	{
		this_->m_ignoreTableChanges = false;
		if(this_->m_autocalc->isChecked())
			this_->CalcAll();
	} BOOST_SCOPE_EXIT_END

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
	m_ignoreTableChanges = true;
	BOOST_SCOPE_EXIT(this_)
	{
		this_->m_ignoreTableChanges = false;
		if(this_->m_autocalc->isChecked())
			this_->CalcAll();
	} BOOST_SCOPE_EXIT_END

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
}


/**
 * add a variable
 */
void MagDynDlg::AddVariableTabItem(int row,
	const std::string& name, const t_cplx& value)
{
	bool bclone = 0;
	m_ignoreTableChanges = true;
	BOOST_SCOPE_EXIT(this_)
	{
		this_->m_ignoreTableChanges = false;
		if(this_->m_autocalc->isChecked())
			this_->CalcAll();
	} BOOST_SCOPE_EXIT_END

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
 * delete table widget items
 */
void MagDynDlg::DelTabItem(QTableWidget *pTab, int begin, int end)
{
	bool needs_recalc = true;
	if(pTab == m_fieldstab)
		needs_recalc = false;

	if(needs_recalc)
		m_ignoreTableChanges = true;
	BOOST_SCOPE_EXIT(this_, needs_recalc)
	{
		if(needs_recalc)
		{
			this_->m_ignoreTableChanges = false;
			if(this_->m_autocalc->isChecked())
				this_->CalcAll();
		}
	} BOOST_SCOPE_EXIT_END

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
}


void MagDynDlg::MoveTabItemUp(QTableWidget *pTab)
{
	bool needs_recalc = true;
	if(pTab == m_fieldstab)
		needs_recalc = false;

	if(needs_recalc)
		m_ignoreTableChanges = true;
	pTab->setSortingEnabled(false);
	BOOST_SCOPE_EXIT(this_, needs_recalc)
	{
		if(needs_recalc)
		{
			this_->m_ignoreTableChanges = false;
			if(this_->m_autocalc->isChecked())
				this_->CalcAll();
		}
	} BOOST_SCOPE_EXIT_END

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
}


void MagDynDlg::MoveTabItemDown(QTableWidget *pTab)
{
	bool needs_recalc = true;
	if(pTab == m_fieldstab)
		needs_recalc = false;

	if(needs_recalc)
		m_ignoreTableChanges = true;
	pTab->setSortingEnabled(false);
	BOOST_SCOPE_EXIT(this_, needs_recalc)
	{
		if(needs_recalc)
		{
			this_->m_ignoreTableChanges = false;
			if(this_->m_autocalc->isChecked())
				this_->CalcAll();
		}
	} BOOST_SCOPE_EXIT_END

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
		CalcAll();
}


/**
 * item contents changed
 */
void MagDynDlg::TermsTableItemChanged(QTableWidgetItem * /*item*/)
{
	if(m_ignoreTableChanges)
		return;

	if(m_autocalc->isChecked())
		CalcAll();
}


/**
 * item contents changed
 */
void MagDynDlg::VariablesTableItemChanged(QTableWidgetItem * /*item*/)
{
	if(m_ignoreTableChanges)
		return;

	if(m_autocalc->isChecked())
		CalcAll();
}


void MagDynDlg::ShowTableContextMenu(
	QTableWidget *pTab, QMenu *pMenu, QMenu *pMenuNoItem, const QPoint& pt)
{
	auto ptGlob = pTab->mapToGlobal(pt);

	if(const auto* item = pTab->itemAt(pt); item)
	{
		ptGlob.setY(ptGlob.y() + pMenu->sizeHint().height()/2);
		pMenu->popup(ptGlob);
	}
	else
	{
		ptGlob.setY(ptGlob.y() + pMenuNoItem->sizeHint().height()/2);
		pMenuNoItem->popup(ptGlob);
	}
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

	if(m_split_inout)
		m_sett->setValue("splitter", m_split_inout->saveState());

	if(m_structplot_dlg)
		m_sett->setValue("geo_struct_view", m_structplot_dlg->saveGeometry());
}


/**
 * refresh and calculate everything
 */
void MagDynDlg::CalcAll()
{
	SyncSitesAndTerms();
	StructPlotSync();
	CalcAllDynamics();
}


/**
 * enable GUI inputs after calculation threads have finished
 */
void MagDynDlg::EnableInput()
{
	m_tabs_in->setEnabled(true);
	m_tabs_out->setEnabled(true);
	m_menu->setEnabled(true);
	m_btnStart->setEnabled(true);
}


/**
 * disable GUI inputs for calculation threads
 */
void MagDynDlg::DisableInput()
{
	m_menu->setEnabled(false);
	m_tabs_out->setEnabled(false);
	m_tabs_in->setEnabled(false);
	m_btnStart->setEnabled(false);
}
