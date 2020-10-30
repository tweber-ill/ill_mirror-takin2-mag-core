/**
 * command line parser
 * @author Tobias Weber <tweber@ill.fr>
 * @date 28-may-18
 * @license see 'LICENSE' file
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
