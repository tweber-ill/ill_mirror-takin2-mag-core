/**
 * Evaluates the command AST
 * @author Tobias Weber <tweber@ill.fr>
 * @date 15-Jun-2018
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
#include "../globals.h"
#include "funcs.h"

#include <cmath>


// ----------------------------------------------------------------------------
// evaluation of the AST
// ----------------------------------------------------------------------------

/**
 * real constant
 */
std::shared_ptr<Symbol> CliASTReal::Eval(CliParserContext&) const
{
	return std::make_shared<SymbolReal>(m_val);
}


/**
 * string constant
 */
std::shared_ptr<Symbol> CliASTString::Eval(CliParserContext&) const
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
 * array element access, e.g. a[5]
 */
std::shared_ptr<Symbol> CliASTArrayAccess::Eval(CliParserContext& ctx) const
{
	if(!m_left && !m_right)
		return nullptr;

	if(auto lefteval=m_left->Eval(ctx), righteval=m_right->Eval(ctx); lefteval && righteval)
	{
		if(righteval->GetType() != SymbolType::REAL)
		{
			ctx.PrintError("Array index has to be of scalar type.");
			return nullptr;
		}

		// index
		std::size_t idx = std::size_t(dynamic_cast<SymbolReal&>(*righteval).GetValue());


		if(lefteval->GetType() == SymbolType::ARRAY)
		{ // get array elements
			const auto& arr = dynamic_cast<SymbolList&>(*lefteval).GetValue();
			if(idx >= arr.size())
			{
				ctx.PrintError("Array index is out of bounds.");
				return nullptr;
			}

			return arr[idx]->copy();
		}
		else if(lefteval->GetType() == SymbolType::DATASET)
		{ // get channels of a dataset
			const auto& dataset = dynamic_cast<SymbolDataset&>(*lefteval).GetValue();
			if(idx >= dataset.GetNumChannels())
			{
				ctx.PrintError("Dataset channel index is out of bounds.");
				return nullptr;
			}

			Dataset newdataset;
			Data dat = dataset.GetChannel(idx);
			newdataset.AddChannel(std::move(dat));

			return std::make_shared<SymbolDataset>(newdataset);
		}
		else
		{
			ctx.PrintError("Variables of type ", Symbol::get_type_name(*lefteval),
				" do not support element access.");
			return nullptr;
		}
	}

	ctx.PrintError("Invalid element access request.");
	return nullptr;
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
		return std::make_shared<SymbolReal>(std::get<0>(iterConst->second));
	}
	else if(iter != workspace->end())
		return iter->second;
	else if(iterConst != g_consts_real.end())
		return std::make_shared<SymbolReal>(std::get<0>(iterConst->second));

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


	// general case: function call with a variable number of arguments
	{
		if(auto iter = g_funcs_gen_vararg.find(ident); iter != g_funcs_gen_vararg.end())
		{	// general function
			return (*std::get<0>(iter->second))(ctx, args);
		}
	}


	// special cases
	if(args.size() == 0)	// function call with no arguments requested
	{
		if(auto iter = g_funcs_gen_0args.find(ident); iter != g_funcs_gen_0args.end())
		{
			return (*std::get<0>(iter->second))(ctx);
		}
		else
		{
			ctx.PrintError("No suitable zero-argument function \"", ident, "\" was found.");
			return nullptr;
		}
	}
	else if(args.size() == 1)	// function call with one argument requested
	{
		if(auto iter = g_funcs_gen_1arg.find(ident); iter != g_funcs_gen_1arg.end())
		{	// general function
			return (*std::get<0>(iter->second))(ctx, args[0]);
		}
		else if(auto iter = g_funcs_real_1arg.find(ident);
			iter != g_funcs_real_1arg.end() && args[0]->GetType() == SymbolType::REAL)
		{	// real function
			t_real funcval = (*std::get<0>(iter->second))(dynamic_cast<SymbolReal&>(*args[0]).GetValue());
			return std::make_shared<SymbolReal>(funcval);
		}
		else if(auto iter = g_funcs_arr_1arg.find(ident);
			iter != g_funcs_arr_1arg.end() && args[0]->GetType() == SymbolType::ARRAY)
		{	// array function
			return (*std::get<0>(iter->second))(*reinterpret_cast<std::shared_ptr<SymbolList>*>(&args[0]));
		}
		else if(auto iter = g_funcs_real_1arg.find(ident);
			iter != g_funcs_real_1arg.end() && args[0]->GetType() == SymbolType::ARRAY)
		{	// real function, but with array argument (evaluate point-wise)
			return call_realfunc_1arg_pointwise(ident, args[0]);
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
			return (*std::get<0>(iter->second))(ctx, args[0], args[1]);
		}
		else if(auto iter = g_funcs_real_2args.find(ident);
			iter != g_funcs_real_2args.end() &&
			args[0]->GetType() == SymbolType::REAL && args[1]->GetType() == SymbolType::REAL)
		{ // real function
			t_real arg1 = dynamic_cast<SymbolReal&>(*args[0]).GetValue();
			t_real arg2 = dynamic_cast<SymbolReal&>(*args[1]).GetValue();
			t_real funcval = (*std::get<0>(iter->second))(arg1, arg2);
			return std::make_shared<SymbolReal>(funcval);
		}
		else if(auto iter = g_funcs_arr_2args.find(ident);
			iter != g_funcs_arr_2args.end()
			&& args[0]->GetType() == SymbolType::ARRAY && args[1]->GetType() == SymbolType::ARRAY)
		{	// array function
			return (*std::get<0>(iter->second))(*reinterpret_cast<std::shared_ptr<SymbolList>*>(&args[0]),
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

void CliASTArrayAccess::Print(std::ostringstream &ostr, int indent) const
{
	for(int i=0; i<indent; ++i) ostr << "\t";
	ostr << "op: array_access\n";
	CliAST::Print(ostr, indent);
}
// ----------------------------------------------------------------------------
