/**
 * lapack test
 * @author Tobias Weber <tweber@ill.fr>
 * @date feb-19
 * @license GPLv3, see 'LICENSE' file
 *
 * g++-8 -std=c++17 -fconcepts -DUSE_LAPACK -I/usr/include/lapacke -I/usr/local/opt/lapack/include -L/usr/local/opt/lapack/lib -o la la.cpp -llapacke
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
	auto M = m::create<t_mat>({1, 2, 3, 3, 2, 6, 4, 2, 4});
	std::cout << "M = " << M << std::endl;

	{
		auto [ok, Q, R] = m_la::qr<t_mat>(M);
		std::cout << "\nok = " << std::boolalpha << ok << std::endl;
		std::cout << "Q = " << Q << std::endl;
		std::cout << "R = " << R << std::endl;
		std::cout << "QR = " << Q*R << std::endl;
	}


	{
		auto Z = m::create<t_mat_cplx>({1, 2, 3, 3, 2, 6, 4, 2, 4});

		auto [ok, evals, evecs] = m_la::eigenvec<t_mat_cplx, t_vec_cplx>(Z, 0, 0, 1);
		std::cout << "\nok = " << std::boolalpha << ok << std::endl;
		for(std::size_t i=0; i<evals.size(); ++i)
			std::cout << "eval: " << evals[i] << ", evec: " << evecs[i] << std::endl;


		auto [ok2, U, Vh, vals] = m_la::singval<t_mat_cplx>(Z);
		std::cout << "\nok = " << std::boolalpha << ok2 << std::endl;
		std::cout << "singvals: ";
		for(std::size_t i=0; i<vals.size(); ++i)
			std::cout << vals[i] << " ";
		std::cout << std::endl;
		std::cout << "U = " << U << "\nVh = " << Vh << std::endl;

		std::cout << "diag{vals} * UVh = " << U*m::diag<t_mat_cplx>(vals)*Vh << std::endl;


		auto [inva, ok3a] = m_la::pseudoinv<t_mat_cplx>(Z);
		auto [invb, ok3b] = m::inv<t_mat_cplx>(Z);
		std::cout << "\nok = " << std::boolalpha << ok3a << ", " << ok3b << std::endl;
		std::cout << "pseudoinv = " << inva << std::endl;
		std::cout << "      inv  = " << invb << std::endl;
 	}

	{
		auto Z = m::create<t_mat>({1, 2, 3, 3, 2, 6, 4, 2, 4});

		auto [ok, evals_re, evals_im, evecs_re, evecs_im] = m_la::eigenvec<t_mat, t_vec>(Z, 0, 0, 1);
		std::cout << "\nok = " << std::boolalpha << ok << std::endl;
		for(std::size_t i=0; i<evals_re.size(); ++i)
			std::cout << "eval: " << evals_re[i] << " + i*" << evals_im[i] 
				<< ", evec: " << evecs_re[i] << " +i*" << evecs_im[i] << std::endl;


		auto [ok2, U, Vt, vals] = m_la::singval<t_mat>(Z);
		std::cout << "\nok = " << std::boolalpha << ok2 << std::endl;
		std::cout << "singvals: ";
		for(std::size_t i=0; i<vals.size(); ++i)
			std::cout << vals[i] << " ";
		std::cout << std::endl;
		std::cout << "U = " << U << "\nVt = " << Vt << std::endl;

		std::cout << "diag{vals} * UVt = " << U*m::diag<t_mat>(vals)*Vt << std::endl;


		auto [inva, ok3a] = m_la::pseudoinv<t_mat>(Z);
		auto [invb, ok3b] = m::inv<t_mat>(Z);
		std::cout << "\nok = " << std::boolalpha << ok3a << ", " << ok3b << std::endl;
		std::cout << "pseudoinv = " << inva << std::endl;
		std::cout << "      inv  = " << invb << std::endl;
	}

	return 0;
}
