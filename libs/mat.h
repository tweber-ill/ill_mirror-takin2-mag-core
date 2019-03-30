/**
 * tlibs2
 * math library
 * @author Tobias Weber <tobias.weber@tum.de>, <tweber@ill.fr>
 * @date 2013 -- 2018
 * @license GPLv3, see 'LICENSE' file
 * @desc Forked on 7-Nov-2018 from the privately and TUM-PhD-developed "tlibs" project (https://github.com/t-weber/tlibs).
 */

#ifndef __TLIBS2_MATH_H__
#define __TLIBS2_MATH_H__

//#define USE_LINALG_OPS
//#define USE_FADDEEVA
//#define USE_LAPACK

#include "log.h"
#include "str.h"
#include "traits.h"

#include <initializer_list>
#include <cmath>
#include <cstdint>
#include <memory>
#include <complex>
#include <algorithm>
#include <functional>
#include <numeric>
#include <utility>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <limits>
#include <iostream>
#include <sstream>

#include <boost/algorithm/minmax_element.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector_expression.hpp>
#include <boost/numeric/ublas/matrix_expression.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/exception.hpp>
#include <boost/math/quaternion.hpp>
#include <boost/math/special_functions/factorials.hpp>
#include <boost/math/special_functions/spherical_harmonic.hpp>
#include <boost/math/special_functions/binomial.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/distributions/normal.hpp>
#include <boost/math/distributions/lognormal.hpp>
#include <boost/math/distributions/cauchy.hpp>
#include <boost/math/distributions/poisson.hpp>
#include <boost/math/distributions/binomial.hpp>
#include <boost/math/distributions/hypergeometric.hpp>
#include <boost/math/distributions/chi_squared.hpp>
#include <boost/math/distributions/students_t.hpp>
#include <boost/math/distributions/fisher_f.hpp>
#include <boost/math/distributions/exponential.hpp>
#include <boost/math/distributions/beta.hpp>
#include <boost/math/distributions/gamma.hpp>
#include <boost/math/distributions/logistic.hpp>
#include <boost/version.hpp>


#if BOOST_VERSION >= 106600
	#include <boost/integer/common_factor_rt.hpp>
	namespace integer = boost::integer;
#else
	#include <boost/math/common_factor_rt.hpp>
	namespace integer = boost::math;
#endif

namespace math = boost::math;
namespace ublas = boost::numeric::ublas;


#ifdef USE_FADDEEVA
	#include <Faddeeva.hh>
	using t_real_fadd = double;
#endif


#ifdef USE_LAPACK
	extern "C"
	{
		#define lapack_complex_float std::complex<float>
		#define lapack_complex_float_real(c) (c.real())
		#define lapack_complex_float_imag(c) (c.imag())
		#define lapack_complex_double std::complex<double>
		#define lapack_complex_double_real(c) (c.real())
		#define lapack_complex_double_imag(c) (c.imag())

		#include <lapacke.h>
	}
#endif


namespace tl2 {


template<typename T=double> constexpr T pi = math::constants::pi<typename get_scalar_type<T>::value_type>();

template<typename INT=int> bool is_even(INT i) { return (i%2 == 0); }
template<typename INT=int> bool is_odd(INT i) { return !is_even<INT>(i); }

template<class T=double> constexpr T r2d(T rad) { return rad/pi<T>*T(180); }	// rad -> deg
template<class T=double> constexpr T d2r(T deg) { return deg/T(180)*pi<T>; }	// deg -> rad
template<class T=double> constexpr T r2m(T rad) { return rad/pi<T>*T(180*60); }	// rad -> min
template<class T=double> constexpr T m2r(T min) { return min/T(180*60)*pi<T>; }	// min -> rad


template<typename T>
T sign(T t)
{
	if(t<0.) return -T(1);
	return T(1);
}


template<typename T> T cot(T t)
{
	return std::tan(T(0.5)*pi<T> - t);
}

template<typename T> T coth(T t)
{
	return T(1) / std::tanh(t);
}


template<class T=double>
struct _get_epsilon_impl
{
	using t_eps = underlying_value_type_t<T>;

	t_eps operator()() const
	{
		return std::numeric_limits<t_eps>::epsilon();
	}
};

template<class T>
typename _get_epsilon_impl<T>::t_eps get_epsilon()
{
	return _get_epsilon_impl<T>()();
}


template<class T = double, class t_eps = typename _get_epsilon_impl<T>::t_eps,
	LinalgType ty = get_linalg_type<T>::value>
struct _float_equal_impl
{
	bool operator()(T t1, T t2, t_eps eps = get_epsilon<T>()) const
	{
		return std::abs<T>(t1-t2) < eps;
	}
};

template<class T, class t_eps>
struct _float_equal_impl<T, t_eps, LinalgType::COMPLEX>
{
	bool operator()(const T& t1, const T& t2, t_eps eps = get_epsilon<T>()) const
	{
		return std::abs<t_eps>(t1.real()-t2.real()) < eps &&
			std::abs<t_eps>(t1.imag()-t2.imag()) < eps;
	}
};

template<typename T = double>
bool float_equal(T t1, T t2, typename _get_epsilon_impl<T>::t_eps eps = get_epsilon<T>())
{
	return _float_equal_impl<T>()(t1, t2, eps);
}


template<class T=double>
bool is_integer(T d, T eps = get_epsilon<T>())
{
	T d_rd = d - std::round(d);
	return float_equal<T>(d_rd, T(0), eps);
}

// -----------------------------------------------------------------------------


template<class T, typename REAL=double>
T lerp(const T& a, const T& b, REAL val)
{
	return a + T((b-a)*val);
}


// x=0..1
template<typename T=double>
T linear_interp(T x0, T x1, T x)
{
	return lerp<T,T>(x0, x1, x);
}


// x=0..1, y=0..1
template<typename T=double>
T bilinear_interp(T x0y0, T x1y0, T x0y1, T x1y1, T x, T y)
{
	T top = linear_interp<T>(x0y1, x1y1, x);
	T bottom = linear_interp<T>(x0y0, x1y0, x);

	return linear_interp<T>(bottom, top, y);
}


template<typename T=double, typename REAL=double,
	template<class...> class t_vec=std::vector>
t_vec<T> linspace(const T& tmin, const T& tmax, std::size_t iNum)
{
	t_vec<T> vec;
	vec.reserve(iNum);

	for(std::size_t i=0; i<iNum; ++i)
		vec.push_back(lerp<T,REAL>(tmin, tmax, REAL(i)/REAL(iNum-1)));
	return vec;
}

template<typename T=double, typename REAL=double,
	template<class...> class t_vec=std::vector>
t_vec<T> logspace(const T& tmin, const T& tmax, std::size_t iNum, T tBase=T(10))
{
	t_vec<T> vec = linspace<T, REAL>(tmin, tmax, iNum);

	for(T& t : vec)
		t = std::pow(tBase, t);
	return vec;
}

template<typename T>
T clamp(T t, T min, T max)
{
	if(t < min) t = min;
	if(t > max) t = max;

	return t;
}


template<class T>
bool is_in_range(T val, T centre, T pm)
{
	pm = std::abs(pm);

	if(val < centre-pm) return false;
	if(val > centre+pm) return false;
	return true;
}

// -----------------------------------------------------------------------------


template<typename T=double>
T log(T tbase, T tval)
{
	return T(std::log(tval)/std::log(tbase));
}

template<typename T=double>
T nextpow(T tbase, T tval)
{
	return T(std::pow(tbase, std::ceil(log(tbase, tval))));
}


// -----------------------------------------------------------------------------

template<class T=double> static constexpr T SIGMA2FWHM = T(2)*std::sqrt(T(2)*std::log(T(2)));
template<class T=double> static constexpr T SIGMA2HWHM = std::sqrt(T(2)*std::log(T(2)));
template<class T=double> static constexpr T FWHM2SIGMA = T(1)/SIGMA2FWHM<T>;
template<class T=double> static constexpr T HWHM2SIGMA = T(1)/SIGMA2HWHM<T>;


template<class T=double>
T gauss_model(T x, T x0, T sigma, T amp, T offs)
{
	T norm = T(1)/(std::sqrt(T(2)*pi<T>) * sigma);
	return amp * norm * std::exp(-0.5 * ((x-x0)/sigma)*((x-x0)/sigma)) + offs;
}

template<class T=double>
T gauss_model_amp(T x, T x0, T sigma, T amp, T offs)
{
	return amp * std::exp(-0.5 * ((x-x0)/sigma)*((x-x0)/sigma)) + offs;
}

template<class T=double>
T lorentz_model_amp(T x, T x0, T hwhm, T amp, T offs)
{
	return amp*hwhm*hwhm / ((x-x0)*(x-x0) + hwhm*hwhm) + offs;
}

template<class T=double>
T gauss_model_amp_slope(T x, T x0, T sigma, T amp, T offs, T slope)
{
	return amp * std::exp(-0.5 * ((x-x0)/sigma)*((x-x0)/sigma)) + (x-x0)*slope + offs;
}

template<class T=double>
T lorentz_model_amp_slope(T x, T x0, T hwhm, T amp, T offs, T slope)
{
	return amp*hwhm*hwhm / ((x-x0)*(x-x0) + hwhm*hwhm) + (x-x0)*slope + offs;
}


template<class T=double>
T parabola_model(T x, T x0, T amp, T offs)
{
	return amp*(x-x0)*(x-x0) + offs;
}

template<class T=double>
T parabola_model_slope(T x, T x0, T amp, T offs, T slope)
{
	return amp*(x-x0)*(x-x0) + (x-x0)*slope + offs;
}


// -----------------------------------------------------------------------------
template<class t_real_to, class t_real_from,
	bool bIsEqu = std::is_same<t_real_from, t_real_to>::value>
struct complex_cast
{
	const std::complex<t_real_to>& operator()(const std::complex<t_real_from>& c) const
	{ return c; }
};

template<class t_real_to, class t_real_from>
struct complex_cast<t_real_to, t_real_from, 0>
{
	std::complex<t_real_to> operator()(const std::complex<t_real_from>& c) const
	{ return std::complex<t_real_to>(t_real_to(c.real()), t_real_to(c.imag())); }
};
// -----------------------------------------------------------------------------


#ifdef USE_FADDEEVA

/**
* Complex error function
*/
template<class T=double>
std::complex<T> erf(const std::complex<T>& z)
{
	complex_cast<t_real_fadd, T> cst;
	complex_cast<T, t_real_fadd> inv_cst;
	return inv_cst(::Faddeeva::erf(cst(z)));
}

/**
* Complex complementary error function
*/
template<class T=double>
std::complex<T> erfc(const std::complex<T>& z)
{
	complex_cast<t_real_fadd, T> cst;
	complex_cast<T, t_real_fadd> inv_cst;
	return inv_cst(::Faddeeva::erfc(cst(z)));
}

/**
* Faddeeva function
*/
template<class T=double>
std::complex<T> faddeeva(const std::complex<T>& z)
{
	std::complex<T> i(0, 1.);
	return std::exp(-z*z) * erfc(-i*z);
}

/**
* Voigt profile
* see e.g.: https://en.wikipedia.org/wiki/Voigt_profile
*/
template<class T=double>
T voigt_model(T x, T x0, T sigma, T gamma, T amp, T offs)
{
	T norm = T(1)/(std::sqrt(T(2)*pi<T>) * sigma);
	std::complex<T> z = std::complex<T>(x-x0, gamma) / (sigma * std::sqrt(T(2)));

	return amp*norm * faddeeva<T>(z).real() + offs;
}

template<class T=double>
T voigt_model_amp(T x, T x0, T sigma, T gamma, T amp, T offs)
{
	std::complex<T> z = std::complex<T>(x-x0, gamma) / (sigma * std::sqrt(T(2)));
	return amp * faddeeva<T>(z).real() + offs;
}

template<class T=double>
T voigt_model_amp_slope(T x, T x0, T sigma, T gamma, T amp, T offs, T slope)
{
	std::complex<T> z = std::complex<T>(x-x0, gamma) / (sigma * std::sqrt(T(2)));
	return amp * faddeeva<T>(z).real() + (x-x0)*slope + offs;
}

#endif


// -----------------------------------------------------------------------------


// wrapper for boost's Y function
template<class T=double>
std::complex<T> Ylm(int l /*0..i*/, int m /*-l..l*/, T th /*0..pi*/, T ph /*0..2pi*/)
{
	return boost::math::spherical_harmonic<T,T>(l,m, th, ph);
}


// -----------------------------------------------------------------------------
// coordinate trafos

template<class T = double>
std::tuple<T,T,T> cart_to_sph(T x, T y, T z)
{
	T rho = std::sqrt(x*x + y*y + z*z);
	T phi = std::atan2(y, x);
	T theta = std::acos(z/rho);

	return std::make_tuple(rho, phi, theta);
}

template<class T = double>
std::tuple<T,T,T> sph_to_cart(T rho, T phi, T theta)
{
	T x = rho * std::cos(phi)*std::sin(theta);
	T y = rho * std::sin(phi)*std::sin(theta);
	T z = rho * std::cos(theta);

	return std::make_tuple(x, y, z);
}

template<class T = double>
std::tuple<T,T,T> cyl_to_sph(T rho_cyl, T phi_cyl, T z_cyl)
{
	T rho = std::sqrt(rho_cyl*rho_cyl + z_cyl*z_cyl);
	T theta = std::acos(z_cyl/rho);

	return std::make_tuple(rho, phi_cyl, theta);
}

template<class T = double>
std::tuple<T,T,T> sph_to_cyl(T rho_sph, T phi_sph, T theta_sph)
{
	T rho = rho_sph * std::sin(theta_sph);
	T z = rho_sph * std::cos(theta_sph);

	return std::make_tuple(rho, phi_sph, z);
}

template<class T = double>
std::tuple<T,T,T> cyl_to_cart(T rho, T phi, T z)
{
	T x = rho * std::cos(phi);
	T y = rho * std::sin(phi);

	return std::make_tuple(x, y, z);
}

template<class T = double>
std::tuple<T,T,T> cart_to_cyl(T x, T y, T z)
{
	T rho = std::sqrt(x*x + y*y);
	T phi = std::atan2(y, x);

	return std::make_tuple(rho, phi, z);
}



template<class T = double>
std::tuple<T,T> crys_to_sph(T twophi_crys, T twotheta_crys)
{
	// converts the out-of-plane scattering angle '2theta' to the spherical theta
	T theta_sph = pi<T>/T(2) - twotheta_crys;
	// converts in-plane scattering angle '2phi' to the spherical phi
	T phi_sph = twophi_crys - pi<T>/T(2);

	return std::make_tuple(phi_sph, theta_sph);
}

template<class T = double>
std::tuple<T,T> sph_to_crys(T phi, T theta)
{
	return crys_to_sph<T>(phi, theta);
}


/**
 * gnomonic projection (similar to perspective projection with fov=90°)
 * @return [x,y]
 * @desc: see http://mathworld.wolfram.com/GnomonicProjection.html
 */
template<class T = double>
std::tuple<T,T> gnomonic_proj(T twophi_crys, T twotheta_crys)
{
	T x = -std::tan(twophi_crys);
	T y = std::tan(twotheta_crys) / std::cos(twophi_crys);

	return std::make_tuple(x, y);
}

/**
 * stereographic projection
 * @return [x,y]
 * @desc: see http://mathworld.wolfram.com/StereographicProjection.html
 */
template<class T = double>
std::tuple<T,T> stereographic_proj(T twophi_crys, T twotheta_crys, T rad)
{
	const T sth = std::sin(twotheta_crys);
	const T cth = std::cos(twotheta_crys);
	const T sph = std::sin(twophi_crys);
	const T cph = std::cos(twophi_crys);

	T x = -T(2) * rad * sph * cth / (T(1) + cth*cph);
	T y = T(2) * rad * sth / (T(1) + cth*cph);

	return std::make_tuple(x, y);
}


// -----------------------------------------------------------------------------


/**
 * point contained in linear range?
 */
template<class T = double>
bool is_in_linear_range(T dStart, T dStop, T dPoint)
{
	if(dStop < dStart)
		std::swap(dStart, dStop);

	return (dPoint >= dStart) && (dPoint <= dStop);
}

/**
 * angle contained in angular range?
 */
template<class T = double>
bool is_in_angular_range(T dStart, T dRange, T dAngle)
{
	if(dStart < T(0)) dStart += T(2)*pi<T>;
	if(dAngle < T(0)) dAngle += T(2)*pi<T>;

	dStart = std::fmod(dStart, T(2)*pi<T>);
	dAngle = std::fmod(dAngle, T(2)*pi<T>);

	T dStop = dStart + dRange;


	// if the end point is contained in the circular range
	if(dStop < T(2)*pi<T>)
	{
		return is_in_linear_range<T>(dStart, dStop, dAngle);
	}
	else // else end point wraps around
	{
		return is_in_linear_range<T>(dStart, T(2)*pi<T>, dAngle) ||
			is_in_linear_range<T>(T(0), dRange-(T(2)*pi<T>-dStart), dAngle);
	}
}

// -----------------------------------------------------------------------------



template<class matrix_type = ublas::matrix<double>>
typename matrix_type::value_type determinant(const matrix_type& mat);


// ----------------------------------------------------------------------------

/**
 * creates a vector
 */
template<class t_vec = ublas::vector<double>,
	template<class...> class t_lst = std::initializer_list>
t_vec make_vec(t_lst<typename t_vec::value_type>&& lst)
{
	using T = typename t_vec::value_type;
	using t_iter = typename t_lst<T>::const_iterator;

	t_vec vec(lst.size());

	std::size_t i=0;
	for(t_iter iter=lst.begin(); iter!=lst.end(); ++i, ++iter)
		vec[i] = std::move(*iter);

	return vec;
}

/**
 * creates a vector
 */
template<class t_vec = ublas::vector<double>,
	template<class...> class t_lst = std::initializer_list>
t_vec make_vec(const t_lst<typename t_vec::value_type>& lst)
{
	using T = typename t_vec::value_type;
	using t_iter = typename t_lst<T>::const_iterator;

	t_vec vec(lst.size());

	std::size_t i=0;
	for(t_iter iter=lst.begin(); iter!=lst.end(); ++i, ++iter)
		vec[i] = *iter;

	return vec;
}

/**
 * creates a matrix
 */
template<class t_mat = ublas::matrix<double>,
	template<class...> class t_lst = std::initializer_list>
t_mat make_mat(t_lst<t_lst<typename t_mat::value_type>>&& lst)
{
	using T = typename t_mat::value_type;

	const std::size_t I = lst.size();
	const std::size_t J = lst.begin()->size();

	t_mat mat(I, J);
	typename t_lst<t_lst<T>>::const_iterator iter = lst.begin();

	for(std::size_t i=0; i<I; ++i, ++iter)
	{
		typename t_lst<T>::const_iterator iterinner = iter->begin();
		for(std::size_t j=0; j<J; ++j, ++iterinner)
		{
			mat(i,j) = std::move(*iterinner);
		}
	}

	return mat;
}


/**
 * unit matrix -- general version
 */
template<class t_mat = ublas::matrix<double>,
	typename std::enable_if<!std::is_convertible<t_mat, ublas::matrix<typename t_mat::value_type>>::value, char>::type=0>
t_mat unit_m(std::size_t N)
{
	t_mat mat(N, N);
	for(std::size_t i=0; i<N; ++i)
		for(std::size_t j=0; j<N; ++j)
			mat(i,j) = (i==j ? 1 : 0);
	return mat;
}

/**
 * unit matrix -- ublas wrapper
 */
template<class t_mat = ublas::matrix<double>,
typename std::enable_if<std::is_convertible<t_mat, ublas::matrix<typename t_mat::value_type>>::value, char>::type=0>
t_mat unit_m(std::size_t N)
{
	return ublas::identity_matrix<typename t_mat::value_type>(N);
}


/**
 * zero matrix -- general version
 */
template<class t_mat = ublas::matrix<double>,
typename std::enable_if<!std::is_convertible<t_mat, ublas::matrix<typename t_mat::value_type>>::value, char>::type=0>
t_mat zero_m(std::size_t N, std::size_t M)
{
	t_mat mat(N, M);
	for(std::size_t i=0; i<N; ++i)
		for(std::size_t j=0; j<M; ++j)
			mat(i,j) = 0;
	return mat;
}

/**
 * zero matrix -- ublas wrapper
 */
template<class t_mat = ublas::matrix<double>,
typename std::enable_if<std::is_convertible<t_mat, ublas::matrix<typename t_mat::value_type>>::value, char>::type=0>
t_mat zero_m(std::size_t N, std::size_t M)
{
	return ublas::zero_matrix<typename t_mat::value_type>(N, M);
}

/**
 * zero matrix -- synonym
 */
template<class t_mat = ublas::matrix<double>>
t_mat zero_matrix(std::size_t N, std::size_t M)
{ return zero_m<t_mat>(N, M); }


/**
 * zero vector -- general version
 */
template<class t_vec = ublas::vector<double>,
	typename std::enable_if<!std::is_convertible<t_vec, ublas::vector<typename t_vec::value_type>>::value, char>::type=0>
t_vec zero_v(std::size_t N)
{
	t_vec vec(N);
	for(std::size_t i=0; i<N; ++i)
		vec(i) = 0;
	return vec;
}

/**
 * zero vector -- ublas wrapper
 */
template<class t_vec = ublas::vector<double>,
	typename std::enable_if<std::is_convertible<t_vec, ublas::vector<typename t_vec::value_type>>::value, char>::type=0>
t_vec zero_v(std::size_t N)
{
	return ublas::zero_vector<typename t_vec::value_type>(N);
}

/**
 * zero vector -- synonym
 */
template<class t_vec = ublas::vector<double>>
t_vec zero_vector(std::size_t N)
{ return zero_v<t_vec>(N); }



/**
 * create a vector of size N filled with value val
 */
template<class vector_type = ublas::vector<double>>
vector_type fill_vector(std::size_t N, typename vector_type::value_type val)
{
	vector_type vec(N);
	for(std::size_t i=0; i<N; ++i)
		vec[i] = val;
	return vec;
}


/**
 * resize matrix, filling up with unity
 */
template<class t_mat = ublas::matrix<double>>
void resize_unity(t_mat& mat, std::size_t N)
{
	const std::size_t iOldSize1 = mat.size1();
	const std::size_t iOldSize2 = mat.size2();

	mat.resize(N,N, true);

	for(std::size_t i=0; i<N; ++i)
		for(std::size_t j=0; j<N; ++j)
		{
			if(i<iOldSize1 && j<iOldSize2) continue;
			mat(i,j) = (i==j ? 1 : 0);
		}
}


/**
 * converts vector t_vec<t_from> to t_vec<t_to>
 */
template<class t_from, class t_to, template<class...> class t_vec = ublas::vector>
t_vec<t_to> convert_vec(const t_vec<t_from>& vec)
{
	//using t_vec_from = t_vec<t_from>;
	using t_vec_to = t_vec<t_to>;

	t_vec_to vecRet(vec.size());

	for(std::size_t i=0; i<vec.size(); ++i)
		vecRet[i] = t_to(vec[i]);

	return vecRet;
}

/**
 * converts vector t_vec_from<t_from> to t_vec_to<t_to>
 */
template<class t_from, class t_to,
	template<class...> class t_vec_from = std::vector,
	template<class...> class t_vec_to = ublas::vector>
t_vec_to<t_to> convert_vec_full(const t_vec_from<t_from>& vec)
{
	t_vec_to<t_to> vecRet(vec.size());

	for(std::size_t i=0; i<vec.size(); ++i)
		vecRet[i] = t_to(vec[i]);

	return vecRet;
}

// ----------------------------------------------------------------------------


template<class vec_type>
bool vec_equal(const vec_type& vec0, const vec_type& vec1,
	typename _get_epsilon_impl<vec_type>::t_eps eps = get_epsilon<vec_type>())
{
	typedef typename vec_type::value_type T;

	if(vec0.size() != vec1.size())
		return false;

	for(std::size_t i=0; i<vec0.size(); ++i)
		if(!float_equal<T>(vec0[i], vec1[i], eps))
			return false;
	return true;
}

template<class mat_type>
bool mat_equal(const mat_type& mat0, const mat_type& mat1,
	typename _get_epsilon_impl<mat_type>::t_eps eps = get_epsilon<mat_type>())
{
	typedef typename mat_type::value_type T;

	if(mat0.size1() != mat1.size1() || mat0.size2() != mat1.size2())
		return false;

	for(std::size_t i=0; i<mat0.size1(); ++i)
		for(std::size_t j=0; j<mat0.size2(); ++j)
			if(!float_equal<T>(mat0(i,j), mat1(i,j), eps))
				return false;
	return true;
}

// ----------------------------------------------------------------------------


/**
 * transpose -- general version
 */
template<typename t_mat = ublas::matrix<double>,
	typename std::enable_if<!std::is_convertible<t_mat, ublas::matrix<typename t_mat::value_type>>::value, char>::type=0>
t_mat transpose(const t_mat& mat)
{
	t_mat matret(mat.size2(), mat.size1());

	for(std::size_t i=0; i<mat.size1(); ++i)
		for(std::size_t j=0; j<mat.size2(); ++j)
			matret(j,i) = mat(i,j);

	return matret;
}

/**
 * transpose -- general version
 */
template<typename t_mat = ublas::matrix<double>,
	typename std::enable_if<std::is_convertible<t_mat, ublas::matrix<typename t_mat::value_type>>::value, char>::type=0>
t_mat transpose(const t_mat& mat)
{
	return ublas::trans(mat);
}


/**
 * conjugate matrix
 */
template<typename t_mat = ublas::matrix<std::complex<double>>>
t_mat conjugate_mat(t_mat mat)
{
	for(std::size_t i=0; i<mat.size1(); ++i)
		for(std::size_t j=0; j<mat.size2(); ++j)
			mat(i,j) = std::conj(mat(i,j));
	return mat;
}


/**
 * hermitian conjugated matrix
 */
template<typename t_mat = ublas::matrix<std::complex<double>>>
t_mat hermitian(const t_mat& mat)
{
	t_mat matret = transpose<t_mat>(mat);
	matret = conjugate_mat<t_mat>(matret);
	return matret;
}


/**
 * conjugate vector
 */
template<typename t_vec = ublas::vector<std::complex<double>>>
t_vec conjugate_vec(t_vec vec)
{
	for(std::size_t i=0; i<vec.size(); ++i)
		vec[i] = std::conj(vec[i]);
	return vec;
}


// ----------------------------------------------------------------------------


/**
 * cross product, c_i = eps_ijk a_j b_k
 */
template<typename vector_type = ublas::vector<double>>
vector_type cross_3(const vector_type& vec0, const vector_type& vec1)
{
	return make_vec<vector_type>
	({
		vec0[1]*vec1[2] - vec1[1]*vec0[2],
		vec0[2]*vec1[0] - vec1[2]*vec0[0],
		vec0[0]*vec1[1] - vec1[0]*vec0[1]
	});
}


/**
 * inner product -- general version
 */
template<typename t_vec = ublas::vector<double>,
	typename std::enable_if<!std::is_convertible<t_vec, ublas::vector<typename t_vec::value_type>>::value, char>::type=0>
typename t_vec::value_type inner(const t_vec& vec0, const t_vec& vec1)
{
	typename t_vec::value_type d(0);
	std::size_t iSize = std::min(vec0.size(), vec1.size());

	for(std::size_t i=0; i<iSize; ++i)
		d += vec0[i]*vec1[i];
	return d;
}


/**
 * complex inner product -- general version
 */
template<typename t_vec = ublas::vector<std::complex<double>>,
	typename std::enable_if<!std::is_convertible<t_vec, ublas::vector<typename t_vec::value_type>>::value, char>::type=0>
typename t_vec::value_type inner_cplx(const t_vec& vec0, const t_vec& vec1)
{
	typename t_vec::value_type d(0);
	std::size_t iSize = std::min(vec0.size(), vec1.size());

	for(std::size_t i=0; i<iSize; ++i)
		d += std::conj(vec0[i])*vec1[i];
	return d;
}


/**
 * inner product -- ublas wrapper
 */
template<typename t_vec = ublas::vector<double>,
	typename std::enable_if<std::is_convertible<t_vec, ublas::vector<typename t_vec::value_type>>::value, char>::type=0>
typename t_vec::value_type inner(const t_vec& vec0, const t_vec& vec1)
{
	return ublas::inner_prod(vec0, vec1);
}


/**
 * complex inner product -- ublas wrapper
 */
template<typename t_vec = ublas::vector<std::complex<double>>,
	typename std::enable_if<std::is_convertible<t_vec, ublas::vector<typename t_vec::value_type>>::value, char>::type=0>
typename t_vec::value_type inner_cplx(const t_vec& vec0, const t_vec& vec1)
{
	t_vec vec0_c = conjugate_vec<t_vec>(vec0);
	return ublas::inner_prod(vec0_c, vec1);
}


/**
 * outer product -- general version
 */
template<typename t_vec = ublas::vector<double>,
	typename t_mat = ublas::matrix<typename t_vec::value_type>,
	typename std::enable_if<!std::is_convertible<t_vec, ublas::vector<typename t_vec::value_type>>::value, char>::type=0>
t_mat outer(const t_vec& vec0, const t_vec& vec1)
{
	std::size_t iSize = std::min(vec0.size(), vec1.size());
	t_mat mat(iSize, iSize);

	for(std::size_t i=0; i<iSize; ++i)
		for(std::size_t j=0; j<iSize; ++j)
			mat(i,j) = vec0[i]*vec1[j];

	return mat;
}


/**
 * complex outer product -- general version
 */
template<typename t_vec = ublas::vector<std::complex<double>>,
	typename t_mat = ublas::matrix<typename t_vec::value_type>,
	typename std::enable_if<!std::is_convertible<t_vec, ublas::vector<typename t_vec::value_type>>::value, char>::type=0>
t_mat outer_cplx(const t_vec& vec0, const t_vec& vec1)
{
	std::size_t iSize = std::min(vec0.size(), vec1.size());
	t_mat mat(iSize, iSize);

	for(std::size_t i=0; i<iSize; ++i)
		for(std::size_t j=0; j<iSize; ++j)
			mat(i,j) = vec0[i]*std::conj(vec1[j]);

	return mat;
}


/**
 * outer product -- ublas wrapper
 */
template<typename t_vec = ublas::vector<double>,
	typename t_mat = ublas::matrix<typename t_vec::value_type>,
	typename std::enable_if<std::is_convertible<t_vec, ublas::vector<typename t_vec::value_type>>::value, char>::type=0>
t_mat outer(const t_vec& vec0, const t_vec& vec1)
{
	return ublas::outer_prod(vec0, vec1);
}


/**
 * complex outer product -- ublas wrapper
 */
template<typename t_vec = ublas::vector<std::complex<double>>,
	typename t_mat = ublas::matrix<typename t_vec::value_type>,
	typename std::enable_if<std::is_convertible<t_vec, ublas::vector<typename t_vec::value_type>>::value, char>::type=0>
t_mat outer_cplx(const t_vec& vec0, const t_vec& vec1)
{
	t_vec vec1_c = conjugate_vec<t_vec>(vec1);
	return ublas::outer_prod(vec0, vec1_c);
}


/**
 * matrix-matrix product -- general version
 * c_ij = a_ik b_kj
 */
template<typename t_mat = ublas::matrix<double>,
typename std::enable_if<!std::is_convertible<t_mat, ublas::matrix<typename t_mat::value_type>>::value, char>::type=0>
t_mat prod_mm(const t_mat& mat0, const t_mat& mat1)
{
	if(mat0.size2() != mat1.size1())
		return t_mat(0,0);

	std::size_t iSize1 = mat0.size1();
	std::size_t iSize2 = mat1.size2();
	std::size_t iSize3 = mat0.size2();
	t_mat mat(iSize1, iSize2);

	for(std::size_t i=0; i<iSize1; ++i)
	{
		for(std::size_t j=0; j<iSize2; ++j)
		{
			mat(i,j) = typename t_mat::value_type(0);
			for(std::size_t k=0; k<iSize3; ++k)
				mat(i,j) += mat0(i,k)*mat1(k,j);
		}
	}

	return mat;
}

/**
 * matrix-matrix product -- ublas wrapper
 */
template<typename t_mat = ublas::matrix<double>,
typename std::enable_if<std::is_convertible<t_mat, ublas::matrix<typename t_mat::value_type>>::value, char>::type=0>
t_mat prod_mm(const t_mat& mat0, const t_mat& mat1)
{
	return ublas::prod(mat0, mat1);
}


/**
 * matrix-vector product -- general version
 * c_i = a_ij b_j
 */
template<typename t_vec = ublas::vector<double>,
typename t_mat = ublas::matrix<typename t_vec::value_type>,
typename std::enable_if<!std::is_convertible<t_vec, ublas::vector<typename t_vec::value_type>>::value, char>::type=0>
t_vec prod_mv(const t_mat& mat, const t_vec& vec)
{
	if(mat.size2() != vec.size())
		return t_vec(0);

	std::size_t iSize1 = mat.size1();
	std::size_t iSize2 = mat.size2();
	t_vec vecret(iSize2);

	for(std::size_t i=0; i<iSize1; ++i)
	{
		vecret(i) = typename t_vec::value_type(0);
		for(std::size_t j=0; j<iSize2; ++j)
			vecret(i) += mat(i,j)*vec(j);
	}

	return vecret;
}

/**
 * matrix-vector product -- ublas wrapper
 */
template<typename t_vec = ublas::vector<double>,
typename t_mat = ublas::matrix<typename t_vec::value_type>,
typename std::enable_if<std::is_convertible<t_vec, ublas::vector<typename t_vec::value_type>>::value, char>::type=0>
t_vec prod_mv(const t_mat& mat, const t_vec& vec)
{
	return ublas::prod(mat, vec);
}


/**
 * vector-matrix product
 */
template<typename t_vec = ublas::vector<double>,
typename t_mat = ublas::matrix<typename t_vec::value_type>,
typename std::enable_if<std::is_convertible<t_vec, ublas::vector<typename t_vec::value_type>>::value, char>::type=0>
t_vec prod_vm(const t_vec& vec, const t_mat& mat)
{
	return prod_mv(transpose(mat), vec);
}



/**
 * 2-norm -- general version
 */
template<class t_vec = ublas::vector<double>,
	typename std::enable_if<!std::is_convertible<t_vec, ublas::vector<typename t_vec::value_type>>::value, char>::type=0>
typename t_vec::value_type veclen(const t_vec& vec)
{
	using T = typename t_vec::value_type;
	T len(0);

	for(std::size_t i=0; i<vec.size(); ++i)
		len += vec[i]*vec[i];

	return std::sqrt(len);
}

/**
 * 2-norm -- ublas wrapper
 */
template<class t_vec = ublas::vector<double>,
	typename std::enable_if<std::is_convertible<t_vec, ublas::vector<typename t_vec::value_type>>::value, char>::type=0>
typename t_vec::value_type veclen(const t_vec& vec)
{
	return ublas::norm_2(vec);
}


/**
 * matrix element <x|M|y>
 */
template<typename t_mat = ublas::matrix<std::complex<double>>,
	typename t_vec = ublas::vector<std::complex<double>>>
typename t_vec::value_type
mat_elem(const t_vec& x, const t_mat& M, const t_vec& y)
{
	t_vec My = prod_mv<t_vec, t_mat>(M, y);
	t_vec x_conj = conjugate_vec<t_vec>(x);

	return inner<t_vec>(x_conj, My);
}

// ----------------------------------------------------------------------------


/**
 * remove an element from a vector
 */
template<class vector_type>
vector_type remove_elem(const vector_type& vec, std::size_t iIdx)
{
	vector_type vecret(vec.size()-1);

	for(std::size_t i=0, j=0; i<vec.size() && j<vecret.size();)
	{
		vecret[j] = vec[i];

		if(i!=iIdx) ++j;
		++i;
	}

	return vecret;
}

/**
 * create a submatrix removing row iRow and column iCol
 */
template<class matrix_type>
matrix_type submatrix(const matrix_type& mat, std::size_t iRow, std::size_t iCol)
{
	matrix_type matret(mat.size1()-1, mat.size2()-1);

	for(std::size_t i=0, i0=0; i<mat.size1() && i0<matret.size1();)
	{
		for(std::size_t j=0, j0=0; j<mat.size2() && j0<matret.size2();)
		{
			matret(i0,j0) = mat(i,j);

			if(j!=iCol) ++j0;
			++j;
		}

		if(i!=iRow) ++i0;
		++i;
	}

	return matret;
}


/**
 * create a submatrix
 */
template<class matrix_type>
matrix_type submatrix_wnd(const matrix_type& mat, std::size_t iSubRows, std::size_t iSubCols,
	std::size_t iBeginRow=0, std::size_t iBeginCol=0)
{
	matrix_type matret(iSubRows, iSubCols);

	for(std::size_t i=0; i<iSubRows; ++i)
		for(std::size_t j=0; j<iSubCols; ++j)
			matret(i, j) = mat(i+iBeginRow, j+iBeginCol);

	return matret;
}


template<class matrix_type>
matrix_type remove_column(const matrix_type& mat, std::size_t iCol)
{
	matrix_type matret(mat.size1(), mat.size2()-1);
	for(std::size_t i=0; i<mat.size1(); ++i)
	{
		for(std::size_t j=0, j0=0; j<mat.size2() && j0<matret.size2(); ++j)
		{
			matret(i,j0) = mat(i,j);
                        if(j!=iCol) ++j0;
		}
	}
	return matret;
}

template<class matrix_type>
void submatrix_copy(matrix_type& mat, const matrix_type& sub,
	std::size_t iRowBegin, std::size_t iColBegin)
{
	for(std::size_t i=0; i<sub.size1(); ++i)
		for(std::size_t j=0; j<sub.size2(); ++j)
			mat(iRowBegin+i, iColBegin+j) = sub(i,j);
}

template<class vec_type>
void subvector_copy(vec_type& vec, const vec_type& sub, std::size_t iRowBegin)
{
	for(std::size_t i=0; i<sub.size(); ++i)
		vec[iRowBegin+i] = sub[i];
}


template<class matrix_type>
matrix_type remove_elems(const matrix_type& mat, std::size_t iIdx)
{
	return submatrix(mat, iIdx, iIdx);
}


/**
 * set matrix column
 */
template<class t_vec = ublas::vector<double>,
	class t_mat = ublas::matrix<typename t_vec::value_type>>
void set_column(t_mat& M, std::size_t iCol, const t_vec& vec)
{
	std::size_t s = std::min(vec.size(), M.size1());
	for(std::size_t i=0; i<s; ++i)
		M(i, iCol) = vec[i];
}

/**
 * set matrix row
 */
template<class t_vec = ublas::vector<double>,
	class t_mat = ublas::matrix<typename t_vec::value_type>>
void set_row(t_mat& M, std::size_t iRow, const t_vec& vec)
{
	std::size_t s = std::min(vec.size(), M.size2());
	for(std::size_t i=0; i<s; ++i)
		M(iRow, i) = vec[i];
}


/**
 * get matrix column -- general version
 */
template<class vector_type = ublas::vector<double>,
	class matrix_type = ublas::matrix<typename vector_type::value_type>,
	typename std::enable_if<!std::is_convertible<matrix_type, ublas::matrix<typename vector_type::value_type>>::value, char>::type=0>
vector_type get_column(const matrix_type& mat, std::size_t iCol)
{
	vector_type vecret(mat.size1());

	for(std::size_t i=0; i<mat.size1(); ++i)
		vecret[i] = mat(i, iCol);

	return vecret;
}

/**
 * get matrix column -- ublas wrapper
 */
template<class vector_type = ublas::vector<double>,
	class matrix_type = ublas::matrix<typename vector_type::value_type>,
	typename std::enable_if<std::is_convertible<matrix_type, ublas::matrix<typename vector_type::value_type>>::value, char>::type=0>
vector_type get_column(const matrix_type& mat, std::size_t iRow)
{
	return vector_type(ublas::column(mat, iRow));
}

/**
 * get matrix row -- general version
 */
template<class vector_type = ublas::vector<double>,
	class matrix_type = ublas::matrix<typename vector_type::value_type>,
	typename std::enable_if<!std::is_convertible<matrix_type, ublas::matrix<typename vector_type::value_type>>::value, char>::type=0>
vector_type get_row(const matrix_type& mat, std::size_t iRow)
{
	vector_type vecret(mat.size2());

	for(std::size_t i=0; i<mat.size2(); ++i)
		vecret[i] = mat(iRow, i);

	return vecret;
}

/**
 * get matrix row -- ublas wrapper
 */
template<class vector_type = ublas::vector<double>,
	class matrix_type = ublas::matrix<typename vector_type::value_type>,
	typename std::enable_if<std::is_convertible<matrix_type, ublas::matrix<typename vector_type::value_type>>::value, char>::type=0>
vector_type get_row(const matrix_type& mat, std::size_t iRow)
{
	return vector_type(ublas::row(mat, iRow));
}


template<class vector_type = ublas::vector<double>,
	class matrix_type = ublas::matrix<typename vector_type::value_type>,
	class cont_type = std::vector<vector_type>>
cont_type get_columns(const matrix_type& mat)
{
	cont_type vec;
	vec.reserve(mat.size2());

	for(std::size_t i=0; i<mat.size2(); ++i)
		vec.push_back(get_column(mat, i));

	return vec;
}


// ----------------------------------------------------------------------------


template<class t_mat = ublas::matrix<double>>
t_mat mirror_matrix(std::size_t iSize, std::size_t iComp)
{
	using T = typename t_mat::value_type;

	t_mat mat = unit_m<t_mat>(iSize);
	mat(iComp, iComp) = T(-1);

	return mat;
}


template<class matrix_type = ublas::matrix<double>>
matrix_type rotation_matrix_2d(typename matrix_type::value_type angle)
{
	typedef typename matrix_type::value_type T;

	T s, c;

	if(angle==0.)
	{
		s = T(0);
		c = T(1);
	}
	else
	{
		s = std::sin(angle);
		c = std::cos(angle);
	}

	return make_mat<matrix_type>
	({	{c, -s},
		{s,  c} });
}


/**
 * generates points in an arc defined by vec1 and vec2 at an angle phi around vec1
 */
template<class t_vec = ublas::vector<double>>
t_vec arc(const t_vec& vec1, const t_vec& vec2, underlying_value_type_t<t_vec> phi)
{
	//using t_real = underlying_value_type_t<t_vec>;
	return std::cos(phi)*vec1 + std::sin(phi)*vec2;
}

/**
 * generates points in a spherical shell
 */
template<class t_vec = ublas::vector<double>>
t_vec sph_shell(const t_vec& vec,
	underlying_value_type_t<t_vec> phi, underlying_value_type_t<t_vec> theta)
{
	using t_real = underlying_value_type_t<t_vec>;

	t_real rho, curphi, curtheta;
	std::tie(rho, curphi, curtheta) = cart_to_sph<t_real>(vec[0], vec[1], vec[2]);

	t_real x,y,z;
	std::tie(x,y,z) = sph_to_cart<t_real>(rho, curphi+phi, curtheta+theta);
	t_vec vecRet = make_vec<t_vec>({x,y,z});
	return vecRet;
}


template<class matrix_type = ublas::matrix<double>>
matrix_type rotation_matrix_3d_x(typename matrix_type::value_type angle)
{
	typedef typename matrix_type::value_type T;

	T s, c;
	if(angle==0.)
	{
		s = T(0);
		c = T(1);
	}
	else
	{
		s = std::sin(angle);
		c = std::cos(angle);
	}

	return make_mat<matrix_type>
	({	{1, 0,  0},
		{0, c, -s},
		{0, s,  c} });
}

template<class matrix_type = ublas::matrix<double>>
matrix_type rotation_matrix_3d_y(typename matrix_type::value_type angle)
{
	typedef typename matrix_type::value_type T;

	T s, c;
	if(angle==0.)
	{
		s = T(0);
		c = T(1);
	}
	else
	{
		s = std::sin(angle);
		c = std::cos(angle);
	}

	return make_mat<matrix_type>
	({	{c,  0, s},
		{0,  1, 0},
		{-s, 0, c} });
}

template<class matrix_type = ublas::matrix<double>>
matrix_type rotation_matrix_3d_z(typename matrix_type::value_type angle)
{
	typedef typename matrix_type::value_type T;

	T s, c;
	if(angle==0.)
	{
		s = T(0);
		c = T(1);
	}
	else
	{
		s = std::sin(angle);
		c = std::cos(angle);
	}

	return make_mat<matrix_type>
	({	{c, -s, 0},
		{s,  c, 0},
		{0,  0, 1} });
}


/**
 * cross-product in matrix form
 */
template<class matrix_type = ublas::matrix<double>,
	class vector_type = ublas::vector<typename matrix_type::value_type>>
matrix_type skew(const vector_type& vec)
{
	if(vec.size() == 3)
	{
		return make_mat<matrix_type>
		({	{       0, -vec[2],  vec[1] },
			{  vec[2],       0, -vec[0] },
			{ -vec[1],  vec[0],       0 } });
	}
	else
		throw Err("Skew only defined for three dimensions.");
}


/**
 * diagonal matrix
 */
template<class matrix_type = ublas::matrix<double>,
	class cont_type = std::initializer_list<typename matrix_type::value_type>>
matrix_type diag_matrix(const cont_type& lst)
{
	matrix_type mat = unit_m<matrix_type>(lst.size());

	std::size_t i = 0;
	for(typename cont_type::const_iterator iter=lst.begin(); iter!=lst.end(); ++iter, ++i)
		mat(i,i) = *iter;

	return mat;
}

/**
 * vector of diagonal matrix elements
 */
template<class t_vec = ublas::vector<double>,
	class t_mat = ublas::matrix<double>>
t_vec diag_vec(const t_mat& mat)
{
	std::size_t N = std::min(mat.size1(), mat.size2());

	t_vec vec(N);
	for(std::size_t i=0; i<N; ++i)
		vec[i] = mat(i,i);

	return vec;
}


template<class matrix_type = ublas::matrix<double>,
	class cont_type = std::initializer_list<typename matrix_type::value_type>>
matrix_type scale_matrix(const cont_type& lst)
{
	return diag_matrix<matrix_type, cont_type>(lst);
}

/**
 * translation matrix in homogeneous coords
 */
template<class t_mat = ublas::matrix<double>,
	class t_cont = std::initializer_list<typename t_mat::value_type>>
t_mat translation_matrix(const t_cont& lst)
{
	t_mat mat = unit_m<t_mat>(lst.size()+1);

	const std::size_t iJ = mat.size2();
	std::size_t i = 0;
	for(typename t_cont::const_iterator iter=lst.begin(); iter!=lst.end(); ++iter, ++i)
		mat(i, iJ-1) = *iter;

	return mat;
}

/**
 * is mat a translation matrix in homogeneous coords?
 */
template<class t_mat = ublas::matrix<double>>
bool is_translation_matrix(const t_mat& mat)
{
	using T = typename t_mat::value_type;

	const std::size_t iI = mat.size1();
	const std::size_t iJ = mat.size2();

	for(std::size_t i=0; i<iI-1; ++i)
	{
		if(!float_equal<T>(mat(i, iJ-1), T(0)))
			return true;
	}
	return false;
}


template<class t_mat = ublas::matrix<double>>
bool is_identity_matrix(const t_mat& mat)
{
	using T = typename t_mat::value_type;
	if(mat.size1() != mat.size2())
		return false;

	const std::size_t iN = mat.size1();

	for(std::size_t i=0; i<iN; ++i)
	{
		for(std::size_t j=0; j<iN; ++j)
		{
			if(i != j)	// off-diagonal elements
			{
				if(!float_equal<T>(mat(i, j), T(0)))
					return false;
			}
			else	// diagonal elements
			{
				if(!float_equal<T>(mat(i, j), T(1)))
					return false;
			}
		}
	}

	return true;
}


/**
 * do the absolute elements form an identity matrix
 * also return the indices of the negative values on the diagonal
 */
template<class t_mat = ublas::matrix<double>>
std::pair<bool, std::vector<std::size_t>> is_abs_identity_matrix(const t_mat& mat)
{
	std::vector<std::size_t> vecMinusses;

	using T = typename t_mat::value_type;
	if(mat.size1() != mat.size2())
		return std::make_pair(false, vecMinusses);

	const std::size_t iN = mat.size1();
	for(std::size_t i=0; i<iN; ++i)
	{
		for(std::size_t j=0; j<iN; ++j)
		{
			if(i != j)	// off-diagonal elements
			{
				if(!float_equal<T>(mat(i, j), T(0)))
					return std::make_pair(false, vecMinusses);
			}
			else	// diagonal elements
			{
				if(!float_equal<T>(std::abs(mat(i, j)), T(1)))
					return std::make_pair(false, vecMinusses);
				if(mat(i,j) < T(0))
					vecMinusses.push_back(i);
			}
		}
	}

	return std::make_pair(true, vecMinusses);
}


template<class t_mat = ublas::matrix<double>>
bool is_inverting_matrix(const t_mat& mat)
{ return is_identity_matrix(-mat); }


/**
 * does the homogeneous matrix mat have a translation component?
 */
template<class t_mat = ublas::matrix<double>>
bool has_translation_components(const t_mat& mat)
{
	using T = typename t_mat::value_type;
	const std::size_t iN = mat.size1();
	if(iN != mat.size2())
		return false;

	// translation?
	for(std::size_t i=0; i<iN-1; ++i)
	{
		if(!float_equal<T>(mat(i, iN-1), T(0)))
			return true;
	}

	return false;
}


/**
 * is mat a centering matrix in homogeneous coords?
 */
template<class t_mat = ublas::matrix<double>>
bool is_centering_matrix(const t_mat& mat)
{
	//if(is_identity_matrix(mat)) return 1;

	using T = typename t_mat::value_type;
	const std::size_t iN = mat.size1();
	if(iN != mat.size2())
		return false;

	// left-upper 3x3 unit matrix?
	if(!is_identity_matrix(submatrix(mat, iN-1, iN-1)))
		return false;

	// translation?
	if(has_translation_components<t_mat>(mat))
			return true;
	return false;
}


/**
 * Euler-Rodrigues formula
 * see e.g.: https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
 */
template<class mat_type = ublas::matrix<double>,
	class vec_type = ublas::vector<typename mat_type::value_type>,
	typename T = typename mat_type::value_type>
mat_type rotation_matrix(const vec_type& _vec, T angle)
{
	const vec_type vec = _vec/veclen(_vec);

	T s, c;
	if(angle == T(0))
	{
		s = T(0);
		c = T(1);
	}
	else
	{
		s = std::sin(angle);
		c = std::cos(angle);
	}

	return (T(1) - c) * outer(vec, vec) +
		c * unit_m(vec.size()) +
		s * skew(vec);
}


template<class matrix_type = ublas::matrix<double>>
typename matrix_type::value_type trace(const matrix_type& mat)
{
	typedef typename matrix_type::value_type T;

	if(mat.size1() != mat.size2())
		return T(0);

	T tr = T(0.);
	for(std::size_t i=0; i<mat.size1(); ++i)
		tr += mat(i,i);
	return tr;
}


/**
 * parallel or perspectivic projection matrix
 * see: https://www.opengl.org/sdk/docs/man2/xhtml/glOrtho.xml
 * see: https://www.opengl.org/sdk/docs/man2/xhtml/gluPerspective.xml
 */
template<class t_mat = ublas::matrix<double, ublas::row_major, ublas::bounded_array<double,4*4>>,
	class T = typename t_mat::value_type>
t_mat proj_matrix(T l, T r, T b, T t, T n, T f, bool bParallel)
{
	// scale to [0:2]
	t_mat matScale = scale_matrix<t_mat>({ T(2)/(r-l), T(2)/(t-b), T(1), T(1) });
	// translate to [-1:1]
	t_mat matTrans = translation_matrix<t_mat>({ -T(0.5)*(r+l), -T(0.5)*(t+b), T(0) });
	matScale = prod_mm(matScale, matTrans);

	// project
	t_mat matProj = unit_m<t_mat>(4);

	if(bParallel)	// parallel
	{
		matProj(2,2) = T(2)*f*n / (n-f);
		matProj(2,3) = (n+f) / (n-f);
	}
	else			// perspectivic
	{
		matProj(2,2) = (n+f) / (n-f);
		matProj(2,3) = T(2)*f*n / (n-f);
		matProj(3,2) = T(-1);
		matProj(3,3) = T(0);
	}

	return prod_mm(matScale, matProj);
}

/**
 * parallel projection matrix
 * see: https://www.opengl.org/sdk/docs/man2/xhtml/glOrtho.xml
 */
template<class t_mat = ublas::matrix<double, ublas::row_major, ublas::bounded_array<double,4*4>>,
	class T = typename t_mat::value_type>
t_mat ortho_matrix(T l, T r, T b, T t, T n, T f)
{
	return proj_matrix<t_mat, T>(l,r, b,t, n,f, 1);
}

/**
 * perspectivic projection matrix
 * see: https://www.opengl.org/sdk/docs/man2/xhtml/gluPerspective.xml
 * also see: similar gnomonic projection of spherical coordinates onto a plane
 */
template<class t_mat = ublas::matrix<double, ublas::row_major, ublas::bounded_array<double,4*4>>,
	class T = typename t_mat::value_type>
t_mat perspective_matrix(T yfov, T asp, T n, T f)
{
	const T y = std::tan(T(0.5)*yfov);
	const T x = y*asp;

	return proj_matrix<t_mat, T>(-x,x, -y,y, n,f, 0);
}


// -----------------------------------------------------------------------------
template<typename T, class FKT, const int iDim=get_type_dim<T>::value>
struct is_nan_or_inf_impl {};

template<typename real_type, class FKT>
struct is_nan_or_inf_impl<real_type, FKT, 0>	// scalar impl.
{
	const FKT& m_fkt;
	is_nan_or_inf_impl(const FKT& fkt) : m_fkt(fkt) {}
	bool operator()(real_type d) const { return m_fkt(d); }
};

template<typename vec_type, class FKT>
struct is_nan_or_inf_impl<vec_type, FKT, 1>		// vector impl.
{
	const FKT& m_fkt;
	is_nan_or_inf_impl(const FKT& fkt) : m_fkt(fkt) {}

