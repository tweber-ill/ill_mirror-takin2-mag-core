/**
 * magnon dynamics -- calculations for dispersion plot
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

#include <QtWidgets/QApplication>

#include <sstream>
#include <thread>
#include <future>
#include <mutex>

#include <boost/scope_exit.hpp>
#include <boost/asio.hpp>
namespace asio = boost::asio;

#include "tlibs2/libs/phys.h"
#include "tlibs2/libs/algos.h"

using namespace tl2_ops;

extern t_real g_eps;
extern int g_prec;
extern int g_prec_gui;


/**
 * clears the dispersion graph
 */
void MagDynDlg::ClearDispersion(bool replot)
{
	m_graphs.clear();

	if(m_plot)
	{
		m_plot->clearPlottables();
		if(replot)
			m_plot->replot();
	}

	m_qs_data.clear();
	m_Es_data.clear();
	m_ws_data.clear();

	for(int i=0; i<3; ++i)
	{
		m_qs_data_channel[i].clear();
		m_Es_data_channel[i].clear();
		m_ws_data_channel[i].clear();
	}

	m_Q_idx = 0;
}


/**
 * draw the calculated dispersion curve
 */
void MagDynDlg::PlotDispersion()
{
	if(!m_plot)
		return;

	m_plot->clearPlottables();
	m_graphs.clear();

	const bool plot_channels = m_plot_channels->isChecked();

	// create plot curves
	if(plot_channels)
	{
		const QColor colChannel[3]
		{
			QColor(0xff, 0x00, 0x00),
			QColor(0x00, 0xff, 0x00),
			QColor(0x00, 0x00, 0xff),
		};

		for(int i=0; i<3; ++i)
		{
			if(!m_plot_channel[i]->isChecked())
				continue;

			GraphWithWeights *graph = new GraphWithWeights(m_plot->xAxis, m_plot->yAxis);
			QPen pen = graph->pen();
			pen.setColor(colChannel[i]);
			pen.setWidthF(1.);
			graph->setPen(pen);
			graph->setBrush(QBrush(pen.color(), Qt::SolidPattern));
			graph->setLineStyle(QCPGraph::lsNone);
			graph->setScatterStyle(QCPScatterStyle(
				QCPScatterStyle::ssDisc, m_weight_scale->value()));
			graph->setAntialiased(true);
			graph->setData(m_qs_data_channel[i], m_Es_data_channel[i], true /*already sorted*/);
			graph->SetWeights(m_ws_data_channel[i]);
			graph->SetWeightScale(m_weight_scale->value(), m_weight_min->value(), m_weight_max->value());
			m_graphs.push_back(graph);
		}
	}
	else
	{
		GraphWithWeights *graph = new GraphWithWeights(m_plot->xAxis, m_plot->yAxis);
		QPen pen = graph->pen();
		const QColor colFull(0xff, 0x00, 0x00);
		pen.setColor(colFull);
		pen.setWidthF(1.);
		graph->setPen(pen);
		graph->setBrush(QBrush(pen.color(), Qt::SolidPattern));
		graph->setLineStyle(QCPGraph::lsNone);
		graph->setScatterStyle(QCPScatterStyle(
			QCPScatterStyle::ssDisc, m_weight_scale->value()));
		graph->setAntialiased(true);
		graph->setData(m_qs_data, m_Es_data, true /*already sorted*/);
		graph->SetWeights(m_ws_data);
		graph->SetWeightScale(m_weight_scale->value(), m_weight_min->value(), m_weight_max->value());
		m_graphs.push_back(graph);
	}

	// set labels
	const char* Q_label[]{ "h (rlu)", "k (rlu)", "l (rlu)" };
	m_plot->xAxis->setLabel(Q_label[m_Q_idx]);

	// set ranges
	auto [min_E_iter, max_E_iter] = std::minmax_element(m_Es_data.begin(), m_Es_data.end());
	m_plot->xAxis->setRange(m_Q_start, m_Q_end);
	if(min_E_iter != m_Es_data.end() && max_E_iter != m_Es_data.end())
	{
		t_real E_range = *max_E_iter - *min_E_iter;

		t_real Emin = *min_E_iter - E_range*0.05;
		t_real Emax = *max_E_iter + E_range*0.05;
		//if(ignore_annihilation)
		//	Emin = t_real(0);

		m_plot->yAxis->setRange(Emin, Emax);
	}
	else
	{
		m_plot->yAxis->setRange(0., 1.);
	}

	m_plot->replot();
}


/**
 * calculate the dispersion branches
 */
