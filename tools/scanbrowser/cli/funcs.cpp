/**
 * Built-in functions
 * @author Tobias Weber <tweber@ill.fr>
 * @date 20-Jun-2018
 * @license see 'LICENSE' file
 */

#include "funcs.h"
#include "../globals.h"
#include "tlibs2/libs/phys.h"
#include "tlibs2/libs/fit.h"


using t_real = t_real_cli;


// ----------------------------------------------------------------------------
// real functions
// ----------------------------------------------------------------------------

/**
 * point-wise evaluation of a real function for an array type
 */
std::shared_ptr<Symbol> call_realfunc_1arg_pointwise(const std::string& ident, std::shared_ptr<Symbol> sym)
{
	auto iter = g_funcs_real_1arg.find(ident);
	if(iter == g_funcs_real_1arg.end())
		return nullptr;

	if(sym->GetType() == SymbolType::ARRAY)
	{
		const auto& arr = dynamic_cast<const SymbolList&>(*sym).GetValue();
		std::vector<std::shared_ptr<Symbol>> arrNew;
		arrNew.reserve(arr.size());

		for(const auto& val : arr)
		{
			auto funcval = call_realfunc_1arg_pointwise(ident, val);
			arrNew.emplace_back(funcval);
		}

		return std::make_shared<SymbolList>(arrNew, false);
	}
	else if(sym->GetType() == SymbolType::REAL)
	{
		auto funcval = (*std::get<0>(iter->second))(dynamic_cast<SymbolReal&>(*sym).GetValue());
		return std::make_shared<SymbolReal>(funcval);
	}

	return nullptr;
}
// ----------------------------------------------------------------------------







// ----------------------------------------------------------------------------
// array functions
// ----------------------------------------------------------------------------

/**
 * dot product
 */
std::shared_ptr<Symbol> func_dot(std::shared_ptr<SymbolList> sym1, std::shared_ptr<SymbolList> sym2)
{
	std::shared_ptr<Symbol> symDot = std::make_shared<SymbolReal>(0);
	const auto& arr1 = sym1->GetValue();
	const auto& arr2 = sym2->GetValue();
	for(std::size_t idx=0; idx<std::min(arr1.size(), arr2.size()); ++idx)
		symDot = Symbol::add(symDot, Symbol::mul(arr1[idx], arr2[idx]));
	return symDot;
}


/**
 * cross product
 */
std::shared_ptr<Symbol> func_cross(std::shared_ptr<SymbolList> sym1, std::shared_ptr<SymbolList> sym2)
{
	SymbolReal symCross(0);
	const auto& arr1 = sym1->GetValue();
	const auto& arr2 = sym2->GetValue();

	if(arr1.size() != 3 && arr2.size() != 3)
		return nullptr;

	// components
	std::vector<std::shared_ptr<Symbol>> vec;
	vec.emplace_back(Symbol::sub(Symbol::mul(arr1[1], arr2[2]), Symbol::mul(arr1[2], arr2[1])));
	vec.emplace_back(Symbol::sub(Symbol::mul(arr1[2], arr2[0]), Symbol::mul(arr1[0], arr2[2])));
	vec.emplace_back(Symbol::sub(Symbol::mul(arr1[0], arr2[1]), Symbol::mul(arr1[1], arr2[0])));

	return std::make_shared<SymbolList>(vec, false);
}


/**
 * norm
 */
std::shared_ptr<Symbol> func_norm(std::shared_ptr<SymbolList> sym)
{
	// dot
	auto symDot = func_dot(sym, sym);
	// sqrt
	return Symbol::pow(*symDot, SymbolReal(0.5));
}
// ----------------------------------------------------------------------------






// ----------------------------------------------------------------------------
// general functions
// ----------------------------------------------------------------------------

/**
 * typeof function
 */
static std::shared_ptr<Symbol> func_typeof(CliParserContext&, std::shared_ptr<Symbol> sym)
{
	const std::string& ty = Symbol::get_type_name(*sym);
	return std::make_shared<SymbolString>(ty);
}


/**
 * sizeof function
 */
