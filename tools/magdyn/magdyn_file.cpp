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

#include <QtCore/QString>
#include <QtWidgets/QApplication>

#include <iostream>
#include <fstream>
#include <sstream>
#include <future>
#include <mutex>
#include <cstdlib>

#include <boost/scope_exit.hpp>
#include <boost/asio.hpp>
namespace asio = boost::asio;

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace pt = boost::property_tree;

#include "tlibs2/libs/log.h"

extern int g_prec;


/**
 * load magnetic structure configuration
 */
void MagDynDlg::Load()
{
	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getOpenFileName(
		this, "Load File", dirLast, "XML Files (*.xml *.XML)");
	if(filename=="" || !QFile::exists(filename))
		return;

	if(Load(filename))
	{
		m_sett->setValue("dir", QFileInfo(filename).path());
		m_recent.AddRecentFile(filename);
	}
}


/**
 * load magnetic structure configuration
 */
bool MagDynDlg::Load(const QString& filename)
{
	try
	{
		BOOST_SCOPE_EXIT(this_)
		{
			this_->m_ignoreCalc = 0;
		} BOOST_SCOPE_EXIT_END
		m_ignoreCalc = 1;

		// properties tree
		pt::ptree node;

		// load from file
		std::ifstream ifstr{filename.toStdString()};
		pt::read_xml(ifstr, node);

		// check signature
		if(auto optInfo = node.get_optional<std::string>("magdyn.meta.info");
			!optInfo || !(*optInfo==std::string{"magdyn_tool"}))
		{
			QMessageBox::critical(this, "Magnon Dynamics", "Unrecognised file format.");
			return false;
		}

		const auto &magdyn = node.get_child("magdyn");

		// settings
		if(auto optVal = magdyn.get_optional<t_real>("config.h_start"))
			m_q_start[0]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.k_start"))
			m_q_start[1]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.l_start"))
			m_q_start[2]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.h_end"))
			m_q_end[0]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.k_end"))
			m_q_end[1]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.l_end"))
			m_q_end[2]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.h"))
			m_q[0]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.k"))
			m_q[1]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.l"))
			m_q[2]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_size>("config.num_Q_points"))
			m_num_points->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.weight_scale"))
			m_weight_scale->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.weight_min"))
			m_weight_min->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.weight_max"))
			m_weight_max->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<bool>("config.auto_calc"))
			m_autocalc->setChecked(*optVal);
		if(auto optVal = magdyn.get_optional<bool>("config.use_DMI"))
			m_use_dmi->setChecked(*optVal);
		if(auto optVal = magdyn.get_optional<bool>("config.use_field"))
			m_use_field->setChecked(*optVal);
		if(auto optVal = magdyn.get_optional<bool>("config.use_temperature"))
			m_use_temperature->setChecked(*optVal);
		if(auto optVal = magdyn.get_optional<bool>("config.use_weights"))
			m_use_weights->setChecked(*optVal);
		if(auto optVal = magdyn.get_optional<bool>("config.unite_degeneracies"))
			m_unite_degeneracies->setChecked(*optVal);
		if(auto optVal = magdyn.get_optional<bool>("config.use_projector"))
			m_use_projector->setChecked(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.field_axis_h"))
			m_rot_axis[0]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.field_axis_k"))
			m_rot_axis[1]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.field_axis_l"))
			m_rot_axis[2]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.field_angle"))
			m_rot_angle->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<int>("config.spacegroup_index"))
			m_comboSG->setCurrentIndex(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.export_start_h"))
			m_exportStartQ[0]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.export_start_k"))
			m_exportStartQ[1]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.export_start_l"))
			m_exportStartQ[2]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.export_end_h1"))
			m_exportEndQ1[0]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.export_end_k1"))
			m_exportEndQ1[1]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.export_end_l1"))
			m_exportEndQ1[2]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.export_end_h2"))
			m_exportEndQ2[0]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.export_end_k2"))
			m_exportEndQ2[1]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.export_end_l2"))
			m_exportEndQ2[2]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.export_end_h3"))
			m_exportEndQ3[0]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.export_end_k3"))
			m_exportEndQ3[1]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.export_end_l3"))
			m_exportEndQ3[2]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_size>("config.export_num_points_1"))
			m_exportNumPoints[0]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_size>("config.export_num_points_2"))
			m_exportNumPoints[1]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_size>("config.export_num_points_3"))
			m_exportNumPoints[2]->setValue(*optVal);

		m_dyn.Load(magdyn);

		// external field
		m_field_dir[0]->setValue(m_dyn.GetExternalField().dir[0]);
		m_field_dir[1]->setValue(m_dyn.GetExternalField().dir[1]);
		m_field_dir[2]->setValue(m_dyn.GetExternalField().dir[2]);
		m_field_mag->setValue(m_dyn.GetExternalField().mag);
		m_align_spins->setChecked(m_dyn.GetExternalField().align_spins);
		if(!m_use_field->isChecked())
			m_dyn.ClearExternalField();

		// ordering vector
		const t_vec_real& ordering = m_dyn.GetOrderingWavevector();
		if(ordering.size() == 3)
		{
			m_ordering[0]->setValue(ordering[0]);
			m_ordering[1]->setValue(ordering[1]);
			m_ordering[2]->setValue(ordering[2]);
		}

		// normal axis
		const t_vec_real& norm = m_dyn.GetRotationAxis();
		if(norm.size() == 3)
		{
			m_normaxis[0]->setValue(norm[0]);
			m_normaxis[1]->setValue(norm[1]);
			m_normaxis[2]->setValue(norm[2]);
		}

		// temperature
		t_real temp = m_dyn.GetTemperature();
		if(temp >= 0.)
			m_temperature->setValue(temp);
		if(!m_use_temperature->isChecked())
			m_dyn.SetTemperature(-1.);

		// clear old tables
		DelTabItem(m_sitestab, -1);
		DelTabItem(m_termstab, -1);
		DelTabItem(m_varstab, -1);

		// variables
		for(const auto& var : m_dyn.GetVariables())
		{
			AddVariableTabItem(-1, var.name, var.value);
		}

		// atom sites
		for(const auto &site : m_dyn.GetAtomSites())
		{
			t_real S = site.spin_mag;

			AddSiteTabItem(-1,
				site.name,
				site.pos[0], site.pos[1], site.pos[2],
				site.spin_dir[0], site.spin_dir[1], site.spin_dir[2], S);
		}

		// exchange terms
		for(const auto& term : m_dyn.GetExchangeTerms())
		{
			AddTermTabItem(-1,
				term.name, term.atom1, term.atom2,
				term.dist[0], term.dist[1], term.dist[2],
				term.J,
				term.dmi[0], term.dmi[1], term.dmi[2]);
		}
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Magnon Dynamics", ex.what());
		return false;
	}

	//SyncSitesAndTerms();
	CalcAll();
	StructPlotSync();

	return true;
}


