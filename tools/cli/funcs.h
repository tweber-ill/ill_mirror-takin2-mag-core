/**
 * Built-in functions
 * @author Tobias Weber <tweber@ill.fr>
 * @date 20-Jun-2018
 * @license see 'LICENSE' file
 */

#ifndef __FUNCS_H__
#define __FUNCS_H__

#include <unordered_map>
#include <string>
#include <memory>

#include "cliparser_types.h"
#include "cliparser.h"


// real functions
extern std::unordered_map<std::string, t_real_cli(*)(t_real_cli)> g_funcs_real_1arg;
extern std::unordered_map<std::string, t_real_cli(*)(t_real_cli, t_real_cli)> g_funcs_real_2args;

// general functions
extern std::unordered_map<std::string, std::shared_ptr<Symbol>(*)(std::shared_ptr<Symbol>)> g_funcs_gen_1arg;

// constants
extern std::unordered_map<std::string, t_real_cli> g_consts_real;


#endif
