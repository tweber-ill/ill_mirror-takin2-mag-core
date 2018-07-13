/**
 * in20 data analysis tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date 6-Apr-2018
 * @license see 'LICENSE' file
 */

#include "mainwnd.h"
#include "globals.h"
#include "funcs.h"


MainWnd::MainWnd(QSettings* pSettings)
	: QMainWindow(), m_pSettings(pSettings), 
	m_pBrowser(new FileBrowser(this, pSettings)),
	m_pWS(new WorkSpace(this, pSettings)),
	m_pCLI(new CommandLine(this, pSettings)),
	m_pCurPlot(new PlotterDock(this))
{
	// the command line widget has to be accessible globally for error output
	g_pCLI = m_pCLI;

	this->setObjectName("in20");
	this->setWindowTitle("IN20 Tool");
	this->resize(800, 600);


	// ------------------------------------------------------------------------
	// Menu Bar
	QMenu *menuFile = new QMenu("File", m_pMenu);
	QMenu *menuView = new QMenu("View", m_pMenu);
	QMenu *menuHelp = new QMenu("Help", m_pMenu);

	// file
	auto *acNew = new QAction(QIcon::fromTheme("document-new"), "New", m_pMenu);
	menuFile->addAction(acNew);
	menuFile->addSeparator();
	auto *acOpen = new QAction(QIcon::fromTheme("document-open"), "Open...", m_pMenu);
	auto *menuOpenRecent = new QMenu("Open Recent", m_pMenu);
	menuFile->addAction(acOpen);
	menuOpenRecent->setIcon(QIcon::fromTheme("document-open-recent"));
	menuFile->addMenu(menuOpenRecent);
	menuFile->addSeparator();
	auto *acSave = new QAction(QIcon::fromTheme("document-save"), "Save", m_pMenu);
	auto *acSaveAs = new QAction(QIcon::fromTheme("document-save-as"), "Save As...", m_pMenu);
	menuFile->addAction(acSave);
	menuFile->addAction(acSaveAs);
	menuFile->addSeparator();
	auto *acPrefs = new QAction("Preferences...", m_pMenu);
	acPrefs->setMenuRole(QAction::PreferencesRole);
	menuFile->addAction(acPrefs);
	menuFile->addSeparator();
	auto *acQuit = new QAction(QIcon::fromTheme("application-exit"), "Quit", m_pMenu);
	acQuit->setMenuRole(QAction::QuitRole);
	menuFile->addAction(acQuit);

	// view
	menuView->addAction(m_pBrowser->toggleViewAction());
	menuView->addAction(m_pWS->toggleViewAction());
	menuView->addAction(m_pCLI->toggleViewAction());
	menuView->addAction(m_pCurPlot->toggleViewAction());

	// help
	auto *acAbout = new QAction(QIcon::fromTheme("help-about"), "About...", m_pMenu);
	auto *acAboutQt = new QAction(QIcon::fromTheme("help-about"), "About Qt...", m_pMenu);
	acAbout->setMenuRole(QAction::AboutRole);
	acAboutQt->setMenuRole(QAction::AboutQtRole);
	menuHelp->addAction(acAbout);
	menuHelp->addAction(acAboutQt);

	m_pMenu->addMenu(menuFile);
	m_pMenu->addMenu(menuView);
	m_pMenu->addMenu(menuHelp);
	this->setMenuBar(m_pMenu);
	// ------------------------------------------------------------------------



	// ------------------------------------------------------------------------
	// docks
	//this->setStatusBar(m_pStatus);
	this->setCentralWidget(m_pMDI);
	this->addDockWidget(Qt::LeftDockWidgetArea, m_pBrowser);
	this->addDockWidget(Qt::RightDockWidgetArea, m_pWS);
	this->addDockWidget(Qt::BottomDockWidgetArea, m_pCLI);
	this->addDockWidget(Qt::BottomDockWidgetArea, m_pCurPlot);
	// ------------------------------------------------------------------------



	// ------------------------------------------------------------------------
	// connections
	connect(m_pBrowser->GetWidget(), &FileBrowserWidget::TransferFiles, m_pWS->GetWidget(), &WorkSpaceWidget::ReceiveFiles);
	connect(m_pWS->GetWidget(), &WorkSpaceWidget::PlotDataset, m_pCurPlot->GetWidget(), &Plotter::Plot);
	connect(m_pBrowser->GetWidget(), &FileBrowserWidget::PlotDataset, m_pCurPlot->GetWidget(), &Plotter::Plot);
	connect(acNew, &QAction::triggered, this, &MainWnd::NewSession);
	connect(acOpen, &QAction::triggered, this, &MainWnd::OpenSession);
	connect(acSave, &QAction::triggered, this, &MainWnd::SaveSession);
	connect(acSaveAs, &QAction::triggered, this, &MainWnd::SaveSessionAs);
	//connect(acPrefs, &QAction::triggered, this, ....);	TODO
	//connect(acAbout, &QAction::triggered, this, ....);	TODO
	connect(acAboutQt, &QAction::triggered, this, []() { qApp->aboutQt(); });
	connect(acQuit, &QAction::triggered, this, &QMainWindow::close);

	// link symbol maps of workspace widget and command line parser
	m_pCLI->GetWidget()->GetParserContext().SetWorkspace(m_pWS->GetWidget()->GetWorkspace());

	// connect the "workspace updated" signal from the command line parser
	m_pCLI->GetWidget()->GetParserContext().GetWorkspaceUpdatedSignal().connect([this](const std::string& ident)
	{
		m_pWS->GetWidget()->UpdateList();
	});
	// ------------------------------------------------------------------------



	// ------------------------------------------------------------------------
	// restore settings
	if(m_pSettings)
	{
		// restore window state
		if(m_pSettings->contains("mainwnd/geo"))
			this->restoreGeometry(m_pSettings->value("mainwnd/geo").toByteArray());
		if(m_pSettings->contains("mainwnd/state"))
			this->restoreState(m_pSettings->value("mainwnd/state").toByteArray());
	}
	// ------------------------------------------------------------------------



	// ------------------------------------------------------------------------
	// add the built-in function list to the completer
	QStringList lstFuncs;
	for(const auto &pair : g_funcs_real_1arg) lstFuncs.push_back(((pair.first + "###" + std::get<1>(pair.second) + " [function, 1 argument]").c_str()));
	for(const auto &pair : g_funcs_real_2args) lstFuncs.push_back(((pair.first + "###" + std::get<1>(pair.second) + " [function, 2 arguments]").c_str()));
	for(const auto &pair : g_funcs_arr_1arg) lstFuncs.push_back(((pair.first + "###" + std::get<1>(pair.second) + " [function, 1 argument]").c_str()));
	for(const auto &pair : g_funcs_arr_2args) lstFuncs.push_back(((pair.first + "###" + std::get<1>(pair.second) + " [function, 2 arguments]").c_str()));
	for(const auto &pair : g_funcs_gen_0args) lstFuncs.push_back(((pair.first + "###" + std::get<1>(pair.second) + " [function, no arguments]").c_str()));
	for(const auto &pair : g_funcs_gen_1arg) lstFuncs.push_back(((pair.first + "###" + std::get<1>(pair.second) + " [function, 1 argument]").c_str()));
	for(const auto &pair : g_funcs_gen_2args) lstFuncs.push_back(((pair.first + "###" + std::get<1>(pair.second) + " [function, 2 arguments]").c_str()));
	for(const auto &pair : g_funcs_gen_vararg) lstFuncs.push_back(((pair.first + "###" + std::get<1>(pair.second) + " [function, variable arguments]").c_str()));
	for(const auto &pair : g_consts_real) lstFuncs.push_back(((pair.first + "###" + std::get<1>(pair.second) + " [constant]").c_str()));
	m_pCLI->GetWidget()->SetCompleterItems(lstFuncs);
	// ------------------------------------------------------------------------
}


MainWnd::~MainWnd()
{}


void MainWnd::showEvent(QShowEvent *pEvt)
{
	QMainWindow::showEvent(pEvt);
}


void MainWnd::closeEvent(QCloseEvent *pEvt)
{
	if(m_pSettings)
	{
		// save window state
		m_pSettings->setValue("mainwnd/geo", this->saveGeometry());
		m_pSettings->setValue("mainwnd/state", this->saveState());
	}

	QMainWindow::closeEvent(pEvt);
}



/**
 * File -> New
 */
void MainWnd::NewSession()
{

}


/**
 * File -> Open
 */
void MainWnd::OpenSession()
{

}


/**
 * File -> Save
 */
void MainWnd::SaveSession()
{

}


/**
 * File -> Save As
 */
void MainWnd::SaveSessionAs()
{

}

