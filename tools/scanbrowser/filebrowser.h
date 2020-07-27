/**
 * data file browser
 * @author Tobias Weber <tweber@ill.fr>
 * @date 9-Apr-2018
 * @license see 'LICENSE' file
 */

#ifndef __FILEBROWSER_H__
#define __FILEBROWSER_H__

#include "data.h"
//#include "plot.h"

#include <QtCore/QSettings>
#include <QtCore/QEvent>
#include <QtWidgets/QWidget>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMenu>

#include <memory>
#include <vector>
#include <string>



/**
 * file browser widget
 */
class FileBrowserWidget : public QWidget
{ Q_OBJECT
private:
	QSettings *m_pSettings = nullptr;

	QLineEdit *m_pEditFolder = new QLineEdit(this);
	QListWidget *m_pListFiles = new QListWidget(this);
	//Plotter *m_pPlotter = new Plotter(this);
	QMenu *m_pFileListContextMenu = new QMenu(m_pListFiles);

public:
	FileBrowserWidget(QWidget *pParent = nullptr, QSettings *pSettings = nullptr);
	virtual ~FileBrowserWidget();

protected:
	void SelectFolder();
	void SetFolder(const QString& str);
	void SetFile(QListWidgetItem *pCur);

	void ShowFileListContextMenu(const QPoint& pt);
	void FileDoubleClicked(QListWidgetItem *pItem);
	void TransferSelectedToWorkspace();
	void TransferToWorkspace(const QList<QListWidgetItem*>&);

	bool eventFilter(QObject *pObj, QEvent *pEvt);

signals:
	void TransferFiles(const std::vector<std::string>&);
	void PlotDataset(const Dataset &dataset);
};



/**
 * the dock which contains the file browser widget
 */
class FileBrowser : public QDockWidget
{
private:
	std::unique_ptr<FileBrowserWidget> m_pBrowser;

public:
	FileBrowser(QWidget* pParent = nullptr, QSettings *pSettings = nullptr);
	virtual ~FileBrowser();

	const FileBrowserWidget *GetWidget() const { return m_pBrowser.get(); }
};

#endif
