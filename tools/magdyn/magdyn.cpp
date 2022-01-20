/**
 * magnon dynamics
 * @author Tobias Weber <tweber@ill.fr>
 * @date jan-2022
 * @license GPLv3
 *
 * References:
 *   - (Toth 2015) S. Toth and B. Lake, J. Phys.: Condens. Matter 27 166002 (2015):
 *     https://doi.org/10.1088/0953-8984/27/16/166002
 *   - (Heinsdorf 2021) N. Heinsdorf, example ferromagnetic calculation, personal communication, 2021, 2022.
 */

#include <tuple>
#include <algorithm>
#include <numeric>
#include <iostream>
#include <fstream>
#include <iomanip>

#include "magdyn.h"
#include "tlibs2/libs/units.h"
#include "tlibs2/libs/helper.h"

namespace pt = boost::property_tree;


// ----------------------------------------------------------------------------
// helper functions
// ----------------------------------------------------------------------------

/**
 * converts the rotation matrix rotating the local spins to ferromagnetic
 * [001] directions into the vectors comprised of the matrix columns
 * @see equation (9) from (Toth 2015).
 */
static std::tuple<t_vec, t_vec> R_to_uv(const t_mat& R)
{
	// imaginary unit
	const t_cplx imag{0., 1.};

	t_vec u = tl2::col<t_mat, t_vec>(R, 0)
		+ imag*tl2::col<t_mat, t_vec>(R, 1);
	t_vec v = tl2::col<t_mat, t_vec>(R, 2);

	return std::make_tuple(u, v);
}
// ----------------------------------------------------------------------------


/**
 * clear all
 */
void MagDyn::Clear()
{
	ClearAtomSites();
	ClearExchangeTerms();
	ClearExternalField();
}


/**
 * clear all couplings
 */
void MagDyn::ClearExchangeTerms()
{
	m_exchange_terms.clear();
}


/**
 * clear all atom sites
 */
void MagDyn::ClearAtomSites()
{
	m_sites.clear();
	m_sites_calc.clear();
}


/**
 * clear the external field settings
 */
void MagDyn::ClearExternalField()
{
	m_field.dir.clear();
	m_field.mag = 0.;
	m_field.align_spins = false;
}


const std::vector<AtomSite>& MagDyn::GetAtomSites() const
{
	return m_sites;
}


const std::vector<ExchangeTerm>& MagDyn::GetExchangeTerms() const
{
	return m_exchange_terms;
}


const ExternalField& MagDyn::GetExternalField() const
{
	return m_field;
}


void MagDyn::AddAtomSite(AtomSite&& site)
{
	m_sites.emplace_back(std::forward<AtomSite&&>(site));
}


void MagDyn::AddExchangeTerm(ExchangeTerm&& term)
{
	m_exchange_terms.emplace_back(std::forward<ExchangeTerm&&>(term));
}


void MagDyn::AddExchangeTerm(t_size atom1, t_size atom2, const t_vec& dist, const t_cplx& J)
{
	ExchangeTerm term
	{
		.atom1 = atom1, // index of first atom
		.atom2 = atom2, // index of second atom
		.dist = dist,   // distance between the atom's unit cells (not the atoms)
		.J = J,         // interaction
	};

	AddExchangeTerm(std::move(term));
}


void MagDyn::SetExternalField(const ExternalField& field)
{
	m_field = field;
}


void MagDyn::SetBraggPeak(t_real h, t_real k, t_real l)
{
	m_bragg = tl2::create<t_vec>({h, k, l});

	// call CalcSpinRotation() afterwards to calculate projector
}


/**
 * calculate the spin rotation trafo
 */
