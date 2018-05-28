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

CliLexer::CliLexer(std::istream &istr, std::ostream &ostr)
	: yyFlexLexer(istr, ostr)
{}

template<> double str_to_real(const std::string& str) { return std::stod(str); }
template<> float str_to_real(const std::string& str) { return std::stof(str); }
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
// Parser
// ----------------------------------------------------------------------------

void CliParserContext::PrintError(const std::string &err)
{
	std::cerr << err << "." << std::endl;
}

void yy::CliParser::error(const std::string &err)
{
	context.PrintError(std::string("Parser error: ") + err);
}

extern yy::CliParser::symbol_type yylex(CliParserContext &context)
{
	return context.GetLexer().yylex(context);
}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// AST
// ----------------------------------------------------------------------------
void CliAST::Print(int indent) const
{
	if(m_left) m_left->Print(indent+1);
	if(m_right) m_right->Print(indent+1);
}

void CliASTReal::Print(int indent) const
{
	for(int i=0; i<indent; ++i) std::cout << "\t";
	std::cout << "real: " << m_val << "\n";
}

void CliASTString::Print(int indent) const
{
	for(int i=0; i<indent; ++i) std::cout << "\t";
	std::cout << "string: " << m_val << "\n";
}

void CliASTIdent::Print(int indent) const
{
	for(int i=0; i<indent; ++i) std::cout << "\t";
	std::cout << "ident: " << m_val << "\n";
}

void CliASTAssign::Print(int indent) const
{
	for(int i=0; i<indent; ++i) std::cout << "\t";
	std::cout << "op: assign\n";
	CliAST::Print(indent);
}

void CliASTPlus::Print(int indent) const
{
	for(int i=0; i<indent; ++i) std::cout << "\t";
	std::cout << "op: +\n";
	CliAST::Print(indent);
}

void CliASTMinus::Print(int indent) const
{
	for(int i=0; i<indent; ++i) std::cout << "\t";
	std::cout << "op: -\n";
	CliAST::Print(indent);
}

void CliASTMult::Print(int indent) const
{
	for(int i=0; i<indent; ++i) std::cout << "\t";
	std::cout << "op: *\n";
	CliAST::Print(indent);
}

void CliASTDiv::Print(int indent) const
{
	for(int i=0; i<indent; ++i) std::cout << "\t";
	std::cout << "op: /\n";
	CliAST::Print(indent);
}

void CliASTMod::Print(int indent) const
{
	for(int i=0; i<indent; ++i) std::cout << "\t";
	std::cout << "op: %\n";
	CliAST::Print(indent);
}

void CliASTPow::Print(int indent) const
{
	for(int i=0; i<indent; ++i) std::cout << "\t";
	std::cout << "op: ^\n";
	CliAST::Print(indent);
}

void CliASTCall::Print(int indent) const
{
	for(int i=0; i<indent; ++i) std::cout << "\t";
	std::cout << "op: call\n";
	CliAST::Print(indent);
}
// ----------------------------------------------------------------------------



int main()
{
	CliParserContext ctx;
	yy::CliParser parser(ctx);
	return parser.parse();
}
