/**
 * magnetic dynamics
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
#include <vector>
#include <deque>
#include <cstdlib>

#include <boost/scope_exit.hpp>
#include <boost/asio.hpp>
namespace asio = boost::asio;

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace pt = boost::property_tree;

#include "tlibs2/libs/log.h"

#ifdef USE_HDF5
	#include "tlibs2/libs/h5file.h"
#endif


// for debugging: write individual data chunks in hdf5 file
//#define WRITE_HDF5_CHUNKS


extern int g_prec;


void MagDynDlg::Clear()
{
	BOOST_SCOPE_EXIT(this_)
	{
		this_->m_ignoreCalc = false;
		//this_->SyncSitesAndTerms();
		this_->StructPlotSync();
	} BOOST_SCOPE_EXIT_END
	m_ignoreCalc = true;

	// clear old tables
	DelTabItem(m_sitestab, -1);
	DelTabItem(m_termstab, -1);
	DelTabItem(m_varstab, -1);
	DelTabItem(m_fieldstab, -1);

	ClearDispersion(true);
	m_hamiltonian->clear();
	m_dyn.Clear();

	SetCurrentFile("");

	// set some defaults
	m_comboSG->setCurrentIndex(0);

	m_ordering[0]->setValue(0.);
	m_ordering[1]->setValue(0.);
	m_ordering[2]->setValue(0.);

	m_normaxis[0]->setValue(1.);
	m_normaxis[1]->setValue(0.);
	m_normaxis[2]->setValue(0.);

	m_weight_scale->setValue(1.);
	m_weight_min->setValue(0.);
	m_weight_max->setValue(9999.);
}


/**
 * set the currently open file and the corresponding window title
 */
void MagDynDlg::SetCurrentFile(const QString& filename)
{
	m_recent.SetCurFile(filename);

	QString title = "Magnetic Dynamics";
	if(filename != "")
		title += " - " + filename;
	setWindowTitle(title);
}


/**
 * load magnetic structure configuration
 */
void MagDynDlg::Load()
{
	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getOpenFileName(
		this, "Load File", dirLast, "Magnetic Dynamics Files (*.magdyn *.xml)");
	if(filename=="" || !QFile::exists(filename))
		return;

	Clear();

	if(Load(filename))
	{
		m_sett->setValue("dir", QFileInfo(filename).path());
		m_recent.AddRecentFile(filename);
		SetCurrentFile(filename);
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
			this_->m_ignoreCalc = false;
			if(this_->m_autocalc->isChecked())
				this_->CalcAll();
		} BOOST_SCOPE_EXIT_END
		m_ignoreCalc = true;

		// properties tree
		pt::ptree node;

		// load from file
		std::ifstream ifstr{filename.toStdString()};
		pt::read_xml(ifstr, node);

		// check signature
		if(auto optInfo = node.get_optional<std::string>("magdyn.meta.info");
			!optInfo || !(*optInfo==std::string{"magdyn_tool"}))
		{
			QMessageBox::critical(this, "Magnetic Dynamics", "Unrecognised file format.");
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
		if(auto optVal = magdyn.get_optional<bool>("config.plot_channels"))
			m_plot_channels->setChecked(*optVal);
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
		if(auto optVal = magdyn.get_optional<bool>("config.ignore_annihilation"))
			m_ignore_annihilation->setChecked(*optVal);
		if(auto optVal = magdyn.get_optional<bool>("config.force_incommensurate"))
			m_force_incommensurate->setChecked(*optVal);
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
		if(auto optVal = magdyn.get_optional<t_real>("config.export_end_h"))
			m_exportEndQ[0]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.export_end_k"))
			m_exportEndQ[1]->setValue(*optVal);
		if(auto optVal = magdyn.get_optional<t_real>("config.export_end_l"))
			m_exportEndQ[2]->setValue(*optVal);
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
		DelTabItem(m_fieldstab, -1);

		// variables
		for(const auto& var : m_dyn.GetVariables())
		{
			AddVariableTabItem(-1, var.name, var.value);
		}

		// get site entries for reading additional infos
		auto sites = magdyn.get_child_optional("atom_sites");

		// atom sites
		for(const auto &site : m_dyn.GetAtomSites())
		{
			t_real S = site.spin_mag;

			// default colour
			std::string rgb = "auto";

			// get additional data from exchange term entry
			if(sites && site.index < sites->size())
			{
				auto siteiter = (*sites).begin();
				std::advance(siteiter, site.index);

				// read colour
				rgb = siteiter->second.get<std::string>("colour", "auto");
			}

			AddSiteTabItem(-1,
				site.name,
				site.pos[0], site.pos[1], site.pos[2],
				site.spin_dir[0], site.spin_dir[1], site.spin_dir[2], S,
				rgb);
		}

		// get exchange terms entries for reading additional infos
		auto terms = magdyn.get_child_optional("exchange_terms");

		// exchange terms
		for(const auto& term : m_dyn.GetExchangeTerms())
		{
			// default colour
			std::string rgb = "#0x00bf00";

			// get additional data from exchange term entry
			if(terms && term.index < terms->size())
			{
				auto termiter = (*terms).begin();
				std::advance(termiter, term.index);

				// read colour
				rgb = termiter->second.get<std::string>("colour", "#0x00bf00");
			}

			AddTermTabItem(-1,
				term.name, term.atom1, term.atom2,
				term.dist[0], term.dist[1], term.dist[2],
				term.J,
				term.dmi[0], term.dmi[1], term.dmi[2],
				rgb);
		}

		// saved fields
		if(auto vars = magdyn.get_child_optional("saved_fields"); vars)
		{
			for(const auto &var : *vars)
			{
				t_real Bh = var.second.get<t_real>("direction_h", 0.);
				t_real Bk = var.second.get<t_real>("direction_k", 0.);
				t_real Bl = var.second.get<t_real>("direction_l", 0.);
				t_real Bmag = var.second.get<t_real>("magnitude", 0.);

				AddFieldTabItem(-1, Bh, Bk, Bl, Bmag);
			}
		}
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Magnetic Dynamics", ex.what());
		return false;
	}

	return true;
}