void MagDyn::CalcSpinRotation()
{
	const std::size_t num_sites = m_sites.size();
	if(num_sites == 0)
		return;

	const t_vec_real zdir = tl2::create<t_vec_real>({0., 0., 1.});

	m_sites_calc.clear();
	m_sites_calc.reserve(num_sites);

	bool use_field = !tl2::equals_0<t_real>(m_field.mag, m_eps)
		&& m_field.dir.size() >= 3;

	if(use_field)
	{
		// rotate field to [001] direction
		auto [field_re, field_im] =
			tl2::split_cplx<t_vec, t_vec_real>(m_field.dir);
		m_rot_field = tl2::convert<t_mat>(
			tl2::rotation<t_mat_real, t_vec_real>(
				field_re, zdir, zdir));
	}

	if(m_bragg.size() == 3)
	{
		// calculate orthogonal projector for magnetic neutron scattering
		t_vec bragg_rot = use_field ? m_rot_field * m_bragg : m_bragg;

		m_proj_neutron = tl2::ortho_projector<t_mat, t_vec>(
			bragg_rot, false);
	}
	else
	{
		// no bragg peak given -> don't project
		m_proj_neutron = tl2::unit<t_mat>(3);
	}


	for(const AtomSite& site : m_sites)
	{
		// rotate local spin to ferromagnetic [001] direction
		auto [spin_re, spin_im] =
			tl2::split_cplx<t_vec, t_vec_real>(site.spin_dir);
		t_mat rot = tl2::convert<t_mat>(
			tl2::rotation<t_mat_real, t_vec_real>(
				spin_re, zdir, zdir));

		// spin rotation of formula 9 from (Toth 2015)
		t_vec u, v;

		if(m_field.align_spins)
			std::tie(u, v) = R_to_uv(m_rot_field);
		else
			std::tie(u, v) = R_to_uv(rot);

		AtomSiteCalc site_calc{};
		site_calc.u_conj = std::move(tl2::conj(u));
		site_calc.u = std::move(u);
		site_calc.v = std::move(v);
		m_sites_calc.emplace_back(std::move(site_calc));
	}
}


/**
 * get the hamiltonian at the given momentum
 * (CalcSpinRotation() needs to be called once before this function)
 * @note implements the formalism given by (Toth 2015)
 * @note a first version for a simplified ferromagnetic dispersion was based on (Heinsdorf 2021)
 */
t_mat MagDyn::GetHamiltonian(t_real _h, t_real _k, t_real _l) const
{
	const std::size_t num_sites = m_sites.size();
	if(num_sites == 0)
		return {};

	// momentum
	const t_vec Q = tl2::create<t_vec>({_h, _k, _l});

	// constants: imaginary unit and 2pi
	constexpr const t_cplx imag{0., 1.};
	constexpr const t_real twopi = t_real(2)*tl2::pi<t_real>;
	// bohr magneton in [meV/T]
	constexpr const t_real muB = tl2::muB<t_real>
		/ tl2::meV<t_real> * tl2::tesla<t_real>;

	// build the interaction matrices J(Q) and J(-Q) of
	// formulas 12 and 14 from (Toth 2015)
	t_mat J_Q = tl2::zero<t_mat>(num_sites*3, num_sites*3);
	t_mat J_Q0 = tl2::zero<t_mat>(num_sites*3, num_sites*3);
	for(const ExchangeTerm& term : m_exchange_terms)
	{
		if(term.atom1 >= num_sites || term.atom2 >= num_sites)
			continue;

		// exchange interaction matrix with dmi as anti-symmetric part,
		// see (Toth 2015) p. 2
		t_mat J = tl2::diag<t_mat>(
			tl2::create<t_vec>({term.J, term.J, term.J}));

		if(term.dmi.size() >= 3)
		{
			// cross product matrix
			J += tl2::skewsymmetric<t_mat, t_vec>(-term.dmi);
		}

		t_mat J_T = tl2::trans(J);

		t_cplx phase_Q = std::exp(-imag * twopi*tl2::inner<t_vec>(term.dist, Q));
		t_cplx phase_mQ = std::exp(-imag * twopi*tl2::inner<t_vec>(term.dist, -Q));

		t_real factor = 1.; //0.5;
		add_submat<t_mat>(J_Q, factor*J*phase_Q, term.atom1*3, term.atom2*3);
		add_submat<t_mat>(J_Q, factor*J_T*phase_mQ, term.atom2*3, term.atom1*3);

		add_submat<t_mat>(J_Q0, factor*J, term.atom1*3, term.atom2*3);
		add_submat<t_mat>(J_Q0, factor*J_T, term.atom2*3, term.atom1*3);
	}


	// create the hamiltonian of formula 25 and 26 from (Toth 2015)
	t_mat A = tl2::create<t_mat>(num_sites, num_sites);
	t_mat B = tl2::create<t_mat>(num_sites, num_sites);
	t_mat C = tl2::zero<t_mat>(num_sites, num_sites);

	bool use_field = !tl2::equals_0<t_real>(m_field.mag, m_eps)
		&& m_field.dir.size() >= 3;

	for(std::size_t i=0; i<num_sites; ++i)
	{
		for(std::size_t j=0; j<num_sites; ++j)
		{
			t_mat J_sub_Q = submat<t_mat>(J_Q, i*3, j*3, 3, 3);

			// TODO: check units of S_i and S_j
			t_real S_i = m_sites[i].spin_mag;
			t_real S_j = m_sites[j].spin_mag;
			const t_vec& u_i = m_sites_calc[i].u;
			const t_vec& u_j = m_sites_calc[j].u;
			const t_vec& u_conj_j = m_sites_calc[j].u_conj;
			const t_vec& v_i = m_sites_calc[i].v;

			t_real factor = 0.5 * std::sqrt(S_i*S_j);
			A(i, j) = factor *
				tl2::inner_noconj<t_vec>(u_i, J_sub_Q * u_conj_j);
			B(i, j) = factor *
				tl2::inner_noconj<t_vec>(u_i, J_sub_Q * u_j);

			if(i == j)
			{
				for(std::size_t k=0; k<num_sites; ++k)
				{
					// TODO: check unit of S_k
					t_real S_k = m_sites[k].spin_mag;
					const t_vec& v_k = m_sites_calc[k].v;

					t_mat J_sub_Q0 = submat<t_mat>(J_Q0, i*3, k*3, 3, 3);
					C(i, j) += S_k * tl2::inner_noconj<t_vec>(v_i, J_sub_Q0 * v_k);
				}
			}

			// include external field, formula 28 from (Toth 2015)
			if(use_field && i == j)
			{
				t_vec B = m_field.dir / tl2::norm<t_vec>(m_field.dir);
				B = B * m_field.mag;

				t_vec gv = m_sites[i].g * v_i;
				t_cplx Bgv = tl2::inner_noconj<t_vec>(B, gv);

				A(i, j) -= 0.5*muB * Bgv;
			}
		}
	}


	// test matrix block
	//return A_conj - C;

	t_mat H = tl2::zero<t_mat>(num_sites*2, num_sites*2);
	set_submat(H, A - C, 0, 0);
	set_submat(H, B, 0, num_sites);
	set_submat(H, tl2::herm(B), num_sites, 0);
	set_submat(H, tl2::conj(A) - C, num_sites, num_sites);

	return H;
}