	bool operator()(const vec_type& vec) const
	{
		for(std::size_t i=0; i<vec.size(); ++i)
			if(m_fkt(vec[i]))
				return true;
		return false;
	}
};

template<typename mat_type, class FKT>
struct is_nan_or_inf_impl<mat_type, FKT, 2>		// matrix impl.
{
	const FKT& m_fkt;
	is_nan_or_inf_impl(const FKT& fkt) : m_fkt(fkt) {}

	bool operator()(const mat_type& mat) const
	{
		for(std::size_t i=0; i<mat.size1(); ++i)
			for(std::size_t j=0; j<mat.size2(); ++j)
				if(m_fkt(mat(i,j)))
					return true;
		return false;
	}
};

template<class T = ublas::matrix<double>>
bool isnan(const T& mat)
{
	typedef underlying_value_type_t<T> real_type;

	using fkt = std::function<bool(real_type)>;
	fkt stdisnan = (bool(*)(real_type))std::isnan;
	is_nan_or_inf_impl<T, fkt> _isnan(stdisnan);
	return _isnan(mat);
}

template<class T = ublas::matrix<double>>
bool isinf(const T& mat)
{
	typedef underlying_value_type_t<T> real_type;

	using fkt = std::function<bool(real_type)>;
	fkt stdisinf = (bool(*)(real_type))std::isinf;
	is_nan_or_inf_impl<T, fkt> _isinf(stdisinf);
	return _isinf(mat);
}

template<class T = ublas::matrix<double>>
bool is_nan_or_inf(const T& mat)
{
	typedef underlying_value_type_t<T> real_type;

	using fkt = std::function<bool(real_type)>;
	fkt stdisnaninf = [](real_type d)->bool { return std::isnan(d) || std::isinf(d); };
	is_nan_or_inf_impl<T, fkt> _isnaninf(stdisnaninf);
	return _isnaninf(mat);
}
// -----------------------------------------------------------------------------


/**
 * calculates the matrix inverse
 *
 * @desc code for inverse based on boost/libs/numeric/ublas/test/test_lu.cpp
 * @desc Boost's test_lu.cpp is (c) 2008 by G. Winkler
 */
template<class mat_type = ublas::matrix<double>>
bool inverse(const mat_type& mat, mat_type& inv)
{
	using T = typename mat_type::value_type;
	const typename mat_type::size_type N = mat.size1();
	if(N != mat.size2())
		return false;
	//if(isnan(mat) || isinf(mat))
	//	return false;

	try
	{
		mat_type lu = mat;
		ublas::permutation_matrix<typename mat_type::size_type> perm(N);

		if(ublas::lu_factorize(lu, perm) != 0)
			return false;

		inv = unit_m<mat_type>(N);
		ublas::lu_substitute(lu, perm, inv);
	}
	catch(const std::exception& ex)
	{
		log_err("Matrix inversion failed with exception: ", ex.what(), ".", "\n",
			"Matrix to be inverted was: ", mat, ".");
		//log_backtrace();
		return false;
	}
	return true;
}


/**
 * R = T^(-1) M T
 * bCongr==1: do a congruence trafo
 * bCongr==0: do a similarity trafo
 */
template<class mat_type = ublas::matrix<double>>
mat_type transform(const mat_type& mat, const mat_type& matTrafo, bool bCongr=0)
{
	mat_type matTrafoInv;
	if(bCongr)
		matTrafoInv = transpose(matTrafo);
	else
		inverse(matTrafo, matTrafoInv);

	mat_type MT = prod_mm(mat, matTrafo);
	mat_type TinvMT = prod_mm(matTrafoInv, MT);

	return TinvMT;
}

/**
 * R = T M T^(-1)
 * bCongr==1: do a congruence trafo
 * bCongr==0: do a similarity trafo
 */
template<class mat_type = ublas::matrix<double>>
mat_type transform_inv(const mat_type& mat, const mat_type& matTrafo, bool bCongr=0)
{
	mat_type matTrafoInv;
	if(bCongr)
		matTrafoInv = transpose(matTrafo);
	else
		inverse(matTrafo, matTrafoInv);

	mat_type MT = prod_mm(mat, matTrafoInv);
	mat_type TinvMT = prod_mm(matTrafo, MT);

	return TinvMT;
}






template<class matrix_type = ublas::matrix<double>>
bool is_diag_matrix(const matrix_type& mat)
{
	typedef typename matrix_type::value_type T;

	for(std::size_t i=0; i<mat.size1(); ++i)
		for(std::size_t j=0; j<mat.size2(); ++j)
		{
			if(i==j) continue;

			if(!float_equal(mat(i,j), T(0.)))
				return false;
		}

	return true;
}

// ----------------------------------------------------------------------------

template<class matrix_type = ublas::matrix<double>,
	class vec_type = ublas::vector<typename matrix_type::value_type>,
	class container_type = std::initializer_list<vec_type>, const bool bRowMat>
inline matrix_type row_col_matrix(const container_type& vecs)
{
	if(vecs.size() == 0)
		return matrix_type(0,0);

	const std::size_t N = vecs.size();
	const std::size_t M = vecs.begin()->size();

	matrix_type mat(bRowMat?N:M, bRowMat?M:N);
	std::size_t j=0;
	for(typename container_type::const_iterator iter=vecs.begin(); iter!=vecs.end(); ++iter)
	{
		const vec_type& vec = *iter;

		for(std::size_t i=0; i<vec.size(); ++i)
		{
			if(bRowMat)
				mat(j,i) = vec[i];
			else
				mat(i,j) = vec[i];
		}

		++j;
	}

	return mat;
}

/**
 * vectors form rows of matrix
 */
template<class matrix_type = ublas::matrix<double>,
	class vec_type = ublas::vector<typename matrix_type::value_type>,
	class container_type = std::initializer_list<vec_type>>
matrix_type row_matrix(const container_type& vecs)
{
	return row_col_matrix<matrix_type, vec_type, container_type, true>(vecs);
}

/**
 * vectors form columns of matrix
 */
template<class matrix_type = ublas::matrix<double>,
	class vec_type = ublas::vector<typename matrix_type::value_type>,
	class container_type = std::initializer_list<vec_type>>
matrix_type column_matrix(const container_type& vecs)
{
	return row_col_matrix<matrix_type, vec_type, container_type, false>(vecs);
}


// ----------------------------------------------------------------------------


template<class t_mat/*=ublas::matrix<double>*/>
typename t_mat::value_type determinant(const t_mat& mat)
{
	using T = typename t_mat::value_type;

	if(mat.size1() != mat.size2())
		return T(0);

	if(mat.size1()==0)
		return T(0);
	else if(mat.size1()==1)
		return mat(0,0);
	else if(mat.size1()==2)
		return mat(0,0)*mat(1,1) - mat(1,0)*mat(0,1);
	else if(mat.size1()==3)
	{
		ublas::vector<T> vec0 = get_column(mat, 0);
		ublas::vector<T> vec1 = get_column(mat, 1);
		ublas::vector<T> vec2 = get_column(mat, 2);

		ublas::vector<T> vecCross = cross_3<ublas::vector<T> >(vec1, vec2);
		return inner(vec0, vecCross);
	}
	else if(mat.size1()>3 && mat.size1()<6)		// recursive expansion, complexity: O(n!)
	{
		size_t i = 0;
		size_t iZeros = 0;

		// count zeros
		for(size_t _i=0; _i<mat.size1(); ++_i)
		{
			ublas::vector<T> vecRow = get_row<ublas::vector<T>, t_mat>(mat, _i);
			size_t iNewZeros = std::count_if(vecRow.begin(), vecRow.end(),
				[](T d) -> bool { return float_equal<T>(d, 0.); });
			if(iNewZeros > iZeros)
			{
				i = _i;
				iZeros = iNewZeros;
			}
		}


		T val = T(0);

		for(size_t j=0; j<mat.size2(); ++j)
		{
			if(float_equal<T>(mat(i,j), 0.))
				continue;

			T dSign = 1.;
			if(is_odd<size_t>(i+j))
				dSign = -1.;

			t_mat matSub = submatrix(mat, i, j);
			val += dSign * mat(i,j) * determinant<t_mat>(matSub);
		}

		return val;
	}
	else if(mat.size1()>=6)				// LU decomposition, complexity: O(n^3)
	{
		t_mat lu = mat;
		size_t N = mat.size1();
		ublas::permutation_matrix<typename t_mat::size_type> perm(N);

		ublas::lu_factorize(lu, perm);

		t_mat L = ublas::triangular_adaptor<t_mat, ublas::unit_lower>(lu);
		t_mat U = ublas::triangular_adaptor<t_mat, ublas::upper>(lu);

		T dDet = T(1.);
		for(size_t i=0; i<mat.size1(); ++i)
			dDet *= L(i,i)*U(i,i);

		size_t iNumSwaps=0;
		for(size_t iSwap=0; iSwap<perm.size(); ++iSwap)
			if(iSwap != perm(iSwap))
				++iNumSwaps;

		if(is_odd<size_t>(iNumSwaps))
			dDet *= T(-1.);

		return dDet;
	}

	return T(0);
}


/**
 * minor determinant
 * see e.g.: https://en.wikipedia.org/wiki/Minor_(linear_algebra)
 */
template<class t_mat = ublas::matrix<double>>
typename t_mat::value_type minor_det(const t_mat& mat, std::size_t iRow, std::size_t iCol)
{
	using T = typename t_mat::value_type;

	t_mat M = submatrix(mat, iRow, iCol);
	return determinant<t_mat>(M);
}

template<class t_mat = ublas::matrix<double>>
typename t_mat::value_type cofactor(const t_mat& mat, std::size_t iRow, std::size_t iCol)
{
	using T = typename t_mat::value_type;

	T m = minor_det(mat, iRow, iCol);
	T s = std::pow(T(-1), T(iRow+1 + iCol+1));

	return m*s;
}

/**
 * adjugate matrix
 * see e.g.: https://en.wikipedia.org/wiki/Adjugate_matrix
 */
template<class t_mat = ublas::matrix<double>>
t_mat adjugate(const t_mat& mat, bool bTranspose=1)
{
	using T = typename t_mat::value_type;

	t_mat matRet(mat.size1(), mat.size2());

	for(std::size_t i=0; i<mat.size1(); ++i)
		for(std::size_t j=0; j<mat.size2(); ++j)
		{
			T c = cofactor<t_mat>(mat, i, j);
			matRet(i,j) = c;
		}

	if(bTranspose)
		matRet = transpose(matRet);
	return matRet;
}


template<class matrix_type = ublas::matrix<double>>
typename matrix_type::value_type get_volume(const matrix_type& mat)
{
	//typedef typename matrix_type::value_type T;
	return std::abs(determinant<matrix_type>(mat));
}


template<class matrix_type = ublas::matrix<double>>
typename matrix_type::value_type get_ellipsoid_volume(const matrix_type& mat)
{
	typedef typename matrix_type::value_type T;
	T tDet = std::abs(determinant<matrix_type>(mat));

	return T(4./3.) * pi<T> * std::sqrt(T(1)/tDet);
}

// ----------------------------------------------------------------------------


/**
 * calculate fractional coordinate basis vectors from angles
 * see: http://www.bmsc.washington.edu/CrystaLinks/man/pdb/part_75.html
 * or: https://en.wikipedia.org/wiki/Fractional_coordinates
 * for the reciprocal lattice this is equal to the B matrix from Acta Cryst. (1967), 22, 457
 */
template<class t_vec>
bool fractional_basis_from_angles(typename t_vec::value_type a,
	typename t_vec::value_type b,
	typename t_vec::value_type c,
	typename t_vec::value_type alpha,
	typename t_vec::value_type beta,
	typename t_vec::value_type gamma,
	t_vec& veca, t_vec& vecb, t_vec& vecc)
{
	typedef typename t_vec::value_type T;

	const T dSG = std::sin(gamma), dCG = std::cos(gamma);
	const T dCA = std::cos(alpha), dCB = std::cos(beta);
	const T dCA2 = dCA*dCA, dCB2 = dCB*dCB, dCG2 = dCG*dCG;

	const T dVol = a*b*c *
		std::sqrt(T(1) - dCA2 - dCB2 - dCG2 + T(2)*dCA*dCB*dCG);
	if(std::isinf(dVol) || std::isnan(dVol))
		return false;

	if(veca.size() != 3) veca.resize(3);
	if(vecb.size() != 3) vecb.resize(3);
	if(vecc.size() != 3) vecc.resize(3);

	veca[0] = a;
	veca[1] = T(0);
	veca[2] = T(0);

	vecb[0] = b*dCG;
	vecb[1] = b*dSG;
	vecb[2] = T(0);

	vecc[0] = c*dCB;
	vecc[1] = c*(dCA - dCB*dCG) / dSG;
	vecc[2] = dVol / (a*b*dSG);

	return true;
}

// ----------------------------------------------------------------------------


/**
 * signed angle wrt basis
 */
template<typename vec_type>
typename vec_type::value_type vec_angle(const vec_type& vec)
{
	if(vec.size() == 2)
		return std::atan2(vec[1], vec[0]);

	throw Err("vec_angle not yet implemented for size != 2.");
}


// -----------------------------------------------------------------------------
/**
 * set values lower than epsilon to zero
 */
template<typename T> void set_eps_0(T& d, underlying_value_type_t<T> eps=-1.);
template<typename T, LinalgType ty=get_linalg_type<T>::value> struct set_eps_0_impl {};

/**
 * set values lower than epsilon to zero
 * scalar version
 */
template<typename real_type>
struct set_eps_0_impl<real_type, LinalgType::REAL>
{
	real_type eps = get_epsilon<real_type>();

