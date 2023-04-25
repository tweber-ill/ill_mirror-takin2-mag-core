/**
 * brillouin zone calculation library
 * @author Tobias Weber <tweber@ill.fr>
 * @date May-2022
 * @license GPLv3, see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2023  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 * "misc" project
 * Copyright (C) 2017-2021  Tobias WEBER (privately developed).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ----------------------------------------------------------------------------
 */


#ifndef __TAKIN_BZLIB_H__
#define __TAKIN_BZLIB_H__

#include <vector>
#include <optional>

#include "tlibs2/libs/maths.h"
#include "../structfact/loadcif.h"

#if __has_include("pathslib/libs/voronoi.h")
	#include "pathslib/libs/voronoi.h"
#else
	#include "voronoi.h"
#endif


/**
 * brillouin zone calculation
 */
template<class t_mat, class t_vec, class t_real = typename t_vec::value_type>
class BZCalc
{
public:
	BZCalc() = default;
	~BZCalc() = default;


	/**
	 * clear old BZ results
	 */
	void ClearBZ()
	{
		m_vertices.clear();

		m_triags.clear();
		m_triags_idx.clear();

		m_all_triags.clear();
		m_all_triags_idx.clear();
	}


	// --------------------------------------------------------------------------------
	// getter and setter
	// --------------------------------------------------------------------------------
	static std::size_t GetErrIdx() { return s_erridx; }

	void SetEps(t_real eps) { m_eps = eps; }
	void SetCrystalB(const t_mat& B) { m_crystB = B; }

	void SetPeaks(const std::vector<t_vec>& peaks) { m_peaks = peaks; }
	const std::vector<t_vec>& GetPeaks() const { return m_peaks; }

	void SetPeaksInvA(const std::vector<t_vec>& peaks) { m_peaks_invA = peaks; }
	const std::vector<t_vec>& GetPeaksInvA() const { return m_peaks_invA; }

	const std::vector<t_vec>& GetVertices() const { return m_vertices; }

	const std::vector<std::vector<t_vec>>& GetTriangles() const { return m_triags; }
	const std::vector<std::vector<std::size_t>>& GetTrianglesIndices() const { return m_triags_idx; }

	const std::vector<t_vec>& GetAllTriangles() const { return m_all_triags; }
	const std::vector<std::size_t>& GetAllTrianglesIndices() const { return m_all_triags_idx; }


	std::size_t Get000Peak() const
	{
		if(m_idx000)
			return *m_idx000;
		return s_erridx;
	}


	void SetSymOps(const std::vector<t_mat>& ops, bool are_centring = false)
	{
		if(are_centring)
		{
			// symops are already centring, add all
			m_symops = ops;
		}
		else
		{
			// add only centring ops
			m_symops.clear();
			m_symops.reserve(ops.size());

			for(const t_mat& op : ops)
			{
				if(!tl2::hom_is_centring<t_mat>(op, m_eps))
					continue;

				m_symops.push_back(op);
			}
		}
	}
	// --------------------------------------------------------------------------------


	// --------------------------------------------------------------------------------
	// calculations
	// --------------------------------------------------------------------------------
	/**
	 * calculate the nuclear bragg peaks in lab coordinates
	 */
	void CalcPeaksInvA()
	{
		// calculate the peaks in lab coordinates
		m_peaks_invA.clear();
		m_peaks_invA.reserve(m_peaks.size());

		for(const t_vec& Q : m_peaks)
		{
			if(!is_reflection_allowed<t_mat, t_vec, t_real>(Q, m_symops, m_eps).first)
				continue;

			// also get the index of the (000) peak
			if(tl2::equals_0(Q, m_eps))
				m_idx000 = m_peaks_invA.size();

			m_peaks_invA.emplace_back(m_crystB * Q);
		}
	}


	/**
	 * calculate the index of the nuclear (000) peak
	 */
	void Calc000Peak()
	{
		for(const t_vec& Q : m_peaks)
		{
			if(!tl2::equals_0(Q, m_eps))
				continue;
			m_idx000 = m_peaks_invA.size();
			break;
		}
	}


	/**
	 * calculate the brillouin zone
	 */
	void CalcBZ()
	{
		ClearBZ();

		if(!m_idx000)
			Calc000Peak();

		// calculate the voronoi diagram's vertices
		std::tie(m_vertices, std::ignore, std::ignore) =
			geo::calc_delaunay(3, m_peaks_invA, false, false, m_idx000);
		m_vertices = tl2::remove_duplicates(m_vertices, m_eps);
		for(t_vec& vertex : m_vertices)
			tl2::set_eps_0(vertex, m_eps);

		// calculate the faces of the BZ
		std::tie(std::ignore, m_triags, std::ignore) =
			geo::calc_delaunay(3, m_vertices, true, false);

		// calculate all BZ triangles
		for(std::vector<t_vec>& bz_triag : m_triags)
		{
			if(bz_triag.size() == 0)
				continue;
			//assert(bz_triag.size() == 3);

			std::vector<std::size_t> triagindices;
			for(t_vec& vert : bz_triag)
			{
				tl2::set_eps_0(vert, m_eps);

				// find index of vert among voronoi vertices
				std::ptrdiff_t voroidx = -1;
				if(auto voro_iter = std::find_if(m_vertices.begin(), m_vertices.end(),
					[&vert, this](const t_vec& vec) -> bool
					{
						return tl2::equals<t_vec>(vec, vert, m_eps);
					}); voro_iter != m_vertices.end())
				{
					voroidx = voro_iter - m_vertices.begin();
				}

				std::size_t idx = (voroidx >= 0 ? voroidx : s_erridx);
				triagindices.push_back(idx);

				m_all_triags.push_back(vert);
				m_all_triags_idx.push_back(idx);
			}  // vertices

			m_triags_idx.emplace_back(std::move(triagindices));
		}  // triangles
	}
	// --------------------------------------------------------------------------------


private:
	t_real m_eps{ 1e-6 };                  // calculation epsilon
	t_mat m_crystB{tl2::unit<t_mat>(3)};   // crystal B matrix

	std::vector<t_mat> m_symops{ };        // space group centring symmetry operations
	std::vector<t_vec> m_peaks{ };         // nuclear bragg peaks
	std::vector<t_vec> m_peaks_invA { };   // nuclear bragg peaks in lab coordinates
	std::optional<std::size_t> m_idx000{}; // index of the (000) peak

	std::vector<t_vec> m_vertices{};            // voronoi vertices

	std::vector<std::vector<t_vec>> m_triags{}; // bz triangles
	std::vector<std::vector<std::size_t>> m_triags_idx{}; // ... and the version with voronoi vertex indices

	std::vector<t_vec> m_all_triags {};            // all brillouin zone triangles
	std::vector<std::size_t> m_all_triags_idx {};  // ... and the version with voronoi vertex indices

	static const std::size_t s_erridx{0xffffffff}; // index for reporting errors
};


#endif