/**
 * get the energies and the spin-correlation at the given momentum
 * @note implements the formalism given by (Toth 2015)
 * @note a first version for a simplified ferromagnetic dispersion was based on (Heinsdorf 2021)
 */
std::tuple<std::vector<t_real>, t_mat>
MagDyn::GetEnergies(t_real h, t_real k, t_real l, bool only_energies) const
{
	t_mat H = GetHamiltonian(h, k, l);
	return GetEnergies(H, h, k, l, only_energies);
}


/**
 * get the energies and the spin-correlation at the given momentum
 * @note implements the formalism given by (Toth 2015)
 * @note a first version for a simplified ferromagnetic dispersion was based on (Heinsdorf 2021)
 */
std::tuple<std::vector<t_real>, t_mat>
MagDyn::GetEnergies(t_mat _H, t_real h, t_real k, t_real l, bool only_energies) const
{
	const std::size_t N = _H.size1();
	const std::size_t num_sites = m_sites.size();

	// constants: imaginary unit and 2pi
	constexpr const t_cplx imag{0., 1.};
	constexpr const t_real twopi = t_real(2)*tl2::pi<t_real>;

	// formula 30 in (Toth 2015)
	t_mat g = tl2::zero<t_mat>(N, N);
	for(std::size_t i=0; i<N/2; ++i)
		g(i, i) = 1.;
	for(std::size_t i=N/2; i<N; ++i)
		g(i, i) = -1.;

	// formula 31 in (Toth 2015)
	t_mat C;
	for(std::size_t retry=0; retry<m_retries_chol; ++retry)
	{
		auto [chol_ok, _C] = tl2_la::chol<t_mat>(_H);

		if(chol_ok)
		{
			C = _C;
			break;
		}
		else
		{
			// try forcing the hamilton to be positive definite
			for(std::size_t i=0; i<N; ++i)
				_H(i, i) += m_eps_chol;
		}

		if(!chol_ok && retry == m_retries_chol-1)
		{
			std::cerr << "Warning: Cholesky decomposition failed"
				<< " for Q = (" << h << ", " << k << ", " << l << ")"
				<< "." << std::endl;
			C = _C;
		}
	}

	t_mat Ch = tl2::herm<t_mat>(C);

	// see p. 5 in (Toth 2015)
	t_mat H = C * g * Ch;

	bool is_herm = tl2::is_symm_or_herm<t_mat, t_real>(H, m_eps);
	if(!is_herm)
	{
		std::cerr << "Warning: Hamiltonian is not hermitian"
			<< " for Q = (" << h << ", " << k << ", " << l << ")"
			<< "." << std::endl;
	}

	// eigenvalues of the hamiltonian correspond to the energies
	// eigenvectors correspond to the spectral weights
	auto [evecs_ok, evals, evecs] =
		tl2_la::eigenvec<t_mat, t_vec, t_cplx, t_real>(
			H, only_energies, is_herm);
	if(!evecs_ok)
	{
		std::cerr << "Warning: Eigensystem calculation failed"
			<< " for Q = (" << h << ", " << k << ", " << l << ")"
			<< "." << std::endl;
	}


	std::vector<t_real> energies;
	energies.reserve(evals.size());

	// if we're not interested in the spectral weights, we can ignore duplicates
	bool remove_duplicates = only_energies;
	for(const auto& eval : evals)
	{
		if(remove_duplicates &&
			std::find_if(energies.begin(), energies.end(), [&eval, this](t_real val) -> bool
			{
				return tl2::equals<t_real>(val, eval.real(), m_eps);
			}) != energies.end())
		{
			continue;
		}

		//if(std::abs(eval.imag()) <= m_eps)
			energies.push_back(eval.real());
	}


	// spectral weights
	t_mat S = tl2::zero<t_mat>(3, 3);

	if(!only_energies)
	{
		// momentum
		const t_vec Q = tl2::create<t_vec>({h, k, l});

		// get the sorting of the energies
		std::vector<std::size_t> sorting(energies.size());
		std::iota(sorting.begin(), sorting.end(), 0);

		std::stable_sort(sorting.begin(), sorting.end(),
			[&energies](std::size_t idx1, std::size_t idx2) -> bool
			{
				return energies[idx1] >= energies[idx2];
			});

		energies = tl2::reorder(energies, sorting);
		evecs = tl2::reorder(evecs, sorting);
		evals = tl2::reorder(evals, sorting);

		t_mat evec_mat = tl2::create<t_mat>(evecs);
		t_mat evec_mat_herm = tl2::herm(evec_mat);

		// formula 32 in (Toth 2015)
		t_mat L = evec_mat_herm * H * evec_mat;
		t_mat E = g*L;

		// re-create energies, to be consistent with the weights
		energies.clear();
		for(std::size_t i=0; i<L.size1(); ++i)
			energies.push_back(L(i,i).real());

		t_mat E_sqrt = E;
		for(std::size_t i=0; i<E.size1(); ++i)
			E_sqrt(i, i) = std::sqrt(std::abs(E_sqrt(i, i)));

		auto [C_inv, inv_ok] = tl2::inv(C);
		if(!inv_ok)
		{
			std::cerr << "Warning: Inversion failed"
				<< " for Q = (" << h << ", " << k << ", " << l << ")"
				<< "." << std::endl;
		}

		// formula 34 in (Toth 2015)
		t_mat trafo = C_inv * evec_mat * E_sqrt;
		t_mat trafo_herm = tl2::herm(trafo);
		/*using namespace tl2_ops;
		tl2::set_eps_0<t_mat, t_real>(trafo, m_eps);
		std::cout << "Trafo: " << trafo << " for Q = (" << h << ", " << k << ", " << l << ")." << std::endl;*/


		// building the spin correlation function of formula 47 in (Toth 2015)
		for(int x_idx=0; x_idx<3; ++x_idx)
		{
			for(int y_idx=0; y_idx<3; ++y_idx)
			{
				// formulas 44 in (Toth 2015)
				t_mat V = tl2::create<t_mat>(num_sites, num_sites);
				t_mat W = tl2::create<t_mat>(num_sites, num_sites);
				t_mat Y = tl2::create<t_mat>(num_sites, num_sites);
				t_mat Z = tl2::create<t_mat>(num_sites, num_sites);

				for(std::size_t i=0; i<num_sites; ++i)
				{
					for(std::size_t j=0; j<num_sites; ++j)
					{
						const t_vec& pos_i = m_sites[i].pos;
						const t_vec& pos_j = m_sites[j].pos;
						t_real S_i = m_sites[i].spin_mag;
						t_real S_j = m_sites[j].spin_mag;

						const t_vec& u_i = m_sites_calc[i].u;
						const t_vec& u_j = m_sites_calc[j].u;
						const t_vec& u_conj_i = m_sites_calc[i].u_conj;
						const t_vec& u_conj_j = m_sites_calc[j].u_conj;

						t_cplx phase = std::sqrt(S_i*S_j) * std::exp(
							imag * twopi *
							tl2::inner<t_vec>(
								(pos_i - pos_j), Q));

						V(i, j) = phase * u_conj_i[x_idx] * u_conj_j[y_idx];
						W(i, j) = phase * u_conj_i[x_idx] * u_j[y_idx];
						Y(i, j) = phase * u_i[x_idx] * u_conj_j[y_idx];
						Z(i, j) = phase * u_i[x_idx] * u_j[y_idx];
					}
				}

				// formula 47 in (Toth 2015)
				t_mat M = tl2::create<t_mat>(num_sites*2, num_sites*2);
				set_submat(M, Y, 0, 0);
				set_submat(M, V, num_sites, 0);
				set_submat(M, Z, 0, num_sites);
				set_submat(M, W, num_sites, num_sites);

				t_mat M_trafo = trafo_herm * M * trafo;

				for(std::size_t i=0; i<M_trafo.size1(); ++i)
					S(x_idx, y_idx) += M_trafo(i, i);
				S(x_idx, y_idx) /= t_real(M_trafo.size1());

				/*using namespace tl2_ops;
				tl2::set_eps_0<t_mat, t_real>(V, m_eps);
				tl2::set_eps_0<t_mat, t_real>(W, m_eps);
				tl2::set_eps_0<t_mat, t_real>(Y, m_eps);
				tl2::set_eps_0<t_mat, t_real>(Z, m_eps);
				std::cout << "x_idx=" << x_idx << ", yidx=" << y_idx;
				std::cout << ", Q = (" << h << ", " << k << ", " << l << ")." << std::endl;
				std::cout << "V=" << V << std::endl;
				std::cout << "W=" << W << std::endl;
				std::cout << "Y=" << Y << std::endl;
				std::cout << "Z=" << Z << std::endl;*/
			}
		}
	}

	return std::make_tuple(energies, S);
}


