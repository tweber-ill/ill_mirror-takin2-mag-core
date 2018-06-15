/**
 * workspace
 * @author Tobias Weber <tweber@ill.fr>
 * @date 25-May-2018
 * @license see 'LICENSE' file
 */

#ifndef __WORKSPACE_H__
#define __WORKSPACE_H__

#include <QtCore/QSettings>
#include <QtWidgets/QWidget>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>

#include <memory>
#include <map>

#include "data.h"
#include "plot.h"



/**
 * work space widget
 */
class WorkSpaceWidget : public QWidget
{
private:
	QSettings *m_pSettings = nullptr;

	QListWidget *m_pListFiles = new QListWidget(this);
	Plotter *m_pPlotter = new Plotter(this);

	// maps an identifier to a dataset
	std::map<std::string, Dataset> m_workspace;

public:
	WorkSpaceWidget(QWidget *pParent = nullptr, QSettings *pSettings = nullptr);
	virtual ~WorkSpaceWidget();

protected:
	void ItemSelected(QListWidgetItem* pCur);
	void ItemDoubleClicked(QListWidgetItem* pCur);

	void UpdateList();

public:
	void ReceiveFiles(const std::vector<std::string>&);
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
};

#endif
