/**
 * cso magnon dynamics
 * @author Tobias Weber <tweber@ill.fr>
 * @date jan-2022
 * @license GPLv3
 *
 * References:
 *   - (Toth 2015) S. Toth and B. Lake, J. Phys.: Condens. Matter 27 166002 (2015):
 *     https://doi.org/10.1088/0953-8984/27/16/166002
 *   - N. Heinsdorf, personal communication, 2021, 2022.
 */

#include <tuple>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>

#include "magdyn.h"



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


/**
 * get the energies at the given momentum
 * (formulas based on calculations and notes provided by N. Heinsdorf, personal communication, 2021)
 */
std::vector<t_real> MagDyn::GetEnergies(t_real _h, t_real _k, t_real _l) const
{
	// momentum
	const t_vec Q = tl2::create<t_vec>({_h, _k, _l});

	// imaginary unit
	const t_cplx imag{0., 1.};
	const t_real twopi = t_real(2)*tl2::pi<t_real>;

	// create hamiltonian of formula 25 from (Toth 2015)
	t_mat H = tl2::zero<t_mat>(m_num_cells, m_num_cells);

	// formulas 12 and 14 from (Toth 2015)
	for(const ExchangeTerm& term : m_exchange_terms)
	{
		t_cplx contrib = term.J * 
			std::exp(-imag * twopi*tl2::inner<t_vec>(term.dist, Q));

		H(term.atom1, term.atom2) += contrib;
		H(term.atom2, term.atom1) += std::conj(contrib);
	}

	// eigenvalues of the hamiltonian correspond to the energies
	// eigenvectors correspond to the spectral weights
	bool only_evals = true;
	bool is_herm = false;
	auto [ok, evals, evecs] =
		tl2_la::eigenvec<t_mat, t_vec, t_cplx, t_real>(H, only_evals, is_herm);

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
