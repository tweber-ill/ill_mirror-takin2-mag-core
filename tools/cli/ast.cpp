/**
 * Evaluates the command AST
 * @author Tobias Weber <tweber@ill.fr>
 * @date 15-Jun-2018
 * @license see 'LICENSE' file
 */

#include "cliparser.h"
#include <cmath>



// ----------------------------------------------------------------------------
// evaluation of symbols
// ----------------------------------------------------------------------------

/**
 * unary minus of a symbol
 */
std::shared_ptr<Symbol> Symbol::uminus(const Symbol &sym)
{
	if(sym.GetType()==SymbolType::REAL)
	{
		return std::make_shared<SymbolReal>(-dynamic_cast<const SymbolReal&>(sym).GetValue()); 
	}

	return nullptr;
}


/**
 * addition of symbols
 */
std::shared_ptr<Symbol> Symbol::add(const Symbol &sym1, const Symbol &sym2)
{
	if(sym1.GetType()==SymbolType::REAL && sym2.GetType()==SymbolType::REAL)
	{
		return std::make_shared<SymbolReal>(
			dynamic_cast<const SymbolReal&>(sym1).GetValue() + 
			dynamic_cast<const SymbolReal&>(sym2).GetValue());
	}
	else if(sym1.GetType()==SymbolType::STRING && sym2.GetType()==SymbolType::STRING)
	{
		return std::make_shared<SymbolString>(
			dynamic_cast<const SymbolString&>(sym1).GetValue() + 
			dynamic_cast<const SymbolString&>(sym2).GetValue());
	}
	else if(sym1.GetType()==SymbolType::STRING && sym2.GetType()==SymbolType::REAL)
	{
		const std::string &str = dynamic_cast<const SymbolString&>(sym1).GetValue();
		t_real val = dynamic_cast<const SymbolReal&>(sym2).GetValue();
		std::ostringstream ostr;
		ostr << str << val;
		return std::make_shared<SymbolString>(ostr.str());
	}
	else if(sym1.GetType()==SymbolType::REAL && sym2.GetType()==SymbolType::STRING)
	{
		t_real val = dynamic_cast<const SymbolReal&>(sym1).GetValue();
		const std::string &str = dynamic_cast<const SymbolString&>(sym2).GetValue();
		std::ostringstream ostr;
		ostr << val << str;
		return std::make_shared<SymbolString>(ostr.str());
	}

	return nullptr;
}


/**
 * subtraction of symbols
 */
std::shared_ptr<Symbol> Symbol::sub(const Symbol &sym1, const Symbol &sym2)
{
	if(sym1.GetType()==SymbolType::REAL && sym2.GetType()==SymbolType::REAL)
	{
		return std::make_shared<SymbolReal>(
			dynamic_cast<const SymbolReal&>(sym1).GetValue() - 
			dynamic_cast<const SymbolReal&>(sym2).GetValue());
	}

	return nullptr;
}


/**
 * multiplication of symbols
 */
std::shared_ptr<Symbol> Symbol::mul(const Symbol &sym1, const Symbol &sym2)
{
	if(sym1.GetType()==SymbolType::REAL && sym2.GetType()==SymbolType::REAL)
	{
		return std::make_shared<SymbolReal>(
			dynamic_cast<const SymbolReal&>(sym1).GetValue() * 
			dynamic_cast<const SymbolReal&>(sym2).GetValue());
	}
	else if(sym1.GetType()==SymbolType::REAL && sym2.GetType()==SymbolType::STRING)
	{
		const std::string &str = dynamic_cast<const SymbolString&>(sym2).GetValue();
		std::string strOut;

		std::size_t n = static_cast<std::size_t>(dynamic_cast<const SymbolReal&>(sym1).GetValue());
		for(std::size_t i=0; i<n; ++i)
			strOut += str;

		return std::make_shared<SymbolString>(strOut);
	}
	else if(sym1.GetType()==SymbolType::STRING && sym2.GetType()==SymbolType::REAL)
	{
		const std::string &str = dynamic_cast<const SymbolString&>(sym1).GetValue();
		std::string strOut;

		std::size_t n = static_cast<std::size_t>(dynamic_cast<const SymbolReal&>(sym2).GetValue());
		for(std::size_t i=0; i<n; ++i)
			strOut += str;

		return std::make_shared<SymbolString>(strOut);
	}

	return nullptr;
}


/**
 * division of symbols
 */
std::shared_ptr<Symbol> Symbol::div(const Symbol &sym1, const Symbol &sym2)
{
	if(sym1.GetType()==SymbolType::REAL && sym2.GetType()==SymbolType::REAL)
	{
		return std::make_shared<SymbolReal>(
			dynamic_cast<const SymbolReal&>(sym1).GetValue() / 
			dynamic_cast<const SymbolReal&>(sym2).GetValue());
	}

	return nullptr;
}


/**
 * modulo of symbols
 */
std::shared_ptr<Symbol> Symbol::mod(const Symbol &sym1, const Symbol &sym2)
{
	if(sym1.GetType()==SymbolType::REAL && sym2.GetType()==SymbolType::REAL)
	{
		return std::make_shared<SymbolReal>(std::fmod(
			dynamic_cast<const SymbolReal&>(sym1).GetValue(),
			dynamic_cast<const SymbolReal&>(sym2).GetValue()));
	}

	return nullptr;
}


/**
 * power of symbols
 */
std::shared_ptr<Symbol> Symbol::pow(const Symbol &sym1, const Symbol &sym2)
{
	if(sym1.GetType()==SymbolType::REAL && sym2.GetType()==SymbolType::REAL)
	{
		return std::make_shared<SymbolReal>(std::pow(
			dynamic_cast<const SymbolReal&>(sym1).GetValue(),
			dynamic_cast<const SymbolReal&>(sym2).GetValue()));
	}

	return nullptr;
}

// ----------------------------------------------------------------------------





// ----------------------------------------------------------------------------
// evaluation of the AST
// ----------------------------------------------------------------------------

std::shared_ptr<Symbol> CliASTReal::Eval() const
{
	return std::make_shared<SymbolReal>(m_val);
}

std::shared_ptr<Symbol> CliASTString::Eval() const
{
	return std::make_shared<SymbolString>(m_val);
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
	if(m_left && m_right)
		return Symbol::add(*m_left->Eval(), *m_right->Eval());
	return nullptr;
}

std::shared_ptr<Symbol> CliASTMinus::Eval() const
{
	if(m_left && m_right)
		return Symbol::sub(*m_left->Eval(), *m_right->Eval());
	else if(m_right && !m_left)
		return Symbol::uminus(*m_right->Eval());
	return nullptr;
}

std::shared_ptr<Symbol> CliASTMult::Eval() const
{
	if(m_left && m_right)
		return Symbol::mul(*m_left->Eval(), *m_right->Eval());
	return nullptr;
}

std::shared_ptr<Symbol> CliASTDiv::Eval() const
{
	if(m_left && m_right)
		return Symbol::div(*m_left->Eval(), *m_right->Eval());
	return nullptr;
}

std::shared_ptr<Symbol> CliASTMod::Eval() const
{
	if(m_left && m_right)
		return Symbol::mod(*m_left->Eval(), *m_right->Eval());
	return nullptr;
}

std::shared_ptr<Symbol> CliASTPow::Eval() const
{
	if(m_left && m_right)
		return Symbol::pow(*m_left->Eval(), *m_right->Eval());
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
