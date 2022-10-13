/**
 * finds a matching space group
 * @author Tobias Weber <tweber@ill.fr>
 * @date jan-2020
 * @license GPLv3, see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2021  Tobias WEBER (Institut Laue-Langevin (ILL),
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

#include "../structfact/loadcif.h"
#include "tlibs2/libs/maths.h"

#include <gemmi/version.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <memory>


using t_real = double;
using t_vec = tl2::vec<t_real, std::vector>;
using t_mat = tl2::mat<t_real, std::vector>;

constexpr t_real g_eps = 1e-6;


/**
 * entry point
 */
int main(int argc, char** argv)
{
	std::cout << "Input atomic positions, 'e' or ENTER to end." << std::endl;

	std::vector<t_vec> vecFinal;
	/*{{ // test data
		tl2::create<t_vec>({ 0.1,  0.1,  0.1}),
		tl2::create<t_vec>({ 0.4, -0.1, -0.4}),
		tl2::create<t_vec>({-0.1, -0.4,  0.4}),
		tl2::create<t_vec>({-0.4,  0.4, -0.1}),
	}}*/;

	std::size_t atomnr = 1;
	while(1)
	{
		std::string strpos;
		std::cout << "Position " << atomnr++ << ": ";

		std::getline(std::cin, strpos);
		boost::trim(strpos);
		if(strpos == "" || strpos == "e")
			break;

		// tokenise
		std::vector<std::string> vecstr;
		boost::split(vecstr, strpos, [](char c) -> bool
		{
			return c == ' ' || c== '\t' || c == ',' || c == ';';
		}, boost::token_compress_on);


		// convert to real vector
		t_vec vecPos;
		for(const std::string& str : vecstr)
			vecPos.emplace_back(std::stod(str));

		// fill up possibly missing coordinates
		while(vecPos.size() < 3)
			vecPos.push_back(0);
		vecPos.resize(3);

		vecFinal.emplace_back(std::move(vecPos));
	}

	if(vecFinal.size() == 0)
	{
		std::cerr << "Insufficient number of positions given." << std::endl;
		return -1;
	}


	std::cout << "\nFull set of positions to match:\n";
	std::size_t ctr = 1;
	for(const auto& pos : vecFinal)
		std::cout << "\t(" << ctr++ << ") " << pos << "\n";
	std::cout << std::endl;


	std::vector<t_vec> vecInit = vecFinal;


	while(1)
	{
		std::cout << "\n--------------------------------------------------------------------------------\n";
		std::cout << "Base set of positions:\n";
		ctr = 1;
		for(const auto& pos : vecInit)
			std::cout << "\t(" << ctr++ << ") " << pos << "\n";
		std::cout << std::endl;

		auto matchingSGs = find_matching_sgs<t_vec, t_mat, t_real>(vecInit, vecFinal, g_eps);

		if(matchingSGs.size())
		{
			std::cout << "Matching space groups:\n";
			ctr = 1;
			for(const auto& sg : matchingSGs)
				std::cout << "\t(" << ctr++ << ") " << std::get<1>(sg) << "\n";
		}
		else
		{
			std::cout << "No matching space groups.\n";
		}
		std::cout << "--------------------------------------------------------------------------------\n";
		std::cout << std::endl;

		vecInit.pop_back();
		if(vecInit.size() == 0)
			break;
	}

	return 0;
}
