/**
 * globals
 * @author Tobias Weber <tweber@ill.fr>
 * @date 19-Jun-2018
 * @license see 'LICENSE' file
 */

#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include "command.h"

#define PROGRAM_VERSION "0.0.1"


// the GUI's command line widget
extern CommandLine *g_pCLI;


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
