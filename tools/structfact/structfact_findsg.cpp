/**
 * structure factor tool -- functions for space group finder tab
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

#include <iostream>

#include "loadcif.h"
#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/phys.h"
#include "tlibs2/libs/algos.h"
#include "tlibs2/libs/qt/helper.h"

using namespace tl2_ops;



void StructFactDlg::AddTabItem_FindSG(int row, t_real x, t_real y, t_real z)
{
	bool bclone = 0;

	if(row == -1)	// append to end of table
		row = m_nuclei_FindSG->rowCount();
	else if(row == -2 && m_iCursorRow_FindSG >= 0)	// use row from member variable
		row = m_iCursorRow_FindSG;
	else if(row == -3 && m_iCursorRow_FindSG >= 0)	// use row from member variable +1
		row = m_iCursorRow_FindSG + 1;
	else if(row == -4 && m_iCursorRow_FindSG >= 0)	// use row from member variable +1
	{
		row = m_iCursorRow_FindSG + 1;
		bclone = 1;
	}

	m_nuclei_FindSG->setSortingEnabled(false);
	m_nuclei_FindSG->insertRow(row);

	if(bclone)
	{
		for(int thecol=0; thecol<3; ++thecol)
			m_nuclei_FindSG->setItem(row, thecol, m_nuclei_FindSG->item(m_iCursorRow_FindSG, thecol)->clone());
	}
	else
	{
		m_nuclei_FindSG->setItem(row, 0, new tl2::NumericTableWidgetItem<t_real>(x));
		m_nuclei_FindSG->setItem(row, 1, new tl2::NumericTableWidgetItem<t_real>(y));
		m_nuclei_FindSG->setItem(row, 2, new tl2::NumericTableWidgetItem<t_real>(z));
	}

	m_nuclei_FindSG->scrollToItem(m_nuclei_FindSG->item(row, 0));
	m_nuclei_FindSG->setCurrentCell(row, 0);
	m_nuclei_FindSG->setSortingEnabled(true);

	//FindSG();
}



void StructFactDlg::DelTabItem_FindSG(int begin, int end)
{
	// if nothing is selected, clear all items
	if(begin == -1 || m_nuclei_FindSG->selectedItems().count() == 0)
	{
		m_nuclei_FindSG->clearContents();
		m_nuclei_FindSG->setRowCount(0);
	}
	else if(begin == -2)	// clear selected
	{
		for(int row : GetSelectedRows_FindSG(true))
			m_nuclei_FindSG->removeRow(row);
	}
	else if(begin >= 0 && end >= 0)		// clear given range
	{
		for(int row=end-1; row>=begin; --row)
			m_nuclei_FindSG->removeRow(row);
	}

	//FindSG();
}


void StructFactDlg::MoveTabItemUp_FindSG()
{
	m_nuclei_FindSG->setSortingEnabled(false);

	auto selected = GetSelectedRows_FindSG(false);
	for(int row : selected)
	{
		if(row == 0)
			continue;

		auto *item = m_nuclei_FindSG->item(row, 0);
		if(!item || !item->isSelected())
			continue;

		m_nuclei_FindSG->insertRow(row-1);
		for(int col=0; col<m_nuclei_FindSG->columnCount(); ++col)
			m_nuclei_FindSG->setItem(row-1, col, m_nuclei_FindSG->item(row+1, col)->clone());
		m_nuclei_FindSG->removeRow(row+1);
	}

	for(int row=0; row<m_nuclei_FindSG->rowCount(); ++row)
	{
		if(auto *item = m_nuclei_FindSG->item(row, 0);
		   item && std::find(selected.begin(), selected.end(), row+1) != selected.end())
		   {
			   for(int col=0; col<m_nuclei_FindSG->columnCount(); ++col)
				   m_nuclei_FindSG->item(row, col)->setSelected(true);
		   }
	}
}


void StructFactDlg::MoveTabItemDown_FindSG()
{
	m_nuclei_FindSG->setSortingEnabled(false);

	auto selected = GetSelectedRows_FindSG(true);
	for(int row : selected)
	{
		if(row == m_nuclei_FindSG->rowCount()-1)
			continue;

		auto *item = m_nuclei_FindSG->item(row, 0);
		if(!item || !item->isSelected())
			continue;

		m_nuclei_FindSG->insertRow(row+2);
		for(int col=0; col<m_nuclei_FindSG->columnCount(); ++col)
			m_nuclei_FindSG->setItem(row+2, col, m_nuclei_FindSG->item(row, col)->clone());
		m_nuclei_FindSG->removeRow(row);
	}

	for(int row=0; row<m_nuclei_FindSG->rowCount(); ++row)
	{
		if(auto *item = m_nuclei_FindSG->item(row, 0);
		   item && std::find(selected.begin(), selected.end(), row-1) != selected.end())
		   {
			   for(int col=0; col<m_nuclei_FindSG->columnCount(); ++col)
				   m_nuclei_FindSG->item(row, col)->setSelected(true);
		   }
	}
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
std::vector<int> StructFactDlg::GetSelectedRows_FindSG(bool sort_reversed) const
{
	std::vector<int> vec;
	vec.reserve(m_nuclei_FindSG->selectedItems().size());

	for(int row=0; row<m_nuclei_FindSG->rowCount(); ++row)
	{
		if(auto *item = m_nuclei_FindSG->item(row, 0); item && item->isSelected())
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
void StructFactDlg::TableItemChanged_FindSG([[maybe_unused]] QTableWidgetItem *item)
{
	//FindSG();
}


void StructFactDlg::ShowTableContextMenu_FindSG(const QPoint& pt)
{
	auto ptGlob = m_nuclei_FindSG->mapToGlobal(pt);

	if(const auto* item = m_nuclei_FindSG->itemAt(pt); item)
	{
		m_iCursorRow_FindSG = item->row();
		ptGlob.setY(ptGlob.y() + m_pTabContextMenu_FindSG->sizeHint().height()/2);
		m_pTabContextMenu_FindSG->popup(ptGlob);
	}
	else
	{
		ptGlob.setY(ptGlob.y() + m_pTabContextMenuNoItem_FindSG->sizeHint().height()/2);
		m_pTabContextMenuNoItem_FindSG->popup(ptGlob);
	}
}
// ----------------------------------------------------------------------------



/**
 * find a matching space group for the given nucleus positions
 */
