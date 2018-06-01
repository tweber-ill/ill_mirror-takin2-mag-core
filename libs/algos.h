/**
 * algorithm helpers
 * @author Tobias Weber <tweber@ill.fr>
 * @date 9-Apr-2018
 * @license see 'LICENSE' file
 */

#ifndef __IN20_ALGOS_H__
#define __IN20_ALGOS_H__

#include <algorithm>


/**
 * copy algorithm with interleave
 */
template<class T1, class T2>
void copy_interleave(T1 inIter, T1 inEnd, T2 outIter, std::size_t interleave, std::size_t startskip)
{
	std::advance(inIter, startskip);

	while(std::distance(inIter, inEnd) > 0)
	{
		*outIter = *inIter;

		++outIter;
		std::advance(inIter, interleave);
	}
}


#endif
