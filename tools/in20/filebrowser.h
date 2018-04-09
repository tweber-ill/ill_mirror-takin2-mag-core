/**
 * data file browser
 * @author Tobias Weber <tweber@ill.fr>
 * @date 9-Apr-2018
 * @license see 'LICENSE' file
 */

#ifndef __FILEBROWSER_H__
#define __FILEBROWSER_H__

#include <QtCore/QSettings>
#include <QtWidgets/QWidget>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>

#include <memory>


/**
 * file browser widget
 */
class FileBrowserWidget : public QWidget
{
private:
	QSettings *m_pSettings = nullptr;

	QLineEdit *m_pEditFolder = new QLineEdit(this);
	QListWidget *m_pListFiles = new QListWidget(this);

public:
	FileBrowserWidget(QWidget *pParent = nullptr, QSettings *pSettings = nullptr);
	virtual ~FileBrowserWidget();

public:
	void SelectFolder();
	void SetFolder(const QString& str);
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
};

#endif