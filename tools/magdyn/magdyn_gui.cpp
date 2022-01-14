/**
 * magnon dynamics
 * @author Tobias Weber <tweber@ill.fr>
 * @date Jan-2022
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

#include "magdyn_gui.h"

#include <QtCore/QDir>
#include <QtGui/QFontDatabase>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <iostream>
#include <fstream>

#include <boost/version.hpp>
#include <boost/config.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace pt = boost::property_tree;

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
namespace algo = boost::algorithm;

#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/phys.h"
#include "tlibs2/libs/algos.h"
#include "tlibs2/libs/helper.h"

using namespace tl2_ops;

constexpr t_real g_eps = 1e-6;
constexpr int g_prec = 6;
constexpr int g_prec_gui = 3;


// columns of exchange terms table
enum : int
{
	COL_NAME = 0,
	COL_ATOM1_IDX, COL_ATOM2_IDX,
	COL_DIST_X, COL_DIST_Y, COL_DIST_Z,
	COL_INTERACTION,

	NUM_COLS
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

		m_termstab->verticalHeader()->setDefaultSectionSize(fontMetrics().lineSpacing() + 4);
		m_termstab->verticalHeader()->setVisible(false);

		m_termstab->setColumnCount(NUM_COLS);
		m_termstab->setHorizontalHeaderItem(COL_NAME, new QTableWidgetItem{"Name"});
		m_termstab->setHorizontalHeaderItem(COL_ATOM1_IDX, new QTableWidgetItem{"Atom 1"});
		m_termstab->setHorizontalHeaderItem(COL_ATOM2_IDX, new QTableWidgetItem{"Atom 2"});
		m_termstab->setHorizontalHeaderItem(COL_DIST_X, new QTableWidgetItem{"Cell Δx"});
		m_termstab->setHorizontalHeaderItem(COL_DIST_Y, new QTableWidgetItem{"Cell Δy"});
		m_termstab->setHorizontalHeaderItem(COL_DIST_Z, new QTableWidgetItem{"Cell Δz"});
		m_termstab->setHorizontalHeaderItem(COL_INTERACTION, new QTableWidgetItem{"Bond J"});

		m_termstab->setColumnWidth(COL_NAME, 90);
		m_termstab->setColumnWidth(COL_ATOM1_IDX, 80);
		m_termstab->setColumnWidth(COL_ATOM2_IDX, 80);
		m_termstab->setColumnWidth(COL_DIST_X, 80);
		m_termstab->setColumnWidth(COL_DIST_Y, 80);
		m_termstab->setColumnWidth(COL_DIST_Z, 80);
		m_termstab->setColumnWidth(COL_INTERACTION, 80);

		QToolButton *pTabBtnAdd = new QToolButton(m_termspanel);
		QToolButton *pTabBtnDel = new QToolButton(m_termspanel);
		QToolButton *pTabBtnUp = new QToolButton(m_termspanel);
		QToolButton *pTabBtnDown = new QToolButton(m_termspanel);

		m_termstab->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});
		pTabBtnAdd->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDel->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnUp->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDown->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});

		pTabBtnAdd->setText("Add Term");
		pTabBtnDel->setText("Delete Term");
		pTabBtnUp->setText("Move Term Up");
		pTabBtnDown->setText("Move Term Down");


		auto pTabGrid = new QGridLayout(m_termspanel);
		pTabGrid->setSpacing(2);
		pTabGrid->setContentsMargins(4,4,4,4);

		int y = 0;
		pTabGrid->addWidget(m_termstab, y,0,1,4);
		pTabGrid->addWidget(pTabBtnAdd, ++y,0,1,1);
		pTabGrid->addWidget(pTabBtnDel, y,1,1,1);
		pTabGrid->addWidget(pTabBtnUp, y,2,1,1);
		pTabGrid->addWidget(pTabBtnDown, y,3,1,1);


		//auto sep1 = new QFrame(m_termspanel); sep1->setFrameStyle(QFrame::HLine);
		//pTabGrid->addWidget(sep1, ++y,0, 1,4);


		// table CustomContextMenu
		QMenu *pTabContextMenu = new QMenu(m_termstab);
		pTabContextMenu->addAction("Add Term Before", this, [this]() { this->AddTabItem(-2); });
		pTabContextMenu->addAction("Add Term After", this, [this]() { this->AddTabItem(-3); });
		pTabContextMenu->addAction("Clone Term", this, [this]() { this->AddTabItem(-4); });
		pTabContextMenu->addAction("Delete Term", this, [this]() { this->DelTabItem(); });


		// table CustomContextMenu in case nothing is selected
		QMenu *pTabContextMenuNoItem = new QMenu(m_termstab);
		pTabContextMenuNoItem->addAction("Add Term", this, [this]() { this->AddTabItem(); });
		pTabContextMenuNoItem->addAction("Delete Term", this, [this]() { this->DelTabItem(); });
		//pTabContextMenuNoItem->addSeparator();


		// signals
		connect(pTabBtnAdd, &QToolButton::clicked, this, [this]() { this->AddTabItem(-1); });
		connect(pTabBtnDel, &QToolButton::clicked, this, [this]() { this->DelTabItem(); });
		connect(pTabBtnUp, &QToolButton::clicked, this, [this]() { this->MoveTabItemUp(m_termstab); });
		connect(pTabBtnDown, &QToolButton::clicked, this, [this]() { this->MoveTabItemDown(m_termstab); });

		connect(m_termstab, &QTableWidget::currentCellChanged, this, &MagDynDlg::TableCurCellChanged);
		connect(m_termstab, &QTableWidget::entered, this, &MagDynDlg::TableCellEntered);
		connect(m_termstab, &QTableWidget::itemChanged, this, &MagDynDlg::TableItemChanged);
		connect(m_termstab, &QTableWidget::customContextMenuRequested, this,
			[this, pTabContextMenu, pTabContextMenuNoItem](const QPoint& pt)
			{ this->ShowTableContextMenu(m_termstab, pTabContextMenu, pTabContextMenuNoItem, pt); });

		tabs->addTab(m_termspanel, "Coupling Terms");
	}


	// dispersion panel
	{
		m_disppanel = new QWidget(this);

		m_plot = new QCustomPlot(m_disppanel);
		m_plot->xAxis->setLabel("Q (rlu)");
		m_plot->yAxis->setLabel("E (meV)");
		m_plot->setInteraction(QCP::iRangeDrag, true);
		m_plot->setInteraction(QCP::iRangeZoom, true);
		m_plot->setSelectionRectMode(QCP::srmZoom);
		m_plot->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});

		m_spin_q_start[0] = new QDoubleSpinBox(m_disppanel);
		m_spin_q_start[1] = new QDoubleSpinBox(m_disppanel);
		m_spin_q_start[2] = new QDoubleSpinBox(m_disppanel);
		m_spin_q_end[0] = new QDoubleSpinBox(m_disppanel);
		m_spin_q_end[1] = new QDoubleSpinBox(m_disppanel);
		m_spin_q_end[2] = new QDoubleSpinBox(m_disppanel);

		m_num_points = new QSpinBox(m_disppanel);
		m_num_points->setMinimum(1);
		m_num_points->setMaximum(9999);
		m_num_points->setValue(512);
		m_num_points->setSizePolicy(
			QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});

		for(int i=0; i<3; ++i)
		{
			m_spin_q_start[i]->setDecimals(2);
			m_spin_q_end[i]->setDecimals(2);
			m_spin_q_start[i]->setMinimum(-99);
			m_spin_q_end[i]->setMinimum(-99);
			m_spin_q_start[i]->setMaximum(+99);
			m_spin_q_end[i]->setMaximum(+99);
			m_spin_q_start[i]->setSingleStep(0.1);
			m_spin_q_end[i]->setSingleStep(0.1);
			m_spin_q_start[i]->setValue(0.);
			m_spin_q_end[i]->setValue(0.);
			m_spin_q_start[i]->setSizePolicy(
				QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
			m_spin_q_end[i]->setSizePolicy(
				QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		}

		m_spin_q_start[0]->setValue(-1.);
		m_spin_q_end[0]->setValue(+1.);

		auto pTabGrid = new QGridLayout(m_disppanel);
		pTabGrid->setSpacing(2);
		pTabGrid->setContentsMargins(4,4,4,4);

		int y = 0;
		pTabGrid->addWidget(m_plot, y++,0,1,4);
		pTabGrid->addWidget(new QLabel(QString("Starting Q:"), m_disppanel), y,0,1,1);
		pTabGrid->addWidget(m_spin_q_start[0], y,1,1,1);
		pTabGrid->addWidget(m_spin_q_start[1], y,2,1,1);
		pTabGrid->addWidget(m_spin_q_start[2], y++,3,1,1);
		pTabGrid->addWidget(new QLabel(QString("Ending Q:"), m_disppanel), y,0,1,1);
		pTabGrid->addWidget(m_spin_q_end[0], y,1,1,1);
		pTabGrid->addWidget(m_spin_q_end[1], y,2,1,1);
		pTabGrid->addWidget(m_spin_q_end[2], y++,3,1,1);
		pTabGrid->addWidget(new QLabel(QString("Number of Qs:"), m_disppanel), y,0,1,1);
		pTabGrid->addWidget(m_num_points, y++,1,1,1);

		// signals
		for(int i=0; i<3; ++i)
		{
			connect(m_spin_q_start[i],
				static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
				[this]() { this->CalcDispersion(); });
			connect(m_spin_q_end[i],
				static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
					[this]() { this->CalcDispersion(); });
		}

		connect(m_num_points,
			static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
			[this]() { this->CalcDispersion(); });

		connect(m_plot, &QCustomPlot::mouseMove,
			this, &MagDynDlg::PlotMouseMove);

		tabs->addTab(m_disppanel, "Dispersion");
	}


	{	// info panel
		auto infopanel = new QWidget(this);
		auto pGrid = new QGridLayout(infopanel);
		pGrid->setSpacing(4);
		pGrid->setContentsMargins(4,4,4,4);

		auto sep1 = new QFrame(infopanel); sep1->setFrameStyle(QFrame::HLine);
		auto sep2 = new QFrame(infopanel); sep2->setFrameStyle(QFrame::HLine);
		auto sep3 = new QFrame(infopanel); sep3->setFrameStyle(QFrame::HLine);

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
		pGrid->addItem(new QSpacerItem(16,16, QSizePolicy::Minimum, QSizePolicy::Expanding), y++,0, 1,1);

		tabs->addTab(infopanel, "Infos");
	}

	// status
	m_status = new QLabel(this);
	m_status->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	m_status->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	// main grid
	auto pmainGrid = new QGridLayout(this);
	pmainGrid->setSpacing(4);
	pmainGrid->setContentsMargins(4,4,4,4);
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

		auto menuPlot = new QMenu("Plot", m_menu);
		auto acSaveFigure = new QAction("Save Figure...", menuPlot);
		auto acRescalePlot = new QAction("Rescale Axes", menuPlot);

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
		menuFile->addAction(acExit);

		menuPlot->addAction(acRescalePlot);
		menuPlot->addAction(acSaveFigure);

		connect(acNew, &QAction::triggered, this,  [this]()
		{
			// clear old tables
			DelTabItem(-1);
		});

		connect(acLoad, &QAction::triggered, this, &MagDynDlg::Load);
		connect(acSave, &QAction::triggered, this, &MagDynDlg::Save);
		connect(acExit, &QAction::triggered, this, &QDialog::close);

		connect(acSaveFigure, &QAction::triggered, this, &MagDynDlg::SavePlotFigure);
		connect(acRescalePlot, &QAction::triggered, [this]()
		{
			if(!m_plot)
				return;

			m_plot->rescaleAxes();
			m_plot->replot();
		});

		m_menu->addMenu(menuFile);
		m_menu->addMenu(menuPlot);
		pmainGrid->setMenuBar(m_menu);
	}


	// restory window size and position
	if(m_sett && m_sett->contains("geo"))
		restoreGeometry(m_sett->value("geo").toByteArray());
	else
		resize(600, 500);


	m_ignoreChanges = 0;
}


void MagDynDlg::AddTabItem(int row,
	const std::string& name,
	t_size atom_1, t_size atom_2,
	t_real dist_x, t_real dist_y, t_real dist_z,
	t_real J)
{
	bool bclone = 0;
	m_ignoreChanges = 1;

	if(row == -1)	// append to end of table
		row = m_termstab->rowCount();
	else if(row == -2 && m_iCursorRow >= 0)	// use row from member variable
		row = m_iCursorRow;
	else if(row == -3 && m_iCursorRow >= 0)	// use row from member variable +1
		row = m_iCursorRow + 1;
	else if(row == -4 && m_iCursorRow >= 0)	// use row from member variable +1
	{
		row = m_iCursorRow + 1;
		bclone = 1;
	}

	//bool sorting = m_termstab->isSortingEnabled();
	m_termstab->setSortingEnabled(false);
	m_termstab->insertRow(row);

	if(bclone)
	{
		for(int thecol=0; thecol<NUM_COLS; ++thecol)
			m_termstab->setItem(row, thecol, m_termstab->item(m_iCursorRow, thecol)->clone());
	}
	else
	{
		m_termstab->setItem(row, COL_NAME, new QTableWidgetItem(name.c_str()));
		m_termstab->setItem(row, COL_ATOM1_IDX, new NumericTableWidgetItem<t_size>(atom_1));
		m_termstab->setItem(row, COL_ATOM2_IDX, new NumericTableWidgetItem<t_size>(atom_2));
		m_termstab->setItem(row, COL_DIST_X, new NumericTableWidgetItem<t_real>(dist_x));
		m_termstab->setItem(row, COL_DIST_Y, new NumericTableWidgetItem<t_real>(dist_y));
		m_termstab->setItem(row, COL_DIST_Z, new NumericTableWidgetItem<t_real>(dist_z));
		m_termstab->setItem(row, COL_INTERACTION, new NumericTableWidgetItem<t_real>(J));
	}

	m_termstab->scrollToItem(m_termstab->item(row, 0));
	m_termstab->setCurrentCell(row, 0);

	m_termstab->setSortingEnabled(/*sorting*/ true);

	m_ignoreChanges = 0;
	CalcExchangeTerms();
}


