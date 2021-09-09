/**
 * evaluates symbols
 * @author Tobias Weber <tweber@ill.fr>
 * @date 15-Jun-2018
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

#include "cliparser.h"
#include "../globals.h"
#include "funcs.h"
#include "tlibs2/libs/algos.h"
#include "tlibs2/libs/str.h"

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
	if(sym1.GetType()==SymbolType::REAL && sym2.GetType()==SymbolType::ARRAY)
	{ // 1.23 + [1,2,3], point-wise addition
		std::vector<std::shared_ptr<Symbol>> arrNew;
		const auto& val = dynamic_cast<const SymbolReal&>(sym1);
		const auto& arr = dynamic_cast<const SymbolList&>(sym2).GetValue();
		for(std::size_t idx=0; idx<arr.size(); ++idx)
			arrNew.emplace_back(Symbol::add(val, *arr[idx]));
		return std::make_shared<SymbolList>(arrNew, false);
	}
	if(sym1.GetType()==SymbolType::ARRAY && sym2.GetType()==SymbolType::REAL)
	{ // [1,2,3] + 1.23, point-wise addition
		std::vector<std::shared_ptr<Symbol>> arrNew;
		const auto& arr = dynamic_cast<const SymbolList&>(sym1).GetValue();
		const auto& val = dynamic_cast<const SymbolReal&>(sym2);
		for(std::size_t idx=0; idx<arr.size(); ++idx)
			arrNew.emplace_back(Symbol::add(*arr[idx], val));
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
	else if(sym1.GetType()==SymbolType::DATASET && sym2.GetType()==SymbolType::REAL)
	{ // data + 1.23
		return std::make_shared<SymbolDataset>(
			dynamic_cast<const SymbolDataset&>(sym1).GetValue() + 
			dynamic_cast<const SymbolReal&>(sym2).GetValue());
	}
	else if(sym1.GetType()==SymbolType::DATASET && sym2.GetType()==SymbolType::REAL)
	{ // 1.23 + data
		return std::make_shared<SymbolDataset>(
			dynamic_cast<const SymbolReal&>(sym1).GetValue() + 
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
	if(sym1.GetType()==SymbolType::ARRAY && sym2.GetType()==SymbolType::REAL)
	{ // [1,2,3] - 1.23, point-wise subtraction
		std::vector<std::shared_ptr<Symbol>> arrNew;
		const auto& arr = dynamic_cast<const SymbolList&>(sym1).GetValue();
		const auto& val = dynamic_cast<const SymbolReal&>(sym2);
		for(std::size_t idx=0; idx<arr.size(); ++idx)
			arrNew.emplace_back(Symbol::sub(*arr[idx], val));
		return std::make_shared<SymbolList>(arrNew, false);
	}
	else if(sym1.GetType()==SymbolType::DATASET && sym2.GetType()==SymbolType::DATASET)
	{ // data1 - data2
		return std::make_shared<SymbolDataset>(
			dynamic_cast<const SymbolDataset&>(sym1).GetValue() - 
			dynamic_cast<const SymbolDataset&>(sym2).GetValue());
	}
	else if(sym1.GetType()==SymbolType::DATASET && sym2.GetType()==SymbolType::REAL)
	{ // data - 1.23
		return std::make_shared<SymbolDataset>(
			dynamic_cast<const SymbolDataset&>(sym1).GetValue() - 
			dynamic_cast<const SymbolReal&>(sym2).GetValue());
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


// array markers
static const std::string begin_arr = "###[";
static const std::string end_arr = "###]";
static const std::string arr_next_elem = "###,";

// dataset markers
static const std::string next_ch = "###CH;";
static const std::string begin_ax_name = "###AXID[";
static const std::string end_ax_name = "###AXID]";
static const std::string begin_axes = "###AXES[";
static const std::string end_axes = "###AXES]";
static const std::string begin_ctrs = "###CTRS[";
static const std::string end_ctrs = "###CTRS]";
static const std::string begin_ctr_errs = "###CTRERRS[";
static const std::string end_ctr_errs = "###CTRERRS]";
static const std::string begin_mons = "###MONS[";
static const std::string end_mons = "###MONS]";
static const std::string begin_mon_errs = "###MONERRS[";
static const std::string end_mon_errs = "###MONERRS]";
static const std::string next_elem = "###;";



/**
 * string representation of real
 */
std::string SymbolReal::serialise() const
{
	std::ostringstream ostr;
	ostr.precision(std::numeric_limits<t_real>::digits10);

	ostr << Symbol::get_type_name(*this) << ":" << GetValue();
	return ostr.str();
}


/**
 * string representation of string (trivial)
 */
std::string SymbolString::serialise() const
{
	return Symbol::get_type_name(*this) + ":" + GetValue();
}


/**
 * string representation of a list
 */