/**
 * save current magnetic structure configuration
 */
void MagDynDlg::Save()
{
	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getSaveFileName(
		this, "Save File", dirLast, "XML Files (*.xml)");
	if(filename=="")
		return;

	if(Save(filename))
	{
		m_sett->setValue("dir", QFileInfo(filename).path());
		m_recent.AddRecentFile(filename);
	}
}


/**
 * save current magnetic structure configuration
 */
bool MagDynDlg::Save(const QString& filename)
{
	try
	{
		// properties tree
		pt::ptree magdyn;

		const char* user = std::getenv("USER");
		if(!user) user = "";

		magdyn.put<std::string>("meta.info", "magdyn_tool");
		magdyn.put<std::string>("meta.date",
			tl2::epoch_to_str<t_real>(tl2::epoch<t_real>()));
		magdyn.put<std::string>("meta.user", user);
		magdyn.put<std::string>("meta.url",
			"https://code.ill.fr/scientific-software/takin");
		magdyn.put<std::string>("meta.doi",
			"https://doi.org/10.5281/zenodo.4117437");
		magdyn.put<std::string>("meta.doi_tlibs",
			"https://doi.org/10.5281/zenodo.5717779");

		// settings
		magdyn.put<t_real>("config.h_start", m_q_start[0]->value());
		magdyn.put<t_real>("config.k_start", m_q_start[1]->value());
		magdyn.put<t_real>("config.l_start", m_q_start[2]->value());
		magdyn.put<t_real>("config.h_end", m_q_end[0]->value());
		magdyn.put<t_real>("config.k_end", m_q_end[1]->value());
		magdyn.put<t_real>("config.l_end", m_q_end[2]->value());
		magdyn.put<t_real>("config.h", m_q[0]->value());
		magdyn.put<t_real>("config.k", m_q[1]->value());
		magdyn.put<t_real>("config.l", m_q[2]->value());
		magdyn.put<t_size>("config.num_Q_points", m_num_points->value());
		magdyn.put<t_real>("config.weight_scale", m_weight_scale->value());
		magdyn.put<t_real>("config.weight_min", m_weight_min->value());
		magdyn.put<t_real>("config.weight_max", m_weight_max->value());
		magdyn.put<bool>("config.auto_calc", m_autocalc->isChecked());
		magdyn.put<bool>("config.use_DMI", m_use_dmi->isChecked());
		magdyn.put<bool>("config.use_field", m_use_field->isChecked());
		magdyn.put<bool>("config.use_temperature", m_use_temperature->isChecked());
		magdyn.put<bool>("config.use_weights", m_use_weights->isChecked());
		magdyn.put<bool>("config.unite_degeneracies", m_unite_degeneracies->isChecked());
		magdyn.put<bool>("config.use_projector", m_use_projector->isChecked());
		magdyn.put<t_real>("config.field_axis_h", m_rot_axis[0]->value());
		magdyn.put<t_real>("config.field_axis_k", m_rot_axis[1]->value());
		magdyn.put<t_real>("config.field_axis_l", m_rot_axis[2]->value());
		magdyn.put<t_real>("config.field_angle", m_rot_angle->value());
		magdyn.put<t_real>("config.spacegroup_index", m_comboSG->currentIndex());
		magdyn.put<t_real>("config.export_start_h", m_exportStartQ[0]->value());
		magdyn.put<t_real>("config.export_start_k", m_exportStartQ[1]->value());
		magdyn.put<t_real>("config.export_start_l", m_exportStartQ[2]->value());
		magdyn.put<t_real>("config.export_end_h1", m_exportEndQ1[0]->value());
		magdyn.put<t_real>("config.export_end_k1", m_exportEndQ1[1]->value());
		magdyn.put<t_real>("config.export_end_l1", m_exportEndQ1[2]->value());
		magdyn.put<t_real>("config.export_end_h2", m_exportEndQ2[0]->value());
		magdyn.put<t_real>("config.export_end_k2", m_exportEndQ2[1]->value());
		magdyn.put<t_real>("config.export_end_l2", m_exportEndQ2[2]->value());
		magdyn.put<t_real>("config.export_end_h3", m_exportEndQ3[0]->value());
		magdyn.put<t_real>("config.export_end_k3", m_exportEndQ3[1]->value());
		magdyn.put<t_real>("config.export_end_l3", m_exportEndQ3[2]->value());
		magdyn.put<t_size>("config.export_num_points_1", m_exportNumPoints[0]->value());
		magdyn.put<t_size>("config.export_num_points_2", m_exportNumPoints[1]->value());
		magdyn.put<t_size>("config.export_num_points_3", m_exportNumPoints[2]->value());

		m_dyn.Save(magdyn);

		pt::ptree node;
		node.put_child("magdyn", magdyn);

		// save to file
		std::ofstream ofstr{filename.toStdString()};
		if(!ofstr)
		{
			QMessageBox::critical(this, "Magnon Dynamics",
				"Cannot open file for writing.");
			return false;
		}

		ofstr.precision(g_prec);
		pt::write_xml(ofstr, node,
			pt::xml_writer_make_settings('\t', 1, std::string{"utf-8"}));
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Magnon Dynamics", ex.what());
		return false;
	}

	return true;
}


