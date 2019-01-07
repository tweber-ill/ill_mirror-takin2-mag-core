/**
 * structure factor tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date Dec-2018
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 28-Dec-2018 from the privately developed "misc" project (https://github.com/t-weber/misc).
 */

#include "structfact.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <locale>
#include <iostream>
#include <fstream>

#include <boost/version.hpp>
#include <boost/config.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string/replace.hpp>
namespace algo = boost::algorithm;
namespace pt = boost::property_tree;

#include "libs/algos.h"
#include "libs/_cxx20/math_algos.h"
//using namespace m;
using namespace m_ops;

using t_vec = std::vector<t_real>;
using t_vec_cplx = std::vector<t_cplx>;
using t_mat = m::mat<t_real, std::vector>;
using t_mat_cplx = m::mat<t_cplx, std::vector>;


enum : int
{
	COL_NAME = 0,
	COL_SCATLEN_RE,
	COL_SCATLEN_IM,
	COL_X, COL_Y, COL_Z,

	NUM_COLS
};


struct PowderLine
{
	t_real Q;
	t_real I;
	std::string peaks;
};


// ----------------------------------------------------------------------------
StructFactDlg::StructFactDlg(QWidget* pParent) : QDialog{pParent},
	m_sett{new QSettings{"tobis_stuff", "structfact"}}
{
	setWindowTitle("Structure Factors");
	setSizeGripEnabled(true);
	setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));

	
	auto tabs = new QTabWidget(this);
	{
		m_nucleipanel = new QWidget(this);

		m_nuclei = new QTableWidget(m_nucleipanel);
		m_nuclei->setShowGrid(true);
		m_nuclei->setSortingEnabled(true);
		m_nuclei->setMouseTracking(true);
		m_nuclei->setSelectionBehavior(QTableWidget::SelectRows);
		m_nuclei->setSelectionMode(QTableWidget::ContiguousSelection);
		m_nuclei->setContextMenuPolicy(Qt::CustomContextMenu);

		m_nuclei->verticalHeader()->setDefaultSectionSize(fontMetrics().lineSpacing() + 4);
		m_nuclei->verticalHeader()->setVisible(false);

		m_nuclei->setColumnCount(NUM_COLS);
		m_nuclei->setHorizontalHeaderItem(COL_NAME, new QTableWidgetItem{"Name"});
		m_nuclei->setHorizontalHeaderItem(COL_SCATLEN_RE, new QTableWidgetItem{"Re{b} (fm)"});
		m_nuclei->setHorizontalHeaderItem(COL_SCATLEN_IM, new QTableWidgetItem{"Im{b} (fm)"});
		m_nuclei->setHorizontalHeaderItem(COL_X, new QTableWidgetItem{"x (frac.)"});
		m_nuclei->setHorizontalHeaderItem(COL_Y, new QTableWidgetItem{"y (frac.)"});
		m_nuclei->setHorizontalHeaderItem(COL_Z, new QTableWidgetItem{"z (frac.)"});

		m_nuclei->setColumnWidth(COL_NAME, 90);
		m_nuclei->setColumnWidth(COL_SCATLEN_RE, 75);
		m_nuclei->setColumnWidth(COL_SCATLEN_IM, 75);
		m_nuclei->setColumnWidth(COL_X, 75);
		m_nuclei->setColumnWidth(COL_Y, 75);
		m_nuclei->setColumnWidth(COL_Z, 75);

		QToolButton *pTabBtnAdd = new QToolButton(m_nucleipanel);
		QToolButton *pTabBtnDel = new QToolButton(m_nucleipanel);
		QToolButton *pTabBtnUp = new QToolButton(m_nucleipanel);
		QToolButton *pTabBtnDown = new QToolButton(m_nucleipanel);
		QToolButton *pTabBtnLoad = new QToolButton(m_nucleipanel);
		QToolButton *pTabBtnSave = new QToolButton(m_nucleipanel);
		QToolButton *pTabBtn3DView = new QToolButton(m_nucleipanel);

		m_nuclei->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});
		pTabBtnAdd->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDel->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnUp->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDown->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnLoad->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnSave->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtn3DView->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});

		pTabBtnAdd->setText("Add Nucleus");
		pTabBtnDel->setText("Delete Nuclei");
		pTabBtnUp->setText("Move Nuclei Up");
		pTabBtnDown->setText("Move Nuclei Down");
		pTabBtnLoad->setText("Load...");
		pTabBtnSave->setText("Save...");
		pTabBtn3DView->setText("3D View...");


		m_editA = new QLineEdit("5", m_nucleipanel);
		m_editB = new QLineEdit("5", m_nucleipanel);
		m_editC = new QLineEdit("5", m_nucleipanel);
		m_editAlpha = new QLineEdit("90", m_nucleipanel);
		m_editBeta = new QLineEdit("90", m_nucleipanel);
		m_editGamma = new QLineEdit("90", m_nucleipanel);


		auto pTabGrid = new QGridLayout(m_nucleipanel);
		pTabGrid->setSpacing(2);
		pTabGrid->setContentsMargins(4,4,4,4);
		int y=0;
		//pTabGrid->addWidget(m_plot.get(), y,0,1,4);
		pTabGrid->addWidget(m_nuclei, y,0,1,4);
		pTabGrid->addWidget(pTabBtnAdd, ++y,0,1,1);
		pTabGrid->addWidget(pTabBtnDel, y,1,1,1);
		pTabGrid->addWidget(pTabBtnUp, y,2,1,1);
		pTabGrid->addWidget(pTabBtnDown, y,3,1,1);
		pTabGrid->addWidget(pTabBtnLoad, ++y,0,1,1);
		pTabGrid->addWidget(pTabBtnSave, y,1,1,1);
		pTabGrid->addWidget(pTabBtn3DView, y,3,1,1);

		auto sep1 = new QFrame(m_nucleipanel); sep1->setFrameStyle(QFrame::HLine);
		pTabGrid->addWidget(sep1, ++y,0, 1,4);

		pTabGrid->addWidget(new QLabel("Lattice (A):"), ++y,0,1,1);
		pTabGrid->addWidget(m_editA, y,1,1,1);
		pTabGrid->addWidget(m_editB, y,2,1,1);
		pTabGrid->addWidget(m_editC, y,3,1,1);
		pTabGrid->addWidget(new QLabel("Angles (deg):"), ++y,0,1,1);
		pTabGrid->addWidget(m_editAlpha, y,1,1,1);
		pTabGrid->addWidget(m_editBeta, y,2,1,1);
		pTabGrid->addWidget(m_editGamma, y,3,1,1);


		// table CustomContextMenu
		m_pTabContextMenu = new QMenu(m_nuclei);
		m_pTabContextMenu->addAction("Add Nucleus Before", this, [this]() { this->AddTabItem(-2); });
		m_pTabContextMenu->addAction("Add Nucleus After", this, [this]() { this->AddTabItem(-3); });
		m_pTabContextMenu->addAction("Clone Nucleus", this, [this]() { this->AddTabItem(-4); });
		m_pTabContextMenu->addAction("Delete Nucleus", this, &StructFactDlg::DelTabItem);


		// table CustomContextMenu in case nothing is selected
		m_pTabContextMenuNoItem = new QMenu(m_nuclei);
		m_pTabContextMenuNoItem->addAction("Add Nucleus", this, [this]() { this->AddTabItem(); });
		m_pTabContextMenuNoItem->addAction("Delete Nucleus", this, &StructFactDlg::DelTabItem);
		//m_pTabContextMenuNoItem->addSeparator();


		// signals
		connect(pTabBtnAdd, &QToolButton::clicked, this, [this]()
		{
			this->AddTabItem(-1);
		});
		connect(pTabBtnDel, &QToolButton::clicked, this, &StructFactDlg::DelTabItem);
		connect(pTabBtnUp, &QToolButton::clicked, this, &StructFactDlg::MoveTabItemUp);
		connect(pTabBtnDown, &QToolButton::clicked, this, &StructFactDlg::MoveTabItemDown);
		connect(pTabBtnLoad, &QToolButton::clicked, this, &StructFactDlg::Load);
		connect(pTabBtnSave, &QToolButton::clicked, this, &StructFactDlg::Save);
		connect(pTabBtn3DView, &QToolButton::clicked, this, [this]()
		{
			// plot widget
			if(!m_dlgPlot)
			{
				m_dlgPlot = new QDialog(this);
				m_dlgPlot->setWindowTitle("3D View");

				m_plot = std::make_shared<GlPlot>(this);
				m_plot->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});

				connect(m_plot.get(), &GlPlot::AfterGLInitialisation, this, &StructFactDlg::AfterGLInitialisation);
				connect(m_plot->GetImpl(), &GlPlot_impl::PickerIntersection, this, &StructFactDlg::PickerIntersection);
				connect(m_plot.get(), &GlPlot::MouseDown, this, &StructFactDlg::PlotMouseDown);
				connect(m_plot.get(), &GlPlot::MouseUp, this, &StructFactDlg::PlotMouseUp);

				m_status3D = new QLabel(this);

				auto grid = new QGridLayout(m_dlgPlot);
				grid->setSpacing(2);
				grid->setContentsMargins(4,4,4,4);
				grid->addWidget(m_plot.get(), 0,0,1,1);
				grid->addWidget(m_status3D, 1,0,1,1);

				if(m_sett && m_sett->contains("geo_3dview"))
					m_dlgPlot->restoreGeometry(m_sett->value("geo_3dview").toByteArray());
				else
					m_dlgPlot->resize(500,500);
			}

			m_dlgPlot->show();
			m_dlgPlot->raise();
			m_dlgPlot->focusWidget();
		});

		connect(m_nuclei, &QTableWidget::currentCellChanged, this, &StructFactDlg::TableCurCellChanged);
		connect(m_nuclei, &QTableWidget::entered, this, &StructFactDlg::TableCellEntered);
		connect(m_nuclei, &QTableWidget::itemChanged, this, &StructFactDlg::TableItemChanged);
		connect(m_nuclei, &QTableWidget::customContextMenuRequested, this, &StructFactDlg::ShowTableContextMenu);

		tabs->addTab(m_nucleipanel, "Nuclei");
	}


	{	// structure factors panel
		auto sfactpanel = new QWidget(this);
		auto pGrid = new QGridLayout(sfactpanel);
		pGrid->setSpacing(4);
		pGrid->setContentsMargins(4,4,4,4);

		m_structfacts = new QPlainTextEdit(sfactpanel);
		m_structfacts->setReadOnly(true);
		m_structfacts->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

		m_maxBZ = new QSpinBox(sfactpanel);
		m_maxBZ->setMinimum(0);
		m_maxBZ->setMaximum(99);
		m_maxBZ->setValue(4);

		pGrid->addWidget(m_structfacts, 0,0, 1,4);
		pGrid->addWidget(new QLabel("Max. Order::"), 1,0,1,1);
		pGrid->addWidget(m_maxBZ, 1,1, 1,1);

		// signals
		connect(m_maxBZ, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this]() { this->Calc(); });

		tabs->addTab(sfactpanel, "Structure Factors");
	}


	{	// powder lines panel
		auto powderpanel = new QWidget(this);
		auto pGrid = new QGridLayout(powderpanel);
		pGrid->setSpacing(4);
		pGrid->setContentsMargins(4,4,4,4);

		m_powderlines = new QPlainTextEdit(powderpanel);
		m_powderlines->setReadOnly(true);
		m_powderlines->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
		m_powderlines->setLineWrapMode(QPlainTextEdit::NoWrap);

		pGrid->addWidget(m_powderlines, 0,0, 1,4);

		tabs->addTab(powderpanel, "Powder Lines");
	}


	{	// info panel
		auto infopanel = new QWidget(this);
		auto pGrid = new QGridLayout(infopanel);
		pGrid->setSpacing(4);
		pGrid->setContentsMargins(4,4,4,4);

		// table grid
		for(int i=0; i<4; ++i)
		{
			m_labelGlInfos[i] = new QLabel("", infopanel);
			m_labelGlInfos[i]->setSizePolicy(QSizePolicy::Ignored, m_labelGlInfos[i]->sizePolicy().verticalPolicy());
		}

		auto sep1 = new QFrame(infopanel); sep1->setFrameStyle(QFrame::HLine);
		auto sep2 = new QFrame(infopanel); sep2->setFrameStyle(QFrame::HLine);
		auto sep3 = new QFrame(infopanel); sep3->setFrameStyle(QFrame::HLine);

		std::string strBoost = BOOST_LIB_VERSION;
		algo::replace_all(strBoost, "_", ".");

		auto labelTitle = new QLabel("Structure Factors", infopanel);
		auto fontTitle = labelTitle->font();
		fontTitle.setBold(true);
		labelTitle->setFont(fontTitle);
		labelTitle->setAlignment(Qt::AlignHCenter);

		auto labelAuthor = new QLabel("Written by Tobias Weber <tweber@ill.fr>.", infopanel);
		labelAuthor->setAlignment(Qt::AlignHCenter);

		auto labelDate = new QLabel("December 2018.", infopanel);
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
		for(int i=0; i<4; ++i)
			pGrid->addWidget(m_labelGlInfos[i], y++,0, 1,1);
		pGrid->addItem(new QSpacerItem(16,16, QSizePolicy::Minimum, QSizePolicy::Expanding), y++,0, 1,1);

		tabs->addTab(infopanel, "Infos");
	}


	// main grid
	auto pmainGrid = new QGridLayout(this);
	pmainGrid->setSpacing(4);
	pmainGrid->setContentsMargins(4,4,4,4);
	pmainGrid->addWidget(tabs, 0,0, 1,1);


	// restory window size and position
	if(m_sett && m_sett->contains("geo"))
		restoreGeometry(m_sett->value("geo").toByteArray());
	else
		resize(600, 500);


	m_ignoreChanges = 0;
}



