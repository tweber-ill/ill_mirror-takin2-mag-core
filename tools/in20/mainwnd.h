/**
 * in20 data analysis tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date 6-Apr-2018
 * @license see 'LICENSE' file
 */

#ifndef __IN20MAINWND_H__
#define __IN20MAINWND_H__

#include <QtCore/QSettings>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMdiArea>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStatusBar>

#include "filebrowser.h"
#include "workspace.h"
#include "command.h"
#include "plot.h"

#include <boost/dll/shared_library.hpp>
#include <memory>



/**
 * dialog plugins
 */
struct PluginDlg
{
	std::shared_ptr<boost::dll::shared_library> dll;
	std::string ty, name, descr;
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

	FileBrowser *m_pBrowser = nullptr;
	WorkSpace *m_pWS = nullptr;
	CommandLine *m_pCLI = nullptr;
	PlotterDock *m_pCurPlot = nullptr;

	QMenu *m_menuOpenRecent = nullptr;
	QStringList m_recentFiles;
	QString m_curFile;

	std::vector<PluginDlg> m_plugin_dlgs;

protected:
	virtual void showEvent(QShowEvent *pEvt) override;
	virtual void closeEvent(QCloseEvent *pEvt) override;

	void SetCurrentFile(const QString &file);
	void SetRecentFiles(const QStringList &files);
	void AddRecentFile(const QString &file);
	void RebuildRecentFiles();

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
