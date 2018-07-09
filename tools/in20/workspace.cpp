/**
 * workspace
 * @author Tobias Weber <tweber@ill.fr>
 * @date 25-May-2018
 * @license see 'LICENSE' file
 */

#include "workspace.h"

#include <QtCore/QDirIterator>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>

#include "globals.h"


using t_real = t_real_dat;


// ----------------------------------------------------------------------------

WorkSpaceWidget::WorkSpaceWidget(QWidget *pParent, QSettings *pSettings)
	: QWidget(pParent), m_pSettings(pSettings)
{
	m_pListFiles->setAlternatingRowColors(true);
	m_pListFiles->installEventFilter(this);

	// ------------------------------------------------------------------------
	auto *pGrid = new QGridLayout(this);
	pGrid->addWidget(m_pListFiles, 1, 0, 2, 2);
	// ------------------------------------------------------------------------


	// ------------------------------------------------------------------------
	// connections
	connect(m_pListFiles, &QListWidget::currentItemChanged, this, &WorkSpaceWidget::ItemSelected);
	connect(m_pListFiles, &QListWidget::itemDoubleClicked, this, &WorkSpaceWidget::ItemDoubleClicked);
	// ------------------------------------------------------------------------


	// ------------------------------------------------------------------------
	// settings
	if(m_pSettings)
	{
	}
	// ------------------------------------------------------------------------
}


WorkSpaceWidget::~WorkSpaceWidget()
{
}


/**
 * an item in the list was selected
 */
void WorkSpaceWidget::ItemSelected(QListWidgetItem* pCur)
{
	if(!pCur) return;

	std::string ident = pCur->text().toStdString();
	auto iter = m_workspace.find(ident);
	if(iter == m_workspace.end())
	{
		print_err("Variable \"", ident, "\" is not in workspace.");
		return;
	}

	auto symdataset = iter->second;
	if(symdataset->GetType() != SymbolType::DATASET)
	{
		print_err("Variable \"", ident, "\" is not of dataset type.");
		return;
	}

	const Dataset& dataset = dynamic_cast<const SymbolDataset&>(*symdataset).GetValue();
	emit PlotDataset(dataset);
}


/**
 * an item in the list was double-clicked
 */
void WorkSpaceWidget::ItemDoubleClicked(QListWidgetItem* pCur)
{
}



/**
 * transfer a file from the file browser and convert it into the internal format
 */
void WorkSpaceWidget::ReceiveFiles(const std::vector<std::string> &files)
{
	for(const auto& file : files)
	{
		QFileInfo filepath(file.c_str());
		if(!filepath.exists())
		{
			print_err("File \"", file, "\" does not exist.");
			//continue;
		}

		auto [ok, dataset] = Dataset::convert_instr_file(file.c_str());
		if(!ok)
		{
			print_err("File \"", file, "\" cannot be converted.");
			continue;
		}

		// insert the dataset into the workspace
		std::string fileident = "sc" + filepath.baseName().toStdString();
		const auto [iter, insert_ok] = m_workspace.emplace(std::make_pair(fileident, std::make_shared<SymbolDataset>(std::move(dataset))));
	}

	UpdateList();
}


/**
 * sync the list widget with the workspace map
 */
void WorkSpaceWidget::UpdateList()
{
	// add missing symbols to workspace list
	for(const auto& [ident, symdataset] : m_workspace)
	{
		// skip real or string variables
		if(symdataset->GetType() != SymbolType::DATASET)
			continue;
		const Dataset& dataset = dynamic_cast<const SymbolDataset&>(*symdataset).GetValue();

		QString qident(ident.c_str());
		// dataset with this identifier already in list?
		if(m_pListFiles->findItems(qident, Qt::MatchCaseSensitive|Qt::MatchExactly).size())
			continue;

		auto *pItem = new QListWidgetItem(m_pListFiles);
		pItem->setText(qident);
		//pItem->setData(Qt::UserRole, qident);
	}


	// remove superfluous symbols from workspace list
	for(int idx=m_pListFiles->count()-1; idx>=0; --idx)
	{
		std::string ident = m_pListFiles->item(idx)->text().toStdString();
		if(m_workspace.find(ident) == m_workspace.end())
			delete m_pListFiles->item(idx);
	}
}


/**
 * re-directed child events
 */
bool WorkSpaceWidget::eventFilter(QObject *pObj, QEvent *pEvt)
{
	if(pObj == m_pListFiles)
	{
		// as the file browser and the work space widget share the same plotter, re-send plot on activation
		if(pEvt->type() == QEvent::FocusIn)
			ItemSelected(m_pListFiles->currentItem());
	}

	return QObject::eventFilter(pObj, pEvt);
}

// ----------------------------------------------------------------------------





// ----------------------------------------------------------------------------

WorkSpace::WorkSpace(QWidget* pParent, QSettings *pSettings)
	: QDockWidget(pParent), m_pWS(std::make_unique<WorkSpaceWidget>(this, pSettings))
{
	this->setObjectName("workSpace");
	this->setWindowTitle("Workspace");
	this->setWidget(m_pWS.get());
}

WorkSpace::~WorkSpace()
{
}

// ----------------------------------------------------------------------------
