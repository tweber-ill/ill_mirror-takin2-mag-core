/**
 * structure factor tool -- functions for nuclei tab
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

#include <QtWidgets/QTabWidget>
#include <QtWidgets/QMessageBox>

#include <iostream>
#include <tuple>

#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/phys.h"
#include "tlibs2/libs/algos.h"
#include "tlibs2/libs/qt/helper.h"

using namespace tl2_ops;


void StructFactDlg::AddTabItem(int row,
	const std::string& name, t_real bRe, t_real bIm, t_real x, t_real y, t_real z, t_real scale, const std::string& col)
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
		m_nuclei->setItem(row, COL_SCATLEN_RE, new tl2::NumericTableWidgetItem<t_real>(bRe));
		m_nuclei->setItem(row, COL_SCATLEN_IM, new tl2::NumericTableWidgetItem<t_real>(bIm));
		m_nuclei->setItem(row, COL_X, new tl2::NumericTableWidgetItem<t_real>(x));
		m_nuclei->setItem(row, COL_Y, new tl2::NumericTableWidgetItem<t_real>(y));
		m_nuclei->setItem(row, COL_Z, new tl2::NumericTableWidgetItem<t_real>(z));
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


void StructFactDlg::DelTabItem(int begin, int end)
{
	m_ignoreChanges = 1;

	// if nothing is selected, clear all items
	if(begin == -1 || m_nuclei->selectedItems().count() == 0)
	{
		if(m_plot)
		{
			for(int row=0; row<m_nuclei->rowCount(); ++row)
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole).toUInt(); obj)
					m_plot->GetRenderer()->RemoveObject(obj);
			m_plot->update();
		}

		m_nuclei->clearContents();
		m_nuclei->setRowCount(0);
	}
	else if(begin == -2)	// clear selected
	{
		for(int row : GetSelectedRows(true))
		{
			// remove 3d object
			if(m_plot)
			{
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole).toUInt(); obj)
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
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole).toUInt(); obj)
					m_plot->GetRenderer()->RemoveObject(obj);
				m_plot->update();
			}

			m_nuclei->removeRow(row);
		}
	}

	m_ignoreChanges = 0;
	Calc();
}


void StructFactDlg::MoveTabItemUp()
{
	m_ignoreChanges = 1;
	m_nuclei->setSortingEnabled(false);

	auto selected = GetSelectedRows(false);
	for(int row : selected)
	{
		if(row == 0)
			continue;

		auto *item = m_nuclei->item(row, 0);
		if(!item || !item->isSelected())
			continue;

		m_nuclei->insertRow(row-1);
		for(int col=0; col<m_nuclei->columnCount(); ++col)
			m_nuclei->setItem(row-1, col, m_nuclei->item(row+1, col)->clone());
		m_nuclei->removeRow(row+1);
	}

	for(int row=0; row<m_nuclei->rowCount(); ++row)
	{
		if(auto *item = m_nuclei->item(row, 0);
			item && std::find(selected.begin(), selected.end(), row+1) != selected.end())
		{
			for(int col=0; col<m_nuclei->columnCount(); ++col)
				m_nuclei->item(row, col)->setSelected(true);
		}
	}

	m_ignoreChanges = 0;
}


void StructFactDlg::MoveTabItemDown()
{
	m_ignoreChanges = 1;
	m_nuclei->setSortingEnabled(false);

	auto selected = GetSelectedRows(true);
	for(int row : selected)
	{
		if(row == m_nuclei->rowCount()-1)
			continue;

		auto *item = m_nuclei->item(row, 0);
		if(!item || !item->isSelected())
			continue;

		m_nuclei->insertRow(row+2);
		for(int col=0; col<m_nuclei->columnCount(); ++col)
			m_nuclei->setItem(row+2, col, m_nuclei->item(row, col)->clone());
		m_nuclei->removeRow(row);
	}

	for(int row=0; row<m_nuclei->rowCount(); ++row)
	{
		if(auto *item = m_nuclei->item(row, 0);
			item && std::find(selected.begin(), selected.end(), row-1) != selected.end())
		{
			for(int col=0; col<m_nuclei->columnCount(); ++col)
				m_nuclei->item(row, col)->setSelected(true);
		}
	}

	m_ignoreChanges = 0;
}


std::vector<int> StructFactDlg::GetSelectedRows(bool sort_reversed) const
{
	std::vector<int> vec;
	vec.reserve(m_nuclei->selectedItems().size());

	for(int row=0; row<m_nuclei->rowCount(); ++row)
	{
		if(auto *item = m_nuclei->item(row, 0); item && item->isSelected())
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
void StructFactDlg::TableCurCellChanged(
	[[maybe_unused]] int rowNew,
	[[maybe_unused]] int colNew,
	[[maybe_unused]] int rowOld,
	[[maybe_unused]] int colOld)
{
}


/**
 * hovered over new row
 */
