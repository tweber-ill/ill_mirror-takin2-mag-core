/**
 * scan browser
 * @author Tobias Weber <tweber@ill.fr>
 * @date 6-Apr-2018
 * @license see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2023  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
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

#define MAX_RECENT_FILES 16

#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>

#include "mainwnd.h"
#include "globals.h"
#include "funcs.h"
#include "tlibs2/libs/file.h"
#include "tlibs2/libs/algos.h"
#include "tlibs2/libs/str.h"
#include "tlibs2/libs/qt/helper.h"

#include <iostream>


MainWnd::MainWnd(QSettings* pSettings)
	: QMainWindow(), m_pSettings(pSettings),
	m_pBrowser(new FileBrowser(this, pSettings)),
	m_pWS(new WorkSpace(this, pSettings)),
	m_pCLI(new CommandLine(this, pSettings)),
	m_pCurPlot(new Plotter(this, pSettings))
{
	// the command line widget has to be accessible globally for error output
	g_pCLI = m_pCLI;

	this->setObjectName("scanbrowser");
	SetCurrentFile("");
	LoadPlugins();


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
	//auto *acPrefs = new QAction("Preferences...", m_pMenu);
	//acPrefs->setMenuRole(QAction::PreferencesRole);
	//menuFile->addAction(acPrefs);   TODO
	//menuFile->addSeparator();
	auto *acQuit = new QAction(QIcon::fromTheme("application-exit"), "Quit", m_pMenu);
	acQuit->setMenuRole(QAction::QuitRole);
	menuFile->addAction(acQuit);

	// view
	menuView->addAction(m_pBrowser->toggleViewAction());
	menuView->addAction(m_pWS->toggleViewAction());
	menuView->addAction(m_pCLI->toggleViewAction());
	//menuView->addAction(m_pCurPlot->toggleViewAction());

	// help
	auto *acAboutQt = new QAction(QIcon::fromTheme("help-about"), "About Qt...", m_pMenu);
	auto *acAbout = new QAction(QIcon::fromTheme("help-about"), "About...", m_pMenu);
	acAboutQt->setMenuRole(QAction::AboutQtRole);
	acAbout->setMenuRole(QAction::AboutRole);
	menuHelp->addAction(acAboutQt);
	menuHelp->addAction(acAbout);

	m_pMenu->addMenu(menuFile);
	m_pMenu->addMenu(menuView);
	if(m_plugin_dlgs.size())
		m_pMenu->addMenu(m_pmenuPluginTools);
	m_pMenu->addMenu(menuHelp);
	m_pMenu->setNativeMenuBar(m_pSettings ? m_pSettings->value("mainwnd/native_gui", false).toBool() : false);
	this->setMenuBar(m_pMenu);
	// ------------------------------------------------------------------------



	// ------------------------------------------------------------------------
	// docks
	//this->setStatusBar(m_pStatus);
	this->setCentralWidget(m_pMDI);
	this->addDockWidget(Qt::LeftDockWidgetArea, m_pBrowser);
	this->addDockWidget(Qt::RightDockWidgetArea, m_pWS);
	this->addDockWidget(Qt::BottomDockWidgetArea, m_pCLI);
	//this->addDockWidget(Qt::BottomDockWidgetArea, m_pCurPlot);
	this->setCentralWidget(m_pCurPlot);
	// ------------------------------------------------------------------------



	// ------------------------------------------------------------------------
	// info dialog
	QDialog *dlgInfo = nullptr;
	{
		auto infopanel = new QWidget(this);
		auto grid = new QGridLayout(infopanel);
		grid->setSpacing(4);
		grid->setContentsMargins(6, 6, 6, 6);

		auto sep1 = new QFrame(infopanel); sep1->setFrameStyle(QFrame::HLine);
		auto sep2 = new QFrame(infopanel); sep2->setFrameStyle(QFrame::HLine);
		auto sep3 = new QFrame(infopanel); sep3->setFrameStyle(QFrame::HLine);

		std::string strBoost = BOOST_LIB_VERSION;
		algo::replace_all(strBoost, "_", ".");

		auto labelTitle = new QLabel("Takin / Scan Browser", infopanel);
		auto fontTitle = labelTitle->font();
		fontTitle.setBold(true);
		labelTitle->setFont(fontTitle);
		labelTitle->setAlignment(Qt::AlignHCenter);

		auto labelAuthor = new QLabel("Written by Tobias Weber <tweber@ill.fr>.", infopanel);
		labelAuthor->setAlignment(Qt::AlignHCenter);

		auto labelDate = new QLabel("2018.", infopanel);
		labelDate->setAlignment(Qt::AlignHCenter);

		int y = 0;
		grid->addWidget(labelTitle, y++,0, 1,1);
		grid->addWidget(labelAuthor, y++,0, 1,1);
		grid->addWidget(labelDate, y++,0, 1,1);

		grid->addItem(new QSpacerItem(16,16,
			QSizePolicy::Minimum, QSizePolicy::Fixed),
			y++,0, 1,1);
		grid->addWidget(sep1, y++,0, 1,1);

		grid->addWidget(new QLabel(
			QString("Compiler: ") +
			QString(BOOST_COMPILER) + ".",
			infopanel), y++,0, 1,1);
		grid->addWidget(new QLabel(
			QString("C++ Library: ") +
			QString(BOOST_STDLIB) + ".",
			infopanel), y++,0, 1,1);
		grid->addWidget(new QLabel(
			QString("Build Date: ") +
			QString(__DATE__) + ", " +
			QString(__TIME__) + ".",
			infopanel), y++,0, 1,1);

		grid->addWidget(sep2, y++,0, 1,1);

		auto labelQt = new QLabel(QString(
			"<a href=\"http://code.qt.io/cgit/\">Qt</a>"
			" Version: %1.").arg(QT_VERSION_STR),
			infopanel);
		labelQt->setOpenExternalLinks(true);
		grid->addWidget(labelQt, y++,0, 1,1);

		auto labelBoost = new QLabel(QString(
			"<a href=\"http://www.boost.org\">Boost</a>"
			" Version: %1.").arg(strBoost.c_str()),
			infopanel);
		labelBoost->setOpenExternalLinks(true);
		grid->addWidget(labelBoost, y++,0, 1,1);

		grid->addWidget(sep3, y++,0, 1,1);

		grid->addItem(new QSpacerItem(16,16,
		QSizePolicy::Minimum, QSizePolicy::Expanding),
			y++,0, 1,1);

		dlgInfo = new QDialog(this);
		dlgInfo->setWindowTitle("About");
		dlgInfo->setSizeGripEnabled(true);
		dlgInfo->setFont(this->font());

		QPushButton *infoDlgOk = new QPushButton("OK", dlgInfo);
		connect(infoDlgOk, &QAbstractButton::clicked,
			dlgInfo, &QDialog::accept);

		auto dlgGrid = new QGridLayout(dlgInfo);
		dlgGrid->setSpacing(8);
		dlgGrid->setContentsMargins(8, 8, 8, 8);
		dlgGrid->addWidget(infopanel, 0,0, 1,4);
		dlgGrid->addWidget(infoDlgOk, 1,3, 1,1);
	}
	// ------------------------------------------------------------------------



	// ------------------------------------------------------------------------
	// connections
	connect(m_pBrowser->GetWidget(), &FileBrowserWidget::TransferFiles, m_pWS->GetWidget(), &WorkSpaceWidget::ReceiveFiles);
	connect(m_pWS->GetWidget(), &WorkSpaceWidget::PlotDataset, m_pCurPlot, &Plotter::Plot);
	connect(m_pBrowser->GetWidget(), &FileBrowserWidget::PlotDataset, m_pCurPlot, &Plotter::Plot);
	connect(acNew, &QAction::triggered, this, &MainWnd::NewFile);
	connect(acOpen, &QAction::triggered, this, static_cast<void(MainWnd::*)()>(&MainWnd::OpenFile));
	connect(acSave, &QAction::triggered, this, static_cast<void(MainWnd::*)()>(&MainWnd::SaveFile));
	connect(acSaveAs, &QAction::triggered, this, &MainWnd::SaveFileAs);
	//connect(acPrefs, &QAction::triggered, this, ....);	TODO
	connect(acAbout, &QAction::triggered, this, &MainWnd::ShowAbout);

	connect(acAboutQt, &QAction::triggered, this, []() { qApp->aboutQt(); });
	connect(acQuit, &QAction::triggered, this, &QMainWindow::close);

	// link symbol maps of workspace widget and command line parser
	m_pCLI->GetWidget()->GetParserContext().SetWorkspace(m_pWS->GetWidget()->GetWorkspace());

	// connect the "workspace updated" signal from the command line parser
	m_pCLI->GetWidget()->GetParserContext().GetWorkspaceUpdatedSignal().connect([this](const std::string& /*ident*/)
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
		else
			this->resize(800, 600);
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
{
	UnloadPlugins();
}


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

	QString filename = QFileDialog::getOpenFileName(this, "Open File", dirLast, "Scan Browser Files (*.sbr *.SBR)");
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

	QString filename = QFileDialog::getSaveFileName(this, "Save File", dirLast, "Scan Browser Files (*.sbr *.SBR)");
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
	static const std::string basename = "scanbrowser/";

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
	static const std::string basename = "scanbrowser/";

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
	static const QString title("Scan Browser");
	m_curFile = file;

	if(m_curFile == "")
		this->setWindowTitle(title);
	else
		this->setWindowTitle(title + " -- " + m_curFile);
}