void StructFactDlg::FindSG()
{
	m_sgmatches->setPlainText("");

	std::vector<t_vec> vecFinal;
	std::ostringstream ostr;
	ostr.precision(g_prec);

	ostr << "Full set of nuclear positions to match:\n";
	for(int row=0; row<m_nuclei_FindSG->rowCount(); ++row)
	{
		auto *itemx = m_nuclei_FindSG->item(row, 0);
		auto *itemy = m_nuclei_FindSG->item(row, 1);
		auto *itemz = m_nuclei_FindSG->item(row, 2);

		if(!itemx || !itemy || !itemz)
			return;

		t_real posx=0, posy=0, posz=0;
		std::istringstream{itemx->text().toStdString()} >> posx;
		std::istringstream{itemy->text().toStdString()} >> posy;
		std::istringstream{itemz->text().toStdString()} >> posz;

		ostr << "\t(" << (row+1) << "): (" << posx << ", " << posy << ", " << posz << ")\n";
		vecFinal.emplace_back(t_vec{{posx, posy, posz}});
	}

	ostr << "\n";
	std::vector<t_vec> vecInit = vecFinal;

	while(1)
	{
		ostr << "\n--------------------------------------------------------------------------------\n";
		ostr << "Base set of nuclear positions:\n";
		std::size_t ctr = 1;
		for(const auto& pos : vecInit)
			ostr << "\t(" << ctr++ << ") " << pos << "\n";
		ostr << std::endl;

		auto matchingSGs = find_matching_sgs<t_vec, t_mat, t_real>(vecInit, vecFinal, g_eps);

		if(matchingSGs.size())
		{
			ostr << "Matching space groups:\n";
			ctr = 1;
			for(const auto& sg : matchingSGs)
				ostr << "\t(" << ctr++ << ") " << std::get<1>(sg) << "\n";
		}
		else
		{
			ostr << "No matching space groups.\n";
		}
		ostr << "--------------------------------------------------------------------------------\n\n";

		vecInit.pop_back();
		if(vecInit.size() == 0)
			break;
	}

	m_sgmatches->setPlainText(ostr.str().c_str());
}
