/**
 * converts a CIF file into a more parsable XML
 * @author Tobias Weber <tweber@ill.fr>
 * @date may-2019
 * @license GPLv3, see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2021  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
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

#include "../structfact/loadcif.h"
#include "tlibs2/libs/maths.h"

#include <gemmi/version.hpp>
#include <iostream>
#include <memory>


using t_real = double;
using t_vec = tl2::vec<t_real, std::vector>;
using t_mat = tl2::mat<t_real, std::vector>;

constexpr t_real g_eps = 1e-6;
constexpr int g_prec = 6;


static const std::string g_strSig = "cif2xml";
static const std::string g_strVer = "0.5";


/**
 * print vector
 */
std::ostream& operator<<(std::ostream& ostr, const t_vec& vec)
{
	for(std::size_t i=0; i<vec.size(); ++i)
	{
		ostr << vec[i];
		if(i < vec.size()-1)
			ostr << ", ";
	}

	return ostr;
}


/**
 * print matrix
 */
std::ostream& operator<<(std::ostream& ostr, const t_mat& mat)
{
	for(std::size_t i=0; i<mat.size1(); ++i)
	{
		for(std::size_t j=0; j<mat.size2(); ++j)
		{
			ostr << mat(i,j);

			if(j < mat.size2()-1)
				ostr << ", ";
		}

		if(i < mat.size1()-1)
			ostr << "; ";
	}

	return ostr;
}



/**
 * convert CIF to XML
 */
bool convert_cif(const char* pcFileIn, const char* pcFileOut)
{
	auto [errstr, atoms, generatedatoms, atomnames, lattice, ops] = load_cif<t_vec, t_mat>(pcFileIn, g_eps);
	if(errstr != "")
		std::cerr << "CIF importer messages:\n" << errstr << std::endl;


	// either open output file (if given) or use standard out
	std::ostream *pOstr = &std::cout;
	std::unique_ptr<std::ostream> _ofstr;
	if(pcFileOut)
	{
		_ofstr = std::make_unique<std::ofstream>(pcFileOut);
		pOstr = _ofstr.get();
	}



	pOstr->precision(g_prec);
	(*pOstr) << "<xtal>" << std::endl;


	// lattice
	(*pOstr) << "\t<lattice>\n";
	(*pOstr) << "\t\t<a> " << lattice.a << " </a>\n";
	(*pOstr) << "\t\t<b> " << lattice.b << " </b>\n";
	(*pOstr) << "\t\t<c> " << lattice.c << " </c>\n";
	(*pOstr) << "\t\t<alpha> " << lattice.alpha << " </alpha>\n";
	(*pOstr) << "\t\t<beta> " << lattice.beta << " </beta>\n";
	(*pOstr) << "\t\t<gamma> " << lattice.gamma << " </gamma>\n";
	(*pOstr) << "\t</lattice>\n";
	(*pOstr) << "\n";


	// basic atoms
	(*pOstr) << "\t<atoms>\n";
	for(std::size_t i=0; i<atoms.size(); ++i)
	{
		if(atomnames.size() == atoms.size())
			(*pOstr) << "\t\t<name" << i << "> " << atomnames[i] << " </name" << i << ">\n";
		(*pOstr) << "\t\t<pos" << i << "> " << atoms[i] << " </pos" << i << ">\n";
	}
	(*pOstr) << "\t</atoms>\n";
	(*pOstr) << "\n";


	// generated atoms
	(*pOstr) << "\t<generated_atoms>\n";
	for(std::size_t i=0; i<generatedatoms.size(); ++i)
	{
		(*pOstr) << "\t\t<pos" << i << ">\n";

		for(std::size_t j=0; j<generatedatoms[i].size(); ++j)
			(*pOstr) << "\t\t\t<gen" << j << "> " << generatedatoms[i][j] << " </gen" << j << ">\n";

		(*pOstr) << "\t\t</pos" << i << ">\n";
	}
	(*pOstr) << "\t</generated_atoms>\n";
	(*pOstr) << "\n";


	// symops
	(*pOstr) << "\t<symops>\n";
	for(std::size_t i=0; i<ops.size(); ++i)
		(*pOstr) << "\t\t<op" << i << "> " << ops[i] << " </op" << i << ">\n";
	(*pOstr) << "\t</symops>\n";
	(*pOstr) << "\n";


	// meta infos
	(*pOstr) << "\t<meta>\n";
	(*pOstr) << "\t\t<origin> " << g_strSig << " </origin>\n";
	(*pOstr) << "\t\t<version> " << g_strVer << " </version>\n";
	(*pOstr) << "\t</meta>\n";


	(*pOstr) << "</xtal>" << std::endl;

	return true;
}



/**
 * show infos about the tool
 */
static void show_infos(const char* pcProg)
{
	std::cout << "CIF to XML converter, version " << g_strVer << ".\n";
	std::cout << "Written by Tobias Weber <tweber@ill.fr> in May 2019.\n";

	std::cout << "\nThis program is free software: you can redistribute it and/or modify "
		"it under the terms of the GNU General Public License as published by "
		"the Free Software Foundation, version 3 of the License.\n"
		"This program is distributed in the hope that it will be useful, "
		"but WITHOUT ANY WARRANTY; without even the implied warranty of "
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
		"GNU General Public License for more details.\n"
		"You should have received a copy of the GNU General Public License "
		"along with this program. If not, see <http://www.gnu.org/licenses/>.\n\n";

	std::cout << "This program uses the Gemmi library (version " << GEMMI_VERSION << "), "
		<< "which is available under: https://github.com/project-gemmi/gemmi.\n";
	std::cout << std::endl;

	std::cerr << "\nUsage: " << pcProg << " <in.cif> <out.xml>\n";
	std::cerr << std::endl;
}




/**
 * entry point
 */
int main(int argc, char** argv)
{
	const char* pcFileIn = nullptr;
	const char* pcFileOut = nullptr;

	if(argc <= 1)
	{
		show_infos(argv[0]);
		return 0;
	}
	else if(argc == 2)
	{
		pcFileIn = argv[1];
	}
	else if(argc >= 3)
	{
		pcFileIn = argv[1];
		pcFileOut = argv[2];
	}


	if(!convert_cif(pcFileIn, pcFileOut))
		return -1;

	return 0;
}