static std::shared_ptr<Symbol> func_sizeof(CliParserContext&, std::shared_ptr<Symbol> sym)
{
	if(sym->GetType() == SymbolType::ARRAY)
	{	// array length
		std::size_t len = dynamic_cast<SymbolList&>(*sym).GetValue().size();
		return std::make_shared<SymbolReal>(len);
	}
	else if(sym->GetType() == SymbolType::STRING)
	{	// string length
		std::size_t len = dynamic_cast<SymbolString&>(*sym).GetValue().length();
		return std::make_shared<SymbolReal>(len);
	}
	else if(sym->GetType() == SymbolType::DATASET)
	{	// number of channels in dataset
		std::size_t len = dynamic_cast<SymbolDataset&>(*sym).GetValue().GetNumChannels();
		return std::make_shared<SymbolReal>(len);
	}

	// 1 for other types
	return std::make_shared<SymbolReal>(1);
}


/**
 * convert a dataset to an array
 */
std::shared_ptr<Symbol> func_array(CliParserContext & ctx, std::shared_ptr<Symbol> _sym)
{
	if(_sym->GetType() == SymbolType::DATASET)
	{
		std::vector<std::shared_ptr<Symbol>> arr;
		const auto& dataset = dynamic_cast<SymbolDataset&>(*_sym).GetValue();

		// iterate channels
		for(std::size_t ch=0; ch<dataset.GetNumChannels(); ++ch)
		{
			const auto& dat = dataset.GetChannel(ch);
			if(dat.GetNumAxes()==0 || dat.GetNumCounters()==0)
			{
				ctx.PrintError("Invalid x axis or counter data in channel ", ch, ".");
				return nullptr;
			}

			const auto& xvals = dat.GetAxis(0);
			const auto& yvals = dat.GetCounter(0);
			const auto& yerrs = dat.GetCounterErrors(0);

			std::vector<std::shared_ptr<Symbol>> x, y, yerr;

			for(std::size_t i=0; i<xvals.size(); ++i)
			{
				x.emplace_back(std::make_shared<SymbolReal>(xvals[i]));
				y.emplace_back(std::make_shared<SymbolReal>(yvals[i]));
				yerr.emplace_back(std::make_shared<SymbolReal>(yerrs[i]));
			}

			// add x, y, yerr to channel
			std::vector<std::shared_ptr<Symbol>> arrCh;
			arrCh.emplace_back(std::make_shared<SymbolList>(x, false));
			arrCh.emplace_back(std::make_shared<SymbolList>(y, false));
			arrCh.emplace_back(std::make_shared<SymbolList>(yerr, false));

			// add channel to array
			arr.emplace_back(std::make_shared<SymbolList>(arrCh, false));
		}

		return std::make_shared<SymbolList>(arr, false);
	}

	return nullptr;
}



/**
 * help
 */
std::shared_ptr<Symbol> func_help(CliParserContext&)
{
	std::ostringstream ostr;

	ostr << "<hr>Takin/Scan Browser version " << PROGRAM_VERSION << ".<br>\n";
	ostr << "Written by Tobias Weber &lt;tweber@ill.fr&gt;, 2018-2021.<hr><br>\n";

	ostr << "Type funcs() or vars() to list available functions or variables.<br>\n";

	return std::make_shared<SymbolString>(ostr.str());
}



/**
 * list of functions
 */
