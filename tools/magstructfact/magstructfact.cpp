/**
 * magnetic structure factor tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date Jan-2019
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 28-Dec-2018 from my privately developed "misc" project (https://github.com/t-weber/misc).
 */

#include "magstructfact.h"

#include <QtCore/QDir>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <tuple>

#include <boost/version.hpp>
#include <boost/config.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/units/systems/si/codata/electron_constants.hpp>
#include <boost/units/systems/si/codata/neutron_constants.hpp>
#include <boost/units/systems/si/codata/electromagnetic_constants.hpp>
namespace algo = boost::algorithm;
namespace pt = boost::property_tree;
namespace si = boost::units::si;
namespace consts = si::constants;

#include "../structfact/loadcif.h"
#include "tlibs2/libs/algos.h"
#include "tlibs2/libs/helper.h"


//using namespace tl2;
using namespace tl2_ops;


constexpr t_real g_eps = 1e-6;
constexpr int g_prec = 6;


// columns of Fourier components table
enum : int
{
	COL_NAME = 0,
	COL_X, COL_Y, COL_Z,				// position
	COL_M_MAG,							// scale factor of FC
	COL_ReM_X, COL_ReM_Y, COL_ReM_Z,	// fourier components
	COL_ImM_X, COL_ImM_Y, COL_ImM_Z,
	COL_RAD,							// drawing radius
	COL_COL,							// colour

	NUM_COLS
};


// columns of propagation vectors table
enum : int
{
	PROP_COL_NAME = 0,
	PROP_COL_X, PROP_COL_Y, PROP_COL_Z,	// propagation direction
	PROP_COL_CONJ,						// conjugate fourier component for this propagation vector?

	PROP_NUM_COLS
};


struct PowderLine
{
	t_real Q{};
	t_real I{};
	std::size_t num_peaks = 0;
	std::string peaks;
};


