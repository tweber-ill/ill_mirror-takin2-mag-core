/**
 * crystal matrix text
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-19
 * @license GPLv3, see 'LICENSE' file
 *
 * g++-8 -std=c++17 -fconcepts -DUSE_LAPACK -I/usr/include/lapacke -I/usr/local/opt/lapack/include -L/usr/local/opt/lapack/lib -o cryst cryst.cpp -llapacke
 */

#include <iostream>
#include <vector>

#include "../../libs/_cxx20/math_algos.h"
using namespace m_ops;


using t_real = double;
//using t_real = float;
using t_cplx = std::complex<t_real>;
using t_vec = std::vector<t_real>;
using t_mat = m::mat<t_real, std::vector>;
using t_vec_cplx = std::vector<t_cplx>;
using t_mat_cplx = m::mat<t_cplx, std::vector>;


int main()
{
	auto A = m::A_matrix<t_mat>(3., 4., 5., 80./180.*M_PI, 100./180.*M_PI, 60./180.*M_PI);
	auto B = m::B_matrix<t_mat>(3., 4., 5., 80./180.*M_PI, 100./180.*M_PI, 60./180.*M_PI);
	auto [B2, ok] = m::inv(A);
	B2 = 2.*M_PI * m::trans(B2);

	std::cout << "A  = " << A << std::endl;
	std::cout << "B  = " << B << std::endl;
	std::cout << "B2 = " << B2 << std::endl;

	return 0;
}
