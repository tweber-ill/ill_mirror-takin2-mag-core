/**
 * finds a matching space group
 * @author Tobias Weber <tweber@ill.fr>
 * @date jan-2020
 * @license GPLv3, see 'LICENSE' file
 */

#include "../structfact/loadcif.h"
#include "libs/_cxx20/math_algos.h"

#include <gemmi/version.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <memory>


using t_real = double;
using t_vec = std::vector<t_real>;
using t_mat = m::mat<t_real, std::vector>;

constexpr t_real g_eps = 1e-6;
constexpr int g_prec = 6;



/**
 * find matching spacegroup
 */
std::vector<std::tuple<int, std::string, std::vector<t_mat>>> find_sgs(
	const std::vector<t_vec>& vecInit, const std::vector<t_vec>& vecFinal)
{
	return find_matching_sgs<t_vec, t_mat, t_real>(vecInit, vecFinal);
}



/**
 * entry point
 */
int main(int argc, char** argv)
{
	std::cout << "Input atomic positions, 'e' or ENTER to end." << std::endl;

	std::vector<t_vec> vecFinal;
	/*{{ // test data
		m::create<t_vec>({ 0.1,  0.1,  0.1}),
		m::create<t_vec>({ 0.4, -0.1, -0.4}),
		m::create<t_vec>({-0.1, -0.4,  0.4}),
		m::create<t_vec>({-0.4,  0.4, -0.1}),
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

		auto matchingSGs = find_sgs(vecInit, vecFinal);

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
