/**
 * finds a matching space group
 * @author Tobias Weber <tweber@ill.fr>
 * @date jan-2020
 * @license GPLv3, see 'LICENSE' file
 */

#include "../structfact/loadcif.h"
#include "libs/_cxx20/math_algos.h"

#include <gemmi/version.hpp>
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
	// TODO: read input data

	// test data
	std::vector<t_vec> vecInit
	{{
		m::create<t_vec>({ 0.1,  0.1,  0.1}),
		m::create<t_vec>({ 0.4, -0.1, -0.4}),
		m::create<t_vec>({-0.1, -0.4,  0.4}),
		m::create<t_vec>({-0.4,  0.4, -0.1}),
	}};
	std::vector<t_vec> vecFinal
	{{
		m::create<t_vec>({ 0.1,  0.1,  0.1}),
		m::create<t_vec>({ 0.4, -0.1, -0.4}),
		m::create<t_vec>({-0.1, -0.4,  0.4}),
		m::create<t_vec>({-0.4,  0.4, -0.1}),
	}};


	std::cout << "Full set of positions to match:\n";
	std::size_t ctr = 1;
	for(const auto& pos : vecFinal)
		std::cout << "\t(" << ctr++ << ") " << pos << "\n";
	std::cout << std::endl;

	while(1)
	{
		std::cout << "\n--------------------------------------------------------------------------------\n";
		std::cout << "Base set of positions:\n";
		ctr = 1;
		for(const auto& pos : vecInit)
			std::cout << "\t(" << ctr++ << ") " << pos << "\n";
		std::cout << std::endl;

		auto matchingSGs = find_sgs(vecInit, vecFinal);

		std::cout << "Matching space groups:\n";
		ctr = 1;
		for(const auto& sg : matchingSGs)
			std::cout << "\t(" << ctr++ << ") " << std::get<1>(sg) << "\n";
		std::cout << "--------------------------------------------------------------------------------\n";
		std::cout << std::endl;

		vecInit.pop_back();
		if(vecInit.size() == 0)
			break;
	}

	return 0;
}
