/**
 * globals
 * @author Tobias Weber <tweber@ill.fr>
 * @date 19-Jun-2018
 * @license see 'LICENSE' file
 */

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include "command.h"


// the GUI's command line widget
static CommandLine *g_pCLI = nullptr;


/**
 * print output
 */
template<typename ...T> void print_out(T&&... msgs)
{
	g_pCLI->GetWidget()->PrintOutput(false, msgs...);
}


/**
 * print error messages
 */
template<typename ...T> void print_err(T&&... msgs)
{
	g_pCLI->GetWidget()->PrintOutput(true, msgs...);
}


#endif
