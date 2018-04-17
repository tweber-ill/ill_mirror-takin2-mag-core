/**
 * data file browser
 * @author Tobias Weber <tweber@ill.fr>
 * @date 9-Apr-2018
 * @license see 'LICENSE' file
 */

#include "filebrowser.h"

#include <QtCore/QDirIterator>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>
#include <algorithm>

#include "tlibs/file/loadinstr.h"


using t_real = double;


// ----------------------------------------------------------------------------

FileBrowserWidget::FileBrowserWidget(QWidget *pParent, QSettings *pSettings)
	: QWidget(pParent), m_pSettings(pSettings)
{
	m_pListFiles->setAlternatingRowColors(true);

	// ------------------------------------------------------------------------
	// layout
	auto *pBtnFolders = new QPushButton("Folder...", this);

	auto *pGrid = new QGridLayout(this);
	pGrid->addWidget(pBtnFolders, 0, 0, 1, 1);
	pGrid->addWidget(m_pEditFolder, 0, 1, 1, 1);
	pGrid->addWidget(m_pListFiles, 1, 0, 2, 2);
	pGrid->addWidget(m_pPlotter, 3, 0, 1, 2);
	// ------------------------------------------------------------------------


	// ------------------------------------------------------------------------
	// connections
	connect(pBtnFolders, &QPushButton::clicked, this, &FileBrowserWidget::SelectFolder);
	connect(m_pEditFolder, &QLineEdit::textChanged, this, &FileBrowserWidget::SetFolder);
	connect(m_pListFiles, &QListWidget::currentItemChanged, this, &FileBrowserWidget::SetFile);
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
	if(dir != "")
		m_pEditFolder->setText(dir);
}


/**
 * set a given folder as working dir and populate file list
 */
void FileBrowserWidget::SetFolder(const QString& dir)
{
	m_pListFiles->clear();

	if(dir == "" || !QDir(dir).exists())
		return;

	// create file list
	std::vector<QString> vecFiles;
	for(QDirIterator iter(dir); ; iter.next())
	{
		if(iter.fileInfo().isFile())
			vecFiles.push_back(iter.fileName());

		if(!iter.hasNext())
			break;
	}


	// sort file list
	std::sort(vecFiles.begin(), vecFiles.end(),
	[](const auto& str1, const auto& str2) -> bool
	{
		return std::lexicographical_compare(str1.begin(), str1.end(), str2.begin(), str2.end());
	});


	// add files to list widget
	for(const auto& strFile : vecFiles)
	{
		QString fileFull = dir + "/" + strFile;

		// load file to get a description
		std::unique_ptr<tl::FileInstrBase<t_real>> pInstr(tl::FileInstrBase<t_real>::LoadInstr(fileFull.toStdString().c_str()));
		if(pInstr && pInstr->GetColNames().size())	// only valid files with a non-zero column count
		{
			QString strDescr;
			auto vecScanVars = pInstr->GetScannedVars();
			if(vecScanVars.size())
			{
				strDescr += ": ";
				for(std::size_t iScVar=0; iScVar<vecScanVars.size(); ++iScVar)
				{
					strDescr += vecScanVars[iScVar].c_str();
					if(iScVar < vecScanVars.size()-1)
						strDescr += ", ";
				}
				strDescr += " scan";
			}

			auto *pItem = new QListWidgetItem(m_pListFiles);
			pItem->setText(strFile + strDescr);
			pItem->setData(Qt::UserRole, fileFull);
		}
	}

	if(m_pSettings)
		m_pSettings->setValue("filebrowser/dir", dir);
}


/**
 * a file in the list was selected
 */
void FileBrowserWidget::SetFile(QListWidgetItem* pCur)
{
	m_pPlotter->clearGraphs();
	if(!pCur) return;

	// load scan file for preview
	QString file = pCur->data(Qt::UserRole).toString();
	std::unique_ptr<tl::FileInstrBase<t_real>> pInstr(tl::FileInstrBase<t_real>::LoadInstr(file.toStdString().c_str()));
	const auto &colnames = pInstr->GetColNames();
	const auto &data = pInstr->GetData();

	if(pInstr && colnames.size())	// only valid files with a non-zero column count
	{
		std::size_t x_idx = 0, y_idx = 1;

		// get x column index
		if(auto vecScanVars = pInstr->GetScannedVars(); vecScanVars.size() >= 1)
		{
			// try to find scan var, if not found, use first column
			pInstr->GetCol(vecScanVars[0], &x_idx);
			if(x_idx >= colnames.size())
				x_idx = 0;
		}

		// get y column index
		{
			// try to find count var, if not found, use second column
			pInstr->GetCol(pInstr->GetCountVar(), &y_idx);
			if(y_idx >= colnames.size())
				y_idx = 1;
		}

		// labels
		m_pPlotter->xAxis->setLabel(x_idx<colnames.size() ? colnames[x_idx].c_str() : "x");
		m_pPlotter->yAxis->setLabel(y_idx<colnames.size() ? colnames[y_idx].c_str() : "y");

		// plot the data
		if(x_idx < data.size() && y_idx < data.size())
		{
			QVector<t_real> x_data, y_data;
			std::copy(data[x_idx].begin(), data[x_idx].end(), std::back_inserter(x_data));
			std::copy(data[y_idx].begin(), data[y_idx].end(), std::back_inserter(y_data));

			auto *graph = m_pPlotter->addGraph();
			graph->setData(x_data, y_data);
		}

		m_pPlotter->replot();
	}
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
