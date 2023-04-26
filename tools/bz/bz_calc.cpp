/**
 * brillouin zone tool
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

#include "bz.h"
#include "bzlib.h"

#include <QtWidgets/QMessageBox>

#include <iostream>
#include <sstream>

#include "tlibs2/libs/phys.h"
#include "tlibs2/libs/algos.h"
#include "tlibs2/libs/expr.h"
#include "tlibs2/libs/qt/helper.h"

using namespace tl2_ops;


/**
 * precalculates Q vectors for BZ cut calculation
 */
void BZDlg::SetDrawOrder(int order, bool recalc)
{
	//std::cout << "draw order: " << order << std::endl;

	// already calculated?
	if(order != m_drawOrder)
	{
		m_drawingPeaks.clear();
		m_drawingPeaks.reserve((2*order+1)*(2*order+1)*(2*order+1));

		for(t_real h=-order; h<=order; ++h)
			for(t_real k=-order; k<=order; ++k)
				for(t_real l=-order; l<=order; ++l)
					m_drawingPeaks.emplace_back(tl2::create<t_vec>({ h, k, l }));

		m_drawOrder = order;
	}

	if(recalc)
		CalcBZCut();
}


/**
 * precalculates Q vectors for BZ calculation
 */
void BZDlg::SetCalcOrder(int order, bool recalc)
{
	//std::cout << "calc order: " << order << std::endl;

	// already calculated?
	if(order != m_calcOrder)
	{
		m_peaks.clear();
		m_peaks.reserve((2*order+1)*(2*order+1)*(2*order+1));

		for(t_real h=-order; h<=order; ++h)
			for(t_real k=-order; k<=order; ++k)
				for(t_real l=-order; l<=order; ++l)
					m_peaks.emplace_back(tl2::create<t_vec>({ h, k, l }));

		m_calcOrder = order;
	}

	if(recalc)
		CalcBZ();
}


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
	if(m_ignoreCalc || !m_peaks.size())
		return;

	const auto ops_centr = GetSymOps(true);

	// set up bz calculator
	BZCalc<t_mat, t_vec, t_real> bzcalc;
	bzcalc.SetEps(g_eps);
	bzcalc.SetSymOps(ops_centr, true);
	bzcalc.SetCrystalB(m_crystB);
	bzcalc.SetPeaks(m_peaks);
	bzcalc.CalcPeaksInvA();

	// calculate bz
	bzcalc.CalcBZ();

	// clear old plot
	ClearBZPlot();

	// set bz triangles
	m_bz_polys = bzcalc.GetTriangles();

	// add gamma point
	std::size_t idx000 = bzcalc.Get000Peak();
	const std::vector<t_vec>& Qs_invA = bzcalc.GetPeaksInvA();
	if(idx000 < Qs_invA.size())
		PlotAddBraggPeak(Qs_invA[idx000]);

	// add voronoi vertices forming the vertices of the BZ
	for(const t_vec& voro : bzcalc.GetVertices())
		PlotAddVoronoiVertex(voro);

	// add voronoi bisectors
	PlotAddTriangles(bzcalc.GetAllTriangles());

	// set bz description string
	m_descrBZ = bzcalc.Print(g_prec);
	m_descrBZJSON = bzcalc.PrintJSON(g_prec);

	if(full_recalc)
		CalcBZCut();
	else
		UpdateBZDescription();
}


/**
 * calculate brillouin zone cut
 * TODO: move calculation into bzlib.h
 */