// ----------------------------------------------------------------------------
void StructFactDlg::AddTabItem(int row, 
	const std::string& name, t_real bRe, t_real bIm, t_real x, t_real y, t_real z)
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
		m_nuclei->setItem(row, COL_SCATLEN_RE, new NumericTableWidgetItem<t_real>(bRe));
		m_nuclei->setItem(row, COL_SCATLEN_IM, new NumericTableWidgetItem<t_real>(bIm));
		m_nuclei->setItem(row, COL_X, new NumericTableWidgetItem<t_real>(x));
		m_nuclei->setItem(row, COL_Y, new NumericTableWidgetItem<t_real>(y));
		m_nuclei->setItem(row, COL_Z, new NumericTableWidgetItem<t_real>(z));
	}

	Add3DItem(row);

	m_nuclei->scrollToItem(m_nuclei->item(row, 0));
	m_nuclei->setCurrentCell(row, 0);

	m_nuclei->setSortingEnabled(/*sorting*/ true);

	m_ignoreChanges = 0;
	Calc();
}


/**
 * add 3d object
 */
void StructFactDlg::Add3DItem(int row)
{
	if(!m_plot) return;

	// add all items
	if(row < 0)
	{
		for(int row=0; row<m_nuclei->rowCount(); ++row)
			Add3DItem(row);
		return;
	}

	auto *itemx = m_nuclei->item(row, COL_X);
	auto *itemy = m_nuclei->item(row, COL_Y);
	auto *itemz = m_nuclei->item(row, COL_Z);
	t_real_gl posx, posy, posz;
	std::istringstream{itemx->text().toStdString()} >> posx;
	std::istringstream{itemy->text().toStdString()} >> posy;
	std::istringstream{itemz->text().toStdString()} >> posz;

	auto obj = m_plot->GetImpl()->AddLinkedObject(m_sphere);
	m_plot->GetImpl()->SetObjectMatrix(obj, m::hom_translation<t_mat_gl>(posx, posy, posz));
	m_plot->update();

	m_nuclei->item(row, COL_NAME)->setData(Qt::UserRole, unsigned(obj));
}


