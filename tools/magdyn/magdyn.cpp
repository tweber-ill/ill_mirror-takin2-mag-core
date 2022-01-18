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
#include <iostream>
#include <fstream>
#include <iomanip>

#include "magdyn.h"
#include "tlibs2/libs/units.h"



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



void MagDyn::ClearExchangeTerms()
{
	m_exchange_terms.clear();
}


void MagDyn::ClearAtomSites()
{
	m_sites.clear();
}


void MagDyn::ClearExternalField()
{
	m_field.dir.clear();
	m_field.mag = 0.;
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


/**
 * get the hamiltonian at the given momentum
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
	const t_vec_real zdir = tl2::create<t_vec_real>({0., 0., 1.});

	std::vector<t_vec> us, us_conj, vs;
	us.reserve(num_sites);
	us_conj.reserve(num_sites);
	vs.reserve(num_sites);

	for(const AtomSite& site : m_sites)
	{
		// rotate spin to ferromagnetic [001] direction
		auto [spin_re, spin_im] = tl2::split_cplx<t_vec, t_vec_real>(site.spin);
		t_mat rot = tl2::convert<t_mat>(
			tl2::rotation<t_mat_real, t_vec_real>(
				spin_re, zdir, zdir));

		// spin rotation of formula 9 from (Toth 2015)
		auto [u, v] = R_to_uv(rot);
		t_vec u_conj = tl2::conj(u);
		us.emplace_back(std::move(u));
		vs.emplace_back(std::move(v));
		us_conj.emplace_back(std::move(u_conj));
	}


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
			t_real S_i = 1.;
			t_real S_j = 1.;
			t_real factor = 0.5 * std::sqrt(S_i*S_j);
			A(i, j) = factor *
				tl2::inner_noconj<t_vec>(us[i], J_sub_Q * us_conj[j]);
			B(i, j) = factor *
				tl2::inner_noconj<t_vec>(us[i], J_sub_Q * us[j]);

			if(i == j)
			{
				for(std::size_t k=0; k<num_sites; ++k)
				{
					// TODO: check unit of S_k
					t_real S_k = 1.;
					t_mat J_sub_Q0 = submat<t_mat>(J_Q0, i*3, k*3, 3, 3);
					C(i, j) += S_k * tl2::inner_noconj<t_vec>(vs[i], J_sub_Q0 * vs[k]);
				}
			}

			// include external field, formula 28 from (Toth 2015)
			if(use_field && i == j)
			{
				t_vec B = m_field.dir / tl2::norm<t_vec>(m_field.dir);
				B = B * m_field.mag;

				t_vec gv = m_sites[i].g * vs[i];
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
 * get the energies at the given momentum
 * @note implements the formalism given by (Toth 2015)
 * @note a first version for a simplified ferromagnetic dispersion was based on (Heinsdorf 2021)
 */
std::vector<t_real> MagDyn::GetEnergies(t_real h, t_real k, t_real l) const
{
	t_mat _H = GetHamiltonian(h, k, l);
	const std::size_t N = _H.size1();

	// formula 30 in (Toth 2015)
	t_mat g = tl2::zero<t_mat>(N, N);
	for(std::size_t i=0; i<N/2; ++i)
		g(i, i) = 1.;
	for(std::size_t i=N/2; i<N; ++i)
		g(i, i) = -1.;

	// formula 31 in (Toth 2015)
	auto [chol_ok, C] = tl2_la::chol<t_mat>(_H);
	t_mat Ch = tl2::herm<t_mat>(C);

	// see p. 5 in (Toth 2015)
	t_mat H = C * g * Ch;

	bool is_herm = tl2::is_symm_or_herm<t_mat, t_real>(H, m_eps);
	if(!is_herm)
		std::cerr << "Warning: Hamiltonian is not hermitian." << std::endl;

	// eigenvalues of the hamiltonian correspond to the energies
	// eigenvectors correspond to the spectral weights
	bool only_evals = true;
	auto [ok, evals, evecs] =
		tl2_la::eigenvec<t_mat, t_vec, t_cplx, t_real>(
			H, only_evals, is_herm);

	std::vector<t_real> energies;
	energies.reserve(evals.size());

	bool remove_duplicates = true;
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

	return energies;
}


/**
 * get the energy of the goldstone mode
 * (formulas based on calculations and notes provided by N. Heinsdorf, personal communication, 2021)
 */
t_real MagDyn::GetGoldstoneEnergy() const
{
	std::vector<t_real> Es = GetEnergies(0., 0., 0.);
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


		auto Es = GetEnergies(h, k, l);
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
