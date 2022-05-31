/**
 * brillouin zone tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date May-2022
 * @license GPLv3, see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2022  Tobias WEBER (Institut Laue-Langevin (ILL),
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

#include "bz.h"

#include <QtWidgets/QMessageBox>

#include <iostream>
#include <sstream>

#include "../structfact/loadcif.h"
#include "tlibs2/libs/phys.h"
#include "tlibs2/libs/algos.h"
#include "tlibs2/libs/qt/helper.h"
#include "pathslib/libs/voronoi.h"

using namespace tl2_ops;


/**
 * calculate crystal B matrix
 */
void BZDlg::CalcB(bool bFullRecalc)
{
	if(m_ignoreCalc)
		return;

	t_real a,b,c, alpha,beta,gamma;
	std::istringstream{m_editA->text().toStdString()} >> a;
	std::istringstream{m_editB->text().toStdString()} >> b;
	std::istringstream{m_editC->text().toStdString()} >> c;
	std::istringstream{m_editAlpha->text().toStdString()} >> alpha;
	std::istringstream{m_editBeta->text().toStdString()} >> beta;
	std::istringstream{m_editGamma->text().toStdString()} >> gamma;

	if(tl2::equals<t_real>(a, 0., g_eps) || a <= 0. ||
		tl2::equals<t_real>(b, 0., g_eps) || b <= 0. ||
		tl2::equals<t_real>(c, 0., g_eps) || c <= 0. ||
		tl2::equals<t_real>(alpha, 0., g_eps) || alpha <= 0. ||
		tl2::equals<t_real>(beta, 0., g_eps) || beta <= 0. ||
		tl2::equals<t_real>(gamma, 0., g_eps) || gamma <= 0.)
	{
		QMessageBox::critical(this, "Brillouin Zones",
			"Error: Invalid lattice.");
		return;
	}

	t_mat crystB = tl2::B_matrix<t_mat>(a, b, c,
		alpha/180.*tl2::pi<t_real>,
		beta/180.*tl2::pi<t_real>,
		gamma/180.*tl2::pi<t_real>);

	bool ok = true;
	t_mat crystA = tl2::unit<t_mat>(3);
	std::tie(crystA, ok) = tl2::inv(crystB);
	if(!ok)
	{
		QMessageBox::critical(this, "Brillouin Zones",
			"Error: Cannot invert B matrix.");
		return;
	}

	m_crystA = crystA * t_real(2)*tl2::pi<t_real>;
	m_crystB = crystB;

	if(m_plot)
	{
		t_mat_gl matA{m_crystA};
		m_plot->GetRenderer()->SetBTrafo(m_crystB, &matA);
	}

	if(bFullRecalc)
		CalcBZ();
}


/**
 * calculate brillouin zone
 */
void BZDlg::CalcBZ()
{
	if(m_ignoreCalc)
		return;

	const auto maxBZ = m_maxBZ->value();
	const auto ops_centr = GetSymOps(true);

	std::ostringstream ostr;
	ostr.precision(g_prec);

#ifdef DEBUG
	ostr << "# centring symmetry operations" << std::endl;
	for(const t_mat& op : ops_centr)
		ostr << op << std::endl;
#endif

	std::vector<t_vec> Qs_invA;
	Qs_invA.reserve((2*maxBZ+1)*(2*maxBZ+1)*(2*maxBZ+1));
	std::size_t idx000 = 0;
	for(t_real h=-maxBZ; h<=maxBZ; ++h)
	{
		for(t_real k=-maxBZ; k<=maxBZ; ++k)
		{
			for(t_real l=-maxBZ; l<=maxBZ; ++l)
			{
				t_vec Q = tl2::create<t_vec>({ h, k, l });

				if(!is_reflection_allowed<t_mat, t_vec, t_real>(
					Q, ops_centr, g_eps).first)
					continue;

				if(tl2::equals_0(Q, g_eps))
					idx000 = Qs_invA.size();

				t_vec Q_invA = m_crystB * Q;
				t_real Qabs_invA = tl2::norm(Q_invA);

				Qs_invA.emplace_back(std::move(Q_invA));
			}
		}
	}


	// calculate voronoi diagram
	auto [voronoi, _triags, _neighbours] =
		geo::calc_delaunay(3, Qs_invA, false, false, idx000);
	voronoi = tl2::remove_duplicates(voronoi, g_eps);

	ClearPlot();

#ifdef DEBUG
	std::ofstream ofstrSites("sites.dat");
	std::cout << "cat sites.dat | qvoronoi s p Fv QV" << idx000 << std::endl;
	ofstrSites << "3 " << Qs_invA.size() << std::endl;
	for(const t_vec& Q : Qs_invA)
	{
		//PlotAddBraggPeak(Q);
		ofstrSites << Q[0] << " " << Q[1] << " " << Q[2] << std::endl;
	}
#endif

	// add gamma point
	PlotAddBraggPeak(Qs_invA[idx000]);

	// add voronoi vertices forming the vertices of the BZ
	ostr << "# Brillouin zone vertices" << std::endl;
	for(std::size_t idx=0; idx<voronoi.size(); ++idx)
	{
		t_vec& voro = voronoi[idx];
		tl2::set_eps_0(voro, g_eps);

		PlotAddVoronoiVertex(voro);

		ostr << "vertex " << idx << ": " << voro << std::endl;
	}

	// calculate the faces of the BZ
	auto [bz_verts, bz_triags, bz_neighbours] =
		geo::calc_delaunay(3, voronoi, true, false);

	std::vector<t_vec> bz_all_triags;
	ostr << "\n# Brillouin zone polygons" << std::endl;
	for(std::size_t idx_triag=0; idx_triag<bz_triags.size(); ++idx_triag)
	{
		auto& triag = bz_triags[idx_triag];

		ostr << "polygon " << idx_triag << ": " << std::endl;
		for(std::size_t idx_vert=0; idx_vert<triag.size(); ++idx_vert)
		{
			t_vec& vert = triag[idx_vert];
			set_eps_0(vert, g_eps);

			// find index of vert among voronoi vertices
			std::ptrdiff_t voroidx = -1;
			if(auto voro_iter = std::find_if(voronoi.begin(), voronoi.end(),
				[&vert](const t_vec& vec) -> bool
				{
					return tl2::equals<t_vec>(vec, vert, g_eps);
				}); voro_iter != voronoi.end())
			{
				voroidx = voro_iter - voronoi.begin();
			}

			bz_all_triags.push_back(vert);
			ostr << "\tvertex " << voroidx << ": "
				<< vert << std::endl;
		}
	}

	PlotAddTriangles(bz_all_triags);

	// brillouin zone description
	m_bz->setPlainText(ostr.str().c_str());
}


/**
 * calculate brillouin zone cut
 */
void BZDlg::CalcBZCut()
{
	t_real nx = m_cutX->value();
	t_real ny = m_cutY->value();
	t_real nz = m_cutZ->value();
	t_real d = m_cutD->value();
	t_vec norm = tl2::create<t_vec>({ nx, ny, nz });
	norm /= tl2::norm<t_vec>(norm);

	PlotSetPlane(norm, d);
}
