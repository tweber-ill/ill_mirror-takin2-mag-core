/**
 * Evaluates symbols
 * @author Tobias Weber <tweber@ill.fr>
 * @date 15-Jun-2018
 * @license see 'LICENSE' file
 */

#include "cliparser.h"
#include "tools/in20/globals.h"
#include "funcs.h"

#include "tlibs/string/string.h"
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
 * string representation of list
 */
std::string SymbolList::serialise() const
{
	std::ostringstream ostr;
	ostr.precision(std::numeric_limits<t_real>::digits10);

	ostr << Symbol::get_type_name(*this) << ":" << "###[";
	bool firstelem = true;
	for(const auto &elem : m_val)
	{
		if(!firstelem) ostr << "###, ";
		ostr << elem->serialise();
		firstelem = false;
	}
	ostr << "###]";

	return ostr.str();
}


/**
 * string representation of dataset
 */
std::string SymbolDataset::serialise() const
{
	std::ostringstream ostr;
	ostr.precision(std::numeric_limits<t_real>::digits10);

	return ostr.str();
}


/**
 * re-construct a symbol from a string representation
 */
std::shared_ptr<Symbol> Symbol::unserialise(const std::string &str)
{
	auto [ty, val] = tl::split_first(str, std::string(":"));
	tl::trim(ty);

	if(ty == "real")
	{
		t_real d = tl::str_to_var<t_real>(val);
		return std::make_shared<SymbolReal>(d);
	}
	else if(ty == "string")
	{
		return std::make_shared<SymbolString>(val);
	}
	else if(ty == "list" || ty == "array")
	{
		std::vector<std::shared_ptr<Symbol>> vec;
		// TODO
	
		return std::make_shared<SymbolList>(vec, ty=="list");
	}
	else if(ty == "dataset")
	{
		Dataset dataset;
		// TODO

		return std::make_shared<SymbolDataset>(dataset);
	}

	print_err("Unknown variable type: ", ty, ".");
	return nullptr;
}