std::string SymbolList::serialise() const
{
	std::ostringstream ostr;
	ostr.precision(std::numeric_limits<t_real>::digits10);

	ostr << Symbol::get_type_name(*this) << ":" << begin_arr;
	bool firstelem = true;
	for(const auto &elem : m_val)
	{
		if(!firstelem) ostr << arr_next_elem;
		ostr << elem->serialise();
		firstelem = false;
	}
	ostr << end_arr;

	return ostr.str();
}


/**
 * string representation of a dataset
 */
std::string SymbolDataset::serialise() const
{
	std::ostringstream ostr;
	ostr.precision(std::numeric_limits<t_real>::digits10);
	ostr << Symbol::get_type_name(*this) << ":";

	const auto &dataset = GetValue();
	for(std::size_t chnr=0; chnr<dataset.GetNumChannels(); ++chnr)
	{
		const auto &data = dataset.GetChannel(chnr);
		const std::size_t num_x = data.GetNumAxes();
		const std::size_t num_ctr = data.GetNumCounters();
		const std::size_t num_mon = data.GetNumMonitors();

		// x axis names
		ostr << begin_ax_name;
		for(std::size_t xidx=0; xidx<num_x; ++xidx)
		{
			ostr << data.GetAxisName(xidx);
			if(xidx < num_x-1) ostr << ",";
		}
		ostr << end_ax_name;

		// x axes
		ostr << begin_axes;
		for(std::size_t xidx=0; xidx<num_x; ++xidx)
		{
			const auto &ax = data.GetAxis(xidx);
			for(std::size_t axidx=0; axidx<ax.size(); ++axidx)
			{
				ostr << ax[axidx];
				if(axidx < ax.size()-1) ostr << ",";
			}
			if(xidx < num_x-1) ostr << next_elem;
		}
		ostr << end_axes;


		// counters
		ostr << begin_ctrs;
		for(std::size_t ctridx=0; ctridx<num_ctr; ++ctridx)
		{
			const auto &ax = data.GetCounter(ctridx);
			for(std::size_t axidx=0; axidx<ax.size(); ++axidx)
			{
				ostr << ax[axidx];
				if(axidx < ax.size()-1) ostr << ",";
			}

			if(ctridx < num_ctr-1) ostr << next_elem;
		}
		ostr << end_ctrs;

		// counter errors
		ostr << begin_ctr_errs;
		for(std::size_t ctridx=0; ctridx<num_ctr; ++ctridx)
		{
			const auto &ax = data.GetCounterErrors(ctridx);
			for(std::size_t axidx=0; axidx<ax.size(); ++axidx)
			{
				ostr << ax[axidx];
				if(axidx < ax.size()-1) ostr << ",";
			}

			if(ctridx < num_ctr-1) ostr << next_elem;
		}
		ostr << end_ctr_errs;


		// monitors
		ostr << begin_mons;
		for(std::size_t ctridx=0; ctridx<num_mon; ++ctridx)
		{
			const auto &ax = data.GetMonitor(ctridx);
			for(std::size_t axidx=0; axidx<ax.size(); ++axidx)
			{
				ostr << ax[axidx];
				if(axidx < ax.size()-1) ostr << ",";
			}

			if(ctridx < num_mon-1) ostr << next_elem;
		}
		ostr << end_mons;

		// monitor errors
		ostr << begin_mon_errs;
		for(std::size_t ctridx=0; ctridx<num_mon; ++ctridx)
		{
			const auto &ax = data.GetMonitorErrors(ctridx);
			for(std::size_t axidx=0; axidx<ax.size(); ++axidx)
			{
				ostr << ax[axidx];
				if(axidx < ax.size()-1) ostr << ",";
			}

			if(ctridx < num_mon-1) ostr << next_elem;
		}
		ostr << end_mon_errs;

		ostr << next_ch;
	}

	return ostr.str();
}


/**
 * re-construct a symbol from a string representation
 */