// ----------------------------------------------------------------------------
MagStructFactDlg::MagStructFactDlg(QWidget* pParent) : QDialog{pParent},
	m_sett{new QSettings{"takin", "magstructfact"}}
{
	setWindowTitle("Magnetic Structure Factors");
	setSizeGripEnabled(true);
	setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));


	auto tabs = new QTabWidget(this);


	{	// fourier components panel
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
		m_nuclei->setHorizontalHeaderItem(COL_X, new QTableWidgetItem{"x (frac.)"});
		m_nuclei->setHorizontalHeaderItem(COL_Y, new QTableWidgetItem{"y (frac.)"});
		m_nuclei->setHorizontalHeaderItem(COL_Z, new QTableWidgetItem{"z (frac.)"});
		m_nuclei->setHorizontalHeaderItem(COL_M_MAG, new QTableWidgetItem{"|M|"});
		m_nuclei->setHorizontalHeaderItem(COL_ReM_X, new QTableWidgetItem{"Re{FC_x}"});
		m_nuclei->setHorizontalHeaderItem(COL_ReM_Y, new QTableWidgetItem{"Re{FC_y}"});
		m_nuclei->setHorizontalHeaderItem(COL_ReM_Z, new QTableWidgetItem{"Re{FC_z}"});
		m_nuclei->setHorizontalHeaderItem(COL_ImM_X, new QTableWidgetItem{"Im{FC_x}"});
		m_nuclei->setHorizontalHeaderItem(COL_ImM_Y, new QTableWidgetItem{"Im{FC_y}"});
		m_nuclei->setHorizontalHeaderItem(COL_ImM_Z, new QTableWidgetItem{"Im{FC_z}"});
		m_nuclei->setHorizontalHeaderItem(COL_RAD, new QTableWidgetItem{"Scale"});
		m_nuclei->setHorizontalHeaderItem(COL_COL, new QTableWidgetItem{"Colour"});

		m_nuclei->setColumnWidth(COL_NAME, 90);
		m_nuclei->setColumnWidth(COL_X, 75);
		m_nuclei->setColumnWidth(COL_Y, 75);
		m_nuclei->setColumnWidth(COL_Z, 75);
		m_nuclei->setColumnWidth(COL_M_MAG, 75);
		m_nuclei->setColumnWidth(COL_ReM_X, 75);
		m_nuclei->setColumnWidth(COL_ReM_Y, 75);
		m_nuclei->setColumnWidth(COL_ReM_Z, 75);
		m_nuclei->setColumnWidth(COL_ImM_X, 75);
		m_nuclei->setColumnWidth(COL_ImM_Y, 75);
		m_nuclei->setColumnWidth(COL_ImM_Z, 75);
		m_nuclei->setColumnWidth(COL_RAD, 75);
		m_nuclei->setColumnWidth(COL_COL, 75);

		QToolButton *pTabBtnAdd = new QToolButton(m_nucleipanel);
		QToolButton *pTabBtnDel = new QToolButton(m_nucleipanel);
		QToolButton *pTabBtnUp = new QToolButton(m_nucleipanel);
		QToolButton *pTabBtnDown = new QToolButton(m_nucleipanel);
		QToolButton *pTabBtnSG = new QToolButton(m_nucleipanel);

		m_nuclei->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});
		pTabBtnAdd->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDel->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnUp->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDown->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnSG->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});

		pTabBtnAdd->setText("Add Fourier Comp.");
		pTabBtnDel->setText("Delete Fourier Comp.");
		pTabBtnUp->setText("Move Fourier Comp. Up");
		pTabBtnDown->setText("Move Fourier Comp. Down");
		pTabBtnSG->setText("Generate");

		m_editA = new QLineEdit("5", m_nucleipanel);
		m_editB = new QLineEdit("5", m_nucleipanel);
		m_editC = new QLineEdit("5", m_nucleipanel);
		m_editAlpha = new QLineEdit("90", m_nucleipanel);
		m_editBeta = new QLineEdit("90", m_nucleipanel);
		m_editGamma = new QLineEdit("90", m_nucleipanel);

		m_comboSG = new QComboBox(m_nucleipanel);


		// get space groups and symops
		for(auto [sgnum, descr, ops] : get_sgs<t_mat>())
		{
			m_comboSG->addItem(descr.c_str(), m_comboSG->count());
			m_SGops.emplace_back(std::move(ops));
		}


		auto pTabGrid = new QGridLayout(m_nucleipanel);
		pTabGrid->setSpacing(2);
		pTabGrid->setContentsMargins(4,4,4,4);
		int y = 0;
		//pTabGrid->addWidget(m_plot.get(), y,0,1,4);
		pTabGrid->addWidget(m_nuclei, y,0,1,4);
		pTabGrid->addWidget(pTabBtnAdd, ++y,0,1,1);
		pTabGrid->addWidget(pTabBtnDel, y,1,1,1);
		pTabGrid->addWidget(pTabBtnUp, y,2,1,1);
		pTabGrid->addWidget(pTabBtnDown, y,3,1,1);

		pTabGrid->addWidget(new QLabel("Space Groups:"), ++y,0,1,1);
		pTabGrid->addWidget(m_comboSG, y,1,1,2);
		pTabGrid->addWidget(pTabBtnSG, y,3,1,1);


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
		QMenu *pTabContextMenu = new QMenu(m_nuclei);
		pTabContextMenu->addAction("Add Fourier Component Before", this, [this]() { this->AddTabItem(-2); });
		pTabContextMenu->addAction("Add Fourier Component After", this, [this]() { this->AddTabItem(-3); });
		pTabContextMenu->addAction("Clone Fourier Component", this, [this]() { this->AddTabItem(-4); });
		pTabContextMenu->addAction("Delete Fourier Component", this, [this]() { this->DelTabItem(); });


		// table CustomContextMenu in case nothing is selected
		QMenu *pTabContextMenuNoItem = new QMenu(m_nuclei);
		pTabContextMenuNoItem->addAction("Add Fourier Component", this, [this]() { this->AddTabItem(); });
		pTabContextMenuNoItem->addAction("Delete Fourier Component", this, [this]() { this->DelTabItem(); });
		//pTabContextMenuNoItem->addSeparator();


		// signals
		for(auto* edit : std::vector<QLineEdit*>{{ m_editA, m_editB, m_editC, m_editAlpha, m_editBeta, m_editGamma }})
			connect(edit, &QLineEdit::textEdited, this, [this]() { this->CalcB(); });

		connect(pTabBtnAdd, &QToolButton::clicked, this, [this]() { this->AddTabItem(-1); });
		connect(pTabBtnDel, &QToolButton::clicked, this, [this]() { this->DelTabItem(); });
		connect(pTabBtnUp, &QToolButton::clicked, this, [this]() { this->MoveTabItemUp(m_nuclei); });
		connect(pTabBtnDown, &QToolButton::clicked, this, [this]() { this->MoveTabItemDown(m_nuclei); });
		connect(pTabBtnSG, &QToolButton::clicked, this, &MagStructFactDlg::GenerateFromSG);

		connect(m_nuclei, &QTableWidget::currentCellChanged, this, &MagStructFactDlg::TableCurCellChanged);
		connect(m_nuclei, &QTableWidget::entered, this, &MagStructFactDlg::TableCellEntered);
		connect(m_nuclei, &QTableWidget::itemChanged, this, &MagStructFactDlg::TableItemChanged);
		connect(m_nuclei, &QTableWidget::customContextMenuRequested, this,
			[this, pTabContextMenu, pTabContextMenuNoItem](const QPoint& pt)
			{ this->ShowTableContextMenu(m_nuclei, pTabContextMenu, pTabContextMenuNoItem, pt); });

		tabs->addTab(m_nucleipanel, "Fourier Components");
	}


	{	// propagation vectors panel
		m_propvecpanel = new QWidget(this);

		m_propvecs = new QTableWidget(m_propvecpanel);
		m_propvecs->setShowGrid(true);
		m_propvecs->setSortingEnabled(true);
		m_propvecs->setMouseTracking(true);
		m_propvecs->setSelectionBehavior(QTableWidget::SelectRows);
		m_propvecs->setSelectionMode(QTableWidget::ContiguousSelection);
		m_propvecs->setContextMenuPolicy(Qt::CustomContextMenu);

		m_propvecs->verticalHeader()->setDefaultSectionSize(fontMetrics().lineSpacing() + 4);
		m_propvecs->verticalHeader()->setVisible(false);

		m_propvecs->setColumnCount(PROP_NUM_COLS);
		m_propvecs->setHorizontalHeaderItem(PROP_COL_NAME, new QTableWidgetItem{"Name"});
		m_propvecs->setHorizontalHeaderItem(PROP_COL_X, new QTableWidgetItem{"x (frac.)"});
		m_propvecs->setHorizontalHeaderItem(PROP_COL_Y, new QTableWidgetItem{"y (frac.)"});
		m_propvecs->setHorizontalHeaderItem(PROP_COL_Z, new QTableWidgetItem{"z (frac.)"});
		m_propvecs->setHorizontalHeaderItem(PROP_COL_CONJ, new QTableWidgetItem{"FC*"});

		m_propvecs->setColumnWidth(PROP_COL_NAME, 90);
		m_propvecs->setColumnWidth(PROP_COL_X, 90);
		m_propvecs->setColumnWidth(PROP_COL_Y, 90);
		m_propvecs->setColumnWidth(PROP_COL_Z, 90);
		m_propvecs->setColumnWidth(PROP_COL_CONJ, 75);

		QToolButton *pTabBtnAdd = new QToolButton(m_propvecpanel);
		QToolButton *pTabBtnDel = new QToolButton(m_propvecpanel);
		QToolButton *pTabBtnUp = new QToolButton(m_propvecpanel);
		QToolButton *pTabBtnDown = new QToolButton(m_propvecpanel);

		m_propvecs->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});
		pTabBtnAdd->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDel->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnUp->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDown->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});

		pTabBtnAdd->setText("Add Vector");
		pTabBtnDel->setText("Delete Vector");
		pTabBtnUp->setText("Move Vector Up");
		pTabBtnDown->setText("Move Vector Down");


		auto pTabGrid = new QGridLayout(m_propvecpanel);
		pTabGrid->setSpacing(2);
		pTabGrid->setContentsMargins(4,4,4,4);
		int y = 0;
		pTabGrid->addWidget(m_propvecs, y,0,1,4);
		pTabGrid->addWidget(pTabBtnAdd, ++y,0,1,1);
		pTabGrid->addWidget(pTabBtnDel, y,1,1,1);
		pTabGrid->addWidget(pTabBtnUp, y,2,1,1);
		pTabGrid->addWidget(pTabBtnDown, y,3,1,1);


		// table CustomContextMenu
		QMenu *pPropContextMenu = new QMenu(m_propvecs);
		pPropContextMenu->addAction("Add Vector Before", this, [this]() { this->AddPropItem(-2); });
		pPropContextMenu->addAction("Add Vector After", this, [this]() { this->AddPropItem(-3); });
		pPropContextMenu->addAction("Clone Vector", this, [this]() { this->AddPropItem(-4); });
		pPropContextMenu->addAction("Delete Vector", this, [this]() { this->DelPropItem(); });


		// table CustomContextMenu in case nothing is selected
		QMenu *pPropContextMenuNoItem = new QMenu(m_propvecs);
		pPropContextMenuNoItem->addAction("Add Vector", this, [this]() { this->AddPropItem(); });
		pPropContextMenuNoItem->addAction("Delete Vector", this, [this]() { this->DelPropItem(); });



		connect(pTabBtnAdd, &QToolButton::clicked, this, [this]() { this->AddPropItem(-1); });
		connect(pTabBtnDel, &QToolButton::clicked, this, [this]() { this->DelPropItem(); });
		connect(pTabBtnUp, &QToolButton::clicked, this,  [this]() { this->MoveTabItemUp(m_propvecs); });
		connect(pTabBtnDown, &QToolButton::clicked, this,  [this]() { this->MoveTabItemUp(m_propvecs); });

		//connect(m_propvecs, &QTableWidget::currentCellChanged, this, &MagStructFactDlg::PropCurCellChanged);
		//connect(m_propvecs, &QTableWidget::entered, this, &MagStructFactDlg::PropCellEntered);
		connect(m_propvecs, &QTableWidget::itemChanged, this, &MagStructFactDlg::PropItemChanged);
		connect(m_propvecs, &QTableWidget::customContextMenuRequested, this,
			[this, pPropContextMenu, pPropContextMenuNoItem](const QPoint& pt)
			{ this->ShowTableContextMenu(m_propvecs, pPropContextMenu, pPropContextMenuNoItem, pt); });

		tabs->addTab(m_propvecpanel, "Propagation Vectors");
	}


	{	// structure factors panel
		auto sfactpanel = new QWidget(this);
		auto pGrid = new QGridLayout(sfactpanel);
		pGrid->setSpacing(4);
		pGrid->setContentsMargins(4,4,4,4);

		m_structfacts = new QPlainTextEdit(sfactpanel);
		m_structfacts->setReadOnly(true);
		m_structfacts->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
		m_structfacts->setLineWrapMode(QPlainTextEdit::NoWrap);

		m_maxBZ = new QSpinBox(sfactpanel);
		m_maxBZ->setMinimum(0);
		m_maxBZ->setMaximum(99);
		m_maxBZ->setValue(4);

		m_RemoveZeroes = new QCheckBox("Remove Zeroes", sfactpanel);
		m_RemoveZeroes->setChecked(true);


		pGrid->addWidget(m_structfacts, 0,0, 1,4);
		pGrid->addWidget(new QLabel("Max. Order:"), 1,0,1,1);
		pGrid->addWidget(m_maxBZ, 1,1, 1,1);
		pGrid->addWidget(m_RemoveZeroes, 1,2, 1,2);


		// signals
		connect(m_maxBZ, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this]() { this->Calc(); });
		connect(m_RemoveZeroes, static_cast<void (QCheckBox::*)(int)>(&QCheckBox::stateChanged), this, [this]() { this->Calc(); });

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


	{	// real magnetic moments
		auto mmpanel = new QWidget(this);
		auto pGrid = new QGridLayout(mmpanel);
		pGrid->setSpacing(4);
		pGrid->setContentsMargins(4,4,4,4);

		m_moments = new QPlainTextEdit(mmpanel);
		m_moments->setReadOnly(true);
		m_moments->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
		m_moments->setLineWrapMode(QPlainTextEdit::NoWrap);

		for(auto*& spin : m_maxSC)
		{
			spin = new QSpinBox(mmpanel);
			spin->setMinimum(0);
			spin->setMaximum(999);
			spin->setValue(4);
		}

		m_maxSC[0]->setPrefix("x = ");
		m_maxSC[1]->setPrefix("y = ");
		m_maxSC[2]->setPrefix("z = ");

		pGrid->addWidget(m_moments, 0,0, 1,4);
		pGrid->addWidget(new QLabel("Max. Supercell Order:"), 1,0,1,1);
		pGrid->addWidget(m_maxSC[0], 1,1, 1,1);
		pGrid->addWidget(m_maxSC[1], 1,2, 1,1);
		pGrid->addWidget(m_maxSC[2], 1,3, 1,1);


		// signals
		for(auto* spin : m_maxSC)
			connect(spin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this]() { this->Calc(); });

		tabs->addTab(mmpanel, "Magnetic Moments");
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

		auto labelTitle = new QLabel("Magnetic Structure Factor Calculator", infopanel);
		auto fontTitle = labelTitle->font();
		fontTitle.setBold(true);
		labelTitle->setFont(fontTitle);
		labelTitle->setAlignment(Qt::AlignHCenter);

		auto labelAuthor = new QLabel("Written by Tobias Weber <tweber@ill.fr>.", infopanel);
		labelAuthor->setAlignment(Qt::AlignHCenter);

		auto labelDate = new QLabel("January 2019.", infopanel);
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


	// menu bar
	{
		m_menu = new QMenuBar(this);
		m_menu->setNativeMenuBar(m_sett ? m_sett->value("native_gui", false).toBool() : false);

		auto menuFile = new QMenu("File", m_menu);
		auto menuView = new QMenu("3D View", m_menu);

		auto acNew = new QAction("New", menuFile);
		auto acLoad = new QAction("Load...", menuFile);
		auto acSave = new QAction("Save...", menuFile);
		auto acImportCIF = new QAction("Import CIF...", menuFile);
		auto acExit = new QAction("Quit", menuFile);
		auto ac3DView = new QAction("Unit Cell / Fourier Components...", menuFile);
		auto ac3DViewSC = new QAction("Super Cell / Magnetic Moments...", menuFile);

		menuFile->addAction(acNew);
		menuFile->addSeparator();
		menuFile->addAction(acLoad);
		menuFile->addAction(acSave);
		menuFile->addSeparator();
		menuFile->addAction(acImportCIF);
		menuFile->addSeparator();
		menuFile->addAction(acExit);
		menuView->addAction(ac3DView);
		menuView->addAction(ac3DViewSC);

		connect(acNew, &QAction::triggered, this,  [this]()
		{
			// clear old tables
			DelTabItem(-1);
			DelPropItem(-1);

			// set some defaults
			m_comboSG->setCurrentIndex(0);
			m_editA->setText("5");
			m_editB->setText("5");
			m_editC->setText("5");
			m_editAlpha->setText("90");
			m_editBeta->setText("90");
			m_editGamma->setText("90");
		});
		connect(acLoad, &QAction::triggered, this, &MagStructFactDlg::Load);
		connect(acSave, &QAction::triggered, this, &MagStructFactDlg::Save);
		connect(acImportCIF, &QAction::triggered, this, &MagStructFactDlg::ImportCIF);
		connect(acExit, &QAction::triggered, this, &QDialog::close);


		// unit cell view
		connect(ac3DView, &QAction::triggered, this, [this]()
		{
			// plot widget
			if(!m_dlgPlot)
			{
				m_dlgPlot = new QDialog(this);
				m_dlgPlot->setWindowTitle("Unit Cell");

				m_plot = std::make_shared<GlPlot>(this);
				m_plot->GetImpl()->SetLight(0, tl2::create<t_vec3_gl>({ 5, 5, 5 }));
				m_plot->GetImpl()->SetLight(1, tl2::create<t_vec3_gl>({ -5, -5, -5 }));
				m_plot->GetImpl()->SetCoordMax(1.);
				m_plot->GetImpl()->SetCamBase(tl2::create<t_mat_gl>({1,0,0,0,  0,0,1,0,  0,-1,0,-1.5,  0,0,0,1}),
					tl2::create<t_vec_gl>({1,0,0,0}), tl2::create<t_vec_gl>({0,0,1,0}));


				auto labCoordSys = new QLabel("Coordinate System:", /*m_dlgPlot*/ this);
				auto comboCoordSys = new QComboBox(/*m_dlgPlot*/ this);
				m_status3D = new QLabel(/*m_dlgPlot*/ this);

				comboCoordSys->addItem("Fractional Units (rlu)");
				comboCoordSys->addItem("Lab Units (A)");


				m_plot->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});
				labCoordSys->setSizePolicy(QSizePolicy{QSizePolicy::Fixed, QSizePolicy::Fixed});

				auto grid = new QGridLayout(m_dlgPlot);
				grid->setSpacing(2);
				grid->setContentsMargins(4,4,4,4);
				grid->addWidget(m_plot.get(), 0,0,1,2);
				grid->addWidget(labCoordSys, 1,0,1,1);
				grid->addWidget(comboCoordSys, 1,1,1,1);
				grid->addWidget(m_status3D, 2,0,1,2);


				connect(m_plot.get(), &GlPlot::AfterGLInitialisation, this, &MagStructFactDlg::AfterGLInitialisation);
				connect(m_plot->GetImpl(), &GlPlot_impl::PickerIntersection, this, &MagStructFactDlg::PickerIntersection);
				connect(m_plot.get(), &GlPlot::MouseDown, this, &MagStructFactDlg::PlotMouseDown);
				//connect(m_plot.get(), &GlPlot::MouseUp, this, [this](bool left, bool mid, bool right) {});
				connect(comboCoordSys, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int val)
				{
					if(this->m_plot)
						this->m_plot->GetImpl()->SetCoordSys(val);
				});


				if(m_sett && m_sett->contains("geo_3dview"))
					m_dlgPlot->restoreGeometry(m_sett->value("geo_3dview").toByteArray());
				else
					m_dlgPlot->resize(500,500);
			}

			m_dlgPlot->show();
			m_dlgPlot->raise();
			m_dlgPlot->focusWidget();
		});


		// super cell view
		connect(ac3DViewSC, &QAction::triggered, this, [this]()
		{
			// plot widget
			if(!m_dlgPlotSC)
			{
				m_dlgPlotSC = new QDialog(this);
				m_dlgPlotSC->setWindowTitle("Super Cell");

				m_plotSC = std::make_shared<GlPlot>(this);
				m_plotSC->GetImpl()->SetLight(0, tl2::create<t_vec3_gl>({ 5, 5, 5 }));
				m_plotSC->GetImpl()->SetLight(1, tl2::create<t_vec3_gl>({ -5, -5, -5 }));
				m_plotSC->GetImpl()->SetCoordMax(1.);
				m_plotSC->GetImpl()->SetCamBase(tl2::create<t_mat_gl>({1,0,0,0,  0,0,1,0,  0,-1,0,-1.5,  0,0,0,1}),
					tl2::create<t_vec_gl>({1,0,0,0}), tl2::create<t_vec_gl>({0,0,1,0}));


				auto labCoordSys = new QLabel("Coordinate System:", /*m_dlgPlotSC*/ this);
				auto comboCoordSys = new QComboBox(/*m_dlgPlotSC*/ this);
				m_status3DSC = new QLabel(/*m_dlgPlotSC*/ this);

				comboCoordSys->addItem("Fractional Units (rlu)");
				comboCoordSys->addItem("Lab Units (A)");


				m_plotSC->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});
				labCoordSys->setSizePolicy(QSizePolicy{QSizePolicy::Fixed, QSizePolicy::Fixed});

				auto grid = new QGridLayout(m_dlgPlotSC);
				grid->setSpacing(2);
				grid->setContentsMargins(4,4,4,4);
				grid->addWidget(m_plotSC.get(), 0,0,1,2);
				grid->addWidget(labCoordSys, 1,0,1,1);
				grid->addWidget(comboCoordSys, 1,1,1,1);
				grid->addWidget(m_status3DSC, 2,0,1,2);


				connect(m_plotSC.get(), &GlPlot::AfterGLInitialisation, this, &MagStructFactDlg::AfterGLInitialisationSC);
				connect(m_plotSC->GetImpl(), &GlPlot_impl::PickerIntersection, this, &MagStructFactDlg::PickerIntersectionSC);
				//connect(m_plotSC.get(), &GlPlot::MouseDown, this, [this](bool left, bool mid, bool right) {});
				//connect(m_plotSC.get(), &GlPlot::MouseUp, this, [this](bool left, bool mid, bool right) {});
				connect(comboCoordSys, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int val)
				{
					if(this->m_plotSC)
						this->m_plotSC->GetImpl()->SetCoordSys(val);
				});


				if(m_sett && m_sett->contains("geo_3dview_sc"))
					m_dlgPlotSC->restoreGeometry(m_sett->value("geo_3dview_sc").toByteArray());
				else
					m_dlgPlotSC->resize(500,500);
			}

			m_dlgPlotSC->show();
			m_dlgPlotSC->raise();
			m_dlgPlotSC->focusWidget();
		});

		m_menu->addMenu(menuFile);
		m_menu->addMenu(menuView);
		pmainGrid->setMenuBar(m_menu);
	}


	// restory window size and position
	if(m_sett && m_sett->contains("geo"))
		restoreGeometry(m_sett->value("geo").toByteArray());
	else
		resize(600, 500);


	m_ignoreChanges = 0;
}



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
		m_nuclei->setItem(row, COL_M_MAG, new NumericTableWidgetItem<t_real>(MMag));
		m_nuclei->setItem(row, COL_X, new NumericTableWidgetItem<t_real>(x));
		m_nuclei->setItem(row, COL_Y, new NumericTableWidgetItem<t_real>(y));
		m_nuclei->setItem(row, COL_Z, new NumericTableWidgetItem<t_real>(z));
		m_nuclei->setItem(row, COL_ReM_X, new NumericTableWidgetItem<t_real>(ReMx));
		m_nuclei->setItem(row, COL_ReM_Y, new NumericTableWidgetItem<t_real>(ReMy));
		m_nuclei->setItem(row, COL_ReM_Z, new NumericTableWidgetItem<t_real>(ReMz));
		m_nuclei->setItem(row, COL_ImM_X, new NumericTableWidgetItem<t_real>(ImMx));
		m_nuclei->setItem(row, COL_ImM_Y, new NumericTableWidgetItem<t_real>(ImMy));
		m_nuclei->setItem(row, COL_ImM_Z, new NumericTableWidgetItem<t_real>(ImMz));
		m_nuclei->setItem(row, COL_RAD, new NumericTableWidgetItem<t_real>(scale));
		m_nuclei->setItem(row, COL_COL, new QTableWidgetItem(col.c_str()));
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
void MagStructFactDlg::Add3DItem(int row)
{
	if(!m_plot) return;

	// add all items
	if(row < 0)
	{
		for(int row=0; row<m_nuclei->rowCount(); ++row)
			Add3DItem(row);
		return;
	}

	auto objSphere = m_plot->GetImpl()->AddLinkedObject(m_sphere, 0,0,0, 1,1,1,1);
	//auto obj = m_plot->GetImpl()->AddSphere(0.05, 0,0,0, 1,1,1,1);
	auto objArrowRe = m_plot->GetImpl()->AddLinkedObject(m_arrow, 0,0,0, 1,1,1,1);
	auto objArrowIm = m_plot->GetImpl()->AddLinkedObject(m_arrow, 0,0,0, 1,1,1,1);

	m_nuclei->item(row, COL_NAME)->setData(Qt::UserRole+0, unsigned(objSphere));	// atomic position
	m_nuclei->item(row, COL_NAME)->setData(Qt::UserRole+1, unsigned(objArrowRe));	// real part of Fourier comp
	m_nuclei->item(row, COL_NAME)->setData(Qt::UserRole+2, unsigned(objArrowIm));	// imag part of Fourier comp

	Sync3DItem(row);
}