	void operator()(real_type& d) const
	{
		if(std::abs(d) < eps)
			d = real_type(0);
	}
};

/**
 * set values lower than epsilon to zero
 * vector version
 */
template<typename vec_type>
struct set_eps_0_impl<vec_type, LinalgType::VECTOR>
{
	using real_type = typename vec_type::value_type;
	real_type eps = get_epsilon<real_type>();

	void operator()(vec_type& vec) const
	{
		for(real_type& d : vec)
			set_eps_0<real_type>(d, eps);
	}
};

/**
 * set values lower than epsilon to zero
 * matrix version
 */
template<typename mat_type>
struct set_eps_0_impl<mat_type, LinalgType::MATRIX>
{
	using real_type = typename mat_type::value_type;
	real_type eps = get_epsilon<real_type>();

	void operator()(mat_type& mat) const
	{
		for(std::size_t i=0; i<mat.size1(); ++i)
			for(std::size_t j=0; j<mat.size2(); ++j)
				set_eps_0<real_type>(mat(i,j), eps);
	}
};

template<typename T>
void set_eps_0(T& d, underlying_value_type_t<T> eps)
{
	set_eps_0_impl<T, get_linalg_type<T>::value> op;
	if(eps >= underlying_value_type_t<T>(0))
		op.eps = eps;
	op(d);
}
// -----------------------------------------------------------------------------


template<typename t_vec, typename T = typename t_vec::value_type>
bool vec_is_collinear(const t_vec& _vec1, const t_vec& _vec2, T eps = get_epsilon<T>())
{
	const t_vec vec1 = _vec1 / veclen(_vec1);
	const t_vec vec2 = _vec2 / veclen(_vec2);

	T tdot = std::abs(inner(vec1, vec2));
	return float_equal<T>(tdot, 1, eps);
}

/**
 * signed angle between two vectors
 */
template<typename vec_type>
typename vec_type::value_type vec_angle(const vec_type& vec0,
	const vec_type& vec1, const vec_type* pvec_norm=nullptr)
{
	typedef typename vec_type::value_type real_type;

	if(vec0.size() != vec1.size())
		throw Err("In vec_angle: Vector sizes do not match.");

	if(vec0.size() == 2)
	{
		return vec_angle<vec_type>(vec0) - vec_angle<vec_type>(vec1);
	}
	if(vec0.size() == 3)
	{
		real_type dC = inner(vec0, vec1);
		vec_type veccross = cross_3<vec_type>(vec0, vec1);
		real_type dS = veclen(veccross);

		real_type dAngle = std::atan2(dS, dC);

		// get signed angle
		if(pvec_norm)
		{
			if(inner(veccross, *pvec_norm) < real_type(0))
				dAngle = -dAngle;
		}

		return dAngle;
	}

	throw Err("vec_angle only implemented for size == 2 and size == 3.");
}


template<class T, LinalgType ty=get_linalg_type<T>::value>
struct vec_angle_unsigned_impl {};

/**
 * unsigned angle between two vectors
 */
template<class T>
struct vec_angle_unsigned_impl<T, LinalgType::VECTOR>
{
	typename T::value_type operator()(const T& q1, const T& q2) const
	{
		typedef typename T::value_type REAL;

		if(q1.size() != q2.size())
			return REAL();

		REAL dot = REAL();
		REAL len1 = REAL();
		REAL len2 = REAL();
		for(std::size_t i=0; i<q1.size(); ++i)
		{
			dot += q1[i]*q2[i];

			len1 += q1[i]*q1[i];
			len2 += q2[i]*q2[i];
		}

		len1 = std::sqrt(len1);
		len2 = std::sqrt(len2);

		dot /= len1;
		dot /= len2;

		return std::acos(dot);
	}
};

template<class T>
typename T::value_type vec_angle_unsigned(const T& q1, const T& q2)
{
	return vec_angle_unsigned_impl<T>()(q1, q2);
}

// -----------------------------------------------------------------------------


/**
 * - see: K. Shoemake, "Animating rotation with quaternion curves":
 *        http://dx.doi.org/10.1145/325334.325242
 * - see: (Bronstein 2008), formula 4.207
 */
template<class T>
T slerp(const T& q1, const T& q2, typename T::value_type t)
{
	typedef typename T::value_type REAL;

	REAL angle = vec_angle_unsigned<T>(q1, q2);

	T q = std::sin((REAL(1)-t)*angle)/std::sin(angle) * q1 +
		std::sin(t*angle)/std::sin(angle) * q2;

	return q;
}



// --------------------------------------------------------------------------------


template<class t_vec=ublas::vector<int>>
t_vec get_gcd_vec(const t_vec& vec)
{
	if(vec.size() <= 1)
		return vec;

	typedef typename t_vec::value_type t_int;

	t_int igcd_total = 1;
	for(std::size_t i=0; i<vec.size()-1; ++i)
	{
		t_int i0 = vec[i];
		t_int i1 = vec[i+1];

		t_int igcd = integer::gcd<t_int>(i0, i1);

		if(i==0)
			igcd_total = igcd;
		else
			igcd_total = integer::gcd<t_int>(igcd, igcd_total);
	}

	if(igcd_total == 0)
		return vec;

	return vec/igcd_total;
}


// --------------------------------------------------------------------------------

/**
 * Householder reflection matrix
 * @desc for the algo, see (Scarpino 2011), p. 268
 */
template<class t_mat = ublas::matrix<double>,
	class t_vec = ublas::vector<typename t_mat::value_type>,
	typename T = typename t_mat::value_type>
	t_mat reflection_matrix(const t_vec& vecNorm)
{
	// projection of "pt" onto (normalised) vector "norm":
	// proj = (norm^t * pt) * norm
	// proj = (norm * norm^t) * pt
	// "Lotfusspunkt" = pt - proj
	// mirror_point = pt - 2*proj
	t_mat mat = -T(2) * outer(vecNorm, vecNorm);
	mat /= inner(vecNorm, vecNorm);

	for(std::size_t i=0; i<vecNorm.size(); ++i)
		mat(i,i) += T(1);

	return mat;
}

/**
 * Householder reflection
 */
template<class t_vec = ublas::vector<double>,
	class t_mat = ublas::matrix<typename t_vec::value_type>,
	typename T = typename t_mat::value_type>
	t_vec reflection(const t_vec& vec, const t_vec& vecNorm)
{
	t_mat mat = reflection_matrix<t_mat, t_vec, T>(vecNorm);
	return prod_mv(mat, vec);
}

/**
 * add a nxn unit matrix to the upper left of a matrix
 */
template<class t_mat = ublas::matrix<double>,
	typename T = typename t_mat::value_type>
	t_mat insert_unity(const t_mat& M, std::size_t n)
{
	if(M.size1()!=M.size2())
		throw Err("Non-square matrix not yet supported.");

	std::size_t m = M.size1();
	t_mat M2 = t_mat(m+n, m+n);

	for(std::size_t iR=0; iR<m+n; ++iR)
	{
		for(std::size_t jR=0; jR<m+n; ++jR)
		{
			if(iR<n || jR<n)
				M2(iR, jR) = (iR==jR ? 1 : 0);
			else
				M2(iR, jR) = M(iR-n, jR-n);
		}
	}

	return M2;
}

/**
 * QR decomposition via householder reflections
 * @desc for the algo, see (Scarpino 2011), pp. 269--272
 */
template<class t_mat = ublas::matrix<double>,
	class t_vec = ublas::vector<typename t_mat::value_type>,
	typename T = typename t_mat::value_type>
bool qr_decomp(const t_mat& M, t_mat& Q, t_mat& R)
{
	std::size_t m = M.size1();
	std::size_t n = M.size2();

	t_mat A = M;
	std::vector<t_mat> vecRefls;

	for(std::size_t i=0; i<std::min(m-1,n); ++i)
	{
		t_vec vec0 = get_column(A, 0);

		// vector of form [123.4 0 0 0] ?
		t_vec vec0_rest = ublas::subrange(vec0, 1, vec0.size());
		if(vec_equal<t_vec>(vec0_rest, zero_v<t_vec>(vec0_rest.size())))
		{
			t_mat matReflM = unit_m(m);
			vecRefls.push_back(matReflM);
			continue;
		}

		t_vec vecE0 = zero_v<t_vec>(vec0.size());
		vecE0[0] = veclen(vec0);

		t_vec vecReflNorm = vec0-vecE0;
		//std::cout << "refl norm: " << vecReflNorm << std::endl;
		t_mat matRefl = reflection_matrix(vecReflNorm);

		A = prod_mm(matRefl, A);
		A = submatrix(A,0,0);

		t_mat matReflM = insert_unity(matRefl, m-matRefl.size1());
		//std::cout << "refl: " << matReflM << std::endl;
		vecRefls.push_back(matReflM);
	}

	if(vecRefls.size() == 0)
		return false;

	Q = unit_m(m);
	for(const t_mat& matRefl : vecRefls)
	{
		t_mat matReflT = transpose(matRefl);
		Q = prod_mm(Q, matReflT);
	}

	/*R = vecRefls[vecRefls.size()-1];
	for(int i=vecRefls.size()-2; i>=0; --i)
		R = prod_mv(R, vecRefls[i]);
	R = prod_mm(R, M);*/

	t_mat QT = transpose(Q);
	R = prod_mm(QT, M);

	return true;
}


template<typename t_vec = ublas::vector<double>,
	typename T = typename t_vec::value_type>
std::vector<t_vec> gram_schmidt(const std::vector<t_vec>& vecs, bool bNorm=true);

/**
 * QR decomposition via gram-schmidt orthogonalisation
 */
template<class t_mat = ublas::matrix<double>,
	class t_vec = ublas::vector<typename t_mat::value_type>,
	typename T = typename t_mat::value_type>
bool qr_decomp_gs(const t_mat& M, t_mat& Q, t_mat& R)
{
	Q = column_matrix(gram_schmidt(get_columns(M), 1));

	// M = QR  =>  Q^T M = R
	R = prod_mm(transpose(Q), M);
	return 1;
}


template<class t_mat = ublas::matrix<double>,
	class t_vec = ublas::vector<typename t_mat::value_type>,
	typename T = typename t_mat::value_type>
t_mat norm_col_vecs(const t_mat& M)
{
	t_mat N(M.size1(), M.size2());

	for(std::size_t i=0; i<M.size2(); ++i)
	{
		t_vec vec0 = get_column(M, i);
		vec0 /= veclen(vec0);

		set_column(N, i, vec0);
	}

	return N;
}

template<class t_mat=ublas::matrix<double>, class t_real=underlying_value_type_t<t_mat>>
bool is_symmetric(const t_mat& mat, t_real eps = get_epsilon<t_real>())
{
	if(mat.size1() != mat.size2())
		return false;

	for(std::size_t i=0; i<mat.size1(); ++i)
		for(std::size_t j=i+1; j<mat.size2(); ++j)
			if(!float_equal(mat(i,j), mat(j,i), eps))
				return false;

	return true;
}


// -----------------------------------------------------------------------------
template<class T, LinalgType ty=get_linalg_type<T>::value> struct apply_fkt_impl {};

template<class T>
struct apply_fkt_impl<T, LinalgType::REAL>
{
	T operator()(T t, const std::function<T(T)>& fkt) const
	{
		return fkt(t);
	}
};

template<class t_vec>
struct apply_fkt_impl<t_vec, LinalgType::VECTOR>
{
	using value_type = underlying_value_type_t<t_vec>;

	t_vec operator()(const t_vec& vec, const std::function<value_type(value_type)>& fkt) const
	{
		t_vec v;
		v.resize(vec.size());

		for(std::size_t i=0; i<vec.size(); ++i)
			v[i] = fkt(vec[i]);

		return v;
	}
};

template<class t_mat>
struct apply_fkt_impl<t_mat, LinalgType::MATRIX>
{
	using value_type = underlying_value_type_t<t_mat>;

	t_mat operator()(const t_mat& mat, const std::function<value_type(value_type)>& fkt) const
	{
		t_mat m;
		m.resize(mat.size1(), mat.size2());

		for(std::size_t i=0; i<mat.size1(); ++i)
			for(std::size_t j=0; j<mat.size2(); ++j)
				m(i,j) = fkt(mat(i,j));

		return m;
	}
};

template<class T, class t_val=underlying_value_type_t<T>>
T apply_fkt(const T& t, const std::function<t_val(t_val)>& fkt)
{
	apply_fkt_impl<T> impl;
	return impl(t, fkt);
}

template<class T, class t_val=underlying_value_type_t<T>>
inline T apply_fkt(const T& t, t_val(*pfkt)(t_val))
{
	std::function<t_val(t_val)> fkt(pfkt);
	return apply_fkt<T, t_val>(t, fkt);
}
// -----------------------------------------------------------------------------


template<class T, LinalgType ty=get_linalg_type<T>::value>
struct get_minmax_impl {};

template<class T>
struct get_minmax_impl<T, LinalgType::REAL>
{
	std::pair<T, T>
	operator()(T t) const
	{
		return std::pair<T,T>(t,t);
	}
};

template<class t_vec>
struct get_minmax_impl<t_vec, LinalgType::VECTOR>
{
	using t_val = underlying_value_type_t<t_vec>;

	std::pair<t_val, t_val>
	operator()(const t_vec& vec) const
	{
		t_val tmin = std::numeric_limits<t_val>::max();
		t_val tmax = -tmin;

		for(std::size_t i=0; i<vec.size(); ++i)
		{
			if(vec[i] < tmin) tmin = vec[i];
			if(vec[i] > tmax) tmax = vec[i];
		}

		return std::pair<t_val, t_val>(tmin, tmax);
	}
};

template<class t_mat>
struct get_minmax_impl<t_mat, LinalgType::MATRIX>
{
	using t_val = underlying_value_type_t<t_mat>;

