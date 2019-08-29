/**
 * hacks
 * @author Tobias Weber <tweber@ill.fr>
 * @date 29-Aug-2019
 * @license see 'LICENSE' file
 */

#include <string>
#include <locale>

#include "hacks.h"



/**
 * function to prevent boost.filesystem linking error
 */
namespace boost{ namespace filesystem{ namespace path_traits{
void convert(const wchar_t *begin, const wchar_t *end,
	std::string& str, const std::codecvt<wchar_t, char, std::mbstate_t>& cvt)
{
	std::size_t len = end-begin;
	str.resize(len);

	std::mbstate_t state;
	const wchar_t* in_next = nullptr;
	char* out_next = nullptr;
	cvt.out(state, begin, end, in_next, str.data(), str.data()+len, out_next);
}
}}}

