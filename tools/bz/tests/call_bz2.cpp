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

// g++ -std=c++20 -o call_bz2 call_bz2.cpp

#include <iostream>
#include <fstream>

#include <boost/process.hpp>
namespace proc = boost::process;


static std::string read_file(const std::string& file)
{
	std::ifstream ifstr(file);
	if(!ifstr)
		return "";

	std::string result, line;
	while(std::getline(ifstr, line))
		result += line + '\n';

	return result;
}


int main(int, char**)
{
	try
	{
		std::string binary = "../build/takin_bz";

		// runs takin using boost.process, see: https://www.boost.org/doc/libs/1_82_0/doc/html/boost_process/tutorial.html
		proc::opstream istr;
		proc::ipstream ostr, errstr;
		proc::child ch(binary + " -c -s", proc::std_in<istr, proc::std_out>ostr, proc::std_err>errstr);
		if(!ch.valid())
		{
			std::cerr << "Invalid process handle." << std::endl;
			return -1;
		}

		ch.detach();

		// write input file to process' standard input
		istr << read_file("../build/0.xml") << std::endl;
		istr.pipe().close();

		ch.wait();

		// read results from process' standard output
		std::string result, line;
		while(std::getline(ostr, line))
			result += line + '\n';

		std::cout << "Result:\n" << result << std::endl;
	}
	catch(const std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
		return -1;
	}

	return 0;
}