	std::pair<t_val, t_val>
	operator()(const t_mat& mat) const
	{
		t_val tmin = std::numeric_limits<t_val>::max();
		t_val tmax = -tmin;

		for(std::size_t i=0; i<mat.size1(); ++i)
			for(std::size_t j=0; j<mat.size2(); ++j)
			{
				if(mat(i,j) < tmin) tmin = mat(i,j);
				if(mat(i,j) > tmax) tmax = mat(i,j);
			}

		return std::pair<t_val, t_val>(tmin, tmax);
	}
};

template<class T>
std::pair<underlying_value_type_t<T>, underlying_value_type_t<T>>
get_minmax(const T& t)
{
	get_minmax_impl<T> impl;
	return impl(t);
}

// -----------------------------------------------------------------------------


/**
 * Calculates the dominant eigenvector/eigenvalue for symmetric matrices
 * see: (Bronstein 2008), equs. (4.148)-(4.151)
 */
template<class t_mat = ublas::matrix<double>,
	class t_vec = ublas::vector<typename t_mat::value_type>,
	typename T = typename t_mat::value_type>
bool eigenvec_dominant_sym(const t_mat& mat, t_vec& evec, T& eval,
	t_vec vecInit = make_vec<t_vec>({1,0,0}),
	std::size_t iMaxIter = 50)
{
	if(mat.size1() != mat.size2())
	{
		log_err("Matrix ", mat, " is not square.");
		return false;
	}

#ifndef NDEBUG
	t_mat matAbs = apply_fkt(mat, std::function<T(T)>((T(*)(T))std::abs));
	T _dEps = get_minmax(matAbs).second / 100.;	// 1% accuracy
	if(!is_symmetric(mat, _dEps)) log_warn("Matrix ", mat, " is not symmetric.");
#endif

	t_vec vecPrev;
	for(std::size_t iIter=0; iIter<iMaxIter; ++iIter)
	{
		if(iIter == iMaxIter-1)
			vecPrev = vecInit;
		vecInit = prod_mv(mat, vecInit);
	}

	const T normInit = veclen(vecInit);
	const T normPrev = veclen(vecPrev);

	eval = normInit / normPrev;
	evec = vecInit / normInit;
	return true;
}


/**
 * Calculates the least dominant eigenvector/eigenvalue for symmetric matrices
 * see: (Bronstein 2008), equs. (4.148)-(4.151)
 */
template<class t_mat = ublas::matrix<double>,
	class t_vec = ublas::vector<typename t_mat::value_type>,
	typename T = typename t_mat::value_type>
bool eigenvec_least_dominant_sym(const t_mat& mat, t_vec& evec, T& eval,
	t_vec vecInit = make_vec<t_vec>({1,0,0}),
	std::size_t iMaxIter = 50)
{
	t_mat M;
	if(!inverse(mat, M))
		return false;

	if(!eigenvec_dominant_sym(M, evec, eval, vecInit, iMaxIter))
		return false;

	eval = T(1)/eval;
	return true;
}


/**
 * Calculates the eigenvectors/eigenvalues for symmetric matrices
 * ! for large matrices use eigenvec_sym  !
 */
template<class t_mat = ublas::matrix<double>,
	class t_vec = ublas::vector<typename t_mat::value_type>,
	typename T = typename t_mat::value_type>
bool eigenvec_sym_simple(const t_mat& mat, std::vector<t_vec>& evecs, std::vector<T>& evals,
	std::size_t MAX_ITER=512, T tEps = std::cbrt(get_epsilon<T>()))
{
	if(mat.size1() != mat.size2())
	{
		log_err("Matrix ", mat, " is not square.");
		return false;
	}

#ifndef NDEBUG
	t_mat matAbs = apply_fkt(mat, std::function<T(T)>((T(*)(T))std::abs));
	T _dEps = get_minmax(matAbs).second / 100.;	// 1% accuracy
	if(!is_symmetric(mat, _dEps)) log_warn("Matrix ", mat, " is not symmetric.");
#endif

	const std::size_t n = mat.size1();
	t_mat I = unit_m<t_mat>(n);
	t_mat M = mat;

	std::size_t iIter = 0;
	for(iIter=0; iIter<MAX_ITER; ++iIter)
	{
		t_mat Q, R;
		if(!qr_decomp(M, Q, R))
		{
			log_err("QR decomposition failed for matrix ", M);
			return false;
		}
		//std::cout << "Q=" << Q << ", R=" << R << std::endl;
		//Q = norm_col_vecs(Q);

		t_mat Mlast = M;
		M = prod_mm(R, Q);
		I = prod_mm(I, Q);


		bool bConverged = 1;
		for(std::size_t iVal=0; iVal<n; ++iVal)
		{
			if(std::abs(M(iVal,iVal)-Mlast(iVal,iVal)) > tEps)
			{
				bConverged = 0;
				break;
			}
		}

		if(bConverged)
			break;
	}

	/*bool bFlipVec = 0;
	if(determinant<t_mat>(I) < T(0))
		bFlipVec = 1;*/

	evals.resize(n);
	evecs.resize(n);

	for(std::size_t iVal=0; iVal<n; ++iVal)
	{
		evals[iVal] = M(iVal, iVal);
		evecs[iVal] = get_column(I, iVal);
	}

	//if(bFlipVec) evecs[0] = -evecs[0];
	return true;
}


template<class t_mat = ublas::matrix<double>,
	class t_vec = ublas::vector<typename t_mat::value_type>,
	typename T = typename t_mat::value_type>
bool eigenvec_approxsym_simple(const t_mat& mat, std::vector<t_vec>& evecs, std::vector<T>& evals,
	std::size_t MAX_ITER=512, T tEps = std::cbrt(get_epsilon<T>()))
{
	t_mat MtM = prod_mm(transpose(mat), mat);
	bool bOk = eigenvec_sym_simple(MtM, evecs, evals, MAX_ITER, tEps);

	for(T& eval : evals)
		eval = std::sqrt(std::abs(eval));
	return bOk;
}


template<typename T=double>
void sort_eigenvecs(std::vector<ublas::vector<T>>& evecs,
	std::vector<T>& evals, bool bOrder=0, T (*pEvalFkt)(T)=0,
	ublas::vector<T>* pUserVec = nullptr)
{
	if(evecs.size() != evals.size())
		return;

	struct Evec
	{
		ublas::vector<T> vec;
		T val;

		T userval = 0;
	};

	std::vector<Evec> myevecs;
	myevecs.reserve(evecs.size());

	for(std::size_t i=0; i<evecs.size(); ++i)
	{
		Evec ev;
		ev.vec = evecs[i];
		ev.val = evals[i];
		if(pUserVec) ev.userval = (*pUserVec)[i];

		myevecs.push_back(ev);
	}


	std::sort(myevecs.begin(), myevecs.end(),
		[&](const Evec& evec1, const Evec& evec2) -> bool
		{
			bool b;
			if(pEvalFkt)
				b = pEvalFkt(evec1.val) < pEvalFkt(evec2.val);
			else
				b = evec1.val < evec2.val;

			if(bOrder) b = !b;
			return b;
		});


	for(std::size_t i=0; i<evecs.size(); ++i)
	{
		evecs[i] = myevecs[i].vec;
		evals[i] = myevecs[i].val;
		if(pUserVec) (*pUserVec)[i] = myevecs[i].userval;
	}
}


// --------------------------------------------------------------------------------

/**
 * project vec1 onto vec2
 * proj_op = |vec2><vec2¦/ len(vec2)^2,  len(vec2) = sqrt(<vec2|vec2>)
 * proj = proj_op * vec1 = |vec2> * <vec2|vec1> / <vec2|vec2>
 */
template<typename t_vec = ublas::vector<double>>
t_vec proj_vec(t_vec vec1, t_vec vec2)
{
	using T = typename t_vec::value_type;

	T tnum = inner(vec1, vec2);
	T tden = inner(vec2, vec2);

	t_vec vecProj = tnum/tden * vec2;
	return vecProj;
}


/**
 * Gram-Schmidt orthogonalisation of basis vectors
 * see e.g.: https://en.wikipedia.org/wiki/Gram%E2%80%93Schmidt_process
 */
template<typename t_vec /*= ublas::vector<double>*/,
	typename T /*= typename t_vec::value_type*/ >
std::vector<t_vec> gram_schmidt(const std::vector<t_vec>& vecs, bool bNorm/*=1*/)
{
	std::vector<t_vec> vecsOut;
	if(vecs.size() == 0)
		return vecsOut;

	vecsOut.resize(vecs.size());

	// iterate through all basis vectors i
	for(std::size_t i=0; i<vecs.size(); ++i)
	{
		vecsOut[i] = vecs[i];

		// iterate through all previous basis vectors j<i
		// and remove projected contributions from i vectors
		for(std::size_t j=0; j<i; ++j)
			vecsOut[i] -= proj_vec<t_vec>(vecs[i], vecsOut[j]);
	}

	// normalise basis?
	if(bNorm)
		for(t_vec& vec : vecsOut)
			vec /= veclen(vec);

	return vecsOut;
}


template<typename t_vec = ublas::vector<double>,
	typename T = typename t_vec::value_type>
std::vector<t_vec> get_ortho_rhs(const std::vector<t_vec>& vecs)
{
	assert(vecs.size() == 2);

	std::vector<t_vec> vecOrtho = gram_schmidt(vecs, true);
	t_vec vecUp = cross_3(vecOrtho[0], vecOrtho[1]);
	vecOrtho.push_back(vecUp);

	return vecOrtho;
}



// ------------------------------------------------------------------------------------------------



/**
 * spin matrices
 * see e.g. (Arfken 2013), p. 110
 */
template<template<class...> class t_mat=ublas::matrix,
	template<class...> class t_vec=ublas::vector,
	class t_real = double>
t_vec<t_mat<std::complex<t_real>>> get_spin_matrices()
{
	t_vec<t_mat<std::complex<t_real>>> vec(3);
	const std::complex<t_real> i(0,1);

	vec[0] = make_mat<t_mat<std::complex<t_real>>>({{0,1}, {1,0}});
	vec[1] = make_mat<t_mat<std::complex<t_real>>>({{0,-i}, {i,0}});
	vec[2] = make_mat<t_mat<std::complex<t_real>>>({{1,0}, {0,-1}});

	return vec;
}

template<template<class...> class t_mat=ublas::matrix,
	template<class...> class t_vec=ublas::vector,
	class t_real = double>
t_vec<t_mat<std::complex<t_real>>> get_ladder_ops()
{
	t_vec<t_mat<std::complex<t_real>>> vecS = get_spin_matrices();

	t_vec<t_mat<std::complex<t_real>>> vec(2);
	const std::complex<t_real> i(0,1);

	vec[0] = vecS[0] + i*vecS[1];	// up
	vec[1] = vecS[0] - i*vecS[1];	// down

	return vec;
}


template<class t_mat=ublas::matrix<double>>
t_mat commutator(const t_mat& A, const t_mat& B)
{
	t_mat AB = prod_mm(A, B);
	t_mat BA = prod_mm(B, A);
	return AB - BA;
}

/**
 * spin rotation in SU(2)
 * see e.g. (Arfken 2013), p. 851
 */
template<template<class...> class t_mat = ublas::matrix, class t_real = double>
t_mat<std::complex<t_real>> rot_spin(int iComp, t_real dAngle)
{
	const auto vecS = get_spin_matrices<t_mat, ublas::vector, t_real>();
	const auto matI = unit_m<t_mat<std::complex<t_real>>>(2);
	const std::complex<t_real> I(0,1);

	t_mat<std::complex<t_real>> mat =
		std::complex<t_real>(std::cos(t_real(0.5)*dAngle)) * matI +
		std::complex<t_real>(std::sin(t_real(0.5)*dAngle)) * I*vecS[iComp];
	return mat;
}


/**
 * CG coefficients
 * formula: see (Arfken 2013), p. 790
 *
 * e.g. two e- spins: s1 = s2 = 0.5, ms[1,2] = 0.5 (up) or -0.5 (down), S = 0 (sing.) or 1 (trip.)
 */
template<class T = double>
T CG_coeff(T S, T s1, T s2, T ms1, T ms2)
{
	T (*fak)(T) = [](T t) -> T { return boost::math::factorial<T>(t); };

	T tCG = fak(S + s1 - s2)*fak(S - s1 + s2)*fak(-S + s1 + s2);
	tCG *= (T(2)*S + T(1));
	tCG *= fak(S + ms1 + ms2) * fak(S - (ms1 + ms2));
	tCG *= fak(s1 + ms1) * fak(s1 - ms1);
	tCG *= fak(s2 + ms2) * fak(s2 - ms2);
	tCG /= fak(S + s1 + s2 + T(1));
	tCG = std::sqrt(tCG);

	auto k_fkt = [&](T k) -> T
	{
		T t = std::pow(T(-1), k);
		t /= fak(k);
		t /= fak(-S + s1 + s2 - k)*fak(S - s1 - ms2 + k)*fak(S - s2 + ms1 + k);
		t /= fak(s2 + ms2 - k)*fak(s1 - ms1 - k);
		return t;
	};

	auto k_minmax = [&]() -> std::pair<T,T>
	{
		T kmax = s1 - ms1;
		kmax = std::min(kmax, s2 + ms2);
		kmax = std::min(kmax, -S + s1 + s2);

		T kmin = -(S - s1 - ms2);
		kmin = std::max(kmin, -(S - s2 + ms1));
		kmin = std::max(kmin, T(0));

		return std::make_pair(kmin, kmax);
	};

	T kmin, kmax;
	std::tie(kmin, kmax) = k_minmax();
	T kfact = T(0);
	for(T k=kmin; k<=kmax; k+=T(1))
		kfact += k_fkt(k);
	tCG *= kfact;

	return tCG;
}


/**
 * Hund's rules
 * see e.g.: (Khomskii 2014), ch. 2.2
 * @return [S, L, J]
 */
template<class t_real = double>
std::tuple<t_real, t_real, t_real>
hund(std::uint16_t l, std::uint16_t iNumEs)
{
	std::uint16_t iNumOrbitals = 2*l+1;
	if(iNumEs > iNumOrbitals*2)
		throw Err("Too many electrons.");

	std::vector<std::uint8_t> vecOrbitals;	// orbitals
	std::vector<std::int16_t> vec_ml;		// mag. q.number
	vecOrbitals.resize(iNumOrbitals);
	vec_ml.resize(iNumOrbitals);
	std::iota(vec_ml.rbegin(), vec_ml.rend(), -l);

	for(std::uint16_t iE=0; iE<iNumEs; ++iE)
		++vecOrbitals[iE%iNumOrbitals];

	t_real S=0, L=0, J=0;
	for(std::size_t iOrbital=0; iOrbital<vecOrbitals.size(); ++iOrbital)
	{
		std::uint8_t iEs = vecOrbitals[iOrbital];
		if(iEs==1)	// unpaired electron
			S += t_real(0.5);

		std::int16_t ml = vec_ml[iOrbital];
		L += t_real(std::int16_t(iEs)*ml);
	}

	if(iNumEs <= iNumOrbitals)
		J = std::abs(L-S);
	else
		J = L+S;

	return std::make_tuple(S,L,J);
}


template<class t_real=double, class t_str=std::string>
t_str get_termsymbol(t_real S, t_real L, t_real J)
{
	static const std::vector<t_str> vecL =
		{"S","P","D","F","G","H","I","K","L","M","N","O"};

	t_str strS = var_to_str<t_real, t_str>(t_real(2)*S+1);
	t_str strL = vecL[std::size_t(L)];
	t_str strJ = var_to_str<t_real, t_str>(J);

	return strS + strL + strJ;
}


/**
* transforms e.g. 1s2 -> [1,0,2]
* @return [n, l, #electrons]
*/
template<class t_str=std::string>
std::tuple<uint16_t, uint16_t, uint16_t>
get_orbital(const t_str& strOrbital)
{
	using t_ch = typename t_str::value_type;
	static const std::unordered_map<t_ch, uint16_t> mapSubOrbitals =
	{
		{'s',0}, {'p',1}, {'d',2}, {'f',3},
		{'g',4}, {'h',5}, {'i',6}, {'k',7},
		{'l',8}, {'m',9}, {'n',10}, {'o',11},
	};

	std::istringstream istr(strOrbital);
	uint16_t n = 0, l = 0, iNumE = 0;
	t_ch cSub = 's';

	istr >> n >> cSub >> iNumE;
	auto iter = mapSubOrbitals.find(cSub);
	if(iter == mapSubOrbitals.end())
		throw Err("Invalid orbital.");
	l = iter->second;

	return std::make_tuple(n,l,iNumE);
}


/**
 * gets term symbol from orbitals
 * @return [S, L, J]
 */
template<class t_real=double, class t_str=std::string>
std::tuple<t_real, t_real, t_real>
hund(const t_str& strOrbitals)
{
	std::tuple<t_real, t_real, t_real> tupTerm(0,0,0);
	std::vector<t_str> vecOrbitals;
	get_tokens<t_str,t_str>(strOrbitals, " ,;", vecOrbitals);

	// all orbitals
	for(const t_str& strOrbital : vecOrbitals)
	{
		std::tuple<uint16_t, uint16_t, uint16_t> tup_nle =
			get_orbital(strOrbital);
		std::tuple<t_real, t_real, t_real> tup =
			hund(std::get<1>(tup_nle), std::get<2>(tup_nle));

		std::get<0>(tupTerm) += std::get<0>(tup);
		std::get<1>(tupTerm) += std::get<1>(tup);
		std::get<2>(tupTerm) += std::get<2>(tup);
	}

	return tupTerm;
}

/**
 * effective g factor
 * see: (Khomskii 2014), equ. (2.13)
 */
template<class T = double>
T eff_gJ(T S, T L, T J, T gL=T(1), T gS=T(2))
{
	T g = T(0.5) * (gL+gS) -
		(S*(S+T(1)) - L*(L+T(1)))
			/ (T(2)*J*(J+T(1))) * (gL-gS);
	return g;
}


/**
 * effective magneton number in units of muB
 * see: (Khomskii 2014), p. 33
 */
template<class T = double>
T eff_magnetons(T gJ, T J)
{
	return gJ * std::sqrt(J * (J+T(1)));
}
// ------------------------------------------------------------------------------------------------




// ------------------------------------------------------------------------------------------------
template<class T1, class T2,
	LinalgType ty1=get_linalg_type<T1>::value,
	LinalgType ty2=get_linalg_type<T2>::value>
struct linalg_mult_op_impl;

// vec * vec
template<class T1, class T2>
struct linalg_mult_op_impl<T1, T2,
	LinalgType::VECTOR, LinalgType::VECTOR>
{
	typedef typename T1::value_type ret_type;

	ret_type operator()(const T1& vec1, const T2& vec2) const
	{
		//typedef typename T1::value_type REAL;
		return inner(vec1, vec2);
	}
};

// mat * mat
template<class T1, class T2>
struct linalg_mult_op_impl<T1, T2,
	LinalgType::MATRIX, LinalgType::MATRIX>
{
	typedef T1 ret_type;

	ret_type operator()(const T1& mat1, const T2& mat2) const
	{
		return prod_mm(mat1, mat2);
	}
};

// mat * vec
template<class T1, class T2>
struct linalg_mult_op_impl<T1, T2,
	LinalgType::MATRIX, LinalgType::VECTOR>
{
	typedef T2 ret_type;

	ret_type operator()(const T1& mat, const T2& vec) const
	{
		return prod_mv(mat, vec);
	}
};



template<class T1, class T2>
typename linalg_mult_op_impl<T1, T2>::ret_type mult(
	const typename std::enable_if<get_linalg_type<T1>::value != LinalgType::UNKNOWN, T1>::type& t1,
	const typename std::enable_if<get_linalg_type<T1>::value != LinalgType::UNKNOWN, T2>::type& t2)
{
	return linalg_mult_op_impl<T1, T2>()(t1, t2);
}


// ----------------------------------------------------------------------------


template<class T1, class T2, class EPS,
	LinalgType ty1 = get_linalg_type<T1>::value,
	LinalgType ty2 = get_linalg_type<T2>::value>
struct linalg_equ_op_impl;

template<class T1, class T2, class EPS>
struct linalg_equ_op_impl<T1, T2, EPS,
	LinalgType::MATRIX, LinalgType::MATRIX>
{
	bool operator()(const T1& mat1, const T2& mat2, EPS eps) const
	{
		return mat_equal(mat1, mat2, eps);
	}
};

template<class T1, class T2, class EPS>
struct linalg_equ_op_impl<T1, T2, EPS,
	LinalgType::VECTOR, LinalgType::VECTOR>
{
	bool operator()(const T1& vec1, const T2& vec2, EPS eps) const
	{
		return vec_equal(vec1, vec2, eps);
	}
};


template<class T1, class T2>
bool equ(
	const typename std::enable_if<get_linalg_type<T1>::value != LinalgType::UNKNOWN, T1>::type& t1,
	const typename std::enable_if<get_linalg_type<T2>::value != LinalgType::UNKNOWN, T2>::type& t2,
	typename _get_epsilon_impl<T1>::t_eps eps = get_epsilon<T1>())
{
	return linalg_equ_op_impl<T1, T2, decltype(eps)>()(t1, t2, eps);
}



#ifdef USE_LINALG_OPS
template<class T1, class T2>
typename linalg_mult_op_impl<T1, T2>::ret_type operator*
	(const T1& t1, const T2& t2)
{
	return mult<T1, T2>(t1, t2);
}


template<class T1, class T2>
bool operator==(const T1& t1, const T2& t2)
{
	typename _get_epsilon_impl<T1>::t_eps eps = get_epsilon<T1>();
	return equ<T1, T2>(t1, t2, eps);
}
#endif
// ------------------------------------------------------------------------------------------------




// ------------------------------------------------------------------------------------------------
// quaternion ops

template<class t_quat = math::quaternion<double>>
t_quat unit_quat()
{
	return t_quat(1, 0,0,0);
}

/**
 * calculates the quaternion inverse
 * @desc see e.g.: (Bronstein 2008), Ch. 4
 */
template<class t_quat = math::quaternion<double>>
t_quat quat_inverse(const t_quat& q)
{
	t_quat qc = math::conj(q);
	return qc / (q*qc);
}

/**
 * quaternion product
 * @desc see: (Kuipers 2002), p. 110
 */
template<class t_quat = math::quaternion<double>>
t_quat quat_prod(const t_quat& q1, const t_quat& q2)
{
	using T = typename t_quat::value_type;
	using t_vec = ublas::vector<T>;

	T r1 = q1.R_component_1();
	T r2 = q2.R_component_1();

	t_vec vec1 = make_vec<t_vec>({q1.R_component_2(), q1.R_component_3(), q1.R_component_4()});
	t_vec vec2 = make_vec<t_vec>({q2.R_component_2(), q2.R_component_3(), q2.R_component_4()});

	T r = r1*r2 - mult<t_vec, t_vec>(vec1, vec2);
	t_vec vec = r1*vec2 + r2*vec1 + cross_3(vec1, vec2);

	return t_quat(r, vec[0], vec[1], vec[2]);
}


// ------------------------------------------------------------------------------------------------



// ------------------------------------------------------------------------------------------------
// SO(3)

/**
 * 3x3 matrix -> quat
 * @desc algo from: http://www.j3d.org/matrix_faq/matrfaq_latest.html#Q55
 */
template<class mat_type = ublas::matrix<double>,
	class quat_type = math::quaternion<typename mat_type::value_type>>
quat_type rot3_to_quat(const mat_type& rot)
{
	using T = typename quat_type::value_type;
	const T tr = trace(rot);
	T v[3], w;

	if(tr > T(0))								// scalar component is largest
	{
		w = T(0.5) * std::sqrt(tr+T(1));
		v[0] = (rot(2,1) - rot(1,2)) / (T(4)*w);
		v[1] = (rot(0,2) - rot(2,0)) / (T(4)*w);
		v[2] = (rot(1,0) - rot(0,1)) / (T(4)*w);
	}
	else
	{
		for(std::size_t iComp=0; iComp<3; ++iComp)	// find largest vector component
		{
			const std::size_t iM = iComp;			// major comp.
			const std::size_t im1 = (iComp+1)%3;	// minor comp. 1
			const std::size_t im2 = (iComp+2)%3;	// minor comp. 2

			if(rot(iM,iM)>=rot(im1,im1) && rot(iM,iM)>=rot(im2,im2))
			{
				v[iM] = T(0.5) * std::sqrt(T(1) + rot(iM,iM) - rot(im1,im1) - rot(im2,im2));
				v[im1] = (rot(im1, iM) + rot(iM, im1)) / (v[iM]*T(4));
				v[im2] = (rot(iM, im2) + rot(im2, iM)) / (v[iM]*T(4));
				w = (rot(im2,im1) - rot(im1,im2)) / (v[iM]*T(4));

				break;
			}

			if(iComp>=2) throw Err("rot3_to_quat: Invalid condition.");
		}
	}

	quat_type quatRet(w, v[0],v[1],v[2]);
	T norm_eucl = math::abs(quatRet);
	return quatRet / norm_eucl;
}


/**
 * quat -> 3x3 matrix
 * @desc see e.g.: (Bronstein 2008), Formulas (4.162a/b)
 */
template<class mat_type=ublas::matrix<double>,
	class quat_type=math::quaternion<typename mat_type::value_type>>
mat_type quat_to_rot3(const quat_type& quat)
{
	const quat_type cquat = math::conj(quat);
	const quat_type i(0,1,0,0), j(0,0,1,0), k(0,0,0,1);

	const quat_type cols[] =
	{
		quat * i * cquat,
		quat * j * cquat,
		quat * k * cquat
	};

	mat_type mat(3,3);
	for(std::size_t icol=0; icol<3; ++icol)
	{
		mat(0, icol) = cols[icol].R_component_2();
		mat(1, icol) = cols[icol].R_component_3();
		mat(2, icol) = cols[icol].R_component_4();
	}

	return mat;
}

// ------------------------------------------------------------------------------------------------



// ------------------------------------------------------------------------------------------------
// vector ops

/**
 * vector -> quat
 * @desc see: (Kuipers 2002), p. 114
 */
template<class t_vec = ublas::vector<double>,
	class t_quat = math::quaternion<typename t_vec::value_type>>
t_quat vec3_to_quat(const t_vec& vec)
{
	using T = typename t_vec::value_type;
	return t_quat(T(0), vec[0], vec[1], vec[2]);
}

/**
 * quat, vector product
 * @desc see: (Kuipers 2002), p. 127
 */
template<class t_vec = ublas::vector<double>,
	class t_quat = math::quaternion<typename t_vec::value_type>>
t_vec quat_vec_prod(const t_quat& q, const t_vec& v)
{
	t_quat qv = vec3_to_quat<t_vec, t_quat>(v);
	t_quat qvq =  q * qv * math::conj(q);

	t_vec vec(3);
	vec[0] = qvq.R_component_2();
	vec[1] = qvq.R_component_3();
	vec[2] = qvq.R_component_4();
	return vec;
}

// ------------------------------------------------------------------------------------------------


/**
 * quat -> complex 2x2 matrix
 * @desc see e.g. (Scherer 2010), p.173
 */
template<template<class...> class t_mat = ublas::matrix,
	class t_real = double,
	class t_quat = math::quaternion<t_real>>
t_mat<std::complex<t_real>> quat_to_cmat(const t_quat& quat)
{
	const auto vecS = get_spin_matrices<t_mat, ublas::vector, t_real>();
	const auto matI = unit_m<t_mat<std::complex<t_real>>>(2);

	t_mat<std::complex<t_real>> mat =
		std::complex<t_real>(quat.R_component_1()) * matI +
		std::complex<t_real>(quat.R_component_2()) * vecS[0] +
		std::complex<t_real>(quat.R_component_3()) * vecS[1] +
		std::complex<t_real>(quat.R_component_4()) * vecS[2];
	return mat;
}


// ------------------------------------------------------------------------------------------------
// rotation axis

template<class quat_type = math::quaternion<double>,
	typename T = typename quat_type::value_type>
std::vector<T> quat_to_euler(const quat_type& quat);

template<typename T=double, class... Args>
std::vector<T> rotation_angle(const ublas::matrix<T, Args...>& rot)
{
	std::vector<T> vecResult;

	if(rot.size1()!=rot.size2())
		return vecResult;
	if(rot.size1()<2)
		return vecResult;

	if(rot.size2()==2)
	{
		// rot = ( c -s )
		//       ( s  c )
		T angle = std::atan2(rot(1,0), rot(0,0));
		vecResult.push_back(angle);
	}
	else if(rot.size2()==3)
	{
		math::quaternion<T> quat =
			rot3_to_quat<decltype(rot), math::quaternion<T>>(rot);
		vecResult = quat_to_euler(quat);
	}

	return vecResult;
}


/**
 * rotation angle
 * @desc see: https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation#Quaternion-derived_rotation_matrix
 */
template<typename T=double>
T rotation_angle(const math::quaternion<T>& quat)
{
	//return 2.*std::asin(math::abs(math::unreal(quat)));
	return T(2)*std::acos(quat.R_component_1());
}


/**
 * quat -> rotation axis
 * @desc see e.g.: (Bronstein 2008), Ch. 4
 */
template<class t_vec = ublas::vector<double>>
t_vec rotation_axis(const math::quaternion<typename t_vec::value_type>& quat)
{
	using T = typename t_vec::value_type;

	t_vec vec(3);
	vec[0] = quat.R_component_2();
	vec[1] = quat.R_component_3();
	vec[2] = quat.R_component_4();

	typename t_vec::value_type angle = rotation_angle(quat);
	vec /= std::sin(T(0.5)*angle);

	return vec;
}


/**
 * rotation axis -> quat
 * @desc see e.g.: (Bronstein 2008), formula (4.193)
 */
template<class quat_type = math::quaternion<double>,
	class vec_type = ublas::vector<typename quat_type::value_type>,
	typename T = typename quat_type::value_type>
quat_type rotation_quat(const vec_type& vec, const T angle)
{
	const T s = std::sin(T(0.5)*angle);
	const T c = std::cos(T(0.5)*angle);
	const T n = std::sqrt(vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]);

	const T x = s * vec[0] / n;
	const T y = s * vec[1] / n;
	const T z = s * vec[2] / n;
	const T r = c;

	return quat_type(r, x,y,z);
}


/**
 * quaternion to rotate vec0 into vec1
 */
template<class t_quat = math::quaternion<double>,
	class t_vec = ublas::vector<typename t_quat::value_type>,
	typename T = typename t_quat::value_type>
t_quat rotation_quat(const t_vec& _vec0, const t_vec& _vec1)
{
	t_vec vec0 = _vec0 / veclen(_vec0);
	t_vec vec1 = _vec1 / veclen(_vec1);

	if(vec_equal(vec0, vec1))
	{ // parallel vectors -> do nothing
		return unit_quat<t_quat>();
	}
	else if(vec_equal(vec0, t_vec(-vec1)))
	{ // antiparallel vectors -> rotate about any perpendicular axis
		t_vec vecPerp(3);
		vecPerp[0] = vec0[2];
		vecPerp[1] = 0;
		vecPerp[2] = -vec0[0];
		return rotation_quat<t_quat, t_vec, T>(vecPerp, pi<T>);
	}

	t_vec veccross = cross_3<t_vec>(vec0, vec1);

	T dC = mult<t_vec, t_vec>(vec0, vec1);
	T dS = veclen(veccross);
	T dAngle = std::atan2(dS, dC);

	return rotation_quat<t_quat, t_vec, T>(veccross, dAngle);
}


template<class quat_type=math::quaternion<double>, typename T = typename quat_type::value_type>
quat_type rotation_quat_x(typename quat_type::value_type angle)
{
	return quat_type(std::cos(T(0.5)*angle),
		std::sin(T(0.5)*angle), T(0), T(0));
}

template<class quat_type=math::quaternion<double>, typename T = typename quat_type::value_type>
quat_type rotation_quat_y(typename quat_type::value_type angle)
{
	return quat_type(std::cos(T(0.5)*angle),
		T(0), std::sin(T(0.5)*angle), T(0));
}

template<class quat_type=math::quaternion<double>, typename T = typename quat_type::value_type>
quat_type rotation_quat_z(typename quat_type::value_type angle)
{
	return quat_type(std::cos(T(0.5)*angle),
		T(0), T(0), std::sin(T(0.5)*angle));
}

// ------------------------------------------------------------------------------------------------



// ------------------------------------------------------------------------------------------------
// Euler angles

/**
 * XYZ euler angles -> quat
 * @desc see: (Kuipers 2002), pp. 166, 167
 */
template<class t_quat = math::quaternion<double>,
	typename T = typename t_quat::value_type>
t_quat euler_to_quat_xyz(T phi, T theta, T psi)
{
	t_quat q1 = rotation_quat_x<t_quat>(phi);
	t_quat q2 = rotation_quat_y<t_quat>(theta);
	t_quat q3 = rotation_quat_z<t_quat>(psi);

	return q3 * q2 * q1;
}

/**
 * ZXZ euler angles -> quat
 * @desc see: (Kuipers 2002), pp. 166, 167
 */
template<class t_quat = math::quaternion<double>,
	typename T = typename t_quat::value_type>
t_quat euler_to_quat_zxz(T phi, T theta, T psi)
{
	t_quat q1 = rotation_quat_z<t_quat>(phi);
	t_quat q2 = rotation_quat_x<t_quat>(theta);
	t_quat q3 = rotation_quat_z<t_quat>(psi);

	return q3 * q2 * q1;
}


/**
 * quat -> XYZ euler angles
 */
template<class quat_type, typename T>
std::vector<T> quat_to_euler_xyz(const quat_type& quat)
{
	T q[] = { quat.R_component_1(), quat.R_component_2(),
		quat.R_component_3(), quat.R_component_4() };

	// formulas from:
	// http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
	T phi = std::atan2(T(2)*(q[0]*q[1] + q[2]*q[3]), T(1)-T(2)*(q[1]*q[1] + q[2]*q[2]));
	T theta = std::asin(T(2)*(q[0]*q[2] - q[3]*q[1]));
	T psi = std::atan2(T(2)*(q[0]*q[3] + q[1]*q[2]), T(1)-T(2)*(q[2]*q[2] + q[3]*q[3]));

	return std::vector<T>({ phi, theta, psi });
}


// use XYZ version by default
template<class t_quat = math::quaternion<double>,
	typename T = typename t_quat::value_type>
t_quat euler_to_quat(T phi, T theta, T psi)
{
	return euler_to_quat_xyz<t_quat, T>(phi, theta, psi);
}

template<class quat_type, typename T>
std::vector<T> quat_to_euler(const quat_type& quat)
{
	return quat_to_euler_xyz<quat_type, T>(quat);
}

// ------------------------------------------------------------------------------------------------



/**
 * @desc see e.g.: (Bronstein 2008), formula (4.217)
 */
template<class quat_type=math::quaternion<double>,
	typename T = typename quat_type::value_type>
quat_type stereo_proj(const quat_type& quat)
{
	return (T(1)+quat) / (T(1)-quat);
}

template<class quat_type=math::quaternion<double>,
	typename T = typename quat_type::value_type>
quat_type stereo_proj_inv(const quat_type& quat)
{
	return (T(1)-quat) / (T(1)+quat);
}


template<class QUAT>
struct vec_angle_unsigned_impl<QUAT, LinalgType::QUATERNION>
{
	typename QUAT::value_type operator()(const QUAT& q1, const QUAT& q2) const
	{
		typedef typename QUAT::value_type REAL;
		REAL dot = q1.R_component_1() * q2.R_component_1() +
			q1.R_component_2() * q2.R_component_2() +
			q1.R_component_3() * q2.R_component_3() +
			q1.R_component_4() * q2.R_component_4();

		dot /= math::abs(q1);
		dot /= math::abs(q2);

		return std::acos(dot);
	}
};



//------------------------------------------------------------------------------


/**
 * trapezoid rule
 * see e.g.: https://en.wikipedia.org/wiki/Trapezoidal_rule
 */
template<class R=double, class A=double>
R numint_trap(const std::function<R(A)>& fkt,
	A x0, A x1)
{
	return R(0.5)*R(x1-x0) * (fkt(x0) + fkt(x1));
}

template<class R=double, class A=double>
R numint_trapN(const std::function<R(A)>& fkt,
	A x0, A x1, std::size_t N)
{
	const A xstep = A(x1-x0)/A(N);

	R xsum = fkt(x0) + fkt(x1);
	for(std::size_t i=1; i<N; ++i)
		xsum += R(2)*fkt(x0 + A(i)*xstep);

	xsum *= R(0.5)*R(xstep);
	return R(xsum);
}


/**
 * rectangle rule
 * see e.g.: https://en.wikipedia.org/wiki/Rectangle_method
 */
template<class R=double, class A=double>
R numint_rect(const std::function<R(A)>& fkt,
	A x0, A x1, std::size_t N)
{
	const A xstep = (x1-x0)/A(N);

	R xsum = R(0);
	for(std::size_t i=0; i<N; ++i)
		xsum += fkt(x0 + A(i)*xstep);

	xsum *= R(xstep);
	return xsum;
}


/**
 * Simpson's rule
 * see e.g.: https://en.wikipedia.org/wiki/Simpson%27s_rule
 */
template<class R=double, class A=double>
R numint_simp(const std::function<R(A)>& fkt,
	A x0, A x1)
{
	return (fkt(x0) + 4.*fkt(0.5*(x0+x1)) + fkt(x1)) * (x1-x0)/6.;
}

template<class R=double, class A=double>
R numint_simpN(const std::function<R(A)>& fkt,
	A x0, A x1, std::size_t N)
{
	const A xstep = (x1-x0)/A(N);
	R xsum = fkt(x0) + fkt(x1);

	for(std::size_t i=1; i<=N/2; ++i)
	{
		xsum += R(2) * fkt(x0 + A(2*i)*xstep);
		xsum += R(4) * fkt(x0 + A(2*i-1)*xstep);
	}
	xsum -= R(2)*fkt(x0 + A(2*N/2)*xstep);

	xsum *= R(xstep)/R(3);
	return xsum;
}


// --------------------------------------------------------------------------------


/**
 * convolution integral of fkt0 and fkt1
 */
template<class R=double, class A=double>
R convolute(const std::function<R(A)>& fkt0, const std::function<R(A)>& fkt1,
	A x, A x0, A x1, std::size_t N)
{
	std::function<R(A,A)> fkt = [&fkt0, &fkt1](A t, A tau) -> R
	{
		return fkt0(tau) * fkt1(t-tau);
	};

	// ... at fixed arg x
	//std::function<R(A)> fktbnd = std::bind1st(fkt, x); /*deprecated in C++17*/
	std::function<R(A)> fktbnd = [&fkt, &x](A t) -> R { return fkt(x, t); };

	return numint_simpN(fktbnd, x0, x1, N);
}



template<class cont_type = std::vector<double>>
cont_type convolute_discrete(const cont_type& f, const cont_type& g)
{
	const std::size_t M = f.size();
	const std::size_t N = g.size();

	cont_type conv;
	conv.reserve(M+N-1);

	for(std::size_t n=0; n<M+N-1; ++n)
	{
		typename cont_type::value_type val = 0.;

		for(std::size_t m=0; m<M; ++m)
			if(n>=m && n-m<N)
				val += f[m] * g[n-m];

		conv.push_back(val);
	}

	return conv;
}


// --------------------------------------------------------------------------------

/**
 * Newton iteration
 */
template<class T = double>
T newton(const std::function<T(T)>& fkt, const std::function<T(T)>& diff,
	T x, std::size_t imax = 128, T eps = std::numeric_limits<T>::epsilon())
{
	T xnew = x;
	for(std::size_t i=0; i<imax; ++i)
	{
		xnew = x - fkt(x)/diff(x);

		if(std::abs(xnew - x) < eps)
			break;

		x = xnew;
	}

	return xnew;
}


