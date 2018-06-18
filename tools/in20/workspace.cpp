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

#include <iostream>


using t_real = t_real_dat;


// ----------------------------------------------------------------------------

WorkSpaceWidget::WorkSpaceWidget(QWidget *pParent, QSettings *pSettings)
	: QWidget(pParent), m_pSettings(pSettings)
{
	m_pListFiles->setAlternatingRowColors(true);

	// ------------------------------------------------------------------------
	auto *pGrid = new QGridLayout(this);
	pGrid->addWidget(m_pListFiles, 1, 0, 2, 2);
	pGrid->addWidget(m_pPlotter, 3, 0, 1, 2);
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
		std::cerr << "Variable \"" << ident << "\" is not in workspace." << std::endl;
		return;
	}

	auto symdataset = iter->second;
	if(symdataset->GetType() != SymbolType::DATASET)
	{
		std::cerr << "Variable \"" << ident << "\" is not of dataset type." << std::endl;
		return;
	}

	const Dataset& dataset = dynamic_cast<const SymbolDataset&>(*symdataset).GetValue();
	m_pPlotter->Plot(dataset);
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
			std::cerr << "File \"" << file << "\" does not exist." << std::endl;
			//continue;
		}

		auto [ok, dataset] = Dataset::convert_instr_file(file.c_str());
		if(!ok)
		{
			std::cerr << "File \"" << file << "\" cannot be converted." << std::endl;
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