void MagDynDlg::CalcDispersion()
{
	if(m_ignoreCalc)
		return;

	BOOST_SCOPE_EXIT(this_)
	{
		this_->EnableInput();
	} BOOST_SCOPE_EXIT_END
	DisableInput();

	// nothing to calculate?
	if(m_dyn.GetAtomSites().size()==0 || m_dyn.GetExchangeTerms().size()==0)
	{
		ClearDispersion(true);
		return;
	}
	else
	{
		ClearDispersion(false);
	}

	t_real Q_start[]
	{
		m_q_start[0]->value(),
		m_q_start[1]->value(),
		m_q_start[2]->value(),
	};

	t_real Q_end[]
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
	m_Q_idx = 0;
	if(Q_range[1] > Q_range[m_Q_idx])
		m_Q_idx = 1;
	if(Q_range[2] > Q_range[m_Q_idx])
		m_Q_idx = 2;

	t_size num_pts = m_num_points->value();

	m_qs_data.reserve(num_pts*10);
	m_Es_data.reserve(num_pts*10);
	m_ws_data.reserve(num_pts*10);

	for(int i=0; i<3; ++i)
	{
		m_qs_data_channel[i].reserve(num_pts*10);
		m_Es_data_channel[i].reserve(num_pts*10);
		m_ws_data_channel[i].reserve(num_pts*10);
	}

	m_Q_start = Q_start[m_Q_idx];
	m_Q_end = Q_end[m_Q_idx];

	// options
	const bool use_goldstone = false;
	const bool unite_degeneracies = m_unite_degeneracies->isChecked();
	const bool ignore_annihilation = m_ignore_annihilation->isChecked();
	const bool use_weights = m_use_weights->isChecked();
	const bool use_projector = m_use_projector->isChecked();
	const bool force_incommensurate = m_force_incommensurate->isChecked();

	t_real E0 = use_goldstone ? m_dyn.GetGoldstoneEnergy() : 0.;
	m_dyn.SetUniteDegenerateEnergies(unite_degeneracies);
	m_dyn.SetForceIncommensurate(force_incommensurate);

	// tread pool
	unsigned int num_threads = std::max<unsigned int>(
		1, std::thread::hardware_concurrency()/2);
	asio::thread_pool pool{num_threads};

	// mutex to protect m_qs_data, m_Es_data, and m_ws_data
	std::mutex mtx;

	using t_task = std::packaged_task<void()>;
	using t_taskptr = std::shared_ptr<t_task>;
	std::vector<t_taskptr> tasks;
	tasks.reserve(num_pts);

	// keep the scanned Q component in ascending order
	if(Q_start[m_Q_idx] > Q_end[m_Q_idx])
	{
		std::swap(Q_start[0], Q_end[0]);
		std::swap(Q_start[1], Q_end[1]);
		std::swap(Q_start[2], Q_end[2]);
	}

	m_stopRequested = false;
	m_progress->setMinimum(0);
	m_progress->setMaximum(num_pts);
	m_progress->setValue(0);
	m_status->setText("Starting calculation.");

	for(t_size i=0; i<num_pts; ++i)
	{
		auto task = [this, &mtx, i, num_pts, E0,
			use_projector, use_weights, ignore_annihilation,
			&Q_start, &Q_end]()
		{
			const t_vec_real Q = tl2::create<t_vec_real>(
			{
				std::lerp(Q_start[0], Q_end[0], t_real(i)/t_real(num_pts-1)),
				std::lerp(Q_start[1], Q_end[1], t_real(i)/t_real(num_pts-1)),
				std::lerp(Q_start[2], Q_end[2], t_real(i)/t_real(num_pts-1)),
			});

			auto energies_and_correlations = m_dyn.GetEnergies(Q, !use_weights);

			for(const auto& E_and_S : energies_and_correlations)
			{
				if(m_stopRequested)
					break;

				t_real E = E_and_S.E - E0;
				if(std::isnan(E) || std::isinf(E))
					continue;
				if(ignore_annihilation && E < t_real(0))
					continue;

				std::lock_guard<std::mutex> _lck{mtx};

				// weights
				if(use_weights)
				{
					t_real weight = use_projector ? E_and_S.weight : E_and_S.weight_full;
					if(std::isnan(weight) || std::isinf(weight))
						continue;

					m_ws_data.push_back(weight);
					//std::cout << Q[m_Q_idx] << " " << E << " " << weight << std::endl;

					for(int channel=0; channel<3; ++channel)
					{
						t_real weight_channel = use_projector
							? E_and_S.weight_channel[channel]
							: E_and_S.weight_channel_full[channel];
						if(!tl2::equals_0<t_real>(weight_channel, g_eps))
						{
							m_Es_data_channel[channel].push_back(E);
							m_qs_data_channel[channel].push_back(Q[m_Q_idx]);
							m_ws_data_channel[channel].push_back(weight_channel);
						}
					}
				}

				m_qs_data.push_back(Q[m_Q_idx]);
				m_Es_data.push_back(E);
			}
		};

		t_taskptr taskptr = std::make_shared<t_task>(task);
		tasks.push_back(taskptr);
		asio::post(pool, [taskptr]() { (*taskptr)(); });
	}

	m_status->setText("Performing calculation.");

	for(std::size_t task_idx=0; task_idx<tasks.size(); ++task_idx)
	{
		t_taskptr task = tasks[task_idx];

		qApp->processEvents();  // process events to see if the stop button was clicked
		if(m_stopRequested)
		{
			pool.stop();
			break;
		}

		task->get_future().get();
		m_progress->setValue(task_idx+1);
	}

	pool.join();

	if(m_stopRequested)
		m_status->setText("Calculation stopped.");
	else
		m_status->setText("Calculation finished.");

	auto sort_data = [](QVector<t_real>& qvec, QVector<t_real>& Evec, QVector<t_real>& wvec)
	{
		// sort vectors by q component
		std::vector<std::size_t> perm = tl2::get_perm(qvec.size(),
			[&qvec, &Evec](std::size_t idx1, std::size_t idx2) -> bool
		{
			// if q components are equal, sort by E
			if(tl2::equals(qvec[idx1], qvec[idx2], g_eps))
				return Evec[idx1] < Evec[idx2];
			return qvec[idx1] < qvec[idx2];
		});

		qvec = tl2::reorder(qvec, perm);
		Evec = tl2::reorder(Evec, perm);
		wvec = tl2::reorder(wvec, perm);
	};

	sort_data(m_qs_data, m_Es_data, m_ws_data);
	for(int i=0; i<3; ++i)
		sort_data(m_qs_data_channel[i], m_Es_data_channel[i], m_ws_data_channel[i]);

	PlotDispersion();
}


