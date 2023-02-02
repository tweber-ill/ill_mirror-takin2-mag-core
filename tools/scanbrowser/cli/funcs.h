/**
 * Built-in functions
 * @author Tobias Weber <tweber@ill.fr>
 * @date 20-Jun-2018
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

#ifndef __FUNCS_H__
#define __FUNCS_H__

#include <unordered_map>
#include <vector>
#include <tuple>
#include <string>
#include <memory>

//#include "cliparser_types.h"
#include "cliparser.h"


// real functions
extern std::unordered_map<std::string, std::tuple<t_real(*)(t_real), std::string>> g_funcs_real_1arg;
extern std::unordered_map<std::string, std::tuple<t_real(*)(t_real, t_real), std::string>> g_funcs_real_2args;

// array functions
extern std::unordered_map<std::string, std::tuple<std::shared_ptr<Symbol>(*)(std::shared_ptr<SymbolList>), std::string>> g_funcs_arr_1arg;
extern std::unordered_map<std::string, std::tuple<std::shared_ptr<Symbol>(*)(std::shared_ptr<SymbolList>, std::shared_ptr<SymbolList>), std::string>> g_funcs_arr_2args;

// general functions
extern std::unordered_map<std::string, std::tuple<std::shared_ptr<Symbol>(*)(CliParserContext&), std::string>> g_funcs_gen_0args;
extern std::unordered_map<std::string, std::tuple<std::shared_ptr<Symbol>(*)(CliParserContext&, std::shared_ptr<Symbol>), std::string>> g_funcs_gen_1arg;
extern std::unordered_map<std::string, std::tuple<std::shared_ptr<Symbol>(*)(CliParserContext&, std::shared_ptr<Symbol>, std::shared_ptr<Symbol>), std::string>> g_funcs_gen_2args;
extern std::unordered_map<std::string, std::tuple<std::shared_ptr<Symbol>(*)(CliParserContext&, const std::vector<std::shared_ptr<Symbol>>&), std::string>> g_funcs_gen_vararg;

// constants
extern std::unordered_map<std::string, std::tuple<t_real, std::string>> g_consts_real;



// functions which are also needed in other modules
extern std::shared_ptr<Symbol> func_dot(std::shared_ptr<SymbolList> sym1, std::shared_ptr<SymbolList> sym2);
extern std::shared_ptr<Symbol> call_realfunc_1arg_pointwise(const std::string& ident, std::shared_ptr<Symbol> sym);


#endif
