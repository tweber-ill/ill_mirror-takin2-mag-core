/**
 * Built-in functions
 * @author Tobias Weber <tweber@ill.fr>
 * @date 20-Jun-2018
 * @license see 'LICENSE' file
 */

#include "funcs.h"
#include <cmath>


using t_real = t_real_cli;


// ----------------------------------------------------------------------------
/**
 * map of real functions with one argument
 */
std::unordered_map<std::string, t_real_cli(*)(t_real)> g_funcs_real_1arg =
{
	std::make_pair("sin", static_cast<t_real(*)(t_real)>(&std::sin)),
	std::make_pair("cos", static_cast<t_real(*)(t_real)>(&std::cos)),
	std::make_pair("tan", static_cast<t_real(*)(t_real)>(&std::tan)),
	std::make_pair("asin", static_cast<t_real(*)(t_real)>(&std::asin)),
	std::make_pair("acos", static_cast<t_real(*)(t_real)>(&std::acos)),
	std::make_pair("atan", static_cast<t_real(*)(t_real)>(&std::atan)),

	std::make_pair("sinh", static_cast<t_real(*)(t_real)>(&std::sinh)),
	std::make_pair("cosh", static_cast<t_real(*)(t_real)>(&std::cosh)),
	std::make_pair("tanh", static_cast<t_real(*)(t_real)>(&std::tanh)),
	std::make_pair("asinh", static_cast<t_real(*)(t_real)>(&std::asinh)),
	std::make_pair("acosh", static_cast<t_real(*)(t_real)>(&std::acosh)),
	std::make_pair("atanh", static_cast<t_real(*)(t_real)>(&std::atanh)),

	std::make_pair("sqrt", static_cast<t_real(*)(t_real)>(&std::sqrt)),
	std::make_pair("cbrt", static_cast<t_real(*)(t_real)>(&std::cbrt)),

	std::make_pair("log", static_cast<t_real(*)(t_real)>(&std::log)),
	std::make_pair("log10", static_cast<t_real(*)(t_real)>(&std::log10)),
	std::make_pair("log2", static_cast<t_real(*)(t_real)>(&std::log2)),
	std::make_pair("exp", static_cast<t_real(*)(t_real)>(&std::exp)),
	std::make_pair("exp2", static_cast<t_real(*)(t_real)>(&std::exp2)),

	std::make_pair("abs", static_cast<t_real(*)(t_real)>(&std::abs)),
	std::make_pair("round", static_cast<t_real(*)(t_real)>(&std::round)),
	std::make_pair("nearbyint", static_cast<t_real(*)(t_real)>(&std::nearbyint)),
	std::make_pair("trunc", static_cast<t_real(*)(t_real)>(&std::trunc)),
	std::make_pair("ceil", static_cast<t_real(*)(t_real)>(&std::ceil)),
	std::make_pair("floor", static_cast<t_real(*)(t_real)>(&std::floor)),

	std::make_pair("erf", static_cast<t_real(*)(t_real)>(&std::erf)),
	std::make_pair("erfc", static_cast<t_real(*)(t_real)>(&std::erfc)),
	//std::make_pair("beta", static_cast<t_real(*)(t_real)>(&std::beta)),
	std::make_pair("gamma", static_cast<t_real(*)(t_real)>(&std::tgamma)),
	std::make_pair("loggamma", static_cast<t_real(*)(t_real)>(&std::lgamma)),
};


/**
 * map of real functions with two arguments
 */
std::unordered_map<std::string, t_real_cli(*)(t_real, t_real)> g_funcs_real_2args =
{
	std::make_pair("pow", static_cast<t_real(*)(t_real, t_real)>(&std::pow)),

	std::make_pair("atan2", static_cast<t_real(*)(t_real, t_real)>(&std::atan2)),
	std::make_pair("hypot", static_cast<t_real(*)(t_real, t_real)>(&std::hypot)),

	std::make_pair("max", static_cast<t_real(*)(t_real, t_real)>(&std::fmax)),
	std::make_pair("min", static_cast<t_real(*)(t_real, t_real)>(&std::fmin)),
	//std::make_pair("diff", static_cast<t_real(*)(t_real, t_real)>(&std::fdim)),

	std::make_pair("remainder", static_cast<t_real(*)(t_real, t_real)>(&std::remainder)),
	std::make_pair("mod", static_cast<t_real(*)(t_real, t_real)>(&std::fmod)),

	std::make_pair("copysign", static_cast<t_real(*)(t_real, t_real)>(&std::copysign)),
};
// ----------------------------------------------------------------------------





// ----------------------------------------------------------------------------
/**
 * typeof function
 */
static std::shared_ptr<Symbol> func_typeof(std::shared_ptr<Symbol> sym)
{
	const std::string& ty = Symbol::get_type_name(*sym);
	return std::make_shared<SymbolString>(ty);
}

/**
 * map of general functions with one argument
 */
std::unordered_map<std::string, std::shared_ptr<Symbol>(*)(std::shared_ptr<Symbol>)> g_funcs_gen_1arg =
{
	std::make_pair("typeof", &func_typeof),
};
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
/**
 * map of real constants
 */
std::unordered_map<std::string, t_real_cli> g_consts_real
{
	std::make_pair("pi", t_real(M_PI)),
};
// ----------------------------------------------------------------------------