std::shared_ptr<Symbol> func_funcs(CliParserContext&)
{
	std::ostringstream ostr;


	if(g_funcs_real_1arg.size())
	{
		ostr << "<table border=\"1\" width=\"75%\">\n";
		ostr << "<caption><b>Real functions with one argument</b></caption>\n";
		ostr << "<tr>" << "<th>" << "Function" << "</th>" << "<th>" << "Description" << "</th>" << "</tr>\n";

		for(const auto& pair : g_funcs_real_1arg)
		{
			ostr << "<tr>";
			ostr << "<td>" << pair.first << "</td>" << "<td>" << std::get<1>(pair.second) << "</td>\n";
			ostr << "</tr>\n";
		}
		ostr << "</table>\n";
		ostr << "<br>\n";
	}


	if(g_funcs_real_2args.size())
	{
		ostr << "<table border=\"1\" width=\"75%\">\n";
		ostr << "<caption><b>Real functions with two arguments</b></caption>\n";
		ostr << "<tr>" << "<th>" << "Function" << "</th>" << "<th>" << "Description" << "</th>" << "</tr>\n";

		for(const auto& pair : g_funcs_real_2args)
		{
			ostr << "<tr>";
			ostr << "<td>" << pair.first << "</td>" << "<td>" << std::get<1>(pair.second) << "</td>\n";
			ostr << "</tr>\n";
		}
		ostr << "</table>\n";
		ostr << "<br>\n";
	}


	if(g_funcs_arr_1arg.size())
	{
		ostr << "<table border=\"1\" width=\"75%\">\n";
		ostr << "<caption><b>Array functions with one argument</b></caption>\n";
		ostr << "<tr>" << "<th>" << "Function" << "</th>" << "<th>" << "Description" << "</th>" << "</tr>\n";

		for(const auto& pair : g_funcs_arr_1arg)
		{
			ostr << "<tr>";
			ostr << "<td>" << pair.first << "</td>" << "<td>" << std::get<1>(pair.second) << "</td>\n";
			ostr << "</tr>\n";
		}
		ostr << "</table>\n";
		ostr << "<br>\n";
	}


	if(g_funcs_arr_2args.size())
	{
		ostr << "<table border=\"1\" width=\"75%\">\n";
		ostr << "<caption><b>Array functions with two arguments</b></caption>\n";
		ostr << "<tr>" << "<th>" << "Function" << "</th>" << "<th>" << "Description" << "</th>" << "</tr>\n";

		for(const auto& pair : g_funcs_arr_2args)
		{
			ostr << "<tr>";
			ostr << "<td>" << pair.first << "</td>" << "<td>" << std::get<1>(pair.second) << "</td>\n";
			ostr << "</tr>\n";
		}
		ostr << "</table>\n";
		ostr << "<br>\n";
	}


	if(g_funcs_gen_0args.size())
	{
		ostr << "<table border=\"1\" width=\"75%\">\n";
		ostr << "<caption><b>General functions with no arguments</b></caption>\n";
		ostr << "<tr>" << "<th>" << "Function" << "</th>" << "<th>" << "Description" << "</th>" << "</tr>\n";

		for(const auto& pair : g_funcs_gen_0args)
		{
			ostr << "<tr>";
			ostr << "<td>" << pair.first << "</td>" << "<td>" << std::get<1>(pair.second) << "</td>\n";
			ostr << "</tr>\n";
		}
		ostr << "</table>\n";
		ostr << "<br>\n";
	}


	if(g_funcs_gen_1arg.size())
	{
		ostr << "<table border=\"1\" width=\"75%\">\n";
		ostr << "<caption><b>General functions with one argument</b></caption>\n";
		ostr << "<tr>" << "<th>" << "Function" << "</th>" << "<th>" << "Description" << "</th>" << "</tr>\n";

		for(const auto& pair : g_funcs_gen_1arg)
		{
			ostr << "<tr>";
			ostr << "<td>" << pair.first << "</td>" << "<td>" << std::get<1>(pair.second) << "</td>\n";
			ostr << "</tr>\n";
		}
		ostr << "</table>\n";
		ostr << "<br>\n";
	}


	if(g_funcs_gen_2args.size())
	{
		ostr << "<table border=\"1\" width=\"75%\" >\n";
		ostr << "<caption><b>General functions with two arguments</b></caption>\n";
		ostr << "<tr>" << "<th>" << "Function" << "</th>" << "<th>" << "Description" << "</th>" << "</tr>\n";

		for(const auto& pair : g_funcs_gen_2args)
		{
			ostr << "<tr>";
			ostr << "<td>" << pair.first << "</td>" << "<td>" << std::get<1>(pair.second) << "</td>\n";
			ostr << "</tr>\n";
		}
		ostr << "</table>\n";
		ostr << "<br>\n";
	}


	if(g_funcs_gen_vararg.size())
	{
		ostr << "<table border=\"1\" width=\"75%\" >\n";
		ostr << "<caption><b>General functions with variable arguments</b></caption>\n";
		ostr << "<tr>" << "<th>" << "Function" << "</th>" << "<th>" << "Description" << "</th>" << "</tr>\n";

		for(const auto& pair : g_funcs_gen_vararg)
		{
			ostr << "<tr>";
			ostr << "<td>" << pair.first << "</td>" << "<td>" << std::get<1>(pair.second) << "</td>\n";
			ostr << "</tr>\n";
		}
		ostr << "</table>\n";
		ostr << "<br>\n";
	}

	return std::make_shared<SymbolString>(ostr.str());
}


