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

#include <QtWidgets/QMessageBox>

#include <iostream>
#include <tuple>

#include <boost/units/systems/si/codata/electron_constants.hpp>
#include <boost/units/systems/si/codata/neutron_constants.hpp>
#include <boost/units/systems/si/codata/electromagnetic_constants.hpp>
namespace si = boost::units::si;
namespace consts = si::constants;

#include "../structfact/loadcif.h"
#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/phys.h"
#include "tlibs2/libs/algos.h"
#include "tlibs2/libs/qt/helper.h"

using namespace tl2_ops;


// ----------------------------------------------------------------------------
void MagStructFactDlg::AddTabItem(int row,
	const std::string& name, t_real MMag, t_real x, t_real y, t_real z,
	t_real ReMx, t_real ReMy, t_real ReMz, t_real ImMx, t_real ImMy, t_real ImMz,
	t_real scale, const std::string& col)
{
	bool bclone = 0;
	m_ignoreChanges = 1;

	if(row == -1)	// append to end of table
		row = m_nuclei->rowCount();
	else if(row == -2 && m_iCursorRow >= 0)	// use row from member variable
		row = m_iCursorRow;
	else if(row == -3 && m_iCursorRow >= 0)	// use row from member variable +1
		row = m_iCursorRow + 1;
	else if(row == -4 && m_iCursorRow >= 0)	// use row from member variable +1
	{
		row = m_iCursorRow + 1;
		bclone = 1;
	}

	//bool sorting = m_nuclei->isSortingEnabled();
	m_nuclei->setSortingEnabled(false);
	m_nuclei->insertRow(row);

	if(bclone)
	{
		for(int thecol=0; thecol<NUM_COLS; ++thecol)
			m_nuclei->setItem(row, thecol, m_nuclei->item(m_iCursorRow, thecol)->clone());
	}
	else
	{
		m_nuclei->setItem(row, COL_NAME, new QTableWidgetItem(name.c_str()));
		m_nuclei->setItem(row, COL_M_MAG, new tl2::NumericTableWidgetItem<t_real>(MMag));
		m_nuclei->setItem(row, COL_X, new tl2::NumericTableWidgetItem<t_real>(x));
		m_nuclei->setItem(row, COL_Y, new tl2::NumericTableWidgetItem<t_real>(y));
		m_nuclei->setItem(row, COL_Z, new tl2::NumericTableWidgetItem<t_real>(z));
		m_nuclei->setItem(row, COL_ReM_X, new tl2::NumericTableWidgetItem<t_real>(ReMx));
		m_nuclei->setItem(row, COL_ReM_Y, new tl2::NumericTableWidgetItem<t_real>(ReMy));
		m_nuclei->setItem(row, COL_ReM_Z, new tl2::NumericTableWidgetItem<t_real>(ReMz));
		m_nuclei->setItem(row, COL_ImM_X, new tl2::NumericTableWidgetItem<t_real>(ImMx));
		m_nuclei->setItem(row, COL_ImM_Y, new tl2::NumericTableWidgetItem<t_real>(ImMy));
		m_nuclei->setItem(row, COL_ImM_Z, new tl2::NumericTableWidgetItem<t_real>(ImMz));
		m_nuclei->setItem(row, COL_RAD, new tl2::NumericTableWidgetItem<t_real>(scale));
		m_nuclei->setItem(row, COL_COL, new QTableWidgetItem(col.c_str()));
	}

	Add3DItem(row);

	m_nuclei->scrollToItem(m_nuclei->item(row, 0));
	m_nuclei->setCurrentCell(row, 0);

	m_nuclei->setSortingEnabled(/*sorting*/ true);

	m_ignoreChanges = 0;
	Calc();
}


