/**
 * hacks
 * @author Tobias Weber <tweber@ill.fr>
 * @date 29-Aug-2019
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

