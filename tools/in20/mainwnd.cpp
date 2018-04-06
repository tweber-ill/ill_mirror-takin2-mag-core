/**
 * in20 data analysis tool
 * @author Tobias Weber
 * @date 6-Apr-2018
 * @license see 'LICENSE' file
 */

#include "mainwnd.h"


MainWnd::MainWnd(QSettings* pSettings)
	: QMainWindow(), m_pSettings(pSettings), 
		m_pMenu(new QMenuBar(this)), m_pStatus(new QStatusBar(this))
{
	this->setWindowTitle("IN20 Tool");
	this->resize(800, 600);

	this->setMenuBar(m_pMenu);
	this->setStatusBar(m_pStatus);

	if(m_pSettings)
	{
		// restore window state
		if(m_pSettings->contains("mainwnd/geo"))
			this->restoreGeometry(m_pSettings->value("mainwnd/geo").toByteArray());
		if(m_pSettings->contains("mainwnd/state"))
			this->restoreState(m_pSettings->value("mainwnd/state").toByteArray());
	}
}


MainWnd::~MainWnd()
{
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
