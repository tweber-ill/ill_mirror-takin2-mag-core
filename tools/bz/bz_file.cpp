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
#include <QtSvg/QSvgGenerator>

#include <iostream>
#include <fstream>
#include <cstdlib>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace pt = boost::property_tree;

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
namespace algo = boost::algorithm;

#include "../structfact/loadcif.h"
#include "tlibs2/libs/algos.h"

using namespace tl2_ops;


void BZDlg::NewFile()
{
	m_ignoreCalc = 1;

	// clear old tables
	DelSymOpTabItem(-1);
	DelFormulaTabItem(-1);

	// set some defaults
	m_comboSG->setCurrentIndex(0);
	m_editA->setValue(5.);
	m_editB->setValue(5.);
	m_editC->setValue(5.);
	m_editAlpha->setValue(90.);
	m_editBeta->setValue(90.);
	m_editGamma->setValue(90.);

	m_cutX->setValue(1);
	m_cutY->setValue(0);
	m_cutZ->setValue(0);
	m_cutNX->setValue(0);
	m_cutNY->setValue(0);
	m_cutNZ->setValue(1);
	m_cutD->setValue(0);
	m_BZDrawOrder->setValue(4);
	m_BZCalcOrder->setValue(4);

	m_ignoreCalc = 0;
	CalcB(true);
}


bool BZDlg::Load(const QString& filename)
{
	m_ignoreCalc = 1;

	try
	{
		pt::ptree node;

		std::ifstream ifstr{filename.toStdString()};
		pt::read_xml(ifstr, node);

		// check signature
		if(auto opt = node.get_optional<std::string>("bz.meta.info");
			!opt || *opt!=std::string{"bz_tool"})
		{
			QMessageBox::critical(this, "Brillouin Zones",
				"Unrecognised file format.");
			m_ignoreCalc = 0;
			return false;
		}


		// clear old items
		DelSymOpTabItem(-1);

		if(auto opt = node.get_optional<t_real>("bz.xtal.a"); opt)
			m_editA->setValue(*opt);
		if(auto opt = node.get_optional<t_real>("bz.xtal.b"); opt)
			m_editB->setValue(*opt);
		if(auto opt = node.get_optional<t_real>("bz.xtal.c"); opt)
			m_editC->setValue(*opt);
		if(auto opt = node.get_optional<t_real>("bz.xtal.alpha"); opt)
			m_editAlpha->setValue(*opt);
		if(auto opt = node.get_optional<t_real>("bz.xtal.beta"); opt)
			m_editBeta->setValue(*opt);
		if(auto opt = node.get_optional<t_real>("bz.xtal.gamma"); opt)
			m_editGamma->setValue(*opt);
		if(auto opt = node.get_optional<int>("bz.order"); opt)
			m_BZCalcOrder->setValue(*opt);
		if(auto opt = node.get_optional<int>("bz.cut.order"); opt)
			m_BZDrawOrder->setValue(*opt);
		if(auto opt = node.get_optional<t_real>("bz.cut.x"); opt)
			m_cutX->setValue(*opt);
		if(auto opt = node.get_optional<t_real>("bz.cut.y"); opt)
			m_cutY->setValue(*opt);
		if(auto opt = node.get_optional<t_real>("bz.cut.z"); opt)
			m_cutZ->setValue(*opt);
		if(auto opt = node.get_optional<t_real>("bz.cut.nx"); opt)
			m_cutNX->setValue(*opt);
		if(auto opt = node.get_optional<t_real>("bz.cut.ny"); opt)
			m_cutNY->setValue(*opt);
		if(auto opt = node.get_optional<t_real>("bz.cut.nz"); opt)
			m_cutNZ->setValue(*opt);
		if(auto opt = node.get_optional<t_real>("bz.cut.d"); opt)
			m_cutD->setValue(*opt);
		if(auto opt = node.get_optional<int>("bz.sg_idx"); opt)
			m_comboSG->setCurrentIndex(*opt);


		// symops
		if(auto symops = node.get_child_optional("bz.symops"); symops)
		{
			for(const auto &symop : *symops)
			{
				auto optOp = symop.second.get<std::string>(
					"op", "1 0 0 0  0 1 0 0  0 0 1 0  0 0 0 1");

				AddSymOpTabItem(-1, StrToOp(optOp));
			}
		}

		// formulas
		if(auto formulas = node.get_child_optional("bz.formulas"); formulas)
		{
			for(const auto &formula : *formulas)
			{
				std::string expr = formula.second.get<std::string>("expr", "");
				AddFormulaTabItem(-1, expr);
			}
		}
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Brillouin Zones", ex.what());
		m_ignoreCalc = 0;
		return false;
	}


	m_ignoreCalc = 0;
	CalcB(true);

	return true;
}