/**
 * calculate all magnon dynamics
 */
void MagDynDlg::CalcAllDynamics()
{
	CalcDispersion();
	CalcHamiltonian();
}


/**
 * calculate the hamiltonian for a single Q value
 */
void MagDynDlg::CalcHamiltonian()
{
	if(m_ignoreCalc)
		return;

	// options
	const bool only_energies = !m_use_weights->isChecked();
	const bool use_projector = m_use_projector->isChecked();
	const bool ignore_annihilation = m_ignore_annihilation->isChecked();
	const bool unite_degeneracies = m_unite_degeneracies->isChecked();
	const bool force_incommensurate = m_force_incommensurate->isChecked();

	m_dyn.SetUniteDegenerateEnergies(unite_degeneracies);
	m_dyn.SetForceIncommensurate(force_incommensurate);

	m_hamiltonian->clear();

	const t_vec_real Q = tl2::create<t_vec_real>(
	{
		m_q[0]->value(),
		m_q[1]->value(),
		m_q[2]->value(),
	});

	std::ostringstream ostr;
	ostr.precision(g_prec_gui);

	// get hamiltonian
	t_mat H = m_dyn.GetHamiltonian(Q);

	// print hamiltonian
	auto print_H = [&ostr](const t_mat& H, const t_vec_real& Qvec)
	{
		ostr << "<p><h3>Hamiltonian at Q = ("
			<< Qvec[0] << ", " << Qvec[1] << ", " << Qvec[2] << ")</h3>";
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
	};

	print_H(H, Q);

	// print shifted hamiltonians for incommensurate case
	if(m_dyn.IsIncommensurate())
	{
		const t_vec_real O = tl2::create<t_vec_real>(
		{
			m_ordering[0]->value(),
			m_ordering[1]->value(),
			m_ordering[2]->value(),
		});

		if(!tl2::equals_0<t_vec_real>(O, g_eps))
		{
			t_mat H_p = m_dyn.GetHamiltonian(Q + O);
			t_mat H_m = m_dyn.GetHamiltonian(Q - O);

			print_H(H_p, Q + O);
			print_H(H_m, Q - O);
		}
	}

	// get energies and correlation functions
	using t_E_and_S = typename decltype(m_dyn)::EnergyAndWeight;
	std::vector<t_E_and_S> energies_and_correlations;

	if(m_dyn.IsIncommensurate())
	{
		energies_and_correlations = m_dyn.GetEnergies(Q, only_energies);
	}
	else
	{
		energies_and_correlations = m_dyn.GetEnergiesFromHamiltonian(H, Q, only_energies);
		if(!only_energies)
			m_dyn.GetIntensities(Q, energies_and_correlations);
		if(unite_degeneracies)
			energies_and_correlations = m_dyn.UniteEnergies(energies_and_correlations);
	}

	if(only_energies)
	{
		// split into positive and negative energies
		std::vector<t_magdyn::EnergyAndWeight> Es_neg, Es_pos;
		for(const t_E_and_S& E_and_S : energies_and_correlations)
		{
			t_real E = E_and_S.E;

			if(E < t_real(0))
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

		if(!ignore_annihilation)
		{
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
		}

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
			t_real E = E_and_S.E;
			if(ignore_annihilation && E < t_real(0))
				continue;

			const t_mat& S = E_and_S.S;
			const t_mat& S_perp = E_and_S.S_perp;
			t_real weight = E_and_S.weight;
			if(!use_projector)
				weight = tl2::trace<t_mat>(S).real();

			tl2::set_eps_0(E);
			tl2::set_eps_0(weight);

			// E
			ostr << "<tr>";
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


/**
 * mouse button has been pressed in the plot
 */
void MagDynDlg::PlotMousePress(QMouseEvent* evt)
{
	// show context menu
	if(evt->buttons() & Qt::RightButton)
	{
		if(!m_menuDisp)
			return;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
		QPoint pos = evt->globalPos();
#else
		QPoint pos = evt->globalPosition().toPoint();
#endif
		m_menuDisp->popup(pos);
		evt->accept();
	}
}