void MainWnd::ShowAbout()
{
	if(!m_pAbout)
		m_pAbout = create_about_dialog(this);

	m_pAbout->show();
	m_pAbout->raise();
	m_pAbout->activateWindow();
}



/**
 * looks for and loads plugin tools
 */
void MainWnd::LoadPlugins()
{
	m_pmenuPluginTools = new QMenu("Tools", m_pMenu);

	std::vector<QString> plugindirs{{ "plugins", "../plugins",
		qApp->applicationDirPath()+"/plugins", qApp->applicationDirPath()+"/../plugins" }};
	for(const auto& plugindir : plugindirs)
	{
		print_out("Looking for plugins in \"", plugindir.toStdString(), "\"...");

		QDirIterator iter(plugindir, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
		while(iter.hasNext())
		{
			std::string dllfile = iter.next().toStdString();
			std::string fileext = tl2::get_fileext(dllfile, true);
			std::string rawfile = tl2::get_file_nodir(dllfile, false);
			if(rawfile == "." || rawfile == "..")
				continue;

			if(fileext == "so" || fileext == "dll" || fileext == "dylib")
			{
				try
				{
					auto dll = std::make_shared<boost::dll::shared_library>(dllfile);
					if(!dll || !dll->is_loaded())
					{
						print_err("Could not load plugin \"", dllfile, "\".");
						continue;
					}

					if(!dll->has("tl_descr") || !dll->has("tl_init") || !dll->has("tl_create") || !dll->has("tl_destroy"))
					{
						print_err("Not a valid plugin: \"", dllfile, "\".");
						continue;
					}


					PluginDlg plugin;
					plugin.dll = dll;

					// plugin functions
					plugin.f_descr = dll->get<PluginDlg::t_descr>("tl_descr");
					plugin.f_init = dll->get<PluginDlg::t_init>("tl_init");
					plugin.f_create = dll->get<PluginDlg::t_create>("tl_create");
					plugin.f_destroy = dll->get<PluginDlg::t_destroy>("tl_destroy");

					// plugin descriptors
					{
						std::vector<std::string> vecdescr;
						tl2::get_tokens<std::string, std::string>(plugin.f_descr(), ";", vecdescr);

						plugin.name = vecdescr[1];
						plugin.descr = vecdescr[2];

						// only accept dialog plugins
						if(vecdescr[0] != "dlg")
							continue;

						// skip plugin if another one with the same name is already registered
						if(std::find_if(m_plugin_dlgs.begin(), m_plugin_dlgs.end(),
							[&plugin](const PluginDlg& otherplugin) -> bool
							{
								return otherplugin.name == plugin.name;
							}) != m_plugin_dlgs.end())
							continue;

						// add menu item
						auto *acTool = new QAction((plugin.name + "...").c_str(), m_pMenu);
						acTool->setToolTip(plugin.descr.c_str());
						m_pmenuPluginTools->addAction(acTool);
						const std::size_t pluginNr = m_plugin_dlgs.size();
						connect(acTool, &QAction::triggered, this, [this, pluginNr]()
						{
							if(pluginNr >= m_plugin_dlgs.size())
							{
								print_err("Invalid tool plugin number ", pluginNr, ".");
								return;
							}

							// get plugin corresponding to this menu item
							auto& plugin = m_plugin_dlgs[pluginNr];
							if(!plugin.inited)
								plugin.inited = plugin.f_init();

							if(plugin.inited && !plugin.dlg)
								plugin.dlg = plugin.f_create(this);

							if(plugin.dlg)
							{
								plugin.dlg->show();
								plugin.dlg->activateWindow();
								plugin.dlg->raise();
								plugin.dlg->setFocus();
							}
						});

						print_out("Tool plugin ", dll->location(), " loaded. ",
							"name: \"", plugin.name, "\", ",
							"description: \"", plugin.descr, "\".");

						m_plugin_dlgs.emplace_back(std::move(plugin));
					}
				}
				catch(const std::exception& ex)
				{
					print_err("Error loading plugin \"", dllfile, "\": ", ex.what(), ".");
				}
			} // dialog plugins in so libraries
			else if(fileext == "py")
			{
				std::ifstream ifstr(dllfile);
				if(!ifstr)
					continue;

				PluginScr plugin;
				plugin.filename = dllfile;
				bool bHasName = false;

				std::string line;
				while(std::getline(ifstr, line))
				{
					tl2::trim(line);
					// looking for plugin descriptors in header comments
					if(line.size()==0 || line[0]!='#')
						continue;

					if(line.find("__ident__") != std::string::npos)
					{
						std::tie(std::ignore, plugin.name) = tl2::split_first<std::string>(line, ":", true, false);
						bHasName = true;
					}
					else if(line.find("__descr__") != std::string::npos)
					{
						std::tie(std::ignore, plugin.descr) = tl2::split_first<std::string>(line, ":", true, false);
					}
				}

				// not a recognised script plugin
				if(!bHasName)
					continue;

				// skip plugin if another one with the same name is already registered
				if(std::find_if(m_plugin_scr.begin(), m_plugin_scr.end(),
					[&plugin](const PluginScr& otherplugin) -> bool
					{
						return otherplugin.name == plugin.name;
					}) != m_plugin_scr.end())
					continue;


				// add menu item
				auto *acTool = new QAction((plugin.name + "...").c_str(), m_pMenu);
				acTool->setToolTip(plugin.descr.c_str());
				m_pmenuPluginTools->addAction(acTool);
				const std::size_t pluginNr = m_plugin_scr.size();
				connect(acTool, &QAction::triggered, this, [this, pluginNr]()
				{
					if(pluginNr >= m_plugin_scr.size())
					{
						print_err("Invalid script plugin number ", pluginNr, ".");
						return;
					}

					// get plugin corresponding to this menu item
					auto& plugin = m_plugin_scr[pluginNr];

					std::string interp = "python";
					std::system((interp + std::string{" "} + plugin.filename).c_str());
				});


				print_out("Script plugin \"", plugin.filename, "\" loaded. ",
					"name: \"", plugin.name, "\", ",
					"description: \"", plugin.descr, "\".");

				m_plugin_scr.emplace_back(std::move(plugin));
			} // py script plugins
		}
	}
}


/**
 * unloads plugins
 */
void MainWnd::UnloadPlugins()
{
	for(auto &plugin : m_plugin_dlgs)
	{
		// remove dialogs
		if(plugin.dlg && plugin.f_destroy)
		{
			plugin.f_destroy(plugin.dlg);
			plugin.dlg = nullptr;
		}

		plugin.inited = false;
	}

	m_plugin_dlgs.clear();
}