/**
 * list of variables
 */
std::shared_ptr<Symbol> func_vars(CliParserContext & ctx)
{
	std::ostringstream ostr;

	// constants
	ostr << "<table border=\"1\" width=\"75%\" >\n";
	ostr << "<caption><b>Constants</b></caption>\n";
	ostr << "<tr>" << "<th>" << "Constant" << "</th>" 
		<< "<th>" << "Type" << "</th>" 
		<< "<th>" << "Value" << "</th>" 
		<< "<th>" << "Description" << "</th>" << "</tr>\n";

	for(const auto& pair : g_consts_real)
	{
		ostr << "<tr>";
		ostr << "<td>" << pair.first << "</td>" 
			<< "<td>" << "real" << "</td>"
			<< "<td>" << std::get<0>(pair.second) << "</td>" 
			<< "<td>" << std::get<1>(pair.second) << "</td>\n";
		ostr << "</tr>\n";
	}
	ostr << "</table>\n";
	ostr << "<br>\n";




	// variables
	if(auto *workspace = ctx.GetWorkspace(); workspace)
	{
		ostr << "<table border=\"1\" width=\"75%\" >\n";
		ostr << "<caption><b>Variables</b></caption>\n";
		ostr << "<tr>" << "<th>" << "Variable" << "</th>" 
			<< "<th>" << "Type" << "</th>" 
			<< "<th>" << "Value" << "</th>" << "</tr>\n";

		for(const auto& pair : *workspace)
		{
			ostr << "<tr>";
			ostr << "<td>" << pair.first << "</td>" 
				<< "<td>" << Symbol::get_type_name(*pair.second) << "</td>"
				<< "<td>" << (*pair.second) << "</td>\n";
			ostr << "</tr>\n";
		}
		ostr << "</table>\n";
		ostr << "<br>\n";
	}


	return std::make_shared<SymbolString>(ostr.str());
}


/**
 * clear all variables
 */
std::shared_ptr<Symbol> func_clear(CliParserContext &ctx)
{
	if(auto *workspace = ctx.GetWorkspace(); workspace)
	{
		std::ostringstream ostr;
		ostr << workspace->size() << " variables removed.";

		workspace->clear();
		ctx.EmitWorkspaceUpdated();

		return std::make_shared<SymbolString>(ostr.str());
	}

	return std::make_shared<SymbolString>("No workspace available.");
}



/**
 * append arrays or datasets
 */
std::shared_ptr<Symbol> func_append(CliParserContext & ctx, const std::vector<std::shared_ptr<Symbol>>& args)
{
	if(args.size() == 0)
	{
		ctx.PrintError("No arguments given.");
		return nullptr;
	}


	// append datasets
	if(args[0]->GetType() == SymbolType::DATASET)
	{
		// first dataset
		Dataset datret = dynamic_cast<const SymbolDataset&>(*args[0]).GetValue();

		for(std::size_t idx=1; idx<args.size(); ++idx)
		{
			if(args[idx]->GetType() != SymbolType::DATASET)
			{
				ctx.PrintError("Mismatching argument types. Expected data sets.");
				return nullptr;
			}

			const auto& dat = dynamic_cast<const SymbolDataset&>(*args[idx]).GetValue();
			datret = Dataset::append(datret, dat);
		}

		return std::make_shared<SymbolDataset>(datret);
	}

	// append arrays
	else if(args[0]->GetType() == SymbolType::ARRAY)
	{
		std::vector<std::shared_ptr<Symbol>> arrret;

		for(std::size_t idx=0; idx<args.size(); ++idx)
		{
			if(args[idx]->GetType() != SymbolType::ARRAY)
			{
				ctx.PrintError("Mismatching argument types. Expected arrays.");
				return nullptr;
			}

			const auto& arr = dynamic_cast<const SymbolList&>(*args[idx]).GetValue();
			for(const auto& sym : arr)
				arrret.push_back(sym->copy());
		}

		return std::make_shared<SymbolList>(arrret, false);
	}


	// otherwise fail
	ctx.PrintError("Invalid argument type for append operation: ", Symbol::get_type_name(*args[0]), ".");
	return nullptr;
}



