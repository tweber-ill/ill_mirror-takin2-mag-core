/**
 * cso magnon dynamics
 * @author Tobias Weber <tweber@ill.fr>
 * @date jan-2022
 * @license GPLv3
 */

#ifndef __MAGDYN_H__
#define __MAGDYN_H__

#include <vector>
#include <string>

#include "defs.h"


struct ExchangeTerm
{
	t_size atom1{};
	t_size atom2{};
	t_vec dist{};
	t_cplx J{};
};


class MagDyn
{
public:
	MagDyn() = default;
	~MagDyn() = default;

	void SetNumCells(std::size_t n) { m_num_cells = n; }

	void ClearExchangeTerms();

	void AddExchangeTerm(ExchangeTerm&& term);
	void AddExchangeTerm(t_size atom1, t_size atom2, const t_vec& cell, const t_cplx& J);
	std::vector<t_real> GetEnergies(t_real h, t_real k, t_real l) const;
	t_real GetGoldstoneEnergy() const;

	void SaveDispersion(const std::string& filename,
		t_real h_start, t_real k_start, t_real l_start,
		t_real h_end, t_real k_end, t_real l_end,
		t_size num_qs = 128) const;

	void SetEpsilon(t_real eps) { m_eps = eps; }
	void SetPrecision(int prec) { m_prec = prec; }


private:
	std::size_t m_num_cells{};
	std::vector<ExchangeTerm> m_exchange_terms{};

	t_real m_eps = 1e-6;
	int m_prec = 6;
};


#endif