/**
 * save current magnetic structure configuration
 */
void MagDynDlg::Save()
{
	const QString& curFile = m_recent.GetCurFile();
	if(curFile == "")
		SaveAs();
	else
		Save(curFile);
}


/**
 * save current magnetic structure configuration
 */
void MagDynDlg::SaveAs()
{
	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getSaveFileName(
		this, "Save File", dirLast, "Magnetic Dynamics Files (*.magdyn)");
	if(filename=="")
		return;

	if(Save(filename))
	{
		m_sett->setValue("dir", QFileInfo(filename).path());
		m_recent.AddRecentFile(filename);
		SetCurrentFile(filename);
	}
}


/**
 * save current magnetic structure configuration
 */
bool MagDynDlg::Save(const QString& filename)
{
	try
	{
		SyncSitesAndTerms();

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
		magdyn.put<bool>("config.plot_channels", m_plot_channels->isChecked());
		magdyn.put<bool>("config.auto_calc", m_autocalc->isChecked());
		magdyn.put<bool>("config.use_DMI", m_use_dmi->isChecked());
		magdyn.put<bool>("config.use_field", m_use_field->isChecked());
		magdyn.put<bool>("config.use_temperature", m_use_temperature->isChecked());
		magdyn.put<bool>("config.use_weights", m_use_weights->isChecked());
		magdyn.put<bool>("config.unite_degeneracies", m_unite_degeneracies->isChecked());
		magdyn.put<bool>("config.ignore_annihilation", m_ignore_annihilation->isChecked());
		magdyn.put<bool>("config.force_incommensurate", m_force_incommensurate->isChecked());
		magdyn.put<bool>("config.use_projector", m_use_projector->isChecked());
		magdyn.put<t_real>("config.field_axis_h", m_rot_axis[0]->value());
		magdyn.put<t_real>("config.field_axis_k", m_rot_axis[1]->value());
		magdyn.put<t_real>("config.field_axis_l", m_rot_axis[2]->value());
		magdyn.put<t_real>("config.field_angle", m_rot_angle->value());
		magdyn.put<t_real>("config.spacegroup_index", m_comboSG->currentIndex());
		magdyn.put<t_real>("config.export_start_h", m_exportStartQ[0]->value());
		magdyn.put<t_real>("config.export_start_k", m_exportStartQ[1]->value());
		magdyn.put<t_real>("config.export_start_l", m_exportStartQ[2]->value());
		magdyn.put<t_real>("config.export_end_h", m_exportEndQ[0]->value());
		magdyn.put<t_real>("config.export_end_k", m_exportEndQ[1]->value());
		magdyn.put<t_real>("config.export_end_l", m_exportEndQ[2]->value());
		magdyn.put<t_size>("config.export_num_points_1", m_exportNumPoints[0]->value());
		magdyn.put<t_size>("config.export_num_points_2", m_exportNumPoints[1]->value());
		magdyn.put<t_size>("config.export_num_points_3", m_exportNumPoints[2]->value());

		// save magnon calculator configuration
		m_dyn.Save(magdyn);

		// saved fields
		for(int field_row = 0; field_row < m_fieldstab->rowCount(); ++field_row)
		{
			const auto* Bh = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
				m_fieldstab->item(field_row, COL_FIELD_H));
			const auto* Bk = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
				m_fieldstab->item(field_row, COL_FIELD_K));
			const auto* Bl = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
				m_fieldstab->item(field_row, COL_FIELD_L));
			const auto* Bmag = static_cast<tl2::NumericTableWidgetItem<t_real>*>(
				m_fieldstab->item(field_row, COL_FIELD_MAG));

			boost::property_tree::ptree itemNode;
			itemNode.put<t_real>("direction_h", Bh ? Bh->GetValue() : 0.);
			itemNode.put<t_real>("direction_k", Bk ? Bk->GetValue() : 0.);
			itemNode.put<t_real>("direction_l", Bl ? Bl->GetValue() : 0.);
			itemNode.put<t_real>("magnitude", Bmag ? Bmag->GetValue() : 0.);

			magdyn.add_child("saved_fields.field", itemNode);
		}

		// get site entries for putting additional infos
		if(auto sites = magdyn.get_child_optional("atom_sites"); sites)
		{
			auto siteiter = (*sites).begin();

			for(std::size_t site_idx = 0; site_idx < std::size_t(m_termstab->rowCount()); ++site_idx)
			{
				// set additional data from exchange term entry
				if(site_idx >= sites->size())
					break;

				// write colour
				std::string rgb = m_sitestab->item(site_idx, COL_SITE_RGB)->text().toStdString();
				siteiter->second.put<std::string>("colour", rgb);

				std::advance(siteiter, 1);
			}
		}

		// get exchange terms entries for putting additional infos
		if(auto terms = magdyn.get_child_optional("exchange_terms"); terms)
		{
			auto termiter = (*terms).begin();

			for(std::size_t term_idx = 0; term_idx < std::size_t(m_termstab->rowCount()); ++term_idx)
			{
				// set additional data from exchange term entry
				if(term_idx >= terms->size())
					break;

				// write colour
				std::string rgb = m_termstab->item(term_idx, COL_XCH_RGB)->text().toStdString();
				termiter->second.put<std::string>("colour", rgb);

				std::advance(termiter, 1);
			}
		}

		pt::ptree node;
		node.put_child("magdyn", magdyn);

		// save to file
		std::ofstream ofstr{filename.toStdString()};
		if(!ofstr)
		{
			QMessageBox::critical(this, "Magnetic Dynamics",
				"Cannot open file for writing.");
			return false;
		}

		ofstr.precision(g_prec);
		pt::write_xml(ofstr, node,
			pt::xml_writer_make_settings('\t', 1, std::string{"utf-8"}));
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Magnetic Dynamics", ex.what());
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
	switch(m_exportFormat->currentData().toInt())
	{
		case EXPORT_HDF5: extension = "HDF5 Files (*.hdf)"; break;
		case EXPORT_GRID: extension = "Takin Grid Files (*.bin)"; break;
		case EXPORT_TEXT: extension = "Text Files (*.txt)"; break;
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
#ifdef USE_HDF5
	std::unique_ptr<H5::H5File> h5file;
#endif
	std::unique_ptr<std::ofstream> ofstr;
	bool file_opened = false;

	const int format = m_exportFormat->currentData().toInt();
	if(format == EXPORT_GRID || format == EXPORT_TEXT)
	{
		ofstr = std::make_unique<std::ofstream>(filename.toStdString());
		ofstr->precision(g_prec);
		file_opened = ofstr->operator bool();
	}

#ifdef USE_HDF5
	else if(format == EXPORT_HDF5)
	{
		h5file = std::make_unique<H5::H5File>(filename.toStdString().c_str(), H5F_ACC_TRUNC);
		h5file->createGroup("meta_infos");
		h5file->createGroup("infos");
		h5file->createGroup("data");
#ifdef WRITE_HDF5_CHUNKS
		h5file->createGroup("chunks");
#endif

		file_opened = true;
	}
#endif

	if(!file_opened)
	{
		QMessageBox::critical(this, "Magnetic Dynamics", "File could not be opened.");
		return false;
	}

	const t_vec_real Qstart = tl2::create<t_vec_real>({
		m_exportStartQ[0]->value(),
		m_exportStartQ[1]->value(),
		m_exportStartQ[2]->value() });
	const t_vec_real Qend = tl2::create<t_vec_real>({
		m_exportEndQ[0]->value(),
		m_exportEndQ[1]->value(),
		m_exportEndQ[2]->value() });

	const t_size num_pts_h = m_exportNumPoints[0]->value();
	const t_size num_pts_k = m_exportNumPoints[1]->value();
	const t_size num_pts_l = m_exportNumPoints[2]->value();

	MagDyn dyn = m_dyn;
	dyn.SetUniteDegenerateEnergies(m_unite_degeneracies->isChecked());
	bool use_weights = m_use_weights->isChecked();
	bool use_projector = m_use_projector->isChecked();

	const t_vec_real dir = Qend - Qstart;
	const t_real inc_h = dir[0] / t_real(num_pts_h);
	const t_real inc_k = dir[1] / t_real(num_pts_k);
	const t_real inc_l = dir[2] / t_real(num_pts_l);
	const t_vec_real Qstep = tl2::create<t_vec_real>({inc_h, inc_k, inc_l});

	// tread pool
	unsigned int num_threads = std::max<unsigned int>(
		1, std::thread::hardware_concurrency()/2);
	asio::thread_pool pool{num_threads};


	using t_taskret = std::deque<
		std::tuple<t_real, t_real, t_real,          // h, k, l
		std::vector<t_real>, std::vector<t_real>,   // E, S
		std::size_t, std::size_t, std::size_t>>;    // h_idx, k_idx, l_idx

	// calculation task
	auto task = [this, use_weights, use_projector, &dyn, inc_l, num_pts_l]
		(t_real h_pos, t_real k_pos, t_real l_pos, std::size_t h_idx, std::size_t k_idx) -> t_taskret
	{
		t_taskret ret;

		// iterate last Q dimension
		for(std::size_t l_idx=0; l_idx<num_pts_l; ++l_idx)
		{
			t_real l = l_pos + inc_l*t_real(l_idx);
			auto energies_and_correlations = dyn.GetEnergies(
				h_pos, k_pos, l, !use_weights);

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

			ret.emplace_back(std::make_tuple(h_pos, k_pos, l, Es, weights, h_idx, k_idx, l_idx));
		}

		return ret;
	};


	// tasks
	using t_task = std::packaged_task<t_taskret(t_real, t_real, t_real, std::size_t, std::size_t)>;
	using t_taskptr = std::shared_ptr<t_task>;
	std::deque<t_taskptr> tasks;
	std::deque<std::future<t_taskret>> futures;

	m_stopRequested = false;
	m_progress->setMinimum(0);
	m_progress->setMaximum(num_pts_h * num_pts_k /** num_pts_l*/);
	m_progress->setValue(0);
	m_status->setText("Starting calculation.");
	DisableInput();

	std::size_t task_idx = 0;
	// iterate first two Q dimensions
	for(std::size_t h_idx=0; h_idx<num_pts_h; ++h_idx)
	{
		if(m_stopRequested)
			break;

		for(std::size_t k_idx=0; k_idx<num_pts_k; ++k_idx)
		{
			if(m_stopRequested)
				break;

			qApp->processEvents();  // process events to see if the stop button was clicked
			if(m_stopRequested)
				break;

			t_vec_real Q = Qstart;
			Q[0] += inc_h*t_real(h_idx);
			Q[1] += inc_k*t_real(k_idx);
			//Q[2] += inc_l*t_real(l_idx);

			// create tasks
			t_taskptr taskptr = std::make_shared<t_task>(task);
			tasks.push_back(taskptr);
			futures.emplace_back(taskptr->get_future());
			asio::post(pool, [taskptr, Q, h_idx, k_idx]()
			{
				(*taskptr)(Q[0], Q[1], Q[2], h_idx, k_idx);
			});

			++task_idx;
			m_progress->setValue(task_idx);
		}
	}

	m_progress->setValue(0);
	m_status->setText("Performing calculation.");

	std::deque<std::uint64_t> hklindices;
	if(format == EXPORT_GRID)  // Takin grid format
	{
		std::uint64_t dummy = 0;  // to be filled by index block index
		ofstr->write(reinterpret_cast<const char*>(&dummy), sizeof(dummy));

		for(int i = 0; i<3; ++i)
		{
			ofstr->write(reinterpret_cast<const char*>(&Qstart[i]), sizeof(Qstart[i]));
			ofstr->write(reinterpret_cast<const char*>(&Qend[i]), sizeof(Qend[i]));
			ofstr->write(reinterpret_cast<const char*>(&Qstep[i]), sizeof(Qstep[i]));
		}

		(*ofstr) << "Takin/Magdyn Grid File Version 2 (doi: https://doi.org/10.5281/zenodo.4117437).";
	}

#ifdef USE_HDF5
	std::vector<t_real> data_energies, data_weights;
	std::vector<std::size_t> data_indices, data_num_branches;
	data_energies.reserve(num_pts_h * num_pts_k * num_pts_l * 32);
	data_weights.reserve(num_pts_h * num_pts_k * num_pts_l * 32);
	data_indices.reserve(num_pts_h * num_pts_k * num_pts_l);
	data_num_branches.reserve(num_pts_h * num_pts_k * num_pts_l);
#endif

	for(std::size_t future_idx=0; future_idx<futures.size(); ++future_idx)
	{
		qApp->processEvents();  // process events to see if the stop button was clicked
		if(m_stopRequested)
		{
			pool.stop();
			break;
		}

		auto results = futures[future_idx].get();

		for(std::size_t result_idx=0; result_idx<results.size(); ++result_idx)
		{
			const auto& result = results[result_idx];
			std::size_t num_branches = std::get<3>(result).size();

			const t_real qh = std::get<0>(result);
			const t_real qk = std::get<1>(result);
			const t_real ql = std::get<2>(result);
			const auto& energies = std::get<3>(result);
			const auto& weights = std::get<4>(result);

			if(format == EXPORT_GRID)           // Takin grid format
			{
				hklindices.push_back(ofstr->tellp());

				// write number of branches
				std::uint32_t _num_branches = std::uint32_t(num_branches);
				ofstr->write(reinterpret_cast<const char*>(&_num_branches), sizeof(_num_branches));
			}
			else if(format == EXPORT_TEXT)      // text format
			{
				(*ofstr) << "Q = " << qh << " " << qk << " " << ql << ":\n";
			}
#ifdef USE_HDF5
			else if(format == EXPORT_HDF5)  // hdf5 format
			{
#ifdef WRITE_HDF5_CHUNKS
				const std::size_t h_idx = std::get<5>(result);
				const std::size_t k_idx = std::get<6>(result);
				const std::size_t l_idx = std::get<7>(result);

				std::ostringstream chunk_name;
				chunk_name << std::hex << h_idx << "_" << k_idx << "_" << l_idx;
				H5::Group data_group = h5file->openGroup("chunks");
				data_group.createGroup(chunk_name.str());

				tl2::set_h5_scalar(*h5file, "chunks/" + chunk_name.str() + "/h", qh);
				tl2::set_h5_scalar(*h5file, "chunks/" + chunk_name.str() + "/k", qk);
				tl2::set_h5_scalar(*h5file, "chunks/" + chunk_name.str() + "/l", ql);

				tl2::set_h5_vector(*h5file, "chunks/" + chunk_name.str() + "/E", energies);
				tl2::set_h5_vector(*h5file, "chunks/" + chunk_name.str() + "/S", weights);
#endif

				// TODO: write this incrementally to the hdf5 file, without these large temporary vectors
				data_num_branches.push_back(energies.size());
				data_indices.push_back(data_energies.size());
				data_energies.insert(data_energies.end(), energies.begin(), energies.end());
				data_weights.insert(data_weights.end(), weights.begin(), weights.end());
			}
#endif

			// iterate energies and weights
			for(std::size_t j=0; j<num_branches; ++j)
			{
				t_real energy = energies[j];
				t_real weight = weights[j];

				if(format == EXPORT_GRID)       // Takin grid format
				{
					// write energies and weights
					ofstr->write(reinterpret_cast<const char*>(&energy), sizeof(energy));
					ofstr->write(reinterpret_cast<const char*>(&weight), sizeof(weight));
				}
				else if(format == EXPORT_TEXT)  // text format
				{
					(*ofstr)
						<< "\tE = " << energy
						<< ", S = " << weight
						<< std::endl;
				}
			}
		}

		m_progress->setValue(future_idx+1);
	}

	pool.join();
	EnableInput();

	if(format == EXPORT_GRID)  // Takin grid format
	{
		std::uint64_t idxblock = ofstr->tellp();

		// write hkl indices
		for(std::uint64_t idx : hklindices)
			ofstr->write(reinterpret_cast<const char*>(&idx), sizeof(idx));

		// write index into index block
		ofstr->seekp(0, std::ios_base::beg);
		ofstr->write(reinterpret_cast<const char*>(&idxblock), sizeof(idxblock));
		ofstr->flush();
	}
#ifdef USE_HDF5
	else if(format == EXPORT_HDF5)
	{
		const char* user = std::getenv("USER");
		if(!user) user = "";
		tl2::set_h5_string<std::string>(*h5file, "meta_infos/type", "takin_grid");
		tl2::set_h5_string<std::string>(*h5file, "meta_infos/description", "Takin/Magdyn grid format");
		tl2::set_h5_string<std::string>(*h5file, "meta_infos/user", user);
		tl2::set_h5_string<std::string>(*h5file, "meta_infos/date", tl2::epoch_to_str<t_real>(tl2::epoch<t_real>()));
		tl2::set_h5_string<std::string>(*h5file, "meta_infos/url", "https://code.ill.fr/scientific-software/takin");
		tl2::set_h5_string<std::string>(*h5file, "meta_infos/doi", "https://doi.org/10.5281/zenodo.4117437");
		tl2::set_h5_string<std::string>(*h5file, "meta_infos/doi_tlibs", "https://doi.org/10.5281/zenodo.5717779");

		tl2::set_h5_string<std::string>(*h5file, "infos/shape", "cuboid");
		tl2::set_h5_vector(*h5file, "infos/Q_start", static_cast<const std::vector<t_real>&>(Qstart));
		tl2::set_h5_vector(*h5file, "infos/Q_end", static_cast<const std::vector<t_real>&>(Qend));
		tl2::set_h5_vector(*h5file, "infos/Q_steps", static_cast<const std::vector<t_real>&>(Qstep));
		tl2::set_h5_vector(*h5file, "infos/Q_dimensions", std::vector<std::size_t>{{ num_pts_h, num_pts_k, num_pts_l }});

		std::vector<std::string> labels{{"h", "k", "l", "E", "S_perp"}};
		tl2::set_h5_string_vector(*h5file, "infos/labels", labels);

		std::vector<std::string> units{{"rlu", "rlu", "rlu", "meV", "a.u."}};
		tl2::set_h5_string_vector(*h5file, "infos/units", units);

		hsize_t index_dims[] = { num_pts_h, num_pts_k, num_pts_l };
		tl2::set_h5_multidim(*h5file, "data/indices", 3, index_dims, data_indices.data());
		tl2::set_h5_multidim(*h5file, "data/branches", 3, index_dims, data_num_branches.data());
		//tl2::set_h5_vector(*h5file, "data/indices", data_indices);
		//tl2::set_h5_vector(*h5file, "data/branches", data_num_branches);

		tl2::set_h5_vector(*h5file, "data/energies", data_energies);
		tl2::set_h5_vector(*h5file, "data/weights", data_weights);

		h5file->close();
	}
#endif

	if(m_stopRequested)
		m_status->setText("Calculation stopped.");
	else
		m_status->setText("Calculation finished.");

	return true;
}
