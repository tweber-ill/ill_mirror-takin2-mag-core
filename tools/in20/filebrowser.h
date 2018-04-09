/**
 * data file browser
 * @author Tobias Weber <tweber@ill.fr>
 * @date 9-Apr-2018
 * @license see 'LICENSE' file
 */

#ifndef __FILEBROWSER_H__
#define __FILEBROWSER_H__

#include <QtWidgets/QWidget>
#include <QtWidgets/QDockWidget>

#include <memory>


/**
 * file browser widget
 */
class FileBrowserWidget : public QWidget
{
private:

public:
	FileBrowserWidget(QWidget *pParent = nullptr);
	virtual ~FileBrowserWidget();
};


/**
 * the dock which contains the file browser widget
 */
class FileBrowser : public QDockWidget
{
private:
	std::unique_ptr<FileBrowserWidget> m_pBrowser;

public:
	FileBrowser(QWidget* pParent = nullptr);
	virtual ~FileBrowser();
};

#endif
