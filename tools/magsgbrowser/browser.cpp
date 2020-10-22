/**
 * space group browser
 * @author Tobias Weber <tweber@ill.fr>
 * @date Apr-2018
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 8-Nov-2018 from my privately developed "magtools" project (https://github.com/t-weber/magtools).
 */

#include "browser.h"
#include "../structfact/loadcif.h"
#include <QtWidgets/QMenuBar>
#include <sstream>


// ----------------------------------------------------------------------------


SgBrowserDlg::SgBrowserDlg(QWidget* pParent, QSettings *pSett)
	: QDialog{pParent}, m_pSettings{pSett}
{
	// ------------------------------------------------------------------------
	// setup UI
	this->setupUi(this);
	auto *pLayout = this->layout();

	// menu
	auto *pMenuBar = new QMenuBar(this);
	auto *pMenuOptions = new QMenu("Options", pMenuBar);

	auto *pShowBNS = new QAction("Show BNS", pMenuOptions);
	pShowBNS->setCheckable(true);
	pShowBNS->setChecked(true);
	pMenuOptions->addAction(pShowBNS);

	pMenuBar->addMenu(pMenuOptions);
	pLayout->setMenuBar(pMenuBar);
	// ------------------------------------------------------------------------


	// ------------------------------------------------------------------------
	// restore settings
	if(m_pSettings)
	{
		if(m_pSettings->contains("sgbrowser/geo"))
			this->restoreGeometry(m_pSettings->value("sgbrowser/geo").toByteArray());
		//if(m_pSettings->contains("sgbrowser/state"))
		//	this->restoreState(m_pSettings->value("sgbrowser/state").toByteArray());
	}
	// ------------------------------------------------------------------------


	// load data
	SetupMagSpaceGroups();
	SetupNuclSpaceGroups();


	// ------------------------------------------------------------------------
	// connections
	connect(treeMagSG, &QTreeWidget::currentItemChanged, this, &SgBrowserDlg::MagSpaceGroupSelected);
	connect(treeNuclSG, &QTreeWidget::currentItemChanged, this, &SgBrowserDlg::NuclSpaceGroupSelected);
	connect(pShowBNS, &QAction::toggled, this, &SgBrowserDlg::SwitchToBNS);

	// synchronise symops when switching the sg tab
	connect(tabSGs, &QTabWidget::currentChanged, this, [this](int idx) -> void
	{
		switch(idx)
		{
			case 0: MagSpaceGroupSelected(treeMagSG->currentItem()); break;
			case 1: NuclSpaceGroupSelected(treeNuclSG->currentItem()); break;
		}
	});
	// ------------------------------------------------------------------------
}


// ----------------------------------------------------------------------------
/**
 * load magnetic space group list
 */
void SgBrowserDlg::SetupMagSpaceGroups()
{
	std::cerr << "Loading magnetic space groups ... ";
	m_magsgs.Load("res/data/magsg.info");
	std::cerr << "Done." << std::endl;

	const auto *pSgs = m_magsgs.GetSpacegroups();
	if(!pSgs) return;

	for(const auto& sg : *pSgs)
	{
		int iNrStruct = sg.GetStructNumber();
		int iNrMag = sg.GetMagNumber();

		// find top-level item with given structural sg number
		auto get_topsg = [](QTreeWidget *pTree, int iStructNr) -> QTreeWidgetItem*
		{
			for(int item=0; item<pTree->topLevelItemCount(); ++item)
			{
				QTreeWidgetItem *pItem = pTree->topLevelItem(item);
				if(!pItem) continue;

				if(pItem->data(0, Qt::UserRole) == iStructNr)
					return pItem;
			}

			return nullptr;
		};


		// find existing top-level structural space group or insert new one
		auto *pTopItem = get_topsg(treeMagSG, iNrStruct);
		if(!pTopItem)
		{
			QString structname = ("(" + std::to_string(iNrStruct) + ") " + sg.GetName()).c_str();

			pTopItem = new QTreeWidgetItem();
			pTopItem->setText(0, structname);
			pTopItem->setData(0, Qt::UserRole, iNrStruct);
			pTopItem->setData(0, Qt::UserRole+1, iNrMag);
			treeMagSG->addTopLevelItem(pTopItem);
		}


		// create magnetic group and add it as sub-item for the corresponding structural group
		QString magname = ("(" + sg.GetNumber() + ") " + sg.GetName()).c_str();

		auto *pSubItem = new QTreeWidgetItem();
		pSubItem->setText(0, magname);
		pSubItem->setData(0, Qt::UserRole, iNrStruct);
		pSubItem->setData(0, Qt::UserRole+1, iNrMag);
		pTopItem->addChild(pSubItem);
	}
}


