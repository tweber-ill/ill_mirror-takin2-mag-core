/**
 * Evaluates the command AST
 * @author Tobias Weber <tweber@ill.fr>
 * @date 15-Jun-2018
 * @license see 'LICENSE' file
 */

#include "cliparser.h"



// ----------------------------------------------------------------------------
// evaluation of the AST
// ----------------------------------------------------------------------------

std::shared_ptr<Symbol> CliASTReal::Eval() const
{
	return nullptr;
}

std::shared_ptr<Symbol> CliASTString::Eval() const
{
	return nullptr;
}

std::shared_ptr<Symbol> CliASTIdent::Eval() const
{
	return nullptr;
}

std::shared_ptr<Symbol> CliASTAssign::Eval() const
{
	return nullptr;
}

std::shared_ptr<Symbol> CliASTPlus::Eval() const
{
	return nullptr;
}

std::shared_ptr<Symbol> CliASTMinus::Eval() const
{
	return nullptr;
}

std::shared_ptr<Symbol> CliASTMult::Eval() const
{
	return nullptr;
}

std::shared_ptr<Symbol> CliASTDiv::Eval() const
{
	return nullptr;
}

std::shared_ptr<Symbol> CliASTMod::Eval() const
{
	return nullptr;
}

std::shared_ptr<Symbol> CliASTPow::Eval() const
{
	return nullptr;
}

std::shared_ptr<Symbol> CliASTCall::Eval() const
{
	return nullptr;
}

// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
// printing of the AST
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