void MagDynDlg::DelTabItem(int begin, int end)
{
	m_ignoreChanges = 1;

	// if nothing is selected, clear all items
	if(begin == -1 || m_termstab->selectedItems().count() == 0)
	{
		m_termstab->clearContents();
		m_termstab->setRowCount(0);
	}
	else if(begin == -2)	// clear selected
	{
		for(int row : GetSelectedRows(m_termstab, true))
		{
			m_termstab->removeRow(row);
		}
	}
	else if(begin >= 0 && end >= 0)		// clear given range
	{
		for(int row=end-1; row>=begin; --row)
		{
			m_termstab->removeRow(row);
		}
	}

	m_ignoreChanges = 0;
	CalcExchangeTerms();
}


void MagDynDlg::MoveTabItemUp(QTableWidget *pTab)
{
	m_ignoreChanges = 1;

	pTab->setSortingEnabled(false);

	auto selected = GetSelectedRows(m_termstab, false);
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


void MagDynDlg::MoveTabItemDown(QTableWidget *pTab)
{
	m_ignoreChanges = 1;

	pTab->setSortingEnabled(false);

	auto selected = GetSelectedRows(m_termstab, true);
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
 * selected a new row
 */
void MagDynDlg::TableCurCellChanged(
	int /*rowNew*/, int /*colNew*/, int /*rowOld*/, int /*colOld*/)
{}


/**
 * hovered over new row
 */
void MagDynDlg::TableCellEntered(const QModelIndex& /*idx*/)
{}


/**
 * item contents changed
 */
void MagDynDlg::TableItemChanged(QTableWidgetItem * /*item*/)
{
	if(!m_ignoreChanges)
		CalcExchangeTerms();
}


void MagDynDlg::ShowTableContextMenu(
	QTableWidget *pTab, QMenu *pMenu, QMenu *pMenuNoItem, const QPoint& pt)
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


/**
 * load settings
 */
void MagDynDlg::Load()
{
	m_ignoreCalc = 1;

	try
	{
		QString dirLast = m_sett->value("dir", "").toString();
		QString filename = QFileDialog::getOpenFileName(this, "Load File", dirLast, "XML Files (*.xml *.XML)");
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

		// settings
		if(auto optVal = node.get_optional<t_real>("magdyn.config.h_start"))
			m_spin_q_start[0]->setValue(*optVal);
		if(auto optVal = node.get_optional<t_real>("magdyn.config.k_start"))
			m_spin_q_start[1]->setValue(*optVal);
		if(auto optVal = node.get_optional<t_real>("magdyn.config.l_start"))
			m_spin_q_start[2]->setValue(*optVal);
		if(auto optVal = node.get_optional<t_real>("magdyn.config.h_end"))
			m_spin_q_end[0]->setValue(*optVal);
		if(auto optVal = node.get_optional<t_real>("magdyn.config.k_end"))
			m_spin_q_end[1]->setValue(*optVal);
		if(auto optVal = node.get_optional<t_real>("magdyn.config.l_end"))
			m_spin_q_end[2]->setValue(*optVal);
		if(auto optVal = node.get_optional<t_size>("magdyn.config.num_Q_points"))
			m_num_points->setValue(*optVal);

		// clear old tables
		DelTabItem(-1);
		// exchange terms
		if(auto nuclei = node.get_child_optional("magdyn.exchange_terms"); nuclei)
		{
			for(const auto &nucl : *nuclei)
			{
				auto optName = nucl.second.get<std::string>("name", "n/a");
				auto optAtom1 = nucl.second.get<t_size>("atom_1_index", 0);
				auto optAtom2 = nucl.second.get<t_size>("atom_2_index", 0);
				auto optDistX = nucl.second.get<t_real>("distance_x", 0.);
				auto optDistY = nucl.second.get<t_real>("distance_y", 0.);
				auto optDistZ = nucl.second.get<t_real>("distance_z", 0.);
				auto optInteraction = nucl.second.get<t_real>("interaction", 0.);

				AddTabItem(-1,
					optName, optAtom1, optAtom2,
					optDistX, optDistY, optDistZ,
					optInteraction);
			}
		}
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Magnon Dynamics", ex.what());
	}

	// recalculate
	m_ignoreCalc = 0;
	CalcExchangeTerms();
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
	pt::ptree node;
	node.put<std::string>("magdyn.meta.info", "magdyn_tool");
	node.put<std::string>("magdyn.meta.date", tl2::epoch_to_str<t_real>(tl2::epoch<t_real>()));

	// settings
	node.put<t_real>("magdyn.config.h_start", m_spin_q_start[0]->value());
	node.put<t_real>("magdyn.config.k_start", m_spin_q_start[1]->value());
	node.put<t_real>("magdyn.config.l_start", m_spin_q_start[2]->value());
	node.put<t_real>("magdyn.config.h_end", m_spin_q_end[0]->value());
	node.put<t_real>("magdyn.config.k_end", m_spin_q_end[1]->value());
	node.put<t_real>("magdyn.config.l_end", m_spin_q_end[2]->value());
	node.put<t_size>("magdyn.config.num_Q_points", m_num_points->value());

	// exchange terms
	for(int row=0; row<m_termstab->rowCount(); ++row)
	{
		t_size atom_1_idx = 0;
		t_size atom_2_idx = 0;
		t_real dist[3]{0., 0., 0.};
		t_real J = 0.;

		std::istringstream{m_termstab->item(row, COL_ATOM1_IDX)->text().toStdString()} >> atom_1_idx;
		std::istringstream{m_termstab->item(row, COL_ATOM2_IDX)->text().toStdString()} >> atom_2_idx;
		std::istringstream{m_termstab->item(row, COL_DIST_X)->text().toStdString()} >> dist[0];
		std::istringstream{m_termstab->item(row, COL_DIST_Y)->text().toStdString()} >> dist[1];
		std::istringstream{m_termstab->item(row, COL_DIST_Z)->text().toStdString()} >> dist[2];
		std::istringstream{m_termstab->item(row, COL_INTERACTION)->text().toStdString()} >> J;

		pt::ptree itemNode;
		itemNode.put<std::string>("name", m_termstab->item(row, COL_NAME)->text().toStdString());
		itemNode.put<t_size>("atom_1_index", atom_1_idx);
		itemNode.put<t_size>("atom_2_index", atom_2_idx);
		itemNode.put<t_real>("distance_x", dist[0]);
		itemNode.put<t_real>("distance_y", dist[1]);
		itemNode.put<t_real>("distance_z", dist[2]);
		itemNode.put<t_real>("interaction", J);

		node.add_child("magdyn.exchange_terms.term", itemNode);
	}

	// save to file
	std::ofstream ofstr{filename.toStdString()};
	if(!ofstr)
	{
		QMessageBox::critical(this, "Magnon Dynamics", "Cannot open file for writing.");
		return;
	}
	ofstr.precision(g_prec);
	pt::write_xml(ofstr, node, pt::xml_writer_make_settings('\t', 1, std::string{"utf-8"}));
}


/**
 * save the plot as pdf
 */
void MagDynDlg::SavePlotFigure()
{
	if(!m_plot)
		return;

	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getSaveFileName(this, "Save Figure", dirLast, "PDf Files (*.pdf *.PDF)");
	if(filename=="")
		return;
	m_sett->setValue("dir", QFileInfo(filename).path());

	m_plot->savePdf(filename);
}


/**
 * get the exchange terms from the table
 */
void MagDynDlg::CalcExchangeTerms()
{
	if(m_ignoreCalc)
		return;

	m_dyn.ClearExchangeTerms();
	t_size num_cells = 0;

	for(int row=0; row<m_termstab->rowCount(); ++row)
	{
		auto *name = m_termstab->item(row, COL_NAME);
		auto *atom_1_idx = m_termstab->item(row, COL_ATOM1_IDX);
		auto *atom_2_idx = m_termstab->item(row, COL_ATOM2_IDX);
		auto *dist_x = m_termstab->item(row, COL_DIST_X);
		auto *dist_y = m_termstab->item(row, COL_DIST_Y);
		auto *dist_z = m_termstab->item(row, COL_DIST_Z);
		auto *interaction = m_termstab->item(row, COL_INTERACTION);

		if(!name || !atom_1_idx || !atom_2_idx || !dist_x || !dist_y || !dist_z || !interaction)
		{
			std::cerr << "Invalid entry in row " << row << "." << std::endl;
			continue;
		}

		ExchangeTerm term;
		term.dist = tl2::zero<t_vec>(3);
		std::istringstream{atom_1_idx->text().toStdString()} >> term.atom1;
		std::istringstream{atom_2_idx->text().toStdString()} >> term.atom2;
		std::istringstream{dist_x->text().toStdString()} >> term.dist[0];
		std::istringstream{dist_y->text().toStdString()} >> term.dist[1];
		std::istringstream{dist_z->text().toStdString()} >> term.dist[2];
		std::istringstream{interaction->text().toStdString()} >> term.J;

		num_cells = std::max({num_cells, term.atom1, term.atom2});
		m_dyn.AddExchangeTerm(std::move(term));
	}

	m_dyn.SetNumCells(num_cells + 1);
	CalcDispersion();
}


/**
 * calculate the dispersion branches
 */
void MagDynDlg::CalcDispersion()
{
	m_plot->clearPlottables();

	const t_real Q_start[]
	{
		m_spin_q_start[0]->value(),
		m_spin_q_start[1]->value(),
		m_spin_q_start[2]->value(),
	};

	const t_real Q_end[]
	{
		m_spin_q_end[0]->value(),
		m_spin_q_end[1]->value(),
		m_spin_q_end[2]->value(),
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

	QVector<t_real> qs_data, Es_data;
	qs_data.reserve(num_pts*10);
	Es_data.reserve(num_pts*10);

	t_real E0 = m_dyn.GetGoldstoneEnergy();

	for(t_size i=0; i<num_pts; ++i)
	{
		t_real Q[]
		{
			std::lerp(Q_start[0], Q_end[0], t_real(i)/t_real(num_pts-1)),
			std::lerp(Q_start[1], Q_end[1], t_real(i)/t_real(num_pts-1)),
			std::lerp(Q_start[2], Q_end[2], t_real(i)/t_real(num_pts-1)),
		};

		auto Es = m_dyn.GetEnergies(Q[0], Q[1], Q[2]);
		for(const auto& E : Es)
		{
			qs_data.push_back(Q[Q_idx]);
			Es_data.push_back(E - E0);
		}
	}

	QCPCurve *curve = new QCPCurve(m_plot->xAxis, m_plot->yAxis);
	QPen pen = curve->pen();
	pen.setColor(QColor(0xff, 0x00, 0x00));
	pen.setWidthF(1.);
	curve->setPen(pen);
	curve->setLineStyle(QCPCurve::lsNone);
	curve->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, 2.));
	curve->setAntialiased(true);
	curve->setData(qs_data, Es_data);

	auto [min_E_iter, max_E_iter] = std::minmax_element(Es_data.begin(), Es_data.end());

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
	if(m_sett)
	{
		m_sett->setValue("geo", saveGeometry());
	}
}


int main(int argc, char** argv)
{
	try
	{
		tl2::set_locales();

		QApplication::addLibraryPath(QString(".") + QDir::separator() + "qtplugins");
		auto app = std::make_unique<QApplication>(argc, argv);
		auto dlg = std::make_unique<MagDynDlg>(nullptr);
		dlg->show();

		return app->exec();
	}
	catch(const std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return 0;
}