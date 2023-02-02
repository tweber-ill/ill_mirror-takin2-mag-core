/**
 * data file browser
 * @author Tobias Weber <tweber@ill.fr>
 * @date 9-Apr-2018
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

#ifndef __FILEBROWSER_H__
#define __FILEBROWSER_H__

#include "data.h"

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