void MagStructFactDlg::DelTabItem(int begin, int end)
{
	m_ignoreChanges = 1;

	// if nothing is selected, clear all items
	if(begin == -1 || m_nuclei->selectedItems().count() == 0)
	{
		if(m_plot)
		{
			for(int row=0; row<m_nuclei->rowCount(); ++row)
			{
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+0).toUInt(); obj)
					m_plot->GetRenderer()->RemoveObject(obj);
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+1).toUInt(); obj)
					m_plot->GetRenderer()->RemoveObject(obj);
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+2).toUInt(); obj)
					m_plot->GetRenderer()->RemoveObject(obj);
			}
			m_plot->update();
		}

		m_nuclei->clearContents();
		m_nuclei->setRowCount(0);
	}
	else if(begin == -2)	// clear selected
	{
		for(int row : GetSelectedRows(m_nuclei, true))
		{
			// remove 3d object
			if(m_plot)
			{
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+0).toUInt(); obj)
					m_plot->GetRenderer()->RemoveObject(obj);
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+1).toUInt(); obj)
					m_plot->GetRenderer()->RemoveObject(obj);
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+2).toUInt(); obj)
					m_plot->GetRenderer()->RemoveObject(obj);
				m_plot->update();
			}

			m_nuclei->removeRow(row);
		}
	}
	else if(begin >= 0 && end >= 0)		// clear given range
	{
		for(int row=end-1; row>=begin; --row)
		{
			// remove 3d object
			if(m_plot)
			{
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+0).toUInt(); obj)
					m_plot->GetRenderer()->RemoveObject(obj);
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+1).toUInt(); obj)
					m_plot->GetRenderer()->RemoveObject(obj);
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+2).toUInt(); obj)
					m_plot->GetRenderer()->RemoveObject(obj);
				m_plot->update();
			}

			m_nuclei->removeRow(row);
		}
	}

	m_ignoreChanges = 0;
	Calc();
}


void MagStructFactDlg::MoveTabItemUp(QTableWidget *pTab)
{
	m_ignoreChanges = 1;
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

	m_ignoreChanges = 0;
}


