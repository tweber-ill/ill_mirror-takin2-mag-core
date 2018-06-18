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
#include <QtWidgets/QComboBox>
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
	QComboBox *m_pEditCLI = new QComboBox(this);

	CliParserContext m_parsectx;


protected:
	void CommandEntered();

public:
	CommandLineWidget(QWidget *pParent = nullptr, QSettings *pSettings = nullptr);
	virtual ~CommandLineWidget();

	CliParserContext& GetParserContext() { return m_parsectx; }
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

	const CommandLineWidget* GetWidget() const { return m_pCLI.get(); }
	CommandLineWidget* GetWidget() { return m_pCLI.get(); }
};


#endif
