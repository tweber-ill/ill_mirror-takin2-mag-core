/**
 * voronoi diagrams / delaunay triangulation (simplified version)
 * @author Tobias Weber <tweber@ill.fr>
 * @date October/November 2020
 * @note Part of TAS-Paths (https://doi.org/10.5281/zenodo.4625649).
 * @note Forked on 19-apr-2021 and 3-jun-2021 from my privately developed "geo" project (https://github.com/t-weber/geo).
 * @license GPLv3, see 'LICENSE' file
 *
 * References for the algorithms:
 *   - (Klein 2005) R. Klein, "Algorithmische Geometrie" (2005),
 *                  ISBN: 978-3540209560 (http://dx.doi.org/10.1007/3-540-27619-X).
 *   - (FUH 2020) R. Klein, C. Icking, "Algorithmische Geometrie" (2020),
 *                Kurs 1840, Fernuni Hagen (https://vu.fernuni-hagen.de/lvuweb/lvu/app/Kurs/1840).
 *   - (Berg 2008) M. de Berg, O. Cheong, M. van Kreveld, M. Overmars, "Computational Geometry" (2008),
 *                 ISBN: 978-3-642-09681-5 (http://dx.doi.org/10.1007/978-3-540-77974-2).
 *
 * ----------------------------------------------------------------------------
 * TAS-Paths (part of the Takin software suite)
 * Copyright (C) 2021-2023  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 * "geo" project
 * Copyright (C) 2020-2021  Tobias WEBER (privately developed).
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

#ifndef __GEO_ALGOS_VORONOI_SIMPLIFIED_H__
#define __GEO_ALGOS_VORONOI_SIMPLIFIED_H__

#warning("Paths library not found, please install TAS-Paths (https://doi.org/10.5281/zenodo.4625649). Using simplified replacement functions.")


#include <libqhullcpp/Qhull.h>
#include <libqhullcpp/QhullFacet.h>
#include <libqhullcpp/QhullRidge.h>
#include <libqhullcpp/QhullFacetList.h>
#include <libqhullcpp/QhullFacetSet.h>
#include <libqhullcpp/QhullVertexSet.h>

#include <set>
#include <optional>
#include <cstdio>

#include <boost/math/quaternion.hpp>

#include "tlibs2/libs/maths.h"


namespace geo {

/**
 * angle of a line
 */
template<class t_vec, class t_real = typename t_vec::value_type>
t_real line_angle(const t_vec& pt1, const t_vec& pt2)
requires tl2::is_vec<t_vec>
{
	t_vec dir = pt2 - pt1;
	t_real angle = std::atan2(t_real(dir[1]), t_real(dir[0]));
	return angle;
}



/**
 * sort vertices by angle
 */
template<class t_vec,
	class t_real = typename t_vec::value_type,
	class t_vec_real = tl2::vec<t_real, std::vector>,
	class t_quat = boost::math::quaternion<t_real>>
std::tuple<std::vector<t_vec>, t_vec_real>
sort_vertices_by_angle(const std::vector<t_vec>& _verts)
requires tl2::is_vec<t_vec> && tl2::is_quat<t_quat>
{
	if(_verts.size() == 0)
			return std::make_tuple(_verts, t_vec());

	std::vector<t_vec> verts = _verts;
	std::size_t dim = verts[0].size();

	// sort by angle
	t_vec_real mean = std::accumulate(verts.begin(), verts.end(), tl2::zero<t_vec>(dim));
	if(_verts.size() < 2)
			return std::make_tuple(verts, mean);
	mean /= t_real(verts.size());

	t_quat rot001 = tl2::unit_quat<t_quat>();
	bool rot_to_001 = (dim == 3 && verts.size() >= 3);
	if(rot_to_001)
	{
		t_vec norm = tl2::cross(verts[2] - verts[0], verts[1] - verts[0]);

		// rotate the vertices so that their normal points to [001]
		t_vec dir001 = tl2::create<t_vec>({ 0, 0, 1 });
		rot001 = tl2::rotation_quat<t_vec, t_quat>(norm, dir001);

		for(t_vec& vert : verts)
			vert = tl2::quat_vec_prod<t_quat, t_vec>(rot001, vert);
	}

	std::stable_sort(verts.begin(), verts.end(),
		[&mean](const t_vec& vec1, const t_vec& vec2)->bool
		{
			return line_angle<t_vec_real, t_real>(mean, vec1)
				< line_angle<t_vec_real, t_real>(mean, vec2);
		});

	// rotate back
	if(rot_to_001)
	{
		t_quat invrot001 = tl2::inv<t_quat>(rot001);
		for(t_vec& vert : verts)
			vert = tl2::quat_vec_prod<t_quat, t_vec>(invrot001, vert);
	}

	return std::make_tuple(verts, mean);
}



// ----------------------------------------------------------------------------
// delaunay triangulation
// @see (Klein 2005), ch. 6, pp. 269f
// @see (FUH 2020), ch. 5.3, pp. 228-232
// ----------------------------------------------------------------------------

/**
 * delaunay triangulation and voronoi vertices
 * @returns [ voronoi vertices, triangles, neighbour triangle indices ]
 *
 * @see http://www.qhull.org/html/qh-code.htm#cpp
 * @see https://github.com/qhull/qhull/tree/master/src/libqhullcpp
 * @see https://github.com/qhull/qhull/blob/master/src/qhulltest/Qhull_test.cpp
 */
