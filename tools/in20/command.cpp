/**
 * Command line
 * @author Tobias Weber <tweber@ill.fr>
 * @date 31-May-2018
 * @license see 'LICENSE' file
 */

#include "command.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLineEdit>


// ----------------------------------------------------------------------------

CommandLineWidget::CommandLineWidget(QWidget *pParent, QSettings *pSettings)
	: QWidget(pParent), m_pSettings(pSettings)
{
	m_pEditHistory->setReadOnly(true);
	m_pEditHistory->setUndoRedoEnabled(false);

	m_pEditCLI->setInsertPolicy(QComboBox::NoInsert);
	m_pEditCLI->setEditable(true);
	m_pEditCLI->lineEdit()->setPlaceholderText("Enter Command");


	// ------------------------------------------------------------------------
	// layout
	auto *pGrid = new QGridLayout(this);
	pGrid->addWidget(m_pEditHistory, 0, 0, 1, 1);
	pGrid->addWidget(m_pEditCLI, 1, 0, 1, 1);
	// ------------------------------------------------------------------------


	// ------------------------------------------------------------------------
	// connections
	connect(m_pEditCLI->lineEdit(), &QLineEdit::returnPressed, this, &CommandLineWidget::CommandEntered);
	// ------------------------------------------------------------------------


	// ------------------------------------------------------------------------
	// settings
	if(m_pSettings)
	{
	}
	// ------------------------------------------------------------------------
}


CommandLineWidget::~CommandLineWidget()
{
}


void CommandLineWidget::CommandEntered()
{
	QString cmd = m_pEditCLI->currentText().trimmed();
	if(m_pEditCLI->findText(cmd) == -1)
		m_pEditCLI->addItem(cmd);
	m_pEditCLI->clearEditText();
	if(!cmd.length()) return;

	m_pEditHistory->insertHtml("<font color=\"#0000ff\"><b>> </b>" + cmd + "</font><br>");


	// parse command
	std::istringstream istr(cmd.toStdString() + "\n");
	m_parsectx.SetLexerInput(istr);

	// remove the asts for old commands
	m_parsectx.ClearASTs();
	yy::CliParser parser(m_parsectx);
	int parse_state = parser.parse();


	// write error log
	for(const auto& err : m_parsectx.GetErrors())
		m_pEditHistory->insertHtml((("<b><font color=\"#ff0000\">" + err + "</font></b><br>").c_str()));
	m_parsectx.ClearErrors();


	if(parse_state != 0)
	{
		m_pEditHistory->insertHtml("<b><font color=\"#ff0000\">Error: Could not parse command.</font></b><br>");
	}
	else
	{
		// evaluate commands
		for(const auto &ast : m_parsectx.GetASTs())
		{
			if(!ast) continue;

			//ast->Print(); std::cout.flush();
			auto sym = ast->Eval(m_parsectx);
			if(sym)
			{
				std::ostringstream ostrRes;

				if(sym->GetType() == SymbolType::REAL)
					ostrRes /*<< "real: "*/ << dynamic_cast<SymbolReal&>(*sym).GetValue();
				else if(sym->GetType() == SymbolType::STRING)
					ostrRes /*<< "string: "*/ << dynamic_cast<SymbolString&>(*sym).GetValue();
				else if(sym->GetType() == SymbolType::DATASET)
					ostrRes << "&lt;Dataset&gt;";

				m_pEditHistory->insertHtml((("<font color=\"#000000\">" + ostrRes.str() + "</font><br>").c_str()));
			}
			else
			{
				m_pEditHistory->insertHtml("<b><font color=\"#ff0000\">"
					"Unable to evaluate expression."
					"</font></b><br>");
			}
		}
	}


	// scroll command list to last command
	auto caret = m_pEditHistory->textCursor();
	caret.movePosition(QTextCursor::End, QTextCursor::MoveAnchor, 1);
	m_pEditHistory->setTextCursor(caret);
}

// ----------------------------------------------------------------------------





// ----------------------------------------------------------------------------

CommandLine::CommandLine(QWidget* pParent, QSettings *pSettings)
	: QDockWidget(pParent), m_pCLI(std::make_unique<CommandLineWidget>(this, pSettings))
{
	this->setObjectName("commandLine");
	this->setWindowTitle("Command Line");
	this->setWidget(m_pCLI.get());
}

CommandLine::~CommandLine()
{
}

// ----------------------------------------------------------------------------