void BZDlg::CalcBZCut()
{
	if(m_ignoreCalc || !m_bz_polys.size() || !m_drawingPeaks.size())
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
	bool calc_bzcut_hull = m_acCutHull->isChecked();

	// get plane coordinate system
	t_vec vec1_rlu = tl2::create<t_vec>({ x, y, z });
	t_vec norm_rlu = tl2::create<t_vec>({ nx, ny, nz });
	vec1_rlu /= tl2::norm<t_vec>(vec1_rlu);
	norm_rlu /= tl2::norm<t_vec>(norm_rlu);

	t_vec vec1_invA = m_crystB * vec1_rlu;
	t_vec norm_invA = m_crystB * norm_rlu;
	m_cut_norm_scale = tl2::norm<t_vec>(norm_invA);
	norm_invA /= m_cut_norm_scale;
	t_real d_invA = d_rlu*m_cut_norm_scale;

	t_vec vec2_invA = tl2::cross<t_vec>(norm_invA, vec1_invA);
	vec1_invA = tl2::cross<t_vec>(vec2_invA, norm_invA);

	vec1_invA /= tl2::norm<t_vec>(vec1_invA);
	vec2_invA /= tl2::norm<t_vec>(vec2_invA);

	t_mat B_inv = m_crystA / (t_real(2)*tl2::pi<t_real>);
	t_vec vec2_rlu = B_inv * vec2_invA;
	vec2_rlu /= tl2::norm<t_vec>(vec2_rlu);


	m_cut_plane = tl2::create<t_mat, t_vec>({ vec1_invA, vec2_invA, norm_invA }, false);
	m_cut_plane_inv = tl2::trans<t_mat>(m_cut_plane);

	// [x, y, Q]
	std::vector<std::tuple<t_vec, t_vec, std::array<t_real, 3>>>
		cut_lines, cut_lines000;

	const auto ops = GetSymOps(true);

	for(const t_vec& Q : m_drawingPeaks)
	{
		if(!is_reflection_allowed<t_mat, t_vec, t_real>(
			Q, ops, g_eps).first)
			continue;

		// (000) peak?
		bool is_000 = tl2::equals_0(Q, g_eps);
		t_vec Q_invA = m_crystB * Q;

		std::vector<t_vec> cut_verts;
		std::optional<t_real> z_comp;

		for(const auto& _bz_poly : m_bz_polys)
		{
			// centre bz around bragg peak
			auto bz_poly = _bz_poly;
			for(t_vec& vec : bz_poly)
				vec += Q_invA;

			auto vecs = tl2::intersect_plane_poly<t_vec>(
				norm_invA, d_invA, bz_poly, g_eps);
			vecs = tl2::remove_duplicates(vecs, g_eps);

			// calculate the hull of the bz cut
			if(calc_bzcut_hull)
			{
				for(const t_vec& vec : vecs)
				{
					t_vec vec_rot = m_cut_plane_inv * vec;
					tl2::set_eps_0(vec_rot, g_eps);

					cut_verts.emplace_back(
						tl2::create<t_vec>({
							vec_rot[0],
							vec_rot[1] }));

					// z component is the same for every vector
					if(!z_comp)
						z_comp = vec_rot[2];
				}
			}
			// alternatively use the lines directly
			else if(vecs.size() >= 2)
			{
				t_vec pt1 = m_cut_plane_inv * vecs[0];
				t_vec pt2 = m_cut_plane_inv * vecs[1];
				tl2::set_eps_0(pt1, g_eps);
				tl2::set_eps_0(pt2, g_eps);

				cut_lines.emplace_back(std::make_tuple(
					pt1, pt2,
					std::array<t_real,3>{Q[0], Q[1], Q[2]}));
				if(is_000)
				{
					cut_lines000.emplace_back(std::make_tuple(
						pt1, pt2,
						std::array<t_real,3>{Q[0], Q[1], Q[2]}));
				}
			}
		}

		// calculate the hull of the bz cut
		if(calc_bzcut_hull)
		{
			cut_verts = tl2::remove_duplicates(cut_verts, g_eps);
			if(cut_verts.size() < 3)
				continue;

			// calculate the faces of the BZ
			auto [bz_verts, bz_triags, bz_neighbours] =
				geo::calc_delaunay(2, cut_verts, true, false);

			for(std::size_t bz_idx=0; bz_idx<bz_verts.size(); ++bz_idx)
			{
				std::size_t bz_idx2 = (bz_idx+1) % bz_verts.size();
				t_vec pt1 = tl2::create<t_vec>({
					bz_verts[bz_idx][0],
					bz_verts[bz_idx][1],
					z_comp ? *z_comp : 0. });
				t_vec pt2 = tl2::create<t_vec>({
					bz_verts[bz_idx2][0],
					bz_verts[bz_idx2][1],
					z_comp ? *z_comp : 0. });
				tl2::set_eps_0(pt1, g_eps);
				tl2::set_eps_0(pt2, g_eps);

				cut_lines.emplace_back(std::make_tuple(
					pt1, pt2,
					std::array<t_real,3>{Q[0], Q[1], Q[2]}));
				if(is_000)
				{
					cut_lines000.emplace_back(std::make_tuple(
						pt1, pt2,
						std::array<t_real,3>{Q[0], Q[1], Q[2]}));
				}
			}
		}
	}


	// get ranges
	m_min_x = std::numeric_limits<t_real>::max();
	m_max_x = -m_min_x;
	m_min_y = std::numeric_limits<t_real>::max();
	m_max_y = -m_min_y;

	for(const auto& tup : cut_lines)
	{
		const auto& pt1 = std::get<0>(tup);
		const auto& pt2 = std::get<1>(tup);

		m_min_x = std::min(m_min_x, pt1[0]);
		m_min_x = std::min(m_min_x, pt2[0]);
		m_max_x = std::max(m_max_x, pt1[0]);
		m_max_x = std::max(m_max_x, pt2[0]);

		m_min_y = std::min(m_min_y, pt1[1]);
		m_min_y = std::min(m_min_y, pt2[1]);
		m_max_y = std::max(m_max_y, pt1[1]);
		m_max_y = std::max(m_max_y, pt2[1]);
	}


	// draw cut
	m_bzscene->ClearAll();
	m_bzscene->AddCut(cut_lines);


	// get description of the cut plane
	tl2::set_eps_0(norm_invA, g_eps); tl2::set_eps_0(norm_rlu, g_eps);
	tl2::set_eps_0(vec1_invA, g_eps); tl2::set_eps_0(vec1_rlu, g_eps);
	tl2::set_eps_0(vec2_invA, g_eps); tl2::set_eps_0(vec2_rlu, g_eps);

	ostr << "# Cutting plane";
	ostr << "\nin relative lattice units:";
	ostr << "\n\tnormal: [" << norm_rlu << "] rlu";
	ostr << "\n\tin-plane vector 1: [" << vec1_rlu << "] rlu";
	ostr << "\n\tin-plane vector 2: [" << vec2_rlu << "] rlu";
	ostr << "\n\tplane offset: " << d_rlu << " rlu";

	ostr << "\nin lab units:";
	ostr << "\n\tnormal: [" << norm_invA << "] Å⁻¹";
	ostr << "\n\tin-plane vector 1: [" << vec1_invA << "] Å⁻¹";
	ostr << "\n\tin-plane vector 2: [" << vec2_invA << "] Å⁻¹";
	ostr << "\n\tplane offset: " << d_invA << " Å⁻¹";
	ostr << "\n" << std::endl;


	// get description of bz cut
	ostr << "# Brillouin zone cut" << std::endl;
	for(std::size_t i=0; i<cut_lines000.size(); ++i)
	{
		const auto& line = cut_lines000[i];

		ostr << "line " << i << ":\n\tvertex 0: (" << std::get<0>(line) << ")"
			<< "\n\tvertex 1: (" << std::get<1>(line) << ")" << std::endl;
	}
	m_descrBZCut = ostr.str();


	// update calculation results
	PlotSetPlane(norm_invA, d_invA);
	UpdateBZDescription();
	CalcFormulas();
}


