/**
 * brillouin zone tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date Maz-2022
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

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <iostream>
#include <fstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace pt = boost::property_tree;

#include "../structfact/loadcif.h"
#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/algos.h"

using namespace tl2_ops;


void BZDlg::Load()
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


		pt::ptree node;

		std::ifstream ifstr{filename.toStdString()};
		pt::read_xml(ifstr, node);

		// check signature
		if(auto opt = node.get_optional<std::string>("bz.meta.info");
			!opt || *opt!=std::string{"bz_tool"})
		{
			QMessageBox::critical(this, "Brillouin Zones",
				"Unrecognised file format.");
			return;
		}


		// clear old items
		DelTabItem(-1);

		if(auto opt = node.get_optional<t_real>("bz.xtal.a"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editA->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("bz.xtal.b"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editB->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("bz.xtal.c"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editC->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("bz.xtal.alpha"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editAlpha->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("bz.xtal.beta"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editBeta->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("bz.xtal.gamma"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editGamma->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<int>("bz.order"); opt)
		{
			m_maxBZ->setValue(*opt);
		}
		if(auto opt = node.get_optional<int>("bz.sg_idx"); opt)
		{
			m_comboSG->setCurrentIndex(*opt);
		}


		// symops
		if(auto symops = node.get_child_optional("bz.symops"); symops)
		{
			std::size_t opidx = 0;
			for(const auto &symop : *symops)
			{
				auto optOp = symop.second.get<std::string>(
					"op", "1 0 0 0  0 1 0 0  0 0 1 0  0 0 0 1");

				AddTabItem(-1, StrToOp(optOp));
				++opidx;
			}
		}
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Brillouin Zones", ex.what());
	}


	m_ignoreCalc = 0;
	CalcB(false);
	CalcBZ();
}


void BZDlg::Save()
{
	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getSaveFileName(
		this, "Save File", dirLast, "XML Files (*.xml *.XML)");
	if(filename=="")
		return;
	m_sett->setValue("dir", QFileInfo(filename).path());


	pt::ptree node;
	node.put<std::string>("bz.meta.info", "bz_tool");
	node.put<std::string>("bz.meta.date", tl2::epoch_to_str<t_real>(tl2::epoch<t_real>()));


	// lattice
	t_real a,b,c, alpha,beta,gamma;
	std::istringstream{m_editA->text().toStdString()} >> a;
	std::istringstream{m_editB->text().toStdString()} >> b;
	std::istringstream{m_editC->text().toStdString()} >> c;
	std::istringstream{m_editAlpha->text().toStdString()} >> alpha;
	std::istringstream{m_editBeta->text().toStdString()} >> beta;
	std::istringstream{m_editGamma->text().toStdString()} >> gamma;

	node.put<t_real>("bz.xtal.a", a);
	node.put<t_real>("bz.xtal.b", b);
	node.put<t_real>("bz.xtal.c", c);
	node.put<t_real>("bz.xtal.alpha", alpha);
	node.put<t_real>("bz.xtal.beta", beta);
	node.put<t_real>("bz.xtal.gamma", gamma);
	node.put<int>("bz.order", m_maxBZ->value());
	node.put<int>("bz.sg_idx", m_comboSG->currentIndex());

	// symop list
	for(int row=0; row<m_symops->rowCount(); ++row)
	{
		pt::ptree itemNode;
		itemNode.put<std::string>("op", m_symops->item(row, COL_OP)->text().toStdString());

		node.add_child("bz.symops.symop", itemNode);
	}

	std::ofstream ofstr{filename.toStdString()};
	if(!ofstr)
	{
		QMessageBox::critical(this, "Brillouin Zones", "Cannot open file for writing.");
		return;
	}
	ofstr.precision(g_prec);
	pt::write_xml(ofstr, node, pt::xml_writer_make_settings('\t', 1, std::string{"utf-8"}));
}


/**
 * load a CIF
 */
void BZDlg::ImportCIF()
{
	m_ignoreCalc = 1;

	try
	{
		QString dirLast = m_sett->value("dir_cif", "").toString();
		QString filename = QFileDialog::getOpenFileName(
			this, "Import CIF", dirLast, "CIF Files (*.cif *.CIF)");
		if(filename=="" || !QFile::exists(filename))
			return;
		m_sett->setValue("dir_cif", QFileInfo(filename).path());

		auto [errstr, atoms, generatedatoms, atomnames, lattice, symops] =
			load_cif<t_vec, t_mat>(filename.toStdString(), g_eps);
		if(errstr != "")
		{
			QMessageBox::critical(this, "CIF Importer", errstr.c_str());
			return;
		}


		// clear old symops
		DelTabItem(-1);

		// lattice
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << lattice.a;
			m_editA->setText(ostr.str().c_str());
		}
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << lattice.b;
			m_editB->setText(ostr.str().c_str());
		}
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << lattice.c;
			m_editC->setText(ostr.str().c_str());
		}
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << lattice.alpha;
			m_editAlpha->setText(ostr.str().c_str());
		}
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << lattice.beta;
			m_editBeta->setText(ostr.str().c_str());
		}
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << lattice.gamma;
			m_editGamma->setText(ostr.str().c_str());
		}


		// symops
		for(std::size_t opnum=0; opnum<symops.size(); ++opnum)
		{
			AddTabItem(-1, symops[opnum]);
		}
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Brillouin Zones", ex.what());
	}


	m_ignoreCalc = 0;
	CalcB(false);
	CalcBZ();
}