/**
 * switched to show BNS (otherwise OG)
 */
void SgBrowserDlg::SwitchToBNS(bool bBNS)
{
	m_showBNS = bBNS;
	MagSpaceGroupSelected(treeMagSG->currentItem());
}


/**
 * magnetic space group selected
 */
void SgBrowserDlg::MagSpaceGroupSelected(QTreeWidgetItem *pItem)
{
	if(!pItem) return;

	// clean up
	listSymOps->clear();
	listWyc->clear();


	int iNrStruct = pItem->data(0, Qt::UserRole).toInt();
	int iNrMag = pItem->data(0, Qt::UserRole+1).toInt();

	const auto* pSg = m_magsgs.GetSpacegroupByNumber(iNrStruct, iNrMag);
	if(!pSg) return;


	// print a symmetry operator
	auto print_sym = [](const t_mat_sg& mat, const t_vec_sg& vec, t_real_sg inv) -> std::string
	{
		std::ostringstream ostr;

		for(std::size_t i=0; i<mat.size1(); ++i)
		{
			// rotation matrix
			ostr << "( ";
			for(std::size_t j=0; j<mat.size2(); ++j)
				ostr << std::setw(ostr.precision()*1.5) << std::right << mat(i,j);
			ostr << " )";

			// translation vector
			ostr << std::setw(ostr.precision()*2) << std::right << "( ";
			ostr << std::setw(ostr.precision()*1.5) << std::right << vec[i]; 
			ostr << " )";

			// time inversion
			if(i==mat.size1()/2)
			{
				ostr << std::setw(ostr.precision()*2) << std::right << "t = ";
				ostr << inv;
			}

			if(i < mat.size1()-1)
				ostr << "\n";
		}

		return ostr.str();
	};


	// print a wyckoff position
	auto print_wyc = [](const t_mat_sg& mat, const t_mat_sg& matRot,  const t_vec_sg& vec) -> std::string
	{
		std::ostringstream ostr;

		for(std::size_t i=0; i<mat.size1(); ++i)
		{
			// rotation matrices
			ostr << "( ";
			for(std::size_t j=0; j<mat.size2(); ++j)
				ostr << std::setw(ostr.precision()*1.5) << std::right << mat(i,j);

			ostr  << " | ";
			for(std::size_t j=0; j<matRot.size2(); ++j)
				ostr << std::setw(ostr.precision()*1.5) << std::right << matRot(i,j);
			ostr << " )";

			// translation vector
			ostr << std::setw(ostr.precision()*2) << std::right << "( ";
			ostr << std::setw(ostr.precision()*1.5) << std::right << vec[i]; 
			ostr << " )";

			if(i < mat.size1()-1)
				ostr << "\n";
		}

		return ostr.str();
	};


	// iterate over symmetries
	const auto *symms = pSg->GetSymmetries(m_showBNS);
	if(symms)
	{
		for(std::size_t iOp=0; iOp<symms->GetRotations().size(); ++iOp)
		{
			const auto& rot = symms->GetRotations()[iOp];
			const auto& trans = symms->GetTranslations()[iOp];
			const auto inv = symms->GetInversions()[iOp];

			auto *pOpItem = new QListWidgetItem();
			pOpItem->setText(print_sym(rot, trans, inv).c_str());
			listSymOps->addItem(pOpItem);
		}
	}


	// iterate over wyckoff positions
	const auto *wycs = pSg->GetWycPositions(m_showBNS);
	if(wycs)
	{
		for(const auto &wyc : *wycs)
		{
			auto *pWycItem = new QTreeWidgetItem();
			pWycItem->setText(0, wyc.GetName().c_str());

			// iterate over trafos
			for(std::size_t iOp=0; iOp<wyc.GetRotations().size(); ++iOp)
			{
				const auto& rot = wyc.GetRotations()[iOp];
				const auto& rotMag = wyc.GetRotationsMag()[iOp];
				const auto& trans = wyc.GetTranslations()[iOp];

				auto *pSubItem = new QTreeWidgetItem();
				pSubItem->setText(0, print_wyc(rot, rotMag, trans).c_str());
				pWycItem->addChild(pSubItem);
			}

			listWyc->addTopLevelItem(pWycItem);
			pWycItem->setExpanded(true);
		}
	}
}
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
/**
 * load nuclear space group list
 */
