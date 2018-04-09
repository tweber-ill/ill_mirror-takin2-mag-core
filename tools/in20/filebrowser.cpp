/**
 * data file browser
 * @author Tobias Weber <tweber@ill.fr>
 * @date 9-Apr-2018
 * @license see 'LICENSE' file
 */

#include "filebrowser.h"



FileBrowserWidget::FileBrowserWidget(QWidget *pParent)
	: QWidget(pParent)
{

}

FileBrowserWidget::~FileBrowserWidget()
{
}



FileBrowser::FileBrowser(QWidget* pParent)
	: QDockWidget(pParent), m_pBrowser(std::make_unique<FileBrowserWidget>(this))
{
	this->setObjectName("fileBrowser");
	this->setWindowTitle("File Browser");
}

FileBrowser::~FileBrowser()
{
}
