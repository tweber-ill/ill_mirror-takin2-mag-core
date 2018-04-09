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
#include <QtWidgets/QStatusBar>

#include "filebrowser.h"


class MainWnd : public QMainWindow
{
private:
	QSettings *m_pSettings = nullptr;
	QMenuBar *m_pMenu = new QMenuBar(this);
	QStatusBar *m_pStatus = new QStatusBar(this);
	QMdiArea *m_pMDI = new QMdiArea(this);
	FileBrowser *m_pBrowser = nullptr;

private:
	virtual void showEvent(QShowEvent *pEvt) override;
	virtual void closeEvent(QCloseEvent *pEvt) override;

public:
	MainWnd(QSettings* pSettings = nullptr);
	virtual ~MainWnd();
};

#endif
