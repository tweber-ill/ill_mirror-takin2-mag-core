/**
 * brillouin zone tool calling test
 * @author Tobias Weber <tweber@ill.fr>
 * @date May-2022
 * @license GPLv3, see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2023  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 * "misc" project
 * Copyright (C) 2017-2021  Tobias WEBER (privately developed).
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

// g++ -std=c++20 -o call_bz call_bz.cpp -lboost_iostreams

#include <iostream>
#include <cstdio>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream_buffer.hpp>
namespace ios = boost::iostreams;


int main(int, char**)
{
	// create stream buffer
	ios::stream_buffer buf(
		ios::file_descriptor_source(
			::fileno(::popen("../build/takin_bz -c -i ../build/0.xml", "r")),
		ios::close_handle));

	// create input stream using the stream buffer
	std::istream istr(&buf);

	// read results from standard output
	std::string result, line;
	while(std::getline(istr, line))
		result += line + '\n';

	std::cout << result << std::endl;
	return 0;
}