/**
 * save the plot as pdf
 */
void MagDynDlg::SavePlotFigure()
{
	if(!m_plot)
		return;

	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getSaveFileName(
		this, "Save Figure", dirLast, "PDF Files (*.pdf)");
	if(filename=="")
		return;
	m_sett->setValue("dir", QFileInfo(filename).path());

	m_plot->savePdf(filename);
}


/**
 * save the dispersion data
 */
void MagDynDlg::SaveDispersion()
{
	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getSaveFileName(
		this, "Save Data", dirLast, "Data Files (*.dat)");
	if(filename=="")
		return;
	m_sett->setValue("dir", QFileInfo(filename).path());

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

	const t_size num_pts = m_num_points->value();

	m_dyn.SaveDispersion(filename.toStdString(),
		Q_start[0], Q_start[1], Q_start[2],
		Q_end[0], Q_end[1], Q_end[2],
		num_pts);
}


/**
 * show dialog and export S(Q, E) into a grid
 */
void MagDynDlg::ExportSQE()
{
	QString extension;
	switch(m_exportFormat->currentIndex())
	{
		case 0: extension = "Takin Grid Files (*.bin)"; break;
		case 1: extension = "Text Files (*.txt)"; break;
	}

	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getSaveFileName(
		this, "Export S(Q,E)", dirLast, extension);
	if(filename == "")
		return;

	if(ExportSQE(filename))
		m_sett->setValue("dir", QFileInfo(filename).path());
}


