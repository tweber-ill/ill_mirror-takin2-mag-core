/**
 * magnon dynamics
 * @author Tobias Weber <tweber@ill.fr>
 * @date Jan-2022
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 28-Dec-2018 from my privately developed "misc" project (https://github.com/t-weber/misc).
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2022  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 * "misc" project
 * Copyright (C) 2017-2022  Tobias WEBER (privately developed).
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

#include "magdyn.h"

#include <sstream>

#include "tlibs2/libs/phys.h"
#include "tlibs2/libs/algos.h"

#include "graph.h"

using namespace tl2_ops;

extern t_real g_eps;
extern int g_prec;
extern int g_prec_gui;


/**
 * calculate the dispersion branches
 */
void MagDynDlg::CalcDispersion()
{
	if(m_ignoreCalc)
		return;

	m_plot->clearPlottables();

	// nothing to calculate?
	if(m_dyn.GetAtomSites().size()==0 || m_dyn.GetExchangeTerms().size()==0)
	{
		m_plot->replot();
		return;
	}

	const t_real Q_start[]
	{
		m_q_start[0]->value(),
		m_q_start[1]->value(),
		m_q_start[2]->value(),
	};

	const t_real Q_end[]
	{
		m_q_end[0]->value(),
		m_q_end[1]->value(),
		m_q_end[2]->value(),
	};

	const t_real Q_range[]
	{
		std::abs(Q_end[0] - Q_start[0]),
		std::abs(Q_end[1] - Q_start[1]),
		std::abs(Q_end[2] - Q_start[2]),
	};

	// Q component with maximum range
	t_size Q_idx = 0;
	if(Q_range[1] > Q_range[Q_idx])
		Q_idx = 1;
	if(Q_range[2] > Q_range[Q_idx])
		Q_idx = 2;

	t_size num_pts = m_num_points->value();

	QVector<t_real> qs_data, Es_data, ws_data;
	qs_data.reserve(num_pts*10);
	Es_data.reserve(num_pts*10);
	ws_data.reserve(num_pts*10);

	t_real weight_scale = m_weight_scale->value();;

	bool use_goldstone = false;
	t_real E0 = use_goldstone ? m_dyn.GetGoldstoneEnergy() : 0.;

	bool only_energies = !m_use_weights->isChecked();
	bool use_projector = m_use_projector->isChecked();

	for(t_size i=0; i<num_pts; ++i)
	{
		t_real Q[]
		{
			std::lerp(Q_start[0], Q_end[0], t_real(i)/t_real(num_pts-1)),
			std::lerp(Q_start[1], Q_end[1], t_real(i)/t_real(num_pts-1)),
			std::lerp(Q_start[2], Q_end[2], t_real(i)/t_real(num_pts-1)),
		};

		auto energies_and_correlations = m_dyn.GetEnergies(Q[0], Q[1], Q[2],
			!m_use_weights->isChecked());

		for(const auto& E_and_S : energies_and_correlations)
		{
			t_real E = E_and_S.E - E0;
			if(std::isnan(E) || std::isinf(E))
				continue;

			qs_data.push_back(Q[Q_idx]);
			Es_data.push_back(E);

			// weights
			if(!only_energies)
			{
				const t_mat& S = E_and_S.S;
				t_real weight = E_and_S.weight;

				if(!use_projector)
					weight = tl2::trace<t_mat>(S).real();

				if(std::isnan(weight) || std::isinf(weight))
					weight = 0.;
				ws_data.push_back(weight * weight_scale);
			}
		}
	}

	//m_plot->addGraph();
	GraphWithWeights *graph = new GraphWithWeights(
		m_plot->xAxis, m_plot->yAxis);
	QPen pen = graph->pen();
	pen.setColor(QColor(0xff, 0x00, 0x00));
	pen.setWidthF(1.);
	graph->setPen(pen);
	graph->setBrush(QBrush(pen.color(), Qt::SolidPattern));
	graph->setLineStyle(QCPGraph::lsNone);
	graph->setScatterStyle(QCPScatterStyle(
		QCPScatterStyle::ssDisc, weight_scale));
	graph->setAntialiased(true);
	graph->setData(qs_data, Es_data, true /*already sorted*/);
	graph->SetWeights(ws_data);

	auto [min_E_iter, max_E_iter] =
		std::minmax_element(Es_data.begin(), Es_data.end());

	const char* Q_label[]{ "h (rlu)", "k (rlu)", "l (rlu)" };
	m_plot->xAxis->setLabel(Q_label[Q_idx]);
	m_plot->xAxis->setRange(Q_start[Q_idx], Q_end[Q_idx]);
	if(min_E_iter != Es_data.end() && max_E_iter != Es_data.end())
	{
		t_real E_range = *max_E_iter - *min_E_iter;
		m_plot->yAxis->setRange(*min_E_iter - E_range*0.05, *max_E_iter + E_range*0.05);
	}
	else
	{
		m_plot->yAxis->setRange(0., 1.);
	}

	m_plot->replot();
}