// --------------------------------------------------------------------------------


/**
 * mean value
 */
template<class vec_type>
typename vec_type::value_type mean_value(const vec_type& vec)
{
	typedef typename vec_type::value_type T;
	if(vec.size()==0) return T(0);
	else if(vec.size()==1) return vec[0];

	T tMean = vec[0];
	for(std::size_t i=1; i<vec.size(); ++i)
		tMean += vec[i];
	tMean /= vec.size();

	return tMean;
}

/**
 * mean value with given probability
 */
template<class vec_type_prob, class vec_type>
typename vec_type::value_type mean_value(const vec_type_prob& vecP, const vec_type& vec)
{
	typedef typename vec_type::value_type T;
	typedef typename vec_type_prob::value_type Tprob;
	std::size_t iSize = std::min(vecP.size(), vec.size());

	if(iSize==0) return T(0);

	T tMean = vecP[0]*vec[0];
	Tprob tProbTotal = vecP[0];
	for(std::size_t i=1; i<iSize; ++i)
	{
		tMean += vecP[i]*vec[i];
		tProbTotal += vecP[i];
	}
	tMean /= tProbTotal;

	return tMean;
}


/**
 * standard deviation of mean value, with correction factor
 * see e.g.: https://en.wikipedia.org/wiki/Bessel%27s_correction
 */
template<class vec_type>
typename vec_type::value_type std_dev(const vec_type& vec, bool bCorr=1)
{
	typedef typename vec_type::value_type T;
	if(vec.size()<=1) return T(0);

	T tProb = T(vec.size());
	if(bCorr) tProb -= T(1);

	T tMean = mean_value(vec);
	T t = T(0);
	for(const T& tval : vec)
		t += (tval-tMean) * (tval-tMean);
	t /= tProb;

	return std::sqrt(t);
}

/**
 * standard deviation with given probability
 */
template<class vec_type_prob, class vec_type>
typename vec_type::value_type std_dev(const vec_type_prob& vecP, const vec_type& vec)
{
	typedef typename vec_type::value_type T;
	std::size_t iSize = std::min(vecP.size(), vec.size());
	if(iSize<=1) return T(0);

	T tMean = mean_value<vec_type_prob, vec_type>(vecP, vec);
	T t = T(0);
	T tProbTotal = T(0);

	for(std::size_t iIdx = 0; iIdx<iSize; ++iIdx)
	{
		t += (vec[iIdx]-tMean)*(vec[iIdx]-tMean) * vecP[iIdx];
		tProbTotal += vecP[iIdx];
	}
	t /= tProbTotal;

	return std::sqrt(t);
}


/**
 * entropy of a discrete distribution
 * S = - < log p_i >
 * see e.g.: https://en.wikipedia.org/wiki/Entropy_(information_theory)
 */
template<class t_real=double, class t_func>
t_real entropy(const t_func& funcPdf, std::size_t iMax)
{
	t_real dS = 0;
	for(std::size_t iArg=0; iArg<iMax; ++iArg)
	{
		t_real dVal = funcPdf(t_real(iArg));
		if(!float_equal(dVal, t_real(0)))
			dS += dVal * std::log(dVal);
	}
	return -dS;
}

/**
 * entropy of a continuous distribution
 * S = - < log p(x_i) >
 * see e.g.: https://en.wikipedia.org/wiki/Entropy_(information_theory)
 */
template<class t_real=double, class t_func>
t_real entropy(const t_func& funcPdf, t_real dXMin, t_real dXMax, std::size_t iSteps=128)
{
	std::function<t_real(t_real)> fktInt = [&funcPdf](t_real dX) -> t_real
	{
		t_real dVal = funcPdf(dX);
		if(!float_equal(dVal, t_real(0)))
			return dVal * std::log(dVal);
		return t_real(0);
	};

	t_real dS = numint_simpN(fktInt, dXMin, dXMax, iSteps);
	return -dS;
}


// -----------------------------------------------------------------------------


/**
 * Stirling's formula for log(n!)
 * see e.g. https://en.wikipedia.org/wiki/Stirling%27s_approximation
 */
template<class t_real = double>
t_real log_nfac(t_real n)
{
	const t_real twopi = t_real(2) * pi<t_real>;
	return n*std::log(n) - n + std::log(twopi*t_real(n)) / t_real(2);
}


/**
 * combinatorics
 * see e.g.: https://de.wikipedia.org/wiki/Abz%C3%A4hlende_Kombinatorik
 */
template<class t_real = double, class t_uint = unsigned>
t_real combinatorics(t_uint n, t_uint k, bool bOrdered, bool bRepetition)
{
	t_real tVal = t_real(0);

	if(bOrdered)	// variation
	{
		if(bRepetition)	// repetition of particles
		{
			tVal = std::pow(t_real(n), t_real(k));
		}
		else	// no repetition of particles
		{
			t_real binom = boost::math::binomial_coefficient<t_real>(n, k);
			tVal = binom * boost::math::factorial<t_real>(k);
		}
	}
	else			// combination
	{
		if(bRepetition)
			tVal = boost::math::binomial_coefficient<t_real>(n+k-1, k);
		else
			tVal = boost::math::binomial_coefficient<t_real>(n, k);
	}

	return tVal;
}


/**
 * possibilities to distribute particles onto niveaus
 * see e.g.: https://de.wikipedia.org/wiki/Abz%C3%A4hlende_Kombinatorik
 */
template<class t_real = double, class t_uint = unsigned>
t_real particles_in_niveaus(t_uint iPart, t_uint iNiv, bool bDistinct, bool bOnePerNiveau)
{
	t_real tCnt = t_real(0);

	if(bDistinct)	// classical
	{
		if(bOnePerNiveau)
			tCnt = combinatorics<t_real, t_uint>(iNiv, iPart, true, false);
		else	// Boltzons
			tCnt = combinatorics<t_real, t_uint>(iNiv, iPart, true, true);
	}
	else	// qm
	{
		if(bOnePerNiveau)	// Fermions
			tCnt = combinatorics<t_real, t_uint>(iNiv, iPart, false, false);
		else	// Bosons
			tCnt = combinatorics<t_real, t_uint>(iNiv, iPart, false, true);
	}

	return tCnt;
}

template<class t_real = double, class t_uint = unsigned>
t_real bosons_in_niveaus(t_uint iPart, t_uint iNiv)
{ return particles_in_niveaus<t_real, t_uint>(iPart, iNiv, 0, 0); }

template<class t_real = double, class t_uint = unsigned>
t_real fermions_in_niveaus(t_uint iPart, t_uint iNiv)
{ return particles_in_niveaus<t_real, t_uint>(iPart, iNiv, 0, 1); }

template<class t_real = double, class t_uint = unsigned>
t_real boltzons_in_niveaus(t_uint iPart, t_uint iNiv)
{ return particles_in_niveaus<t_real, t_uint>(iPart, iNiv, 1, 0); }


// -----------------------------------------------------------------------------


enum class DistrType
{
	// continuous
	NORMAL, LOGNORMAL, CAUCHY, CHI2, STUDENT,
	FISHER, EXP, BETA, GAMMA, LOGISTIC,

	// discrete
	BINOMIAL, HYPERGEOMETRIC, POISSON,

	NONE,
};


// ----------------------------------------------------------------------------
/**
 * collect properties of the various distributions
 */
template<class t_real, class t_distr, class=void> struct distr_traits {};

template<class t_real, class t_distr>
struct distr_traits<t_real, t_distr, typename std::enable_if<std::is_same<t_distr, math::normal_distribution<t_real>>::value>::type>
{
	using value_type = t_real;
	static constexpr std::size_t iNumArgs = 2;
	static constexpr bool bIsDiscrete = 0;
	static constexpr DistrType distr_type = DistrType::NORMAL;
	static constexpr const char* pcName = "Normal";
	static constexpr const char* pcParam1 = "mu";
	static constexpr const char* pcParam2 = "sigma";
};

template<class t_real, class t_distr>
struct distr_traits<t_real, t_distr, typename std::enable_if<std::is_same<t_distr, math::lognormal_distribution<t_real>>::value>::type>
{
	using value_type = t_real;
	static constexpr std::size_t iNumArgs = 2;
	static constexpr bool bIsDiscrete = 0;
	static constexpr DistrType distr_type = DistrType::LOGNORMAL;
	static constexpr const char* pcName = "Log-Normal";
	static constexpr const char* pcParam1 = "mu";
	static constexpr const char* pcParam2 = "sigma";
};

template<class t_real, class t_distr>
struct distr_traits<t_real, t_distr, typename std::enable_if<std::is_same<t_distr, math::cauchy_distribution<t_real>>::value>::type>
{
	using value_type = t_real;
	static constexpr std::size_t iNumArgs = 2;
	static constexpr bool bIsDiscrete = 0;
	static constexpr DistrType distr_type = DistrType::CAUCHY;
	static constexpr const char* pcName = "Cauchy";
	static constexpr const char* pcParam1 = "mu";
	static constexpr const char* pcParam2 = "sigma";
};

template<class t_real, class t_distr>
struct distr_traits<t_real, t_distr, typename std::enable_if<std::is_same<t_distr, math::poisson_distribution<t_real>>::value>::type>
{
	using value_type = t_real;
	static constexpr std::size_t iNumArgs = 1;
	static constexpr bool bIsDiscrete = 1;
	static constexpr DistrType distr_type = DistrType::POISSON;
	static constexpr const char* pcName = "Poisson";
	static constexpr const char* pcParam1 = "lambda";
};

template<class t_real, class t_distr>
struct distr_traits<t_real, t_distr, typename std::enable_if<std::is_same<t_distr, math::binomial_distribution<t_real>>::value>::type>
{
	using value_type = t_real;
	static constexpr std::size_t iNumArgs = 2;
	static constexpr bool bIsDiscrete = 1;
	static constexpr DistrType distr_type = DistrType::BINOMIAL;
	static constexpr const char* pcName = "Binomial";
	static constexpr const char* pcParam1 = "n";
	static constexpr const char* pcParam2 = "p";
};

template<class t_real, class t_distr>
struct distr_traits<t_real, t_distr, typename std::enable_if<std::is_same<t_distr, math::hypergeometric_distribution<t_real>>::value>::type>
{
	using value_type = t_real;
	static constexpr std::size_t iNumArgs = 3;
	static constexpr bool bIsDiscrete = 1;
	static constexpr DistrType distr_type = DistrType::HYPERGEOMETRIC;
	static constexpr const char* pcName = "Hypergeometric";
	static constexpr const char* pcParam1 = "r";
	static constexpr const char* pcParam2 = "n";
	static constexpr const char* pcParam3 = "N";
};

template<class t_real, class t_distr>
struct distr_traits<t_real, t_distr, typename std::enable_if<std::is_same<t_distr, math::chi_squared_distribution<t_real>>::value>::type>
{
	using value_type = t_real;
	static constexpr std::size_t iNumArgs = 1;
	static constexpr bool bIsDiscrete = 0;
	static constexpr DistrType distr_type = DistrType::CHI2;
	static constexpr const char* pcName = "Chi^2";
	static constexpr const char* pcParam1 = "dof";
};

template<class t_real, class t_distr>
struct distr_traits<t_real, t_distr, typename std::enable_if<std::is_same<t_distr, math::students_t_distribution<t_real>>::value>::type>
{
	using value_type = t_real;
	static constexpr std::size_t iNumArgs = 1;
	static constexpr bool bIsDiscrete = 0;
	static constexpr DistrType distr_type = DistrType::STUDENT;
	static constexpr const char* pcName = "Student";
	static constexpr const char* pcParam1 = "dof";
};

template<class t_real, class t_distr>
struct distr_traits<t_real, t_distr, typename std::enable_if<std::is_same<t_distr, math::fisher_f_distribution<t_real>>::value>::type>
{
	using value_type = t_real;
	static constexpr std::size_t iNumArgs = 2;
	static constexpr bool bIsDiscrete = 0;
	static constexpr DistrType distr_type = DistrType::FISHER;
	static constexpr const char* pcName = "Fisher";
	static constexpr const char* pcParam1 = "dof1";
	static constexpr const char* pcParam2 = "dof2";
};

template<class t_real, class t_distr>
struct distr_traits<t_real, t_distr, typename std::enable_if<std::is_same<t_distr, math::exponential_distribution<t_real>>::value>::type>
{
	using value_type = t_real;
	static constexpr std::size_t iNumArgs = 1;
	static constexpr bool bIsDiscrete = 0;
	static constexpr DistrType distr_type = DistrType::EXP;
	static constexpr const char* pcName = "Exponential";
	static constexpr const char* pcParam1 = "lambda";
};

template<class t_real, class t_distr>
struct distr_traits<t_real, t_distr, typename std::enable_if<std::is_same<t_distr, math::beta_distribution<t_real>>::value>::type>
{
	using value_type = t_real;
	static constexpr std::size_t iNumArgs = 2;
	static constexpr bool bIsDiscrete = 0;
	static constexpr DistrType distr_type = DistrType::BETA;
	static constexpr const char* pcName = "Beta";
	static constexpr const char* pcParam1 = "alpha";
	static constexpr const char* pcParam2 = "beta";
};

template<class t_real, class t_distr>
struct distr_traits<t_real, t_distr, typename std::enable_if<std::is_same<t_distr, math::gamma_distribution<t_real>>::value>::type>
{
	using value_type = t_real;
	static constexpr std::size_t iNumArgs = 2;
	static constexpr bool bIsDiscrete = 0;
	static constexpr DistrType distr_type = DistrType::GAMMA;
	static constexpr const char* pcName = "Gamma";
	static constexpr const char* pcParam1 = "mu";
	static constexpr const char* pcParam2 = "sigma";
};