void SgBrowserDlg::SetupNuclSpaceGroups()
{
	m_nuclsgs = get_sgs<t_mat44_sg>(false, false);

	for(int iSG=0; iSG<static_cast<int>(m_nuclsgs.size()); ++iSG)
	{
		int iNrStruct = std::get<0>(m_nuclsgs[iSG]);
		const std::string& strName = std::get<1>(m_nuclsgs[iSG]);

		// find top-level item with given structural sg number
		auto get_topsg = [](QTreeWidget *pTree, int iStructNr) -> QTreeWidgetItem*
		{
			for(int item=0; item<pTree->topLevelItemCount(); ++item)
			{
				QTreeWidgetItem *pItem = pTree->topLevelItem(item);
				if(!pItem) continue;

				if(pItem->data(0, Qt::UserRole) == iStructNr)
					return pItem;
			}

			return nullptr;
		};


		// find existing top-level structural space group or insert new one
		auto *pTopItem = get_topsg(treeNuclSG, iNrStruct);
		if(!pTopItem)
		{
			QString structname = ("(" + std::to_string(iNrStruct) + ") " + strName).c_str();

			pTopItem = new QTreeWidgetItem();
			pTopItem->setText(0, structname);
			pTopItem->setData(0, Qt::UserRole, iNrStruct);
			pTopItem->setData(0, Qt::UserRole+1, iSG);
			treeNuclSG->addTopLevelItem(pTopItem);
		}


		// create nuclear group config and add it as sub-item for the corresponding structural group
		QString magname = ("(" + std::to_string(iNrStruct) + 
			"-" + std::to_string(pTopItem->childCount()+1) + ") " + strName).c_str();

		auto *pSubItem = new QTreeWidgetItem();
		pSubItem->setText(0, magname);
		pSubItem->setData(0, Qt::UserRole, iNrStruct);
		pSubItem->setData(0, Qt::UserRole+1, iSG);
		pTopItem->addChild(pSubItem);
	}
}


/**
 * nuclear space group selected
 */
void SgBrowserDlg::NuclSpaceGroupSelected(QTreeWidgetItem *pItem)
{
	if(!pItem) return;

	// clean up
	listSymOps->clear();
	listWyc->clear();


	int iNrStruct = pItem->data(0, Qt::UserRole).toInt();
	int iNrSG = pItem->data(0, Qt::UserRole+1).toInt();
	if(iNrSG < 0 || iNrSG >= m_nuclsgs.size())
		return;

	const auto& sg = m_nuclsgs[iNrSG];
	const auto& trafos = std::get<2>(sg);


	// print a symmetry operator
	auto print_sym = [](const t_mat44_sg& mat) -> std::string
	{
		std::ostringstream ostr;

		for(std::size_t i=0; i<3; ++i)
		{
			// rotation matrix
			ostr << "( ";
			for(std::size_t j=0; j<3; ++j)
				ostr << std::setw(ostr.precision()*1.5) << std::right << mat(i,j);
			ostr << " )";

			// translation vector
			ostr << std::setw(ostr.precision()*2) << std::right << "( ";
			ostr << std::setw(ostr.precision()*1.5) << std::right << mat(i, 3); 
			ostr << " )";

			if(i < mat.size1()-2)
				ostr << "\n";
		}

		return ostr.str();
	};


	// iterate over symmetries
	for(std::size_t iOp=0; iOp<trafos.size(); ++iOp)
	{
		const auto& trafo = trafos[iOp];

		auto *pOpItem = new QListWidgetItem();
		pOpItem->setText(print_sym(trafo).c_str());
		listSymOps->addItem(pOpItem);
	}
}
// ----------------------------------------------------------------------------





// ----------------------------------------------------------------------------
void SgBrowserDlg::showEvent(QShowEvent *pEvt)
{
	QDialog::showEvent(pEvt);
}


void SgBrowserDlg::hideEvent(QHideEvent *pEvt)
{
	QDialog::hideEvent(pEvt);
}


void SgBrowserDlg::closeEvent(QCloseEvent *pEvt)
{
	if(m_pSettings)
	{
		m_pSettings->setValue("sgbrowser/geo", this->saveGeometry());
		//m_pSettings->setValue("sgbrowser/state", this->saveState());
	}

	QDialog::closeEvent(pEvt);
}

// ----------------------------------------------------------------------------
