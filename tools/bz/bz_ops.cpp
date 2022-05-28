/**
 * brillouin zone tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date May-2022
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 28-Dec-2018 from my privately developed "misc" project (https://github.com/t-weber/misc).
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

#include <QtWidgets/QTabWidget>
#include <QtWidgets/QMessageBox>

#include <iostream>
#include <tuple>

#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/phys.h"
#include "tlibs2/libs/algos.h"
#include "tlibs2/libs/qt/helper.h"

using namespace tl2_ops;


/**
 * converts a symmetry operation matrix to a string
 */
std::string BZDlg::OpToStr(const t_mat& op)
{
	std::ostringstream ostr;
	ostr.precision(g_prec);

	for(std::size_t row=0; row<op.size1(); ++row)
	{
		for(std::size_t col=0; col<op.size2(); ++col)
		{
			t_real elem = op(row, col);
			tl2::set_eps_0(elem);
			ostr << elem << " ";
		}

		ostr << " ";
	}

	return ostr.str();
}


/**
 * convert a string to a symmetry operation matrix
 */
t_mat BZDlg::StrToOp(const std::string& str)
{
	t_mat op = tl2::unit<t_mat>(4);

	std::istringstream istr(str);
	for(std::size_t row=0; row<op.size1(); ++row)
		for(std::size_t col=0; col<op.size2(); ++col)
			istr >> op(row, col);

	return op;
}


void BZDlg::AddTabItem(int row, const t_mat& op)
{
	bool bclone = 0;
	m_ignoreChanges = 1;

	if(row == -1)	// append to end of table
		row = m_symops->rowCount();
	else if(row == -2 && m_iCursorRow >= 0)	// use row from member variable
		row = m_iCursorRow;
	else if(row == -3 && m_iCursorRow >= 0)	// use row from member variable +1
		row = m_iCursorRow + 1;
	else if(row == -4 && m_iCursorRow >= 0)	// use row from member variable +1
	{
		row = m_iCursorRow + 1;
		bclone = 1;
	}

	//bool sorting = m_symops->isSortingEnabled();
	m_symops->setSortingEnabled(false);
	m_symops->insertRow(row);

	if(bclone)
	{
		for(int thecol=0; thecol<NUM_COLS; ++thecol)
			m_symops->setItem(row, thecol, m_symops->item(m_iCursorRow, thecol)->clone());
	}
	else
	{
		m_symops->setItem(row, COL_OP,
			new QTableWidgetItem(OpToStr(op).c_str()));
	}

	m_symops->scrollToItem(m_symops->item(row, 0));
	m_symops->setCurrentCell(row, 0);

	m_symops->setSortingEnabled(/*sorting*/ true);

	m_ignoreChanges = 0;
	CalcBZ();
}


void BZDlg::DelTabItem(int begin, int end)
{
	m_ignoreChanges = 1;

	// if nothing is selected, clear all items
	if(begin == -1 || m_symops->selectedItems().count() == 0)
	{
		m_symops->clearContents();
		m_symops->setRowCount(0);
	}
	else if(begin == -2)	// clear selected
	{
		for(int row : GetSelectedRows(true))
		{
			m_symops->removeRow(row);
		}
	}
	else if(begin >= 0 && end >= 0)		// clear given range
	{
		for(int row=end-1; row>=begin; --row)
		{
			m_symops->removeRow(row);
		}
	}

	m_ignoreChanges = 0;
	CalcBZ();
}


void BZDlg::MoveTabItemUp()
{
	m_ignoreChanges = 1;
	m_symops->setSortingEnabled(false);

	auto selected = GetSelectedRows(false);
	for(int row : selected)
	{
		if(row == 0)
			continue;

		auto *item = m_symops->item(row, 0);
		if(!item || !item->isSelected())
			continue;

		m_symops->insertRow(row-1);
		for(int col=0; col<m_symops->columnCount(); ++col)
			m_symops->setItem(row-1, col, m_symops->item(row+1, col)->clone());
		m_symops->removeRow(row+1);
	}

	for(int row=0; row<m_symops->rowCount(); ++row)
	{
		if(auto *item = m_symops->item(row, 0);
			item && std::find(selected.begin(), selected.end(), row+1) != selected.end())
		{
			for(int col=0; col<m_symops->columnCount(); ++col)
				m_symops->item(row, col)->setSelected(true);
		}
	}

	m_ignoreChanges = 0;
}


