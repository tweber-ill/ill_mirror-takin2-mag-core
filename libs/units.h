/**
 * tlibs2
 * physical units library
 * @author Tobias Weber <tobias.weber@tum.de>, <tweber@ill.fr>
 * @date Dec-2015 -- 2018
 * @license GPLv3, see 'LICENSE' file
 * @desc Forked on 7-Nov-2018 from the privately and TUM-PhD-developed "tlibs" project (https://github.com/t-weber/tlibs).
 */

#ifndef __TLIBS2_UNITS__
#define __TLIBS2_UNITS__

#include <boost/units/unit.hpp>
#include <boost/units/quantity.hpp>
#include <boost/units/dimensionless_quantity.hpp>
#include <boost/units/cmath.hpp>
#include <boost/units/physical_dimensions.hpp>
#include <boost/units/io.hpp>

#include <boost/units/systems/si.hpp>
#include <boost/units/systems/angle/degrees.hpp>
#include <boost/units/systems/si/codata/universal_constants.hpp>
#include <boost/units/systems/si/codata/neutron_constants.hpp>
#include <boost/units/systems/si/codata/electron_constants.hpp>
#include <boost/units/systems/si/codata/electromagnetic_constants.hpp>
#include <boost/units/systems/si/codata/physico-chemical_constants.hpp>

#include <boost/math/constants/constants.hpp>
#include <boost/numeric/ublas/vector.hpp>

namespace units = boost::units;
namespace co = boost::units::si::constants::codata;


