/**
 * structure factor tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date Dec-2018
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 28-Dec-2018 from my privately developed "misc" project (https://github.com/t-weber/misc).
 */

#include "structfact.h"

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
namespace algo = boost::algorithm;
namespace pt = boost::property_tree;

#include "loadcif.h"
#include "tlibs2/libs/algos.h"
#include "tlibs2/libs/helper.h"


//using namespace tl2;
using namespace tl2_ops;


constexpr t_real g_eps = 1e-6;
constexpr int g_prec = 6;

std::vector<std::string> StructFactDlg::g_default_colours
{{
	"#ff0000", "#0000ff", "#00ff00", "#000000",
}};


enum : int
{
	COL_NAME = 0,
	COL_SCATLEN_RE,
	COL_SCATLEN_IM,
	COL_X, COL_Y, COL_Z,
	COL_RAD,
	COL_COL,

	NUM_COLS
};


struct PowderLine
{
	t_real Q{};
	t_real I{};
	std::size_t num_peaks = 0;
	std::string peaks;
};


// ----------------------------------------------------------------------------
StructFactDlg::StructFactDlg(QWidget* pParent) : QDialog{pParent},
	m_sett{new QSettings{"takin", "structfact"}}
{
	setWindowTitle("Nuclear Structure Factors");
	setSizeGripEnabled(true);
	setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));


	auto tabs = new QTabWidget(this);


	{ // nuclei panel
		QWidget *nucleipanel = new QWidget(this);

		m_nuclei = new QTableWidget(nucleipanel);
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
		m_nuclei->setHorizontalHeaderItem(COL_RAD, new QTableWidgetItem{"Radius"});
		m_nuclei->setHorizontalHeaderItem(COL_COL, new QTableWidgetItem{"Colour"});

		m_nuclei->setColumnWidth(COL_NAME, 90);
		m_nuclei->setColumnWidth(COL_SCATLEN_RE, 75);
		m_nuclei->setColumnWidth(COL_SCATLEN_IM, 75);
		m_nuclei->setColumnWidth(COL_X, 75);
		m_nuclei->setColumnWidth(COL_Y, 75);
		m_nuclei->setColumnWidth(COL_Z, 75);
		m_nuclei->setColumnWidth(COL_RAD, 75);
		m_nuclei->setColumnWidth(COL_COL, 75);

		QToolButton *pTabBtnAdd = new QToolButton(nucleipanel);
		QToolButton *pTabBtnDel = new QToolButton(nucleipanel);
		QToolButton *pTabBtnUp = new QToolButton(nucleipanel);
		QToolButton *pTabBtnDown = new QToolButton(nucleipanel);
		QToolButton *pTabBtnSG = new QToolButton(nucleipanel);

		m_nuclei->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});
		pTabBtnAdd->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDel->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnUp->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDown->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnSG->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});

		pTabBtnAdd->setText("Add Nucleus");
		pTabBtnDel->setText("Delete Nuclei");
		pTabBtnUp->setText("Move Nuclei Up");
		pTabBtnDown->setText("Move Nuclei Down");
		pTabBtnSG->setText("Generate");

		m_editA = new QLineEdit("5", nucleipanel);
		m_editB = new QLineEdit("5", nucleipanel);
		m_editC = new QLineEdit("5", nucleipanel);
		m_editAlpha = new QLineEdit("90", nucleipanel);
		m_editBeta = new QLineEdit("90", nucleipanel);
		m_editGamma = new QLineEdit("90", nucleipanel);

		m_comboSG = new QComboBox(nucleipanel);


		// get space groups and symops
		for(auto [sgnum, descr, ops] : get_sgs<t_mat>())
		{
			m_comboSG->addItem(descr.c_str(), m_comboSG->count());
			m_SGops.emplace_back(std::move(ops));
		}


		auto pTabGrid = new QGridLayout(nucleipanel);
		pTabGrid->setSpacing(2);
		pTabGrid->setContentsMargins(4,4,4,4);
		int y=0;
		//pTabGrid->addWidget(m_plot.get(), y,0,1,4);
		pTabGrid->addWidget(m_nuclei, y,0,1,4);
		pTabGrid->addWidget(pTabBtnAdd, ++y,0,1,1);
		pTabGrid->addWidget(pTabBtnDel, y,1,1,1);
		pTabGrid->addWidget(pTabBtnUp, y,2,1,1);
		pTabGrid->addWidget(pTabBtnDown, y,3,1,1);

		pTabGrid->addWidget(new QLabel("Space Groups:"), ++y,0,1,1);
		pTabGrid->addWidget(m_comboSG, y,1,1,2);
		pTabGrid->addWidget(pTabBtnSG, y,3,1,1);


		auto sep1 = new QFrame(nucleipanel); sep1->setFrameStyle(QFrame::HLine);
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
		m_pTabContextMenu->addAction("Delete Nucleus", this, [this]() { StructFactDlg::DelTabItem(); });


		// table CustomContextMenu in case nothing is selected
		m_pTabContextMenuNoItem = new QMenu(m_nuclei);
		m_pTabContextMenuNoItem->addAction("Add Nucleus", this, [this]() { this->AddTabItem(); });
		m_pTabContextMenuNoItem->addAction("Delete Nucleus", this, [this]() { StructFactDlg::DelTabItem(); });
		//m_pTabContextMenuNoItem->addSeparator();


		// signals
		for(auto* edit : std::vector<QLineEdit*>{{ m_editA, m_editB, m_editC, m_editAlpha, m_editBeta, m_editGamma }})
			connect(edit, &QLineEdit::textEdited, this, [this]() { this->CalcB(); });

		connect(pTabBtnAdd, &QToolButton::clicked, this, [this]() { this->AddTabItem(-1); });
		connect(pTabBtnDel, &QToolButton::clicked, this, [this]() { StructFactDlg::DelTabItem(); });
		connect(pTabBtnUp, &QToolButton::clicked, this, &StructFactDlg::MoveTabItemUp);
		connect(pTabBtnDown, &QToolButton::clicked, this, &StructFactDlg::MoveTabItemDown);
		connect(pTabBtnSG, &QToolButton::clicked, this, &StructFactDlg::GenerateFromSG);

		connect(m_nuclei, &QTableWidget::currentCellChanged, this, &StructFactDlg::TableCurCellChanged);
		connect(m_nuclei, &QTableWidget::entered, this, &StructFactDlg::TableCellEntered);
		connect(m_nuclei, &QTableWidget::itemChanged, this, &StructFactDlg::TableItemChanged);
		connect(m_nuclei, &QTableWidget::customContextMenuRequested, this, &StructFactDlg::ShowTableContextMenu);

		tabs->addTab(nucleipanel, "Nuclei");
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


	{ // find space group panel
		auto findsgpanel = new QWidget(this);

		m_nuclei_FindSG = new QTableWidget(findsgpanel);
		m_nuclei_FindSG->setShowGrid(true);
		m_nuclei_FindSG->setSortingEnabled(true);
		m_nuclei_FindSG->setMouseTracking(true);
		m_nuclei_FindSG->setSelectionBehavior(QTableWidget::SelectRows);
		m_nuclei_FindSG->setSelectionMode(QTableWidget::ContiguousSelection);
		m_nuclei_FindSG->setContextMenuPolicy(Qt::CustomContextMenu);

		m_nuclei_FindSG->verticalHeader()->setDefaultSectionSize(fontMetrics().lineSpacing() + 4);
		m_nuclei_FindSG->verticalHeader()->setVisible(false);

		m_nuclei_FindSG->setColumnCount(3);
		m_nuclei_FindSG->setHorizontalHeaderItem(0, new QTableWidgetItem{"x (frac.)"});
		m_nuclei_FindSG->setHorizontalHeaderItem(1, new QTableWidgetItem{"y (frac.)"});
		m_nuclei_FindSG->setHorizontalHeaderItem(2, new QTableWidgetItem{"z (frac.)"});

		m_nuclei_FindSG->setColumnWidth(0, 200);
		m_nuclei_FindSG->setColumnWidth(1, 200);
		m_nuclei_FindSG->setColumnWidth(2, 200);

		m_sgmatches = new QPlainTextEdit(findsgpanel);
		m_sgmatches->setReadOnly(true);
		m_sgmatches->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));


		QToolButton *pTabBtnAdd = new QToolButton(findsgpanel);
		QToolButton *pTabBtnDel = new QToolButton(findsgpanel);
		QToolButton *pTabBtnUp = new QToolButton(findsgpanel);
		QToolButton *pTabBtnDown = new QToolButton(findsgpanel);
		QToolButton *pBtnCalc = new QToolButton(findsgpanel);

		m_nuclei_FindSG->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});
		pTabBtnAdd->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDel->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnUp->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pTabBtnDown->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});
		pBtnCalc->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Fixed});

		pTabBtnAdd->setText("Add Nucleus");
		pTabBtnDel->setText("Delete Nuclei");
		pTabBtnUp->setText("Move Nuclei Up");
		pTabBtnDown->setText("Move Nuclei Down");
		pBtnCalc->setText("Find Matching Space Groups");


		auto pTabGrid = new QGridLayout(findsgpanel);
		pTabGrid->setSpacing(2);
		pTabGrid->setContentsMargins(4,4,4,4);
		int y=0;
		pTabGrid->addWidget(m_nuclei_FindSG, y,0,1,4);
		pTabGrid->addWidget(m_sgmatches, ++y,0,1,4);
		pTabGrid->addWidget(pTabBtnAdd, ++y,0,1,1);
		pTabGrid->addWidget(pTabBtnDel, y,1,1,1);
		pTabGrid->addWidget(pTabBtnUp, y,2,1,1);
		pTabGrid->addWidget(pTabBtnDown, y,3,1,1);
		pTabGrid->addWidget(pBtnCalc, ++y,2,1,2);


		// table CustomContextMenu
		m_pTabContextMenu_FindSG = new QMenu(m_nuclei_FindSG);
		m_pTabContextMenu_FindSG->addAction("Add Nucleus Before", this, [this]() { this->AddTabItem_FindSG(-2); });
		m_pTabContextMenu_FindSG->addAction("Add Nucleus After", this, [this]() { this->AddTabItem_FindSG(-3); });
		m_pTabContextMenu_FindSG->addAction("Clone Nucleus", this, [this]() { this->AddTabItem_FindSG(-4); });
		m_pTabContextMenu_FindSG->addAction("Delete Nucleus", this, [this]() { StructFactDlg::DelTabItem_FindSG(); });


		// table CustomContextMenu in case nothing is selected
		m_pTabContextMenuNoItem_FindSG = new QMenu(m_nuclei_FindSG);
		m_pTabContextMenuNoItem_FindSG->addAction("Add Nucleus", this, [this]() { this->AddTabItem_FindSG(); });
		m_pTabContextMenuNoItem_FindSG->addAction("Delete Nucleus", this, [this]() { StructFactDlg::DelTabItem_FindSG(); });


		// signals
		connect(pTabBtnAdd, &QToolButton::clicked, this, [this]() { this->AddTabItem_FindSG(-1); });
		connect(pTabBtnDel, &QToolButton::clicked, this, [this]() { StructFactDlg::DelTabItem_FindSG(); });
		connect(pTabBtnUp, &QToolButton::clicked, this, &StructFactDlg::MoveTabItemUp_FindSG);
		connect(pTabBtnDown, &QToolButton::clicked, this, &StructFactDlg::MoveTabItemDown_FindSG);
		connect(pBtnCalc, &QToolButton::clicked, this, &StructFactDlg::FindSG);

		connect(m_nuclei_FindSG, &QTableWidget::itemChanged, this, &StructFactDlg::TableItemChanged_FindSG);
		connect(m_nuclei_FindSG, &QTableWidget::customContextMenuRequested, this, &StructFactDlg::ShowTableContextMenu_FindSG);

		tabs->addTab(findsgpanel, "Match Space Group");
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

		auto labelTitle = new QLabel("Nuclear Structure Factor Calculator", infopanel);
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


	// menu bar
	{
		m_menu = new QMenuBar(this);
		m_menu->setNativeMenuBar(m_sett ? m_sett->value("native_gui", false).toBool() : false);

		auto menuFile = new QMenu("File", m_menu);
		auto menuView = new QMenu("View", m_menu);

		auto acNew = new QAction("New", menuFile);
		auto acLoad = new QAction("Load...", menuFile);
		auto acSave = new QAction("Save...", menuFile);
		auto acImportCIF = new QAction("Import CIF...", menuFile);
		auto acImportTAZ = new QAction("Import TAZ...", menuFile);
		auto acExportTAZ = new QAction("Export TAZ...", menuFile);
		auto acExit = new QAction("Quit", menuFile);
		auto ac3DView = new QAction("3D View...", menuFile);

		menuFile->addAction(acNew);
		menuFile->addSeparator();
		menuFile->addAction(acLoad);
		menuFile->addAction(acSave);
		menuFile->addSeparator();
		menuFile->addAction(acImportCIF);
		menuFile->addSeparator();
		menuFile->addAction(acImportTAZ);
		menuFile->addAction(acExportTAZ);
		menuFile->addSeparator();
		menuFile->addAction(acExit);
		menuView->addAction(ac3DView);

		connect(acNew, &QAction::triggered, this,  [this]()
		{
			// clear old table
			DelTabItem(-1);

			// set some defaults
			m_comboSG->setCurrentIndex(0);
			m_editA->setText("5");
			m_editB->setText("5");
			m_editC->setText("5");
			m_editAlpha->setText("90");
			m_editBeta->setText("90");
			m_editGamma->setText("90");
		});
		connect(acLoad, &QAction::triggered, this, &StructFactDlg::Load);
		connect(acSave, &QAction::triggered, this, &StructFactDlg::Save);
		connect(acImportCIF, &QAction::triggered, this, &StructFactDlg::ImportCIF);
		connect(acImportTAZ, &QAction::triggered, this, &StructFactDlg::ImportTAZ);
		connect(acExportTAZ, &QAction::triggered, this, &StructFactDlg::ExportTAZ);
		connect(acExit, &QAction::triggered, this, &QDialog::close);
		connect(ac3DView, &QAction::triggered, this, [this]()
		{
			// plot widget
			if(!m_dlgPlot)
			{
				m_dlgPlot = new QDialog(this);
				m_dlgPlot->setWindowTitle("Unit Cell - 3D View");

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


				connect(m_plot.get(), &GlPlot::AfterGLInitialisation, this, &StructFactDlg::AfterGLInitialisation);
				connect(m_plot->GetImpl(), &GlPlot_impl::PickerIntersection, this, &StructFactDlg::PickerIntersection);
				connect(m_plot.get(), &GlPlot::MouseDown, this, &StructFactDlg::PlotMouseDown);
				connect(m_plot.get(), &GlPlot::MouseUp, this, &StructFactDlg::PlotMouseUp);
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

		m_menu->addMenu(menuFile);
		m_menu->addMenu(menuView);
		pmainGrid->setMenuBar(m_menu);
	}


	// restore window size and position
	if(m_sett && m_sett->contains("geo"))
		restoreGeometry(m_sett->value("geo").toByteArray());
	else
		resize(600, 500);


	m_ignoreChanges = 0;
}



// ============================================================================
// functions for nuclei tab

// ----------------------------------------------------------------------------

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
		m_nuclei->setItem(row, COL_SCATLEN_RE, new NumericTableWidgetItem<t_real>(bRe));
		m_nuclei->setItem(row, COL_SCATLEN_IM, new NumericTableWidgetItem<t_real>(bIm));
		m_nuclei->setItem(row, COL_X, new NumericTableWidgetItem<t_real>(x));
		m_nuclei->setItem(row, COL_Y, new NumericTableWidgetItem<t_real>(y));
		m_nuclei->setItem(row, COL_Z, new NumericTableWidgetItem<t_real>(z));
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

	qreal r=1, g=1, b=1;
	QColor col{itemCol->text()};
	col.getRgbF(&r, &g, &b);

	auto obj = m_plot->GetImpl()->AddLinkedObject(m_sphere, 0,0,0, r,g,b,1);
	//auto obj = m_plot->GetImpl()->AddSphere(0.05, 0,0,0, r,g,b,1);
	m_plot->GetImpl()->SetObjectMatrix(obj, tl2::hom_translation<t_mat_gl>(posx, posy, posz)*tl2::hom_scaling<t_mat_gl>(scale,scale,scale));
	m_plot->GetImpl()->SetObjectLabel(obj, itemName->text().toStdString());
	m_plot->update();

	m_nuclei->item(row, COL_NAME)->setData(Qt::UserRole, unsigned(obj));
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
					m_plot->GetImpl()->RemoveObject(obj);
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

			qreal r=1, g=1, b=1;
			QColor col{itemCol->text()};
			col.getRgbF(&r, &g, &b);

			m_plot->GetImpl()->SetObjectMatrix(obj, tl2::hom_translation<t_mat_gl>(posx, posy, posz)*tl2::hom_scaling<t_mat_gl>(scale,scale,scale));
			m_plot->GetImpl()->SetObjectCol(obj, r, g, b, 1);
			m_plot->GetImpl()->SetObjectLabel(obj, itemName->text().toStdString());
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
// ============================================================================




// ============================================================================
// functions for space group finder tab

// ----------------------------------------------------------------------------

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
		m_nuclei_FindSG->setItem(row, 0, new NumericTableWidgetItem<t_real>(x));
		m_nuclei_FindSG->setItem(row, 1, new NumericTableWidgetItem<t_real>(y));
		m_nuclei_FindSG->setItem(row, 2, new NumericTableWidgetItem<t_real>(z));
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
void StructFactDlg::TableItemChanged_FindSG(QTableWidgetItem *item)
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

// ============================================================================



// ----------------------------------------------------------------------------
void StructFactDlg::Load()
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
		if(auto opt = node.get_optional<std::string>("sfact.meta.info"); !opt || *opt!=std::string{"sfact_tool"})
		{
			QMessageBox::critical(this, "Structure Factors", "Unrecognised file format.");
			return;
		}


		// clear old nuclei
		DelTabItem(-1);

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
		if(auto opt = node.get_optional<int>("sfact.removezeroes"); opt)
		{
			m_RemoveZeroes->setChecked(*opt != 0);
		}
		if(auto opt = node.get_optional<int>("sfact.sg_idx"); opt)
		{
			m_comboSG->setCurrentIndex(*opt);
		}


		// nuclei
		if(auto nuclei = node.get_child_optional("sfact.nuclei"); nuclei)
		{
			std::size_t nucIdx = 0;
			for(const auto &nucl : *nuclei)
			{
				auto optName = nucl.second.get<std::string>("name", "n/a");
				auto optbRe = nucl.second.get<t_real>("b_Re", 0.);
				auto optbIm = nucl.second.get<t_real>("b_Im", 0.);
				auto optX = nucl.second.get<t_real>("x", 0.);
				auto optY = nucl.second.get<t_real>("y", 0.);
				auto optZ = nucl.second.get<t_real>("z", 0.);
				auto optRad = nucl.second.get<t_real>("rad", 1.);
				auto optCol = nucl.second.get<std::string>("col", g_default_colours[nucIdx%g_default_colours.size()]);

				AddTabItem(-1, optName, optbRe, optbIm, optX,  optY, optZ, optRad, optCol);
				++nucIdx;
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
	node.put<int>("sfact.removezeroes", m_RemoveZeroes->isChecked());
	node.put<int>("sfact.sg_idx", m_comboSG->currentIndex());

	// nucleus list
	for(int row=0; row<m_nuclei->rowCount(); ++row)
	{
		t_real bRe{},bIm{}, x{},y{},z{}, scale{};
		std::istringstream{m_nuclei->item(row, COL_SCATLEN_RE)->text().toStdString()} >> bRe;
		std::istringstream{m_nuclei->item(row, COL_SCATLEN_IM)->text().toStdString()} >> bIm;
		std::istringstream{m_nuclei->item(row, COL_X)->text().toStdString()} >> x;
		std::istringstream{m_nuclei->item(row, COL_Y)->text().toStdString()} >> y;
		std::istringstream{m_nuclei->item(row, COL_Z)->text().toStdString()} >> z;
		std::istringstream{m_nuclei->item(row, COL_RAD)->text().toStdString()} >> scale;

		pt::ptree itemNode;
		itemNode.put<std::string>("name", m_nuclei->item(row, COL_NAME)->text().toStdString());
		itemNode.put<t_real>("b_Re", bRe);
		itemNode.put<t_real>("b_Im", bIm);
		itemNode.put<t_real>("x", x);
		itemNode.put<t_real>("y", y);
		itemNode.put<t_real>("z", z);
		itemNode.put<t_real>("rad", scale);
		itemNode.put<std::string>("col", m_nuclei->item(row, COL_COL)->text().toStdString());

		node.add_child("sfact.nuclei.nucleus", itemNode);
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
 * load a TAZ file
 */
void StructFactDlg::ImportTAZ()
{
	m_ignoreCalc = 1;

	try
	{
		QString dirLast = m_sett->value("dir_taz", "").toString();
		QString filename = QFileDialog::getOpenFileName(this, "Load File", dirLast, "TAZ Files (*.taz *.TAZ)");
		if(filename=="" || !QFile::exists(filename))
			return;
		m_sett->setValue("dir_taz", QFileInfo(filename).path());


		pt::ptree node;

		std::ifstream ifstr{filename.toStdString()};
		pt::read_xml(ifstr, node);

		// clear old nuclei
		DelTabItem(-1);

		if(auto opt = node.get_optional<t_real>("taz.sample.a"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editA->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("taz.sample.b"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editB->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("taz.sample.c"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editC->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("taz.sample.alpha"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editAlpha->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("taz.sample.beta"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editBeta->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("taz.sample.gamma"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editGamma->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<std::string>("taz.sample.spacegroup"); opt)
		{
			// TODO
			//std::cout << *opt << std::endl;
		}

		// nuclei
		if(auto opt = node.get_optional<std::size_t>("taz.sample.atoms.num"); opt)
		{
			std::size_t numAtoms = *opt;
			for(std::size_t atomIdx=0; atomIdx<numAtoms; ++atomIdx)
			{
				std::string strNum = tl2::var_to_str(atomIdx);

				std::string name = node.get<std::string>("taz.sample.atoms."+strNum+".name", "n/a");
				t_real x = node.get<t_real>("taz.sample.atoms."+strNum+".x", 0.);
				t_real y = node.get<t_real>("taz.sample.atoms."+strNum+".y", 0.);
				t_real z = node.get<t_real>("taz.sample.atoms."+strNum+".z", 0.);

				// TODO
				t_real bRe = 0.;
				t_real bIm = 0.;

				t_real rad = 1.;
				std::string col = g_default_colours[atomIdx%g_default_colours.size()];

				AddTabItem(-1, name, bRe, bIm, x, y, z, rad, col);
			}
		}
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Structure Factors", ex.what());
	}

	m_ignoreCalc = 0;
	GenerateFromSG();
	CalcB(false);
	Calc();
}


/**
 * save a TAZ file
 */
void StructFactDlg::ExportTAZ()
{
	QString dirLast = m_sett->value("dir_taz", "").toString();
	QString filename = QFileDialog::getSaveFileName(this, "Export TAZ", dirLast, "TAZ Files (*.taz *.TAZ)");
	if(filename=="" || !QFile::exists(filename))
		return;
	m_sett->setValue("dir_taz", QFileInfo(filename).path());

	std::ofstream ofstr{filename.toStdString()};
	if(!ofstr)
	{
		QMessageBox::critical(this, "Structure Factors", "Cannot open file for writing.");
		return;
	}
	ofstr.precision(g_prec);

	ofstr << "<taz>\n";
	ofstr << "\t<meta><info>Exported from Takin/Structfact.</info></meta>\n";

	// sample infos
	ofstr << "\t<sample>\n";
	ofstr << "\t\t<a>" << m_editA->text().toStdString() << "</a>\n";
	ofstr << "\t\t<b>" << m_editB->text().toStdString() << "</b>\n";
	ofstr << "\t\t<c>" << m_editC->text().toStdString() << "</c>\n";
	ofstr << "\t\t<alpha>" << m_editAlpha->text().toStdString() << "</alpha>\n";
	ofstr << "\t\t<beta>" << m_editBeta->text().toStdString() << "</beta>\n";
	ofstr << "\t\t<gamma>" << m_editGamma->text().toStdString() << "</gamma>\n";

	// P1 only has the identity trafo, so we can directly output all raw nucleus positions
	ofstr << "\t\t<spacegroup>P1</spacegroup>\n";

	// nucleus list
	ofstr << "\t\t<atoms>\n";
	ofstr << "\t\t\t<num>" << m_nuclei->rowCount() << "</num>\n";
	for(int row=0; row<m_nuclei->rowCount(); ++row)
	{
		ofstr << "\t\t\t<" << row << ">\n";
		ofstr << "\t\t\t\t<name>" << m_nuclei->item(row, COL_NAME)->text().toStdString() << "</name>\n";
		ofstr << "\t\t\t\t<x>" << m_nuclei->item(row, COL_X)->text().toStdString() << "</x>\n";
		ofstr << "\t\t\t\t<y>" << m_nuclei->item(row, COL_Y)->text().toStdString() << "</y>\n";
		ofstr << "\t\t\t\t<z>" << m_nuclei->item(row, COL_Z)->text().toStdString() << "</z>\n";
		ofstr << "\t\t\t</" << row << ">\n";
	}

	ofstr << "\t\t</atoms>\n";
	ofstr << "\t</sample>\n";

	ofstr << "</taz>\n";
}


/**
 * load a CIF
 */
void StructFactDlg::ImportCIF()
{
	m_ignoreCalc = 1;

	try
	{
		QString dirLast = m_sett->value("dir_cif", "").toString();
		QString filename = QFileDialog::getOpenFileName(this, "Import CIF", dirLast, "CIF Files (*.cif *.CIF)");
		if(filename=="" || !QFile::exists(filename))
			return;
		m_sett->setValue("dir_cif", QFileInfo(filename).path());

		auto [errstr, atoms, generatedatoms, atomnames, lattice, symops] =
			load_cif<t_vec, t_mat>(filename.toStdString(), g_eps);
		if(errstr != "")
		{
			QMessageBox::critical(this, "Structure Factors", errstr.c_str());
			return;
		}


		// clear old nuclei
		DelTabItem(-1);

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
// ----------------------------------------------------------------------------


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
			return;
		}

		auto ops = m_SGops[sgidx];
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
		m_crystA *= t_real_gl(2)*tl2::pi<t_real_gl>;
	}

	if(m_plot)
	{
		t_mat_gl matA{m_crystA};
		m_plot->GetImpl()->SetBTrafo(m_crystB, &matA);
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

				t_vec r = tl2::create<t_vec>({0,0,0});
				std::istringstream{itemX->text().toStdString()} >> r[0];
				std::istringstream{itemY->text().toStdString()} >> r[1];
				std::istringstream{itemZ->text().toStdString()} >> r[2];
				t_vec rlab = m_crystA * r;

				std::ostringstream ostr; ostr.precision(g_prec);
				ostr << itemname->text().toStdString();
				ostr << "; r = (" << r[0] << ", " << r[1] << ", " << r[2] << ") rlu";
				ostr << "; r = (" << rlab[0] << ", " << rlab[1] << ", " << rlab[2] << ") A";

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

	// reference sphere for linked objects
	m_sphere = m_plot->GetImpl()->AddSphere(0.05, 0.,0.,0., 1.,1.,1.,1.);
	m_plot->GetImpl()->SetObjectVisible(m_sphere, false);

	// B matrix
	m_plot->GetImpl()->SetBTrafo(m_crystB);

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
#ifndef BUILD_LIB	// build application


int main(int argc, char** argv)
{
	set_gl_format(1, _GL_MAJ_VER, _GL_MIN_VER, 8);
	tl2::set_locales();

	QApplication::addLibraryPath(QString(".") + QDir::separator() + "qtplugins");
	auto app = std::make_unique<QApplication>(argc, argv);
	auto dlg = std::make_unique<StructFactDlg>(nullptr);
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
	return "dlg;Structure Factors;Calculates nuclear structure factors.";
}


/**
 * create the plugin main dialog
 */
//std::shared_ptr<QDialog> create(QWidget *pParent)
QDialog* create(QWidget *pParent)
{
	//std::cout << "In " << __FUNCTION__ << std::endl;
	//return std::make_shared<StructFactDlg>(pParent);
	return new StructFactDlg(pParent);
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