/**
 * sync the properties of a 3d object
 */
void MagStructFactDlg::Sync3DItem(int row)
{
	if(!m_plot) return;

	// sync all items
	if(row < 0)
	{
		for(int row=0; row<m_nuclei->rowCount(); ++row)
			Sync3DItem(row);
		return;
	}

	std::size_t objSphere = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+0).toUInt();
	std::size_t objArrowRe = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+1).toUInt();
	std::size_t objArrowIm = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+2).toUInt();
	if(!objSphere || !objArrowRe || !objArrowIm)
		return;

	auto *itemName = m_nuclei->item(row, COL_NAME);
	auto *itemx = m_nuclei->item(row, COL_X);
	auto *itemy = m_nuclei->item(row, COL_Y);
	auto *itemz = m_nuclei->item(row, COL_Z);
	auto *itemM = m_nuclei->item(row, COL_M_MAG);
	auto *itemReMX = m_nuclei->item(row, COL_ReM_X);
	auto *itemReMY = m_nuclei->item(row, COL_ReM_Y);
	auto *itemReMZ = m_nuclei->item(row, COL_ReM_Z);
	auto *itemImMX = m_nuclei->item(row, COL_ImM_X);
	auto *itemImMY = m_nuclei->item(row, COL_ImM_Y);
	auto *itemImMZ = m_nuclei->item(row, COL_ImM_Z);
	auto *itemsc = m_nuclei->item(row, COL_RAD);
	auto *itemCol = m_nuclei->item(row, COL_COL);

	t_real_gl posx=0, posy=0, posz=0, M=1, ReMX=0, ReMY=0, ReMZ=1, ImMX=0, ImMY=0, ImMZ=1, scale=1;
	std::istringstream{itemx->text().toStdString()} >> posx;
	std::istringstream{itemy->text().toStdString()} >> posy;
	std::istringstream{itemz->text().toStdString()} >> posz;
	std::istringstream{itemM->text().toStdString()} >> M;
	std::istringstream{itemReMX->text().toStdString()} >> ReMX;
	std::istringstream{itemReMY->text().toStdString()} >> ReMY;
	std::istringstream{itemReMZ->text().toStdString()} >> ReMZ;
	std::istringstream{itemImMX->text().toStdString()} >> ImMX;
	std::istringstream{itemImMY->text().toStdString()} >> ImMY;
	std::istringstream{itemImMZ->text().toStdString()} >> ImMZ;
	std::istringstream{itemsc->text().toStdString()} >> scale;

	qreal r=1, g=1, b=1;
	QColor col{itemCol->text()};
	col.getRgbF(&r, &g, &b);

	t_mat_gl matSphere = tl2::hom_translation<t_mat_gl>(posx, posy, posz) *
		tl2::hom_scaling<t_mat_gl>(M*scale, M*scale, M*scale);

	auto vecReM = tl2::create<t_vec_gl>({t_real_gl(ReMX), t_real_gl(ReMY), t_real_gl(ReMZ)});
	auto vecImM = tl2::create<t_vec_gl>({t_real_gl(ImMX), t_real_gl(ImMY), t_real_gl(ImMZ)});
	auto normReM = tl2::norm<t_vec_gl>(vecReM);
	auto normImM = tl2::norm<t_vec_gl>(vecImM);

	t_mat_gl matArrowRe = GlPlot_impl::GetArrowMatrix(
		vecReM, 									// to
		1, 											// post-scale
		tl2::create<t_vec_gl>({0, 0, 0}),				// post-translate
		tl2::create<t_vec_gl>({0, 0, 1}),				// from
		M*scale, 									// pre-scale
		tl2::create<t_vec_gl>({posx, posy, posz})		// pre-translate
	);

	t_mat_gl matArrowIm = GlPlot_impl::GetArrowMatrix(
		vecImM, 									// to
		1, 											// post-scale
		tl2::create<t_vec_gl>({0, 0, 0}),				// post-translate
		tl2::create<t_vec_gl>({0, 0, 1}),				// from
		M*scale, 									// pre-scale
		tl2::create<t_vec_gl>({posx, posy, posz})		// pre-translate
	);

	m_plot->GetImpl()->SetObjectMatrix(objSphere, matSphere);
	m_plot->GetImpl()->SetObjectMatrix(objArrowRe, matArrowRe);
	m_plot->GetImpl()->SetObjectMatrix(objArrowIm, matArrowIm);
	m_plot->GetImpl()->SetObjectLabel(objSphere, itemName->text().toStdString());
	m_plot->GetImpl()->SetObjectCol(objSphere, r, g, b, 1);
	m_plot->GetImpl()->SetObjectCol(objArrowRe, r, g, b, 1);
	m_plot->GetImpl()->SetObjectCol(objArrowIm, 1.-r, 1.-g, 1.-b, 1);
	m_plot->GetImpl()->SetObjectVisible(objArrowRe, !tl2::equals<t_real_gl>(normReM, 0, g_eps));
	m_plot->GetImpl()->SetObjectVisible(objArrowIm, !tl2::equals<t_real_gl>(normImM, 0, g_eps));
	m_plot->update();
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
					m_plot->GetImpl()->RemoveObject(obj);
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+1).toUInt(); obj)
					m_plot->GetImpl()->RemoveObject(obj);
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+2).toUInt(); obj)
					m_plot->GetImpl()->RemoveObject(obj);
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
					m_plot->GetImpl()->RemoveObject(obj);
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+1).toUInt(); obj)
					m_plot->GetImpl()->RemoveObject(obj);
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+2).toUInt(); obj)
					m_plot->GetImpl()->RemoveObject(obj);
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
					m_plot->GetImpl()->RemoveObject(obj);
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+1).toUInt(); obj)
					m_plot->GetImpl()->RemoveObject(obj);
				if(std::size_t obj = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+2).toUInt(); obj)
					m_plot->GetImpl()->RemoveObject(obj);
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
		m_propvecs->setItem(row, PROP_COL_X, new NumericTableWidgetItem<t_real>(x));
		m_propvecs->setItem(row, PROP_COL_Y, new NumericTableWidgetItem<t_real>(y));
		m_propvecs->setItem(row, PROP_COL_Z, new NumericTableWidgetItem<t_real>(z));
		m_propvecs->setItem(row, PROP_COL_CONJ, new NumericTableWidgetItem<int>(bConjFC));
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