namespace tl2 {


template<typename T=double> constexpr T __pi = boost::math::constants::pi<T>();

// general quantities
template<class Sys, class T=double> using t_length = units::quantity<units::unit<units::length_dimension, Sys>, T>;
template<class Sys, class T=double> using t_momentum = units::quantity<units::unit<units::momentum_dimension, Sys>, T>;
template<class Sys, class T=double> using t_wavenumber = units::quantity<units::unit<units::wavenumber_dimension, Sys>, T>;
template<class Sys, class T=double> using t_velocity = units::quantity<units::unit<units::velocity_dimension, Sys>, T>;
template<class Sys, class T=double> using t_frequency = units::quantity<units::unit<units::frequency_dimension, Sys>, T>;
template<class Sys, class T=double> using t_energy = units::quantity<units::unit<units::energy_dimension, Sys>, T>;
template<class Sys, class T=double> using t_angle = units::quantity<units::unit<units::plane_angle_dimension, Sys>, T>;
template<class Sys, class T=double> using t_temperature = units::quantity<units::unit<units::temperature_dimension, Sys>, T>;
template<class Sys, class T=double> using t_mass = units::quantity<units::unit<units::mass_dimension, Sys>, T>;
template<class Sys, class T=double> using t_time = units::quantity<units::unit<units::time_dimension, Sys>, T>;
template<class Sys, class T=double> using t_flux = units::quantity<units::unit<units::magnetic_flux_density_dimension, Sys>, T>;
template<class Sys, class T=double> using t_area = units::quantity<units::unit<units::area_dimension, Sys>, T>;
template<class Sys, class T=double> using t_volume = units::quantity<units::unit<units::volume_dimension, Sys>, T>;

template<class Sys, class T=double> using t_length_inverse =
	units::quantity<units::unit<units::derived_dimension<units::length_base_dimension, -1>::type, Sys>, T>;
template<class Sys, class T=double> using t_length_square =
	units::quantity<units::unit<units::derived_dimension<units::length_base_dimension, 2>::type, Sys>, T>;
template<class Sys, class T=double> using t_momentum_square =
	units::quantity<units::unit<units::derived_dimension<units::momentum_dimension, 2>::type, Sys>, T>;
template<class Sys, class T=double> using t_action =
	units::quantity<units::unit<typename units::derived_dimension
	<units::mass_base_dimension,1, units::length_base_dimension,2, units::time_base_dimension,-1>::type, Sys>, T>;
template<class Sys, class T=double> using t_energy_per_temperature =
	units::quantity<units::unit<typename units::derived_dimension
	<units::mass_base_dimension,1, units::length_base_dimension,2,
	units::time_base_dimension,-2, units::temperature_base_dimension,-1>::type, Sys>, T>;
template<class Sys, class T=double> using t_energy_per_field =
	units::quantity<units::unit<typename units::derived_dimension
	<units::current_base_dimension,1, units::length_base_dimension,2>::type, Sys>, T>;
template<class Sys, class T=double> using t_inv_flux_time =
	units::quantity<units::unit<typename units::derived_dimension
	<units::current_base_dimension,1, units::time_base_dimension,1, units::mass_base_dimension,-1>::type, Sys>, T>;
template<class Sys, class T=double> using t_dimensionless =
	units::quantity<units::unit<units::dimensionless_type, Sys>, T>;


// synonyms
template<class Sys, class T=double> using t_freq = t_frequency<Sys, T>;
template<class Sys, class T=double> using t_temp = t_temperature<Sys, T>;


// si quantities -- partial specialisations
template<class Y=double> using t_length_si = t_length<units::si::system, Y>;
template<class Y=double> using t_length_inverse_si = t_length_inverse<units::si::system, Y>;
template<class Y=double> using t_momentum_si = t_momentum<units::si::system, Y>;
template<class Y=double> using t_wavenumber_si = t_wavenumber<units::si::system, Y>;
template<class Y=double> using t_velocity_si = t_velocity<units::si::system, Y>;
template<class Y=double> using t_frequency_si = t_frequency<units::si::system, Y>;
template<class Y=double> using t_energy_si = t_energy<units::si::system, Y>;
template<class Y=double> using t_angle_si = t_angle<units::si::system, Y>;
template<class Y=double> using t_temperature_si = t_temperature<units::si::system, Y>;
template<class Y=double> using t_mass_si = t_mass<units::si::system, Y>;
template<class Y=double> using t_time_si = t_time<units::si::system, Y>;
template<class Y=double> using t_flux_si = t_flux<units::si::system, Y>;
template<class Y=double> using t_area_si = t_area<units::si::system, Y>;
template<class Y=double> using t_action_si = t_action<units::si::system, Y>;
template<class Y=double> using t_energy_per_temperature_si = t_energy_per_temperature<units::si::system, Y>;
template<class Y=double> using t_inv_flux_time_si = t_inv_flux_time<units::si::system, Y>;


// constants
template<class Y=double> constexpr t_mass_si<Y> kg = Y(1) * units::si::kilograms;
template<class Y=double> constexpr t_temperature_si<Y> kelvin = Y(1) * units::si::kelvins;
template<class Y=double> constexpr t_temperature_si<Y> kelvins = kelvin<Y>;
template<class Y=double> constexpr t_angle_si<Y> deg = Y(__pi<Y>/Y(180.)) * units::si::radians;
template<class Y=double> constexpr t_angle_si<Y> radian = Y(1) * units::si::radians;
template<class Y=double> constexpr t_angle_si<Y> radians = radian<Y>;
template<class Y=double> constexpr t_flux_si<Y> tesla = Y(1) * units::si::teslas;
template<class Y=double> constexpr t_flux_si<Y> teslas = Y(1)*units::si::teslas;
template<class Y=double> constexpr t_flux_si<Y> kilogauss = Y(0.1) * units::si::teslas;
template<class Y=double> constexpr t_length_si<Y> meter = Y(1) * units::si::meters;
template<class Y=double> constexpr t_length_si<Y> meters = meter<Y>;
template<class Y=double> constexpr t_length_si<Y> cm = meters<Y>/Y(100);
template<class Y=double> constexpr t_time_si<Y> seconds = Y(1) * units::si::seconds;
template<class Y=double> constexpr t_time_si<Y> sec = seconds<Y>;
template<class Y=double> constexpr t_time_si<Y> ps = Y(1e-12)*seconds<Y>;
template<class Y=double> constexpr t_area_si<Y> barns = Y(1e-28) * units::si::meters*units::si::meters;;
template<class Y=double> constexpr t_area_si<Y> barn = barns<Y>;
template<class Y=double> constexpr t_energy_si<Y> meV = Y(1e-3) * Y(co::e/units::si::coulombs)*units::si::coulombs*units::si::volts;;
template<class Y=double> constexpr t_energy_si<Y> eV = Y(1) * Y(co::e/units::si::coulombs)*units::si::coulombs*units::si::volts;
template<class Y=double> constexpr t_length_si<Y> angstrom = Y(1e-10) * units::si::meters;
template<class Y=double> constexpr t_length_si<Y> angstroms = angstrom<Y>;
template<class Y=double> constexpr t_mass_si<Y> m_n = Y(co::m_n/units::si::kilograms)*units::si::kilograms;
template<class Y=double> constexpr t_mass_si<Y> m_e = Y(co::m_e/units::si::kilograms)*units::si::kilograms;
template<class Y=double> constexpr t_mass_si<Y> amu = Y(co::m_u/units::si::kilograms)*units::si::kilograms;
template<class Y=double> constexpr t_action_si<Y> hbar = Y(co::hbar/units::si::joules/units::si::seconds)*units::si::joules*units::si::seconds;
template<class Y=double> constexpr t_action_si<Y> h = hbar<Y> * Y(2)*__pi<Y>;
template<class Y=double> constexpr t_velocity_si<Y> c = Y(co::c/units::si::meters*units::si::seconds)*units::si::meters/units::si::seconds;

template<class Y=double> constexpr t_length_si<Y> r_e = Y(co::r_e/units::si::meters)*units::si::meters;
template<class Y=double> constexpr Y g_n = Y(co::g_n.value());
template<class Y=double> constexpr Y g_e = Y(co::g_e.value());
template<class Y=double> constexpr t_inv_flux_time<units::si::system, Y> gamma_n = Y(co::gamma_n*units::si::tesla*units::si::seconds)/units::si::tesla/units::si::seconds;
template<class Y=double> constexpr t_inv_flux_time<units::si::system, Y> gamma_e = Y(co::gamma_e*units::si::tesla*units::si::seconds)/units::si::tesla/units::si::seconds;
template<class Y=double> constexpr t_energy_per_temperature<units::si::system, Y> kB = Y(co::k_B*units::si::kelvin/units::si::joules)/units::si::kelvin*units::si::joules;
template<class Y=double> constexpr t_energy_per_field<units::si::system, Y> muB = Y(co::mu_B/units::si::joules*units::si::tesla)*units::si::joules/units::si::tesla;
template<class Y=double> constexpr t_energy_per_field<units::si::system, Y> mu_n = Y(co::mu_n/units::si::joules*units::si::tesla)*units::si::joules/units::si::tesla;
template<class Y=double> constexpr t_energy_per_field<units::si::system, Y> mu_N = Y(co::mu_N/units::si::joules*units::si::tesla)*units::si::joules/units::si::tesla;
template<class Y=double> constexpr t_energy_per_field<units::si::system, Y> mu_e = Y(co::mu_e/units::si::joules*units::si::tesla)*units::si::joules/units::si::tesla;



// helper functions
template<class t_quant>
t_quant my_units_sqrt(const decltype(t_quant() * t_quant())& val)
{
	using t_quant_sq = decltype(t_quant() * t_quant());
	using Y = typename t_quant::value_type;

	t_quant one_quant = t_quant::from_value(Y(1));
	t_quant_sq one_quant_sq = t_quant_sq::from_value(Y(1));

	Y valsq = Y(val / one_quant_sq);
	return std::sqrt(valsq) * one_quant;
}

template<class t_quant>
decltype(t_quant()*t_quant()) my_units_pow2(const t_quant& val)
{
	return val*val;
}

template<class t_elem, template<class...> class t_vec=boost::numeric::ublas::vector>
t_elem my_units_norm2(const t_vec<t_elem>& vec)
{
	using t_elem_sq = decltype(t_elem()*t_elem());
	t_elem_sq tRet = t_elem_sq();

	for(std::size_t i=0; i<vec.size(); ++i)
		tRet += vec[i]*vec[i];

	return my_units_sqrt<t_elem>(tRet);
}

}
#endif