/**
 * get the energy of the goldstone mode
 * (formulas based on calculations and notes provided by N. Heinsdorf, personal communication, 2021)
 */
t_real MagDyn::GetGoldstoneEnergy() const
{
	auto [Es, S] = GetEnergies(0., 0., 0., true);
	if(auto min_iter = std::min_element(Es.begin(), Es.end()); min_iter != Es.end())
		return *min_iter;

	return 0.;
}


/**
 * generates the dispersion plot along the given q path
 */
void MagDyn::SaveDispersion(const std::string& filename,
	t_real h_start, t_real k_start, t_real l_start,
	t_real h_end, t_real k_end, t_real l_end,
	t_size num_qs) const
{
	std::ofstream ofstr{filename};
	ofstr.precision(m_prec);

	ofstr << std::setw(m_prec*2) << std::left << "# h"
		<< std::setw(m_prec*2) << std::left << "k"
		<< std::setw(m_prec*2) << std::left << "l"
		<< std::setw(m_prec*2) << std::left << "energies"
		<< "\n";

	for(t_size i=0; i<num_qs; ++i)
	{
		t_real h = std::lerp(h_start, h_end, t_real(i)/t_real(num_qs-1));
		t_real k = std::lerp(k_start, k_end, t_real(i)/t_real(num_qs-1));
		t_real l = std::lerp(l_start, l_end, t_real(i)/t_real(num_qs-1));


		auto [Es, S] = GetEnergies(h, k, l, true);
		for(const auto& E : Es)
		{
			ofstr
				<< std::setw(m_prec*2) << std::left << h
				<< std::setw(m_prec*2) << std::left << k
				<< std::setw(m_prec*2) << std::left << l
				<< std::setw(m_prec*2) << E
				<< std::endl;
		}
	}
}


