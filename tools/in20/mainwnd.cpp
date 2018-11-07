/**
 * in20 data analysis tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date 6-Apr-2018
 * @license see 'LICENSE' file
 */

#define MAX_RECENT_FILES 16

#include "mainwnd.h"
#include "globals.h"
#include "funcs.h"
#include "libs/file.h"
#include "libs/algos.h"

#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>


using t_real = double;


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
	SetCurrentFile("");
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
	m_menuOpenRecent = new QMenu("Open Recent", m_pMenu);
	menuFile->addAction(acOpen);
	m_menuOpenRecent->setIcon(QIcon::fromTheme("document-open-recent"));
	menuFile->addMenu(m_menuOpenRecent);
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
	connect(acNew, &QAction::triggered, this, &MainWnd::NewFile);
	connect(acOpen, &QAction::triggered, this, static_cast<void(MainWnd::*)()>(&MainWnd::OpenFile));
	connect(acSave, &QAction::triggered, this, static_cast<void(MainWnd::*)()>(&MainWnd::SaveFile));
	connect(acSaveAs, &QAction::triggered, this, &MainWnd::SaveFileAs);
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
		if(m_pSettings->contains("mainwnd/recentsessions"))
			SetRecentFiles(m_pSettings->value("mainwnd/recentsessions").toStringList());

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
		// remove superfluous entries and save the recent files list
		while(m_recentFiles.size() > MAX_RECENT_FILES)
			m_recentFiles.pop_front();
		m_pSettings->setValue("mainwnd/recentsessions", m_recentFiles);

		// save window state
		m_pSettings->setValue("mainwnd/geo", this->saveGeometry());
		m_pSettings->setValue("mainwnd/state", this->saveState());
	}

	QMainWindow::closeEvent(pEvt);
}



/**
 * File -> New
 */
void MainWnd::NewFile()
{
	SetCurrentFile("");
	m_pWS->GetWidget()->GetWorkspace()->clear();
	m_pWS->GetWidget()->UpdateList();
}


/**
 * File -> Open
 */
void MainWnd::OpenFile()
{
	QString dirLast = m_pSettings->value("mainwnd/sessiondir", "").toString();

	QString filename = QFileDialog::getOpenFileName(this, "Open File", dirLast, "IN20 Files (*.in20 *.IN20)");
	if(filename=="" || !QFile::exists(filename))
		return;

	if(OpenFile(filename))
		m_pSettings->setValue("mainwnd/sessiondir", QFileInfo(filename).path());
}


/**
 * File -> Save
 */
void MainWnd::SaveFile()
{
	if(m_curFile == "")
		SaveFileAs();
	else
		SaveFile(m_curFile);
}


/**
 * File -> Save As
 */
void MainWnd::SaveFileAs()
{
	QString dirLast = m_pSettings->value("mainwnd/sessiondir", "").toString();

	QString filename = QFileDialog::getSaveFileName(this, "Save File", dirLast, "IN20 Files (*.in20 *.IN20)");
	if(filename=="")
		return;

	if(SaveFile(filename))
		m_pSettings->setValue("mainwnd/sessiondir", QFileInfo(filename).path());
}


/**
 * load File
 */
bool MainWnd::OpenFile(const QString &file)
{
	static const std::string basename = "in20/";

	if(file=="" || !QFile::exists(file))
		return false;


	// load xml
	tl2::Prop<std::string> prop;
	prop.SetSeparator('/');
	if(!prop.Load(file.toStdString(), tl2::PropType::XML))
	{
		QMessageBox::critical(this, "Error", "Could not open session.");	
		return false;
	}

	// check format and version
	auto optFmt = prop.QueryOpt<std::string>(basename + "format");
	auto optVer = prop.QueryOpt<std::string>(basename + "version");
	auto optTime = prop.QueryOpt<t_real>(basename + "timestamp");
	if(!optFmt || *optFmt != "session")
	{
		QMessageBox::critical(this, "Error", "Not a session file. Ignoring.");
		return false;
	}
	if(optVer && optTime)
		print_out("Loading session file version ", *optVer, ", dated ", tl2::epoch_to_str(*optTime), ".");


	m_pWS->GetWidget()->GetWorkspace()->clear();
	m_pWS->GetWidget()->LoadWorkspace(basename, prop);


	SetCurrentFile(file);
	AddRecentFile(file);
	return true;
}


/**
 * save File
 */
bool MainWnd::SaveFile(const QString &file)
{
	static const std::string basename = "in20/";

	if(file=="")
		return false;

	std::unordered_map<std::string, std::string> sessionmap;

	// set format and version
	sessionmap[basename + "format"] = "session";
	sessionmap[basename + "version"] = PROGRAM_VERSION;
	sessionmap[basename + "timestamp"] = tl2::var_to_str(tl2::epoch<t_real>());


	// save workspace variables
	m_pWS->GetWidget()->SaveWorkspace(basename, sessionmap);


	tl2::Prop<std::string> prop;
	prop.SetSeparator('/');
	prop.Add(sessionmap);

	if(!prop.Save(file.toStdString(), tl2::PropType::XML))
	{
		QMessageBox::critical(this, "Error", "Could not save session.");	
		return false;
	}

	SetCurrentFile(file);
	AddRecentFile(file);
	return true;
}


/**
 * add a file to the recent files menu
 */
void MainWnd::AddRecentFile(const QString &file)
{
	for(const auto &recentfile : m_recentFiles)
	{
		// file already in list?
		if(recentfile == file)
			return;
	}

	m_recentFiles.push_back(file);
	RebuildRecentFiles();
}


/**
 * set the recent file menu
 */
void MainWnd::SetRecentFiles(const QStringList &files)
{
	m_recentFiles = files;
	RebuildRecentFiles();
}


/**
 * create the "recent files" sub-menu
 */
void MainWnd::RebuildRecentFiles()
{
	m_menuOpenRecent->clear();

	std::size_t num_recent_files = 0;
	for(auto iter = m_recentFiles.rbegin(); iter != m_recentFiles.rend(); ++iter)
	{
		QString filename = *iter;
		auto *acFile = new QAction(QIcon::fromTheme("document"), filename, m_pMenu);

		connect(acFile, &QAction::triggered, [this, filename]() { this->OpenFile(filename); });
		m_menuOpenRecent->addAction(acFile);

		if(++num_recent_files >= MAX_RECENT_FILES)
			break;
	}
}


/**
 * remember current file and set window title
 */
void MainWnd::SetCurrentFile(const QString &file)
{
	static const QString title("IN20 Tool");
	m_curFile = file;

	if(m_curFile == "")
		this->setWindowTitle(title);
	else	
		this->setWindowTitle(title + " -- " + m_curFile);
}