/**
 * append datasets as the channels of a new dataset
 */
std::shared_ptr<Symbol> func_append_channels(CliParserContext & ctx, const std::vector<std::shared_ptr<Symbol>>& args)
{
	if(args.size() == 0)
	{
		ctx.PrintError("No arguments given.");
		return nullptr;
	}


	// append datasets
	if(args[0]->GetType() == SymbolType::DATASET)
	{
		// first dataset
		Dataset datret = dynamic_cast<const SymbolDataset&>(*args[0]).GetValue();

		for(std::size_t idx=1; idx<args.size(); ++idx)
		{
			if(args[idx]->GetType() != SymbolType::DATASET)
			{
				ctx.PrintError("Mismatching argument types. Expected data sets.");
				return nullptr;
			}

			const auto& dat = dynamic_cast<const SymbolDataset&>(*args[idx]).GetValue();
			datret = Dataset::append_channels(datret, dat);
		}

		return std::make_shared<SymbolDataset>(datret);
	}


	// otherwise fail
	ctx.PrintError("Invalid argument type for append operation: ", Symbol::get_type_name(*args[0]), ".");
	return nullptr;
}



/**
 * point-wise addition of arrays or datasets
 */
std::shared_ptr<Symbol> func_add_pointwise(CliParserContext & ctx, const std::vector<std::shared_ptr<Symbol>>& args)
{
	if(args.size() == 0)
	{
		ctx.PrintError("No arguments given.");
		return nullptr;
	}


	// add datasets
	if(args[0]->GetType() == SymbolType::DATASET)
	{
		// first dataset
		Dataset datret = dynamic_cast<const SymbolDataset&>(*args[0]).GetValue();

		for(std::size_t idx=1; idx<args.size(); ++idx)
		{
			if(args[idx]->GetType() != SymbolType::DATASET)
			{
				ctx.PrintError("Mismatching argument types. Expected data sets.");
				return nullptr;
			}

			const auto& dat = dynamic_cast<const SymbolDataset&>(*args[idx]).GetValue();
			datret = Dataset::add_pointwise(datret, dat);
		}

		return std::make_shared<SymbolDataset>(datret);
	}


	// otherwise simply call + operator
	std::shared_ptr<Symbol> symRet = std::make_shared<SymbolReal>(0);
	for(std::size_t idx=0; idx<args.size(); ++idx)
		symRet = Symbol::add(symRet, args[idx]);

	return symRet;
}



/**
 * normalise dataset to monitor counter
 */
std::shared_ptr<Symbol> func_normtomon(CliParserContext & ctx, std::shared_ptr<Symbol> arg)
{
	if(!arg || arg->GetType() != SymbolType::DATASET)
	{
		ctx.PrintError("Expected a data set.");
		return nullptr;
	}

	const auto &dat = dynamic_cast<const SymbolDataset&>(*arg);
	return std::make_shared<SymbolDataset>(dat.GetValue().norm(0));
}



/**
 * fit function
 */
