/**
 * in20 data analysis tool
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

#ifndef __SCANBROWSER_MAINWND_H__
#define __SCANBROWSER_MAINWND_H__

#include <QtCore/QSettings>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QDialog>
#include <QtWidgets/QMdiArea>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStatusBar>

#include "filebrowser.h"
#include "workspace.h"
#include "command.h"
#include "plot.h"
#include "about.h"

#include <boost/dll/shared_library.hpp>
#include <memory>



/**
 * dialog plugins
 */
struct PluginDlg
{
	std::shared_ptr<boost::dll::shared_library> dll;
	std::string name, descr;
	bool inited = false;

	using t_descr = const char*(*)();
	using t_init = bool(*)();
	using t_create = QDialog*(*)(QWidget*);
	using t_destroy = void(*)(QDialog*);

	t_descr f_descr = nullptr;
	t_init f_init = nullptr;
	t_create f_create = nullptr;
	t_destroy f_destroy = nullptr;

	QDialog *dlg = nullptr;
};



/**
 * script plugins
 */
struct PluginScr
{
	std::string name, descr;
	std::string filename;
};



/**
 * main dialog
 */
class MainWnd : public QMainWindow
{
private:
	QSettings *m_pSettings = nullptr;

	QMenuBar *m_pMenu = new QMenuBar(this);
	QMenu *m_pmenuPluginTools = nullptr;
	//QStatusBar *m_pStatus = new QStatusBar(this);
	QMdiArea *m_pMDI = new QMdiArea(this);
	QDialog *m_pAbout = nullptr;

	FileBrowser *m_pBrowser = nullptr;
	WorkSpace *m_pWS = nullptr;
	CommandLine *m_pCLI = nullptr;
	Plotter *m_pCurPlot = nullptr;

	QMenu *m_menuOpenRecent = nullptr;
	QStringList m_recentFiles;
	QString m_curFile;

	std::vector<PluginDlg> m_plugin_dlgs;	// tool/dialog plugins
	std::vector<PluginScr> m_plugin_scr;	// callable script plugins

protected:
	virtual void showEvent(QShowEvent *pEvt) override;
	virtual void closeEvent(QCloseEvent *pEvt) override;

	void SetCurrentFile(const QString &file);
	void SetRecentFiles(const QStringList &files);
	void AddRecentFile(const QString &file);
	void RebuildRecentFiles();

	void ShowAbout();

	void LoadPlugins();
	void UnloadPlugins();

public:
	MainWnd(QSettings* pSettings = nullptr);
	virtual ~MainWnd();

	// session file menu operations
	void NewFile();
	void OpenFile();
	void SaveFile();
	void SaveFileAs();

	// actual session file operations
	bool OpenFile(const QString &file);
	bool SaveFile(const QString &file);
};

#endif