template<class t_vec, class t_quat = boost::math::quaternion<typename t_vec::value_type>>
requires tl2::is_vec<t_vec> && tl2::is_quat<t_quat>
std::tuple<std::vector<t_vec>, std::vector<std::vector<t_vec>>, std::vector<std::set<std::size_t>>>
calc_delaunay(int dim, const std::vector<t_vec>& verts,
	bool only_hull, bool triangulate = true,
	std::optional<std::size_t> onlysite_idx = std::nullopt)
{
	using namespace tl2_ops;
	namespace qh = orgQhull;

	using t_real = typename t_vec::value_type;
	using t_real_qhull = coordT;

	const t_real eps = 1e-5;
	std::vector<t_vec> voronoi;                     // voronoi vertices
	std::vector<std::vector<t_vec>> triags;         // delaunay triangles
	std::vector<std::set<std::size_t>> neighbours;  // neighbour triangle indices

	try
	{
		std::vector<t_real_qhull> _verts;
		_verts.reserve(verts.size() * dim);
		for(const t_vec& vert : verts)
			for(int i=0; i<dim; ++i)
				_verts.push_back(t_real_qhull{vert[i]});

		std::ostringstream options;
		if(only_hull)
			options << "Qt";
		else
			options << "v Qu Qbb";
		if(triangulate)
			options << " QJ";

		qh::Qhull qh{};
		// workaround because qhull seems to call the qh_fprintf function
		// in libqhull_r instead of the correct one in libqhullcpp
		std::FILE *ferr = /*stderr*/ std::fopen("/dev/null", "w");
		qh.qh()->ferr = ferr;
		qh.setOutputStream(nullptr);
		qh.setErrorStream(nullptr);
		qh.setFactorEpsilon(eps);
		qh.runQhull("triag", dim, int(_verts.size()/dim),
			_verts.data(), options.str().c_str());
		std::fclose(ferr);
		if(qh.hasQhullMessage())
			std::cout << qh.qhullMessage() << std::endl;


		qh::QhullFacetList facets{qh.facetList()};
		std::vector<void*> facetHandles{};
		facetHandles.reserve(facets.size());
		voronoi.reserve(facets.size());
		triags.reserve(facets.size());
		neighbours.reserve(facets.size());


		// use "voronoi" array for hull vertices, if not needed otherwise
		if(only_hull)
		{
			qh::QhullVertexList hull_vertices{qh.vertexList()};
			for(auto iterVert=hull_vertices.begin(); iterVert!=hull_vertices.end(); ++iterVert)
			{
				qh::QhullPoint pt = iterVert->point();

				t_vec vec = tl2::create<t_vec>(dim);
				for(int i=0; i<dim; ++i)
					vec[i] = t_real{pt[i]};

				voronoi.emplace_back(std::move(vec));
			}

			if(dim == 2 || dim == 3)
			{
				std::tie(voronoi, std::ignore)
					= sort_vertices_by_angle<t_vec, t_real, t_vec, t_quat>(
						voronoi);
			}
		}


		// get all triangles
		for(auto iterFacet=facets.begin(); iterFacet!=facets.end(); ++iterFacet)
		{
			if(iterFacet->isUpperDelaunay())
				continue;

			// get triangle vertices
			std::vector<t_vec> thetriag;
			qh::QhullVertexSet vertices = iterFacet->vertices();

			for(auto iterVertex=vertices.begin(); iterVertex!=vertices.end(); ++iterVertex)
			{
				qh::QhullPoint pt = (*iterVertex).point();

				t_vec vec = tl2::create<t_vec>(dim);
				for(int i=0; i<dim; ++i)
					vec[i] = t_real{pt[i]};

				thetriag.emplace_back(std::move(vec));
			}

			// limit to the voronoi region of only one site?
			if(onlysite_idx)
			{
				const t_vec& site = verts[*onlysite_idx];

				// does the delaunay triangle contain this site?
				bool found_site = false;
				for(const t_vec& vec : thetriag)
				{
					if(tl2::equals<t_vec>(site, vec, eps))
					{
						found_site = true;
						break;
					}
				}

				if(!found_site)
					continue;
			}

			// get voronoi vertices
			if(!only_hull)
			{
				qh::QhullPoint pt = iterFacet->voronoiVertex();

				t_vec vec = tl2::create<t_vec>(dim);
				for(int i=0; i<dim; ++i)
					vec[i] = t_real{pt[i]};

				voronoi.emplace_back(std::move(vec));
			}

			// sort triangle vertices
			if(dim == 2 || dim == 3)
			{
				std::tie(thetriag, std::ignore)
					= sort_vertices_by_angle<t_vec, t_real, t_vec, t_quat>(
						thetriag);
			}

			facetHandles.push_back(iterFacet->getBaseT());
			triags.emplace_back(std::move(thetriag));
		}


		// find neighbouring triangles
		if(!only_hull)
		{
			neighbours.resize(triags.size());

			std::size_t facetIdx = 0;
			for(auto iterFacet=facets.begin(); iterFacet!=facets.end(); ++iterFacet)
			{
				if(iterFacet->isUpperDelaunay())
					continue;

				qh::QhullFacetSet neighbourFacets{iterFacet->neighborFacets()};
				for(auto iterNeighbour=neighbourFacets.begin(); iterNeighbour!=neighbourFacets.end();
					++iterNeighbour)
				{
					void* handle = (*iterNeighbour).getBaseT();
					auto iterHandle = std::find(facetHandles.begin(), facetHandles.end(), handle);
					if(iterHandle != facetHandles.end())
					{
						std::size_t handleIdx = iterHandle - facetHandles.begin();
						neighbours[facetIdx].insert(handleIdx);
					}
				}

				if(++facetIdx >= triags.size())
					break;
			}
		}
	}
	catch(const std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}

	return std::make_tuple(voronoi, triags, neighbours);
}

}

#endif