std::shared_ptr<Symbol> func_fit (CliParserContext & ctx, const std::vector<std::shared_ptr<Symbol>>& args)
{
	if(args.size() < 2)
	{
		ctx.PrintError("Insufficient number of arguments given. At least a dataset and a fit function is needed.");
		return nullptr;
	}

	// TODO: multiple data sets
	if(args[0]->GetType() != SymbolType::DATASET)
	{
		ctx.PrintError("The first argument has to be a data set or a list of data sets.");
		return nullptr;
	}

	if(args[1]->GetType() != SymbolType::STRING)
	{
		ctx.PrintError("The second argument has to be a string giving the fit expression.");
		return nullptr;
	}


	const auto& dataset = dynamic_cast<SymbolDataset&>(*args[0]).GetValue();
	const std::string& strexpr = dynamic_cast<SymbolString&>(*args[1]).GetValue();

	std::vector<std::string> vars;
	std::vector<t_real> initials;
	std::vector<t_real> errs;
	std::vector<bool> fixed;


	// if no variables are given, determine them
	if(vars.size() == 0)
	{
		tl2::ExprParser<t_real> expr;
		expr.parse(strexpr);
		const auto& exprvars = expr.get_vars();
		for(const auto& pair : exprvars)
		{
			if(pair.first == "x")
				continue;
			
			vars.push_back(pair.first);
		}
	}

	initials.resize(vars.size(), 0);
	errs.resize(vars.size(), 0);
	fixed.resize(vars.size(), false);


	// fit all channels in the data set
	for(std::size_t channel=0; channel<dataset.GetNumChannels(); ++channel)
	{
		const auto& dat = dataset.GetChannel(channel);

		const auto& xvals = dat.GetAxis(0);
		const auto& yvals = dat.GetCounter(0);
		const auto& yerrs = dat.GetCounterErrors(0);

		tl2::fit_expr(strexpr, xvals, yvals, yerrs, "x", vars, initials, errs, &fixed);
	}

	return nullptr;
}
// ----------------------------------------------------------------------------









// ----------------------------------------------------------------------------
// maps
// ----------------------------------------------------------------------------

/**
 * map of real functions with one argument
 */
std::unordered_map<std::string, std::tuple<t_real_cli(*)(t_real), std::string>> g_funcs_real_1arg =
{
	std::make_pair("sin", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::sin), "")),
	std::make_pair("cos", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::cos), "")),
	std::make_pair("tan", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::tan), "")),
	std::make_pair("asin", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::asin), "")),
	std::make_pair("acos", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::acos), "")),
	std::make_pair("atan", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::atan), "")),

	std::make_pair("sinh", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::sinh), "")),
	std::make_pair("cosh", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::cosh), "")),
	std::make_pair("tanh", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::tanh), "")),
	std::make_pair("asinh", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::asinh), "")),
	std::make_pair("acosh", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::acosh), "")),
	std::make_pair("atanh", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::atanh), "")),

	std::make_pair("sqrt", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::sqrt), "")),
	std::make_pair("cbrt", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::cbrt), "")),

	std::make_pair("log", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::log), "")),
	std::make_pair("log10", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::log10), "")),
	std::make_pair("log2", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::log2), "")),
	std::make_pair("exp", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::exp), "")),
	std::make_pair("exp2", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::exp2), "")),

	std::make_pair("abs", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::abs), "")),
	std::make_pair("round", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::round), "")),
	std::make_pair("nearbyint", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::nearbyint), "")),
	std::make_pair("trunc", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::trunc), "")),
	std::make_pair("ceil", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::ceil), "")),
	std::make_pair("floor", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::floor), "")),

	std::make_pair("erf", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::erf), "")),
	std::make_pair("erfc", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::erfc), "")),
	//std::make_pair("beta", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::beta), "")),
	std::make_pair("gamma", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::tgamma), "")),
	std::make_pair("loggamma", std::make_tuple(static_cast<t_real(*)(t_real)>(&std::lgamma), "")),
};


/**
 * map of real functions with two arguments
 */
