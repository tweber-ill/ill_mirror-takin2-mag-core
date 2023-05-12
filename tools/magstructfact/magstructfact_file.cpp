/**
 * magnetic structure factor tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date Jan-2019
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 28-Dec-2018 from my privately developed "misc" project (https://github.com/t-weber/misc).
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2021  Tobias WEBER (Institut Laue-Langevin (ILL),
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

#include "magstructfact.h"

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <cstdlib>

#include <boost/scope_exit.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
namespace algo = boost::algorithm;
namespace pt = boost::property_tree;

#include "../structfact/loadcif.h"
#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/phys.h"
#include "tlibs2/libs/algos.h"
#include "tlibs2/libs/qt/helper.h"

using namespace tl2_ops;


void MagStructFactDlg::Load()
{
	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getOpenFileName(this, "Load File", dirLast, "XML Files (*.xml *.XML)");
	if(filename=="" || !QFile::exists(filename))
		return;

	if(Load(filename))
		m_sett->setValue("dir", QFileInfo(filename).path());
}


bool MagStructFactDlg::Load(const QString& filename)
{
	BOOST_SCOPE_EXIT(this_)
	{
		this_->m_ignoreCalc = false;
	} BOOST_SCOPE_EXIT_END
	m_ignoreCalc = true;

	try
	{
		pt::ptree node;

		std::ifstream ifstr{filename.toStdString()};
		pt::read_xml(ifstr, node);

		// check signature
		if(auto optInfo = node.get_optional<std::string>("sfact.meta.info");
			!optInfo || !(*optInfo==std::string{"magsfact_tool"} || *optInfo==std::string{"sfact_tool"}))
		{
			QMessageBox::critical(this, "Structure Factors", "Unrecognised file format.");
			return false;
		}
		else if(*optInfo == std::string{"sfact_tool"})
		{
			QMessageBox::warning(this, "Structure Factors", "File only contains nuclear information. Trying to load.");
		}


		// clear old tables
		DelTabItem(-1);
		DelPropItem(-1);


		// lattice
		if(auto opt = node.get_optional<t_real>("sfact.xtal.a"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editA->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("sfact.xtal.b"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editB->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("sfact.xtal.c"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editC->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("sfact.xtal.alpha"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editAlpha->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("sfact.xtal.beta"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editBeta->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("sfact.xtal.gamma"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editGamma->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<int>("sfact.order"); opt)
		{
			m_maxBZ->setValue(*opt);
		}
		if(auto opt = node.get_optional<int>("sfact.scorder_x"); opt)
		{
			m_maxSC[0]->setValue(*opt);
		}
		if(auto opt = node.get_optional<int>("sfact.scorder_y"); opt)
		{
			m_maxSC[1]->setValue(*opt);
		}
		if(auto opt = node.get_optional<int>("sfact.scorder_z"); opt)
		{
			m_maxSC[2]->setValue(*opt);
		}
		if(auto opt = node.get_optional<int>("sfact.removezeroes"); opt)
		{
			m_RemoveZeroes->setChecked(*opt != 0);
		}
		if(auto opt = node.get_optional<int>("sfact.sg_idx"); opt)
		{
			m_comboSG->setCurrentIndex(*opt);
		}


		// fourier components
		if(auto nuclei = node.get_child_optional("sfact.nuclei"); nuclei)
		{
			for(const auto &nucl : *nuclei)
			{
				auto optName = nucl.second.get<std::string>("name", "n/a");
				auto optMMag = nucl.second.get<t_real>("M_mag", 1.);
				auto optX = nucl.second.get<t_real>("x", 0.);
				auto optY = nucl.second.get<t_real>("y", 0.);
				auto optZ = nucl.second.get<t_real>("z", 0.);
				auto optReMX = nucl.second.get<t_real>("ReMx", 0.);
				auto optReMY = nucl.second.get<t_real>("ReMy", 0.);
				auto optReMZ = nucl.second.get<t_real>("ReMz", 0.);
				auto optImMX = nucl.second.get<t_real>("ImMx", 0.);
				auto optImMY = nucl.second.get<t_real>("ImMy", 0.);
				auto optImMZ = nucl.second.get<t_real>("ImMz", 0.);
				auto optRad = nucl.second.get<t_real>("rad", 1.);
				auto optCol = nucl.second.get<std::string>("col", "#ff0000");

				AddTabItem(-1, optName, optMMag, optX,  optY, optZ,
					optReMX, optReMY, optReMZ, optImMX, optImMY, optImMZ,
					optRad, optCol);
			}
		}


		// propagation vectors
		if(auto propvecs = node.get_child_optional("sfact.propvecs"); propvecs)
		{
			for(const auto &propvec : *propvecs)
			{
				auto optName = propvec.second.get<std::string>("name", "n/a");
				auto optX = propvec.second.get<t_real>("x", 0.);
				auto optY = propvec.second.get<t_real>("y", 0.);
				auto optZ = propvec.second.get<t_real>("z", 0.);
				auto optConj = propvec.second.get<int>("conjFC", 0);

				AddPropItem(-1, optName, optX,  optY, optZ, optConj!=0);
			}
		}
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Structure Factors", ex.what());
		return false;
	}


	CalcB(false);
	Calc();

	return true;
}


void MagStructFactDlg::Save()
{
	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getSaveFileName(this, "Save File", dirLast, "XML Files (*.xml *.XML)");
	if(filename=="")
		return;

	if(Save(filename))
		m_sett->setValue("dir", QFileInfo(filename).path());
}


bool MagStructFactDlg::Save(const QString& filename)
{
	pt::ptree node;

	// meta infos
        const char* user = std::getenv("USER");
        if(!user) user = "";

	node.put<std::string>("sfact.meta.info", "magsfact_tool");
	node.put<std::string>("sfact.meta.date", tl2::epoch_to_str<t_real>(tl2::epoch<t_real>()));
	node.put<std::string>("sfact.meta.user", user);
	node.put<std::string>("sfact.meta.url", "https://code.ill.fr/scientific-software/takin");
	node.put<std::string>("sfact.meta.doi", "https://doi.org/10.5281/zenodo.4117437");
	node.put<std::string>("sfact.meta.doi_tlibs", "https://doi.org/10.5281/zenodo.5717779");


	// lattice
	t_real a,b,c, alpha,beta,gamma;
	std::istringstream{m_editA->text().toStdString()} >> a;
	std::istringstream{m_editB->text().toStdString()} >> b;
	std::istringstream{m_editC->text().toStdString()} >> c;
	std::istringstream{m_editAlpha->text().toStdString()} >> alpha;
	std::istringstream{m_editBeta->text().toStdString()} >> beta;
	std::istringstream{m_editGamma->text().toStdString()} >> gamma;

	node.put<t_real>("sfact.xtal.a", a);
	node.put<t_real>("sfact.xtal.b", b);
	node.put<t_real>("sfact.xtal.c", c);
	node.put<t_real>("sfact.xtal.alpha", alpha);
	node.put<t_real>("sfact.xtal.beta", beta);
	node.put<t_real>("sfact.xtal.gamma", gamma);
	node.put<int>("sfact.order", m_maxBZ->value());
	node.put<int>("sfact.scorder_x", m_maxSC[0]->value());
	node.put<int>("sfact.scorder_y", m_maxSC[1]->value());
	node.put<int>("sfact.scorder_z", m_maxSC[2]->value());
	node.put<int>("sfact.removezeroes", m_RemoveZeroes->isChecked());
	node.put<int>("sfact.sg_idx", m_comboSG->currentIndex());


	// fourier component list
	for(int row=0; row<m_nuclei->rowCount(); ++row)
	{
		t_real MMag{}, x{},y{},z{}, ReMx{}, ReMy{}, ReMz{}, ImMx{}, ImMy{}, ImMz{}, scale{};
		std::istringstream{m_nuclei->item(row, COL_M_MAG)->text().toStdString()} >> MMag;
		std::istringstream{m_nuclei->item(row, COL_X)->text().toStdString()} >> x;
		std::istringstream{m_nuclei->item(row, COL_Y)->text().toStdString()} >> y;
		std::istringstream{m_nuclei->item(row, COL_Z)->text().toStdString()} >> z;
		std::istringstream{m_nuclei->item(row, COL_ReM_X)->text().toStdString()} >> ReMx;
		std::istringstream{m_nuclei->item(row, COL_ReM_Y)->text().toStdString()} >> ReMy;
		std::istringstream{m_nuclei->item(row, COL_ReM_Z)->text().toStdString()} >> ReMz;
		std::istringstream{m_nuclei->item(row, COL_ImM_X)->text().toStdString()} >> ImMx;
		std::istringstream{m_nuclei->item(row, COL_ImM_Y)->text().toStdString()} >> ImMy;
		std::istringstream{m_nuclei->item(row, COL_ImM_Z)->text().toStdString()} >> ImMz;
		std::istringstream{m_nuclei->item(row, COL_RAD)->text().toStdString()} >> scale;

		pt::ptree itemNode;
		itemNode.put<std::string>("name", m_nuclei->item(row, COL_NAME)->text().toStdString());
		itemNode.put<t_real>("M_mag", MMag);
		itemNode.put<t_real>("x", x);
		itemNode.put<t_real>("y", y);
		itemNode.put<t_real>("z", z);
		itemNode.put<t_real>("ReMx", ReMx);
		itemNode.put<t_real>("ReMy", ReMy);
		itemNode.put<t_real>("ReMz", ReMz);
		itemNode.put<t_real>("ImMx", ImMx);
		itemNode.put<t_real>("ImMy", ImMy);
		itemNode.put<t_real>("ImMz", ImMz);
		itemNode.put<t_real>("rad", scale);
		itemNode.put<std::string>("col", m_nuclei->item(row, COL_COL)->text().toStdString());

		node.add_child("sfact.nuclei.nucleus", itemNode);
	}


	// propagation vectors list
	for(int row=0; row<m_propvecs->rowCount(); ++row)
	{
		t_real x{},y{},z{};
		int iConj{0};
		std::istringstream{m_propvecs->item(row, PROP_COL_X)->text().toStdString()} >> x;
		std::istringstream{m_propvecs->item(row, PROP_COL_Y)->text().toStdString()} >> y;
		std::istringstream{m_propvecs->item(row, PROP_COL_Z)->text().toStdString()} >> z;
		std::istringstream{m_propvecs->item(row, PROP_COL_CONJ)->text().toStdString()} >> iConj;

		pt::ptree itemNode;
		itemNode.put<std::string>("name", m_propvecs->item(row, PROP_COL_NAME)->text().toStdString());
		itemNode.put<t_real>("x", x);
		itemNode.put<t_real>("y", y);
		itemNode.put<t_real>("z", z);
		itemNode.put<int>("conjFC", iConj);

		node.add_child("sfact.propvecs.vec", itemNode);
	}


	std::ofstream ofstr{filename.toStdString()};
	if(!ofstr)
	{
		QMessageBox::critical(this, "Structure Factors", "Cannot open file for writing.");
		return false;
	}

	ofstr.precision(g_prec);
	pt::write_xml(ofstr, node, pt::xml_writer_make_settings('\t', 1, std::string{"utf-8"}));

	return true;
}


/**
 * load an mCIF
 */
