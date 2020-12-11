/**
 * polarisation test
 * @author Tobias Weber <tweber@ill.fr>
 * @date aug-18
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 8-Nov-2018 from my privately developed "magtools" project (https://github.com/t-weber/magtools).
 *
 * g++ -std=c++2a -fconcepts -I../.. -o pol_cli pol_cli.cpp
 */

#include <iostream>
#include "tlibs2/libs/math20.h"

using namespace tl2;
using namespace tl2_ops;

using t_real = double;
using t_cplx = std::complex<t_real>;
using t_vec = std::vector<t_cplx>;
using t_mat = mat<t_cplx, std::vector>;
using t_matvec = std::vector<t_mat>;


std::istringstream get_istr(std::istream& istr)
{
	std::string _str;
	std::getline(istr, _str);
	return std::istringstream(_str);
}


int main()
{
	constexpr bool bCheck = true;

	t_cplx N(0,0);
	t_vec Mperp = create<t_vec>({0, 0, t_cplx(1,2)});
	t_vec P = create<t_vec>({0, 1, 1});


	std::cout << "N = "; get_istr(std::cin) >> N;
	std::cout << "Mperp = "; std::cin >> Mperp; Mperp.resize(3);
	std::cout << "P = "; std::cin >> P; P.resize(3);

	std::cout << "\n";
	std::cout << "N = " << N << "\n";
	std::cout << "Mperp = " << Mperp << "\n";
	std::cout << "P = " << P << "\n";
	std::cout << std::endl;

	std::cout << "density matrix = " << pol_density_mat<t_vec, t_mat>(P) << "\n";
	std::cout << std::endl;

	auto [I, P_f] = blume_maleev_indir<t_mat, t_vec, t_cplx>(P, Mperp, N);
	std::cout << "I = " << I << "\nP_f = " << P_f << std::endl;

	// double-check results
	if constexpr(bCheck)
	{
		auto [I2, P_f2] = blume_maleev<t_vec, t_cplx>(P, Mperp, N);
		if(!equals(I, I2, 1e-5) || !equals(P_f, P_f2, 1e-5))
		{
			std::cerr << "Mismatch between blume_maleev() and blume_maleev_indir()!" << std::endl;
		}
	}

	return 0;
}