bool BZDlg::Save(const QString& filename)
{
	pt::ptree node;

	// meta infos
	const char* user = std::getenv("USER");
	if(!user) user = "";

	node.put<std::string>("bz.meta.info", "bz_tool");
	node.put<std::string>("bz.meta.date", tl2::epoch_to_str<t_real>(tl2::epoch<t_real>()));
	node.put<std::string>("bz.meta.user", user);
	node.put<std::string>("bz.meta.url", "https://code.ill.fr/scientific-software/takin");
	node.put<std::string>("bz.meta.doi", "https://doi.org/10.5281/zenodo.4117437");
	node.put<std::string>("bz.meta.doi_tlibs", "https://doi.org/10.5281/zenodo.5717779");

	// lattice
	node.put<t_real>("bz.xtal.a", m_editA->value());
	node.put<t_real>("bz.xtal.b", m_editB->value());
	node.put<t_real>("bz.xtal.c", m_editC->value());
	node.put<t_real>("bz.xtal.alpha", m_editAlpha->value());
	node.put<t_real>("bz.xtal.beta", m_editBeta->value());
	node.put<t_real>("bz.xtal.gamma", m_editGamma->value());
	node.put<int>("bz.order", m_BZCalcOrder->value());
	node.put<int>("bz.cut.order", m_BZDrawOrder->value());
	node.put<t_real>("bz.cut.x", m_cutX->value());
	node.put<t_real>("bz.cut.y", m_cutY->value());
	node.put<t_real>("bz.cut.z", m_cutZ->value());
	node.put<t_real>("bz.cut.nx", m_cutNX->value());
	node.put<t_real>("bz.cut.ny", m_cutNY->value());
	node.put<t_real>("bz.cut.nz", m_cutNZ->value());
	node.put<t_real>("bz.cut.d", m_cutD->value());
	node.put<int>("bz.sg_idx", m_comboSG->currentIndex());

	// symop list
	for(int row=0; row<m_symops->rowCount(); ++row)
	{
		std::string opstr = m_symops->item(row, COL_OP)->text().toStdString();
		algo::replace_all(opstr, "\n", " ");

		pt::ptree itemNode;
		itemNode.put<std::string>("op", opstr);
		node.add_child("bz.symops.symop", itemNode);
	}

	// formula list
	for(int row=0; row<m_formulas->rowCount(); ++row)
	{
		std::string opstr = m_formulas->item(row, COL_FORMULA)->text().toStdString();
		algo::replace_all(opstr, "\n", " ");

		pt::ptree itemNode;
		itemNode.put<std::string>("expr", opstr);
		node.add_child("bz.formulas.formula", itemNode);
	}

	std::ofstream ofstr{filename.toStdString()};
	if(!ofstr)
	{
		QMessageBox::critical(this, "Brillouin Zones", "Cannot open file for writing.");
		return false;
	}
	ofstr.precision(g_prec);
	pt::write_xml(ofstr, node, pt::xml_writer_make_settings('\t', 1, std::string{"utf-8"}));

	return true;
}


void BZDlg::Load()
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


void BZDlg::Save()
{
	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getSaveFileName(
		this, "Save File", dirLast, "XML Files (*.xml *.XML)");
	if(filename=="")
		return;

	if(Save(filename))
	{
		m_sett->setValue("dir", QFileInfo(filename).path());
		m_recent.AddRecentFile(filename);
	}
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
		DelSymOpTabItem(-1);

		// lattice
		m_editA->setValue(lattice.a);
		m_editB->setValue(lattice.b);
		m_editC->setValue(lattice.c);
		m_editAlpha->setValue(lattice.alpha);
		m_editBeta->setValue(lattice.beta);
		m_editGamma->setValue(lattice.gamma);


		// symops
		for(std::size_t opnum=0; opnum<symops.size(); ++opnum)
		{
			AddSymOpTabItem(-1, symops[opnum]);
		}
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Brillouin Zones", ex.what());
	}


	m_ignoreCalc = 0;
	CalcB(true);
}


void BZDlg::SaveCutSVG()
{
	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getSaveFileName(
		this, "Save SVG File", dirLast, "SVG Files (*.svg *.SVG)");
	if(filename=="")
		return;
	m_sett->setValue("dir", QFileInfo(filename).path());

	QSvgGenerator svg;
	svg.setSize(QSize{800, 800});
	svg.setViewBox(QRect{0, 0, 800, 800});
	svg.setFileName(filename);
	svg.setTitle("Brillouin Zone Cut");
	svg.setDescription("Created with Takin (https://doi.org/10.5281/zenodo.4117437).");

	QPainter painter;
	painter.begin(&svg);
	m_bzscene->render(&painter);
	painter.end();
}
