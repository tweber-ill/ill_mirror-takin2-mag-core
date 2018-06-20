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
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>

#include "tlibs/file/loadinstr.h"
using t_real = t_real_dat;


// ----------------------------------------------------------------------------

FileBrowserWidget::FileBrowserWidget(QWidget *pParent, QSettings *pSettings)
	: QWidget(pParent), m_pSettings(pSettings)
{
	m_pListFiles->setAlternatingRowColors(true);
	m_pListFiles->installEventFilter(this);


	// ------------------------------------------------------------------------
	// layout
	auto *pBtnFolders = new QPushButton("Folder...", this);
	auto *pBtnTransfer = new QPushButton("To Workspace", this);
	auto *pCheckMultiSelect = new QCheckBox("Multi-Select", this);

	auto *pGrid = new QGridLayout(this);
	pGrid->addWidget(pBtnFolders, 0, 0, 1, 1);
	pGrid->addWidget(m_pEditFolder, 0, 1, 1, 1);
	pGrid->addWidget(m_pListFiles, 1, 0, 2, 2);
	pGrid->addWidget(pCheckMultiSelect, 3, 0, 1, 1);
	pGrid->addWidget(pBtnTransfer, 3, 1, 1, 1);
	//pGrid->addWidget(m_pPlotter, 4, 0, 1, 2);
	// ------------------------------------------------------------------------


	// ------------------------------------------------------------------------
	// connections
	connect(pBtnFolders, &QPushButton::clicked, this, &FileBrowserWidget::SelectFolder);
	connect(pCheckMultiSelect, &QCheckBox::stateChanged, this, &FileBrowserWidget::SetMultiSelect);
	connect(pBtnTransfer, &QPushButton::clicked, this, &FileBrowserWidget::TransferSelectedToWorkspace);
	connect(m_pEditFolder, &QLineEdit::textChanged, this, &FileBrowserWidget::SetFolder);
	connect(m_pListFiles, &QListWidget::currentItemChanged, this, &FileBrowserWidget::SetFile);
	connect(m_pListFiles, &QListWidget::itemDoubleClicked, this, &FileBrowserWidget::FileDoubleClicked);
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
		QString fileFull = dir + QDir::separator() + strFile;

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
	if(!pCur) return;

	// load scan file for preview
	QString file = pCur->data(Qt::UserRole).toString();
	if(auto [ok, dataset] = Dataset::convert_instr_file(file.toStdString().c_str()); ok)
	{
		//m_pPlotter->Plot(dataset);
		emit PlotDataset(dataset);
	}
}


void FileBrowserWidget::SetMultiSelect(int checked)
{
	bool bMultiSel = checked != 0;
	m_pListFiles->setSelectionMode(bMultiSel ? QAbstractItemView::MultiSelection : QAbstractItemView::SingleSelection);
}


/**
 * a file in the list was double-clicked
 */
void FileBrowserWidget::FileDoubleClicked(QListWidgetItem *pItem)
{
	QList lst({ pItem });
	TransferToWorkspace(lst);
}

void FileBrowserWidget::TransferSelectedToWorkspace()
{
	TransferToWorkspace(m_pListFiles->selectedItems());
}

/**
 * emit the file transfer
 */
void FileBrowserWidget::TransferToWorkspace(const QList<QListWidgetItem*> &lst)
{
	std::vector<std::string> files;

	// extract file names
	for(const auto* item : lst)
	{
		if(!item) continue;
		std::string file = item->data(Qt::UserRole).toString().toStdString();

		files.emplace_back(std::move(file));
	}

	emit TransferFiles(files);
}


/**
 * re-directed child events
 */
bool FileBrowserWidget::eventFilter(QObject *pObj, QEvent *pEvt)
{
	if(pObj == m_pListFiles)
	{
		//std::cout << int(pEvt->type()) << std::endl;

		// as the file browser and the work space widget share the same plotter, re-send plot on activation
		if(pEvt->type() == QEvent::FocusIn)
			SetFile(m_pListFiles->currentItem());
	}

	return QObject::eventFilter(pObj, pEvt);
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