void StructFactDlg::TableCellEntered(const QModelIndex&)
{
}


/**
 * item contents changed
 */
void StructFactDlg::TableItemChanged(QTableWidgetItem *item)
{
	// update associated 3d object
	if(item && m_plot)
	{
		int row = item->row();
		if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole).toUInt(); obj)
		{
			auto *itemName = m_nuclei->item(row, COL_NAME);
			auto *itemx = m_nuclei->item(row, COL_X);
			auto *itemy = m_nuclei->item(row, COL_Y);
			auto *itemz = m_nuclei->item(row, COL_Z);
			auto *itemsc = m_nuclei->item(row, COL_RAD);
			auto *itemCol = m_nuclei->item(row, COL_COL);

			t_real_gl posx=0, posy=0, posz=0, scale=1;
			std::istringstream{itemx->text().toStdString()} >> posx;
			std::istringstream{itemy->text().toStdString()} >> posy;
			std::istringstream{itemz->text().toStdString()} >> posz;
			std::istringstream{itemsc->text().toStdString()} >> scale;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
			qreal r=1, g=1, b=1;
#else
			float r=1, g=1, b=1;
#endif
			QColor col{itemCol->text()};
			col.getRgbF(&r, &g, &b);

			m_plot->GetRenderer()->SetObjectMatrix(obj, tl2::hom_translation<t_mat_gl>(posx, posy, posz)*tl2::hom_scaling<t_mat_gl>(scale,scale,scale));
			m_plot->GetRenderer()->SetObjectCol(obj, r, g, b, 1);
			m_plot->GetRenderer()->SetObjectLabel(obj, itemName->text().toStdString());
			m_plot->update();
		}
	}

	if(!m_ignoreChanges)
		Calc();
}


void StructFactDlg::ShowTableContextMenu(const QPoint& pt)
{
	auto ptGlob = m_nuclei->mapToGlobal(pt);

	if(const auto* item = m_nuclei->itemAt(pt); item)
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
 * generate symmetric nuclei from space group
 */
void StructFactDlg::GenerateFromSG()
{
	m_ignoreCalc = 1;

	try
	{
		// symops of current space group
		auto sgidx = m_comboSG->itemData(m_comboSG->currentIndex()).toInt();
		if(sgidx < 0 || std::size_t(sgidx) >= m_SGops.size())
		{
			QMessageBox::critical(this, "Structure Factors", "Invalid space group selected.");
			m_ignoreCalc = 0;
			return;
		}

		const auto& ops = m_SGops[sgidx];
		std::vector<std::tuple<std::string, t_real, t_real, t_real, t_real, t_real, t_real, std::string>> generatednuclei;

		// iterate nuclei
		int orgRowCnt = m_nuclei->rowCount();
		for(int row=0; row<orgRowCnt; ++row)
		{
			t_real bRe{}, bIm{}, x{},y{},z{}, scale{};
			std::istringstream{m_nuclei->item(row, COL_SCATLEN_RE)->text().toStdString()} >> bRe;
			std::istringstream{m_nuclei->item(row, COL_SCATLEN_IM)->text().toStdString()} >> bIm;
			std::istringstream{m_nuclei->item(row, COL_X)->text().toStdString()} >> x;
			std::istringstream{m_nuclei->item(row, COL_Y)->text().toStdString()} >> y;
			std::istringstream{m_nuclei->item(row, COL_Z)->text().toStdString()} >> z;
			std::istringstream{m_nuclei->item(row, COL_RAD)->text().toStdString()} >> scale;
			std::string name = m_nuclei->item(row, COL_NAME)->text().toStdString();
			std::string col = m_nuclei->item(row, COL_COL)->text().toStdString();

			t_vec nucl = tl2::create<t_vec>({x, y, z, 1});
			auto newnuclei = tl2::apply_ops_hom<t_vec, t_mat, t_real>(nucl, ops, g_eps);

			for(const auto& newnucl : newnuclei)
			{
				//AddTabItem(-1, name, bRe, bIm, newnucl[0], newnucl[1], newnucl[2], scale, col);
				generatednuclei.emplace_back(std::make_tuple(name, bRe, bIm, newnucl[0], newnucl[1], newnucl[2], scale, col));
			}
		}

		// remove original nuclei
		//DelTabItem(0, orgRowCnt);
		DelTabItem(-1);

		// add new nuclei
		for(const auto& nucl : generatednuclei)
			std::apply(&StructFactDlg::AddTabItem, std::tuple_cat(std::make_tuple(this, -1), nucl));
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Structure Factors", ex.what());
	}


	m_ignoreCalc = 0;
	CalcB(false);
	Calc();
}


/**
 * reads nuclei positions from table
 */
std::vector<NuclPos> StructFactDlg::GetNuclei() const
{
	std::vector<NuclPos> vec;

	for(int row=0; row<m_nuclei->rowCount(); ++row)
	{
		auto *name = m_nuclei->item(row, COL_NAME);
		auto *bRe = m_nuclei->item(row, COL_SCATLEN_RE);
		auto *bIm = m_nuclei->item(row, COL_SCATLEN_IM);
		auto *x = m_nuclei->item(row, COL_X);
		auto *y = m_nuclei->item(row, COL_Y);
		auto *z = m_nuclei->item(row, COL_Z);

		if(!name || !bRe || !bIm || !x || !y || !z)
		{
			std::cerr << "Invalid entry in row " << row << "." << std::endl;
			continue;
		}

		NuclPos nucl;
		t_real _bRe, _bIm;
		nucl.name = name->text().toStdString();
		std::istringstream{bRe->text().toStdString()} >> _bRe;
		std::istringstream{bIm->text().toStdString()} >> _bIm;
		std::istringstream{x->text().toStdString()} >> nucl.pos[0];
		std::istringstream{y->text().toStdString()} >> nucl.pos[1];
		std::istringstream{z->text().toStdString()} >> nucl.pos[2];
		nucl.b = t_cplx{_bRe, _bIm};

		vec.emplace_back(std::move(nucl));
	}

	return vec;
}



// ----------------------------------------------------------------------------
struct PowderLine
{
        t_real Q{};
        t_real I{};
        std::size_t num_peaks = 0;
        std::string peaks;
};


/**
 * calculate crystal B matrix
 */
void StructFactDlg::CalcB(bool bFullRecalc)
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

	m_crystB = tl2::B_matrix<t_mat>(a, b, c,
		alpha/180.*tl2::pi<t_real>, beta/180.*tl2::pi<t_real>, gamma/180.*tl2::pi<t_real>);

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
	if(bFullRecalc)
		Calc();
}


