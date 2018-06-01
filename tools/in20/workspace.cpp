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
#include <algorithm>

#include "tlibs/file/loadinstr.h"


using t_real = double;


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
		// load instrument data file
		std::unique_ptr<tl::FileInstrBase<t_real>> pInstr(tl::FileInstrBase<t_real>::LoadInstr(file.c_str()));
		const auto &colnames = pInstr->GetColNames();

		if(pInstr && colnames.size())	// only valid files with a non-zero column count
		{
			// process polarisation data
			pInstr->SetPolNames("p1", "p2", "i11", "i10");
			pInstr->ParsePolData();


			// get scan axis indices
			std::vector<std::size_t> scan_idx;
			for(const auto& scanvar : pInstr->GetScannedVars())
			{
				std::size_t idx = 0;
				pInstr->GetCol(scanvar, &idx);
				if(idx < colnames.size())
					scan_idx.push_back(idx);
			}
			// try first axis if none found
			if(scan_idx.size() == 0) scan_idx.push_back(0);


			// get counter column index
			std::vector<std::size_t> ctr_idx;
			{
				std::size_t idx = 0;
				pInstr->GetCol(pInstr->GetCountVar(), &idx);
				if(idx < colnames.size())
					ctr_idx.push_back(idx);
			}
			// try second axis if none found
			if(ctr_idx.size() == 0) ctr_idx.push_back(1);


			// get monitor column index
			std::vector<std::size_t> mon_idx;
			{
				std::size_t idx = 0;
				pInstr->GetCol(pInstr->GetMonVar(), &idx);
				if(idx < colnames.size())
					mon_idx.push_back(idx);
			}
		}
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
