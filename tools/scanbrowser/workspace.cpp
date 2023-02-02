/**
 * workspace
 * @author Tobias Weber <tweber@ill.fr>
 * @date 25-May-2018
 * @license see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2023  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ----------------------------------------------------------------------------
 */

#include "workspace.h"

#include <QtCore/QDirIterator>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>

#include "tlibs2/libs/str.h"
#include "globals.h"



// ----------------------------------------------------------------------------

WorkSpaceWidget::WorkSpaceWidget(QWidget *pParent, QSettings *pSettings)
	: QWidget(pParent), m_pSettings(pSettings)
{
	// ------------------------------------------------------------------------
	// file list
	m_pListFiles->setAlternatingRowColors(true);
	m_pListFiles->installEventFilter(this);
	m_pListFiles->setContextMenuPolicy(Qt::CustomContextMenu);

	m_pFileListContextMenu->setTitle("Workspace");
	m_pFileListContextMenu->addAction("Rename Variable", this, &WorkSpaceWidget::StartItemEditor);
	m_pFileListContextMenu->addAction("Remove Variable", this, &WorkSpaceWidget::RemoveSelectedItems);
	// ------------------------------------------------------------------------


	// ------------------------------------------------------------------------
	auto *pGrid = new QGridLayout(this);
	pGrid->addWidget(m_pListFiles, 1, 0, 2, 2);
	// ------------------------------------------------------------------------


	// ------------------------------------------------------------------------
	// connections
	connect(m_pListFiles, &QListWidget::currentItemChanged, this, &WorkSpaceWidget::ItemSelected);
	connect(m_pListFiles, &QListWidget::itemDoubleClicked, this, &WorkSpaceWidget::ItemDoubleClicked);
	connect(m_pListFiles->itemDelegate(), &QAbstractItemDelegate::commitData, this, &WorkSpaceWidget::ItemEdited);
	connect(m_pListFiles, &QListWidget::customContextMenuRequested, this, &WorkSpaceWidget::ShowFileListContextMenu);
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
 * shows a context menu for selected files
 */
void WorkSpaceWidget::ShowFileListContextMenu(const QPoint& pt)
{
	// no file selected
	if(!m_pListFiles->itemAt(pt))
		return;

	auto ptGlob = m_pListFiles->mapToGlobal(pt);
	ptGlob.setY(ptGlob.y() + 8);
	m_pFileListContextMenu->popup(ptGlob);
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
void WorkSpaceWidget::ItemDoubleClicked(QListWidgetItem*)
{
}


/**
 * show the editor to rename the (first) selected item
 */
void WorkSpaceWidget::StartItemEditor()
{
	auto selected = m_pListFiles->selectedItems();
	if(selected.size() == 0) return;

	m_pListFiles->editItem(*selected.begin());
}


/**
 * an item in the list was edited
 */
void WorkSpaceWidget::ItemEdited()
{
	// the edited item is the one where the text does not match to the user data
	for(int item=0; item<m_pListFiles->count(); ++item)
	{
		auto* pItem = m_pListFiles->item(item);
		auto newName = pItem->text();
		auto oldName = pItem->data(Qt::UserRole).toString();

		// TODO: check if newName is a valid identifier
		if(newName == "")
		{
			print_err("\"", newName.toStdString(), "\" is an invalid identifier.");
			pItem->setText(oldName);	// rename back
			continue;
		}

		// try to change name
		if(newName != oldName)
		{
			// update the workspace map
			auto node = m_workspace.extract(oldName.toStdString());
			if(!node)
			{
				print_err("Variable \"", oldName.toStdString(), "\" cannot be found.");
				continue;
			}

			node.key() = newName.toStdString();
			auto inserted = m_workspace.insert(std::move(node));

			if(inserted.inserted)
			{
				// sync data with new name
				pItem->setData(Qt::UserRole, newName);
			}
			else
			{
				// renaming failed, undo changes
				inserted.node.key() = oldName.toStdString();
				m_workspace.insert(std::move(inserted.node));
				pItem->setText(oldName);

				print_err("Variable \"", oldName.toStdString(), "\" cannot be renamed due to a conflict.");
			}

			print_out("Variable renamed: \"", oldName.toStdString(), "\" -> \"", newName.toStdString(), "\".");
		}
	}
}



/**
 * removes selected items from the workspace
 */
void WorkSpaceWidget::RemoveSelectedItems()
{
	auto selected = m_pListFiles->selectedItems();

	for(auto* item : selected)
	{
		// remove corresponding workspace variable
		if(auto iter = m_workspace.find(item->text().toStdString()); iter != m_workspace.end())
			m_workspace.erase(iter);
	}

	UpdateList();
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
		/*const auto [iter, insert_ok] = */m_workspace.emplace(std::make_pair(fileident, std::make_shared<SymbolDataset>(std::move(dataset))));
	}

	UpdateList();
}


/**
 * sync the list widget with the workspace map
 */
void WorkSpaceWidget::UpdateList()
{
	// add missing symbols to list widget
	for(const auto& [ident, symdataset] : m_workspace)
	{
		// skip real or string variables
		if(symdataset->GetType() != SymbolType::DATASET)
			continue;
		//const Dataset& dataset = dynamic_cast<const SymbolDataset&>(*symdataset).GetValue();

		QString qident(ident.c_str());
		// dataset with this identifier already in list?
		if(m_pListFiles->findItems(qident, Qt::MatchCaseSensitive|Qt::MatchExactly).size())
			continue;

		auto *pItem = new QListWidgetItem(m_pListFiles);
		pItem->setText(qident);
		pItem->setData(Qt::UserRole, qident);	// also set identifier as data (used in renaming)
		pItem->setFlags(Qt::ItemIsEditable | pItem->flags());
	}


	// remove superfluous symbols from list widget
	for(int idx=m_pListFiles->count()-1; idx>=0; --idx)
	{
		// only keep datasets
		auto iter = m_workspace.find(m_pListFiles->item(idx)->text().toStdString());
		if(iter == m_workspace.end() || (!iter->second) || iter->second->GetType() != SymbolType::DATASET)
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



/**
 * load workspace variables from a property tree
 */
bool WorkSpaceWidget::LoadWorkspace(const std::string &basename, const tl2::Prop<std::string> &prop)
{
	// iterate over save variables
	std::size_t varnum = 0;
	while(true)
	{
		const std::string keyName = basename + "workspace/var_" + tl2::var_to_str(varnum) + "/name";
		const std::string keyValue = basename + "workspace/var_" + tl2::var_to_str(varnum) + "/value";
		++varnum;

		auto key = prop.QueryOpt<std::string>(keyName);
		auto val = prop.QueryOpt<std::string>(keyValue);
		if(!key || !val) break;		// no more variables?

		// unserialise symbol from value string
		auto sym = Symbol::unserialise(*val);
		if(!sym)
		{
			print_err("Cannot unserialise variable \"", *key, "\".");
			continue;
		}

		m_workspace.insert(std::make_pair(*key, sym));
	}

	UpdateList();
	return true;
}


/**
 * save workspace variables to a map
 */
bool WorkSpaceWidget::SaveWorkspace(const std::string &basename, std::unordered_map<std::string, std::string> &map) const
{
	std::size_t varnum = 0;
	for(const auto &pair : m_workspace)
	{
		if(!pair.second)
			continue;

		map[basename + "workspace/var_" + tl2::var_to_str(varnum) + "/name"] = pair.first;
		map[basename + "workspace/var_" + tl2::var_to_str(varnum) + "/value"] = pair.second->serialise();
		++varnum;
	}

	return true;
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