template<class t_real, class t_distr>
struct distr_traits<t_real, t_distr, typename std::enable_if<std::is_same<t_distr, math::logistic_distribution<t_real>>::value>::type>
{
	using value_type = t_real;
	static constexpr std::size_t iNumArgs = 2;
	static constexpr bool bIsDiscrete = 0;
	static constexpr DistrType distr_type = DistrType::LOGISTIC;
	static constexpr const char* pcName = "Logistic";
	static constexpr const char* pcParam1 = "mu";
	static constexpr const char* pcParam2 = "sigma";
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
/**
 * sort parameter names into runtime and static containers
 */
template<class t_distr_traits, class=void> struct _distr_params {};

template<class t_distr_traits>
struct _distr_params<t_distr_traits,
	typename std::enable_if<t_distr_traits::iNumArgs==1>::type>
{
	static constexpr std::array<const char*, t_distr_traits::iNumArgs> get_arr()
	{
		return std::array<const char*, t_distr_traits::iNumArgs>
		({ t_distr_traits::pcParam1 });
	}

	template<template<class...> class t_vec=std::vector>
	static t_vec<const char*> get_vec()
	{
		return t_vec<const char*>({ t_distr_traits::pcParam1 });
	}
};

template<class t_distr_traits>
struct _distr_params<t_distr_traits,
	typename std::enable_if<t_distr_traits::iNumArgs==2>::type>
{
	static constexpr std::array<const char*, t_distr_traits::iNumArgs> get_arr()
	{
		return std::array<const char*, t_distr_traits::iNumArgs>
		({ t_distr_traits::pcParam1, t_distr_traits::pcParam2 });
	}

	template<template<class...> class t_vec=std::vector>
	static t_vec<const char*> get_vec()
	{
		return t_vec<const char*>({ t_distr_traits::pcParam1, t_distr_traits::pcParam2 });
	}
};

template<class t_distr_traits>
struct _distr_params<t_distr_traits,
	typename std::enable_if<t_distr_traits::iNumArgs==3>::type>
{
	static constexpr std::array<const char*, t_distr_traits::iNumArgs> get_arr()
	{
		return std::array<const char*, t_distr_traits::iNumArgs>
		({ t_distr_traits::pcParam1, t_distr_traits::pcParam2, t_distr_traits::pcParam3 });
	}

	template<template<class...> class t_vec=std::vector>
	static t_vec<const char*> get_vec()
	{
		return t_vec<const char*>({ t_distr_traits::pcParam1, t_distr_traits::pcParam2,
			t_distr_traits::pcParam3 });
	}
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
/**
 * probability distribution base class
 */
template<class t_real=double>
class DistrBase
{
public:
	virtual t_real pdf(t_real x) const = 0;
	virtual t_real cdf(t_real x) const = 0;	// cdf(x) == P(X <= x)
	virtual t_real cdf_inv(t_real p) const = 0;

	virtual t_real operator()(t_real x) const { return pdf(x); }
};


/**
 * class for specific distributions
 */
template<class t_distr, class t_real=typename t_distr::value_type,
	std::size_t iParams=distr_traits<t_real, t_distr>::iNumArgs>
class Distr : public DistrBase<t_real>
{
public:
	using value_type = t_real;
	using distr_type = t_distr;
	using traits_type = distr_traits<t_real, t_distr>;

protected:
	t_distr distr;

public:
	template<std::size_t _iParams=iParams>
	Distr(t_real dParam1,
		typename std::enable_if<_iParams==1, void>::type* =nullptr)
		: distr(dParam1)
	{}

	template<std::size_t _iParams=iParams>
	Distr(t_real dParam1, t_real dParam2,
		typename std::enable_if<_iParams==2, void>::type* =nullptr)
		: distr(dParam1, dParam2)
	{}

	template<std::size_t _iParams=iParams>
	Distr(t_real dParam1, t_real dParam2, t_real dParam3,
		typename std::enable_if<_iParams==3, void>::type* =nullptr)
		: distr(dParam1, dParam2, dParam3)
	{}


	static constexpr const char* GetName()
	{ return traits_type::pcName; }

	static constexpr std::size_t GetNumParams()
	{ return traits_type::iNumArgs; }

	static constexpr std::array<const char*, GetNumParams()> GetParamNames()
	{ return _distr_params<traits_type>::get_arr(); }
	static std::vector<const char*> GetParamNamesVec()
	{ return _distr_params<traits_type>::get_vec(); }


	virtual t_real pdf(t_real x) const override
	{
		if(traits_type::bIsDiscrete) x = std::round(x);
		return math::pdf(distr, x);
	}

	virtual t_real cdf(t_real x) const override
	{
		if(traits_type::bIsDiscrete) x = std::round(x);
		return math::cdf(distr, x);
	}

	virtual t_real cdf_inv(t_real p) const override
	{
		return math::quantile(distr, p);
	}
};
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
/**
 * typedefs for specific distributions
 */
// continuous
template<class t_real> using t_normal_dist = Distr<math::normal_distribution<t_real>>;
template<class t_real> using t_lognormal_dist = Distr<math::lognormal_distribution<t_real>>;
template<class t_real> using t_cauchy_dist = Distr<math::cauchy_distribution<t_real>>;
template<class t_real> using t_chi2_dist = Distr<math::chi_squared_distribution<t_real>>;
template<class t_real> using t_student_dist = Distr<math::students_t_distribution<t_real>>;
template<class t_real> using t_fisher_dist = Distr<math::fisher_f_distribution<t_real>>;
template<class t_real> using t_exp_dist = Distr<math::exponential_distribution<t_real>>;
template<class t_real> using t_beta_dist = Distr<math::beta_distribution<t_real>>;
template<class t_real> using t_gamma_dist = Distr<math::gamma_distribution<t_real>>;
template<class t_real> using t_logistic_dist = Distr<math::logistic_distribution<t_real>>;

// discrete
template<class t_real> using t_poisson_dist = Distr<math::poisson_distribution<t_real>>;
template<class t_real> using t_binomial_dist = Distr<math::binomial_distribution<t_real>>;
template<class t_real> using t_hypergeo_dist = Distr<math::hypergeometric_distribution<t_real>>;



// ----------------------------------------------------------------------------




/**
 * calculates the covariance and the correlation matrices
 * covariance: C_ij = cov(X_i, X_j) = < (X_i - <X_i>) * (X_j - <X_j>) >
 * correlation: K_ij = C_ij / (sigma_i sigma_j)
 * see e.g.: http://www.itl.nist.gov/div898/handbook/pmc/section5/pmc541.htm
 * see also e.g.: (Arfken 2013) p. 1142
 */
template<typename T=double>
std::tuple<ublas::matrix<T>, ublas::matrix<T>>
covariance(const std::vector<ublas::vector<T>>& vecVals, const std::vector<T>* pProb = 0)
{
	using t_mat = ublas::matrix<T>;
	using t_vecvec = typename std::remove_reference<decltype(vecVals)>::type;
	using t_innervec_org = decltype(vecVals[0]);
	using t_innervec = typename std::remove_const<
		typename std::remove_reference<t_innervec_org>::type>::type;

	if(vecVals.size() == 0) return std::make_tuple(t_mat(), t_mat());

	// mean vector <X_i>
	t_innervec vecMean;
	if(pProb)
		vecMean = mean_value<std::vector<T>, t_vecvec>(*pProb, vecVals);
	else
		vecMean = mean_value<t_vecvec>(vecVals);

	t_mat matCov = zero_m<t_mat>(vecVals[0].size(), vecVals[0].size());
	T tSum = T(0);
	const std::size_t N = vecVals.size();

	for(std::size_t i=0; i<N; ++i)
	{
		T tprob = T(1);

		// X_i - <X_i>
		t_innervec vec = vecVals[i] - vecMean;

		// matrix elements, AA^t
		t_mat matOuter = outer(vec, vec);

		// probabilities for final averaging, <...>
		if(pProb)
		{
			tprob = (*pProb)[i];
			matOuter *= tprob;
		}

		matCov += matOuter;
		tSum += tprob;
	}

	// average, sometimes defined as C /= (N-1)
	matCov /= tSum /*-T(1)*/;


	// --------------------------------------------------------------------------------
	// correlation matrix
	t_innervec vecVar = diag_vec(matCov);
	t_innervec vecStdDev(vecVar.size());

	std::transform(vecVar.begin(), vecVar.end(), vecStdDev.begin(),
		[](typename t_innervec::value_type d) -> typename t_innervec::value_type
		{ return std::sqrt(d); });

	t_mat matStdDev = outer(vecStdDev, vecStdDev);
	t_mat matCorr = ublas::element_div(matCov, matStdDev);
	// --------------------------------------------------------------------------------

	return std::make_tuple(matCov, matCorr);
}


// -----------------------------------------------------------------------------


/**
 * calculates chi^2 distance of a function model to data points
 * chi^2 = sum( (y_i - f(x_i))^2 / sigma_i^2 )
 * see e.g.: (Arfken 2013), p. 1170
 */
template<class T, class t_func, class t_iter_dat=T*>
T chi2(const t_func& func, std::size_t N,
	const t_iter_dat x, const t_iter_dat y, const t_iter_dat dy)
{
	using t_dat = typename std::remove_pointer<t_iter_dat>::type;
	T tchi2 = T(0);

	for(std::size_t i=0; i<N; ++i)
	{
		T td = T(y[i]) - func(T(x[i]));
		T tdy = dy ? T(dy[i]) : T(0.1*td);	// 10% error if none given

		if(std::abs(tdy) < std::numeric_limits<t_dat>::min())
			tdy = std::numeric_limits<t_dat>::min();

		T tchi = T(td) / T(tdy);
		tchi2 += tchi*tchi;
	}

	return tchi2;
}

template<class t_vec, class t_func>
typename t_vec::value_type chi2(const t_func& func,
	const t_vec& x, const t_vec& y, const t_vec& dy)
{
	using T = typename t_vec::value_type;
	return chi2<T, t_func, T*>(func, x.size(), x.data(), y.data(),
		dy.size() ? dy.data() : nullptr);
}


/**
 * chi^2 which doesn't use an x value, but an index instead: y[idx] - func(idx)
 */
template<class T, class t_func, class t_iter_dat=T*>
T chi2_idx(const t_func& func, std::size_t N, const t_iter_dat y, const t_iter_dat dy)
{
	using t_dat = typename std::remove_pointer<t_iter_dat>::type;
	T tchi2 = T(0);

	for(std::size_t i=0; i<N; ++i)
	{
		T td = T(y[i]) - func(i);
		T tdy = dy ? T(dy[i]) : T(0.1*td);	// 10% error if none given

		if(std::abs(tdy) < std::numeric_limits<t_dat>::min())
			tdy = std::numeric_limits<t_dat>::min();

		T tchi = T(td) / T(tdy);
		tchi2 += tchi*tchi;
	}

	return tchi2;
}


/**
 * direct chi^2 calculation with a model array instead of a model function
 */
template<class T, class t_iter_dat=T*>
T chi2_direct(std::size_t N, const t_iter_dat func_y, const t_iter_dat y, const t_iter_dat dy)
{
	using t_dat = typename std::remove_pointer<t_iter_dat>::type;
	T tchi2 = T(0);

	for(std::size_t i=0; i<N; ++i)
	{
		T td = T(y[i]) - T(func_y[i]);
		T tdy = dy ? T(dy[i]) : T(0.1*td);	// 10% error if none given

		if(std::abs(tdy) < std::numeric_limits<t_dat>::min())
			tdy = std::numeric_limits<t_dat>::min();

		T tchi = T(td) / T(tdy);
		tchi2 += tchi*tchi;
	}

	return tchi2;
}



/**
 * multi-dimensional chi^2 function
 */
template<class T, class T_dat, class t_func, template<class...> class t_vec=std::vector>
T chi2_nd(const t_func& func,
	const t_vec<t_vec<T_dat>>& vecvecX, const t_vec<T_dat>& vecY, const t_vec<T_dat>& vecDY)
{
	T tchi2 = T(0);

	for(std::size_t i=0; i<vecvecX.size(); ++i)
	{
		T td = T(vecY[i]) - func(vecvecX[i]);
		T tdy = vecDY[i];

		if(std::abs(tdy) < std::numeric_limits<T_dat>::min())
			tdy = std::numeric_limits<T_dat>::min();

		T tchi = T(td) / T(tdy);
		tchi2 += tchi*tchi;
	}

	return tchi2;
}


// -----------------------------------------------------------------------------


/**
 * Confidence interval of array data mean using t-distribution
 * see e.g.: (Arfken 2013), pp. 1176ff
 */
template<class t_real = double, class t_vec = std::vector<t_real>>
std::tuple<t_real, t_real, t_real>	// [mean, stddev, confidence]
confidence(const t_vec& vec, t_real dProb)
{
	t_real dMean = mean_value(vec);
	t_real dStd = std_dev(vec, true);

	t_real dDof = t_real(vec.size()-1);
	t_student_dist<t_real> t(dDof);
	t_real dConf = t.cdf_inv(0.5 + dProb/2.);
	dConf *= dStd / std::sqrt(dDof);

	return std::make_tuple(dMean, dStd, dConf);
}



//------------------------------------------------------------------------------

template<typename T = double>
bool solve_linear(const ublas::matrix<T>& M, const ublas::vector<T>& v, ublas::vector<T>& x);

template<typename T> class Line;

template<typename T> class Plane
{
public:
	using t_vec = ublas::vector<T>;
	using t_mat = ublas::matrix<T>;

protected:
	bool m_bValid = 0;
	t_vec m_vecX0;
	t_vec m_vecDir0, m_vecDir1;
	t_vec m_vecNorm;
	T m_d;

public:
	/**
	 * plane from a point and a normal
	 */
	Plane(const t_vec& vec0, const t_vec& vecNorm)
		: m_vecX0(vec0), m_vecNorm(vecNorm)
	{
		// normalise normal
		T tLenNorm = veclen(m_vecNorm);
		if(float_equal<T>(tLenNorm, 0.) || tLenNorm!=tLenNorm)
		{ m_bValid = 0; return; }
		m_vecNorm /= tLenNorm;

		// Hessian form: vecX0*vecNorm - d = 0
		m_d = inner(m_vecX0, m_vecNorm);


		// find direction vectors
		std::vector<t_vec> vecTry =
			{ make_vec({T(1), T(0), T(0)}),
			make_vec({T(0), T(1), T(0)}),
			make_vec({T(0), T(0), T(1)})};

		std::size_t iIdxBest = 0;
		T dDot = T(1);
		for(std::size_t iIdx=0; iIdx<vecTry.size(); ++iIdx)
		{
			const t_vec& vec = vecTry[iIdx];

			T dDotCur = std::abs(inner(vec, m_vecNorm));
			if(dDotCur < dDot)
			{
				iIdxBest = iIdx;
				dDot = dDotCur;
			}
		}
		m_vecDir0 = vecTry[iIdxBest];
		m_vecDir1 = cross_3(m_vecNorm, m_vecDir0);
		m_vecDir0 = cross_3(m_vecDir1, m_vecNorm);

		//m_vecDir0 /= veclen(m_vecDir0);
		//m_vecDir1 /= veclen(m_vecDir1);

		m_bValid = 1;
	}

	/**
	 * plane from a point and two directions on the plane
	 */
	Plane(const t_vec& vec0,
		const t_vec& dir0, const t_vec& dir1)
		: m_vecX0(vec0), m_vecDir0(dir0), m_vecDir1(dir1)
	{
		// calculate normal
		m_vecNorm = cross_3(dir0, dir1);

		// normalise normal
		T tLenNorm = veclen(m_vecNorm);
		if(float_equal<T>(tLenNorm, 0.) || tLenNorm!=tLenNorm)
		{ m_bValid = 0; return; }
		m_vecNorm /= tLenNorm;

		// Hessian form: vecX0*vecNorm - d = 0
		m_d = inner(m_vecX0, m_vecNorm);
		m_bValid = 1;
	}

	Plane() = default;
	~Plane() = default;


	const t_vec& GetX0() const { return m_vecX0; }
	const t_vec& GetDir0() const { return m_vecDir0; }
	const t_vec& GetDir1() const { return m_vecDir1; }
	const t_vec& GetNorm() const { return m_vecNorm; }
	const T& GetD() const { return m_d; }


	T GetDist(const t_vec& vecPt) const
	{
		return inner(vecPt, m_vecNorm) - m_d;
	}

	T GetAngle(const Plane<T>& plane) const
	{
		T dot = inner(GetNorm(), plane.GetNorm());
		return std::acos(dot);
	}

	T GetAngle(const t_vec& _vec) const
	{
		t_vec vec = _vec / veclen(_vec);
		T dot = inner(GetNorm(), vec);
		return std::asin(dot);
	}


	void FlipNormal()
	{
		m_vecNorm = -m_vecNorm;
		m_d = -m_d;

		std::swap(m_vecDir0, m_vecDir1);
	}


	/**
	 * "Lotfußpunkt"
	 */
	t_vec GetDroppedPerp(const t_vec& vecP, T *pdDist=0) const
	{
		T dist = GetDist(vecP);
		t_vec vecdropped = vecP - dist*m_vecNorm;

		if(pdDist)
		{
			t_vec vecD = vecP - vecdropped;
			*pdDist = std::sqrt(inner(vecD, vecD));
		}

		return vecdropped;
	}


	/**
	 * determine on which side of the plane a point is located
	 */
	bool GetSide(const t_vec& vecP, T *pdDist=0) const
	{
		T dDist = GetDist(vecP);
		if(pdDist) *pdDist = dDist;
		return dDist < T(0);
	}

	/**
	 * determine if a point is on the plane
	 */
	bool IsOnPlane(const t_vec& vecPt, T eps = get_epsilon<T>()) const
	{
		T dDist = GetDist(vecPt);
		return float_equal(dDist, T(0), eps);
	}

	bool IsParallel(const Plane<T>& plane, T eps = get_epsilon<T>()) const
	{
		return vec_is_collinear<t_vec>(GetNorm(), plane.GetNorm(), eps);
	}


	/**
	 * plane-plane intersection
	 * http://mathworld.wolfram.com/Plane-PlaneIntersection.html
	 */
	bool intersect(const Plane<T>& plane2, Line<T>& lineRet,
		T eps = get_epsilon<T>()) const
	{
		if(IsParallel(plane2, eps))
			return false;

		const Plane<T>& plane1 = *this;

		// direction vector
		t_vec vecDir = cross_3(plane1.GetNorm(), plane2.GetNorm());

		// find common point in the two planes
		t_mat M = row_matrix( { plane1.GetNorm(), plane2.GetNorm() } );

		t_vec vecD(2);
		vecD[0] = plane1.GetD();
		vecD[1] = plane2.GetD();

		t_vec vec0(3);
		if(!solve_linear(M, vecD, vec0))
			return 0;

		lineRet = Line<T>(vec0, vecDir);
		return true;
	}


	/**
	 * intersection point of three planes
	 * http://mathworld.wolfram.com/Plane-PlaneIntersection.html
	 */
	bool intersect(const Plane<T>& plane2, const Plane<T>& plane3, t_vec& ptRet,
		T eps = get_epsilon<T>()) const
	{
		const Plane<T>& plane1 = *this;

		// det
		const t_mat matNorms = row_matrix( { plane1.GetNorm(), plane2.GetNorm(), plane3.GetNorm() } );
		const T detM = determinant(matNorms);
		if(float_equal(detM, T(0), eps))
			return false;

		// direction vectors
		const t_vec vecDir12 = cross_3(plane1.GetNorm(), plane2.GetNorm());
		const t_vec vecDir23 = cross_3(plane2.GetNorm(), plane3.GetNorm());
		const t_vec vecDir31 = cross_3(plane3.GetNorm(), plane1.GetNorm());

		ptRet = (plane1.GetD()*vecDir23 + plane2.GetD()*vecDir31 + plane3.GetD()*vecDir12) / detM;
		if(is_nan_or_inf(ptRet))
			return false;
		return true;
	}


	bool IsValid() const { return m_bValid; }
};


//------------------------------------------------------------------------------


template<typename T> class Line
{
public:
	using t_vec = ublas::vector<T>;
	using t_mat = ublas::matrix<T>;

protected:
	t_vec m_vecX0;
	t_vec m_vecDir;

public:
	Line() {}
	Line(const t_vec& vec0, const t_vec& dir)
		: m_vecX0(vec0), m_vecDir(dir)
	{}

	~Line() = default;

	t_vec operator()(T t) const
	{
		return m_vecX0 + t*m_vecDir;
	}

	const t_vec& GetX0() const { return m_vecX0; }
	const t_vec& GetDir() const { return m_vecDir; }

	/**
	 * distance to line l1
	 */
	T GetDist(const Line<T>& l1) const
	{
		const Line<T>& l0 = *this;

		// vector normal to both directions defining the distance line
		t_vec vecNorm = cross_3(l0.GetDir(), l1.GetDir());
		T tlenNorm = veclen(vecNorm);

		t_vec vec01 = l1.GetX0() - l0.GetX0();

		// if the lines are parallel, any point (e.g. the X0s) can be used
		if(float_equal(tlenNorm, T(0)))
			return veclen(vec01);

		// project x0_1 - x0_0 onto vecNorm
		T tdot = std::abs(inner(vec01, vecNorm));
		return tdot / tlenNorm;
	}

	/**
	 * distance to a point
	 */
	T GetDist(const t_vec& vecPt) const
	{
		const t_vec& vecX0 = GetX0();
		const t_vec& vecDir = GetDir();

		T tlenDir = veclen(vecDir);

		// area of parallelogram spanned by vecDir and vecPt-vecX0
		// 	== ||vecDir||*||vecPt-vecX0||*sin(th)
		T tArea = veclen(cross_3(vecDir, vecPt-vecX0));

		// length of perpendicular line from vecPt, also by sine theorem
		// 	== ||vecPt-vecX0||*sin(th)
		return tArea / tlenDir;
	}

	bool IsParallel(const Line<T>& line, T eps = get_epsilon<T>()) const
	{
		return vec_is_collinear<t_vec>(GetDir(), line.GetDir(), eps);
	}


	T GetAngle(const Line<T>& line) const
	{
		t_vec dir1 = GetDir();
		t_vec dir2 = line.GetDir();

		dir1 /= veclen(dir1);
		dir2 /= veclen(dir2);

		T dot = inner(dir1, dir2);
		return std::acos(dot);
	}


	/**
	 * "Lotfußpunkt"
	 */
	t_vec GetDroppedPerp(const t_vec& vecP, T *pdDist=0) const
	{
		const t_vec& vecDir = GetDir();

		// projection of vecP-x0 onto vecDir
		T t = inner<t_vec>(vecP-GetX0(), vecDir) / inner(vecDir, vecDir);

		t_vec vecdropped = operator()(t);

		if(pdDist)
		{
			t_vec vecD = vecP - vecdropped;
			*pdDist = std::sqrt(inner(vecD, vecD));
		}

		return vecdropped;
	}


	/**
	 * determine on which side of the line a point is located
	 */
	bool GetSide(const t_vec& vecP, T *pdDist=0) const
	{
		const std::size_t N = m_vecDir.size();
		if(N != 2)
		{
			log_err("\"Side of line\" only defined for 2d vectors.");
			return false;
		}

		t_vec vecDropped = GetDroppedPerp(vecP, pdDist);


		t_vec vecNorm(2);
		vecNorm[0] = m_vecDir[1];
		vecNorm[1] = -m_vecDir[0];

		T tDot = inner<t_vec>(vecP-vecDropped, vecNorm);
		return tDot < T(0);
	}


	/**
	 * line-plane intersection
	 * http://mathworld.wolfram.com/Line-PlaneIntersection.html
	 */
	bool intersect(const Plane<T>& plane, T& t, T eps = get_epsilon<T>()) const
	{
		const std::size_t N = m_vecDir.size();
		if(N != 3)
		{
			log_err("Line-plane intersection only implemented for 3d vectors.");
			return false;
		}

		const t_vec& posl = this->GetX0();
		const t_vec& dirl = this->GetDir();

		const t_vec& xp0 = plane.GetX0();
		const t_vec xp1 = plane.GetX0() + plane.GetDir0();
		const t_vec xp2 = plane.GetX0() + plane.GetDir1();

		t_mat matDenom(N+1,N+1);
		matDenom(0,0) = 1;		matDenom(0,1) = 1;		matDenom(0,2) = 1;		matDenom(0,3) = 0;
		matDenom(1,0) = xp0[0];	matDenom(1,1) = xp1[0];	matDenom(1,2) = xp2[0];	matDenom(1,3) = dirl[0];
		matDenom(2,0) = xp0[1];	matDenom(2,1) = xp1[1];	matDenom(2,2) = xp2[1];	matDenom(2,3) = dirl[1];
		matDenom(3,0) = xp0[2];	matDenom(3,1) = xp1[2];	matDenom(3,2) = xp2[2];	matDenom(3,3) = dirl[2];

		T denom = determinant(matDenom);
		if(float_equal<T>(denom, 0., eps))
			return false;

		t_mat matNum(N+1,N+1);
		matNum(0,0) = 1;		matNum(0,1) = 1;		matNum(0,2) = 1;		matNum(0,3) = 1;
		matNum(1,0) = xp0[0];	matNum(1,1) = xp1[0];	matNum(1,2) = xp2[0];	matNum(1,3) = posl[0];
		matNum(2,0) = xp0[1];	matNum(2,1) = xp1[1];	matNum(2,2) = xp2[1];	matNum(2,3) = posl[1];
		matNum(3,0) = xp0[2];	matNum(3,1) = xp1[2];	matNum(3,2) = xp2[2];	matNum(3,3) = posl[2];

		T num = determinant(matNum);

		t = -num / denom;
		return true;
	}


	/**
	 * line-line intersection
	 * see e.g.: https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection
	 *
	 * pos0 + t0*dir0 = pos1 + t1*dir1
	 * pos0 - pos1 = t1*dir1 - t0*dir0
	 * exact: b = Mx  ->  M^(-1)*b = x
	 * approx: M^t b = M^t M x  ->  (M^t M)^(-1) * M^t b = x
	 */
	bool intersect(const Line<T>& line, T& t, T eps = get_epsilon<T>()) const
	{
		if(IsParallel(line, eps))
			return false;

		const t_vec& pos0 =  this->GetX0();
		const t_vec& pos1 =  line.GetX0();

		const t_vec& dir0 =  this->GetDir();
		const t_vec& dir1 =  line.GetDir();

		const t_vec pos = pos0-pos1;
		t_mat M = column_matrix({-dir0, dir1});
		t_mat Mt = transpose(M);
		t_mat MtM = prod_mm(Mt, M);

		t_mat MtMinv;
		if(!inverse(MtM, MtMinv))
			return false;

		t_vec Mtb = prod_mv(Mt, pos);
		t_vec params = prod_mv(MtMinv, Mtb);
		t = params[0];

		return true;
	}


	/**
	 * middle perpendicular line (in 2d)
	 */
	bool GetMiddlePerp(Line<T>& linePerp) const
	{
		const std::size_t N = m_vecDir.size();
		if(N != 2)
		{
			log_err("Perpendicular line only implemented for 2d vectors.");
			return false;
		}

		t_vec vecDir(2);
		vecDir[0] = -m_vecDir[1];
		vecDir[1] = m_vecDir[0];

		t_vec vecPos = this->operator()(0.5);

		linePerp = Line<T>(vecPos, vecDir);
		return true;
	}

	/**
	 * middle perpendicular plane (in 3d)
	 */
	bool GetMiddlePerp(Plane<T>& planePerp) const
	{
		const std::size_t N = m_vecDir.size();
		if(N != 3)
		{
			log_err("Perpendicular plane only implemented for 3d vectors.");
			return false;
		}

		t_vec vecPos = this->operator()(0.5);
		planePerp = Plane<T>(vecPos, m_vecDir);
		return true;
	}
};


template<typename T>
std::ostream& operator<<(std::ostream& ostr, const Plane<T>& plane)
{
	ostr << plane.GetX0() << " + s*" << plane.GetDir0()
		<< " + t*" << plane.GetDir1();
	return ostr;
}

template<typename T>
std::ostream& operator<<(std::ostream& ostr, const Line<T>& line)
{
	ostr << line.GetX0() << " + t*" << line.GetDir();
	return ostr;
}



/**
 * intersection of "plane" and polygon (defined by "planePoly" and vertPoly")
 */
template<class t_vec = ublas::vector<double>,
	template<class...> class t_cont = std::vector,
	class T = typename t_vec::value_type>
bool intersect_plane_poly(const Plane<T>& plane,
	const Plane<T>& planePoly, const t_cont<t_vec>& vertPoly,
	Line<T>& lineRes, T eps = get_epsilon<T>())
{
	if(!vertPoly.size())
		return false;

	bool bFirstSide = plane.GetSide(vertPoly[0]);

	// are all vertices on the same side?
	if(std::all_of(vertPoly.begin(), vertPoly.end(),
		[&plane, bFirstSide](const t_vec& vert) -> bool
		{ return plane.GetSide(vert) == bFirstSide; }))
		return false;


	if(!plane.intersect(planePoly, lineRes, eps))
		return false;

	return true;
}


/**
 * intersection of "line" and polygon (defined by "planePoly" and vertPoly")
 */
template<class t_vec = ublas::vector<double>,
	template<class...> class t_cont = std::vector,
	class T = typename t_vec::value_type>
bool intersect_line_poly(const Line<T>& line,
	const Plane<T>& planePoly, const t_cont<t_vec>& vertPoly,
	t_vec& vecIntersect, T eps = get_epsilon<T>())
{
	// point of intersection with plane
	T t;
	if(!line.intersect(planePoly, t, eps))
		return false;
	vecIntersect = line(t);

	// is intersection point within polygon?
	const t_vec vecFaceCentre = mean_value(vertPoly);

	//std::ostringstream ostr;
	//ostr << "intersect: " << vecIntersect << ", face centre: " << vecFaceCentre << "\n";

	for(std::size_t iVert = 0; iVert < vertPoly.size(); ++iVert)
	{
		std::size_t iNextVert = iVert < (vertPoly.size()-1) ? iVert+1 : 0;

		const t_vec vecEdge = vertPoly[iNextVert] - vertPoly[iVert];
		const t_vec vecEdgeCentre = vertPoly[iVert] + T(0.5)*vecEdge;
		const t_vec vecOut = vecFaceCentre - vecEdgeCentre;
		t_vec vecNorm = cross_3(vecEdge, planePoly.GetNorm());
		if(inner(vecNorm, vecOut) < T(0))
			vecNorm = -vecNorm;

		const Plane<T> planeEdge(vecEdgeCentre, vecNorm);

		if(planeEdge.GetDist(vecIntersect) < -eps)
			return false;

		//ostr << "Face " << iVert << ": " << vecEdgeCentre << ", " << vecNorm << ", "
		//	<< planeEdge.GetDist(vecIntersect) << "\n";
	}

	//std::cout << ostr.str() << std::endl;
	return true;
}



/**
 * sort vertices in a convex polygon
 */
template<class t_vec = ublas::vector<double>,
	template<class...> class t_cont = std::vector,
	class T = typename t_vec::value_type>
void sort_poly_verts_norm(t_cont<t_vec>& vecPoly, const t_vec& _vecNorm)
{
	if(vecPoly.size() <= 1)
		return;

	// line from centre to vertex
	const t_vec vecCentre = mean_value(vecPoly);
	const t_vec vecNorm = _vecNorm / veclen(_vecNorm);

	t_vec vec0 = vecPoly[0] - vecCentre;

	std::stable_sort(vecPoly.begin(), vecPoly.end(),
		[&vecCentre, &vec0, &vecNorm](const t_vec& vertex1, const t_vec& vertex2) -> bool
		{
			t_vec vec1 = vertex1 - vecCentre;
			t_vec vec2 = vertex2 - vecCentre;

			return vec_angle(vec0, vec1, &vecNorm) < vec_angle(vec0, vec2, &vecNorm);
		});
}


/**
 * sort vertices in a convex polygon using an absolute centre for determining the normal
 */
template<class t_vec = ublas::vector<double>,
	template<class...> class t_cont = std::vector,
	class T = typename t_vec::value_type>
void sort_poly_verts(t_cont<t_vec>& vecPoly, const t_vec& vecAbsCentre)
{
	if(vecPoly.size() <= 1)
		return;

	// line from centre to vertex
	const t_vec vecCentre = mean_value(vecPoly);
	// face normal
	t_vec vecNorm = vecCentre - vecAbsCentre;

	sort_poly_verts_norm<t_vec, t_cont, T>(vecPoly, vecNorm);
}


/**
 * sort vertices in a convex polygon determining normal
 */
template<class t_vec = ublas::vector<double>,
	template<class...> class t_cont = std::vector,
	class T = typename t_vec::value_type>
void sort_poly_verts(t_cont<t_vec>& vecPoly)
{
	if(vecPoly.size() <= 1)
		return;

	// line from centre to vertex
	const t_vec vecCentre = mean_value(vecPoly);
	// face normal
	t_vec vecNormBest;
	T tBestCross = T(0);

	// find non-collinear vectors
	for(std::size_t iVecPoly=1; iVecPoly<vecPoly.size(); ++iVecPoly)
	{
		t_vec vecNorm = cross_3<t_vec>(vecPoly[0]-vecCentre, vecPoly[1]-vecCentre);
		T tCross = veclen(vecNorm);
		if(tCross > tBestCross)
		{
			tBestCross = tCross;
			vecNormBest = vecNorm;
		}
	}

	// nothing found
	if(vecNormBest.size() < vecCentre.size())
		return;

	sort_poly_verts_norm<t_vec, t_cont, T>(vecPoly, vecNormBest);
}



/**
 * get the polygon's face normal vector
 */
template<class t_vec = ublas::vector<double>,
	template<class...> class t_cont = std::vector,
	class T = typename t_vec::value_type>
t_vec get_face_normal(const t_cont<t_vec>& vecVerts, t_vec vecCentre,
	T eps = get_epsilon<T>())
{
	t_vec vecNorm;

	if(vecVerts.size() < 3)
		return vecNorm;

	const t_vec& vec1 = vecVerts[1] - vecVerts[0];
	for(std::size_t iVec2 = 2; iVec2 < vecVerts.size(); ++iVec2)
	{
		const t_vec& vec2 = vecVerts[iVec2] - vecVerts[0];
		t_vec vecCross = cross_3(vec1, vec2);
		if(veclen(vecCross) > eps)
		{
			vecNorm = vecCross;
			break;
		}
	}

	// nothing found
	if(vecNorm.size() == 0)
		return vecNorm;


	// vector pointing "outwards"
	const t_vec vecPolyCentre = mean_value(vecVerts);
	t_vec vecCentreToFace = vecPolyCentre - vecCentre;
	vecCentreToFace /= veclen(vecCentreToFace);

	vecNorm /= veclen(vecNorm);
	if(inner(vecNorm, vecCentreToFace) < T(0))
		vecNorm = -vecNorm;

	//log_debug("vec = ", vecCentreToFace, ",\tnorm = ", vecNorm);
	return vecNorm;
}


/**
 * creates polyhedron faces out of vertices
 */
template<class t_vec = ublas::vector<double>,
	template<class...> class t_cont = std::vector,
	class T = typename t_vec::value_type>
t_cont<t_cont<t_vec>> verts_to_polyhedron(
	const t_cont<t_vec>& vecVerts, T eps = get_epsilon<T>())
{
	t_cont<t_cont<t_vec>> vecPolys;
	t_cont<Plane<T>> vecPlanes;

	// too few vertices
	if(vecVerts.size() < 3)
		return vecPolys;


	auto fkt_try_add_new_pt = [eps](t_cont<t_vec>& vecPoly, const t_vec& vecPt) -> void
	{
		for(const t_vec& vecOld : vecPoly)
		{
			// vecPt already in set?
			if(vec_equal(vecOld, vecPt, T(1)*eps))
				return;
		}

		// add new point
		vecPoly.push_back(vecPt);
	};


	const t_vec vecCentre = mean_value(vecVerts);


	for(std::size_t i=0; i<vecVerts.size(); ++i)
	{
		for(std::size_t j=i+1; j<vecVerts.size(); ++j)
		{
			//if(i==j) continue;
			for(std::size_t k=j+1; k<vecVerts.size(); ++k)
			{
				//if(i==k || j==k) continue;

				const t_vec& vert0 = vecVerts[i];
				const t_vec& vert1 = vecVerts[j];
				const t_vec& vert2 = vecVerts[k];

				bool bPointsUsed = 0;
				for(std::size_t iPlane=0; iPlane<vecPlanes.size(); ++iPlane)
				{
					const Plane<T>& plane = vecPlanes[iPlane];
					t_cont<t_vec>& vecPoly = vecPolys[iPlane];

					// plane already in set?
					if(plane.IsOnPlane(vert0, T(10)*eps) &&
						plane.IsOnPlane(vert1, T(10)*eps) &&
						plane.IsOnPlane(vert2, T(10)*eps))
					{
						// add new points to poly if not already present
						fkt_try_add_new_pt(vecPoly, vert0);
						fkt_try_add_new_pt(vecPoly, vert1);
						fkt_try_add_new_pt(vecPoly, vert2);

						bPointsUsed = 1;
						break;
					}
				}


				// new plane / poly
				if(!bPointsUsed)
				{
					t_cont<t_vec> vecPoly = { vert0, vert1, vert2 };
					sort_poly_verts<t_vec, t_cont, T>(vecPoly, vecCentre);
					Plane<T> plane(vecPoly[0], vecPoly[1]-vecPoly[0], vecPoly[2]-vecPoly[0]);

					// check if all other vertices are on the negative side of the polygon
					bool bIsHull = 1;
					for(const t_vec& vecOtherVert : vecVerts)
					{
						if(plane.GetDist(vecOtherVert) > T(10)*eps)
						{
							bIsHull = 0;
							break;
						}
					}

					if(bIsHull)
					{
						vecPolys.emplace_back(std::move(vecPoly));
						vecPlanes.emplace_back(std::move(plane));
					}
				}
			}
		}
	}

	// too few polygons => remove polyhedron
	if(vecPolys.size() < 3)
		vecPolys = decltype(vecPolys)();

	// sort vertices
	for(auto& vecPoly : vecPolys)
		sort_poly_verts<t_vec, t_cont, T>(vecPoly, vecCentre);

	return vecPolys;
}


//------------------------------------------------------------------------------


template<class T = double>
class Quadric
{
public:
	using t_vec = ublas::vector<T>;
	using t_mat = ublas::matrix<T>;

protected:
	// x^T Q x  +  r x  +  s  =  0
	t_mat m_Q = zero_m<t_mat>(3,3);
	t_vec m_r = zero_v<t_vec>(3);
	T m_s = 0;

	t_vec m_vecOffs = zero_v<t_vec>(3);
	bool m_bQSymm = 1;

protected:
	void CheckSymm()
	{
		m_bQSymm = is_symmetric(m_Q, std::cbrt(get_epsilon<T>()));
		//log_debug("Q = ", m_Q, ", symm: ", m_bQSymm);
	}

public:
	Quadric() {}
	Quadric(std::size_t iDim)
		: m_Q(zero_m<t_mat>(iDim,iDim)), m_r(zero_v<t_vec>(iDim))
	{ CheckSymm(); }
	Quadric(const t_mat& Q) : m_Q(Q)
	{ CheckSymm(); }
	Quadric(const t_mat& Q, const t_vec& r, T s)
		: m_Q(Q), m_r(r), m_s(s)
	{ CheckSymm(); }
	~Quadric() {}

	void SetDim(std::size_t iDim) { m_Q.resize(iDim, iDim, 1); }

	const Quadric<T>& operator=(const Quadric<T>& quad)
	{
		this->m_Q = quad.m_Q;
		this->m_r = quad.m_r;
		this->m_s = quad.m_s;
		this->m_vecOffs = quad.m_vecOffs;
		this->m_bQSymm = quad.m_bQSymm;

		return *this;
	}

	Quadric<T>& operator=(Quadric<T>&& quad)
	{
		this->m_Q = std::move(quad.m_Q);
		this->m_r = std::move(quad.m_r);
		this->m_s = std::move(quad.m_s);
		this->m_vecOffs = std::move(quad.m_vecOffs);
		this->m_bQSymm = quad.m_bQSymm;

		return *this;
	}

	Quadric(const Quadric<T>& quad) { *this = quad; }
	Quadric(Quadric<T>&& quad) { *this = quad; }

	void SetOffset(const t_vec& vec) { m_vecOffs = vec; }
	const t_vec& GetOffset() const { return m_vecOffs; }

	const t_mat& GetQ() const { return m_Q; }
	const t_vec& GetR() const { return m_r; }
	T GetS() const { return m_s; }

	void SetQ(const t_mat& Q) { m_Q = Q; CheckSymm(); }
	void SetR(const t_vec& r) { m_r = r; }
	void SetS(T s) { m_s = s; }

	T operator()(const t_vec& _x) const
	{
		t_vec x = _x-m_vecOffs;

		t_vec vecQ = prod_mv(m_Q, x);
		T dQ = inner(x, vecQ);
		T dR = inner(m_r, x);

		return dQ + dR + m_s;
	}

	/**
	 * remove column and row iIdx
	 */
	void RemoveElems(std::size_t iIdx)
	{
		m_Q = remove_elems(m_Q, iIdx);
		m_r = remove_elem(m_r, iIdx);
		m_vecOffs = remove_elem(m_vecOffs, iIdx);
	}

	void transform(const t_mat& S)
	{
		m_Q = tl2::transform<t_mat>(m_Q, S, 1);
		CheckSymm();
	}


	/**
	 * Q = O D O^T
	 * O: eigenvecs, D: eigenvals
	 */
	bool GetPrincipalAxes(t_mat& matEvecs, std::vector<T>& vecEvals,
		Quadric<T>* pquadPrincipal=nullptr) const
	{
		std::vector<t_vec> evecs;

		bool bEV = 0;
		if(m_bQSymm)
			bEV = eigenvec_sym(m_Q, evecs, vecEvals);
		else
			bEV = eigenvec_approxsym(m_Q, evecs, vecEvals);

		if(!bEV)
		{
			log_err("Cannot determine eigenvectors.");
			return false;
		}

		sort_eigenvecs<T>(evecs, vecEvals, 1,
			[](T d) -> T { return 1./std::sqrt(d); });

		if(determinant(matEvecs) < T(0) && evecs.size() >= 2)
		{
			std::swap(evecs[evecs.size()-2], evecs[evecs.size()-1]);
			std::swap(vecEvals[vecEvals.size()-2], vecEvals[vecEvals.size()-1]);
		}

		matEvecs = column_matrix(evecs);

		if(pquadPrincipal)
		{
			t_mat matEvals = diag_matrix(vecEvals);

			//log_debug("evecs = ", matEvecs);
			//log_debug("evals = ", matEvals);
			/*auto vecAngle = rotation_angle(matEvecs);
			if(vecAngle.size() >= 1)
			{
				T dAngle = vecAngle[0];
				log_debug("angle = ", dAngle);
			}*/

			pquadPrincipal->SetDim(vecEvals.size());
			pquadPrincipal->SetQ(matEvals);
			pquadPrincipal->SetS(GetS());

			t_mat matEvecsT = transpose(matEvecs);
			pquadPrincipal->SetR(prod_mv(matEvecsT, GetR()));
		}

		return true;
	}


	/**
	 * only valid in principal axis system:
	 * x^T Q x + rx = 0
	 * q11*x1^2 + r1*x1 + ... = 0
	 * q11*(x1^2 + r1/q11*x1) = 0
	 * completing the square: q11*(x1 + r1/(2*q11))^2 - r1^2/(4*q11)
	 */
	t_vec GetPrincipalOffset() const
	{
		t_vec vecOffs = GetR();
		//log_debug("offset in: ", vecOffs);

		for(std::size_t i=0; i<vecOffs.size(); ++i)
			vecOffs[i] /= -T(2)*GetQ()(i,i);

		//log_debug("offset out: ", vecOffs);
		return vecOffs;
	}


	/**
	 * here: only for x^T Q x + s  =  0, i.e. for r=0
	 * quad: x^T Q x + s = 0; line: x = x0 + t d
	 * (x0 + t d)^T Q (x0 + t d) + s = 0
	 * (x0 + t d)^T Q x0 + (x0 + t d)^T Q t d + s = 0
	 * (x0^T + t d^T) Q x0 + (x0^T + t d^T) Q t d + s = 0
	 * x0^T Q x0 + s  +  (d^T Q x0 + x0^T Q d) t  +  d^T Q d t^2 = 0
	 */
	std::vector<T> intersect(const Line<T>& line) const
	{
		const t_mat& Q = GetQ();
		const T& s = m_s;
		const t_vec& d = line.GetDir();
		const t_vec x0 = line.GetX0() - m_vecOffs;;

		// solving at^2 + bt + c = 0 for t
		t_vec vecQd = prod_mv(Q, d);
		T a = inner(d, vecQd);

		t_vec vecQx0 = prod_mv(Q, x0);
		T c = inner(x0, vecQx0) + s;

		T b = inner(x0, vecQd);
		b += inner(d, vecQx0);

		//std::cout << "a=" << a << ", b=" << b << ", c=" << c << std::endl;
		return quadratic_solve(a,b,c);
	}
};


template<class T = double>
std::ostream& operator<<(std::ostream& ostr, const Quadric<T>& quad)
{
	ostr << "Q = " << quad.GetQ() << ", ";
	//ostr << "r = " << quad.GetR() << ", ";
	ostr << "s = " << quad.GetS();
	return ostr;
}


template<class T = double>
class QuadSphere : public Quadric<T>
{
protected:

public:
	QuadSphere() : Quadric<T>()
	{
		this->m_s = T(-1);
	}

	QuadSphere(std::size_t iDim) : Quadric<T>(iDim)
	{
		this->m_s = T(-1);
	}

	QuadSphere(T r) : Quadric<T>(3)
	{
		this->m_Q(0,0) =
			this->m_Q(1,1) =
			this->m_Q(2,2) = T(1.)/(r*r);

		this->m_s = T(-1.);
	}

	QuadSphere(std::size_t iDim, T r) : Quadric<T>(iDim)
	{
		for(std::size_t i=0; i<iDim; ++i)
			this->m_Q(i,i) = T(1.)/(r*r);

		this->m_s = T(-1.);
	}

	/**
	 * only valid in principal axis system
	 */
	T GetRadius() const
	{
		return std::abs(this->m_s) /
			std::sqrt(std::abs(this->m_Q(0,0)));
	}

	T GetVolume() const
	{
		return get_ellipsoid_volume(this->m_Q) /
			std::abs(this->m_s);
	}

	~QuadSphere() {}
};


template<class T = double>
class QuadEllipsoid : public Quadric<T>
{
protected:

public:
	QuadEllipsoid() : Quadric<T>()
	{
		this->m_s = T(-1);
	}

	QuadEllipsoid(std::size_t iDim) : Quadric<T>(iDim)
	{
		this->m_s = T(-1);
	}

	QuadEllipsoid(T a, T b) : Quadric<T>(2)
	{
		this->m_Q(0,0) = T(1)/(a*a);
		this->m_Q(1,1) = T(1)/(b*b);

		this->m_s = T(-1);
	}

	QuadEllipsoid(T a, T b, T c) : Quadric<T>(3)
	{
		this->m_Q(0,0) = T(1)/(a*a);
		this->m_Q(1,1) = T(1)/(b*b);
		this->m_Q(2,2) = T(1)/(c*c);

		this->m_s = T(-1);
	}

	QuadEllipsoid(T a, T b, T c, T d) : Quadric<T>(4)
	{
		this->m_Q(0,0) = T(1)/(a*a);
		this->m_Q(1,1) = T(1)/(b*b);
		this->m_Q(2,2) = T(1)/(c*c);
		this->m_Q(3,3) = T(1)/(d*d);

		this->m_s = T(-1);
	}

	~QuadEllipsoid() {}

	/**
	 * only valid in principal axis system
	 */
	T GetRadius(std::size_t i) const
	{
		return std::abs(this->m_s) /
			std::sqrt(std::abs(this->m_Q(i,i)));
	}

	T GetVolume() const
	{
		return get_ellipsoid_volume(this->m_Q) /
			std::abs(this->m_s);
	}
};


//------------------------------------------------------------------------------


template<typename T = double>
std::vector<std::size_t> find_zeroes(std::size_t N, const T* pIn)
{
	using t_vec = ublas::vector<T>;
	std::vector<std::size_t> vecIndices;

	for(std::size_t i=0; i<N-1; ++i)
	{
		t_vec zero(2);
		zero[0] = zero[1] = 0.;
		t_vec xdir(2);
		xdir[0] = 1.; xdir[1] = 0.;
		Line<T> xaxis(zero, xdir);

		t_vec pos0(2);
		pos0[0] = 0.; pos0[1] = pIn[i];
		t_vec pos1(2);
		pos1[0] = 1.; pos1[1] = pIn[i+1];
		Line<T> line(pos0, pos1-pos0);

		T param;
		if(!line.intersect(xaxis, param))
			continue;

		t_vec posInters = line(param);
		if(posInters[0]>=0. && posInters[0]<=1.)
			vecIndices.push_back(i);
	}

	return vecIndices;
}



template<class t_vec = ublas::vector<double>>
class GeometricPrimitive
{
public:
	using t_vertices = std::vector<t_vec>;
	using t_polyindices = std::vector<std::vector<std::size_t>>;
	using t_polyindex = typename t_polyindices::value_type;

protected:
	virtual t_vertices& GetVertices() = 0;
	virtual t_polyindices& GetPolyIndices() = 0;

public:
	virtual std::size_t GetVertexCount() const = 0;
	virtual std::size_t GetPolyCount() const = 0;

	virtual const t_vec& GetVertex(std::size_t iVert) const = 0;
	virtual const t_polyindex& GetPolyIndex(std::size_t iPoly) const = 0;

	/**
	 * polygons comprising the solid
	 */
	virtual t_vertices GetPoly(std::size_t iPoly) const
	{
		std::vector<t_vec> vecPoly;

		for(std::size_t iIdx : GetPolyIndex(iPoly))
			vecPoly.push_back(GetVertex(iIdx));

		return vecPoly;
	}

	/**
	 * normal vectors to polygon faces
	 */
	virtual t_vec GetPolyNormal(std::size_t iPoly) const
	{
		t_vec vecNorm;

		t_polyindex vecPolyIdx = GetPolyIndex(iPoly);
		if(vecPolyIdx.size() < 3)	// too few vertices
			return vecNorm;

		t_vec vec1 = GetVertex(vecPolyIdx[1]) - GetVertex(vecPolyIdx[0]);
		t_vec vec2 = GetVertex(vecPolyIdx[2]) - GetVertex(vecPolyIdx[1]);
		vecNorm = cross_3(vec1, vec2);
		vecNorm /= veclen(vecNorm);

		return vecNorm;
	}


	/**
	 * tesselate using a central position
	 */
	virtual void SubdividePolysInMiddle()
	{
		t_vertices& vecVertices = GetVertices();
		t_polyindices& vecvecPolyIndices = GetPolyIndices();
		t_polyindices vecvecNewPolyIndices;

		// iterate over all polys
		for(const auto& vecPolyIndices : vecvecPolyIndices)
		{
			std::size_t iNumVerts = vecPolyIndices.size();
			std::vector<t_vec> vecVerts;

			for(std::size_t iVert=0; iVert<iNumVerts; ++iVert)
				vecVerts.push_back(GetVertex(vecPolyIndices[iVert]));

			// add a new vertex in the middle of the polygon
			t_vec vecVertMid = mean_value(vecVerts);
			vecVertices.push_back(vecVertMid);
			std::size_t iIdxVertMid = vecVertices.size()-1;

			// new vertex indices
			for(std::size_t iVert=0; iVert<iNumVerts; ++iVert)
			{
				std::size_t iVertIdx = vecPolyIndices[iVert];
				std::size_t iNextVertIdx = vecPolyIndices[(iVert+1) % iNumVerts];

				std::vector<std::size_t> vecNewVerts = { iVertIdx, iNextVertIdx, iIdxVertMid };
				vecvecNewPolyIndices.emplace_back(std::move(vecNewVerts));
			}
		}

		// replace poly indices
		vecvecPolyIndices = std::move(vecvecNewPolyIndices);
	}


	virtual void SubdividePolysInMiddle(std::size_t iIters)
	{
		for(std::size_t iIter=0; iIter<iIters; ++iIter)
			SubdividePolysInMiddle();
	}


	/**
	 * tesselate along polygon edges
	 */
	virtual void SubdividePolysAlongEdges()
	{
		t_vertices& vecVertices = GetVertices();
		t_polyindices& vecvecPolyIndices = GetPolyIndices();
		t_polyindices vecvecNewPolyIndices;

		// iterate over all polys
		for(const auto& vecPolyIndices : vecvecPolyIndices)
		{
			std::size_t iNumVerts = vecPolyIndices.size();
			if(iNumVerts != 3)	// only for triangles!
				break;

			t_vec vecMidEdge1 =
				(GetVertex(vecPolyIndices[0]) + GetVertex(vecPolyIndices[1])) / 2.;
			t_vec vecMidEdge2 =
				(GetVertex(vecPolyIndices[1]) + GetVertex(vecPolyIndices[2])) / 2.;
			t_vec vecMidEdge3 =
				(GetVertex(vecPolyIndices[2]) + GetVertex(vecPolyIndices[0])) / 2.;

			// add a new vertices along the edges
			vecVertices.push_back(vecMidEdge1);
			std::size_t iIdxMidEdge1 = vecVertices.size()-1;
			vecVertices.push_back(vecMidEdge2);
			std::size_t iIdxMidEdge2 = vecVertices.size()-1;
			vecVertices.push_back(vecMidEdge3);
			std::size_t iIdxMidEdge3 = vecVertices.size()-1;

			// new polygon indices
			vecvecNewPolyIndices.emplace_back(std::vector<std::size_t>
				({ vecPolyIndices[0], iIdxMidEdge1, iIdxMidEdge3 }));
			vecvecNewPolyIndices.emplace_back(std::vector<std::size_t>
				({ iIdxMidEdge1, vecPolyIndices[1], iIdxMidEdge2 }));
			vecvecNewPolyIndices.emplace_back(std::vector<std::size_t>
				({ iIdxMidEdge3, iIdxMidEdge2, vecPolyIndices[2] }));
			vecvecNewPolyIndices.emplace_back(std::vector<std::size_t>
				({ iIdxMidEdge1, iIdxMidEdge2, iIdxMidEdge3 }));
		}

		// replace poly indices
		vecvecPolyIndices = std::move(vecvecNewPolyIndices);
	}


	virtual void SubdividePolysAlongEdges(std::size_t iIters)
	{
		for(std::size_t iIter=0; iIter<iIters; ++iIter)
			SubdividePolysAlongEdges();
	}

};


// ----------------------------------------------------------------------------


/**
 * Tetrahedron
 * see e.g.: https://en.wikipedia.org/wiki/Platonic_solid
 */
template<class t_vec = ublas::vector<double>>
class Tetrahedron : public GeometricPrimitive<t_vec>
{
public:
	using t_vertices = typename GeometricPrimitive<t_vec>::t_vertices;
	using t_polyindices = typename GeometricPrimitive<t_vec>::t_polyindices;
	using t_polyindex = typename GeometricPrimitive<t_vec>::t_polyindex;

protected:
	// vertices
	t_vertices m_vecVertices =
	{
		make_vec<t_vec>({  1,  1,  1 }),	// 0
		make_vec<t_vec>({  1, -1, -1 }),	// 1
		make_vec<t_vec>({ -1, -1,  1 }),	// 2
		make_vec<t_vec>({ -1,  1, -1 }),	// 3
	};

	// polygons
	t_polyindices m_vecPolyIndices =
	{
		{ 0, 2, 1 }, { 0, 1, 3 },
		{ 0, 3, 2 }, { 1, 2, 3 },
	};

protected:
	virtual t_vertices& GetVertices() override { return m_vecVertices; }
	virtual t_polyindices& GetPolyIndices() override { return m_vecPolyIndices; }

public:
	Tetrahedron() = default;
	~Tetrahedron() = default;

	virtual std::size_t GetVertexCount() const override
	{ return m_vecVertices.size(); }

	virtual std::size_t GetPolyCount() const override
	{ return m_vecPolyIndices.size(); }

	virtual const t_vec& GetVertex(std::size_t iVert) const override
	{ return m_vecVertices[iVert]; }

	virtual const t_polyindex& GetPolyIndex(std::size_t iPoly) const override
	{ return m_vecPolyIndices[iPoly]; }
};


// ----------------------------------------------------------------------------


/**
 * Cube
 * see e.g.: https://en.wikipedia.org/wiki/Platonic_solid
 */
template<class t_vec = ublas::vector<double>>
class Cube : public GeometricPrimitive<t_vec>
{
public:
	using t_vertices = typename GeometricPrimitive<t_vec>::t_vertices;
	using t_polyindices = typename GeometricPrimitive<t_vec>::t_polyindices;
	using t_polyindex = typename GeometricPrimitive<t_vec>::t_polyindex;

protected:
	// vertices
	t_vertices m_vecVertices =
	{
		make_vec<t_vec>({  1,  1,  1 }),	// 0
		make_vec<t_vec>({  1,  1, -1 }),	// 1
		make_vec<t_vec>({  1, -1, -1 }),	// 2
		make_vec<t_vec>({  1, -1,  1 }),	// 3
		make_vec<t_vec>({ -1,  1,  1 }),	// 4
		make_vec<t_vec>({ -1,  1, -1 }),	// 5
		make_vec<t_vec>({ -1, -1, -1 }),	// 6
		make_vec<t_vec>({ -1, -1,  1 }),	// 7
	};

	// polygons
	t_polyindices m_vecPolyIndices =
	{
		{ 4, 5, 6, 7 } /*-x*/, { 7, 6, 2, 3 } /*-y*/, { 1, 2, 6, 5 } /*-z*/,
		{ 3, 2, 1, 0 } /*+x*/, { 5, 4, 0, 1 } /*+y*/, { 4, 7, 3, 0 } /*+z*/,
	};

protected:
	virtual t_vertices& GetVertices() override { return m_vecVertices; }
	virtual t_polyindices& GetPolyIndices() override { return m_vecPolyIndices; }

public:
	Cube() = default;
	~Cube() = default;

	virtual std::size_t GetVertexCount() const override
	{ return m_vecVertices.size(); }

	virtual std::size_t GetPolyCount() const override
	{ return m_vecPolyIndices.size(); }

	virtual const t_vec& GetVertex(std::size_t iVert) const override
	{ return m_vecVertices[iVert]; }

	virtual const t_polyindex& GetPolyIndex(std::size_t iPoly) const override
	{ return m_vecPolyIndices[iPoly]; }
};


// ----------------------------------------------------------------------------


/**
 * Octahedron
 * see e.g.: https://en.wikipedia.org/wiki/Platonic_solid
 */
template<class t_vec = ublas::vector<double>>
class Octahedron : public GeometricPrimitive<t_vec>
{
public:
	using t_vertices = typename GeometricPrimitive<t_vec>::t_vertices;
	using t_polyindices = typename GeometricPrimitive<t_vec>::t_polyindices;
	using t_polyindex = typename GeometricPrimitive<t_vec>::t_polyindex;

protected:
	// vertices
	t_vertices m_vecVertices =
	{
		make_vec<t_vec>({  1,  0,  0 }),	// 0
		make_vec<t_vec>({ -1,  0,  0 }),	// 1
		make_vec<t_vec>({  0,  1,  0 }),	// 2
		make_vec<t_vec>({  0, -1,  0 }),	// 3
		make_vec<t_vec>({  0,  0,  1 }),	// 4
		make_vec<t_vec>({  0,  0, -1 }),	// 5
	};

	// polygons
	t_polyindices m_vecPolyIndices =
	{
		{ 0, 2, 4 }, { 0, 5, 2 }, { 0, 4, 3 }, { 0, 3, 5 },
		{ 1, 4, 2 }, { 1, 2, 5 }, { 1, 3, 4 }, { 1, 5, 3 },
	};

protected:
	virtual t_vertices& GetVertices() override { return m_vecVertices; }
	virtual t_polyindices& GetPolyIndices() override { return m_vecPolyIndices; }

public:
	Octahedron() = default;
	~Octahedron() = default;

	virtual std::size_t GetVertexCount() const override
	{ return m_vecVertices.size(); }

	virtual std::size_t GetPolyCount() const override
	{ return m_vecPolyIndices.size(); }

	virtual const t_vec& GetVertex(std::size_t iVert) const override
	{ return m_vecVertices[iVert]; }

	virtual const t_polyindex& GetPolyIndex(std::size_t iPoly) const override
	{ return m_vecPolyIndices[iPoly]; }
};


// ----------------------------------------------------------------------------


/**
 * Icosahedron
 * see e.g.: https://en.wikipedia.org/wiki/Platonic_solid
 */
template<class t_vec = ublas::vector<double>>
class Icosahedron : public GeometricPrimitive<t_vec>
{
public:
	using t_vertices = typename GeometricPrimitive<t_vec>::t_vertices;
	using t_polyindices = typename GeometricPrimitive<t_vec>::t_polyindices;
	using t_polyindex = typename GeometricPrimitive<t_vec>::t_polyindex;

protected:
	// Golden Ratio
	/*static constexpr*/ const typename t_vec::value_type s_g = 0.5 + 0.5*std::sqrt(5.);

	// vertices
	t_vertices m_vecVertices =
	{
		make_vec<t_vec>({  0,  1,  s_g }),	// 0
		make_vec<t_vec>({  0,  1, -s_g }),	// 1
		make_vec<t_vec>({  0, -1,  s_g }),	// 2
		make_vec<t_vec>({  0, -1, -s_g }),	// 3

		make_vec<t_vec>({  1,  s_g,  0 }),	// 4
		make_vec<t_vec>({  1, -s_g,  0 }),	// 5
		make_vec<t_vec>({ -1,  s_g,  0 }),	// 6
		make_vec<t_vec>({ -1, -s_g,  0 }),	// 7

		make_vec<t_vec>({  s_g,  0,  1 }),	// 8
		make_vec<t_vec>({  s_g,  0, -1 }),	// 9
		make_vec<t_vec>({ -s_g,  0,  1 }),	// 10
		make_vec<t_vec>({ -s_g,  0, -1 }),	// 11
	};

	// polygons
	t_polyindices m_vecPolyIndices =
	{
		{ 0, 10, 2 }, { 0, 2, 8 }, { 0, 8, 4 }, { 0,  4, 6 }, { 0,  6, 10 },	// upper cap
		{ 3,  5, 7 }, { 3, 9, 5 }, { 3, 1, 9 }, { 3, 11, 1 }, { 3,  7, 11 },	// lower cap
		{ 10, 7, 2 }, { 2, 5, 8 }, { 8, 9, 4 }, { 4,  1, 6 }, { 6, 11, 10 },	// sides
		{  7, 5, 2 }, { 5, 9, 8 }, { 9, 1, 4 }, { 1, 11, 6 }, { 11, 7, 10 },	// sides
	};

protected:
	virtual t_vertices& GetVertices() override { return m_vecVertices; }
	virtual t_polyindices& GetPolyIndices() override { return m_vecPolyIndices; }

public:
	Icosahedron() = default;
	~Icosahedron() = default;

	virtual std::size_t GetVertexCount() const override
	{ return m_vecVertices.size(); }

	virtual std::size_t GetPolyCount() const override
	{ return m_vecPolyIndices.size(); }

	virtual const t_vec& GetVertex(std::size_t iVert) const override
	{ return m_vecVertices[iVert]; }

	virtual const t_polyindex& GetPolyIndex(std::size_t iPoly) const override
	{ return m_vecPolyIndices[iPoly]; }
};


// ----------------------------------------------------------------------------


/**
 * Dodecahedron
 * see e.g.: https://en.wikipedia.org/wiki/Platonic_solid
 */
template<class t_vec = ublas::vector<double>>
class Dodecahedron : public GeometricPrimitive<t_vec>
{
public:
	using t_vertices = typename GeometricPrimitive<t_vec>::t_vertices;
	using t_polyindices = typename GeometricPrimitive<t_vec>::t_polyindices;
	using t_polyindex = typename GeometricPrimitive<t_vec>::t_polyindex;

protected:
	// Golden Ratio
	/*static constexpr*/ const typename t_vec::value_type s_g = 0.5 + 0.5*std::sqrt(5.);

	// vertices
	t_vertices m_vecVertices =
	{
		make_vec<t_vec>({  0,  1./s_g,  s_g }),	// 0
		make_vec<t_vec>({  0,  1./s_g, -s_g }),	// 1
		make_vec<t_vec>({  0, -1./s_g,  s_g }),	// 2
		make_vec<t_vec>({  0, -1./s_g, -s_g }),	// 3

		make_vec<t_vec>({  1./s_g,  s_g,  0 }),	// 4
		make_vec<t_vec>({  1./s_g, -s_g,  0 }),	// 5
		make_vec<t_vec>({ -1./s_g,  s_g,  0 }),	// 6
		make_vec<t_vec>({ -1./s_g, -s_g,  0 }),	// 7

		make_vec<t_vec>({  s_g,  0,  1./s_g }),	// 8
		make_vec<t_vec>({  s_g,  0, -1./s_g }),	// 9
		make_vec<t_vec>({ -s_g,  0,  1./s_g }),	// 10
		make_vec<t_vec>({ -s_g,  0, -1./s_g }),	// 11

		make_vec<t_vec>({  1,  1,  1 }),	// 12
		make_vec<t_vec>({  1,  1, -1 }),	// 13
		make_vec<t_vec>({  1, -1, -1 }),	// 14
		make_vec<t_vec>({  1, -1,  1 }),	// 15

		make_vec<t_vec>({ -1,  1,  1 }),	// 16
		make_vec<t_vec>({ -1,  1, -1 }),	// 17
		make_vec<t_vec>({ -1, -1, -1 }),	// 18
		make_vec<t_vec>({ -1, -1,  1 }),	// 19
	};

	// polygons
	t_polyindices m_vecPolyIndices =
	{
		{ 16, 10, 19,  2, 0 }, { 19, 7,  5, 15,  2 }, { 15, 8, 12, 0, 2 }, { 12, 4, 6, 16, 0 },	// top cap
		{ 18, 11, 17,  1, 3 }, {  6, 4, 13,  1, 17 }, { 13, 9, 14, 3, 1 }, { 14, 5, 7, 18, 3 },	// bottom cap
		{ 19, 10, 11, 18, 7 }, { 16, 6, 17, 11, 10 }, { 15, 5, 14, 9, 8 }, { 12, 8, 9, 13, 4 },	// sides
	};

protected:
	virtual t_vertices& GetVertices() override { return m_vecVertices; }
	virtual t_polyindices& GetPolyIndices() override { return m_vecPolyIndices; }

public:
	Dodecahedron() = default;
	~Dodecahedron() = default;

	virtual std::size_t GetVertexCount() const override
	{ return m_vecVertices.size(); }

	virtual std::size_t GetPolyCount() const override
	{ return m_vecPolyIndices.size(); }

	virtual const t_vec& GetVertex(std::size_t iVert) const override
	{ return m_vecVertices[iVert]; }

	virtual const t_polyindex& GetPolyIndex(std::size_t iPoly) const override
	{ return m_vecPolyIndices[iPoly]; }
};


// ----------------------------------------------------------------------------


/**
 * tessellated sphere
 */
template<class t_vec = ublas::vector<double>,
	template<class...> class t_underlying_solid = Icosahedron>
class TesselSphere : public t_underlying_solid<t_vec>
{
public:
	TesselSphere(typename t_vec::value_type dRad = 1., std::size_t iSubdivisions=2)
	{
		t_underlying_solid<t_vec>::SubdividePolysAlongEdges(iSubdivisions);

		for(t_vec& vecVertex : t_underlying_solid<t_vec>::m_vecVertices)
		{
			vecVertex /= veclen(vecVertex);
			vecVertex *= dRad;
		}
	}

	~TesselSphere() = default;
};



// ------------------------------------------------------------------------------------------------



#ifdef USE_LAPACK

// selects the float or double version of a lapack function
template<class T1, class T2, class F1, class F2>
struct select_func
{
	F1* m_f1 = nullptr;
	F2* m_f2 = nullptr;

	select_func(F1* f1, F2* f2) : m_f1(f1), m_f2(f2) {}

	template<class T>
	typename std::enable_if<std::is_same<T, T1>::value, F1*>::type
		get_func() { return m_f1; }
	template<class T>
	typename std::enable_if<std::is_same<T, T2>::value, F2*>::type
		get_func() { return m_f2; }
};


// ----------------------------------------------------------------------------


/**
 * qr decomposition: M = QR
 */
template<class T>
bool qr(const ublas::matrix<T>& M,
	ublas::matrix<T>& Q, ublas::matrix<T>& R)
{
	select_func<float, double, decltype(LAPACKE_sgeqrf), decltype(LAPACKE_dgeqrf)>
		sfunc(LAPACKE_sgeqrf, LAPACKE_dgeqrf);
	auto pfunc = sfunc.get_func<T>();

	const typename ublas::matrix<T>::size_type m = M.size1();
	const typename ublas::matrix<T>::size_type n = M.size2();

	const std::size_t iTauSize = m;//std::min<std::size_t>(m,n);

	std::unique_ptr<T, std::default_delete<T[]>>
		uptrMem(new T[n*m + iTauSize]);
	T *pMem = uptrMem.get();

	T *pMat = pMem;
	T *pTau = pMem + n*m;

	for(std::size_t i=0; i<m; ++i)
		for(std::size_t j=0; j<n; ++j)
			pMat[i*n + j] = M(i,j);

	// see: http://www.math.utah.edu/software/lapack/lapack-d/dgeqrf.html
	int iInfo = (*pfunc)(LAPACK_ROW_MAJOR, m, n, pMat, n, pTau);
	//std::cout << "dgeqrt: " << iInfo << std::endl;

	R = ublas::matrix<T>(m,n);
	for(std::size_t i=0; i<m; ++i)
		for(std::size_t j=0; j<n; ++j)
		{
			if(j>=i)
				R(i,j) = pMat[i*n + j];
			else
				R(i,j) = 0.;
		}
	//std::cout << "R = " << R << std::endl;

	ublas::vector<T> v(iTauSize);

	const ublas::matrix<T> ident = unit_m<ublas::matrix<T>>(iTauSize);
	Q = ident;

	for(std::size_t k=1; k<=iTauSize; ++k)
	{
		T dTau = pTau[k-1];
		//std::cout << "tau " << k << " = " << dTau << std::endl;

		for(std::size_t i=1; i<=k-1; ++i)
			v[i-1] = 0.;
		v[k-1] = 1.;

		for(std::size_t i=k+1; i<=iTauSize; ++i)
			v[i-1] = pMat[(i-1)*n + (k-1)];

		ublas::matrix<T> VV = outer(v, transpose(v));
		ublas::matrix<T> H = ident - dTau*VV;

		Q = prod_mm(Q, H);
	}

	//std::cout << "Q = " << Q << std::endl;
	return (iInfo==0);
}


/**
 * solve M^T M x = M^T v for x
 */
template<typename T = double>
bool solve_linear_approx(const ublas::matrix<T>& M, const ublas::vector<T>& v, ublas::vector<T>& x)
{
	if(M.size1() <= M.size2())
	{
		//std::cerr << "Error: Matrix has to be overdetermined." << std::endl;
		return false;
	}

	ublas::matrix<T> Q, R;
	if(!qr(M, Q, R))
		return false;

	// M^T M x = M^T v
	// R^T Q^T Q R x = R^T Q^T v
	// R^T R x = R^T Q^T v

	const ublas::matrix<T> RT = transpose(R);
	const ublas::matrix<T> QT = transpose(Q);
	const ublas::matrix<T> RTR = prod_mm(RT, R);
	const ublas::matrix<T> RTQT = prod_mm(RT, QT);

	const ublas::vector<T> vnew = prod_mv(RTQT, v);
	return solve_linear<T>(RTR, vnew, x);
}


/**
 * solve Mx = v for x
 */
template<typename T /*= double*/>
bool solve_linear(const ublas::matrix<T>& M, const ublas::vector<T>& v, ublas::vector<T>& x)
{
	if(M.size1() == M.size2())		// determined, TODO: check rank
	{
		try
		{
			const std::size_t N = M.size1();

			ublas::matrix<T> lu = M;
			ublas::permutation_matrix<typename ublas::matrix<T>::size_type> perm(N);

			typename ublas::matrix<T>::size_type sing = ublas::lu_factorize(lu, perm);
			if(sing != 0)
				return false;

			x = v;
			ublas::lu_substitute(lu, perm, x);
		}
		catch(const std::exception& ex)
		{
			log_err("Linear equation solver failed with exception: ", ex.what(), ".");
			return false;
		}
	}
	else if(M.size1() < M.size2())	// underdetermined
	{
		ublas::matrix<T> Q, R;
		if(!qr(M, Q, R))
			return false;
		typedef typename ublas::vector<T>::size_type t_int;

		// M x = v
		// QR x = v
		// R x = Q^T v

		ublas::vector<T> vnew = prod_mv(transpose(Q), v);

		/*std::cout << "M = " << M << std::endl;
		std::cout << "Q = " << Q << std::endl;
		std::cout << "R = " << R << std::endl;
		std::cout << "v' = " << vnew << std::endl;*/

		x = zero_v<ublas::vector<T>>(M.size2());
		ublas::vector<T> xnew(R.size1());
		bool bOk = 0;

		/*
		// pick one of the solutions
		// TODO: resort columns so that Rupper doesn't get singular
		ublas::matrix<T> Rupper = ublas::subrange(R, 0, R.size1(),
						R.size2()-R.size1(), R.size2());
		//std::cout << "Rupper = " << Rupper << std::endl;

		bOk = solve_linear(Rupper, vnew, xnew);

		for(t_int i=0; i<xnew.size(); ++i)
			x[x.size()-xnew.size()+i] = xnew[i];
		*/

		// find non-singular right-upper submatrix
		std::vector<t_int> vecDelCols;
		std::size_t iNumToDel = R.size2()-R.size1();
		if(iNumToDel != 1)
		{
			log_err(__func__, " not yet implemented.");
			return false;
		}

		bool bFoundNonSingular = 0;
		ublas::matrix<T> Rsub;
		for(std::ptrdiff_t iCol=std::ptrdiff_t(R.size2()-1); iCol>=0; --iCol)
		{
			Rsub = remove_column(R, (std::size_t)iCol);
			//std::cout << "Rsub" << Rsub << std::endl;
			//std::cout << "det: " << determinant(Rsub) << std::endl;

			T det = determinant<ublas::matrix<T>>(Rsub);
			if(!float_equal<T>(det, 0.))
			{
				bFoundNonSingular = 1;
				vecDelCols.push_back(iCol);
				break;
			}
		}

		if(!bFoundNonSingular)
		{
			log_err("No non-singluar submatrix found in linear equation solver.");
			return false;
		}

		bOk = solve_linear(Rsub, vnew, xnew);
		//std::cout << "Rsub = " << Rsub << std::endl;
		//std::cout << "v' = " << vnew << std::endl;
		//std::cout << "x' = " << xnew << std::endl;

		for(t_int i=0, i0=0; i<xnew.size() && i0<x.size(); ++i, ++i0)
		{
			while(std::find(vecDelCols.begin(), vecDelCols.end(), i0) != vecDelCols.end())
				++i0;
			x[i0] = xnew[i];
		}

		return bOk;
	}
	else if(M.size1() > M.size2())	// overdetermined
		return solve_linear_approx<T>(M,v,x);
	else
		return false;

	return true;
}


// ----------------------------------------------------------------------------


/**
 * calculates the eigenvectors of a general matrix
 */
template<class T>
bool eigenvec(const ublas::matrix<T>& mat,
	std::vector<ublas::vector<T>>& evecs_real,
	std::vector<ublas::vector<T>>& evecs_imag,
	std::vector<T>& evals_real,
	std::vector<T>& evals_imag,
	bool bNorm = false)
{
	bool bOk = true;
	select_func<float, double, decltype(LAPACKE_sgeev), decltype(LAPACKE_dgeev)>
		sfunc(LAPACKE_sgeev, LAPACKE_dgeev);
	auto pfunc = sfunc.get_func<T>();

	if(mat.size1() != mat.size2())
		return false;
	if(mat.size1()==0 || mat.size1()==1)
		return false;

	const std::size_t iOrder = mat.size1();
	evecs_real.resize(iOrder); evecs_imag.resize(iOrder);
	evals_real.resize(iOrder); evals_imag.resize(iOrder);
	for(std::size_t i=0; i<iOrder; ++i)
	{
		evecs_real[i].resize(iOrder);
		evecs_imag[i].resize(iOrder);
	}

	std::unique_ptr<T, std::default_delete<T[]>>
		uptrMem(new T[iOrder*iOrder + iOrder*iOrder + iOrder*iOrder]);
	T *pMatrix = uptrMem.get();
	T *pEVs = pMatrix + iOrder*iOrder;

	for(std::size_t i=0; i<iOrder; ++i)
		for(std::size_t j=0; j<iOrder; ++j)
			pMatrix[i*iOrder + j] = mat(i,j);

	int iInfo = (*pfunc)(LAPACK_ROW_MAJOR, 'N', 'V', iOrder,
		pMatrix, iOrder, evals_real.data(), evals_imag.data(),
		nullptr, iOrder, pEVs, iOrder);

	if(iInfo!=0)
	{
		log_err("Could not solve general real eigenproblem",
			" (lapack error ", iInfo , ").");
		bOk = false;
	}

	for(std::size_t i=0; i<iOrder; ++i)
	{
		bool bIsReal = 0;
		if(float_equal<T>(evals_imag[i], 0.))
			bIsReal = 1;

		if(bIsReal)
		{
			for(std::size_t j=0; j<iOrder; ++j)
			{
				evecs_real[i][j] = pEVs[j*iOrder + i];
				evecs_imag[i][j] = 0.;
			}
		}
		else
		{
			for(std::size_t j=0; j<iOrder; ++j)
			{
				evecs_real[i][j] = pEVs[j*iOrder + i];
				evecs_imag[i][j] = pEVs[j*iOrder + i+1];

				evecs_real[i+1][j] = pEVs[j*iOrder + i];
				evecs_imag[i+1][j] = -pEVs[j*iOrder + i+1];
			}
			++i; // check: (next eigenval) == -(currrent eigenval)
		}
	}


	// normalise
	if(bNorm && bOk)
	{
		for(std::size_t i=0; i<evecs_real.size(); ++i)
		{
			T len = T(0);
			for(std::size_t j=0; j<evecs_real[i].size(); ++j)
				len += evecs_real[i][j]*evecs_real[i][j] + evecs_imag[i][j]*evecs_imag[i][j];
			len = std::sqrt(len);

			evecs_real[i] /= len;
			evecs_imag[i] /= len;
		}
	}

	return bOk;
}



/**
 * calculates only the eigenvalues of a general matrix
 */
template<class T>
bool eigenval(const ublas::matrix<T>& mat, std::vector<T>& evals_real, std::vector<T>& evals_imag)
{
	bool bOk = true;
	select_func<float, double, decltype(LAPACKE_sgeev), decltype(LAPACKE_dgeev)>
		sfunc(LAPACKE_sgeev, LAPACKE_dgeev);
	auto pfunc = sfunc.get_func<T>();

	if(mat.size1() != mat.size2())
		return false;
	if(mat.size1()==0 || mat.size1()==1)
		return false;

	const std::size_t iOrder = mat.size1();
	evals_real.resize(iOrder); evals_imag.resize(iOrder);

	std::unique_ptr<T, std::default_delete<T[]>> uptrMem(new T[iOrder*iOrder]);
	T *pMatrix = uptrMem.get();

	for(std::size_t i=0; i<iOrder; ++i)
		for(std::size_t j=0; j<iOrder; ++j)
			pMatrix[i*iOrder + j] = mat(i,j);

	int iInfo = (*pfunc)(LAPACK_ROW_MAJOR, 'N', 'N', iOrder,
		pMatrix, iOrder, evals_real.data(), evals_imag.data(),
		nullptr, iOrder, nullptr, iOrder);

	if(iInfo!=0)
	{
		log_err("Could not solve general real eigenproblem",
			" (lapack error ", iInfo , ").");
		bOk = false;
	}

	return bOk;
}



/**
 * calculates the eigenvectors of a general complex matrix
 */
template<class T>
bool eigenvec_cplx(const ublas::matrix<std::complex<T>>& mat,
	std::vector<ublas::vector<std::complex<T>>>& evecs,
	std::vector<std::complex<T>>& evals,
	bool bNorm = false)
{
	using t_cplx = std::complex<T>;
	bool bOk = true;
	select_func<float, double, decltype(LAPACKE_cgeev), decltype(LAPACKE_zgeev)>
		sfunc(LAPACKE_cgeev, LAPACKE_zgeev);
	auto pfunc = sfunc.get_func<T>();

	if(mat.size1() != mat.size2())
		return false;
	if(mat.size1()==0 || mat.size1()==1)
		return false;

	const std::size_t iOrder = mat.size1();
	evecs.resize(iOrder);
	evals.resize(iOrder);
	for(std::size_t i=0; i<iOrder; ++i)
		evecs[i].resize(iOrder);

	std::unique_ptr<t_cplx, std::default_delete<t_cplx[]>>
		uptrMem(new t_cplx[iOrder*iOrder + iOrder*iOrder + iOrder]);
	t_cplx *pMatrix = uptrMem.get();

	for(std::size_t i=0; i<iOrder; ++i)
		for(std::size_t j=0; j<iOrder; ++j)
			pMatrix[i*iOrder + j] = mat(i,j);

	t_cplx *pEVs = pMatrix + iOrder*iOrder;
	t_cplx *pEVals = pEVs + iOrder*iOrder;

	int iInfo = (*pfunc)(LAPACK_ROW_MAJOR, 'N', 'V', iOrder,
		pMatrix, iOrder, pEVals,
		nullptr, iOrder, pEVs, iOrder);

	if(iInfo!=0)
	{
		log_err("Could not solve general complex eigenproblem",
			" (lapack error ", iInfo , ").");
		bOk = false;
	}

	for(std::size_t i=0; i<iOrder; ++i)
	{
		for(std::size_t j=0; j<iOrder; ++j)
			evecs[i][j] = pEVs[j*iOrder + i];
		evals[i] = pEVals[i];

		if(bNorm && bOk)
			evecs[i] /= veclen(evecs[i]);
	}

	return bOk;
}



/**
 * calculates only the eigenvalues of a general complex matrix
 */
template<class T>
bool eigenval_cplx(const ublas::matrix<std::complex<T>>& mat, std::vector<std::complex<T>>& evals)
{
	using t_cplx = std::complex<T>;
	bool bOk = true;
	select_func<float, double, decltype(LAPACKE_cgeev), decltype(LAPACKE_zgeev)>
		sfunc(LAPACKE_cgeev, LAPACKE_zgeev);
	auto pfunc = sfunc.get_func<T>();

	if(mat.size1() != mat.size2())
		return false;
	if(mat.size1()==0 || mat.size1()==1)
		return false;

	const std::size_t iOrder = mat.size1();
	evals.resize(iOrder);

	std::unique_ptr<t_cplx, std::default_delete<t_cplx[]>>
		uptrMem(new t_cplx[iOrder*iOrder + iOrder]);
	t_cplx *pMatrix = uptrMem.get();

	for(std::size_t i=0; i<iOrder; ++i)
		for(std::size_t j=0; j<iOrder; ++j)
			pMatrix[i*iOrder + j] = mat(i,j);

	t_cplx *pEVals = pMatrix + iOrder*iOrder;

	int iInfo = (*pfunc)(LAPACK_ROW_MAJOR, 'N', 'N', iOrder,
		pMatrix, iOrder, pEVals,
		nullptr, iOrder, nullptr, iOrder);

	if(iInfo!=0)
	{
		log_err("Could not solve general complex eigenproblem",
			" (lapack error ", iInfo , ").");
		bOk = false;
	}

	for(std::size_t i=0; i<iOrder; ++i)
		evals[i] = pEVals[i];

	return bOk;
}



// ----------------------------------------------------------------------------



/**
 * calculates the eigenvectors of a symmetric matrix
 */
template<class T>
bool eigenvec_sym(const ublas::matrix<T>& mat,
	std::vector<ublas::vector<T>>& evecs,
	std::vector<T>& evals,
	bool bNorm = false)
{
	bool bOk = true;
	select_func<float, double, decltype(LAPACKE_ssyev), decltype(LAPACKE_dsyev)>
		sfunc(LAPACKE_ssyev, LAPACKE_dsyev);
	auto pfunc = sfunc.get_func<T>();

	if(mat.size1() != mat.size2())
		return false;
	if(mat.size1()==0 || mat.size1()==1)
		return false;

	const std::size_t iOrder = mat.size1();
	evecs.resize(iOrder);
	evals.resize(iOrder);
	for(std::size_t i=0; i<iOrder; ++i)
		evecs[i].resize(iOrder);

	std::unique_ptr<T, std::default_delete<T[]>>
		uptrMat(new T[iOrder*iOrder]);
	T *pMatrix = uptrMat.get();

	for(std::size_t i=0; i<iOrder; ++i)
		for(std::size_t j=0; j<iOrder; ++j)
			pMatrix[i*iOrder + j] = (j>=i ? mat(i,j) : T(0));

	int iInfo = (*pfunc)(LAPACK_ROW_MAJOR, 'V', 'U',
		iOrder, pMatrix, iOrder, evals.data());

	if(iInfo!=0)
	{
		log_err("Could not solve symmetric eigenproblem",
			" (lapack error ", iInfo, ").");
		bOk = false;
	}

	for(std::size_t i=0; i<iOrder; ++i)
	{
		for(std::size_t j=0; j<iOrder; ++j)
			evecs[i][j] = pMatrix[j*iOrder + i];

		if(bNorm && bOk)
			evecs[i] /= veclen(evecs[i]);
	}

	//if(determinant<ublas::matrix<T>>(column_matrix(evecs)) < 0.)
	//	evecs[0] = -evecs[0];
	return bOk;
}



/**
 * calculates only the eigenvalues of a symmetric matrix
 */
template<class T>
bool eigenval_sym(const ublas::matrix<T>& mat, std::vector<T>& evals)
{
	bool bOk = true;
	select_func<float, double, decltype(LAPACKE_ssyev), decltype(LAPACKE_dsyev)>
		sfunc(LAPACKE_ssyev, LAPACKE_dsyev);
	auto pfunc = sfunc.get_func<T>();

	if(mat.size1() != mat.size2())
		return false;
	if(mat.size1()==0 || mat.size1()==1)
		return false;

	const std::size_t iOrder = mat.size1();
	evals.resize(iOrder);

	std::unique_ptr<T, std::default_delete<T[]>>
		uptrMat(new T[iOrder*iOrder]);
	T *pMatrix = uptrMat.get();

	for(std::size_t i=0; i<iOrder; ++i)
		for(std::size_t j=0; j<iOrder; ++j)
			pMatrix[i*iOrder + j] = mat(i,j);

	int iInfo = (*pfunc)(LAPACK_ROW_MAJOR, 'N', 'U',
		iOrder, pMatrix, iOrder, evals.data());

	if(iInfo!=0)
	{
		log_err("Could not solve symmetric eigenproblem",
			" (lapack error ", iInfo, ").");
		bOk = false;
	}

	return bOk;
}




/**
 * calculates the eigenvectors of a hermitian matrix
 */
template<class T>
bool eigenvec_herm(const ublas::matrix<std::complex<T>>& mat,
	std::vector<ublas::vector<std::complex<T>>>& evecs,
	std::vector<T>& evals,
	bool bNorm = false)
{
	using t_cplx = std::complex<T>;
	bool bOk = true;

	select_func<float, double, decltype(LAPACKE_cheev), decltype(LAPACKE_zheev)>
		sfunc(LAPACKE_cheev, LAPACKE_zheev);
	auto pfunc = sfunc.get_func<T>();

	if(mat.size1() != mat.size2())
		return false;
	if(mat.size1()==0 || mat.size1()==1)
		return false;

	const std::size_t iOrder = mat.size1();
	evecs.resize(iOrder);
	evals.resize(iOrder);
	for(std::size_t i=0; i<iOrder; ++i)
		evecs[i].resize(iOrder);

	std::unique_ptr<t_cplx, std::default_delete<t_cplx[]>>
		uptrMat(new t_cplx[iOrder*iOrder]);
	t_cplx *pMatrix = uptrMat.get();

	for(std::size_t i=0; i<iOrder; ++i)
		for(std::size_t j=0; j<iOrder; ++j)
			pMatrix[i*iOrder + j] = (j>=i ? mat(i,j) : T(0));

	int iInfo = (*pfunc)(LAPACK_ROW_MAJOR, 'V', 'U',
		iOrder, pMatrix, iOrder, evals.data());

	if(iInfo != 0)
	{
		log_err("Could not solve hermitian eigenproblem",
			" (lapack error ", iInfo, ").");
		bOk = false;
	}

	for(std::size_t i=0; i<iOrder; ++i)
	{
		for(std::size_t j=0; j<iOrder; ++j)
			evecs[i][j] = pMatrix[j*iOrder + i];
		if(bNorm)
			evecs[i] /= veclen(evecs[i]);
	}
	return bOk;
}



/**
 * calculates only the eigenvalues of a hermitian matrix
 */
template<class T>
bool eigenval_herm(const ublas::matrix<std::complex<T>>& mat, std::vector<T>& evals)
{
	using t_cplx = std::complex<T>;
	bool bOk = true;

	select_func<float, double, decltype(LAPACKE_cheev), decltype(LAPACKE_zheev)>
		sfunc(LAPACKE_cheev, LAPACKE_zheev);
	auto pfunc = sfunc.get_func<T>();

	if(mat.size1() != mat.size2())
		return false;
	if(mat.size1()==0 || mat.size1()==1)
		return false;

	const std::size_t iOrder = mat.size1();
	evals.resize(iOrder);

	std::unique_ptr<t_cplx, std::default_delete<t_cplx[]>>
		uptrMat(new t_cplx[iOrder*iOrder]);
	t_cplx *pMatrix = uptrMat.get();

	for(std::size_t i=0; i<iOrder; ++i)
		for(std::size_t j=0; j<iOrder; ++j)
			pMatrix[i*iOrder + j] = mat(i,j);

	int iInfo = (*pfunc)(LAPACK_ROW_MAJOR, 'N', 'U',
		iOrder, pMatrix, iOrder, evals.data());

	if(iInfo != 0)
	{
		log_err("Could not solve hermitian eigenproblem",
			" (lapack error ", iInfo, ").");
		bOk = false;
	}

	return bOk;
}



/**
 * calculates selected eigenvectors of a hermitian matrix
 */
template<class T>
bool eigenvecsel_herm(const ublas::matrix<std::complex<T>>& mat,
	std::vector<ublas::vector<std::complex<T>>>& evecs,
	std::vector<T>& evals,
	bool bNorm=0, T minval=-1, T maxval=-2, T eps=T(-1))
{
	// select needed functions
	select_func<float, double, decltype(LAPACKE_cheevr), decltype(LAPACKE_zheevr)>
	sfunc(LAPACKE_cheevr, LAPACKE_zheevr);
	auto pfunc = sfunc.get_func<T>();

	select_func<float, double, decltype(LAPACKE_slamch), decltype(LAPACKE_dlamch)>
	_lamch(LAPACKE_slamch, LAPACKE_dlamch);
	auto lamch = _lamch.get_func<T>();


	using t_cplx = std::complex<T>;
	bool bOk = true;

	if(mat.size1() != mat.size2())
		return false;
	if(mat.size1()==0 || mat.size1()==1)
		return false;

	const std::size_t iOrder = mat.size1();
	evecs.resize(iOrder);
	evals.resize(iOrder);
	for(std::size_t i=0; i<iOrder; ++i)
		evecs[i].resize(iOrder);

	std::unique_ptr<t_cplx, std::default_delete<t_cplx[]>>
		uptrMat(new t_cplx[iOrder*iOrder + iOrder*iOrder]);
	t_cplx *pMatrix = uptrMat.get();
	t_cplx *pEVsOrtho = pMatrix + iOrder*iOrder;

	std::unique_ptr<int, std::default_delete<int[]>>
		uptrIdxArr(new int[2*iOrder]);
	int *pIdxArr = uptrIdxArr.get();

	for(std::size_t i=0; i<iOrder; ++i)
		for(std::size_t j=0; j<iOrder; ++j)
			pMatrix[i*iOrder + j] = (j>=i ? mat(i,j) : T(0));

	// use maximum precision if none given
	if(eps < T(0))
		eps = lamch('S');
	//std::cout << "eps = " << eps << std::endl;

	// if an invalid range is given, select all eigenvalues
	bool bSelectAll = (minval > maxval);

	int minidx = 1, maxidx = iOrder;
	int iNumFound = 0;
	int iInfo = (*pfunc)(LAPACK_ROW_MAJOR, 'V', bSelectAll?'A':'V', 'U',
		iOrder, pMatrix, iOrder, minval, maxval, minidx, maxidx,
		eps, &iNumFound, evals.data(), pEVsOrtho, iOrder, pIdxArr);

	if(iInfo != 0)
	{
		log_err("Could not solve hermitian eigenproblem",
				" (lapack error ", iInfo, ").");
		bOk = false;
	}

	evecs.resize(iNumFound);
	evals.resize(iNumFound);
	for(std::size_t i=0; i<iNumFound; ++i)
	{
		for(std::size_t j=0; j<iOrder; ++j)
			evecs[i][j] = /*pMatrix*/pEVsOrtho[j*iOrder + i];
		if(bNorm)
			evecs[i] /= veclen(evecs[i]);
	}
	return bOk;
}



// ----------------------------------------------------------------------------



/**
 * calculates the singular values of a real matrix: M = U diag(vals) V^t
 */
template<typename T>
bool singvec(const ublas::matrix<T>& mat,
	ublas::matrix<T>& matU, ublas::matrix<T>& matV, std::vector<T>& vecsvals)
{
	select_func<float, double, decltype(LAPACKE_sgesvd), decltype(LAPACKE_dgesvd)>
		sfunc(LAPACKE_sgesvd, LAPACKE_dgesvd);
	auto pfunc = sfunc.get_func<T>();

	const std::size_t iM = mat.size1();
	const std::size_t iN = mat.size2();
	const std::size_t iMin = std::min(iM,iN);

	vecsvals.resize(iMin);
	matU.resize(iM, iM);
	matV.resize(iN, iN);

	std::unique_ptr<T, std::default_delete<T[]>> uptrMat(new T[iM*iN]);
	std::unique_ptr<T, std::default_delete<T[]>> uptrWork(new T[iM*iN]);	// TODO: find correct size
	std::unique_ptr<T, std::default_delete<T[]>> uptrU(new T[iM*iM]);
	std::unique_ptr<T, std::default_delete<T[]>> uptrVt(new T[iN*iN]);

	for(std::size_t i=0; i<iM; ++i)
		for(std::size_t j=0; j<iN; ++j)
			uptrMat.get()[i*iN + j] = mat(i,j);


	int iInfo = (*pfunc)(LAPACK_ROW_MAJOR, 'A', 'A', iM, iN, uptrMat.get(), iN,
		vecsvals.data(), uptrU.get(), iM, uptrVt.get(), iN, uptrWork.get());

	bool bOk = true;
	if(iInfo != 0)
	{
		log_err("Could not solve real singular value problem",
			" (lapack error ", iInfo, ").");
		bOk = false;
	}

	for(std::size_t i=0; i<iM; ++i)
		for(std::size_t j=0; j<iM; ++j)
			matU(i,j) = uptrU.get()[i*iM + j];
	for(std::size_t i=0; i<iN; ++i)
		for(std::size_t j=0; j<iN; ++j)
			matV(j,i) = uptrVt.get()[i*iN + j];	// transposed

	return bOk;
}



/**
 * calculates the singular values of a complex matrix: M = U diag(vals) (V*)^t
 */
template<typename T>
bool singvec_cplx(const ublas::matrix<std::complex<T>>& mat,
	ublas::matrix<std::complex<T>>& matU, ublas::matrix<std::complex<T>>& matV,
	std::vector<T>& vecsvals)
{
	using t_cplx = std::complex<T>;

	select_func<float, double, decltype(LAPACKE_cgesvd), decltype(LAPACKE_zgesvd)>
		sfunc(LAPACKE_cgesvd, LAPACKE_zgesvd);
	auto pfunc = sfunc.get_func<T>();

	const std::size_t iM = mat.size1();
	const std::size_t iN = mat.size2();
	const std::size_t iMin = std::min(iM,iN);

	vecsvals.resize(iMin);
	matU.resize(iM, iM);
	matV.resize(iN, iN);

	std::unique_ptr<t_cplx, std::default_delete<t_cplx[]>> uptrMat(new t_cplx[iM*iN]);
	std::unique_ptr<T, std::default_delete<T[]>> uptrWork(new T[iM*iN]);	// TODO: find correct size
	std::unique_ptr<t_cplx, std::default_delete<t_cplx[]>> uptrU(new t_cplx[iM*iM]);
	std::unique_ptr<t_cplx, std::default_delete<t_cplx[]>> uptrVt(new t_cplx[iN*iN]);

	for(std::size_t i=0; i<iM; ++i)
		for(std::size_t j=0; j<iN; ++j)
			uptrMat.get()[i*iN + j] = mat(i,j);


	int iInfo = (*pfunc)(LAPACK_ROW_MAJOR, 'A', 'A', iM, iN, uptrMat.get(), iN,
		vecsvals.data(), uptrU.get(), iM, uptrVt.get(), iN, uptrWork.get());

	bool bOk = true;
	if(iInfo != 0)
	{
		log_err("Could not solve complex singular value problem",
			" (lapack error ", iInfo, ").");
		bOk = false;
	}

	for(std::size_t i=0; i<iM; ++i)
		for(std::size_t j=0; j<iM; ++j)
			matU(i,j) = uptrU.get()[i*iM + j];
	for(std::size_t i=0; i<iN; ++i)
		for(std::size_t j=0; j<iN; ++j)
			matV(j,i) = uptrVt.get()[i*iN + j];	// transposed

	return bOk;
}



// ----------------------------------------------------------------------------



/**
 * pseudoinverse of a real diagonal matrix
 * see: https://de.wikipedia.org/wiki/Pseudoinverse#Berechnung
 */
template<typename T=double>
ublas::matrix<T> pseudoinverse_diag(const ublas::matrix<T>& mat)
{
	std::size_t N = std::min(mat.size1(), mat.size2());
	ublas::matrix<T> matRet = mat;

	for(std::size_t i=0; i<N; ++i)
	{
		if(!float_equal(mat(i,i), T(0)))
			matRet(i,i) = T(1) / mat(i,i);
	}

	return matRet;
}

/**
 * pseudoinverse M+ of a real matrix
 * M  = U D (V*)^t
 * M+ = V D+ (U*)^t
 * see: https://de.wikipedia.org/wiki/Pseudoinverse#Berechnung
 */
template<typename T=double>
bool pseudoinverse(const ublas::matrix<T>& mat, ublas::matrix<T>& matInv)
{
	std::vector<T> vecS;
	ublas::matrix<T> matU, matV;

	if(!singvec(mat, matU, matV, vecS))
		return false;

	ublas::matrix<T> matS = diag_matrix(vecS);
	matS = pseudoinverse_diag(matS);
	matU = transpose(matU);

	matInv = prod_mm(matS, matU);
	matInv = prod_mm(matV, matInv);

	return true;
}


/**
 * calculates the approximate eigenvectors
 */
template<typename T=double>
bool eigenvec_approxsym(const ublas::matrix<T>& mat,
	std::vector<ublas::vector<T>>& evecs, std::vector<T>& evals)
{
	ublas::matrix<T> matU, matV;
	bool bOk = singvec(mat, matU, matV, evals);

	evecs.resize(matV.size2());
	for(std::size_t j=0; j<matV.size2(); ++j)
		evecs[j] = get_column(matV, j);

	return bOk;
}


#endif




/**
 * count permutations to get from "idx1" to "idx2" (idx2 has to be sorted)
 * WARNING: doesn't work: next_permutation randomly shuffles the permutations!
 */
template<class t_lst = std::vector<std::size_t>>
std::pair<bool, std::size_t> count_permutations(const t_lst& idx1, const t_lst& _idx2)
{
	std::size_t iPerms = 0;
	bool bHasPerm = 1;
	t_lst idx2 = _idx2;

	while(1)
	{
		if(std::equal(idx1.begin(), idx1.end(), idx2.begin()))
			break;
		if(!std::next_permutation(idx2.begin(), idx2.end()))
		{
			bHasPerm = 0;
			break;
		}

		++iPerms;
	}

	return std::make_pair(bHasPerm, iPerms);
}


/**
 * count permutations to get to ordered indices (0-based)
 */
template<class t_lst = std::vector<std::size_t>>
std::pair<bool, std::size_t> count_permutations(const t_lst& _vecIdx)
{
	std::size_t iNumPerms = 0;
	bool bHasPerm = 1;
	t_lst vecIdx = _vecIdx;

	for(std::size_t iIdx = 0; iIdx < vecIdx.size(); ++iIdx)
	{
		// already at correct position?
		if(vecIdx[iIdx] == iIdx)
			continue;

		// else permutate
		auto iterPos = std::find(vecIdx.begin()+iIdx, vecIdx.end(), iIdx);
		if(iterPos == vecIdx.end())
		{
			// no permutation found
			bHasPerm = 0;
			break;
		}

		std::iter_swap(vecIdx.begin()+iIdx, iterPos);
		++iNumPerms;
	}

	return std::make_pair(bHasPerm, iNumPerms);
}


/**
 * elements of the (cartesian) epsilon tensor (indices 0-based)
 */
template<typename T = double, class t_lst = std::vector<std::size_t>>
T epsilon_tensor(const t_lst& idx)
{
	//t_lst idxIota(idx.size());
	//std::size_t iMin = *std::min_element(idx.begin(), idx.end());
	//std::iota(idxIota.begin(), idxIota.end(), iMin);

	std::size_t iNumPerms;
	bool bHasPerm;
	std::tie(bHasPerm, iNumPerms) = count_permutations<t_lst>(idx/*, idxIota*/);

	if(!bHasPerm)
		return T(0);
	if(is_even(iNumPerms))
		return T(1);
	return T(-1);
}


/**
 * elements of the (non-cartesian) epsilon tensor (indices 0-based)
 */
template<typename T = double, class t_lst = std::vector<std::size_t>,
	class t_mat = ublas::matrix<double>>
T epsilon_tensor(const t_mat& matGcov, const t_lst& idx, bool bCov = 1)
{
	T tEps = epsilon_tensor<T, t_lst>(idx);
	T tMetr = std::abs(determinant(matGcov));
	T tDet = std::sqrt(tMetr)*tEps;

	if(!bCov && !float_equal(tDet, T(0)))
		tDet = T(1) / tDet;
	return tDet;
}


/**
 * creates a metric tensor
 */
template<class t_mat = ublas::matrix<double>,
	class t_vec = ublas::vector<double>,
	template<class...> class t_lst = std::initializer_list>
t_mat make_metric_cov(const t_lst<t_vec>& lstVecsCov)
{
	using T = typename t_mat::value_type;
	const std::size_t iDim = std::min(lstVecsCov.size(), lstVecsCov.begin()->size());
	t_mat matG(iDim, iDim);

	typename t_lst<t_vec>::const_iterator iter;
	std::size_t i;
	for(iter=lstVecsCov.begin(), i=0; i<iDim; ++iter, ++i)
	{
		typename t_lst<t_vec>::const_iterator iter2;
		std::size_t j;
		for(iter2=lstVecsCov.begin(), j=0; j<iDim; ++iter2, ++j)
		{
			matG(i,j) = mult<t_vec, t_vec>(*iter, *iter2);
		}
	}

	return matG;
}


/**
 * inner product using metric matGCov
 */
template<class t_mat = ublas::matrix<double>,
	class t_vec = ublas::vector<double>>
typename t_vec::value_type inner_prod(const t_mat& matGCov,
	const t_vec& vec1Contra, const t_vec& vec2Contra)
{
	t_vec vec1Cov = mult<t_mat, t_vec>(matGCov, vec1Contra);
	return mult<t_vec, t_vec>(vec1Cov, vec2Contra);
}


/**
 * vector length
 */
template<class t_mat = ublas::matrix<double>,
	class t_vec = ublas::vector<double>>
typename t_vec::value_type vec_len(const t_mat& matGCov,
	const t_vec& vecContra)
{
	using T = typename t_vec::value_type;

	T tdot = inner_prod(matGCov, vecContra, vecContra);
	return std::sqrt(tdot);
}


/**
 * angle between vectors
 */
template<class t_mat = ublas::matrix<double>,
	class t_vec = ublas::vector<double>>
typename t_vec::value_type vec_angle(const t_mat& matGCov,
	const t_vec& vec1Contra, const t_vec& vec2Contra)
{
	using T = typename t_vec::value_type;

	T lenv1 = vec_len<t_mat, t_vec>(matGCov, vec1Contra);
	T lenv2 = vec_len<t_mat, t_vec>(matGCov, vec2Contra);
	T v1v2 = inner_prod<t_mat, t_vec>(matGCov, vec1Contra, vec2Contra);

	return std::acos(v1v2 / (lenv1 * lenv2));
}


/**
 * cross product using metric matGContra
 * see: (Arens 2015), p. 815
 */
template<class t_mat = ublas::matrix<double>,
	class t_vec = ublas::vector<double>>
t_vec cross_prod_contra(const t_mat& matGCov,
	const t_vec& vec1Contra, const t_vec& vec2Contra,
	bool bNorm = false)
{
	using T = typename t_mat::value_type;
	const std::size_t iDim = matGCov.size1();

	t_mat matGContra;
	inverse(matGCov, matGContra);

	t_vec vecCrossContra = zero_v<t_vec>(iDim);

	for(std::size_t j=0; j<iDim; ++j)
	{
		for(std::size_t k=0; k<iDim; ++k)
		{
			for(std::size_t l=0; l<iDim; ++l)
			{
				T eps = epsilon_tensor(matGCov, {l,j,k}, true);
				if(eps == T(0)) continue;

				for(std::size_t i=0; i<iDim; ++i)
					vecCrossContra[i] += matGContra(i,l)*eps * vec1Contra[j]*vec2Contra[k];
			}
		}
	}

	if(bNorm)
		vecCrossContra /= vec_len<t_mat, t_vec>(matGCov, vecCrossContra);
	return vecCrossContra;
}



/**
 * tensor product
 * see e.g.: (Arfken 2013), p. 109
 */
template<class t_mat = ublas::matrix<double>>
t_mat tensor_prod(const t_mat& mat1, const t_mat& mat2)
{
	t_mat mat(mat1.size1()*mat2.size1(), mat1.size2()*mat2.size2());

	for(std::size_t i=0; i<mat1.size1(); ++i)
	{
		for(std::size_t j=0; j<mat1.size2(); ++j)
		{
			t_mat matElem = mat1(i,j)*mat2;
			submatrix_copy(mat, matElem,
				i*mat2.size1(), j*mat2.size2());
		}
	}
	return mat;
}


}
#endif
