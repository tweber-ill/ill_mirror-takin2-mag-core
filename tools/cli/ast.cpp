/**
 * Evaluates the command AST
 * @author Tobias Weber <tweber@ill.fr>
 * @date 15-Jun-2018
 * @license see 'LICENSE' file
 */

#include "cliparser.h"
#include "tools/in20/globals.h"
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
	else if(sym.GetType()==SymbolType::DATASET)
	{
		return std::make_shared<SymbolDataset>(-dynamic_cast<const SymbolDataset&>(sym).GetValue()); 
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
	else if(sym1.GetType()==SymbolType::DATASET && sym2.GetType()==SymbolType::DATASET)
	{
		return std::make_shared<SymbolDataset>(
			dynamic_cast<const SymbolDataset&>(sym1).GetValue() + 
			dynamic_cast<const SymbolDataset&>(sym2).GetValue());
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
	else if(sym1.GetType()==SymbolType::DATASET && sym2.GetType()==SymbolType::DATASET)
	{
		return std::make_shared<SymbolDataset>(
			dynamic_cast<const SymbolDataset&>(sym1).GetValue() - 
			dynamic_cast<const SymbolDataset&>(sym2).GetValue());
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
	else if(sym1.GetType()==SymbolType::REAL && sym2.GetType()==SymbolType::DATASET)
	{
		return std::make_shared<SymbolDataset>(
			dynamic_cast<const SymbolReal&>(sym1).GetValue() * 
			dynamic_cast<const SymbolDataset&>(sym2).GetValue());
	}
	else if(sym1.GetType()==SymbolType::DATASET && sym2.GetType()==SymbolType::REAL)
	{
		return std::make_shared<SymbolDataset>(
			dynamic_cast<const SymbolDataset&>(sym1).GetValue() * 
			dynamic_cast<const SymbolReal&>(sym2).GetValue());
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
	else if(sym1.GetType()==SymbolType::DATASET && sym2.GetType()==SymbolType::REAL)
	{
		return std::make_shared<SymbolDataset>(
			dynamic_cast<const SymbolDataset&>(sym1).GetValue() / 
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

/**
 * real constant
 */
std::shared_ptr<Symbol> CliASTReal::Eval(CliParserContext& ctx) const
{
	return std::make_shared<SymbolReal>(m_val);
}


/**
 * string constant
 */
std::shared_ptr<Symbol> CliASTString::Eval(CliParserContext& ctx) const
{
	return std::make_shared<SymbolString>(m_val);
}


/**
 * variable identifier in symbol map
 */
std::shared_ptr<Symbol> CliASTIdent::Eval(CliParserContext& ctx) const
{
	auto *workspace = ctx.GetWorkspace();
	if(!workspace)
	{
		ctx.PrintError("No workspace linked to parser.");
		return nullptr;
	}

	auto iter = workspace->find(m_val);
	if(iter == workspace->end())
	{
		ctx.PrintError("Variable \"", m_val, "\" is not in workspace.");
		return nullptr;
	}

	return iter->second;
}


/**
 * assignment operation
 */
std::shared_ptr<Symbol> CliASTAssign::Eval(CliParserContext& ctx) const
{
	auto *workspace = ctx.GetWorkspace();
	if(!workspace)
	{
		ctx.PrintError("No workspace linked to parser.");
		return nullptr;
	}

	if(!m_left || !m_right)
		return nullptr;
	if(m_left->GetType() != CliASTType::IDENT && m_left->GetType() != CliASTType::STRING)
	{
		ctx.PrintError("Left-hand side of assignment has to be an identifier.");
		return nullptr;
	}

	if(auto righteval=m_right->Eval(ctx); righteval)
	{
		std::string ident;
		if(m_left->GetType() == CliASTType::IDENT)
			ident = dynamic_cast<CliASTIdent&>(*m_left).GetValue();
		else if(m_left->GetType() == CliASTType::STRING)
			ident = dynamic_cast<CliASTString&>(*m_left).GetValue();
			// TODO: Check if string is also a valid identifier!

		const auto [iter, insert_ok] =
			workspace->emplace(std::make_pair(ident, righteval));

		ctx.EmitWorkspaceUpdated(ident);
		return iter->second;
	}

	return nullptr;
}


/**
 * addition
 */
std::shared_ptr<Symbol> CliASTPlus::Eval(CliParserContext& ctx) const
{
	if(!m_left || !m_right)
		return nullptr;
	
	if(auto lefteval=m_left->Eval(ctx), righteval=m_right->Eval(ctx); lefteval && righteval)
		return Symbol::add(*lefteval, *righteval);

	return nullptr;
}


/**
 * subtraction
 */
std::shared_ptr<Symbol> CliASTMinus::Eval(CliParserContext& ctx) const
{
	if(m_left && m_right)
	{
		if(auto lefteval=m_left->Eval(ctx), righteval=m_right->Eval(ctx); lefteval && righteval)
			return Symbol::sub(*lefteval, *righteval);
	}
	else if(m_right && !m_left)
	{
		if(auto righteval=m_right->Eval(ctx); righteval)
			return Symbol::uminus(*righteval);
		return Symbol::uminus(*m_right->Eval(ctx));
	}
	return nullptr;
}


/**
 * multiplication
 */
std::shared_ptr<Symbol> CliASTMult::Eval(CliParserContext& ctx) const
{
	if(!m_left || !m_right)
		return nullptr;
	
	if(auto lefteval=m_left->Eval(ctx), righteval=m_right->Eval(ctx); lefteval && righteval)
		return Symbol::mul(*lefteval, *righteval);

	return nullptr;
}


/**
 * division
 */
std::shared_ptr<Symbol> CliASTDiv::Eval(CliParserContext& ctx) const
{
	if(!m_left || !m_right)
		return nullptr;
	
	if(auto lefteval=m_left->Eval(ctx), righteval=m_right->Eval(ctx); lefteval && righteval)
		return Symbol::div(*lefteval, *righteval);

	return nullptr;
}


/**
 * modulo
 */
std::shared_ptr<Symbol> CliASTMod::Eval(CliParserContext& ctx) const
{
	if(!m_left || !m_right)
		return nullptr;
	
	if(auto lefteval=m_left->Eval(ctx), righteval=m_right->Eval(ctx); lefteval && righteval)
		return Symbol::mod(*lefteval, *righteval);

	return nullptr;
}


/**
 * power
 */
std::shared_ptr<Symbol> CliASTPow::Eval(CliParserContext& ctx) const
{
	if(!m_left || !m_right)
		return nullptr;
	
	if(auto lefteval=m_left->Eval(ctx), righteval=m_right->Eval(ctx); lefteval && righteval)
		return Symbol::pow(*lefteval, *righteval);

	return nullptr;
}


/**
 * function call operation
 */
std::shared_ptr<Symbol> CliASTCall::Eval(CliParserContext& ctx) const
{
	// arguments
	std::vector<std::shared_ptr<Symbol>> args;

	// at least one function argument was given
	if(m_right)
	{
		if(auto righteval=m_right->Eval(ctx); righteval)
		{
			if(righteval->GetType()==SymbolType::LIST)
			{
				// two or more arguments are collected in a list
				auto &rightvec = dynamic_cast<SymbolList&>(*righteval).GetValue();
				for(auto &sym : rightvec)
					args.emplace_back(sym);
			}
			else
			{
				// one argument
				args.emplace_back(righteval);
			}
		}
	}

	return nullptr;
}


/**
 * list of expressions
 */
std::shared_ptr<Symbol> CliASTExprList::Eval(CliParserContext& ctx) const
{
	if(!m_left || !m_right)
		return nullptr;

	// transform the tree into a flat vector
	std::vector<std::shared_ptr<Symbol>> vec;

	if(auto lefteval=m_left->Eval(ctx), righteval=m_right->Eval(ctx); lefteval && righteval)
	{
		// lhs of AST
		if(lefteval->GetType() == SymbolType::LIST)
		{
			auto &leftvec = dynamic_cast<SymbolList&>(*lefteval).GetValue();
			for(auto& val : leftvec)
				vec.emplace_back(val);
		}
		else
		{
			vec.emplace_back(lefteval);
		}

		// rhs of AST
		if(righteval->GetType() == SymbolType::LIST)
		{
			auto &rightvec = dynamic_cast<SymbolList&>(*righteval).GetValue();
			for(auto& val : rightvec)
				vec.emplace_back(val);
		}
		else
		{
			vec.emplace_back(righteval);
		}
	}

	return std::make_shared<SymbolList>(vec);
}

// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
// printing of the AST
// ----------------------------------------------------------------------------

void CliAST::Print(std::ostringstream &ostr, int indent) const
{
	if(m_left) m_left->Print(ostr, indent+1);
	if(m_right) m_right->Print(ostr, indent+1);
}

void CliASTReal::Print(std::ostringstream &ostr, int indent) const
{
	for(int i=0; i<indent; ++i) ostr << "\t";
	ostr << "real: " << m_val << "\n";
}

void CliASTString::Print(std::ostringstream &ostr, int indent) const
{
	for(int i=0; i<indent; ++i) ostr << "\t";
	ostr << "string: " << m_val << "\n";
}

void CliASTIdent::Print(std::ostringstream &ostr, int indent) const
{
	for(int i=0; i<indent; ++i) ostr << "\t";
	ostr << "ident: " << m_val << "\n";
}

void CliASTAssign::Print(std::ostringstream &ostr, int indent) const
{
	for(int i=0; i<indent; ++i) ostr << "\t";
	ostr << "op: assign\n";
	CliAST::Print(ostr, indent);
}

void CliASTPlus::Print(std::ostringstream &ostr, int indent) const
{
	for(int i=0; i<indent; ++i) ostr << "\t";
	ostr << "op: +\n";
	CliAST::Print(ostr, indent);
}

void CliASTMinus::Print(std::ostringstream &ostr, int indent) const
{
	for(int i=0; i<indent; ++i) ostr << "\t";
	ostr << "op: -\n";
	CliAST::Print(ostr, indent);
}

void CliASTMult::Print(std::ostringstream &ostr, int indent) const
{
	for(int i=0; i<indent; ++i) ostr << "\t";
	ostr << "op: *\n";
	CliAST::Print(ostr, indent);
}

void CliASTDiv::Print(std::ostringstream &ostr, int indent) const
{
	for(int i=0; i<indent; ++i) ostr << "\t";
	ostr << "op: /\n";
	CliAST::Print(ostr, indent);
}

void CliASTMod::Print(std::ostringstream &ostr, int indent) const
{
	for(int i=0; i<indent; ++i) ostr << "\t";
	ostr << "op: %\n";
	CliAST::Print(ostr, indent);
}

void CliASTPow::Print(std::ostringstream &ostr, int indent) const
{
	for(int i=0; i<indent; ++i) ostr << "\t";
	ostr << "op: ^\n";
	CliAST::Print(ostr, indent);
}

void CliASTCall::Print(std::ostringstream &ostr, int indent) const
{
	for(int i=0; i<indent; ++i) ostr << "\t";
	ostr << "op: call\n";
	CliAST::Print(ostr, indent);
}

void CliASTExprList::Print(std::ostringstream &ostr, int indent) const
{
	for(int i=0; i<indent; ++i) ostr << "\t";
	ostr << "op: expr_list\n";
	CliAST::Print(ostr, indent);
}
// ----------------------------------------------------------------------------