/**
 * export S(Q, E) into a grid
 */
bool MagDynDlg::ExportSQE(const QString& filename)
{
	std::ofstream ofstr(filename.toStdString());
	ofstr.precision(g_prec);

	if(!ofstr)
	{
		QMessageBox::critical(this, "Magnon Dynamics", "File could not be opened.");
		return false;
	}

	const t_vec_real Q0 = tl2::create<t_vec_real>({
		m_exportStartQ[0]->value(),
		m_exportStartQ[1]->value(),
		m_exportStartQ[2]->value() });
	const t_vec_real Q1 = tl2::create<t_vec_real>({
		m_exportEndQ1[0]->value(),
		m_exportEndQ1[1]->value(),
		m_exportEndQ1[2]->value() });
	const t_vec_real Q2 = tl2::create<t_vec_real>({
		m_exportEndQ2[0]->value(),
		m_exportEndQ2[1]->value(),
		m_exportEndQ2[2]->value() });
	const t_vec_real Q3 = tl2::create<t_vec_real>({
		m_exportEndQ3[0]->value(),
		m_exportEndQ3[1]->value(),
		m_exportEndQ3[2]->value() });

	const t_size num_pts1 = m_exportNumPoints[0]->value();
	const t_size num_pts2 = m_exportNumPoints[1]->value();
	const t_size num_pts3 = m_exportNumPoints[2]->value();

	const int format = m_exportFormat->currentIndex();

	MagDyn dyn = m_dyn;
	dyn.SetUniteDegenerateEnergies(m_unite_degeneracies->isChecked());
	bool use_weights = m_use_weights->isChecked();
	bool use_projector = m_use_projector->isChecked();

	const t_vec_real dir1 = Q1 - Q0;
	const t_vec_real dir2 = Q2 - Q0;
	const t_vec_real dir3 = Q3 - Q0;

	const t_vec_real inc1 = dir1 / t_real(num_pts1-1);
	const t_vec_real inc2 = dir2 / t_real(num_pts1-1);
	const t_vec_real inc3 = dir3 / t_real(num_pts1-1);

	// tread pool
	unsigned int num_threads = std::max<unsigned int>(
		1, std::thread::hardware_concurrency()/2);
	asio::thread_pool pool{num_threads};


	// h, k, l, E, S
	using t_taskret = std::tuple<t_real, t_real, t_real, std::vector<t_real>, std::vector<t_real>>;

	// calculation task
	auto task = [this, use_weights, use_projector, &dyn](t_real h, t_real k, t_real l) -> t_taskret
	{
		auto energies_and_correlations = dyn.GetEnergies(
			h, k, l, !use_weights);

		std::vector<t_real> Es, weights;
		Es.reserve(energies_and_correlations.size());
		weights.reserve(energies_and_correlations.size());

		for(const auto& E_and_S : energies_and_correlations)
		{
			if(m_stopRequested)
				break;

			t_real E = E_and_S.E;
			if(std::isnan(E) || std::isinf(E))
				continue;

			const t_mat& S = E_and_S.S;
			t_real weight = E_and_S.weight;

			if(!use_projector)
				weight = tl2::trace<t_mat>(S).real();

			if(std::isnan(weight) || std::isinf(weight))
				weight = 0.;

			Es.push_back(E);
			weights.push_back(weight);
		}

		return std::make_tuple(h, k, l, Es, weights);
	};


	// tasks
	using t_task = std::packaged_task<t_taskret(t_real, t_real, t_real)>;
	using t_taskptr = std::shared_ptr<t_task>;
	std::vector<t_taskptr> tasks;
	std::vector<std::future<t_taskret>> futures;
	tasks.reserve(num_pts1 * num_pts2 * num_pts3);
	futures.reserve(num_pts1 * num_pts2 * num_pts3);

	m_stopRequested = false;
	m_progress->setMinimum(0);
	m_progress->setMaximum(num_pts1 * num_pts2 * num_pts3);
	m_progress->setValue(0);


	for(std::size_t i1=0; i1<num_pts1; ++i1)
	{
		if(m_stopRequested)
			break;

		for(std::size_t i2=0; i2<num_pts2; ++i2)
		{
			if(m_stopRequested)
				break;

			for(std::size_t i3=0; i3<num_pts3; ++i3)
			{
				qApp->processEvents();  // process events to see if the stop button was clicked
				if(m_stopRequested)
					break;

				t_vec_real Q = Q0 + inc1*t_real(i1) + inc2*t_real(i2) + inc3*t_real(i3);

				// create tasks
				t_taskptr taskptr = std::make_shared<t_task>(task);
				tasks.push_back(taskptr);
				futures.emplace_back(taskptr->get_future());
				asio::post(pool, [taskptr, Q]() { (*taskptr)(Q[0], Q[1], Q[2]); });
			}
		}
	}


	for(std::size_t i=0; i<futures.size(); ++i)
	{
		qApp->processEvents();  // process events to see if the stop button was clicked
		if(m_stopRequested)
		{
			pool.stop();
			break;
		}

		auto result = futures[i].get();

		if(format == 1)  // text format
		{
			ofstr
				<< "Q = "
				<< std::get<0>(result) << " "
				<< std::get<1>(result) << " "
				<< std::get<2>(result) << ":\n";
		}

		// iterate energies and weights
		for(std::size_t j=0; j<std::get<3>(result).size(); ++j)
		{
			if(format == 1)  // text format
			{
				ofstr
					<< "\tE = " << std::get<3>(result)[j]
					<< ", w = " << std::get<4>(result)[j] << std::endl;
			}
		}

		m_progress->setValue(i+1);
	}

	pool.join();

	if(m_stopRequested)
		m_status->setText("Calculation stopped.");
	else
		m_status->setText("Calculation finished.");

	return true;
}
