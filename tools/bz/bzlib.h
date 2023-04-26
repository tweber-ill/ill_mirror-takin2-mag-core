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
//requires tl2::is_mat<t_mat> && tl2::is_vec<t_vec>
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

	/**
	 * sets up a crystal lattice and angles
	 */
	void SetCrystal(t_real a, t_real b, t_real c,
		t_real alpha = 90., t_real beta = 90., t_real gamma = 90.)
	{
		t_mat crystB = tl2::B_matrix<t_mat>(a, b, c,
			alpha/180.*tl2::pi<t_real>,
			beta/180.*tl2::pi<t_real>,
			gamma/180.*tl2::pi<t_real>);

		SetCrystalB(crystB);
	}

	void SetPeaks(const std::vector<t_vec>& peaks) { m_peaks = peaks; }
	const std::vector<t_vec>& GetPeaks() const { return m_peaks; }

	void SetPeaksInvA(const std::vector<t_vec>& peaks) { m_peaks_invA = peaks; }
	const std::vector<t_vec>& GetPeaksInvA() const { return m_peaks_invA; }

	const std::vector<t_vec>& GetVertices() const { return m_vertices; }

	const std::vector<std::vector<t_vec>>& GetTriangles() const { return m_triags; }
	const std::vector<std::vector<std::size_t>>& GetTrianglesIndices() const { return m_triags_idx; }

	const std::vector<t_vec>& GetAllTriangles() const { return m_all_triags; }
	const std::vector<std::size_t>& GetAllTrianglesIndices() const { return m_all_triags_idx; }


	/**
	 * get the index of the (000) bragg peak
	 */
	std::size_t Get000Peak() const
	{
		if(m_idx000)
			return *m_idx000;
		return s_erridx;
	}


	/**
	 * set up a list of symmetry operations (given by the space group)
	 * @returns number of actually set symops
	 */
	std::size_t SetSymOps(const std::vector<t_mat>& ops, bool are_centring = false)
	{
		if(are_centring)
		{
			// symops are already purely centring, add all
			m_symops = ops;
		}
		else
		{
			// only add centring ops
			m_symops.clear();
			m_symops.reserve(ops.size());

			for(const t_mat& op : ops)
			{
				if(!tl2::hom_is_centring<t_mat>(op, m_eps))
					continue;

				m_symops.push_back(op);
			}
		}

		return m_symops.size();
	}


	std::size_t SetSymOpsFromSpaceGroup(const std::string& sgname)
	{
		std::vector<t_mat> ops = get_sg_ops<t_mat, t_real>(sgname);
		return SetSymOps(ops, false);
	}
	// --------------------------------------------------------------------------------


	// --------------------------------------------------------------------------------
	// calculations
	// --------------------------------------------------------------------------------
	/**
	 * calculate the nuclear bragg peaks in lab coordinates
	 * @returns number of created peaks
	 */
	std::size_t CalcPeaksInvA()
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

		return m_peaks_invA.size();
	}


	/**
	 * create nuclear bragg peaks up to the given order
	 * @returns number of created peaks
	 */
	std::size_t CalcPeaks(int order, bool cleate_invA = false)
	{
		m_peaks.clear();
		m_peaks.reserve((2*order+1)*(2*order+1)*(2*order+1));

		for(int h=-order; h<=order; ++h)
		{
			for(int k=-order; k<=order; ++k)
			{
				for(int l=-order; l<=order; ++l)
				{
					m_peaks.emplace_back(
						tl2::create<t_vec>(
							{ t_real(h), t_real(k), t_real(l) }));
				}
			}
		}

		if(cleate_invA)
			CalcPeaksInvA();

		return m_peaks.size();
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
	bool CalcBZ()
	{
		ClearBZ();

		if(!m_idx000)
			Calc000Peak();

		// calculate the voronoi diagram's vertices
		std::tie(m_vertices, std::ignore, std::ignore) =
			geo::calc_delaunay(3, m_peaks_invA, false, false, m_idx000);
		m_vertices = tl2::remove_duplicates(m_vertices, m_eps);
		if(!m_vertices.size())
			return false;

		for(t_vec& vertex : m_vertices)
			tl2::set_eps_0(vertex, m_eps);

		// calculate the faces of the BZ
		std::tie(std::ignore, m_triags, std::ignore) =
			geo::calc_delaunay(3, m_vertices, true, false);
		if(!m_triags.size())
			return false;

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

		return true;
	}
	// --------------------------------------------------------------------------------


	// --------------------------------------------------------------------------------
	// output
	// --------------------------------------------------------------------------------
	/**
	 * print a description of the bz
	 */
	std::string Print(int prec = 6) const
	{
		std::ostringstream ostr;
		ostr.precision(prec);

#ifdef DEBUG
		ostr << "# centring symmetry operations\n";
		for(const t_mat& op : m_symops)
			ostr << op << "\n";
#endif

		// voronoi vertices forming the vertices of the bz
		const std::vector<t_vec>& voronoiverts = GetVertices();
		ostr << "# Brillouin zone vertices\n";
		for(std::size_t idx=0; idx<voronoiverts.size(); ++idx)
		{
			const t_vec& voro = voronoiverts[idx];
			ostr << "vertex " << idx << ": (" << voro << ")\n";
		}

		// voronoi bisectors
		const auto& bz_polys = GetTriangles();
		const auto& bz_polys_idx = GetTrianglesIndices();

		ostr << "\n# Brillouin zone polygons\n";
		for(std::size_t idx_triag=0; idx_triag<bz_polys.size(); ++idx_triag)
		{
			const auto& triag = bz_polys[idx_triag];
			const auto& triag_idx = bz_polys_idx[idx_triag];

			ostr << "polygon " << idx_triag << ": \n";
			for(std::size_t idx_vert=0; idx_vert<triag.size(); ++idx_vert)
			{
				const t_vec& vert = triag[idx_vert];
				std::size_t voroidx = triag_idx[idx_vert];

				ostr << "\tvertex " << voroidx << ": (" << vert << ")\n";
			}
		}

		return ostr.str();
	}


	/**
	 * export a description of the bz in json format
	 */
	std::string PrintJSON(int prec = 6) const
	{
		std::ostringstream ostr;
		ostr.precision(prec);

		ostr << "{\n";

		// voronoi vertices forming the vertices of the bz
		const std::vector<t_vec>& voronoiverts = GetVertices();
		ostr << "\"vertices\" : [\n";
		for(std::size_t idx=0; idx<voronoiverts.size(); ++idx)
		{
			const t_vec& voro = voronoiverts[idx];
			ostr << "\t[ " << voro[0] << ", " << voro[1] << ", " << voro[2] << " ]";
			if(idx < voronoiverts.size() - 1)
				ostr << ",";
			ostr << "\n";
		}
		ostr << "],\n\n";

		// voronoi bisectors
		const auto& bz_polys_idx = GetTrianglesIndices();
		ostr << "\"polygons\" : [\n";
		for(std::size_t idx_triag=0; idx_triag<bz_polys_idx.size(); ++idx_triag)
		{
			const auto& triag_idx = bz_polys_idx[idx_triag];

			ostr << "\t[ ";
			for(std::size_t idx_vert=0; idx_vert<triag_idx.size(); ++idx_vert)
			{
				ostr << triag_idx[idx_vert];
				if(idx_vert < triag_idx.size() - 1)
					ostr << ", ";
			}
			ostr << " ]";
			if(idx_triag < bz_polys_idx.size() - 1)
				ostr << ",";
			ostr << "\n";
		}
		ostr << "]\n";

		ostr << "}\n";
		return ostr.str();
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
