/**
 * structure factor tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date Dec-2018
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

#include "structfact.h"

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>

#include <iostream>
#include <fstream>
#include <random>
#include <cstdlib>

#include <boost/scope_exit.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace pt = boost::property_tree;

#include "loadcif.h"
#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/algos.h"

using namespace tl2_ops;


std::vector<std::string> StructFactDlg::g_default_colours
{{
	"#ff0000", "#0000ff", "#00ff00", "#000000",
}};


void StructFactDlg::Load()
{
	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getOpenFileName(this, "Load File", dirLast, "XML Files (*.xml *.XML)");
	if(filename=="" || !QFile::exists(filename))
		return;

	if(Load(filename))
		m_sett->setValue("dir", QFileInfo(filename).path());

}

bool StructFactDlg::Load(const QString& filename)
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
		if(auto opt = node.get_optional<std::string>("sfact.meta.info"); !opt || *opt!=std::string{"sfact_tool"})
		{
			QMessageBox::critical(this, "Structure Factors", "Unrecognised file format.");
			return false;
		}


		// clear old nuclei
		DelTabItem(-1);

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
		if(auto opt = node.get_optional<int>("sfact.removezeroes"); opt)
		{
			m_RemoveZeroes->setChecked(*opt != 0);
		}
		if(auto opt = node.get_optional<int>("sfact.sg_idx"); opt)
		{
			m_comboSG->setCurrentIndex(*opt);
		}


		// nuclei
		if(auto nuclei = node.get_child_optional("sfact.nuclei"); nuclei)
		{
			std::size_t nucIdx = 0;
			for(const auto &nucl : *nuclei)
			{
				auto optName = nucl.second.get<std::string>("name", "n/a");
				auto optbRe = nucl.second.get<t_real>("b_Re", 0.);
				auto optbIm = nucl.second.get<t_real>("b_Im", 0.);
				auto optX = nucl.second.get<t_real>("x", 0.);
				auto optY = nucl.second.get<t_real>("y", 0.);
				auto optZ = nucl.second.get<t_real>("z", 0.);
				auto optRad = nucl.second.get<t_real>("rad", 1.);
				auto optCol = nucl.second.get<std::string>("col", g_default_colours[nucIdx%g_default_colours.size()]);

				AddTabItem(-1, optName, optbRe, optbIm, optX,  optY, optZ, optRad, optCol);
				++nucIdx;
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


void StructFactDlg::Save()
{
	QString dirLast = m_sett->value("dir", "").toString();
	QString filename = QFileDialog::getSaveFileName(this, "Save File", dirLast, "XML Files (*.xml *.XML)");
	if(filename=="")
		return;

	if(Save(filename))
		m_sett->setValue("dir", QFileInfo(filename).path());
}


bool StructFactDlg::Save(const QString& filename)
{
	pt::ptree node;

	// meta infos
        const char* user = std::getenv("USER");
        if(!user) user = "";

	node.put<std::string>("sfact.meta.info", "sfact_tool");
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
	node.put<int>("sfact.removezeroes", m_RemoveZeroes->isChecked());
	node.put<int>("sfact.sg_idx", m_comboSG->currentIndex());

	// nucleus list
	for(int row=0; row<m_nuclei->rowCount(); ++row)
	{
		t_real bRe{},bIm{}, x{},y{},z{}, scale{};
		std::istringstream{m_nuclei->item(row, COL_SCATLEN_RE)->text().toStdString()} >> bRe;
		std::istringstream{m_nuclei->item(row, COL_SCATLEN_IM)->text().toStdString()} >> bIm;
		std::istringstream{m_nuclei->item(row, COL_X)->text().toStdString()} >> x;
		std::istringstream{m_nuclei->item(row, COL_Y)->text().toStdString()} >> y;
		std::istringstream{m_nuclei->item(row, COL_Z)->text().toStdString()} >> z;
		std::istringstream{m_nuclei->item(row, COL_RAD)->text().toStdString()} >> scale;

		pt::ptree itemNode;
		itemNode.put<std::string>("name", m_nuclei->item(row, COL_NAME)->text().toStdString());
		itemNode.put<t_real>("b_Re", bRe);
		itemNode.put<t_real>("b_Im", bIm);
		itemNode.put<t_real>("x", x);
		itemNode.put<t_real>("y", y);
		itemNode.put<t_real>("z", z);
		itemNode.put<t_real>("rad", scale);
		itemNode.put<std::string>("col", m_nuclei->item(row, COL_COL)->text().toStdString());

		node.add_child("sfact.nuclei.nucleus", itemNode);
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
 * load a TAZ file
 */
