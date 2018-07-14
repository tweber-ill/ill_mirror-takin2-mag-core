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


class MainWnd : public QMainWindow
{
private:
	QSettings *m_pSettings = nullptr;

	QMenuBar *m_pMenu = new QMenuBar(this);
	//QStatusBar *m_pStatus = new QStatusBar(this);
	QMdiArea *m_pMDI = new QMdiArea(this);

	FileBrowser *m_pBrowser = nullptr;
	WorkSpace *m_pWS = nullptr;
	CommandLine *m_pCLI = nullptr;
	PlotterDock *m_pCurPlot = nullptr;

	QMenu *m_menuOpenRecent = nullptr;
	QStringList m_recentFiles;
	QString m_curFile;

protected:
	virtual void showEvent(QShowEvent *pEvt) override;
	virtual void closeEvent(QCloseEvent *pEvt) override;

	void SetCurrentFile(const QString &file);
	void SetRecentFiles(const QStringList &files);
	void AddRecentFile(const QString &file);
	void RebuildRecentFiles();

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
