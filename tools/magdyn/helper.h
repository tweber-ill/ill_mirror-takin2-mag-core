/**
 * magnon dynamics -- helper functions
 * @author Tobias Weber <tweber@ill.fr>
 * @date Mar-2023
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 28-Dec-2018 from my privately developed "misc" project (https://github.com/t-weber/misc).
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2022  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 * "misc" project
 * Copyright (C) 2017-2022  Tobias WEBER (privately developed).
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

#ifndef __MAG_DYN_HELPERS__
#define __MAG_DYN_HELPERS__


#include <string>
#include <sstream>

#include "tlibs2/libs/str.h"


/**
 * get the rgb colour values from a string
 */
template<class t_val>
bool get_colour(const std::string& _col, t_val *rgb)
{
	std::string col = tl2::trimmed(_col);

	// use default colour
	if(col == "" || col == "auto")
		return false;

	std::istringstream istrcolour(col);

	// optional colour code prefix
	if(istrcolour.peek() == '#')
		istrcolour.get();

	std::size_t colour = 0;
	istrcolour >> std::hex >> colour;

	if constexpr(std::is_floating_point_v<t_val>)
	{
		// get the colour values as floats in the range [0, 1]
		rgb[0] = t_val((colour & 0xff0000) >> 16) / t_val(0xff);
		rgb[1] = t_val((colour & 0x00ff00) >> 8) / t_val(0xff);
		rgb[2] = t_val((colour & 0x0000ff) >> 0) / t_val(0xff);
	}
	else
	{
		// get the colour values as bytes in the range [0, 255]
		rgb[0] = t_val((colour & 0xff0000) >> 16);
		rgb[1] = t_val((colour & 0x00ff00) >> 8);
		rgb[2] = t_val((colour & 0x0000ff) >> 0);
	}

	return true;
}


#endif