std::unordered_map<std::string, std::tuple<t_real_cli(*)(t_real, t_real), std::string>> g_funcs_real_2args =
{
	std::make_pair("pow", std::make_tuple(static_cast<t_real(*)(t_real, t_real)>(&std::pow), "")),

	std::make_pair("atan2", std::make_tuple(static_cast<t_real(*)(t_real, t_real)>(&std::atan2), "")),
	std::make_pair("hypot", std::make_tuple(static_cast<t_real(*)(t_real, t_real)>(&std::hypot), "")),

	std::make_pair("max", std::make_tuple(static_cast<t_real(*)(t_real, t_real)>(&std::fmax), "")),
	std::make_pair("min", std::make_tuple(static_cast<t_real(*)(t_real, t_real)>(&std::fmin), "")),
	//std::make_pair("diff", std::make_tuple(static_cast<t_real(*)(t_real, t_real)>(&std::fdim), "")),

	std::make_pair("remainder", std::make_tuple(static_cast<t_real(*)(t_real, t_real)>(&std::remainder), "")),
	std::make_pair("mod", std::make_tuple(static_cast<t_real(*)(t_real, t_real)>(&std::fmod), "")),

	std::make_pair("copysign", std::make_tuple(static_cast<t_real(*)(t_real, t_real)>(&std::copysign), "")),
};



/**
 * map of array functions with one argument
 */
std::unordered_map<std::string, std::tuple<std::shared_ptr<Symbol>(*)(std::shared_ptr<SymbolList>), std::string>> g_funcs_arr_1arg =
{
	std::make_pair("norm", std::make_tuple(&func_norm, "Euclidian norm")),
};

/**
 * map of array functions with two arguments
 */
std::unordered_map<std::string, std::tuple<std::shared_ptr<Symbol>(*)(std::shared_ptr<SymbolList>, std::shared_ptr<SymbolList>), std::string>> g_funcs_arr_2args =
{
	std::make_pair("dot", std::make_tuple(&func_dot, "inner product")),
	std::make_pair("cross", std::make_tuple(&func_cross, "cross product")),
};



/**
 * map of general functions with zero arguments
 */
std::unordered_map<std::string, std::tuple<std::shared_ptr<Symbol>(*)
	(CliParserContext&), std::string>> g_funcs_gen_0args =
{
	std::make_pair("help", std::make_tuple(&func_help, "display help")),
	std::make_pair("vars", std::make_tuple(&func_vars, "list variables")),
	std::make_pair("funcs", std::make_tuple(&func_funcs, "list functions")),
	std::make_pair("clear", std::make_tuple(&func_clear, "remove all variables")),
};

/**
 * map of general functions with one argument
 */
std::unordered_map<std::string, std::tuple<std::shared_ptr<Symbol>(*)
	(CliParserContext&, std::shared_ptr<Symbol>), std::string>> g_funcs_gen_1arg =
{
	std::make_pair("typeof", std::make_tuple(&func_typeof, "return symbol type")),
	std::make_pair("sizeof", std::make_tuple(&func_sizeof, "return symbol size")),
	std::make_pair("toarray", std::make_tuple(&func_array, "convert a dataset to an array")),

	std::make_pair("normtomon", std::make_tuple(&func_normtomon, "normalise data set to monitor counter")),
};

/**
 * map of general functions with two arguments
 */
std::unordered_map<std::string, std::tuple<std::shared_ptr<Symbol>(*)
	(CliParserContext&, std::shared_ptr<Symbol>, std::shared_ptr<Symbol>), std::string>> g_funcs_gen_2args =
{
};

/**
 * map of general functions with variable arguments
 */
std::unordered_map<std::string, std::tuple<std::shared_ptr<Symbol>(*)
	(CliParserContext&, const std::vector<std::shared_ptr<Symbol>>&), std::string>> g_funcs_gen_vararg = 
{
	std::make_pair("append", std::make_tuple(&func_append, "appends two or more data sets")),
	std::make_pair("append_channels", std::make_tuple(&func_append_channels, "appends two or more data set as individual channels")),
	std::make_pair("add_pointwise", std::make_tuple(&func_add_pointwise, "pointwise addition of two or more data sets")),
	std::make_pair("fit", std::make_tuple(&func_fit, "fitting of one or more data sets")),
};



/**
 * map of real constants
 */
std::unordered_map<std::string, std::tuple<t_real_cli, std::string>> g_consts_real
{
	std::make_pair("pi", std::make_tuple(tl2::pi<t_real>, "pi")),
};
// ----------------------------------------------------------------------------