void MagStructFactDlg::MoveTabItemDown(QTableWidget *pTab)
{
	m_ignoreChanges = 1;
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

	m_ignoreChanges = 0;
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
std::vector<int> MagStructFactDlg::GetSelectedRows(QTableWidget *pTab, bool sort_reversed) const
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
 * selected a new row
 */
void MagStructFactDlg::TableCurCellChanged(int rowNew, int colNew, int rowOld, int colOld)
{}


/**
 * hovered over new row
 */
void MagStructFactDlg::TableCellEntered(const QModelIndex& idx)
{}


/**
 * item contents changed
 */
void MagStructFactDlg::TableItemChanged(QTableWidgetItem *item)
{
	// update associated 3d object
	Sync3DItem(item->row());

	if(!m_ignoreChanges)
		Calc();
}


void MagStructFactDlg::ShowTableContextMenu(QTableWidget *pTab, QMenu *pMenu, QMenu *pMenuNoItem, const QPoint& pt)
{
	auto ptGlob = pTab->mapToGlobal(pt);

	if(const auto* item = pTab->itemAt(pt); item)
	{
		m_iCursorRow = item->row();
		ptGlob.setY(ptGlob.y() + pMenu->sizeHint().height()/2);
		pMenu->popup(ptGlob);
	}
	else
	{
		ptGlob.setY(ptGlob.y() + pMenuNoItem->sizeHint().height()/2);
		pMenuNoItem->popup(ptGlob);
	}
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
void MagStructFactDlg::AddPropItem(int row,
	const std::string& name, t_real x, t_real y, t_real z, bool bConjFC)
{
	bool bclone = 0;
	m_ignoreChanges = 1;

	if(row == -1)	// append to end of table
		row = m_propvecs->rowCount();
	else if(row == -2 && m_iCursorRow >= 0)	// use row from member variable
		row = m_iCursorRow;
	else if(row == -3 && m_iCursorRow >= 0)	// use row from member variable +1
		row = m_iCursorRow + 1;
	else if(row == -4 && m_iCursorRow >= 0)	// use row from member variable +1
	{
		row = m_iCursorRow + 1;
		bclone = 1;
	}

	//bool sorting = m_propvecs->isSortingEnabled();
	m_propvecs->setSortingEnabled(false);
	m_propvecs->insertRow(row);

	if(bclone)
	{
		for(int thecol=0; thecol<PROP_NUM_COLS; ++thecol)
			m_propvecs->setItem(row, thecol, m_propvecs->item(m_iCursorRow, thecol)->clone());
	}
	else
	{
		m_propvecs->setItem(row, PROP_COL_NAME, new QTableWidgetItem(name.c_str()));
		m_propvecs->setItem(row, PROP_COL_X, new tl2::NumericTableWidgetItem<t_real>(x));
		m_propvecs->setItem(row, PROP_COL_Y, new tl2::NumericTableWidgetItem<t_real>(y));
		m_propvecs->setItem(row, PROP_COL_Z, new tl2::NumericTableWidgetItem<t_real>(z));
		m_propvecs->setItem(row, PROP_COL_CONJ, new tl2::NumericTableWidgetItem<int>(bConjFC));
	}

	//Add3DItem(row);	TODO

	m_propvecs->scrollToItem(m_propvecs->item(row, 0));
	m_propvecs->setCurrentCell(row, 0);

	m_propvecs->setSortingEnabled(/*sorting*/ true);

	m_ignoreChanges = 0;
	Calc();
}


void MagStructFactDlg::DelPropItem(int begin, int end)
{
	m_ignoreChanges = 1;

	// if nothing is selected, clear all items
	if(begin == -1 || m_propvecs->selectedItems().count() == 0)
	{
		m_propvecs->clearContents();
		m_propvecs->setRowCount(0);
	}
	else if(begin == -2)	// clear selected
	{
		for(int row : GetSelectedRows(m_propvecs, true))
			m_propvecs->removeRow(row);
	}
	else if(begin >= 0 && end >= 0)		// clear given range
	{
		for(int row=end-1; row>=begin; --row)
			m_propvecs->removeRow(row);
	}

	m_ignoreChanges = 0;
	Calc();
}


/**
 * item contents changed
 */
void MagStructFactDlg::PropItemChanged(QTableWidgetItem *item)
{
	// update associated 3d object
	//Sync3DItem(item->row());

	if(!m_ignoreChanges)
		Calc();
}
// ----------------------------------------------------------------------------



/**
 * generate symmetric nuclei from space group
 */
void MagStructFactDlg::GenerateFromSG()
{
	m_ignoreCalc = 1;

	try
	{
		// symops of current space group
		auto sgidx = m_comboSG->itemData(m_comboSG->currentIndex()).toInt();
		if(sgidx < 0 || sgidx >= m_SGops.size())
		{
			QMessageBox::critical(this, "Structure Factors", "Invalid space group selected.");
			return;
		}

		const auto& ops = m_SGops[sgidx];
		std::vector<std::tuple<std::string, t_real,
			t_real, t_real, t_real,
			t_real, t_real, t_real, t_real, t_real, t_real,
			t_real, std::string>> generatednuclei;

		// iterate nuclei
		int orgRowCnt = m_nuclei->rowCount();
		for(int row=0; row<orgRowCnt; ++row)
		{
			t_real MMag{}, x{},y{},z{}, ReMx{}, ReMy{}, ReMz{}, ImMx{}, ImMy{}, ImMz{}, scale{};
			std::istringstream{m_nuclei->item(row, COL_M_MAG)->text().toStdString()} >> MMag;
			std::istringstream{m_nuclei->item(row, COL_X)->text().toStdString()} >> x;
			std::istringstream{m_nuclei->item(row, COL_Y)->text().toStdString()} >> y;
			std::istringstream{m_nuclei->item(row, COL_Z)->text().toStdString()} >> z;
			std::istringstream{m_nuclei->item(row, COL_ReM_X)->text().toStdString()} >> ReMx;
			std::istringstream{m_nuclei->item(row, COL_ReM_Y)->text().toStdString()} >> ReMy;
			std::istringstream{m_nuclei->item(row, COL_ReM_Z)->text().toStdString()} >> ReMz;
			std::istringstream{m_nuclei->item(row, COL_ImM_X)->text().toStdString()} >> ImMx;
			std::istringstream{m_nuclei->item(row, COL_ImM_Y)->text().toStdString()} >> ImMy;
			std::istringstream{m_nuclei->item(row, COL_ImM_Z)->text().toStdString()} >> ImMz;
			std::istringstream{m_nuclei->item(row, COL_RAD)->text().toStdString()} >> scale;
			std::string name = m_nuclei->item(row, COL_NAME)->text().toStdString();
			std::string col = m_nuclei->item(row, COL_COL)->text().toStdString();

			t_vec nucl = tl2::create<t_vec>({x, y, z, 1});
			auto newnuclei = tl2::apply_ops_hom<t_vec, t_mat, t_real>(nucl, ops, g_eps);

			for(const auto& newnucl : newnuclei)
			{
				generatednuclei.emplace_back(std::make_tuple(name, MMag, newnucl[0], newnucl[1], newnucl[2],
					ReMx, ReMy, ReMz, ImMx, ImMy, ImMz,		// TODO: apply sg ops to spins
					scale, col));
			}
		}

		// remove original nuclei
		DelTabItem(-1);

		// add new nuclei
		for(const auto& nucl : generatednuclei)
			std::apply(&MagStructFactDlg::AddTabItem, std::tuple_cat(std::make_tuple(this, -1), nucl));
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Structure Factors", ex.what());
	}

	m_ignoreCalc = 0;
	Calc();
}



// ----------------------------------------------------------------------------
/**
 * reads nuclei positions from table
 */
std::vector<NuclPos> MagStructFactDlg::GetNuclei() const
{
	std::vector<NuclPos> vec;

	for(int row=0; row<m_nuclei->rowCount(); ++row)
	{
		auto *name = m_nuclei->item(row, COL_NAME);
		auto *MMag = m_nuclei->item(row, COL_M_MAG);
		auto *x = m_nuclei->item(row, COL_X);
		auto *y = m_nuclei->item(row, COL_Y);
		auto *z = m_nuclei->item(row, COL_Z);
		auto *ReMx = m_nuclei->item(row, COL_ReM_X);
		auto *ReMy = m_nuclei->item(row, COL_ReM_Y);
		auto *ReMz = m_nuclei->item(row, COL_ReM_Z);
		auto *ImMx = m_nuclei->item(row, COL_ImM_X);
		auto *ImMy = m_nuclei->item(row, COL_ImM_Y);
		auto *ImMz = m_nuclei->item(row, COL_ImM_Z);
		auto *scale = m_nuclei->item(row, COL_RAD);
		auto *col = m_nuclei->item(row, COL_COL);

		if(!name || !MMag || !x || !y || !z || !ReMx || !ReMy || !ReMz || !ImMx || !ImMy || !ImMz || !scale || !col)
		{
			std::cerr << "Invalid entry in row " << row << "." << std::endl;
			continue;
		}

		NuclPos nucl;
		nucl.name = name->text().toStdString();
		nucl.col = col->text().toStdString();
		std::istringstream{MMag->text().toStdString()} >> nucl.MAbs;
		std::istringstream{x->text().toStdString()} >> nucl.pos[0];
		std::istringstream{y->text().toStdString()} >> nucl.pos[1];
		std::istringstream{z->text().toStdString()} >> nucl.pos[2];
		std::istringstream{ReMx->text().toStdString()} >> nucl.ReM[0];
		std::istringstream{ReMy->text().toStdString()} >> nucl.ReM[1];
		std::istringstream{ReMz->text().toStdString()} >> nucl.ReM[2];
		std::istringstream{ImMx->text().toStdString()} >> nucl.ImM[0];
		std::istringstream{ImMy->text().toStdString()} >> nucl.ImM[1];
		std::istringstream{ImMz->text().toStdString()} >> nucl.ImM[2];
		std::istringstream{scale->text().toStdString()} >> nucl.scale;

		vec.emplace_back(std::move(nucl));
	}

	return vec;
}


/**
 * calculate crystal B matrix
 */
void MagStructFactDlg::CalcB(bool bFullRecalc)
{
	if(m_ignoreCalc)
		return;

	t_real a,b,c, alpha,beta,gamma;
	std::istringstream{m_editA->text().toStdString()} >> a;
	std::istringstream{m_editB->text().toStdString()} >> b;
	std::istringstream{m_editC->text().toStdString()} >> c;
	std::istringstream{m_editAlpha->text().toStdString()} >> alpha;
	std::istringstream{m_editBeta->text().toStdString()} >> beta;
	std::istringstream{m_editGamma->text().toStdString()} >> gamma;

	m_crystB = tl2::B_matrix<t_mat>(
		a, b, c,
		alpha/180.*tl2::pi<t_real>,
		beta/180.*tl2::pi<t_real>,
		gamma/180.*tl2::pi<t_real>);

	bool ok = true;
	std::tie(m_crystA, ok) = tl2::inv(m_crystB);
	if(!ok)
	{
		m_crystA = tl2::unit<t_mat>();
		std::cerr << "Error: Cannot invert B matrix." << std::endl;
	}
	else
	{
		m_crystA *= t_real(2)*tl2::pi<t_real>;
	}

	if(m_plot)
	{
		t_mat_gl matA{m_crystA};
		m_plot->GetRenderer()->SetBTrafo(m_crystB, &matA);
	}
	if(m_plotSC)
	{
		t_mat_gl matA{m_crystA};
		m_plotSC->GetRenderer()->SetBTrafo(m_crystB, &matA);
	}

	if(bFullRecalc)
		Calc();
}


/**
 * calculate structure factors
 */
void MagStructFactDlg::Calc()
{
	if(m_ignoreCalc)
		return;

	const t_real p = -t_real(consts::codata::mu_n/consts::codata::mu_N*consts::codata::r_e/si::meters)*0.5e15;
	const auto maxBZ = m_maxBZ->value();
	const auto maxSCx = m_maxSC[0]->value();
	const auto maxSCy = m_maxSC[1]->value();
	const auto maxSCz = m_maxSC[2]->value();
	const bool remove_zeroes = m_RemoveZeroes->isChecked();


	// propagation vectors
	std::vector<t_vec> propvecs;
	std::vector<bool> conjFCs;
	for(int row=0; row<m_propvecs->rowCount(); ++row)
	{
		t_real x{},y{},z{};
		int iConj{0};
		std::istringstream{m_propvecs->item(row, PROP_COL_X)->text().toStdString()} >> x;
		std::istringstream{m_propvecs->item(row, PROP_COL_Y)->text().toStdString()} >> y;
		std::istringstream{m_propvecs->item(row, PROP_COL_Z)->text().toStdString()} >> z;
		std::istringstream{m_propvecs->item(row, PROP_COL_CONJ)->text().toStdString()} >> iConj;

		propvecs.emplace_back(tl2::create<t_vec>({x, y, z}));
		conjFCs.push_back(iConj != 0);
	}


	// powder lines
	std::vector<PowderLine> powderlines;
	auto add_powderline = [&powderlines](t_real Q, t_real I,
		t_real h, t_real k, t_real l)
	{
		std::ostringstream ostrPeak; ostrPeak.precision(g_prec);
		ostrPeak << "(" << h << "," << k << "," << l << "); ";

		// is this Q value already in the vector?
		bool foundQ = false;
		for(auto& line : powderlines)
		{
			if(tl2::equals<t_real>(line.Q, Q, g_eps))
			{
				line.I += I;
				line.peaks +=  ostrPeak.str();
				++line.num_peaks;
				foundQ = true;
				break;
			}
		}

		// start a new line
		if(!foundQ)
		{
			PowderLine line;
			line.Q = Q;
			line.I = I;
			line.peaks = ostrPeak.str();
			line.num_peaks = 1;
			powderlines.emplace_back(std::move(line));
		}
	};


	std::vector<t_cplx> bs;
	std::vector<t_vec> pos;
	std::vector<t_vec_cplx> Ms;
	std::vector<t_real> scales;
	std::vector<std::string> names;
	std::vector<std::string> cols;

	for(const auto& nucl : GetNuclei())
	{
		pos.emplace_back(tl2::create<t_vec>({ nucl.pos[0], nucl.pos[1], nucl.pos[2] }));
		Ms.emplace_back(nucl.MAbs * tl2::create<t_vec_cplx>({
			t_cplx{nucl.ReM[0], nucl.ImM[0]},
			t_cplx{nucl.ReM[1], nucl.ImM[1]},
			t_cplx{nucl.ReM[2], nucl.ImM[2]} }));
		names.emplace_back(std::move(nucl.name));
		cols.emplace_back(std::move(nucl.col));
		scales.push_back(nucl.scale);
	}


	std::ostringstream ostr, ostrPowder;
	ostr.precision(g_prec);
	ostrPowder.precision(g_prec);

	ostr << "# Magnetic single-crystal structure factors:" << "\n";
	ostr << "# "
		<< std::setw(g_prec*1.2-2) << std::right << "h" << " "
		<< std::setw(g_prec*1.2) << std::right << "k" << " "
		<< std::setw(g_prec*1.2) << std::right << "l" << " "
		<< std::setw(g_prec*2) << std::right << "|Q| (1/A)" << " "
		<< std::setw(g_prec*2) << std::right << "|Fm|^2" << " "
		<< std::setw(g_prec*2) << std::right << "|Fm_perp|^2" << " "
		<< std::setw(g_prec*5) << std::right << "Fm_x (fm)" << " "
		<< std::setw(g_prec*5) << std::right << "Fm_y (fm)" << " "
		<< std::setw(g_prec*5) << std::right << "Fm_z (fm)" << " "
		<< std::setw(g_prec*5) << std::right << "Fm_perp_x (fm)" << " "
		<< std::setw(g_prec*5) << std::right << "Fm_perp_y (fm)" << " "
		<< std::setw(g_prec*5) << std::right << "Fm_perp_z (fm)" << "\n";


	ostrPowder << "# Magnetic powder lines:" << "\n";
	ostrPowder << "# "
		<< std::setw(g_prec*2-2) << std::right << "|Q| (1/A)" << " "
		<< std::setw(g_prec*2) << std::right << "|F|^2" << " "
		<< std::setw(g_prec*2) << std::right << "Mult." << "\n";


	// iterate brillouin zones
	for(t_real h=-maxBZ; h<=maxBZ; ++h)
	for(t_real k=-maxBZ; k<=maxBZ; ++k)
	for(t_real l=-maxBZ; l<=maxBZ; ++l)
	{
		// iterate propagation vectors
		for(const auto& prop : propvecs)
		{
			auto Q = tl2::create<t_vec>({ h,k,l }) + prop;
			auto Q_invA = m_crystB * Q;
			auto Qabs_invA = tl2::norm(Q_invA);
			auto Q_cplx = tl2::create<t_vec_cplx>({ Q[0], Q[1], Q[2] });

			// magnetic structure factor
			auto Fm = p * tl2::structure_factor<t_vec, t_vec_cplx>(Ms, pos, Q, nullptr);
			bool Fm_is_zero = 1;

			// set small value to zero
			for(auto &comp : Fm)
			{
				if(tl2::equals<t_real>(comp.real(), t_real(0), g_eps))
					comp.real(0.);
				else
					Fm_is_zero = 0;
				if(tl2::equals<t_real>(comp.imag(), t_real(0), g_eps))
					comp.imag(0.);
				else
					Fm_is_zero = 0;
			}
			if(Fm.size() == 0)
				Fm = tl2::zero<t_vec_cplx>(3);

			// neutron scattering: orthogonal projection onto plane with normal Q.
			auto Fm_perp = tl2::ortho_project<t_vec_cplx>(Fm, Q_cplx, false);
			//auto proj = tl2::ortho_projector<t_mat_cplx, t_vec_cplx>(Q_cplx, false);
			//auto Fm_perp = proj * Fm;

			// set small value to zero
			for(auto &comp : Fm_perp)
			{
				if(tl2::equals<t_real>(comp.real(), t_real(0), g_eps))
					comp.real(0.);
				if(tl2::equals<t_real>(comp.imag(), t_real(0), g_eps))
					comp.imag(0.);
			}

			t_real I = tl2::inner(Fm, Fm).real();
			t_real I_perp = tl2::inner(Fm_perp, Fm_perp).real();

			if(std::isnan(I_perp))
			{
				I_perp = 0.;
				for(auto& comp : Fm_perp)
					comp = t_cplx{0.,0.};
			}

			if(remove_zeroes && Fm_is_zero)
				continue;

			add_powderline(Qabs_invA, I_perp, h,k,l);

			ostr
				<< std::setw(g_prec*1.2) << std::right << h+prop[0] << " "
				<< std::setw(g_prec*1.2) << std::right << k+prop[1] << " "
				<< std::setw(g_prec*1.2) << std::right << l+prop[2] << " "
				<< std::setw(g_prec*2) << std::right << Qabs_invA << " "
				<< std::setw(g_prec*2) << std::right << I << " "
				<< std::setw(g_prec*2) << std::right << I_perp << " "
				<< std::setw(g_prec*5) << std::right << Fm[0] << " "
				<< std::setw(g_prec*5) << std::right << Fm[1] << " "
				<< std::setw(g_prec*5) << std::right << Fm[2] << " "
				<< std::setw(g_prec*5) << std::right << Fm_perp[0] << " "
				<< std::setw(g_prec*5) << std::right << Fm_perp[1] << " "
				<< std::setw(g_prec*5) << std::right << Fm_perp[2] << "\n";
		}
	}

	// single-crystal peaks
	m_structfacts->setPlainText(ostr.str().c_str());


	// powder peaks
	std::stable_sort(powderlines.begin(), powderlines.end(),
		[](const PowderLine& line1, const PowderLine& line2) -> bool
		{
			return line1.Q < line2.Q;
		});

	for(const auto& line : powderlines)
	{
		ostrPowder
			<< std::setw(g_prec*2) << std::right << line.Q << " "
			<< std::setw(g_prec*2) << std::right << line.I << " "
			<< std::setw(g_prec*2) << std::right << line.num_peaks << " "
			<< line.peaks << "\n";
	}

	m_powderlines->setPlainText(ostrPowder.str().c_str());


	// ------------------------------------------------------------------------


	// generate real magnetic moments from the fourier components

	// remove old 3d objects
	if(m_plotSC)
	{
		for(auto obj : m_3dobjsSC)
			m_plotSC->GetRenderer()->RemoveObject(obj);
		m_3dobjsSC.clear();
	}

	std::ostringstream ostrMoments;
	ostrMoments.precision(g_prec);

	ostrMoments << "# Magnetic moments:" << "\n";
	ostrMoments << "# "
		<< std::setw(g_prec*2-2) << std::right << "Name" << " "		// name of nucleus
		<< std::setw(g_prec*2) << std::right << "x" << " "			// position of nucleus in supercell
		<< std::setw(g_prec*2) << std::right << "y" << " "
		<< std::setw(g_prec*2) << std::right << "z" << " "
		<< std::setw(g_prec*2) << std::right << "Re{M_x}" << " "	// magnetic moment
		<< std::setw(g_prec*2) << std::right << "Re{M_y}" << " "
		<< std::setw(g_prec*2) << std::right << "Re{M_z}" << " "
		<< std::setw(g_prec*2) << std::right << "Im{M_x}" << " "
		<< std::setw(g_prec*2) << std::right << "Im{M_y}" << " "
		<< std::setw(g_prec*2) << std::right << "Im{M_z}" << " "
		<< std::setw(g_prec*1.2) << std::right << "sc_x" << " "		// centring of supercell origin
		<< std::setw(g_prec*1.2) << std::right << "sc_y" << " "
		<< std::setw(g_prec*1.2) << std::right << "sc_z" << "\n";

	//std::vector<t_vec_cplx> moments;
	auto vecCentring = tl2::create<t_vec>({0, 0, 0});

	const t_cplx imag{0., 1.};
	const t_real twopi = tl2::pi<t_real>*t_real{2};

	// iterate over supercell
	for(t_real sc_x=-maxSCx; sc_x<=maxSCx; ++sc_x)
	for(t_real sc_y=-maxSCy; sc_y<=maxSCy; ++sc_y)
	for(t_real sc_z=-maxSCz; sc_z<=maxSCz; ++sc_z)
	{
		auto vecCellCentre = tl2::create<t_vec>({ sc_x, sc_y, sc_z })
			+ vecCentring;

		// iterate magnetic atoms
		for(std::size_t nuclidx=0; nuclidx<Ms.size(); ++nuclidx)
		{
			const t_vec_cplx& fourier = Ms[nuclidx];
			const std::string& name = names[nuclidx];
			const std::string& colstr = cols[nuclidx];
			auto thepos = pos[nuclidx] + vecCellCentre;
			auto scale = scales[nuclidx];

			auto posGL = tl2::convert<t_vec_gl>(thepos);
			auto moment = tl2::create<t_vec_cplx>({0, 0, 0});
			auto fourier_conj = tl2::conj(fourier);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
			qreal r=1, g=1, b=1;
#else
			float r=1, g=1, b=1;
#endif
			QColor col{colstr.c_str()};
			col.getRgbF(&r, &g, &b);

			// iterate propagation vectors
			for(std::size_t propidx=0; propidx<propvecs.size(); ++propidx)
			{
				const auto& propvec = propvecs[propidx];
				auto *pfourier = conjFCs[propidx] ? &fourier_conj : &fourier;
				moment += *pfourier *
					std::exp(imag*twopi *
						tl2::inner<t_vec>(
							propvec, vecCellCentre));
			}

			// set small values to zero
			for(auto &comp : moment)
			{
				if(tl2::equals<t_real>(comp.real(), t_real(0), g_eps))
					comp.real(0.);
				if(tl2::equals<t_real>(comp.imag(), t_real(0), g_eps))
					comp.imag(0.);
			}


			// add 3d objs to super cell view
			if(m_plotSC)
			{
				auto objArrowRe = m_plotSC->GetRenderer()->AddLinkedObject(m_arrowSC, 0,0,0, 1,1,1,1);
				auto objArrowIm = m_plotSC->GetRenderer()->AddLinkedObject(m_arrowSC, 0,0,0, 1,1,1,1);

				auto [_vecReM, _vecImM] =
					tl2::split_cplx<t_vec_cplx, t_vec>(moment);
				auto vecReM = tl2::convert<t_vec_gl>(_vecReM);
				auto vecImM = tl2::convert<t_vec_gl>(_vecImM);

				auto normReM = tl2::norm<t_vec_gl>(vecReM);
				auto normImM = tl2::norm<t_vec_gl>(vecImM);

				t_mat_gl matArrowRe = tl2::get_arrow_matrix<t_vec_gl, t_mat_gl, t_real_gl>(
					vecReM, 									// to
					1, 											// post-scale
					tl2::create<t_vec_gl>({0, 0, 0}),				// post-translate
					tl2::create<t_vec_gl>({0, 0, 1}),				// from
					normReM*scale,							// pre-scale
					posGL										// pre-translate
				);

				t_mat_gl matArrowIm = tl2::get_arrow_matrix<t_vec_gl, t_mat_gl, t_real_gl>(
					vecImM, 									// to
					1, 											// post-scale
					tl2::create<t_vec_gl>({0, 0, 0}),				// post-translate
					tl2::create<t_vec_gl>({0, 0, 1}),				// from
					normImM*scale,								// pre-scale
					posGL										// pre-translate
				);

				// labels
				std::ostringstream ostrMom;
				ostrMom.precision(g_prec);
				ostrMom
					<< "Re{M} = (" << moment[0].real() << " " << moment[1].real() << " " << moment[2].real() << "); "
					<< "Im{M} = (" << moment[0].imag() << " " << moment[1].imag() << " " << moment[2].imag() << "); "
					<< "r = (" << thepos[0] << " " << thepos[1] << " " << thepos[2] << ")";

				m_plotSC->GetRenderer()->SetObjectMatrix(objArrowRe, matArrowRe);
				m_plotSC->GetRenderer()->SetObjectMatrix(objArrowIm, matArrowIm);
				m_plotSC->GetRenderer()->SetObjectCol(objArrowRe, r, g, b, 1.);
				m_plotSC->GetRenderer()->SetObjectCol(objArrowIm, 1.-r, 1.-g, 1.-b, 1.);
				//m_plotSC->GetRenderer()->SetObjectLabel(objArrowRe, name + " (real)");
				//m_plotSC->GetRenderer()->SetObjectLabel(objArrowIm, name + " (imag)");
				m_plotSC->GetRenderer()->SetObjectDataString(objArrowRe, name + " (real); " + ostrMom.str());
				m_plotSC->GetRenderer()->SetObjectDataString(objArrowIm, name + " (imag); " + ostrMom.str());
				m_plotSC->GetRenderer()->SetObjectVisible(objArrowRe, !tl2::equals<t_real_gl>(normReM, 0, g_eps));
				m_plotSC->GetRenderer()->SetObjectVisible(objArrowIm, !tl2::equals<t_real_gl>(normImM, 0, g_eps));

				m_3dobjsSC.push_back(objArrowRe);
				m_3dobjsSC.push_back(objArrowIm);
			}

			ostrMoments
				<< std::setw(g_prec*2) << std::right << name << " "
				<< std::setw(g_prec*2) << std::right << thepos[0] << " "
				<< std::setw(g_prec*2) << std::right << thepos[1] << " "
				<< std::setw(g_prec*2) << std::right << thepos[2] << " "
				<< std::setw(g_prec*2) << std::right << moment[0].real() << " "
				<< std::setw(g_prec*2) << std::right << moment[1].real() << " "
				<< std::setw(g_prec*2) << std::right << moment[2].real() << " "
				<< std::setw(g_prec*2) << std::right << moment[0].imag() << " "
				<< std::setw(g_prec*2) << std::right << moment[1].imag() << " "
				<< std::setw(g_prec*2) << std::right << moment[2].imag() << " "
				<< std::setw(g_prec*1.2) << std::right << vecCellCentre[0] << " "
				<< std::setw(g_prec*1.2) << std::right << vecCellCentre[1] << " "
				<< std::setw(g_prec*1.2) << std::right << vecCellCentre[2] << "\n";

			//moments.emplace_back(std::move(moment));
		}
	}

	if(m_plotSC) m_plotSC->update();
	m_moments->setPlainText(ostrMoments.str().c_str());
}
// ----------------------------------------------------------------------------
