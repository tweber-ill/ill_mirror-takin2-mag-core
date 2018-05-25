/**
 * in20 data analysis tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date 6-Apr-2018
 * @license see 'LICENSE' file
 */

#include "mainwnd.h"
//#include <iostream>


MainWnd::MainWnd(QSettings* pSettings)
	: QMainWindow(), m_pSettings(pSettings), 
	m_pBrowser(new FileBrowser(this, pSettings)),
	m_pWS(new WorkSpace(this, pSettings))
{
	this->setObjectName("in20");
	this->setWindowTitle("IN20 Tool");
	this->resize(800, 600);


	// ------------------------------------------------------------------------
	// Menu Bar
	QMenu *pMenuView = new QMenu("View", m_pMenu);

	QAction *pShowFileBrowser = new QAction("Show File Browser", pMenuView);
	pShowFileBrowser->setChecked(m_pBrowser->isVisible());
	connect(pShowFileBrowser, &QAction::triggered, m_pBrowser, &FileBrowser::show);
	pMenuView->addAction(pShowFileBrowser);

	QAction *pShowWorkSpace = new QAction("Show Workspace", pMenuView);
	pShowWorkSpace->setChecked(m_pWS->isVisible());
	connect(pShowWorkSpace, &QAction::triggered, m_pWS, &FileBrowser::show);
	pMenuView->addAction(pShowWorkSpace);

	m_pMenu->addMenu(pMenuView);
	this->setMenuBar(m_pMenu);
	// ------------------------------------------------------------------------


	this->setStatusBar(m_pStatus);
	this->setCentralWidget(m_pMDI);
	this->addDockWidget(Qt::LeftDockWidgetArea, m_pBrowser);
	this->addDockWidget(Qt::RightDockWidgetArea, m_pWS);


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