/**
 * calculate the hamiltonian for a single Q value
 */
void MagDynDlg::CalcHamiltonian()
{
	if(m_ignoreCalc)
		return;

	m_hamiltonian->clear();

	const t_real Q[]
	{
		m_q[0]->value(),
		m_q[1]->value(),
		m_q[2]->value(),
	};

	std::ostringstream ostr;
	ostr.precision(g_prec_gui);

	// get hamiltonian
	t_mat H = m_dyn.GetHamiltonian(Q[0], Q[1], Q[2]);

	ostr << "<p><h3>Hamiltonian</h3>";
	ostr << "<table style=\"border:0px\">";

	for(std::size_t i=0; i<H.size1(); ++i)
	{
		ostr << "<tr>";
		for(std::size_t j=0; j<H.size2(); ++j)
		{
			t_cplx elem = H(i, j);
			tl2::set_eps_0<t_cplx, t_real>(elem, g_eps);
			ostr << "<td style=\"padding-right:8px\">"
				<< elem << "</td>";
		}
		ostr << "</tr>";
	}
	ostr << "</table></p>";


	// get energies and correlation functions
	bool only_energies = !m_use_weights->isChecked();
	bool use_projector = m_use_projector->isChecked();

	auto energies_and_correlations = m_dyn.GetEnergies(H, Q[0], Q[1], Q[2], only_energies);
	using t_E_and_S = typename decltype(energies_and_correlations)::value_type;

	if(only_energies)
	{
		// split into positive and negative energies
		std::vector<EnergyAndWeight> Es_neg, Es_pos;
		for(const t_E_and_S& E_and_S : energies_and_correlations)
		{
			t_real E = E_and_S.E;

			if(E < 0.)
				Es_neg.push_back(E_and_S);
			else
				Es_pos.push_back(E_and_S);
		}

		std::stable_sort(Es_neg.begin(), Es_neg.end(),
			[](const t_E_and_S& E_and_S_1, const t_E_and_S& E_and_S_2) -> bool
		{
			t_real E1 = E_and_S_1.E;
			t_real E2 = E_and_S_2.E;
			return std::abs(E1) < std::abs(E2);
		});

		std::stable_sort(Es_pos.begin(), Es_pos.end(),
			[](const t_E_and_S& E_and_S_1, const t_E_and_S& E_and_S_2) -> bool
		{
			t_real E1 = E_and_S_1.E;
			t_real E2 = E_and_S_2.E;
			return std::abs(E1) < std::abs(E2);
		});

		ostr << "<p><h3>Energies</h3>";
		ostr << "<table style=\"border:0px\">";
		ostr << "<tr>";
		ostr << "<th style=\"padding-right:8px\">Creation</th>";
		for(const t_E_and_S& E_and_S : Es_pos)
		{
			t_real E = E_and_S.E;
			tl2::set_eps_0(E);

			ostr << "<td style=\"padding-right:8px\">"
				<< E << " meV" << "</td>";
		}
		ostr << "</tr>";

		ostr << "<tr>";
		ostr << "<th style=\"padding-right:8px\">Annihilation</th>";
		for(const t_E_and_S& E_and_S : Es_neg)
		{
			t_real E = E_and_S.E;
			tl2::set_eps_0(E);

			ostr << "<td style=\"padding-right:8px\">"
				<< E << " meV" << "</td>";
		}
		ostr << "</tr>";
		ostr << "</table></p>";
	}
	else
	{
		std::stable_sort(energies_and_correlations.begin(), energies_and_correlations.end(),
			[](const t_E_and_S& E_and_S_1, const t_E_and_S& E_and_S_2) -> bool
		{
			t_real E1 = E_and_S_1.E;
			t_real E2 = E_and_S_2.E;
			return E1 < E2;
		});

		ostr << "<p><h3>Spectrum</h3>";
		ostr << "<table style=\"border:0px\">";
		ostr << "<tr>";
		ostr << "<th style=\"padding-right:16px\">Energy E</td>";
		ostr << "<th style=\"padding-right:16px\">Correlation S(Q, E)</td>";
		ostr << "<th style=\"padding-right:16px\">Neutron S⟂(Q, E)</td>";
		ostr << "<th style=\"padding-right:16px\">Weight</td>";
		ostr << "</tr>";
		for(const t_E_and_S& E_and_S : energies_and_correlations)
		{
			ostr << "<tr>";
			t_real E = E_and_S.E;
			const t_mat& S = E_and_S.S;
			const t_mat& S_perp = E_and_S.S_perp;
			t_real weight = E_and_S.weight;
			if(!use_projector)
				weight = tl2::trace<t_mat>(S).real();

			tl2::set_eps_0(E);
			tl2::set_eps_0(weight);

			// E
			ostr << "<td style=\"padding-right:16px\">"
				<< E << " meV" << "</td>";

			// S(Q, E)
			ostr << "<td style=\"padding-right:16px\">";
			ostr << "<table style=\"border:0px\">";
			for(std::size_t i=0; i<S.size1(); ++i)
			{
				ostr << "<tr>";
				for(std::size_t j=0; j<S.size2(); ++j)
				{
					t_cplx elem = S(i, j);
					tl2::set_eps_0<t_cplx, t_real>(elem, g_eps);
					ostr << "<td style=\"padding-right:8px\">"
						<< elem << "</td>";
				}
				ostr << "</tr>";
			}
			ostr << "</table>";
			ostr << "</td>";

			// S_perp(Q, E)
			ostr << "<td style=\"padding-right:16px\">";
			ostr << "<table style=\"border:0px\">";
			for(std::size_t i=0; i<S_perp.size1(); ++i)
			{
				ostr << "<tr>";
				for(std::size_t j=0; j<S_perp.size2(); ++j)
				{
					t_cplx elem = S_perp(i, j);
					tl2::set_eps_0<t_cplx, t_real>(elem, g_eps);
					ostr << "<td style=\"padding-right:8px\">"
						<< elem << "</td>";
				}
				ostr << "</tr>";
			}
			ostr << "</table>";
			ostr << "</td>";

			// tr(S_perp(Q, E))
			ostr << "<td style=\"padding-right:16px\">" << weight << "</td>";

			ostr << "</tr>";
		}
		ostr << "</table></p>";
	}

	m_hamiltonian->setHtml(ostr.str().c_str());
}


/**
 * mouse move event of the plot
 */
void MagDynDlg::PlotMouseMove(QMouseEvent* evt)
{
	if(!m_status)
		return;

	t_real Q = m_plot->xAxis->pixelToCoord(evt->pos().x());
	t_real E = m_plot->yAxis->pixelToCoord(evt->pos().y());

	QString status("Q = %1 rlu, E = %2 meV.");
	status = status.arg(Q, 0, 'g', g_prec_gui).arg(E, 0, 'g', g_prec_gui);
	m_status->setText(status);
}