void StructFactDlg::DelTabItem(bool clearAll)
{
	m_ignoreChanges = 1;

	// if nothing is selected, clear all items
	if(clearAll || m_nuclei->selectedItems().count() == 0)
	{
		if(m_plot)
		{
			for(int row=0; row<m_nuclei->rowCount(); ++row)
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole).toUInt(); obj)
					m_plot->GetImpl()->RemoveObject(obj);
			m_plot->update();
		}

		m_nuclei->clearContents();
		m_nuclei->setRowCount(0);
	}
	else	// clear selected
	{
		for(int row : GetSelectedRows(true))
		{
			// remove 3d object
			if(m_plot)
			{
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole).toUInt(); obj)
					m_plot->GetImpl()->RemoveObject(obj);
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
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
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
void StructFactDlg::TableCurCellChanged(int rowNew, int colNew, int rowOld, int colOld)
{
}


/**
 * hovered over new row
 */
void StructFactDlg::TableCellEntered(const QModelIndex& idx)
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
			auto *itemx = m_nuclei->item(row, COL_X);
			auto *itemy = m_nuclei->item(row, COL_Y);
			auto *itemz = m_nuclei->item(row, COL_Z);
			t_real_gl posx, posy, posz;
			std::istringstream{itemx->text().toStdString()} >> posx;
			std::istringstream{itemy->text().toStdString()} >> posy;
			std::istringstream{itemz->text().toStdString()} >> posz;

			m_plot->GetImpl()->SetObjectMatrix(obj, m::hom_translation<t_mat_gl>(posx, posy, posz));
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
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
void StructFactDlg::Load()
{
	try
	{
		QString dirLast = m_sett->value("dir", "").toString();
		QString filename = QFileDialog::getOpenFileName(this, "Load File", dirLast, "XML Files (*.xml *.XML)");
		if(filename=="" || !QFile::exists(filename))
			return;
		m_sett->setValue("dir", QFileInfo(filename).path());


		pt::ptree node;

		std::ifstream ifstr{filename.toStdString()};
		pt::read_xml(ifstr, node);

		// check signature
		if(auto opt = node.get_optional<std::string>("sfact.meta.info"); !opt || *opt!=std::string{"sfact_tool"})
		{
			QMessageBox::critical(this, "Structure Factors", "Unrecognised file format.");
			return;
		}


		// clear old nuclei
		DelTabItem(true);

		if(auto opt = node.get_optional<t_real>("sfact.xtal.a"); opt)
		{
			std::ostringstream ostr; ostr << *opt;
			m_editA->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("sfact.xtal.b"); opt)
		{
			std::ostringstream ostr; ostr << *opt;
			m_editB->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("sfact.xtal.c"); opt)
		{
			std::ostringstream ostr; ostr << *opt;
			m_editC->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("sfact.xtal.alpha"); opt)
		{
			std::ostringstream ostr; ostr << *opt;
			m_editAlpha->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("sfact.xtal.beta"); opt)
		{
			std::ostringstream ostr; ostr << *opt;
			m_editBeta->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("sfact.xtal.gamma"); opt)
		{
			std::ostringstream ostr; ostr << *opt;
			m_editGamma->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<int>("sfact.order"); opt)
		{
			m_maxBZ->setValue(*opt);
		}


		// nuclei
		if(auto nuclei = node.get_child_optional("sfact.nuclei"); nuclei)
		{
			for(const auto &nucl : *nuclei)
			{
				auto optName = nucl.second.get<std::string>("name", "n/a");
				auto optbRe = nucl.second.get<t_real>("b_Re", 0.);
				auto optbIm = nucl.second.get<t_real>("b_Im", 0.);
				auto optX = nucl.second.get<t_real>("x", 0.);
				auto optY = nucl.second.get<t_real>("y", 0.);
				auto optZ = nucl.second.get<t_real>("z", 0.);

				AddTabItem(-1, optName, optbRe, optbIm, optX,  optY, optZ);
			}
		}
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Structure Factors", ex.what());
	}
}


void StructFactDlg::Save()
{
	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getSaveFileName(this, "Save File", dirLast, "XML Files (*.xml *.XML)");
	if(filename=="")
		return;
	m_sett->setValue("dir", QFileInfo(filename).path());


	pt::ptree node;
	node.put<std::string>("sfact.meta.info", "sfact_tool");
	node.put<std::string>("sfact.meta.date", tl2::epoch_to_str<t_real>(tl2::epoch<t_real>()));


	// lattice
	t_real a,b,c, alpha,beta,gamma;
	std::istringstream{m_editA->text().toStdString()} >> a;
	std::istringstream{m_editB->text().toStdString()} >> b;
	std::istringstream{m_editC->text().toStdString()} >> c;
	std::istringstream{m_editAlpha->text().toStdString()} >> alpha;
	std::istringstream{m_editBeta->text().toStdString()} >> beta;
	std::istringstream{m_editGamma->text().toStdString()} >> gamma;

	node.put<t_real>("sfact.xtal.a", a);
	node.put<t_real>("sfact.xtal.b", b);
	node.put<t_real>("sfact.xtal.c", c);
	node.put<t_real>("sfact.xtal.alpha", alpha);
	node.put<t_real>("sfact.xtal.beta", beta);
	node.put<t_real>("sfact.xtal.gamma", gamma);
	node.put<int>("sfact.order", m_maxBZ->value());

	// nucleus list
	for(int row=0; row<m_nuclei->rowCount(); ++row)
	{
		t_real bRe,bIm, x,y,z;
		std::istringstream{m_nuclei->item(row, COL_SCATLEN_RE)->text().toStdString()} >> bRe;
		std::istringstream{m_nuclei->item(row, COL_SCATLEN_IM)->text().toStdString()} >> bIm;
		std::istringstream{m_nuclei->item(row, COL_X)->text().toStdString()} >> x;
		std::istringstream{m_nuclei->item(row, COL_Y)->text().toStdString()} >> y;
		std::istringstream{m_nuclei->item(row, COL_Z)->text().toStdString()} >> z;

		pt::ptree itemNode;
		itemNode.put<std::string>("name", m_nuclei->item(row, COL_NAME)->text().toStdString());
		itemNode.put<t_real>("b_Re", bRe);
		itemNode.put<t_real>("b_Im", bIm);
		itemNode.put<t_real>("x", x);
		itemNode.put<t_real>("y", y);
		itemNode.put<t_real>("z", z);

		node.add_child("sfact.nuclei.nucleus", itemNode);
	}

	std::ofstream ofstr{filename.toStdString()};
	pt::write_xml(ofstr, node, pt::xml_writer_make_settings('\t', 1, std::string{"utf-8"}));
}
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
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



/**
 * calculate structure factors
 */
void StructFactDlg::Calc()
{
	const t_real eps = 1e-6;
	const int prec = 6;
	const auto maxBZ = m_maxBZ->value();


	// powder lines
	std::vector<PowderLine> powderlines;
	auto add_powderline = [&powderlines, eps](t_real Q, t_real I,
		t_real h, t_real k, t_real l)
	{
		std::ostringstream ostrPeak;
		ostrPeak << "(" << h << "," << k << "," << l << "); ";

		// is this Q value already in the vector?
		bool foundQ = false;
		for(auto& line : powderlines)
		{
			if(m::equals<t_real>(line.Q, Q, eps))
			{
				line.I += I;
				line.peaks +=  ostrPeak.str();
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
			powderlines.emplace_back(std::move(line));
		}
	};


	// lattice B matrix
	t_real a,b,c, alpha,beta,gamma;
	std::istringstream{m_editA->text().toStdString()} >> a;
	std::istringstream{m_editB->text().toStdString()} >> b;
	std::istringstream{m_editC->text().toStdString()} >> c;
	std::istringstream{m_editAlpha->text().toStdString()} >> alpha;
	std::istringstream{m_editBeta->text().toStdString()} >> beta;
	std::istringstream{m_editGamma->text().toStdString()} >> gamma;

	//auto crystB = m::unit<t_mat>(3);
	auto crystB = m::B_matrix<t_mat>(a, b, c,
		alpha/180.*m::pi<t_real>, beta/180.*m::pi<t_real>, gamma/180.*m::pi<t_real>);


	std::vector<t_cplx> bs;
	std::vector<t_vec> pos;

	for(const auto& nucl : GetNuclei())
	{
		bs.push_back(nucl.b);
		pos.emplace_back(m::create<t_vec>({ nucl.pos[0], nucl.pos[1], nucl.pos[2] }));
	}


	std::ostringstream ostr, ostrPowder;
	ostr.precision(prec);
	ostrPowder.precision(prec);

	ostr << "# Nuclear single-crystal structure factors:" << "\n";
	ostr << "# "
		<< std::setw(prec*1.2-2) << std::right << "h" << " "
		<< std::setw(prec*1.2) << std::right << "k" << " "
		<< std::setw(prec*1.2) << std::right << "l" << " "
		<< std::setw(prec*2) << std::right << "|Q| (1/A)" << " "
		<< std::setw(prec*2) << std::right << "|Fn|^2" << " "
		<< std::setw(prec*5) << std::right << "Fn (fm)" << "\n";

	ostrPowder << "# Nuclear powder lines:" << "\n";
	ostrPowder << "# "
		<< std::setw(prec*2-2) << std::right << "|Q| (1/A)" << " "
		<< std::setw(prec*2) << std::right << "|F|^2" << "\n";


	for(t_real h=-maxBZ; h<=maxBZ; ++h)
	{
		for(t_real k=-maxBZ; k<=maxBZ; ++k)
		{
			for(t_real l=-maxBZ; l<=maxBZ; ++l)
			{
				auto Q = m::create<t_vec>({h,k,l}) /*+ prop*/;
				auto Q_invA = crystB * Q;
				auto Qabs_invA = m::norm(Q_invA);

				// nuclear structure factor
				auto Fn = m::structure_factor<t_vec, t_cplx>(bs, pos, Q);
				if(m::equals<t_cplx>(Fn, t_cplx(0), eps)) Fn = 0.;
				if(m::equals<t_real>(Fn.real(), 0, eps)) Fn.real(0.);
				if(m::equals<t_real>(Fn.imag(), 0, eps)) Fn.imag(0.);
				auto I = (std::conj(Fn)*Fn).real();

				add_powderline(Qabs_invA, I, h,k,l);

				ostr
					<< std::setw(prec*1.2) << std::right << h << " "
					<< std::setw(prec*1.2) << std::right << k << " "
					<< std::setw(prec*1.2) << std::right << l << " "
					<< std::setw(prec*2) << std::right << Qabs_invA << " "
					<< std::setw(prec*2) << std::right << I << " "
					<< std::setw(prec*5) << std::right << Fn << "\n";
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
			<< std::setw(prec*2) << std::right << line.Q << " "
			<< std::setw(prec*2) << std::right << line.I << " "
			<< line.peaks << "\n";
	}

	m_powderlines->setPlainText(ostrPowder.str().c_str());
}
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
/**
 * mouse hovers over 3d object
 */
void StructFactDlg::PickerIntersection(const t_vec3_gl* pos, std::size_t objIdx, const t_vec3_gl* posSphere)
{
	if(pos)
		m_curPickedObj = long(objIdx);
	else
		m_curPickedObj = -1;


	if(m_curPickedObj > 0)
	{
		// find corresponding nucleus in table
		for(int row=0; row<m_nuclei->rowCount(); ++row)
		{
			if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole).toUInt(); long(obj)==m_curPickedObj)
			{
				auto *itemname = m_nuclei->item(row, COL_NAME);
				auto *itemX = m_nuclei->item(row, COL_X);
				auto *itemY = m_nuclei->item(row, COL_Y);
				auto *itemZ = m_nuclei->item(row, COL_Z);

				std::ostringstream ostr;
				ostr << itemname->text().toStdString();
				ostr << ", r = (";
				ostr << itemX->text().toStdString() << ", ";
				ostr << itemY->text().toStdString() << ", ";
				ostr << itemZ->text().toStdString();
				ostr << ") rlu";

				Set3DStatusMsg(ostr.str().c_str());
				break;
			}
		}
	}
	else
		Set3DStatusMsg("");
}


/**
 * set status label text in 3d dialog
 */
void StructFactDlg::Set3DStatusMsg(const std::string& msg)
{
	m_status3D->setText(msg.c_str());
}



/**
 * mouse button pressed
 */
void StructFactDlg::PlotMouseDown(bool left, bool mid, bool right)
{
	if(left && m_curPickedObj > 0)
	{
		// find corresponding nucleus in table
		for(int row=0; row<m_nuclei->rowCount(); ++row)
		{
			if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole).toUInt(); long(obj)==m_curPickedObj)
			{
				m_nuclei->setCurrentCell(row, 0);
				break;
			}
		}
	}
}


