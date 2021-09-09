/**
 * Command line
 * @author Tobias Weber <tweber@ill.fr>
 * @date 31-May-2018
 * @license see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2021  Tobias WEBER (Institut Laue-Langevin (ILL),
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

#include "command.h"
#include "tlibs2/libs/algos.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QCompleter>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QTableView>
#include <QtWidgets/QHeaderView>
#include <QtGui/QStandardItemModel>


// ----------------------------------------------------------------------------

CommandLineWidget::CommandLineWidget(QWidget *pParent, QSettings *pSettings)
	: QWidget(pParent), m_pSettings(pSettings)
{
	m_pEditHistory->setReadOnly(true);
	m_pEditHistory->setUndoRedoEnabled(false);

	m_pEditCLI->setInsertPolicy(QComboBox::NoInsert);
	m_pEditCLI->setEditable(true);
	m_pEditCLI->lineEdit()->setPlaceholderText("Enter Command");
	m_pEditCLI->lineEdit()->setFocus();


	m_pEditCLI->setCompleter(new QCompleter(this));
	m_pEditCLI->completer()->setModel(new QStandardItemModel(this));

	auto *completerPopup = new QTableView(this);
	completerPopup->setShowGrid(false);
	completerPopup->verticalHeader()->setDefaultSectionSize(fontMetrics().lineSpacing() + 2);
	completerPopup->verticalHeader()->setVisible(false);
	completerPopup->horizontalHeader()->setVisible(false);
	completerPopup->setSelectionBehavior(QTableView::SelectRows);
	completerPopup->setSelectionMode(QTableView::SingleSelection);
	completerPopup->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	completerPopup->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	m_pEditCLI->completer()->setPopup(completerPopup);
	m_pEditCLI->completer()->setCaseSensitivity(Qt::CaseSensitive);
	m_pEditCLI->completer()->setModelSorting(/*QCompleter::CaseSensitivelySortedModel*/ QCompleter::UnsortedModel);
	m_pEditCLI->completer()->setFilterMode(Qt::MatchStartsWith);
	m_pEditCLI->completer()->setCompletionColumn(0);
	m_pEditCLI->completer()->setCompletionMode(QCompleter::PopupCompletion);



	// ------------------------------------------------------------------------
	// layout
	auto *pGrid = new QGridLayout(this);
	pGrid->addWidget(m_pEditHistory, 0, 0, 1, 1);
	pGrid->addWidget(m_pEditCLI, 1, 0, 1, 1);
	// ------------------------------------------------------------------------


	// ------------------------------------------------------------------------
	// connections
	connect(m_pEditCLI->lineEdit(), &QLineEdit::returnPressed, this, &CommandLineWidget::CommandEntered);
	connect(m_pEditCLI->completer(), static_cast<void(QCompleter::*)(const QString&)>(&QCompleter::activated),
		this, &CommandLineWidget::CompleterActivated);
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


void CommandLineWidget::resizeEvent(QResizeEvent *evt)
{
	// resize the completer popup
	auto *completerPopup = static_cast<QTableView*>(m_pEditCLI->completer()->popup());
	completerPopup->setColumnWidth(0, m_pEditCLI->width()/2 - 8);
	completerPopup->setColumnWidth(1, m_pEditCLI->width()/2 - 8);

	if(evt)
		QWidget::resizeEvent(evt);
}


/**
 * update the autocomplete list
 */
void CommandLineWidget::UpdateCompleter()
{
	auto *mod = static_cast<QStandardItemModel*>(m_pEditCLI->completer()->model());
	mod->clear();

	// add user-defined items to list
	for(const auto& str : m_completerItems)
	{
		auto lst = str.split("###");
		QString strItem, strDesc;
		if(lst.size() >= 1) strItem = lst[0];
		if(lst.size() >= 2) strDesc = lst[1];

		auto *item = new QStandardItem(strItem);
		auto *desc = new QStandardItem(strDesc);
		mod->appendRow({item, desc});
	}

	// recent commands stored in combo box
	for(int idx=0; idx<m_pEditCLI->count(); ++idx)
	{
		auto *item = new QStandardItem(m_pEditCLI->itemIcon(idx), m_pEditCLI->itemText(idx));
		auto *desc = new QStandardItem("command history");
		mod->appendRow({item, desc});
	}

	// ensure the correct size of the columns in the popup table
	resizeEvent(nullptr);
}


/**
 * clicked on a completer item
 */
void CommandLineWidget::CompleterActivated(const QString& str)
{
	//std::cout << str.toStdString() << std::endl;
}


void CommandLineWidget::CommandEntered()
{
	QString cmd = m_pEditCLI->currentText().trimmed();
	if(m_pEditCLI->findText(cmd) == -1)
		m_pEditCLI->addItem(cmd);
	m_pEditCLI->clearEditText();
	if(!cmd.length()) return;

	bool bLightTheme = palette().color(QPalette::WindowText).lightnessF() < 0.5;
	QString colInput = bLightTheme ? "#0000ff" : "#ffff00";

	std::string timestamp = tl2::epoch_to_str(tl2::epoch());
	m_pEditHistory->insertHtml("<b><font color=\"#008800\">" + QString(timestamp.c_str()) + "&gt;</font> " +
		"<font color=\"" + colInput + "\">" + cmd + "</font></b><br>");


	// parse command
	std::istringstream istr(cmd.toStdString() + "\n");
	m_parsectx.SetLexerInput(istr);

	// remove the asts for old commands
	m_parsectx.ClearASTs();
	yy::CliParser parser(m_parsectx);
	int parse_state = parser.parse();


	// write error log
	for(const auto& err : m_parsectx.GetErrors())
		PrintOutput(1, err.c_str());
	m_parsectx.ClearErrors();


	if(parse_state != 0)
	{
		PrintOutput(1, "Error: Could not parse command.");
	}
	else
	{
		// evaluate commands
		for(const auto &ast : m_parsectx.GetASTs())
		{
			if(!ast) continue;

			// debug output of AST
			/*std::ostringstream ostrAST;
			ast->Print(ostrAST);
			PrintOutput(0, "<pre>", ostrAST.str(), "</pre>");*/

			auto sym = ast->Eval(m_parsectx);

			// write error log
			for(const auto& err : m_parsectx.GetErrors())
				PrintOutput(1, err.c_str());
			m_parsectx.ClearErrors();

			if(sym)
			{
				std::ostringstream ostrRes;
				sym->print(ostrRes);
				PrintOutput(0, ostrRes.str().c_str());

				// add successful commands to completer
				UpdateCompleter();

				// save last result to workspace
				if(auto *workspace = m_parsectx.GetWorkspace(); workspace)
				{
					workspace->insert_or_assign("__last__", sym);
					m_parsectx.EmitWorkspaceUpdated("__last__");
				}
			}
			else
			{
				PrintOutput(1, "Unable to evaluate expression.");
			}
		}
	}
}


void CommandLineWidget::ScrollToEnd()
{
	// scroll command list to last command
	auto caret = m_pEditHistory->textCursor();
	caret.movePosition(QTextCursor::End, QTextCursor::MoveAnchor, 1);
	m_pEditHistory->setTextCursor(caret);
}


void CommandLineWidget::PrintOutputString(bool is_err, const QString &str)
{
	QString colText = palette().color(QPalette::WindowText).name();

	if(is_err)
		m_pEditHistory->insertHtml("<b><font color=\"#ff0000\">" + str + "</font></b><br>");
	else
		m_pEditHistory->insertHtml("<font color=\"" + colText + "\">" + str + "</font><br>");

	ScrollToEnd();
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
