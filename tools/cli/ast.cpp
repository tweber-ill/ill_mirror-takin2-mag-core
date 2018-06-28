/**
 * Evaluates the command AST
 * @author Tobias Weber <tweber@ill.fr>
 * @date 15-Jun-2018
 * @license see 'LICENSE' file
 */

#include "cliparser.h"
#include "tools/in20/globals.h"
#include "funcs.h"

#include <cmath>


using t_real = t_real_cli;


// ----------------------------------------------------------------------------
// evaluation of symbols
// ----------------------------------------------------------------------------

/**
 * symbol type name
 */
const std::string& Symbol::get_type_name(const Symbol &sym)
{
	static const std::unordered_map<SymbolType, std::string> map =
	{
		std::make_pair(SymbolType::REAL, "real"),
		std::make_pair(SymbolType::STRING, "string"),
		std::make_pair(SymbolType::LIST, "list"),
		std::make_pair(SymbolType::ARRAY, "array"),
		std::make_pair(SymbolType::DATASET, "dataset"),
	};

	return map.find(sym.GetType())->second;
}


/**
 * unary minus of a symbol
 */
std::shared_ptr<Symbol> Symbol::uminus(const Symbol &sym)
{
	if(sym.GetType()==SymbolType::REAL)
	{ // -1.23
		return std::make_shared<SymbolReal>(-dynamic_cast<const SymbolReal&>(sym).GetValue()); 
	}
	if(sym.GetType()==SymbolType::ARRAY)
	{ // -[1,2,3]
		std::vector<std::shared_ptr<Symbol>> arrNew;
		const auto& arr = dynamic_cast<const SymbolList&>(sym).GetValue();
		for(std::size_t idx=0; idx<arr.size(); ++idx)
			arrNew.emplace_back(Symbol::uminus(*arr[idx]));
		return std::make_shared<SymbolList>(arrNew, false);
	}
	else if(sym.GetType()==SymbolType::DATASET)
	{ // -data
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
	{ // 1.23 + 1.23
		return std::make_shared<SymbolReal>(
			dynamic_cast<const SymbolReal&>(sym1).GetValue() + 
			dynamic_cast<const SymbolReal&>(sym2).GetValue());
	}
	if(sym1.GetType()==SymbolType::ARRAY && sym2.GetType()==SymbolType::ARRAY)
	{ // [1,2,3] + [1,2,3]
		std::vector<std::shared_ptr<Symbol>> arrNew;
		const auto& arr1 = dynamic_cast<const SymbolList&>(sym1).GetValue();
		const auto& arr2 = dynamic_cast<const SymbolList&>(sym2).GetValue();
		for(std::size_t idx=0; idx<std::min(arr1.size(), arr2.size()); ++idx)
			arrNew.emplace_back(Symbol::add(*arr1[idx], *arr2[idx]));
		return std::make_shared<SymbolList>(arrNew, false);
	}
	else if(sym1.GetType()==SymbolType::STRING && sym2.GetType()==SymbolType::STRING)
	{ // "abc" + "123"
		return std::make_shared<SymbolString>(
			dynamic_cast<const SymbolString&>(sym1).GetValue() + 
			dynamic_cast<const SymbolString&>(sym2).GetValue());
	}
	else if(sym1.GetType()==SymbolType::DATASET && sym2.GetType()==SymbolType::DATASET)
	{ // data1 + data2
		return std::make_shared<SymbolDataset>(
			dynamic_cast<const SymbolDataset&>(sym1).GetValue() + 
			dynamic_cast<const SymbolDataset&>(sym2).GetValue());
	}
	else if(sym1.GetType()==SymbolType::STRING && sym2.GetType()==SymbolType::REAL)
	{ // "abc" + 3
		const std::string &str = dynamic_cast<const SymbolString&>(sym1).GetValue();
		t_real val = dynamic_cast<const SymbolReal&>(sym2).GetValue();
		std::ostringstream ostr;
		ostr << str << val;
		return std::make_shared<SymbolString>(ostr.str());
	}
	else if(sym1.GetType()==SymbolType::REAL && sym2.GetType()==SymbolType::STRING)
	{ // 3 + "abc"
		t_real val = dynamic_cast<const SymbolReal&>(sym1).GetValue();
		const std::string &str = dynamic_cast<const SymbolString&>(sym2).GetValue();
		std::ostringstream ostr;
		ostr << val << str;
		return std::make_shared<SymbolString>(ostr.str());
	}

	return nullptr;
}

std::shared_ptr<Symbol> Symbol::add(std::shared_ptr<Symbol> sym1, std::shared_ptr<Symbol> sym2)
{
	return Symbol::add(*sym1, *sym2);
}


/**
 * subtraction of symbols
 */
std::shared_ptr<Symbol> Symbol::sub(const Symbol &sym1, const Symbol &sym2)
{
	if(sym1.GetType()==SymbolType::REAL && sym2.GetType()==SymbolType::REAL)
	{ // 1.23 - 2.34
		return std::make_shared<SymbolReal>(
			dynamic_cast<const SymbolReal&>(sym1).GetValue() - 
			dynamic_cast<const SymbolReal&>(sym2).GetValue());
	}
	if(sym1.GetType()==SymbolType::ARRAY && sym2.GetType()==SymbolType::ARRAY)
	{ // [1,2,3] - [1,2,3]
		std::vector<std::shared_ptr<Symbol>> arrNew;
		const auto& arr1 = dynamic_cast<const SymbolList&>(sym1).GetValue();
		const auto& arr2 = dynamic_cast<const SymbolList&>(sym2).GetValue();
		for(std::size_t idx=0; idx<std::min(arr1.size(), arr2.size()); ++idx)
			arrNew.emplace_back(Symbol::sub(*arr1[idx], *arr2[idx]));
		return std::make_shared<SymbolList>(arrNew, false);
	}
	else if(sym1.GetType()==SymbolType::DATASET && sym2.GetType()==SymbolType::DATASET)
	{ // data1 - data2
		return std::make_shared<SymbolDataset>(
			dynamic_cast<const SymbolDataset&>(sym1).GetValue() - 
			dynamic_cast<const SymbolDataset&>(sym2).GetValue());
	}

	return nullptr;
}

std::shared_ptr<Symbol> Symbol::sub(std::shared_ptr<Symbol> sym1, std::shared_ptr<Symbol> sym2)
{
	return Symbol::sub(*sym1, *sym2);
}


/**
 * multiplication of symbols
 */
std::shared_ptr<Symbol> Symbol::mul(const Symbol &sym1, const Symbol &sym2)
{
	if(sym1.GetType()==SymbolType::REAL && sym2.GetType()==SymbolType::REAL)
	{ // 1.23 * 2.34
		return std::make_shared<SymbolReal>(
			dynamic_cast<const SymbolReal&>(sym1).GetValue() * 
			dynamic_cast<const SymbolReal&>(sym2).GetValue());
	}
	else if(sym1.GetType()==SymbolType::REAL && sym2.GetType()==SymbolType::STRING)
	{ // 5 * "123"
		const std::string &str = dynamic_cast<const SymbolString&>(sym2).GetValue();
		std::string strOut;

		std::size_t n = static_cast<std::size_t>(dynamic_cast<const SymbolReal&>(sym1).GetValue());
		for(std::size_t i=0; i<n; ++i)
			strOut += str;

		return std::make_shared<SymbolString>(strOut);
	}
	else if(sym1.GetType()==SymbolType::STRING && sym2.GetType()==SymbolType::REAL)
	{ // 5 * "123"
		const std::string &str = dynamic_cast<const SymbolString&>(sym1).GetValue();
		std::string strOut;

		std::size_t n = static_cast<std::size_t>(dynamic_cast<const SymbolReal&>(sym2).GetValue());
		for(std::size_t i=0; i<n; ++i)
			strOut += str;

		return std::make_shared<SymbolString>(strOut);
	}
	if(sym1.GetType()==SymbolType::REAL && sym2.GetType()==SymbolType::ARRAY)
	{ // 5 * [1,2,3]
		std::vector<std::shared_ptr<Symbol>> arrNew;
		const auto& arr = dynamic_cast<const SymbolList&>(sym2).GetValue();
		for(std::size_t idx=0; idx<arr.size(); ++idx)
			arrNew.emplace_back(Symbol::mul(sym1, *arr[idx]));
		return std::make_shared<SymbolList>(arrNew, false);
	}
	if(sym1.GetType()==SymbolType::ARRAY && sym2.GetType()==SymbolType::REAL)
	{ // [1,2,3] * 5
		std::vector<std::shared_ptr<Symbol>> arrNew;
		const auto& arr = dynamic_cast<const SymbolList&>(sym1).GetValue();
		for(std::size_t idx=0; idx<arr.size(); ++idx)
			arrNew.emplace_back(Symbol::mul(*arr[idx], sym2));
		return std::make_shared<SymbolList>(arrNew, false);
	}
	if(sym1.GetType()==SymbolType::ARRAY && sym2.GetType()==SymbolType::ARRAY)
	{ // [1,2,3] * [1,2,3]
		return func_dot(std::make_shared<SymbolList>(dynamic_cast<const SymbolList&>(sym1)),
			std::make_shared<SymbolList>(dynamic_cast<const SymbolList&>(sym2)));
	}
	else if(sym1.GetType()==SymbolType::REAL && sym2.GetType()==SymbolType::DATASET)
	{ // 3 * data
		return std::make_shared<SymbolDataset>(
			dynamic_cast<const SymbolReal&>(sym1).GetValue() * 
			dynamic_cast<const SymbolDataset&>(sym2).GetValue());
	}
	else if(sym1.GetType()==SymbolType::DATASET && sym2.GetType()==SymbolType::REAL)
	{ // data * 3
		return std::make_shared<SymbolDataset>(
			dynamic_cast<const SymbolDataset&>(sym1).GetValue() * 
			dynamic_cast<const SymbolReal&>(sym2).GetValue());
	}

	return nullptr;
}

std::shared_ptr<Symbol> Symbol::mul(std::shared_ptr<Symbol> sym1, std::shared_ptr<Symbol> sym2)
{
	return Symbol::mul(*sym1, *sym2);
}


/**
 * division of symbols
 */
std::shared_ptr<Symbol> Symbol::div(const Symbol &sym1, const Symbol &sym2)
{
	if(sym1.GetType()==SymbolType::REAL && sym2.GetType()==SymbolType::REAL)
	{ // 1.23 / 5.67
		return std::make_shared<SymbolReal>(
			dynamic_cast<const SymbolReal&>(sym1).GetValue() / 
			dynamic_cast<const SymbolReal&>(sym2).GetValue());
	}
	if(sym1.GetType()==SymbolType::ARRAY && sym2.GetType()==SymbolType::REAL)
	{ // [1,2,3] / 5
		std::vector<std::shared_ptr<Symbol>> arrNew;
		const auto& arr = dynamic_cast<const SymbolList&>(sym1).GetValue();
		for(std::size_t idx=0; idx<arr.size(); ++idx)
			arrNew.emplace_back(Symbol::div(*arr[idx], sym2));
		return std::make_shared<SymbolList>(arrNew, false);
	}
	else if(sym1.GetType()==SymbolType::DATASET && sym2.GetType()==SymbolType::REAL)
	{ // data / 5.
		return std::make_shared<SymbolDataset>(
			dynamic_cast<const SymbolDataset&>(sym1).GetValue() / 
			dynamic_cast<const SymbolReal&>(sym2).GetValue());
	}

	return nullptr;
}

std::shared_ptr<Symbol> Symbol::div(std::shared_ptr<Symbol> sym1, std::shared_ptr<Symbol> sym2)
{
	return Symbol::div(*sym1, *sym2);
}


/**
 * modulo of symbols
 */
std::shared_ptr<Symbol> Symbol::mod(const Symbol &sym1, const Symbol &sym2)
{
	if(sym1.GetType()==SymbolType::REAL && sym2.GetType()==SymbolType::REAL)
	{ // 12. % 5.
		return std::make_shared<SymbolReal>(std::fmod(
			dynamic_cast<const SymbolReal&>(sym1).GetValue(),
			dynamic_cast<const SymbolReal&>(sym2).GetValue()));
	}
	if(sym1.GetType()==SymbolType::ARRAY && sym2.GetType()==SymbolType::REAL)
	{ // [1,2,3] % 5
		std::vector<std::shared_ptr<Symbol>> arrNew;
		const auto& arr = dynamic_cast<const SymbolList&>(sym1).GetValue();
		for(std::size_t idx=0; idx<arr.size(); ++idx)
			arrNew.emplace_back(Symbol::mod(*arr[idx], sym2));
		return std::make_shared<SymbolList>(arrNew, false);
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
 * recursively evaluate a list and collect symbols into a vector
 */
static std::vector<std::shared_ptr<Symbol>> 
list_eval(CliParserContext& ctx, std::shared_ptr<CliAST> left, std::shared_ptr<CliAST> right)
{
	std::vector<std::shared_ptr<Symbol>> vec;

	if(left)
	{
		if(auto lefteval = left->Eval(ctx); lefteval)
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
		}
	}

	if(right)
	{
		if(auto righteval = right->Eval(ctx); righteval)
		{
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
	}

	return vec;
}


/**
 * array, e.g. [1, 2, 3, 4]
 * an array is composed of a list: [ list ]
 */
std::shared_ptr<Symbol> CliASTArray::Eval(CliParserContext& ctx) const
{
	if(!m_left && !m_right)
		return nullptr;

	// transform the tree into a flat vector
	auto vec = list_eval(ctx, m_left, m_right);
	return std::make_shared<SymbolList>(vec, false);
}


/**
 * variable identifier in symbol or constants map
 */
std::shared_ptr<Symbol> CliASTIdent::Eval(CliParserContext& ctx) const
{
	// look in constants map
	auto iterConst = g_consts_real.find(m_val);

	// is workspace available?
	auto *workspace = ctx.GetWorkspace();
	if(!workspace)
	{
		ctx.PrintError("No workspace linked to parser.");
		return nullptr;
	}

	// look in workspace variables map
	auto iter = workspace->find(m_val);
	if(iter == workspace->end() && iterConst == g_consts_real.end())
	{
		ctx.PrintError("Identifier \"", m_val, "\" names neither a constant nor a workspace variable.");
		return nullptr;
	}
	else if(iter != workspace->end() && iterConst != g_consts_real.end())
	{
		ctx.PrintError("Identifier \"", m_val, "\" names both a constant and a workspace variable, using constant.");
		return std::make_shared<SymbolReal>(iterConst->second);
	}
	else if(iter != workspace->end())
		return iter->second;
	else if(iterConst != g_consts_real.end())
		return std::make_shared<SymbolReal>(iterConst->second);

	return nullptr;
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
	if(m_left->GetType() != CliASTType::IDENT /*&& m_left->GetType() != CliASTType::STRING*/)
	{
		ctx.PrintError("Left-hand side of assignment has to be an identifier.");
		return nullptr;
	}


	// get identifier to be assigned
	std::string ident;
	if(m_left->GetType() == CliASTType::IDENT)
		ident = dynamic_cast<CliASTIdent&>(*m_left).GetValue();
	//else if(m_left->GetType() == CliASTType::STRING)
	//	ident = dynamic_cast<CliASTString&>(*m_left).GetValue();
	//	// TODO: Check if string is also a valid identifier!


	// is this variable already in the constants map?
	auto iterConst = g_consts_real.find(ident);
	if(iterConst != g_consts_real.end())
	{
		ctx.PrintError("Identifier \"", ident, "\" cannot be re-assigned, it names an internal constant.");
		return nullptr;
	}


	// assign variable
	if(auto righteval=m_right->Eval(ctx); righteval)
	{
		const auto [iter, inserted] =
			workspace->insert_or_assign(ident, righteval);
		if(!inserted)
			print_out("Variable \"", ident, "\" was overwritten.");

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
	// function name
	std::string ident;
	if(m_left->GetType() == CliASTType::IDENT)
	{
		ident = dynamic_cast<CliASTIdent&>(*m_left).GetValue();
	}
	else
	{
		ctx.PrintError("Left-hand side of function call has to be an identifier.");
		return nullptr;
	}


	// arguments
	std::vector<std::shared_ptr<Symbol>> args;

	// at least one function argument was given
	if(m_right)
	{
		if(auto righteval = m_right->Eval(ctx); righteval)
		{
			if(righteval->GetType() == SymbolType::LIST)
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


	if(args.size() == 1)	// function call with one argument requested
	{
		if(auto iter = g_funcs_gen_1arg.find(ident); iter != g_funcs_gen_1arg.end())
		{	// general function
			return (*iter->second)(args[0]);
		}
		else if(auto iter = g_funcs_real_1arg.find(ident);
			iter != g_funcs_real_1arg.end() && args[0]->GetType() == SymbolType::REAL)
		{	// real function
			t_real funcval = (*iter->second)(dynamic_cast<SymbolReal&>(*args[0]).GetValue());
			return std::make_shared<SymbolReal>(funcval);
		}
		else if(auto iter = g_funcs_arr_1arg.find(ident);
			iter != g_funcs_arr_1arg.end() && args[0]->GetType() == SymbolType::ARRAY)
		{	// array function
			return (*iter->second)(*reinterpret_cast<std::shared_ptr<SymbolList>*>(&args[0]));
		}
		else
		{
			ctx.PrintError("No suitable one-argument function \"", ident, "\" was found.");
			return nullptr;
		}
	}
	else if(args.size() == 2)	// function call with two arguments requested
	{
		if(auto iter = g_funcs_gen_2args.find(ident); iter != g_funcs_gen_2args.end())
		{	// general function
			return (*iter->second)(args[0], args[1]);
		}
		else if(auto iter = g_funcs_real_2args.find(ident); 
			iter != g_funcs_real_2args.end() &&
			args[0]->GetType() == SymbolType::REAL && args[1]->GetType() == SymbolType::REAL)
		{ // real function
			t_real arg1 = dynamic_cast<SymbolReal&>(*args[0]).GetValue();
			t_real arg2 = dynamic_cast<SymbolReal&>(*args[1]).GetValue();
			t_real funcval = (*iter->second)(arg1, arg2);
			return std::make_shared<SymbolReal>(funcval);
		}
		else if(auto iter = g_funcs_arr_2args.find(ident);
			iter != g_funcs_arr_2args.end()
			&& args[0]->GetType() == SymbolType::ARRAY && args[1]->GetType() == SymbolType::ARRAY)
		{	// array function
			return (*iter->second)(*reinterpret_cast<std::shared_ptr<SymbolList>*>(&args[0]),
				*reinterpret_cast<std::shared_ptr<SymbolList>*>(&args[1]));
		}
		else
		{
			ctx.PrintError("No suitable two-argument function \"", ident, "\" was found.");
			return nullptr;
		}
	}

	ctx.PrintError("No suitable ", args.size(), "-argument function \"", ident, "\" was found.");
	return nullptr;
}


/**
 * list of expressions: e.g. 1,2,3,4
 */
std::shared_ptr<Symbol> CliASTExprList::Eval(CliParserContext& ctx) const
{
	if(!m_left && !m_right)
		return nullptr;

	// transform the tree into a flat vector
	auto vec = list_eval(ctx, m_left, m_right);
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

void CliASTArray::Print(std::ostringstream &ostr, int indent) const
{
	for(int i=0; i<indent; ++i) ostr << "\t";
	ostr << "op: array\n";
	CliAST::Print(ostr, indent);
}
// ----------------------------------------------------------------------------
