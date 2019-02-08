/**
 * lapack test
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-19
 * @license GPLv3, see 'LICENSE' file
 *
 * g++-8 -std=c++17 -fconcepts -DUSE_LAPACK -I/usr/local/opt/lapack/include -L/usr/local/opt/lapack/lib -o la la.cpp -llapacke
 */

#include <iostream>
#include <vector>

#include "../../libs/_cxx20/math_algos.h"
using namespace m_ops;


//using t_real = double;
using t_real = float;
using t_vec = std::vector<t_real>;
using t_mat = m::mat<t_real, std::vector>;


int main()
{
	auto M = m::create<t_mat>({1, 2, 3, 3, 2, 6, 4, 2, 4});
	std::cout << "M = " << M << std::endl;

	{
		auto [Q, R] = m::qr<t_mat, t_vec>(M);
		std::cout << "\nQ = " << Q << std::endl;
		std::cout << "R = " << R << std::endl;
		std::cout << "QR = " << Q*R << std::endl;
	}

	{
		auto [ok, Q, R] = m_la::qr<t_mat>(M);
		std::cout << "\nok = " << std::boolalpha << ok << std::endl;
		std::cout << "Q = " << Q << std::endl;
		std::cout << "R = " << R << std::endl;
		std::cout << "QR = " << Q*R << std::endl;
	}

	return 0;
}