/**
 * load a configuration from a property tree
 */
bool MagDyn::Load(const pt::ptree& node)
{
	Clear();

	// atom sites
	if(auto sites = node.get_child_optional("atom_sites"); sites)
	{
		for(const auto &site : *sites)
		{
			AtomSite atom_site;

			atom_site.name = site.second.get<std::string>("name", "n/a");

			atom_site.pos = tl2::create<t_vec>(
			{
				site.second.get<t_real>("position_x", 0.),
				site.second.get<t_real>("position_y", 0.),
				site.second.get<t_real>("position_z", 0.),
			});

			atom_site.spin_dir = tl2::create<t_vec>(
			{
				site.second.get<t_real>("spin_x", 0.),
				site.second.get<t_real>("spin_y", 0.),
				site.second.get<t_real>("spin_z", 1.),
			});

			atom_site.spin_mag = site.second.get<t_real>("spin_magnitude", 1.);
			atom_site.g = -2. * tl2::unit<t_mat>(3);

			m_sites.emplace_back(std::move(atom_site));
		}
	}

	// exchange terms
	if(auto terms = node.get_child_optional("exchange_terms"); terms)
	{
		for(const auto &term : *terms)
		{
			ExchangeTerm exchange_term;

			exchange_term.name = term.second.get<std::string>("name", "n/a");
			exchange_term.atom1 = term.second.get<t_size>("atom_1_index", 0);
			exchange_term.atom2 = term.second.get<t_size>("atom_2_index", 0);

			exchange_term.dist = tl2::create<t_vec>(
			{
				term.second.get<t_real>("distance_x", 0.),
				term.second.get<t_real>("distance_y", 0.),
				term.second.get<t_real>("distance_z", 0.),
			});

			exchange_term.J = term.second.get<t_real>("interaction", 0.);

			exchange_term.dmi = tl2::create<t_vec>(
			{
				term.second.get<t_real>("dmi_x", 0.),
				term.second.get<t_real>("dmi_y", 0.),
				term.second.get<t_real>("dmi_z", 0.),
			});

			m_exchange_terms.emplace_back(std::move(exchange_term));
		}
	}

	// external field
	if(auto field = node.get_child_optional("field"); field)
	{
		m_field.dir = tl2::zero<t_vec>(3);
		m_field.mag = 0.;
		m_field.align_spins = false;

		m_field.dir = tl2::create<t_vec>(
		{
			field->get<t_real>("direction_h", 0.),
			field->get<t_real>("direction_k", 0.),
			field->get<t_real>("direction_l", 1.),
		});

		if(auto optVal = field->get_optional<t_real>("magnitude"))
			m_field.mag = *optVal;

		if(auto optVal = field->get_optional<bool>("align_spins"))
			m_field.align_spins = *optVal;
	}

	CalcSpinRotation();
	return true;
}


