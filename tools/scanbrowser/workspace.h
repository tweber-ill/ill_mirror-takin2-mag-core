/**
 * workspace
 * @author Tobias Weber <tweber@ill.fr>
 * @date 25-May-2018
 * @license see 'LICENSE' file
 */

#ifndef __WORKSPACE_H__
#define __WORKSPACE_H__

#include "cli/cliparser.h"
#include "data.h"
#include "tlibs2/libs/file.h"

#include <QtCore/QSettings>
#include <QtCore/QEvent>
#include <QtWidgets/QWidget>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMenu>

#include <memory>
#include <map>



/**
 * work space widget
 */
class WorkSpaceWidget : public QWidget
{ Q_OBJECT
private:
	QSettings *m_pSettings = nullptr;
	QListWidget *m_pListFiles = new QListWidget(this);
	QMenu *m_pFileListContextMenu = new QMenu(m_pListFiles);

	// maps an identifier to a dataset
	std::map<std::string, std::shared_ptr<Symbol>> m_workspace;

public:
	WorkSpaceWidget(QWidget *pParent = nullptr, QSettings *pSettings = nullptr);
	virtual ~WorkSpaceWidget();

	std::map<std::string, std::shared_ptr<Symbol>>* GetWorkspace() { return &m_workspace; }

	bool LoadWorkspace(const std::string &basename, const tl2::Prop<std::string> &prop);
	bool SaveWorkspace(const std::string &basename, std::unordered_map<std::string, std::string> &map) const;

protected:
	void ShowFileListContextMenu(const QPoint& pt);
	void ItemSelected(QListWidgetItem* pCur);
	void ItemDoubleClicked(QListWidgetItem* pCur);
	void StartItemEditor();		// rename item
	void ItemEdited();			// item was renamed
	void RemoveSelectedItems();
	bool eventFilter(QObject *pObj, QEvent *pEvt);

public:
	void ReceiveFiles(const std::vector<std::string>&);
	void UpdateList();

signals:
	void PlotDataset(const Dataset &dataset);
};



/**
 * the dock which contains the workspace widget
 */
class WorkSpace : public QDockWidget
{
private:
	std::unique_ptr<WorkSpaceWidget> m_pWS;

public:
	WorkSpace(QWidget* pParent = nullptr, QSettings *pSettings = nullptr);
	virtual ~WorkSpace();

	const WorkSpaceWidget* GetWidget() const { return m_pWS.get(); }
	WorkSpaceWidget* GetWidget() { return m_pWS.get(); }
};

#endif
