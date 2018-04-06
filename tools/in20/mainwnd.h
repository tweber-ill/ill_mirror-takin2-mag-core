/**
 * in20 data analysis tool
 * @author Tobias Weber
 * @date 6-Apr-2018
 * @license see 'LICENSE' file
 */

#ifndef __IN20MAINWND_H__
#define __IN20MAINWND_H__

#include <QMainWindow>
#include <QSettings>
#include <QMenuBar>
#include <QStatusBar>


class MainWnd : public QMainWindow
{
private:
	QSettings *m_pSettings = nullptr;
	QMenuBar *m_pMenu = nullptr;
	QStatusBar *m_pStatus = nullptr;

private:
	virtual void closeEvent(QCloseEvent *pEvt) override;

public:
	MainWnd(QSettings* pSettings = nullptr);
	virtual ~MainWnd();
};

#endif
