/**
 * regression test
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-19
 * @license GPLv3, see 'LICENSE' file
 *
 * g++-8 -std=c++17 -fconcepts -o leastsq leastsq.cpp
 * g++-8 -std=c++17 -fconcepts -DUSE_LAPACK -I/usr/include/lapacke -I/usr/local/opt/lapack/include -L/usr/local/opt/lapack/lib -o leastsq leastsq.cpp -llapacke
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
	//std::cout << m::stoval<unsigned int>("123") << std::endl;

	auto x = m::create<t_vec>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
	auto y = m::create<t_vec>({5, 5, 7, 9, 9.5, 10.5, 10.5, 12, 13.5, 14});

	auto [params, ok] = m::leastsq<t_vec>(x, y, 1);
	std::cout << "ok: " << ok << ", params: " << params << std::endl;

	return 0;
}