void MagStructFactDlg::ImportCIF()
{/*
	QString dirLast = m_sett->value("dir_cif", "").toString();
	QString filename = QFileDialog::getOpenFileName(this, "Import CIF", dirLast, "CIF Files (*.cif *.CIF)");
	if(filename=="" || !QFile::exists(filename))
		return;
	m_sett->setValue("dir_cif", QFileInfo(filename).path());

	auto [errstr, atoms, generatedatoms, atomnames, lattice, symops] =
		load_cif<t_vec, t_mat>(filename.toStdString(), g_eps);
	if(errstr)
	{
		QMessageBox::critical(this, "Structure Factors", errstr);
		return;
	}


	// clear old nuclei
	DelTabItem(-1);
	DelPropItem(-1);

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
	CalcB(false);


	// atoms
	std::mt19937 gen{tl2::epoch<unsigned int>()};
	for(std::size_t atomnum=0; atomnum<atoms.size(); ++atomnum)
	{
		// random colour
		std::ostringstream ostrcol;
		std::uniform_int_distribution<int> dist{0, 255};
		ostrcol << "#" << std::hex << std::setw(2) << std::setfill('0') << dist(gen)
			<< std::setw(2) << std::setfill('0') << dist(gen)
			<< std::setw(2) << std::setfill('0') << dist(gen);

		for(std::size_t symnr=0; symnr<generatedatoms[atomnum].size(); ++symnr)
		{
			AddTabItem(-1, atomnames[atomnum], 0, 0,
				generatedatoms[atomnum][symnr][0],  generatedatoms[atomnum][symnr][1], generatedatoms[atomnum][symnr][2],
				1, ostrcol.str());
		}
	}*/
}