// ----------------------------------------------------------------------------
void MagStructFactDlg::Load()
{
	m_ignoreCalc = 1;

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
		if(auto optInfo = node.get_optional<std::string>("sfact.meta.info");
			!optInfo || !(*optInfo==std::string{"magsfact_tool"} || *optInfo==std::string{"sfact_tool"}))
		{
			QMessageBox::critical(this, "Structure Factors", "Unrecognised file format.");
			return;
		}
		else if(*optInfo == std::string{"sfact_tool"})
		{
			QMessageBox::warning(this, "Structure Factors", "File only contains nuclear information. Trying to load.");
		}


		// clear old tables
		DelTabItem(-1);
		DelPropItem(-1);


		// lattice
		if(auto opt = node.get_optional<t_real>("sfact.xtal.a"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editA->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("sfact.xtal.b"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editB->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("sfact.xtal.c"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editC->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("sfact.xtal.alpha"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editAlpha->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("sfact.xtal.beta"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editBeta->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("sfact.xtal.gamma"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editGamma->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<int>("sfact.order"); opt)
		{
			m_maxBZ->setValue(*opt);
		}
		if(auto opt = node.get_optional<int>("sfact.scorder_x"); opt)
		{
			m_maxSC[0]->setValue(*opt);
		}
		if(auto opt = node.get_optional<int>("sfact.scorder_y"); opt)
		{
			m_maxSC[1]->setValue(*opt);
		}
		if(auto opt = node.get_optional<int>("sfact.scorder_z"); opt)
		{
			m_maxSC[2]->setValue(*opt);
		}
		if(auto opt = node.get_optional<int>("sfact.removezeroes"); opt)
		{
			m_RemoveZeroes->setChecked(*opt != 0);
		}
		if(auto opt = node.get_optional<int>("sfact.sg_idx"); opt)
		{
			m_comboSG->setCurrentIndex(*opt);
		}


		// fourier components
		if(auto nuclei = node.get_child_optional("sfact.nuclei"); nuclei)
		{
			for(const auto &nucl : *nuclei)
			{
				auto optName = nucl.second.get<std::string>("name", "n/a");
				auto optMMag = nucl.second.get<t_real>("M_mag", 1.);
				auto optX = nucl.second.get<t_real>("x", 0.);
				auto optY = nucl.second.get<t_real>("y", 0.);
				auto optZ = nucl.second.get<t_real>("z", 0.);
				auto optReMX = nucl.second.get<t_real>("ReMx", 0.);
				auto optReMY = nucl.second.get<t_real>("ReMy", 0.);
				auto optReMZ = nucl.second.get<t_real>("ReMz", 0.);
				auto optImMX = nucl.second.get<t_real>("ImMx", 0.);
				auto optImMY = nucl.second.get<t_real>("ImMy", 0.);
				auto optImMZ = nucl.second.get<t_real>("ImMz", 0.);
				auto optRad = nucl.second.get<t_real>("rad", 1.);
				auto optCol = nucl.second.get<std::string>("col", "#ff0000");

				AddTabItem(-1, optName, optMMag, optX,  optY, optZ,
					optReMX, optReMY, optReMZ, optImMX, optImMY, optImMZ,
					optRad, optCol);
			}
		}


		// propagation vectors
		if(auto propvecs = node.get_child_optional("sfact.propvecs"); propvecs)
		{
			for(const auto &propvec : *propvecs)
			{
				auto optName = propvec.second.get<std::string>("name", "n/a");
				auto optX = propvec.second.get<t_real>("x", 0.);
				auto optY = propvec.second.get<t_real>("y", 0.);
				auto optZ = propvec.second.get<t_real>("z", 0.);
				auto optConj = propvec.second.get<int>("conjFC", 0);

				AddPropItem(-1, optName, optX,  optY, optZ, optConj!=0);
			}
		}
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Structure Factors", ex.what());
	}


	m_ignoreCalc = 0;
	CalcB(false);
	Calc();
}


void MagStructFactDlg::Save()
{
	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getSaveFileName(this, "Save File", dirLast, "XML Files (*.xml *.XML)");
	if(filename=="")
		return;
	m_sett->setValue("dir", QFileInfo(filename).path());


	pt::ptree node;
	node.put<std::string>("sfact.meta.info", "magsfact_tool");
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
	node.put<int>("sfact.scorder_x", m_maxSC[0]->value());
	node.put<int>("sfact.scorder_y", m_maxSC[1]->value());
	node.put<int>("sfact.scorder_z", m_maxSC[2]->value());
	node.put<int>("sfact.removezeroes", m_RemoveZeroes->isChecked());
	node.put<int>("sfact.sg_idx", m_comboSG->currentIndex());


	// fourier component list
	for(int row=0; row<m_nuclei->rowCount(); ++row)
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

		pt::ptree itemNode;
		itemNode.put<std::string>("name", m_nuclei->item(row, COL_NAME)->text().toStdString());
		itemNode.put<t_real>("M_mag", MMag);
		itemNode.put<t_real>("x", x);
		itemNode.put<t_real>("y", y);
		itemNode.put<t_real>("z", z);
		itemNode.put<t_real>("ReMx", ReMx);
		itemNode.put<t_real>("ReMy", ReMy);
		itemNode.put<t_real>("ReMz", ReMz);
		itemNode.put<t_real>("ImMx", ImMx);
		itemNode.put<t_real>("ImMy", ImMy);
		itemNode.put<t_real>("ImMz", ImMz);
		itemNode.put<t_real>("rad", scale);
		itemNode.put<std::string>("col", m_nuclei->item(row, COL_COL)->text().toStdString());

		node.add_child("sfact.nuclei.nucleus", itemNode);
	}


	// propagation vectors list
	for(int row=0; row<m_propvecs->rowCount(); ++row)
	{
		t_real x{},y{},z{};
		int iConj{0};
		std::istringstream{m_propvecs->item(row, PROP_COL_X)->text().toStdString()} >> x;
		std::istringstream{m_propvecs->item(row, PROP_COL_Y)->text().toStdString()} >> y;
		std::istringstream{m_propvecs->item(row, PROP_COL_Z)->text().toStdString()} >> z;
		std::istringstream{m_propvecs->item(row, PROP_COL_CONJ)->text().toStdString()} >> iConj;

		pt::ptree itemNode;
		itemNode.put<std::string>("name", m_propvecs->item(row, PROP_COL_NAME)->text().toStdString());
		itemNode.put<t_real>("x", x);
		itemNode.put<t_real>("y", y);
		itemNode.put<t_real>("z", z);
		itemNode.put<int>("conjFC", iConj);

		node.add_child("sfact.propvecs.vec", itemNode);
	}


	std::ofstream ofstr{filename.toStdString()};
	if(!ofstr)
	{
		QMessageBox::critical(this, "Structure Factors", "Cannot open file for writing.");
		return;
	}
	ofstr.precision(g_prec);
	pt::write_xml(ofstr, node, pt::xml_writer_make_settings('\t', 1, std::string{"utf-8"}));
}


/**
 * load an mCIF
 */
void MagStructFactDlg::ImportCIF()
{/*
	QString dirLast = m_sett->value("dir_cif", "").toString();
	QString filename = QFileDialog::getOpenFileName(this, "Import CIF", dirLast, "CIF Files (*.cif *.CIF)");
	if(filename=="" || !QFile::exists(filename))
		return;
	m_sett->setValue("dir_cif", QFileInfo(filename).path());

	auto [errstr, atoms, generatedatoms, atomnames, lattice, symops] =
		load_cif<t_vec, t_mat>(filename.toStdString(), g_eps);
	if(errstr)
	{
		QMessageBox::critical(this, "Structure Factors", errstr);
		return;
	}


	// clear old nuclei
	DelTabItem(-1);
	DelPropItem(-1);

	// lattice
	{
		std::ostringstream ostr; ostr.precision(g_prec); ostr << lattice.a;
		m_editA->setText(ostr.str().c_str());
	}
	{
		std::ostringstream ostr; ostr.precision(g_prec); ostr << lattice.b;
		m_editB->setText(ostr.str().c_str());
	}
	{
		std::ostringstream ostr; ostr.precision(g_prec); ostr << lattice.c;
		m_editC->setText(ostr.str().c_str());
	}
	{
		std::ostringstream ostr; ostr.precision(g_prec); ostr << lattice.alpha;
		m_editAlpha->setText(ostr.str().c_str());
	}
	{
		std::ostringstream ostr; ostr.precision(g_prec); ostr << lattice.beta;
		m_editBeta->setText(ostr.str().c_str());
	}
	{
		std::ostringstream ostr; ostr.precision(g_prec); ostr << lattice.gamma;
		m_editGamma->setText(ostr.str().c_str());
	}
	CalcB(false);


	// atoms
	std::mt19937 gen{tl2::epoch<unsigned int>()};
	for(std::size_t atomnum=0; atomnum<atoms.size(); ++atomnum)
	{
		// random colour
		std::ostringstream ostrcol;
		std::uniform_int_distribution<int> dist{0, 255};
		ostrcol << "#" << std::hex << std::setw(2) << std::setfill('0') << dist(gen)
			<< std::setw(2) << std::setfill('0') << dist(gen)
			<< std::setw(2) << std::setfill('0') << dist(gen);

		for(std::size_t symnr=0; symnr<generatedatoms[atomnum].size(); ++symnr)
		{
			AddTabItem(-1, atomnames[atomnum], 0, 0,
				generatedatoms[atomnum][symnr][0],  generatedatoms[atomnum][symnr][1], generatedatoms[atomnum][symnr][2],
				1, ostrcol.str());
		}
	}*/
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

		auto ops = m_SGops[sgidx];
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
		m_crystA *= t_real_gl(2)*tl2::pi<t_real_gl>;
	}

	if(m_plot)
	{
		t_mat_gl matA{m_crystA};
		m_plot->GetImpl()->SetBTrafo(m_crystB, &matA);
	}
	if(m_plotSC)
	{
		t_mat_gl matA{m_crystA};
		m_plotSC->GetImpl()->SetBTrafo(m_crystB, &matA);
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


	for(t_real h=-maxBZ; h<=maxBZ; ++h)
	{
		for(t_real k=-maxBZ; k<=maxBZ; ++k)
		{
			for(t_real l=-maxBZ; l<=maxBZ; ++l)
			{
				for(const auto& prop : propvecs)
				{
					auto Q = tl2::create<t_vec>({ h,k,l }) + prop;
					auto Q_invA = m_crystB * Q;
					auto Qabs_invA = tl2::norm(Q_invA);

					// magnetic structure factor
					auto Fm = p * tl2::structure_factor<t_vec, t_vec_cplx>(Ms, pos, Q, nullptr);
					bool Fm_is_zero = 1;
					for(auto &comp : Fm)
					{
						if(tl2::equals<t_real>(comp.real(), t_real(0), g_eps)) comp.real(0.); else Fm_is_zero = 0;
						if(tl2::equals<t_real>(comp.imag(), t_real(0), g_eps)) comp.imag(0.); else Fm_is_zero = 0;
					}
					if(Fm.size() == 0)
						Fm = tl2::zero<t_vec_cplx>(3);

					// neutron scattering: orthogonal projection onto plane with normal Q.
					auto Fm_perp = tl2::ortho_project<t_vec_cplx>(
						Fm, tl2::create<t_vec_cplx>({Q[0], Q[1], Q[2]}), false);
					for(auto &comp : Fm_perp)
					{
						if(tl2::equals<t_real>(comp.real(), t_real(0), g_eps)) comp.real(0.);
						if(tl2::equals<t_real>(comp.imag(), t_real(0), g_eps)) comp.imag(0.);
					}

					t_real I = (std::conj(Fm[0])*Fm[0] +
						std::conj(Fm[1])*Fm[1] +
						std::conj(Fm[2])*Fm[2]).real();
					t_real I_perp = (std::conj(Fm_perp[0])*Fm_perp[0] +
						std::conj(Fm_perp[1])*Fm_perp[1] +
						std::conj(Fm_perp[2])*Fm_perp[2]).real();

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
			m_plotSC->GetImpl()->RemoveObject(obj);
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

	for(t_real sc_x=-maxSCx; sc_x<=maxSCx; ++sc_x)
	{
		for(t_real sc_y=-maxSCy; sc_y<=maxSCy; ++sc_y)
		{
			for(t_real sc_z=-maxSCz; sc_z<=maxSCz; ++sc_z)
			{
				auto vecCellCentre = tl2::create<t_vec>({ sc_x, sc_y, sc_z }) + vecCentring;

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

					qreal r=1, g=1, b=1;
					QColor col{colstr.c_str()};
					col.getRgbF(&r, &g, &b);

					for(std::size_t propidx=0; propidx<propvecs.size(); ++propidx)
					{
						const auto& propvec = propvecs[propidx];
						auto *pfourier = conjFCs[propidx] ? &fourier_conj : &fourier;
						moment += *pfourier * std::exp(t_cplx{0,1}*tl2::pi<t_real>*t_real{2} * tl2::inner<t_vec>(propvec, vecCellCentre));
					}

					for(auto &comp : moment)
					{
						if(tl2::equals<t_real>(comp.real(), t_real(0), g_eps)) comp.real(0.);
						if(tl2::equals<t_real>(comp.imag(), t_real(0), g_eps)) comp.imag(0.);
					}


					// add 3d objs to super cell view
					if(m_plotSC)
					{
						auto objArrowRe = m_plotSC->GetImpl()->AddLinkedObject(m_arrowSC, 0,0,0, 1,1,1,1);
						auto objArrowIm = m_plotSC->GetImpl()->AddLinkedObject(m_arrowSC, 0,0,0, 1,1,1,1);

						auto [_vecReM, _vecImM] = tl2::split_cplx<t_vec_cplx, t_vec>(moment);
						auto vecReM = tl2::convert<t_vec_gl>(_vecReM);
						auto vecImM = tl2::convert<t_vec_gl>(_vecImM);

						auto normReM = tl2::norm<t_vec_gl>(vecReM);
						auto normImM = tl2::norm<t_vec_gl>(vecImM);

						t_mat_gl matArrowRe = GlPlot_impl::GetArrowMatrix(
							vecReM, 									// to
							1, 											// post-scale
							tl2::create<t_vec_gl>({0, 0, 0}),				// post-translate
							tl2::create<t_vec_gl>({0, 0, 1}),				// from
							normReM*scale, 								// pre-scale
							posGL										// pre-translate
						);

						t_mat_gl matArrowIm = GlPlot_impl::GetArrowMatrix(
							vecImM, 									// to
							1, 											// post-scale
							tl2::create<t_vec_gl>({0, 0, 0}),				// post-translate
							tl2::create<t_vec_gl>({0, 0, 1}),				// from
							normImM*scale, 								// pre-scale
							posGL										// pre-translate
						);

						// labels
						std::ostringstream ostrMom;
						ostrMom.precision(g_prec);
						ostrMom
							<< "Re{M} = (" << moment[0].real() << " " << moment[1].real() << " " << moment[2].real() << "); "
							<< "Im{M} = (" << moment[0].imag() << " " << moment[1].imag() << " " << moment[2].imag() << "); "
							<< "r = (" << thepos[0] << " " << thepos[1] << " " << thepos[2] << ")";

						m_plotSC->GetImpl()->SetObjectMatrix(objArrowRe, matArrowRe);
						m_plotSC->GetImpl()->SetObjectMatrix(objArrowIm, matArrowIm);
						m_plotSC->GetImpl()->SetObjectCol(objArrowRe, r, g, b, 1.);
						m_plotSC->GetImpl()->SetObjectCol(objArrowIm, 1.-r, 1.-g, 1.-b, 1.);
						//m_plotSC->GetImpl()->SetObjectLabel(objArrowRe, name + " (real)");
						//m_plotSC->GetImpl()->SetObjectLabel(objArrowIm, name + " (imag)");
						m_plotSC->GetImpl()->SetObjectDataString(objArrowRe, name + " (real); " + ostrMom.str());
						m_plotSC->GetImpl()->SetObjectDataString(objArrowIm, name + " (imag); " + ostrMom.str());
						m_plotSC->GetImpl()->SetObjectVisible(objArrowRe, !tl2::equals<t_real_gl>(normReM, 0, g_eps));
						m_plotSC->GetImpl()->SetObjectVisible(objArrowIm, !tl2::equals<t_real_gl>(normImM, 0, g_eps));

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
		}
	}

	if(m_plotSC) m_plotSC->update();
	m_moments->setPlainText(ostrMoments.str().c_str());
}
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
/**
 * mouse hovers over 3d object in unit cell view
 */
void MagStructFactDlg::PickerIntersection(const t_vec3_gl* pos, std::size_t objIdx, const t_vec3_gl* posSphere)
{
	if(pos && m_plot)
		m_curPickedObj = long(objIdx);
	else
		m_curPickedObj = -1;

	if(m_curPickedObj > 0)
	{
		// find corresponding nucleus in table
		for(int row=0; row<m_nuclei->rowCount(); ++row)
		{
			std::size_t objSphere = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+0).toUInt();
			std::size_t objArrowRe = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+1).toUInt();
			std::size_t objArrowIm = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+2).toUInt();

			if(long(objSphere)==m_curPickedObj || long(objArrowRe)==m_curPickedObj || long(objArrowIm)==m_curPickedObj)
			{
				auto *itemname = m_nuclei->item(row, COL_NAME);
				auto *itemX = m_nuclei->item(row, COL_X);
				auto *itemY = m_nuclei->item(row, COL_Y);
				auto *itemZ = m_nuclei->item(row, COL_Z);

				t_vec r = tl2::create<t_vec>({0,0,0});
				std::istringstream{itemX->text().toStdString()} >> r[0];
				std::istringstream{itemY->text().toStdString()} >> r[1];
				std::istringstream{itemZ->text().toStdString()} >> r[2];
				t_vec rlab = m_crystA * r;

				std::ostringstream ostr; ostr.precision(g_prec);
				ostr << itemname->text().toStdString();
				ostr << "; r = (" << r[0] << ", " << r[1] << ", " << r[2] << ") rlu";
				ostr << "; r = (" << rlab[0] << ", " << rlab[1] << ", " << rlab[2] << ") A";

				m_status3D->setText(ostr.str().c_str());
				break;
			}
		}
	}
	else
		m_status3D->setText("");
}


/**
 * mouse hovers over 3d object in super cell view
 */
void MagStructFactDlg::PickerIntersectionSC(const t_vec3_gl* pos, std::size_t objIdx, const t_vec3_gl* posSphere)
{
	if(pos && m_plotSC)
	{
		const std::string& str = m_plotSC->GetImpl()->GetObjectDataString(objIdx);
		m_status3DSC->setText(str.c_str());
	}
	else
		m_status3DSC->setText("");
}


/**
 * mouse button pressed
 */
void MagStructFactDlg::PlotMouseDown(bool left, bool mid, bool right)
{
	if(left && m_curPickedObj > 0)
	{
		// find corresponding nucleus in table
		for(int row=0; row<m_nuclei->rowCount(); ++row)
		{
			std::size_t objSphere = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+0).toUInt();
			std::size_t objArrowRe = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+1).toUInt();
			std::size_t objArrowIm = m_nuclei->item(row, COL_NAME)->data(Qt::UserRole+2).toUInt();

			if(long(objSphere)==m_curPickedObj || long(objArrowRe)==m_curPickedObj || long(objArrowIm)==m_curPickedObj)
			{
				m_nuclei->setCurrentCell(row, 0);
				break;
			}
		}
	}
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
/**
 * initialise objects for unit cell view
 */
void MagStructFactDlg::AfterGLInitialisation()
{
	if(!m_plot) return;
	SetGLInfos();

	// reference sphere and arrow for linked objects
	m_sphere = m_plot->GetImpl()->AddSphere(0.05, 0.,0.,0., 1.,1.,1.,1.);
	m_arrow = m_plot->GetImpl()->AddArrow(0.015, 0.25, 0.,0.,0.5,  1.,1.,1.,1.);
	m_plot->GetImpl()->SetObjectVisible(m_sphere, false);
	m_plot->GetImpl()->SetObjectVisible(m_arrow, false);

	// B matrix
	m_plot->GetImpl()->SetBTrafo(m_crystB);

	// add all 3d objects
	Add3DItem(-1);
}


/**
 * initialise objects for super cell view
 */
void MagStructFactDlg::AfterGLInitialisationSC()
{
	if(!m_plotSC) return;
	SetGLInfos();

	// reference sphere and arrow for linked objects
	m_sphereSC = m_plotSC->GetImpl()->AddSphere(0.05, 0.,0.,0., 1.,1.,1.,1.);
	m_arrowSC = m_plotSC->GetImpl()->AddArrow(0.015, 0.25, 0.,0.,0.5,  1.,1.,1.,1.);
	m_plotSC->GetImpl()->SetObjectVisible(m_sphereSC, false);
	m_plotSC->GetImpl()->SetObjectVisible(m_arrowSC, false);

	// B matrix
	m_plotSC->GetImpl()->SetBTrafo(m_crystB);

	// add all 3d objects (generated in calc)
	Calc();
}


/**
 * set descriptions of the gl device in the info tab
 */
void MagStructFactDlg::SetGLInfos()
{
	static bool already_set = 0;
	if(already_set) return;

	// try whichever gl plotter is available first
	for(auto* plot : { m_plot.get(), m_plotSC.get() })
	{
		if(!plot) continue;

		auto [strGlVer, strGlShaderVer, strGlVendor, strGlRenderer] = plot->GetImpl()->GetGlDescr();
		m_labelGlInfos[0]->setText(QString("GL Version: ") + strGlVer.c_str() + QString("."));
		m_labelGlInfos[1]->setText(QString("GL Shader Version: ") + strGlShaderVer.c_str() + QString("."));
		m_labelGlInfos[2]->setText(QString("GL Vendor: ") + strGlVendor.c_str() + QString("."));
		m_labelGlInfos[3]->setText(QString("GL Device: ") + strGlRenderer.c_str() + QString("."));

		already_set = 1;
		break;
	}
}


void MagStructFactDlg::closeEvent(QCloseEvent *evt)
{
	if(m_sett)
	{
		m_sett->setValue("geo", saveGeometry());
		if(m_dlgPlot)
			m_sett->setValue("geo_3dview", m_dlgPlot->saveGeometry());
		if(m_dlgPlotSC)
			m_sett->setValue("geo_3dview_sc", m_dlgPlotSC->saveGeometry());
	}
}
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
#ifndef BUILD_LIB	// build application


int main(int argc, char** argv)
{
	set_gl_format(1, _GL_MAJ_VER, _GL_MIN_VER, 8);
	tl2::set_locales();

	QApplication::addLibraryPath(QString(".") + QDir::separator() + "qtplugins");
	auto app = std::make_unique<QApplication>(argc, argv);
	auto dlg = std::make_unique<MagStructFactDlg>(nullptr);
	dlg->show();

	return app->exec();
}


#else	// build library


#include <boost/dll/alias.hpp>


/**
 * initialise plugin
 */
bool init()
{
	set_gl_format(1, _GL_MAJ_VER, _GL_MIN_VER, 8);
	tl2::set_locales();

	return true;
}


/**
 * plugin descriptor
 * type; title; description
 */
const char* descr()
{
	return "dlg;Magnetic Structure Factors;Calculates magnetic structure factors.";
}


/**
 * create the plugin main dialog
 */
//std::shared_ptr<QDialog> create(QWidget *pParent)
QDialog* create(QWidget *pParent)
{
	//std::cout << "In " << __FUNCTION__ << std::endl;
	//return std::make_shared<MagStructFactDlg>(pParent);
	return new MagStructFactDlg(pParent);
}


/**
 * destroy the plugin main dialog
 */
void destroy(QDialog* dlg)
{
	//std::cout << "In " << __FUNCTION__ << std::endl;
	if(dlg) delete dlg;
}


BOOST_DLL_ALIAS(init, tl_init);
BOOST_DLL_ALIAS(descr, tl_descr);
BOOST_DLL_ALIAS(create, tl_create);
BOOST_DLL_ALIAS(destroy, tl_destroy);


#endif
// ----------------------------------------------------------------------------
