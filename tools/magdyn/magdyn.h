/**
 * magnon dynamics
 * @author Tobias Weber <tweber@ill.fr>
 * @date jan-2022
 * @license GPLv3
 *
 * References:
 *   - (Toth 2015) S. Toth and B. Lake, J. Phys.: Condens. Matter 27 166002 (2015):
 *     https://doi.org/10.1088/0953-8984/27/16/166002
 *   - N. Heinsdorf, personal communication, 2021, 2022.
 */

#ifndef __MAGDYN_H__
#define __MAGDYN_H__

#include <vector>
#include <string>

#include "defs.h"


struct AtomSite
{
	t_vec pos{};       // atom position
	t_vec spin_dir{};  // spin direction
	t_real spin_mag{}; // spin magnitude
	t_mat g{};         // g factor
};


struct AtomSiteCalc
{
	t_vec u{}, u_conj{};
	t_vec v{};
};


struct ExchangeTerm
{
	t_size atom1{};    // atom 1 index
	t_size atom2{};    // atom 2 index
	t_vec dist{};      // distance between unit cells
	t_cplx J{};        // Heisenberg interaction
	t_vec dmi{};       // Dzyaloshinskij-Moriya interaction
};


struct ExternalField
{
	t_vec dir{};       // field direction
	t_real mag{};      // field magnitude
};


class MagDyn
{
public:
	MagDyn() = default;
	~MagDyn() = default;

	void ClearAtomSites();
	void ClearExchangeTerms();
	void ClearExternalField();

	void AddAtomSite(AtomSite&& site);
	void CalcSpinRotation();

	void AddExchangeTerm(ExchangeTerm&& term);
	void AddExchangeTerm(t_size atom1, t_size atom2, const t_vec& cell, const t_cplx& J);

	void SetExternalField(const ExternalField& field);

	t_mat GetHamiltonian(t_real h, t_real k, t_real l) const;

	std::vector<t_real> GetEnergies(t_real h, t_real k, t_real l) const;

	t_real GetGoldstoneEnergy() const;

	void SaveDispersion(const std::string& filename,
		t_real h_start, t_real k_start, t_real l_start,
		t_real h_end, t_real k_end, t_real l_end,
		t_size num_qs = 128) const;

	void SetEpsilon(t_real eps) { m_eps = eps; }
	void SetPrecision(int prec) { m_prec = prec; }


private:
	std::vector<AtomSite> m_sites{};
	std::vector<ExchangeTerm> m_exchange_terms{};
	ExternalField m_field{};

	std::vector<AtomSiteCalc> m_sites_calc{};

	std::size_t m_retries_chol = 10;
	t_real m_eps_chol = 0.05;

	t_real m_eps = 1e-6;
	int m_prec = 6;
};


#endif
