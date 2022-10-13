/**
 * converts magnetic space group table
 * @author Tobias Weber <tweber@ill.fr>
 * @date 18-nov-17
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 8-Nov-2018 from my privately developed "magtools" project (https://github.com/t-weber/magtools).
 *
 * g++-8 -std=c++17 -fconcepts -I../../ -o convmag convmag.cpp
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2021  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 * "magtools" project
 * Copyright (C) 2017-2018  Tobias WEBER (privately developed).
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

#include <iostream>
#include <fstream>
#include <tuple>
#include <vector>

//#include <boost/numeric/ublas/matrix.hpp>
//#include <boost/numeric/ublas/vector.hpp>
//namespace ublas = boost::numeric::ublas;

#include <boost/property_tree/ptree.hpp>
//#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/info_parser.hpp>
namespace ptree = boost::property_tree;

#include <boost/algorithm/string/trim.hpp>
namespace algo = boost::algorithm;

#include "tlibs2/libs/maths.h"


using t_real = double;
//using t_mat = ublas::matrix<t_real>;
//using t_vec = ublas::vector<t_real>;
using t_mat = tl2::mat<t_real, std::vector>;
using t_vec = tl2::vec<t_real, std::vector>;


bool bSaveOG = false;

std::string to_str(const t_mat& mat)
{
	// special cases
	static const auto zero = tl2::zero<t_mat>(mat.size1(), mat.size2());
	static const auto unit = tl2::unit<t_mat>(mat.size1(), mat.size2());

	if(tl2::equals<t_mat, t_real>(mat, zero))
		return "0";
	else if(tl2::equals<t_mat, t_real>(mat, unit))
		return "1";

	// general case
	std::ostringstream ostr;

	for(std::size_t i=0; i<mat.size1(); ++i)
	{
		for(std::size_t j=0; j<mat.size2(); ++j)
		{
			ostr << mat(i,j);
			if(j < mat.size2()-1)
				ostr << " ";
		}

		if(i < mat.size2()-1)
			ostr << "\t";
	}

	return ostr.str();
}


std::string to_str(const t_vec& vec)
{
	// spacial cases
	static const auto zero = tl2::zero<t_vec>(vec.size());
	static const auto x = tl2::create<t_vec>({1,0,0});
	static const auto y = tl2::create<t_vec>({0,1,0});
	static const auto z = tl2::create<t_vec>({0,0,1});
	static const auto mx = tl2::create<t_vec>({-1,0,0});
	static const auto my = tl2::create<t_vec>({0,-1,0});
	static const auto mz = tl2::create<t_vec>({0,0,-1});

	if(tl2::equals<t_vec>(vec, zero))
		return "0";
	else if(tl2::equals<t_vec>(vec, x))
		return "x";
	else if(tl2::equals<t_vec>(vec, y))
		return "y";
	else if(tl2::equals<t_vec>(vec, z))
		return "z";
	else if(tl2::equals<t_vec>(vec, mx))
		return "-x";
	else if(tl2::equals<t_vec>(vec, my))
		return "-y";
	else if(tl2::equals<t_vec>(vec, mz))
		return "-z";

	// general case
	std::ostringstream ostr;

	for(std::size_t i=0; i<vec.size(); ++i)
	{
		ostr << vec[i];
		if(i < vec.size()-1)
			ostr << " ";
	}

	return ostr.str();
}


std::ostream& operator<<(std::ostream& ostr, const t_mat& mat)
{
	ostr << to_str(mat);
	return ostr;
}


std::ostream& operator<<(std::ostream& ostr, const t_vec& vec)
{
	ostr << to_str(vec);
	return ostr;
}


template<class T>
T get_num(std::istream& istr)
{
	T t;
	istr >> t;
	return t;
}


std::string get_string(std::istream& istr)
{
	// skip whitespace
	while(1)
	{
		char c = istr.get();
		if(c!=' ' && c!='\t')
		{
			istr.unget();
			break;
		}
	}

	// string start
	char c = istr.get();
	if(c != '\"')
	{
		istr.unget();
		std::cerr << "Expected a string." << std::endl;
	}

	// read string
	std::string str;
	while(1)
	{
		c = istr.get();
		// end of string?
		if(c == '\"')
			break;

		str += c;
	}

	algo::trim(str);
	return str;
}


t_mat get_matrix(std::size_t N, std::size_t M, std::istream& istr)
{
	t_mat mat(N,M);

	for(std::size_t i=0; i<N; ++i)
		for(std::size_t j=0; j<M; ++j)
			istr >> mat(i,j);

	return mat;
}


t_vec get_vector(std::size_t N, std::istream& istr)
{
	t_vec vec(N);

	for(std::size_t i=0; i<N; ++i)
		istr >> vec(i);

	return vec;
}


std::tuple<std::string, t_mat> get_pointgroup_op(std::istream& istr)
{
	int iNum = get_num<int>(istr);

	std::string strName = get_string(istr);
	std::string strOpXYZ = get_string(istr);
	t_mat matOp = get_matrix(3,3, istr);

	//std::cout << strName << ": " << matOp << "\n";
	return { strName, matOp };
}


void convert_spacegroup(std::istream& istr, ptree::ptree& prop, const std::string& strPath,
	const std::vector<std::tuple<std::string, t_mat>>* pPtOps = nullptr,
	const std::vector<std::tuple<std::string, t_mat>>* pHexPtOps = nullptr)
{
	int iNrBNS[2];
	istr >> iNrBNS[0] >> iNrBNS[1];
	std::string strNrBNS = get_string(istr);
	std::string strSGBNS = get_string(istr);

	int iNrOG[3];
	istr >> iNrOG[0] >> iNrOG[1] >> iNrOG[2];
	std::string strNrOG = get_string(istr);
	std::string strSGOG = get_string(istr);

	prop.put(strPath + "bns.id", strSGBNS);
	prop.put(strPath + "og.id", strSGOG);

	std::ostringstream ostrNrBNS, ostrNrOG;
	ostrNrBNS << iNrBNS[0] << "." << iNrBNS[1];
	ostrNrOG << iNrOG[0] << "." << iNrOG[1] << "." << iNrOG[2];

	prop.put(strPath + "bns.nr", ostrNrBNS.str());
	prop.put(strPath + "og.nr", ostrNrOG.str());

	// consistency check
	if(ostrNrBNS.str()!=strNrBNS || ostrNrOG.str()!=strNrOG)
		std::cerr << "Mismatch of space group number in " << strSGBNS << "." << std::endl;

	bool bIsHex = (iNrBNS[0]>=143 && iNrBNS[0]<=194);


	int iTy = get_num<int>(istr);
	if(iTy == 4)	// BNS -> OG trafo
	{
		t_mat matBNS2OG = get_matrix(3,3, istr);
		t_vec vecBNS2OG = get_vector(3, istr);
		t_real numBNS2OG = get_num<t_real>(istr);

		prop.put(strPath + "bns2og.R", to_str(matBNS2OG));
		prop.put(strPath + "bns2og.v", to_str(vecBNS2OG));
		prop.put(strPath + "bns2og.d", numBNS2OG);
	}


	//std::size_t iMaxOperBNS = 0;
	std::size_t iNumOpsBNS = get_num<std::size_t>(istr);
	for(std::size_t iPtOp=0; iPtOp<iNumOpsBNS; ++iPtOp)
	{
		std::size_t iOperBNS = get_num<std::size_t>(istr);
		//iMaxOperBNS = std::max(iOperBNS, iMaxOperBNS);
		t_vec vecBNS = get_vector(3, istr);
		t_real numBNS = get_num<t_real>(istr);
		int itInvBNS = get_num<int>(istr);

		if(pPtOps && !bIsHex)
			prop.put(strPath + "bns.ops.R" + std::to_string(iPtOp+1), to_str(std::get<1>(pPtOps->at(iOperBNS-1))));
		else if(pHexPtOps && bIsHex)
			prop.put(strPath + "bns.ops.R" + std::to_string(iPtOp+1), to_str(std::get<1>(pHexPtOps->at(iOperBNS-1))));
		else
		{
			std::cerr << "Invalid point group index for " << strSGBNS << "." << std::endl;
			prop.put(strPath + "bns.ops.R" + std::to_string(iPtOp+1), iOperBNS);
		}
		prop.put(strPath + "bns.ops.v" + std::to_string(iPtOp+1), to_str(vecBNS));
		prop.put(strPath + "bns.ops.d" + std::to_string(iPtOp+1), numBNS);
		prop.put(strPath + "bns.ops.t" + std::to_string(iPtOp+1), itInvBNS);
	}
	//std::cout << "\nmax op: " << iMaxOperBNS << std::endl;

	std::size_t iNumLattVecsBNS = get_num<std::size_t>(istr);
	for(std::size_t iVec=0; iVec<iNumLattVecsBNS; ++iVec)
	{
		t_vec vecBNS = get_vector(3, istr);
		t_real numBNS = get_num<t_real>(istr);

		prop.put(strPath + "bns.lat.v" + std::to_string(iVec+1), to_str(vecBNS));
		prop.put(strPath + "bns.lat.d" + std::to_string(iVec+1), numBNS);
	}

	std::size_t iNumWycBNS = get_num<std::size_t>(istr);
	for(std::size_t iWyc=0; iWyc<iNumWycBNS; ++iWyc)
	{
		std::size_t iNumPos = get_num<std::size_t>(istr);
		std::size_t iMult = get_num<std::size_t>(istr);
		std::string strWycName = get_string(istr);

		prop.put(strPath + "bns.wyc.s" + std::to_string(iWyc+1) + ".l", strWycName);
		prop.put(strPath + "bns.wyc.s" + std::to_string(iWyc+1) + ".m", iMult);

		for(std::size_t iPos=0; iPos<iNumPos; ++iPos)
		{
			t_vec vecWyc = get_vector(3, istr);
			t_real numWyc = get_num<t_real>(istr);
			t_mat matWycXYZ = get_matrix(3, 3, istr);
			t_mat matWycMXMYMZ = get_matrix(3, 3, istr);

			prop.put(strPath + "bns.wyc.s" + std::to_string(iWyc+1)
				+ ".v" + std::to_string(iPos+1), to_str(vecWyc));
			prop.put(strPath + "bns.wyc.s" + std::to_string(iWyc+1)
				+ ".d" + std::to_string(iPos+1), numWyc);
			prop.put(strPath + "bns.wyc.s" + std::to_string(iWyc+1)
				+ ".R" + std::to_string(iPos+1), to_str(matWycXYZ));
			prop.put(strPath + "bns.wyc.s" + std::to_string(iWyc+1)
				+ ".M" + std::to_string(iPos+1), to_str(matWycMXMYMZ));
		}
	}


	if(iTy == 4)	// OG
	{
		std::size_t iNumPtOpsOG = get_num<std::size_t>(istr);
		for(std::size_t iPtOp=0; iPtOp<iNumPtOpsOG; ++iPtOp)
		{
			std::size_t iOperOG = get_num<std::size_t>(istr);
			t_vec vecOG = get_vector(3, istr);
			t_real numOG = get_num<t_real>(istr);
			int itInvOG = get_num<int>(istr);

			if(bSaveOG)
			{
				if(pPtOps && !bIsHex)
					prop.put(strPath + "og.ops.R" + std::to_string(iPtOp+1), to_str(std::get<1>(pPtOps->at(iOperOG-1))));
				else if(pHexPtOps && bIsHex)
					prop.put(strPath + "og.ops.R" + std::to_string(iPtOp+1), to_str(std::get<1>(pHexPtOps->at(iOperOG-1))));
				else
					prop.put(strPath + "og.ops.R" + std::to_string(iPtOp+1), iOperOG);
				prop.put(strPath + "og.ops.v" + std::to_string(iPtOp+1), to_str(vecOG));
				prop.put(strPath + "og.ops.d" + std::to_string(iPtOp+1), numOG);
				prop.put(strPath + "og.ops.t" + std::to_string(iPtOp+1), itInvOG);
			}
		}

		std::size_t iNumLattVecsOG = get_num<std::size_t>(istr);
		for(std::size_t iVec=0; iVec<iNumLattVecsOG; ++iVec)
		{
			t_vec vecOG = get_vector(3, istr);
			t_real numOG = get_num<t_real>(istr);

			if(bSaveOG)
			{
				prop.put(strPath + "og.lat.v" + std::to_string(iVec+1), to_str(vecOG));
				prop.put(strPath + "og.lat.d" + std::to_string(iVec+1), numOG);
			}
		}

		std::size_t iNumWycOG = get_num<std::size_t>(istr);
		for(std::size_t iWyc=0; iWyc<iNumWycOG; ++iWyc)
		{
			std::size_t iNumPos = get_num<std::size_t>(istr);
			std::size_t iMult = get_num<std::size_t>(istr);
			std::string strWycName = get_string(istr);

			if(bSaveOG)
			{
				prop.put(strPath + "og.wyc.s" + std::to_string(iWyc+1) + ".l", strWycName);
				prop.put(strPath + "og.wyc.s" + std::to_string(iWyc+1) + ".m", iMult);
			}

			for(std::size_t iPos=0; iPos<iNumPos; ++iPos)
			{
				t_vec vecWyc = get_vector(3, istr);
				t_real numWyc = get_num<t_real>(istr);
				t_mat matWycXYZ = get_matrix(3, 3, istr);
				t_mat matWycMXMYMZ = get_matrix(3, 3, istr);

				if(bSaveOG)
				{
					prop.put(strPath + "og.wyc.s" + std::to_string(iWyc+1)
						+ ".v" + std::to_string(iPos+1) , to_str(vecWyc));
					prop.put(strPath + "og.wyc.s" + std::to_string(iWyc+1)
						+ ".d" + std::to_string(iPos+1) , numWyc);
					prop.put(strPath + "og.wyc.s" + std::to_string(iWyc+1)
						+ ".R" + std::to_string(iPos+1) , to_str(matWycXYZ));
					prop.put(strPath + "og.wyc.s" + std::to_string(iWyc+1)
						+ ".M" + std::to_string(iPos+1), to_str(matWycMXMYMZ));
				}
			}
		}
	}

	//std::cout << strSGBNS << ", " << strSGOG << "\n";
	//std::cout << "."; std::cout.flush();
}


bool convert_table(const char* pcInFile, const char* pcOutFile)
{
	std::ifstream istr(pcInFile);
	if(!istr)
	{
		std::cerr << "Cannot open \"" << pcInFile << "\"." << std::endl;
		return false;
	}

	std::vector<std::tuple<std::string, t_mat>> vecPtOps, vecHexPtOps;

	// read 48 point group operators
	std::cout << "Reading point group operators...\n";
	for(std::size_t i=0; i<48; ++i)
		vecPtOps.emplace_back(get_pointgroup_op(istr));

	// read 24 hexagonal point group operators
	std::cout << "Reading hexagonal point group operators...\n";
	for(std::size_t i=0; i<24; ++i)
		vecHexPtOps.emplace_back(get_pointgroup_op(istr));


	ptree::ptree prop;

	// convert the 1651 3D space groups
	std::cout << "Converting space groups...\n";
	for(std::size_t i=0; i<1651; ++i)
	{
		std::cout << "\rGroup " << (i+1) << " / 1651...     ";
		std::cout.flush();

		std::string strPath = "mag_groups.sg" + std::to_string(i+1) + ".";
		convert_spacegroup(istr, prop, strPath, &vecPtOps, &vecHexPtOps);
	}
	std::cout << "\n";


	// reference for the magnetic space group data
	prop.put("mag_groups.source", "Magnetic space group data obtained from <a href=\"https://stokes.byu.edu/iso/magneticspacegroups.php\">ISO-MAG, ISOTROPY Sofware Suite</a>.");
	prop.put("mag_groups.source_url", "https://stokes.byu.edu/iso/magnetic_data.txt");


	try
	{
		/*ptree::write_xml(pcOutFile, prop,
			std::locale(),
			ptree::xml_writer_make_settings('\t', 1, std::string("utf-8")));*/
		ptree::write_info(pcOutFile, prop,
			std::locale(),
			ptree::info_writer_make_settings('\t', 1));
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in setup tool: " << ex.what() << std::endl;
		return false;
	}

	return true;
}


int main(int argc, char **argv)
{
	if(argc >= 3)
		convert_table(argv[1], argv[2]);
	else
		convert_table("ext/magsg.dat", "res/data/magsg.info");

	return 0;
}
