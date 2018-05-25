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
	auto *pBtnTransfer = new QPushButton("To Workspace", this);
	auto *pCheckMultiSelect = new QCheckBox("Multi-Select", this);

	auto *pGrid = new QGridLayout(this);
	pGrid->addWidget(pBtnFolders, 0, 0, 1, 1);
	pGrid->addWidget(m_pEditFolder, 0, 1, 1, 1);
	pGrid->addWidget(m_pListFiles, 1, 0, 2, 2);
	pGrid->addWidget(pCheckMultiSelect, 3, 0, 1, 1);
	pGrid->addWidget(pBtnTransfer, 3, 1, 1, 1);
	pGrid->addWidget(m_pPlotter, 4, 0, 1, 2);
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
 * copy algorithm with interleave
 */
template<class T1, class T2>
void copy_interleave(T1 inIter, T1 inEnd, T2 outIter, std::size_t interleave, std::size_t startskip)
{
	std::advance(inIter, startskip);

	while(std::distance(inIter, inEnd) > 0)
	{
		*outIter = *inIter;

		++outIter;
		std::advance(inIter, interleave);
	}
}


/**
 * a file in the list was selected
 */
void FileBrowserWidget::SetFile(QListWidgetItem* pCur)
{
	static const std::vector<unsigned> colors = {
		0xffff0000, 0xff0000ff, 0xff009900, 0xff000000,
	};

	m_pPlotter->clearGraphs();
	if(!pCur) return;

	// load scan file for preview
	QString file = pCur->data(Qt::UserRole).toString();
	std::unique_ptr<tl::FileInstrBase<t_real>> pInstr(tl::FileInstrBase<t_real>::LoadInstr(file.toStdString().c_str()));
	const auto &colnames = pInstr->GetColNames();
	const auto &data = pInstr->GetData();

	if(pInstr && colnames.size())	// only valid files with a non-zero column count
	{
		// process polarisation data
		pInstr->SetPolNames("p1", "p2", "i11", "i10");
		pInstr->ParsePolData();


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

		t_real xmin = std::numeric_limits<t_real>::max();
		t_real xmax = -xmin;
		t_real ymin = std::numeric_limits<t_real>::max();
		t_real ymax = -xmin;

		// plot the data
		if(x_idx < data.size() && y_idx < data.size())
		{
			std::size_t numpolstates = pInstr->NumPolChannels();
			if(numpolstates == 0) numpolstates = 1;

			// iterate all (polarisation) subplots
			for(std::size_t graphidx=0; graphidx<numpolstates; ++graphidx)
			{
				// get x, y, yerr data
				QVector<t_real> x_data, y_data, y_err;
				copy_interleave(data[x_idx].begin(), data[x_idx].end(), std::back_inserter(x_data), numpolstates, graphidx);
				copy_interleave(data[y_idx].begin(), data[y_idx].end(), std::back_inserter(y_data), numpolstates, graphidx);
				std::transform(data[y_idx].begin(), data[y_idx].end(), std::back_inserter(y_err),
				[](t_real y) -> t_real
				{
					if(tl::float_equal<t_real>(y, 0))
						return 1;
					return std::sqrt(y);
				});


				// graph
				auto *graph = m_pPlotter->addGraph();
				auto *graph_err = new QCPErrorBars(m_pPlotter->xAxis, m_pPlotter->yAxis);
				graph_err->setDataPlottable(graph);

				QPen pen = QPen(QColor(colors[graphidx % colors.size()]));
				QBrush brush(pen.color());
				t_real ptsize = 8;
				graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, pen, brush, ptsize));
				graph->setPen(pen);
				graph_err->setSymbolGap(ptsize);

				graph->setData(x_data, y_data);
				graph_err->setData(y_err);


				// ranges
				auto xminmax = std::minmax_element(x_data.begin(), x_data.end());
				auto yminmax = std::minmax_element(y_data.begin(), y_data.end());
				auto yerrminmax = std::minmax_element(y_err.begin(), y_err.end());

				xmin = std::min(*xminmax.first, xmin);
				xmax = std::max(*xminmax.second, xmax);
				ymin = std::min(*yminmax.first - *yerrminmax.first, ymin);
				ymax = std::max(*yminmax.second + *yerrminmax.second, ymax);
			}


			m_pPlotter->xAxis->setRange(xmin, xmax);
			m_pPlotter->yAxis->setRange(ymin, ymax);
		}

		m_pPlotter->replot();
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
 * load selected file(s) into workspace
 */
void FileBrowserWidget::TransferToWorkspace(const QList<QListWidgetItem*>& lst)
{

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
