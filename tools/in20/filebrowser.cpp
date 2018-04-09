/**
 * data file browser
 * @author Tobias Weber <tweber@ill.fr>
 * @date 9-Apr-2018
 * @license see 'LICENSE' file
 */

#include "filebrowser.h"
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>


// ----------------------------------------------------------------------------

FileBrowserWidget::FileBrowserWidget(QWidget *pParent, QSettings *pSettings)
	: QWidget(pParent), m_pSettings(pSettings)
{
	// ------------------------------------------------------------------------
	// layout
	auto *pBtnFolders = new QPushButton("Folder...", this);

	auto *pGrid = new QGridLayout(this);
	pGrid->addWidget(pBtnFolders, 0, 0, 1, 1);
	pGrid->addWidget(m_pEditFolder, 0, 1, 1, 1);
	pGrid->addWidget(m_pListFiles, 1, 0, 2, 2);
	// ------------------------------------------------------------------------


	// ------------------------------------------------------------------------
	// connections
	connect(pBtnFolders, &QPushButton::clicked, this, &FileBrowserWidget::SelectFolder);
	connect(m_pEditFolder, &QLineEdit::textChanged, this, &FileBrowserWidget::SetFolder);
	// ------------------------------------------------------------------------


	// ------------------------------------------------------------------------
	// settings
	if(m_pSettings)
	{
		m_pEditFolder->setText(m_pSettings->value("filebrowser/dir", "").toString());
	}
	// ------------------------------------------------------------------------
}


FileBrowserWidget::~FileBrowserWidget()
{
}


/**
 * open a folder selection dialog
 */
void FileBrowserWidget::SelectFolder()
{
	QString dir;
	if(m_pSettings)
		dir = m_pSettings->value("filebrowser/dir", "").toString();
	dir = QFileDialog::getExistingDirectory(this, "Select Folder", dir);

	m_pEditFolder->setText(dir);
}


/**
 * set a given folder as working dir and populate file list
 */
void FileBrowserWidget::SetFolder(const QString& dir)
{
	if(dir == "" || !QDir(dir).exists())
		return;

	// TODO

	if(m_pSettings)
		m_pSettings->setValue("filebrowser/dir", dir);
}
// ----------------------------------------------------------------------------





// ----------------------------------------------------------------------------

FileBrowser::FileBrowser(QWidget* pParent, QSettings *pSettings)
	: QDockWidget(pParent), m_pBrowser(std::make_unique<FileBrowserWidget>(this, pSettings))
{
	this->setObjectName("fileBrowser");
	this->setWindowTitle("File Browser");
	this->setWidget(m_pBrowser.get());
}

FileBrowser::~FileBrowser()
{
}

// ----------------------------------------------------------------------------