/**
 * evaluate the formulas in the table and plot them
 */
void BZDlg::CalcFormulas()
{
	m_bzscene->ClearCurves();
	if(m_max_x < m_min_x)
		return;

	t_real plane_d = m_cutD->value() * m_cut_norm_scale;

	std::vector<std::string> formulas = GetFormulas();
	for(const std::string& formula : formulas)
	{
		try
		{
			tl2::ExprParser<t_real> parser;
			parser.SetAutoregisterVariables(false);
			parser.register_var("x", 0.);

			if(bool ok = parser.parse(formula); !ok)
				continue;

			int num_pts = 512;
			t_real x_delta = (m_max_x - m_min_x) / t_real(num_pts);

			std::vector<t_vec> curve;
			curve.reserve(num_pts);

			for(t_real x=m_min_x; x<=m_max_x; x+=x_delta)
			{
				t_vec QinvA = m_cut_plane * tl2::create<t_vec>({ x, 0., plane_d });

				parser.register_var("x", x);
				parser.register_var("Qx", QinvA[0]);
				parser.register_var("Qy", QinvA[1]);
				t_real y = parser.eval();
				if(y < m_min_y || y > m_max_y)
					continue;

				curve.emplace_back(tl2::create<t_vec>({ x, y }));
			}

			m_bzscene->AddCurve(curve);
		}
		catch(const std::exception& ex)
		{
			m_status->setText(ex.what());
		}
	}
}


/**
 * calculate reciprocal coordinates of the cursor position
 */
void BZDlg::BZCutMouseMoved(t_real x, t_real y)
{
	t_real plane_d = m_cutD->value() * m_cut_norm_scale;

	t_vec QinvA = m_cut_plane * tl2::create<t_vec>({ x, y, plane_d });
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