void StructFactDlg::ImportTAZ()
{
	BOOST_SCOPE_EXIT(this_)
	{
		this_->m_ignoreCalc = false;
	} BOOST_SCOPE_EXIT_END
	m_ignoreCalc = true;

	try
	{
		QString dirLast = m_sett->value("dir_taz", "").toString();
		QString filename = QFileDialog::getOpenFileName(this, "Load File", dirLast, "TAZ Files (*.taz *.TAZ)");
		if(filename=="" || !QFile::exists(filename))
			return;
		m_sett->setValue("dir_taz", QFileInfo(filename).path());


		pt::ptree node;

		std::ifstream ifstr{filename.toStdString()};
		pt::read_xml(ifstr, node);

		// clear old nuclei
		DelTabItem(-1);

		if(auto opt = node.get_optional<t_real>("taz.sample.a"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editA->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("taz.sample.b"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editB->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("taz.sample.c"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editC->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("taz.sample.alpha"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editAlpha->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("taz.sample.beta"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editBeta->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<t_real>("taz.sample.gamma"); opt)
		{
			std::ostringstream ostr; ostr.precision(g_prec); ostr << *opt;
			m_editGamma->setText(ostr.str().c_str());
		}
		if(auto opt = node.get_optional<std::string>("taz.sample.spacegroup"); opt)
		{
			// TODO
			//std::cout << *opt << std::endl;
		}

		// nuclei
		if(auto opt = node.get_optional<std::size_t>("taz.sample.atoms.num"); opt)
		{
			std::size_t numAtoms = *opt;
			for(std::size_t atomIdx=0; atomIdx<numAtoms; ++atomIdx)
			{
				std::string strNum = tl2::var_to_str(atomIdx);

				std::string name = node.get<std::string>("taz.sample.atoms."+strNum+".name", "n/a");
				t_real x = node.get<t_real>("taz.sample.atoms."+strNum+".x", 0.);
				t_real y = node.get<t_real>("taz.sample.atoms."+strNum+".y", 0.);
				t_real z = node.get<t_real>("taz.sample.atoms."+strNum+".z", 0.);

				// TODO
				t_real bRe = 0.;
				t_real bIm = 0.;

				t_real rad = 1.;
				std::string col = g_default_colours[atomIdx%g_default_colours.size()];

				AddTabItem(-1, name, bRe, bIm, x, y, z, rad, col);
			}
		}
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Structure Factors", ex.what());
	}

	GenerateFromSG();
	CalcB(false);
	Calc();
}


/**
 * save a TAZ file
 */
void StructFactDlg::ExportTAZ()
{
	QString dirLast = m_sett->value("dir_taz", "").toString();
	QString filename = QFileDialog::getSaveFileName(this, "Export TAZ", dirLast, "TAZ Files (*.taz *.TAZ)");
	if(filename=="" || !QFile::exists(filename))
		return;
	m_sett->setValue("dir_taz", QFileInfo(filename).path());

	std::ofstream ofstr{filename.toStdString()};
	if(!ofstr)
	{
		QMessageBox::critical(this, "Structure Factors", "Cannot open file for writing.");
		return;
	}
	ofstr.precision(g_prec);

	ofstr << "<taz>\n";
	ofstr << "\t<meta><info>Exported from Takin/Structfact.</info></meta>\n";

	// sample infos
	ofstr << "\t<sample>\n";
	ofstr << "\t\t<a>" << m_editA->text().toStdString() << "</a>\n";
	ofstr << "\t\t<b>" << m_editB->text().toStdString() << "</b>\n";
	ofstr << "\t\t<c>" << m_editC->text().toStdString() << "</c>\n";
	ofstr << "\t\t<alpha>" << m_editAlpha->text().toStdString() << "</alpha>\n";
	ofstr << "\t\t<beta>" << m_editBeta->text().toStdString() << "</beta>\n";
	ofstr << "\t\t<gamma>" << m_editGamma->text().toStdString() << "</gamma>\n";

	// P1 only has the identity trafo, so we can directly output all raw nucleus positions
	ofstr << "\t\t<spacegroup>P1</spacegroup>\n";

	// nucleus list
	ofstr << "\t\t<atoms>\n";
	ofstr << "\t\t\t<num>" << m_nuclei->rowCount() << "</num>\n";
	for(int row=0; row<m_nuclei->rowCount(); ++row)
	{
		ofstr << "\t\t\t<" << row << ">\n";
		ofstr << "\t\t\t\t<name>" << m_nuclei->item(row, COL_NAME)->text().toStdString() << "</name>\n";
		ofstr << "\t\t\t\t<x>" << m_nuclei->item(row, COL_X)->text().toStdString() << "</x>\n";
		ofstr << "\t\t\t\t<y>" << m_nuclei->item(row, COL_Y)->text().toStdString() << "</y>\n";
		ofstr << "\t\t\t\t<z>" << m_nuclei->item(row, COL_Z)->text().toStdString() << "</z>\n";
		ofstr << "\t\t\t</" << row << ">\n";
	}

	ofstr << "\t\t</atoms>\n";
	ofstr << "\t</sample>\n";

	ofstr << "</taz>\n";
}


/**
 * load a CIF
 */
void StructFactDlg::ImportCIF()
{
	BOOST_SCOPE_EXIT(this_)
	{
		this_->m_ignoreCalc = false;
	} BOOST_SCOPE_EXIT_END
	m_ignoreCalc = true;

	try
	{
		QString dirLast = m_sett->value("dir_cif", "").toString();
		QString filename = QFileDialog::getOpenFileName(this, "Import CIF", dirLast, "CIF Files (*.cif *.CIF)");
		if(filename=="" || !QFile::exists(filename))
			return;
		m_sett->setValue("dir_cif", QFileInfo(filename).path());

		auto [errstr, atoms, generatedatoms, atomnames, lattice, symops] =
			load_cif<t_vec, t_mat>(filename.toStdString(), g_eps);
		if(errstr != "")
		{
			QMessageBox::critical(this, "Structure Factors", errstr.c_str());
			return;
		}


		// clear old nuclei
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
		}
	}
	catch(const std::exception& ex)
	{
		QMessageBox::critical(this, "Structure Factors", ex.what());
	}


	CalcB(false);
	Calc();
}
