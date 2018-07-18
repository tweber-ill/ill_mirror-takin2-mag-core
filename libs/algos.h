/**
 * algorithm helpers
 * @author Tobias Weber <tweber@ill.fr>
 * @date 9-Apr-2018
 * @license see 'LICENSE' file
 */

#ifndef __IN20_ALGOS_H__
#define __IN20_ALGOS_H__

#include <algorithm>
#include <string>


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


/**
 * count number of ocurrences of a sub-string in a string
 */
static std::size_t count_occurrences(const std::string &str, const std::string &tok)
{
	std::size_t num = 0;
	std::size_t start = 0;
	const std::size_t len_tok = tok.length();

	while(true)
	{
		std::size_t idx = str.find(tok, start);
		if(idx == std::string::npos)
			break;

		++num;
		start += idx+len_tok;
	}

	return num;
}


#endif
