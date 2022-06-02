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
void BZDlg::CalcB(bool full_recalc)
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
	m_crystB = std::move(crystB);

	if(m_plot)
	{
		t_mat_gl matA{m_crystA};
		m_plot->GetRenderer()->SetBTrafo(m_crystB, &matA);
	}

	if(full_recalc)
		CalcBZ();
}


/**
 * calculate brillouin zone
 */
void BZDlg::CalcBZ(bool full_recalc)
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
				//t_real Qabs_invA = tl2::norm(Q_invA);
				Qs_invA.emplace_back(std::move(Q_invA));
			}
		}
	}


	// calculate voronoi diagram
	auto [voronoi, _triags, _neighbours] =
		geo::calc_delaunay(3, Qs_invA, false, false, idx000);
	voronoi = tl2::remove_duplicates(voronoi, g_eps);

	ClearBZPlot();
	m_bz_polys.clear();

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

	m_bz_polys = std::move(bz_triags);
	PlotAddTriangles(bz_all_triags);

	m_descrBZ = ostr.str();

	if(full_recalc)
		CalcBZCut();
	else
		UpdateBZDescription();
}


/**
 * calculate brillouin zone cut
 */
void BZDlg::CalcBZCut()
{
	if(m_ignoreCalc || !m_bz_polys.size())
		return;

	std::ostringstream ostr;
	ostr.precision(g_prec);

	t_real x = m_cutX->value();
	t_real y = m_cutY->value();
	t_real z = m_cutZ->value();
	t_real nx = m_cutNX->value();
	t_real ny = m_cutNY->value();
	t_real nz = m_cutNZ->value();
	t_real d_rlu = m_cutD->value();

	// get plane coordinate system
	t_vec vec1 = m_crystB * tl2::create<t_vec>({ x, y, z });
	t_vec norm = tl2::create<t_vec>({ nx, ny, nz });
	norm = m_crystB * norm / tl2::norm<t_vec>(norm);
	m_cut_norm_scale = tl2::norm<t_vec>(norm);
	norm /= m_cut_norm_scale;
	t_real d_invA = d_rlu *m_cut_norm_scale;

	t_vec vec2 = tl2::cross<t_vec>(norm, vec1);
	vec1 = tl2::cross<t_vec>(vec2, norm);
	vec1 /= tl2::norm<t_vec>(vec1);
	vec2 /= tl2::norm<t_vec>(vec2);

	m_cut_plane = tl2::create<t_mat, t_vec>({ vec1, vec2, norm }, false);
	m_cut_plane_inv = tl2::trans<t_mat>(m_cut_plane);

	std::vector<std::pair<t_vec, t_vec>> cut_lines, cut_lines000;

	const auto order = m_BZOrder->value();
	const auto ops = GetSymOps(true);

	for(t_real h=-order; h<=order; ++h)
	{
		for(t_real k=-order; k<=order; ++k)
		{
			for(t_real l=-order; l<=order; ++l)
			{
				t_vec Q = tl2::create<t_vec>({ h, k, l });

				if(!is_reflection_allowed<t_mat, t_vec, t_real>(
					Q, ops, g_eps).first)
					continue;

				// (000) peak?
				bool is_000 = tl2::equals_0(Q, g_eps);
				t_vec Q_invA = m_crystB * Q;

				for(const auto& _bz_poly : m_bz_polys)
				{
					// centre bz around bragg peak
					auto bz_poly = _bz_poly;
					for(t_vec& vec : bz_poly)
						vec += Q_invA;

					auto vecs = tl2::intersect_plane_poly<t_vec>(
						norm, d_invA, bz_poly, g_eps);
					vecs = tl2::remove_duplicates(vecs, g_eps);

					if(vecs.size() >= 2)
					{
						t_vec pt1 = m_cut_plane_inv * vecs[0];
						t_vec pt2 = m_cut_plane_inv * vecs[1];
						tl2::set_eps_0(pt1, g_eps);
						tl2::set_eps_0(pt2, g_eps);

						cut_lines.emplace_back(std::make_pair(pt1, pt2));
						if(is_000)
							cut_lines000.emplace_back(std::make_pair(pt1, pt2));
					}
				}
			}
		}
	}


	m_bzscene->clear();
	m_bzscene->AddCut(cut_lines);

	ostr << "# Brillouin zone cut" << std::endl;
	for(std::size_t i=0; i<cut_lines000.size(); ++i)
	{
		const auto& line = cut_lines000[i];

		ostr << "line " << i << ":\n\tvertex 0: " << line.first
			<< "\n\tvertex 1: " << line.second << std::endl;
	}
	m_descrBZCut = ostr.str();

	PlotSetPlane(norm, d_invA);
	UpdateBZDescription();
}


/**
 * calculate reciprocal coordinates of the cursor position
 */
void BZDlg::BZCutMouseMoved(t_real x, t_real y)
{
	t_real d = m_cutD->value() * m_cut_norm_scale;

	t_vec QinvA = m_cut_plane * tl2::create<t_vec>({ x, y, d });
	t_mat B_inv = m_crystA / (t_real(2)*tl2::pi<t_real>);
	t_vec Qrlu = B_inv * QinvA;

	tl2::set_eps_0(QinvA, g_eps);
	tl2::set_eps_0(Qrlu, g_eps);

	std::ostringstream ostr;
	ostr.precision(g_prec_gui);

	ostr << "Q = (" << QinvA[0] << ", " << QinvA[1] << ", " << QinvA[2] << ") Å⁻¹";
	ostr << " = (" << Qrlu[0] << ", " << Qrlu[1] << ", " << Qrlu[2] << ") rlu.";
	m_status->setText(ostr.str().c_str());	
}
