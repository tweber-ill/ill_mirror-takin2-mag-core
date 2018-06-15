/**
 * Command line
 * @author Tobias Weber <tweber@ill.fr>
 * @date 31-May-2018
 * @license see 'LICENSE' file
 */

#ifndef __CLI_WND_H__
#define __CLI_WND_H__


#include <QtCore/QSettings>
#include <QtWidgets/QWidget>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTextEdit>

#include <memory>
#include "tools/cli/cliparser.h"


/**
 * command line widget
 */
class CommandLineWidget : public QWidget
{
private:
	QSettings *m_pSettings = nullptr;

	QTextEdit *m_pEditHistory = new QTextEdit(this);
	QLineEdit *m_pEditCLI = new QLineEdit(this);

	CliParserContext m_parsectx;


protected:
	void CommandEntered();

public:
	CommandLineWidget(QWidget *pParent = nullptr, QSettings *pSettings = nullptr);
	virtual ~CommandLineWidget();
};



/**
 * the dock which contains the command line widget
 */
class CommandLine : public QDockWidget
{
private:
	std::unique_ptr<CommandLineWidget> m_pCLI;

public:
	CommandLine(QWidget* pParent = nullptr, QSettings *pSettings = nullptr);
	virtual ~CommandLine();
};


#endif