void BZDlg::MoveTabItemDown()
{
	m_ignoreChanges = 1;
	m_symops->setSortingEnabled(false);

	auto selected = GetSelectedRows(true);
	for(int row : selected)
	{
		if(row == m_symops->rowCount()-1)
			continue;

		auto *item = m_symops->item(row, 0);
		if(!item || !item->isSelected())
			continue;

		m_symops->insertRow(row+2);
		for(int col=0; col<m_symops->columnCount(); ++col)
			m_symops->setItem(row+2, col, m_symops->item(row, col)->clone());
		m_symops->removeRow(row);
	}

	for(int row=0; row<m_symops->rowCount(); ++row)
	{
		if(auto *item = m_symops->item(row, 0);
			item && std::find(selected.begin(), selected.end(), row-1) != selected.end())
		{
			for(int col=0; col<m_symops->columnCount(); ++col)
				m_symops->item(row, col)->setSelected(true);
		}
	}

	m_ignoreChanges = 0;
}


std::vector<int> BZDlg::GetSelectedRows(bool sort_reversed) const
{
	std::vector<int> vec;
	vec.reserve(m_symops->selectedItems().size());

	for(int row=0; row<m_symops->rowCount(); ++row)
	{
		if(auto *item = m_symops->item(row, 0); item && item->isSelected())
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
 * selected a new row
 */
void BZDlg::TableCurCellChanged(
	[[maybe_unused]] int rowNew,
	[[maybe_unused]] int colNew,
	[[maybe_unused]] int rowOld,
	[[maybe_unused]] int colOld)
{
}


/**
 * hovered over new row
 */
void BZDlg::TableCellEntered(const QModelIndex&)
{
}


/**
 * item contents changed
 */
void BZDlg::TableItemChanged(QTableWidgetItem *item)
{
	// update associated 3d object
	if(item)
	{
		int row = item->row();
		if(std::size_t obj = m_symops->item(row, COL_OP)->
			data(Qt::UserRole).toUInt(); obj)
		{
			auto *itemOp = m_symops->item(row, COL_OP);

			// TODO
		}
	}

	if(!m_ignoreChanges)
		CalcBZ();
}


void BZDlg::ShowTableContextMenu(const QPoint& pt)
{
	auto ptGlob = m_symops->mapToGlobal(pt);

	if(const auto* item = m_symops->itemAt(pt); item)
	{
		m_iCursorRow = item->row();
		ptGlob.setY(ptGlob.y() + m_pTabContextMenu->sizeHint().height()/2);
		m_pTabContextMenu->popup(ptGlob);
	}
	else
	{
		ptGlob.setY(ptGlob.y() + m_pTabContextMenuNoItem->sizeHint().height()/2);
		m_pTabContextMenuNoItem->popup(ptGlob);
	}
}


/**
 * get symmetry operations from selected space group
 */
void BZDlg::GetSymOpsFromSG()
{
	m_ignoreCalc = 1;

	try
	{
		// symops of current space group
		auto sgidx = m_comboSG->itemData(m_comboSG->currentIndex()).toInt();
		if(sgidx < 0 || std::size_t(sgidx) >= m_SGops.size())
		{
			QMessageBox::critical(this, "Space Group Conversion",
				"Invalid space group selected.");
			m_ignoreCalc = 0;
			return;
		}

		// remove original symops
		//DelTabItem(0, orgRowCnt);
		DelTabItem(-1);

		// add symops
		for(const auto& op : m_SGops[sgidx])
		{
			AddTabItem(-1, op);
		}
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Space Group Conversion", ex.what());
	}


	m_ignoreCalc = 0;
	CalcB(false);
	CalcBZ();
}


/**
 * reads symmetry operations from table
 */
std::vector<t_mat> BZDlg::GetSymOps(bool only_centring) const
{
	std::vector<t_mat> ops;

	for(int row=0; row<m_symops->rowCount(); ++row)
	{
		auto *op_item = m_symops->item(row, COL_OP);
		if(!op_item)
		{
			std::cerr << "Invalid entry in row " << row << "." << std::endl;
			continue;
		}

		t_mat op = StrToOp(op_item->text().toStdString());

		bool add_op = true;
		if(only_centring)
			add_op = tl2::hom_is_centering<t_mat>(op, g_eps);

		if(add_op)
			ops.emplace_back(std::move(op));
	}

	return ops;
}