/**
 * save a configuration to a property tree
 */
bool MagDyn::Save(pt::ptree& node)
{
	// external field
	node.put<t_real>("field.direction_h", m_field.dir[0].real());
	node.put<t_real>("field.direction_k", m_field.dir[1].real());
	node.put<t_real>("field.direction_l", m_field.dir[2].real());
	node.put<t_real>("field.magnitude", m_field.mag);
	node.put<bool>("field.align_spins", m_field.align_spins);

	// atom sites
	for(const auto& site : GetAtomSites())
	{
		pt::ptree itemNode;
		itemNode.put<std::string>("name", site.name);
		itemNode.put<t_real>("position_x", site.pos[0].real());
		itemNode.put<t_real>("position_y", site.pos[1].real());
		itemNode.put<t_real>("position_z", site.pos[2].real());
		itemNode.put<t_real>("spin_x", site.spin_dir[0].real());
		itemNode.put<t_real>("spin_y", site.spin_dir[1].real());
		itemNode.put<t_real>("spin_z", site.spin_dir[2].real());
		itemNode.put<t_real>("spin_magnitude", site.spin_mag);

		node.add_child("atom_sites.site", itemNode);
	}

	// exchange terms
	for(const auto& term : GetExchangeTerms())
	{
		pt::ptree itemNode;
		itemNode.put<std::string>("name", term.name);
		itemNode.put<t_size>("atom_1_index", term.atom1);
		itemNode.put<t_size>("atom_2_index", term.atom2);
		itemNode.put<t_real>("distance_x", term.dist[0].real());
		itemNode.put<t_real>("distance_y", term.dist[1].real());
		itemNode.put<t_real>("distance_z", term.dist[2].real());
		itemNode.put<t_real>("interaction", term.J.real());
		itemNode.put<t_real>("dmi_x", term.dmi[0].real());
		itemNode.put<t_real>("dmi_y", term.dmi[1].real());
		itemNode.put<t_real>("dmi_z", term.dmi[2].real());

		node.add_child("exchange_terms.term", itemNode);
	}

	return true;
}
