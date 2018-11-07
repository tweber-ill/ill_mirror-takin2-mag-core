/**
 * tlibs2
 * physics library
 * @author Tobias Weber <tobias.weber@tum.de>, <tweber@ill.fr>
 * @date 2012 -- 2018
 * @license GPLv3, see 'LICENSE' file
 * @desc Forked on 7-Nov-2018 from the privately and TUM-PhD-developed "tlibs" project (https://github.com/t-weber/tlibs).
 */

#ifndef __TLIBS2_PHYS__
#define __TLIBS2_PHYS__

#include "units.h"
#include "math.h"
#include "log.h"

#include <boost/units/pow.hpp>
#include <cmath>


namespace tl2 {


// --------------------------------------------------------------------------------


template<class T=double> constexpr T KSQ2E = T(0.5) * hbar<T>/angstrom<T>/m_n<T> * hbar<T>/angstrom<T>/meV<T>;
template<class T=double> constexpr T E2KSQ = T(1)/KSQ2E<T>;


// --------------------------------------------------------------------------------


// --------------------------------------------------------------------------------
// de Broglie stuff
// lam = h/p

template<class Sys, class Y>
t_momentum<Sys,Y> lam2p(const t_length<Sys,Y>& lam)
{
	return h<Y> / lam;
}

template<class Sys, class Y>
t_length<Sys,Y> p2lam(const t_momentum<Sys,Y>& p)
{
	return h<Y> / p;
}


// lam = 2pi/k
template<class Sys, class Y>
t_length<Sys,Y> k2lam(const t_wavenumber<Sys,Y>& k)
{
	return Y(2.)*pi<Y> / k;
}

template<class Sys, class Y>
t_wavenumber<Sys,Y> lam2k(const t_length<Sys,Y>& lam)
{
	return Y(2.)*pi<Y> / lam;
}

template<class Sys, class Y>
t_momentum<Sys,Y> k2p(const t_wavenumber<Sys,Y>& k)
{
	return hbar<Y>*k;
}

template<class Sys, class Y>
t_wavenumber<Sys,Y> p2k(const t_momentum<Sys,Y>& p)
{
	return p/hbar<Y>;
}

template<class Sys, class Y>
t_velocity<Sys,Y> k2v(const t_wavenumber<Sys,Y>& k)
{
	return k2p(k) / m_n<Y>;
}

template<class Sys, class Y>
t_wavenumber<Sys,Y> v2k(const t_velocity<Sys,Y>& v)
{
	return m_n<Y>*v/hbar<Y>;
}
// --------------------------------------------------------------------------------




// --------------------------------------------------------------------------------
// E = hbar*omega

template<class Sys, class Y>
t_energy<Sys,Y> omega2E(const t_freq<Sys,Y>& omega)
{
	return hbar<Y> * omega;
}

template<class Sys, class Y>
t_freq<Sys,Y> E2omega(const t_energy<Sys,Y>& en)
{
	return en / hbar<Y>;
}

template<class Sys, class Y>
t_energy<Sys,Y> k2E_direct(const t_wavenumber<Sys,Y>& k)
{
	t_momentum<Sys,Y> p = hbar<Y>*k;
	t_energy<Sys,Y> E = p*p / (Y(2.)*m_n<Y>);
	return E;
}

template<class Sys, class Y>
t_wavenumber<Sys,Y> E2k_direct(const t_energy<Sys,Y>& _E, bool &bImag)
{
	bImag = (_E < Y(0.)*meV<Y>);
	t_energy<Sys,Y> E = bImag ? -_E : _E;

	auto pp = Y(2.) * m_n<Y> * E;
	//t_momentum<Sys,Y> p = units::sqrt<typename decltype(pp)::unit_type, Y>(pp);
	t_momentum<Sys,Y> p = my_units_sqrt<t_momentum<Sys,Y>>(pp);
	t_wavenumber<Sys,Y> k = p / hbar<Y>;
	return k;
}
// --------------------------------------------------------------------------------


// --------------------------------------------------------------------------------
// indirect calculations using conversion factors for numerical stability

template<class Sys, class Y>
t_energy<Sys,Y> k2E(const t_wavenumber<Sys,Y>& k)
{
	Y dk = k*angstrom<Y>;
	Y dE = KSQ2E<Y> * dk*dk;
	return dE * meV<Y>;
}

template<class Sys, class Y>
t_wavenumber<Sys,Y> E2k(const t_energy<Sys,Y>& _E, bool &bImag)
{
	bImag = (_E < Y(0.)*meV<Y>);
	t_energy<Sys,Y> E = bImag ? -_E : _E;
	const Y dE = E / meV<Y>;
	const Y dk = std::sqrt(E2KSQ<Y> * dE);
	return dk / angstrom<Y>;
}

// --------------------------------------------------------------------------------




// --------------------------------------------------------------------------------
/**
 * Bragg equation
 * real: n * lam = 2d * sin(twotheta/2)
 */
template<class Sys, class Y>
t_length<Sys,Y> bragg_real_lam(const t_length<Sys,Y>& d,
	const t_angle<Sys,Y>& twotheta, Y n = Y(1))
{
	return Y(2.)*d/n * units::sin(twotheta/Y(2.));
}

template<class Sys, class Y>
t_length<Sys,Y> bragg_real_d(const t_length<Sys,Y>& lam,
	const t_angle<Sys,Y>& twotheta, Y n = Y(1))
{
	return n * lam / (Y(2.)*units::sin(twotheta/Y(2.)));
}

template<class Sys, class Y>
t_angle<Sys,Y> bragg_real_twotheta(const t_length<Sys,Y>& d,
	const t_length<Sys,Y>& lam, Y n = Y(1))
{
	auto dS = n*lam/(Y(2.)*d);
	if(std::abs(Y(dS)) > Y(1))
		throw Err("Invalid twotheta angle.");
	return units::asin(dS) * Y(2.);
}


/**
 * reciprocal Bragg equation: G * lam = 4pi * sin(twotheta/2)
 */
template<class Sys, class Y>
t_angle<Sys,Y> bragg_recip_twotheta(const t_wavenumber<Sys,Y>& G,
	const t_length<Sys,Y>& lam, Y n = Y(1))
{
	auto dS = G*n*lam/(Y(4)*pi<Y>);
	if(std::abs(Y(dS)) > Y(1))
		throw Err("Invalid twotheta angle.");
	return units::asin(dS) * Y(2);
}

template<class Sys, class Y>
t_wavenumber<Sys,Y> bragg_recip_G(const t_length<Sys,Y>& lam,
	const t_angle<Sys,Y>& twotheta, Y n = Y(1))
{
	return Y(4)*pi<Y> / (n*lam) * units::sin(twotheta/Y(2));
}

template<class Sys, class Y>
t_wavenumber<Sys,Y> bragg_recip_Q(const t_length<Sys,Y>& lam,
	const t_angle<Sys,Y>& twotheta, Y n = Y(1))
{ return bragg_recip_G<Sys,Y>(lam,twotheta,n); }

template<class Sys, class Y>
t_length<Sys,Y> bragg_recip_lam(const t_wavenumber<Sys,Y>& G,
	const t_angle<Sys,Y>& twotheta, Y n = Y(1))
{
	return Y(4)*pi<Y> / G * units::sin(twotheta/Y(2)) / n;
}


/**
 * reciprocal Bragg equation [2]: n * G = 2*k * sin(twotheta/2)
 */
template<class Sys, class Y>
t_wavenumber<Sys,Y> bragg_recip_G(const t_wavenumber<Sys,Y>& k,
	const t_angle<Sys,Y>& twotheta, Y n = Y(1))
{
	return Y(2)*k / n * units::sin(twotheta/Y(2));
}

template<class Sys, class Y>
t_wavenumber<Sys,Y> bragg_recip_k(const t_wavenumber<Sys,Y>& G,
	const t_angle<Sys,Y>& twotheta, Y n = Y(1))
{
	return n*G / (Y(2) * units::sin(twotheta/Y(2)));
}

template<class Sys, class Y>
t_angle<Sys,Y> bragg_recip_twotheta(const t_wavenumber<Sys,Y>& G,
	const t_wavenumber<Sys,Y>& k, Y n = Y(1))
{
	auto dS = n * G / (Y(2) * k);
	if(std::abs(Y(dS)) > Y(1))
		throw Err("Invalid twotheta angle.");
	return units::asin(dS) * Y(2);
}



// G = 2pi / d
template<class Sys, class Y>
t_length<Sys,Y> G2d(const t_wavenumber<Sys,Y>& G)
{
	return Y(2.)*pi<Y> / G;
}

template<class Sys, class Y>
t_wavenumber<Sys,Y> d2G(const t_length<Sys,Y>& d)
{
	return Y(2.)*pi<Y> / d;
}

// --------------------------------------------------------------------------------


// --------------------------------------------------------------------------------
/**
 * differentiated Bragg equation:
 * n lam = 2d sin(th)						| diff
 * n dlam = 2dd sin(th) + 2d cos(th) dth	| / Bragg equ
 * dlam/lam = dd/d + cos(th)/sin(th) dth
 *
 * n G = 2k sin(th)
 * n dG = 2dk sin(th) + 2k cos(th) dth
 * dG/G = dk/k + cos(th)/sin(th) dth
 */
template<class Sys, class Y=double>
Y bragg_diff(Y dDoverD, const t_angle<Sys,Y>& theta, Y dTheta)
{
	Y dLamOverLam = dDoverD + units::cos(theta)/units::sin(theta) * dTheta;
	return dLamOverLam;
}

// --------------------------------------------------------------------------------




// --------------------------------------------------------------------------------
// see e.g. ILL blue book sec. 2.6-2

template<class Sys, class Y>
t_wavenumber<Sys,Y> kinematic_plane(bool bFixedKi,
	const t_energy<Sys,Y>& EiEf, const t_energy<Sys,Y>& DeltaE,
	const t_angle<Sys,Y>& twotheta)
{
	const t_energy<Sys,Y> dE = DeltaE;
	if(bFixedKi)
		dE = -dE;

	t_wavenumber<Sys,Y> Q =
		units::sqrt(Y(2.)*m_n<Y> / hbar<Y>) *
		(Y(2.)*EiEf + dE - Y(2.)*units::cos(twotheta) *
		units::sqrt(EiEf*(EiEf + dE)));

	return Q;
}

template<class Sys, class Y>
t_energy<Sys,Y> kinematic_plane(bool bFixedKi, bool bBranch,
	const t_energy<Sys,Y>& EiEf, const t_wavenumber<Sys,Y>& Q,
	const t_angle<Sys,Y>& twotheta)
{
	auto c = Y(2.)*m_n<Y> / (hbar<Y>*hbar<Y>);
	Y ctt = units::cos(twotheta);
	Y c2tt = units::cos(Y(2.)*twotheta);

	Y dSign = Y(-1.);
	if(bBranch)
		dSign = Y(1.);

	Y dSignFixedKf = Y(1.);
	if(bFixedKi)
		dSignFixedKf = Y(-1.);

	using t_sqrt_rt = decltype(c*c*EiEf*ctt);
	using t_rt = decltype(t_sqrt_rt()*t_sqrt_rt());
	t_rt rt = c*c*c*c * (-EiEf*EiEf)*ctt*ctt
		+ c*c*c*c*EiEf*EiEf*ctt*ctt*c2tt
		+ Y(2.)*c*c*c*EiEf*Q*Q*ctt*ctt;

	t_energy<Sys,Y> E =
		Y(1.)/(c*c)*(dSignFixedKf*Y(2.)*c*c*EiEf*ctt*ctt
		- dSignFixedKf*Y(2.)*c*c*EiEf
		+ dSign*std::sqrt(Y(2.)) * my_units_sqrt<t_sqrt_rt>(rt)
		+ dSignFixedKf*c*Q*Q);

	return E;
}
// --------------------------------------------------------------------------------



// --------------------------------------------------------------------------------
// scattering triangle / TAS stuff

/**
 * Q_vec = ki_vec - kf_vec
 * kf_vec = ki_vec - Q_vec
 * kf^2 = ki^2 + Q^2 - 2ki Q cos th
 * cos th = (-kf^2 + ki^2 + Q^2) / (2kiQ)
 */
template<class Sys, class Y>
t_angle<Sys,Y> get_angle_ki_Q(const t_wavenumber<Sys,Y>& ki,
	const t_wavenumber<Sys,Y>& kf,
	const t_wavenumber<Sys,Y>& Q,
	bool bPosSense=1, bool bAngleOutsideTriag=0)
{
	t_angle<Sys,Y> angle;

	if(Q*angstrom<Y> == Y(0.))
		angle = pi<Y>/Y(2) * units::si::radians;
	else
	{
		auto c = (ki*ki - kf*kf + Q*Q) / (Y(2.)*ki*Q);
		if(units::abs(c) > Y(1.))
			throw Err("Scattering triangle not closed.");

		angle = units::acos(c);
	}

	if(bAngleOutsideTriag) angle = pi<Y>*units::si::radians - angle;
	if(!bPosSense) angle = -angle;

	return angle;
}

/**
 * Q_vec = ki_vec - kf_vec
 * ki_vec = Q_vec + kf_vec
 * ki^2 = Q^2 + kf^2 + 2Q kf cos th
 * cos th = (ki^2 - Q^2 - kf^2) / (2Q kf)
 */
template<class Sys, class Y>
t_angle<Sys,Y> get_angle_kf_Q(const t_wavenumber<Sys,Y>& ki,
	const t_wavenumber<Sys,Y>& kf,
	const t_wavenumber<Sys,Y>& Q,
	bool bPosSense=1, bool bAngleOutsideTriag=1)
{
	t_angle<Sys,Y> angle;

	if(Q*angstrom<Y> == Y(0.))
		angle = pi<Y>/Y(2) * units::si::radians;
	else
	{
		auto c = (ki*ki - kf*kf - Q*Q) / (Y(2.)*kf*Q);
		if(units::abs(c) > Y(1.))
			throw Err("Scattering triangle not closed.");

		angle = units::acos(c);
	}

	if(!bAngleOutsideTriag) angle = pi<Y>*units::si::radians - angle;
	if(!bPosSense) angle = -angle;

	return angle;
}


template<class Sys, class Y>
t_angle<Sys,Y> get_mono_twotheta(const t_wavenumber<Sys,Y>& k,
	const t_length<Sys,Y>& d, bool bPosSense=1)
{
	const Y dOrder = Y(1.);
	t_angle<Sys,Y> tt = bragg_real_twotheta(d, k2lam(k), dOrder);
	if(!bPosSense)
		tt = -tt;
	return tt;
}

template<class Sys, class Y>
t_wavenumber<Sys,Y> get_mono_k(const t_angle<Sys,Y>& _theta,
	const t_length<Sys,Y>& d, bool bPosSense=1)
{
	t_angle<Sys,Y> theta = _theta;
	if(!bPosSense)
		theta = -theta;

	const Y dOrder = Y(1.);
	return lam2k(bragg_real_lam(d, Y(2.)*theta, dOrder));
}


/**
 * Q_vec = ki_vec - kf_vec
 * Q^2 = ki^2 + kf^2 - 2ki kf cos 2th
 *cos 2th = (-Q^2 + ki^2 + kf^2) / (2ki kf)
 */
template<class Sys, class Y>
t_angle<Sys,Y> get_sample_twotheta(const t_wavenumber<Sys,Y>& ki,
	const t_wavenumber<Sys,Y>& kf, const t_wavenumber<Sys,Y>& Q,
	bool bPosSense=1)
{
	t_dimensionless<Sys,Y> ttCos = (ki*ki + kf*kf - Q*Q)/(Y(2.)*ki*kf);
	if(units::abs(ttCos) > Y(1.))
		throw Err("Scattering triangle not closed.");

	t_angle<Sys,Y> tt;
	tt = units::acos(ttCos);

	if(!bPosSense) tt = -tt;
	return tt;
}


/**
 * again cos theorem:
 * Q_vec = ki_vec - kf_vec
 * Q^2 = ki^2 + kf^2 - 2ki kf cos 2th
 * Q = sqrt(ki^2 + kf^2 - 2ki kf cos 2th)
 */
template<class Sys, class Y>
const t_wavenumber<Sys,Y>
get_sample_Q(const t_wavenumber<Sys,Y>& ki,
	const t_wavenumber<Sys,Y>& kf, const t_angle<Sys,Y>& tt)
{
	t_dimensionless<Sys,Y> ctt = units::cos(tt);
	decltype(ki*ki) Qsq = ki*ki + kf*kf - Y(2.)*ki*kf*ctt;
	if(Y(Qsq*angstrom<Y>*angstrom<Y>) < Y(0.))
	{
		// TODO

		Qsq = -Qsq;
	}

	//t_wavenumber<Sys,Y> Q = units::sqrt(Qsq);
	t_wavenumber<Sys,Y> Q = my_units_sqrt<t_wavenumber<Sys,Y>>(Qsq);
	return Q;
}



template<class Sys, class Y>
t_energy<Sys,Y> get_energy_transfer(const t_wavenumber<Sys,Y>& ki,
	const t_wavenumber<Sys,Y>& kf)
{
	return k2E<Sys,Y>(ki) - k2E<Sys,Y>(kf);
}


/**
 * (hbar*ki)^2 / (2*mn)  -  (hbar*kf)^2 / (2mn)  =  E
 * 1) ki^2  =  +E * 2*mn / hbar^2  +  kf^2
 * 2) kf^2  =  -E * 2*mn / hbar^2  +  ki^2
 */
template<class Sys, class Y>
t_wavenumber<Sys,Y> get_other_k(const t_energy<Sys,Y>& E,
	const t_wavenumber<Sys,Y>& kfix, bool bFixedKi)
{
	auto kE_sq = E*Y(2.)*(m_n<Y>/hbar<Y>)/hbar<Y>;
	if(bFixedKi) kE_sq = -kE_sq;

	auto k_sq = kE_sq + kfix*kfix;
	if(k_sq*angstrom<Y>*angstrom<Y> < Y(0.))
		throw Err("Scattering triangle not closed.");

	//return units::sqrt(k_sq);
	return my_units_sqrt<t_wavenumber<Sys,Y>>(k_sq);
}

// --------------------------------------------------------------------------------



// --------------------------------------------------------------------------------

/**
 * kf^3 mono/ana reflectivity factor, see e.g. (Shirane 2002) p. 125
 */
template<class Sys, class Y>
Y ana_effic_factor(const t_wavenumber<Sys, Y>& kf, const t_angle<Sys, Y>& theta)
{
	return kf*kf*kf / units::tan(theta) * angstrom<Y>*angstrom<Y>*angstrom<Y>;
}

/**
 * kf^3 mono/ana reflectivity factor, see e.g. (Shirane 2002) p. 125
 */
template<class Sys, class Y>
Y ana_effic_factor(const t_wavenumber<Sys, Y>& kf, const t_length<Sys, Y>& d)
{
	t_angle<Sys, Y> theta = Y(0.5)*units::abs(get_mono_twotheta<Sys, Y>(kf, d, true));
	return ana_effic_factor<Sys, Y>(kf, theta);
}

// --------------------------------------------------------------------------------



/**
 * Bose distribution
 * see e.g.: (Shirane 2002), p. 28
 */
template<class t_real=double>
t_real bose(t_real E, t_real T)
{
	const t_real _kB = kB<t_real> * kelvin<t_real>/meV<t_real>;

	t_real n = t_real(1)/(std::exp(std::abs(E)/(_kB*T)) - t_real(1));
	if(E >= t_real(0))
		n += t_real(1);

	return n;
}


/**
 * Bose factor with a lower cutoff energy
 */
template<class t_real=double>
t_real bose_cutoff(t_real E, t_real T, t_real E_cutoff=t_real(0.02))
{
	t_real dB;

	E_cutoff = std::abs(E_cutoff);
	if(std::abs(E) < E_cutoff)
		dB = bose<t_real>(sign(E)*E_cutoff, T);
	else
		dB = bose<t_real>(E, T);

	return dB;
}


template<class Sys, class Y>
Y bose(const t_energy<Sys,Y>& E, const t_temperature<Sys,Y>& T,
	t_energy<Sys,Y> E_cutoff = -meV<Y>)
{
	if(E_cutoff < Y(0)*meV<Y>)
		return bose<Y>(Y(E/meV<Y>), Y(T/kelvin<Y>));
	else
		return bose_cutoff<Y>(Y(E/meV<Y>), Y(T/kelvin<Y>),
			Y(E_cutoff/meV<Y>));
}


/**
 * see: B. Fak, B. Dorner, Physica B 234-236 (1997) pp. 1107-1108
 */
template<class t_real=double>
t_real DHO_model(t_real E, t_real T, t_real E0, t_real hwhm, t_real amp, t_real offs)
{
	//if(E0*E0 - hwhm*hwhm < 0.) return 0.;
	return std::abs(bose<t_real>(E, T)*amp/(E0*pi<t_real>) *
		(hwhm/((E-E0)*(E-E0) + hwhm*hwhm) - hwhm/((E+E0)*(E+E0) + hwhm*hwhm)))
		+ offs;
}


// --------------------------------------------------------------------------------

/**
 * Fermi distribution
 */
template<class t_real=double>
t_real fermi(t_real E, t_real mu, t_real T)
{
	const t_real _kB = kB<t_real> * kelvin<t_real>/meV<t_real>;
	t_real n = t_real(1)/(std::exp((E-mu)/(_kB*T)) + t_real(1));
	return n;
}

template<class Sys, class Y>
Y fermi(const t_energy<Sys,Y>& E, const t_energy<Sys,Y>& mu, const t_temperature<Sys,Y>& T)
{
	return fermi<Y>(Y(E/meV<Y>), Y(mu/meV<Y>), Y(T/kelvin<T>));
}

// --------------------------------------------------------------------------------


/**
 * get macroscopic from microscopic cross-section
 */
template<class Sys, class Y=double>
t_length_inverse<Sys, Y> macro_xsect(const t_area<Sys, Y>& xsect,
	unsigned int iNumAtoms, const t_volume<Sys, Y>& volUC)
{
	return xsect * Y(iNumAtoms) / volUC;
}



// --------------------------------------------------------------------------------

/**
 * thin lens equation: 1/f = 1/lenB + 1/lenA
 */
template<class Sys, class Y=double>
t_length<Sys, Y> focal_len(const t_length<Sys, Y>& lenBefore, const t_length<Sys, Y>& lenAfter)
{
	const t_length_inverse<Sys, Y> f_inv = Y(1)/lenBefore + Y(1)/lenAfter;
	return Y(1) / f_inv;
}

/**
 * optimal mono/ana curvature, see e.g. (Shirane 2002) p. 66
 * or nicos/nicos-core.git/tree/nicos/devices/tas/mono.py in nicos
 * or Monochromator_curved.comp in McStas
 */
template<class Sys, class Y=double>
t_length<Sys, Y> foc_curv(const t_length<Sys, Y>& lenBefore, const t_length<Sys, Y>& lenAfter,
	const t_angle<Sys, Y>& tt, bool bVert)
{
	const t_length<Sys, Y> f = focal_len<Sys, Y>(lenBefore, lenAfter);
	const Y s = Y(units::abs(units::sin(Y(0.5)*tt)));

	const t_length<Sys, Y> curv = bVert ? Y(2)*f*s : Y(2)*f/s;

	return curv;
}


// --------------------------------------------------------------------------------


// --------------------------------------------------------------------------------
/**
 * @brief disc chopper burst time, see: NIMA 492, pp. 97-104 (2002)
 * @param r chopper radius
 * @param L chopper window length
 * @param om chopper frequency
 * @param bCounterRot single disc or two counter-rotating discs?
 * @param bSigma burst time in sigma or fwhm?
 * @return burst time
 */
template<class Sys, class Y=double>
t_time<Sys,Y> burst_time(const t_length<Sys,Y>& r, 
	const t_length<Sys,Y>& L, const t_freq<Sys,Y>& om, bool bCounterRot,
	bool bSigma=1)
{
	const Y tSig = bSigma ? FWHM2SIGMA<Y> : Y(1);
	Y tScale = bCounterRot ? Y(2) : Y(1);
	return L / (r * om * tScale) * tSig;
}

template<class Sys, class Y=double>
t_length<Sys,Y> burst_time_L(const t_length<Sys,Y>& r,
	const t_time<Sys,Y>& dt, const t_freq<Sys,Y>& om, bool bCounterRot,
	bool bSigma=1)
{
	const Y tSig = bSigma ? FWHM2SIGMA<Y> : Y(1);
	Y tScale = bCounterRot ? Y(2) : Y(1);
	return dt * r * om * tScale / tSig;
}

template<class Sys, class Y=double>
t_length<Sys,Y> burst_time_r(const t_time<Sys,Y>& dt,
	const t_length<Sys,Y>& L, const t_freq<Sys,Y>& om, bool bCounterRot,
	bool bSigma=1)
{
	const Y tSig = bSigma ? FWHM2SIGMA<Y> : Y(1);
	Y tScale = bCounterRot ? Y(2) : Y(1);
	return L / (dt * om * tScale) * tSig;
}

template<class Sys, class Y=double>
t_freq<Sys,Y> burst_time_om(const t_length<Sys,Y>& r, 
	const t_length<Sys,Y>& L, const t_time<Sys,Y>& dt, bool bCounterRot,
	bool bSigma=1)
{
	const Y tSig = bSigma ? FWHM2SIGMA<Y> : Y(1);
	Y tScale = bCounterRot ? Y(2) : Y(1);
	return L / (r * dt * tScale) * tSig;
}
// --------------------------------------------------------------------------------



// --------------------------------------------------------------------------------

/**
 * @brief collimation
 * @param L length of collimator
 * @param w distance between blade
 * @param bSigma calculate sigma or fwhm?
 * @return angular divergence
 */
template<class Sys, class Y=double>
t_angle<Sys,Y> colli_div(const t_length<Sys,Y>& L, const t_length<Sys,Y>& w, bool bSigma=1)
{
	const Y tSig = bSigma ? FWHM2SIGMA<Y> : Y(1);
	return units::atan(w/L) * tSig;
}

template<class Sys, class Y=double>
t_length<Sys,Y> colli_div_L(const t_angle<Sys,Y>& ang, const t_length<Sys,Y>& w, bool bSigma=1)
{
	const Y tSig = bSigma ? FWHM2SIGMA<Y> : Y(1);
	return w/units::tan(ang/tSig);
}

template<class Sys, class Y=double>
t_length<Sys,Y> colli_div_w(const t_length<Sys,Y>& L, const t_angle<Sys,Y>& ang, bool bSigma=1)
{
	const Y tSig = bSigma ? FWHM2SIGMA<Y> : Y(1);
	return units::tan(ang/tSig) * L;
}

// --------------------------------------------------------------------------------



// --------------------------------------------------------------------------------
/**
 * @brief velocity selector
 * @return selector angular frequency
 */
template<class Sys, class Y=double>
t_freq<Sys, Y> vsel_freq(const t_length<Sys,Y>& lam,
	const t_length<Sys,Y>& len, const t_angle<Sys,Y>& twist)
{
	t_velocity<Sys,Y> v_n = k2v<Sys,Y>(lam2k<Sys,Y>(lam));
	return v_n*twist / (len * radian<Y>);
}

template<class Sys, class Y=double>
t_length<Sys,Y> vsel_len(const t_length<Sys,Y>& lam,
	const t_freq<Sys, Y>& om, const t_angle<Sys,Y>& twist)
{
	t_velocity<Sys,Y> v_n = k2v<Sys,Y>(lam2k<Sys,Y>(lam));
	return v_n*twist / (om * radian<Y>);
}

template<class Sys, class Y=double>
t_angle<Sys,Y> vsel_twist(const t_length<Sys,Y>& lam,
	const t_freq<Sys, Y>& om, const t_length<Sys,Y>& len)
{
	t_velocity<Sys,Y> v_n = k2v<Sys,Y>(lam2k<Sys,Y>(lam));
	return  (len * om * radian<Y>) / v_n;
}

template<class Sys, class Y=double>
t_length<Sys,Y> vsel_lam(const t_angle<Sys,Y>& twist,
	const t_freq<Sys, Y>& om, const t_length<Sys,Y>& len)
{
	t_velocity<Sys,Y> v_n = (len * om * radian<Y>) / twist;
	return k2lam<Sys,Y>(v2k<Sys,Y>(v_n));
}

// --------------------------------------------------------------------------------


}
#endif
