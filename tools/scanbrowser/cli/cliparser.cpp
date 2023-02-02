/**
 * command line parser
 * @author Tobias Weber <tweber@ill.fr>
 * @date 28-may-18
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

#include "cliparser.h"



// ----------------------------------------------------------------------------
// Lexer
// ----------------------------------------------------------------------------

CliLexer::CliLexer(CliParserContext *ctx)
	: yyFlexLexer(), m_pContext(ctx)
{}

template<> double str_to_real(const std::string& str) { return std::stod(str); }
template<> float str_to_real(const std::string& str) { return std::stof(str); }

void CliLexer::LexerError(const char *err)
{
	if(m_pContext) m_pContext->PrintError(std::string("Lexer error: ") + err + std::string("."));
}
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
// Parser
// ----------------------------------------------------------------------------

void CliParserContext::PrintErrorString(const std::string &err)
{
	m_errors.push_back(err);
	//std::cerr << err << "." << std::endl;
}

void yy::CliParser::error(const std::string &err)
{
	ctx.PrintError(std::string("Parser error: ") + err + std::string("."));
}

void CliParserContext::SetLexerInput(std::istream &istr)
{
	m_lex.switch_streams(&istr/*, &std::cout*/);
}

extern yy::CliParser::symbol_type yylex(CliParserContext &ctx)
{
	return ctx.GetLexer().yylex(ctx);
}
// ----------------------------------------------------------------------------
