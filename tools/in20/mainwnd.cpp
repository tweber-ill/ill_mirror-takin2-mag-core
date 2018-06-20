/**
 * in20 data analysis tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date 6-Apr-2018
 * @license see 'LICENSE' file
 */

#include "mainwnd.h"
#include "globals.h"


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
	QMenu *pMenuView = new QMenu("View", m_pMenu);

	pMenuView->addAction(m_pBrowser->toggleViewAction());
	pMenuView->addAction(m_pWS->toggleViewAction());
	pMenuView->addAction(m_pCLI->toggleViewAction());
	pMenuView->addAction(m_pCurPlot->toggleViewAction());

	m_pMenu->addMenu(pMenuView);
	this->setMenuBar(m_pMenu);
	// ------------------------------------------------------------------------


	//this->setStatusBar(m_pStatus);
	this->setCentralWidget(m_pMDI);
	this->addDockWidget(Qt::LeftDockWidgetArea, m_pBrowser);
	this->addDockWidget(Qt::RightDockWidgetArea, m_pWS);
	this->addDockWidget(Qt::BottomDockWidgetArea, m_pCLI);
	this->addDockWidget(Qt::BottomDockWidgetArea, m_pCurPlot);


	// ------------------------------------------------------------------------
	// connections
	connect(m_pBrowser->GetWidget(), &FileBrowserWidget::TransferFiles,
		m_pWS->GetWidget(), &WorkSpaceWidget::ReceiveFiles);
	connect(m_pWS->GetWidget(), &WorkSpaceWidget::PlotDataset, m_pCurPlot->GetWidget(), &Plotter::Plot);
	connect(m_pBrowser->GetWidget(), &FileBrowserWidget::PlotDataset, m_pCurPlot->GetWidget(), &Plotter::Plot);

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