/**
 * calculate structure factors
 */
void StructFactDlg::Calc()
{
	if(m_ignoreCalc)
		return;

	const auto maxBZ = m_maxBZ->value();
	const bool remove_zeroes = m_RemoveZeroes->isChecked();


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

	for(const auto& nucl : GetNuclei())
	{
		bs.push_back(nucl.b);
		pos.emplace_back(tl2::create<t_vec>({ nucl.pos[0], nucl.pos[1], nucl.pos[2] }));
	}


	std::ostringstream ostr, ostrPowder;
	ostr.precision(g_prec);
	ostrPowder.precision(g_prec);

	ostr << "# Nuclear single-crystal structure factors:" << "\n";
	ostr << "# "
		<< std::setw(g_prec*1.2-2) << std::right << "h" << " "
		<< std::setw(g_prec*1.2) << std::right << "k" << " "
		<< std::setw(g_prec*1.2) << std::right << "l" << " "
		<< std::setw(g_prec*2) << std::right << "|Q| (1/A)" << " "
		<< std::setw(g_prec*2) << std::right << "|Fn|^2" << " "
		<< std::setw(g_prec*5) << std::right << "Fn (fm)" << "\n";

	ostrPowder << "# Nuclear powder lines:" << "\n";
	ostrPowder << "# "
		<< std::setw(g_prec*2-2) << std::right << "|Q| (1/A)" << " "
		<< std::setw(g_prec*2) << std::right << "|F|^2" << " "
		<< std::setw(g_prec*2) << std::right << "Mult." << "\n";


	for(t_real h=-maxBZ; h<=maxBZ; ++h)
	{
		for(t_real k=-maxBZ; k<=maxBZ; ++k)
		{
			for(t_real l=-maxBZ; l<=maxBZ; ++l)
			{
				auto Q = tl2::create<t_vec>({h,k,l}) /*+ prop*/;
				auto Q_invA = m_crystB * Q;
				auto Qabs_invA = tl2::norm(Q_invA);

				// nuclear structure factor
				auto Fn = tl2::structure_factor<t_vec, t_cplx>(bs, pos, Q);
				if(tl2::equals<t_cplx>(Fn, t_cplx(0), g_eps)) Fn = 0.;
				if(tl2::equals<t_real>(Fn.real(), 0, g_eps)) Fn.real(0.);
				if(tl2::equals<t_real>(Fn.imag(), 0, g_eps)) Fn.imag(0.);
				auto I = (std::conj(Fn)*Fn).real();

				if(remove_zeroes && tl2::equals<t_cplx>(Fn, t_cplx(0), g_eps))
					continue;

				add_powderline(Qabs_invA, I, h,k,l);

				ostr
					<< std::setw(g_prec*1.2) << std::right << h << " "
					<< std::setw(g_prec*1.2) << std::right << k << " "
					<< std::setw(g_prec*1.2) << std::right << l << " "
					<< std::setw(g_prec*2) << std::right << Qabs_invA << " "
					<< std::setw(g_prec*2) << std::right << I << " "
					<< std::setw(g_prec*5) << std::right << Fn << "\n";
			}
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
}
// ----------------------------------------------------------------------------
