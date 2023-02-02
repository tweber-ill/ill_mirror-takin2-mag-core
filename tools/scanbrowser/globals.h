/**
 * globals
 * @author Tobias Weber <tweber@ill.fr>
 * @date 19-Jun-2018
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

#ifndef __SCANBROWSER_GLOBALS_H__
#define __SCANBROWSER_GLOBALS_H__


#include "types.h"
#include "command.h"


#define PROGRAM_VERSION "0.1"



// the GUI's command line widget
extern CommandLine *g_pCLI;

// output precision
extern std::size_t g_prec;

// epsilon value for merging data columns
extern t_real_dat g_eps_merge;



/**
 * print output
 */
template<typename ...T> void print_out(T&&... msgs)
{
	if(g_pCLI)
		g_pCLI->GetWidget()->PrintOutput(false, msgs...);
}


/**
 * print error messages
 */
template<typename ...T> void print_err(T&&... msgs)
{
	if(g_pCLI)
		g_pCLI->GetWidget()->PrintOutput(true, msgs...);
}


#endif