/**
 * mouse button released
 */
void StructFactDlg::PlotMouseUp(bool left, bool mid, bool right)
{
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
void StructFactDlg::AfterGLInitialisation()
{
	if(!m_plot) return;

	m_sphere = m_plot->GetImpl()->AddSphere(0.1, 0.,0.,0., 1.,0.,0.,1.);
	m_plot->GetImpl()->SetObjectVisible(m_sphere, false);

	// add all 3d objects
	Add3DItem(-1);

	// GL device info
	auto [strGlVer, strGlShaderVer, strGlVendor, strGlRenderer]
		= m_plot->GetImpl()->GetGlDescr();
	m_labelGlInfos[0]->setText(QString("GL Version: ") + strGlVer.c_str() + QString("."));
	m_labelGlInfos[1]->setText(QString("GL Shader Version: ") + strGlShaderVer.c_str() + QString("."));
	m_labelGlInfos[2]->setText(QString("GL Vendor: ") + strGlVendor.c_str() + QString("."));
	m_labelGlInfos[3]->setText(QString("GL Device: ") + strGlRenderer.c_str() + QString("."));
}


void StructFactDlg::closeEvent(QCloseEvent *evt)
{
	if(m_sett)
	{
		m_sett->setValue("geo", saveGeometry());
		if(m_dlgPlot)
			m_sett->setValue("geo_3dview", m_dlgPlot->saveGeometry());
	}
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
static inline void set_locales()
{
	std::ios_base::sync_with_stdio(false);

	::setlocale(LC_ALL, "C");
	std::locale::global(std::locale("C"));
	QLocale::setDefault(QLocale::C);
}


int main(int argc, char** argv)
{
	set_gl_format(1, _GL_MAJ_VER, _GL_MIN_VER, 8);
	set_locales();

	auto app = std::make_unique<QApplication>(argc, argv);
	auto dlg = std::make_unique<StructFactDlg>(nullptr);
	dlg->show();

	return app->exec();
}
// ----------------------------------------------------------------------------
