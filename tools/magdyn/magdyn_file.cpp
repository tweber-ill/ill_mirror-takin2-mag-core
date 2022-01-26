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

#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace pt = boost::property_tree;

#include "tlibs2/libs/log.h"

extern int g_prec;


/**
 * load settings
 */
void MagDynDlg::Load()
{
	m_ignoreCalc = 1;

	try
	{
		QString dirLast = m_sett->value("dir", "").toString();
		QString filename = QFileDialog::getOpenFileName(
			this, "Load File", dirLast, "XML Files (*.xml *.XML)");
		if(filename=="" || !QFile::exists(filename))
			return;
		m_sett->setValue("dir", QFileInfo(filename).path());

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
			return;
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
		if(auto optVal = magdyn.get_optional<bool>("config.use_DMI"))
			m_use_dmi->setChecked(*optVal);
		if(auto optVal = magdyn.get_optional<bool>("config.use_field"))
			m_use_field->setChecked(*optVal);
		if(auto optVal = magdyn.get_optional<bool>("config.use_temperature"))
			m_use_temperature->setChecked(*optVal);
		if(auto optVal = magdyn.get_optional<bool>("config.use_weights"))
			m_use_weights->setChecked(*optVal);
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

		m_dyn.Load(magdyn);

		// external field
		m_field_dir[0]->setValue(m_dyn.GetExternalField().dir[0].real());
		m_field_dir[1]->setValue(m_dyn.GetExternalField().dir[1].real());
		m_field_dir[2]->setValue(m_dyn.GetExternalField().dir[2].real());
		m_field_mag->setValue(m_dyn.GetExternalField().mag);
		m_align_spins->setChecked(m_dyn.GetExternalField().align_spins);

		// bragg peak
		const t_vec& bragg = m_dyn.GetBraggPeak();
		if(bragg.size() == 3)
		{
			m_bragg[0]->setValue(bragg[0].real());
			m_bragg[1]->setValue(bragg[1].real());
			m_bragg[2]->setValue(bragg[2].real());
		}

		// temperature
		t_real temp = m_dyn.GetTemperature();
		if(temp >= 0.)
			m_temperature->setValue(temp);

		// clear old tables
		DelTabItem(m_sitestab, -1);
		DelTabItem(m_termstab, -1);

		// atom sites
		for(const auto &site : m_dyn.GetAtomSites())
		{
			AddSiteTabItem(-1,
				site.name,
				site.pos[0].real(), site.pos[1].real(), site.pos[2].real(),
				site.spin_dir[0].real(), site.spin_dir[1].real(), site.spin_dir[2].real(),
				site.spin_mag);
		}

		// exchange terms
		for(const auto& term : m_dyn.GetExchangeTerms())
		{
			AddTermTabItem(-1,
				term.name, term.atom1, term.atom2,
				term.dist[0].real(), term.dist[1].real(), term.dist[2].real(),
				term.J.real(),
				term.dmi[0].real(), term.dmi[1].real(), term.dmi[2].real());
		}
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Magnon Dynamics", ex.what());
	}

	m_ignoreCalc = 0;
	CalcDispersion();
	CalcHamiltonian();

	StructPlotSync();
}


/**
 * save current settings
 */
void MagDynDlg::Save()
{
	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getSaveFileName(this, "Save File", dirLast, "XML Files (*.xml *.XML)");
	if(filename=="")
		return;
	m_sett->setValue("dir", QFileInfo(filename).path());

	// properties tree
	pt::ptree magdyn;

	magdyn.put<std::string>("meta.info", "magdyn_tool");
	magdyn.put<std::string>("meta.date",
		tl2::epoch_to_str<t_real>(tl2::epoch<t_real>()));

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
	magdyn.put<t_size>("config.weight_scale", m_weight_scale->value());
	magdyn.put<bool>("config.use_DMI", m_use_dmi->isChecked());
	magdyn.put<bool>("config.use_field", m_use_field->isChecked());
	magdyn.put<bool>("config.use_temperature", m_use_temperature->isChecked());
	magdyn.put<bool>("config.use_weights", m_use_weights->isChecked());
	magdyn.put<bool>("config.use_projector", m_use_projector->isChecked());
	magdyn.put<t_real>("config.field_axis_h", m_rot_axis[0]->value());
	magdyn.put<t_real>("config.field_axis_k", m_rot_axis[1]->value());
	magdyn.put<t_real>("config.field_axis_l", m_rot_axis[2]->value());
	magdyn.put<t_real>("config.field_angle", m_rot_angle->value());
	magdyn.put<t_real>("config.spacegroup_index", m_comboSG->currentIndex());
	m_dyn.Save(magdyn);

	pt::ptree node;
	node.put_child("magdyn", magdyn);

	// save to file
	std::ofstream ofstr{filename.toStdString()};
	if(!ofstr)
	{
		QMessageBox::critical(this, "Magnon Dynamics",
			"Cannot open file for writing.");
		return;
	}
	ofstr.precision(g_prec);
	pt::write_xml(ofstr, node,
		pt::xml_writer_make_settings('\t', 1, std::string{"utf-8"}));
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
		this, "Save Figure", dirLast, "PDf Files (*.pdf *.PDF)");
	if(filename=="")
		return;
	m_sett->setValue("dir", QFileInfo(filename).path());

	m_plot->savePdf(filename);
}