std::shared_ptr<Symbol> Symbol::unserialise(const std::string &str)
{
	static const std::size_t len_begin_arr = begin_arr.length();


	auto [ty, val] = tl2::split_first(str, std::string(":"));
	tl2::trim(ty);

	if(ty == "real")
	{
		t_real d = tl2::str_to_var<t_real>(val);
		return std::make_shared<SymbolReal>(d);
	}
	else if(ty == "string")
	{
		return std::make_shared<SymbolString>(val);
	}
	else if(ty == "list" || ty == "array")
	{
		// find array boundaries in string repr
		std::size_t arr_begin = val.find(begin_arr);
		std::size_t arr_end = val.rfind(end_arr);
		if(arr_begin == std::string::npos || arr_end == std::string::npos)
		{
			print_err("Invalid array begin or end indicators: \"", val, "\".");
			return nullptr;
		}
		arr_begin += len_begin_arr;
		val = val.substr(arr_begin, arr_end-arr_begin);


		std::vector<std::shared_ptr<Symbol>> vec;

		// unserialise vector elements
		std::vector<std::string> toks;
		tl2::get_tokens_seq<std::string, std::string>(val, arr_next_elem, toks);
		std::string accum_tok;
		for(const auto &tok : toks)
		{
			// chain back together elements of nested sub-array
			if(accum_tok == "")
				accum_tok += tok;
			else
				accum_tok += arr_next_elem + tok;

			// balance brackets of nested arrays
			int nest_level = int(tl2::count_occurrences(accum_tok, begin_arr)) - int(tl2::count_occurrences(accum_tok, end_arr));
			if(nest_level < 0)
			{
				print_err("Unbalanced brackets in serialised array representation: \"", accum_tok, "\"");
				return nullptr;
			}
			else if(nest_level > 0) 	// get next token until end bracket is found
			{
				continue;
			}
			else if(nest_level == 0)	// found matching brackets
			{
				auto elemsym = unserialise(accum_tok);
				if(elemsym == nullptr)
				{
					print_err("Invalid array element: \"", accum_tok, "\".");
					return nullptr;
				}
				vec.emplace_back(elemsym);

				accum_tok = "";
			}
		}

		return std::make_shared<SymbolList>(vec, ty=="list");
	}
	else if(ty == "dataset")
	{
		Dataset dataset;

		// iterate channels
		std::vector<std::string> tok_channels;
		tl2::get_tokens_seq<std::string, std::string>(val, next_ch, tok_channels, true);
		for(auto &tok_ch : tok_channels)
		{
			tl2::trim(tok_ch);
			if(tok_ch == "") continue;

			// get the various parts of the data structure
			std::string str_axis_names = tl2::str_between(tok_ch, begin_ax_name, end_ax_name, false, true);
			std::string str_axes = tl2::str_between(tok_ch, begin_axes, end_axes, false, true);
			std::string str_ctrs = tl2::str_between(tok_ch, begin_ctrs, end_ctrs, false, true);
			std::string str_ctr_errs = tl2::str_between(tok_ch, begin_ctr_errs, end_ctr_errs, false, true);
			std::string str_mons = tl2::str_between(tok_ch, begin_mons, end_mons, false, true);
			std::string str_mon_errs = tl2::str_between(tok_ch, begin_mon_errs, end_mon_errs, false, true);


			Data data;

			// axis names
			{
				std::vector<std::string> axis_names;
				tl2::get_tokens<std::string, std::string>(str_axis_names, ",", axis_names);
				data.SetAxisNames(std::move(axis_names));
			}

			// axes
			{
				std::vector<std::string> tok_axes;
				tl2::get_tokens_seq<std::string, std::string>(str_axes, next_elem, tok_axes, true);
				for(const auto &tok_axis : tok_axes)
				{
					std::vector<t_real_dat> axis;
					tl2::get_tokens<t_real_dat, std::string>(tok_axis, ",", axis);
					data.AddAxis(std::move(axis));
				}
			}

			// counters
			{
				std::vector<std::string> tok_ctrs, tok_ctr_errs;
				tl2::get_tokens_seq<std::string, std::string>(str_ctrs, next_elem, tok_ctrs, true);
				tl2::get_tokens_seq<std::string, std::string>(str_ctr_errs, next_elem, tok_ctr_errs, true);
				for(std::size_t idxax=0; idxax<std::min(tok_ctrs.size(),tok_ctr_errs.size()); ++idxax)
				{
					const auto &tok_axis = tok_ctrs[idxax];
					const auto &tok_axis_err = tok_ctr_errs[idxax];

					std::vector<t_real_dat> axis, axis_err;
					tl2::get_tokens<t_real_dat, std::string>(tok_axis, ",", axis);
					tl2::get_tokens<t_real_dat, std::string>(tok_axis_err, ",", axis_err);

					data.AddCounter(std::move(axis), std::move(axis_err));
				}
			}

			// monitors
			{
				std::vector<std::string> tok_ctrs, tok_ctr_errs;
				tl2::get_tokens_seq<std::string, std::string>(str_mons, next_elem, tok_ctrs, true);
				tl2::get_tokens_seq<std::string, std::string>(str_mon_errs, next_elem, tok_ctr_errs, true);
				for(std::size_t idxax=0; idxax<std::min(tok_ctrs.size(),tok_ctr_errs.size()); ++idxax)
				{
					const auto &tok_axis = tok_ctrs[idxax];
					const auto &tok_axis_err = tok_ctr_errs[idxax];

					std::vector<t_real_dat> axis, axis_err;
					tl2::get_tokens<t_real_dat, std::string>(tok_axis, ",", axis);
					tl2::get_tokens<t_real_dat, std::string>(tok_axis_err, ",", axis_err);

					data.AddMonitor(std::move(axis), std::move(axis_err));
				}
			}

			dataset.AddChannel(std::move(data));
		}

		return std::make_shared<SymbolDataset>(dataset);
	}

	print_err("Unknown variable type: ", ty, ".");
	return nullptr;
}
