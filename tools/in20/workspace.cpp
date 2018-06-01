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

#include <boost/filesystem.hpp>

#include "libs/algos.h"
#include "tlibs/file/loadinstr.h"


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
		boost::filesystem::path filepath(file);
		if(!boost::filesystem::exists(filepath))
			continue;

		// load instrument data file
		std::unique_ptr<tl::FileInstrBase<t_real>> pInstr(tl::FileInstrBase<t_real>::LoadInstr(filepath.string().c_str()));
		const auto &colnames = pInstr->GetColNames();
		const auto &filedata = pInstr->GetData();

		if(!pInstr || !colnames.size())	// only valid files with a non-zero column count
			continue;


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


		std::size_t numpolstates = pInstr->NumPolChannels();
		if(numpolstates == 0) numpolstates = 1;


		Dataset dataset;

		// iterate through all (polarisation) subplots
		for(std::size_t polstate=0; polstate<numpolstates; ++polstate)
		{
			Data data;

			// get scan axes
			for(std::size_t idx : scan_idx)
			{
				std::vector<t_real> thedat;
				copy_interleave(filedata[idx].begin(), filedata[idx].end(), std::back_inserter(thedat), numpolstates, polstate);
				data.AddAxis(thedat, colnames[idx]);
			}


			// get counters
			for(std::size_t idx : ctr_idx)
			{
				std::vector<t_real> thedat, theerr;
				copy_interleave(filedata[idx].begin(), filedata[idx].end(), std::back_inserter(thedat), numpolstates, polstate);
				std::transform(filedata[idx].begin(), filedata[idx].end(), std::back_inserter(theerr),
					[](t_real y) -> t_real
					{
						if(tl::float_equal<t_real>(y, 0))
							return 1;
						return std::sqrt(y);
					});

				data.AddCounter(thedat, theerr);
			}


			// get monitors
			for(std::size_t idx : mon_idx)
			{
				std::vector<t_real> thedat, theerr;
				copy_interleave(filedata[idx].begin(), filedata[idx].end(), std::back_inserter(thedat), numpolstates, polstate);
				std::transform(filedata[idx].begin(), filedata[idx].end(), std::back_inserter(theerr),
					[](t_real y) -> t_real
					{
						if(tl::float_equal<t_real>(y, 0))
							return 1;
						return std::sqrt(y);
					});

				data.AddMonitor(thedat, theerr);
			}

			dataset.AddChannel(std::move(data));
		}


		// insert the dataset into the workspace
		std::string fileident = "data" + filepath.stem().string();
		const auto [iter, insert_ok] = m_workspace.emplace(std::make_pair(fileident, std::move(dataset)));
	}

	UpdateList();
}


/**
 * sync the list widget with the workspace map
 */
void WorkSpaceWidget::UpdateList()
{
	for(const auto& [ident, dataset] : m_workspace)
	{
		QString qident(ident.c_str());
		// dataset with this identifier already in list?
		if(m_pListFiles->findItems(qident, Qt::MatchCaseSensitive|Qt::MatchExactly).size())
			continue;

		auto *pItem = new QListWidgetItem(m_pListFiles);
		pItem->setText(qident);
		pItem->setData(Qt::UserRole, qident);
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
