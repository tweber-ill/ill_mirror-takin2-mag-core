/**
 * loads atom dynamics file
 * @author Tobias Weber <tweber@ill.fr>
 * @date Dec-2019
 * @license GPLv3, see 'LICENSE' file
 *
 * g++ -std=c++17  -I ../../tlibs2 -o moldyn moldyn.cpp
 */

#include <fstream>
#include <iostream>
#include <vector>
#include <string>

#include "libs/str.h"


using t_real = double;


int main(int argc, char** argv)
{
	if(argc <= 1)
		return -1;


	std::ifstream ifstr{argv[1]};
	if(!ifstr)
	{
		std::cerr << "Cannot open \"" << argv[1] << "\".";
		return -1;
	}


	const std::string strDelim{" \t"};


	std::string strSys;
	std::getline(ifstr, strSys);
	tl2::trim(strSys);
	std::cout << "System: " << strSys << std::endl;



	std::string strTmp;
	std::getline(ifstr, strTmp);
	int numSys = tl2::str_to_var<int>(strTmp);
	std::cout << "# System: " << numSys << std::endl;



	std::string strVecs1;
	std::string strVecs2;
	std::string strVecs3;
	std::getline(ifstr, strVecs1);
	std::getline(ifstr, strVecs2);
	std::getline(ifstr, strVecs3);

	std::vector<t_real> _vecBase1, _vecBase2, _vecBase3;
	tl2::get_tokens<t_real>(strVecs1, strDelim, _vecBase1);
	tl2::get_tokens<t_real>(strVecs2, strDelim, _vecBase2);
	tl2::get_tokens<t_real>(strVecs3, strDelim, _vecBase3);

	if(_vecBase1.size()!=3 || _vecBase2.size()!=3 || _vecBase3.size()!=3)
	{
		std::cerr << "Invalid base vectors." << std::endl;
		return -1;
	}

	std::vector<t_real> vecBase1(3), vecBase2(3), vecBase3(3);
	vecBase1[0] = _vecBase1[0]; vecBase2[0] = _vecBase1[1]; vecBase3[0] = _vecBase1[2];
	vecBase1[1] = _vecBase2[0]; vecBase2[1] = _vecBase2[1]; vecBase3[1] = _vecBase2[2];
	vecBase1[2] = _vecBase3[0]; vecBase2[2] = _vecBase3[1]; vecBase3[2] = _vecBase3[2];

	std::cout << "Base vector 1: ";
	for(int i=0; i<3; ++i)
		std::cout << vecBase1[i] << ", ";
	std::cout << std::endl;

	std::cout << "Base vector 2: ";
	for(int i=0; i<3; ++i)
		std::cout << vecBase2[i] << ", ";
	std::cout << std::endl;

	std::cout << "Base vector 3: ";
	for(int i=0; i<3; ++i)
		std::cout << vecBase3[i] << ", ";
	std::cout << std::endl;



	std::vector<std::string> vecAtoms;
	std::vector<int> vecAtomNums;

	std::string strAtoms;
	std::string strAtomNums;
	std::getline(ifstr, strAtoms);
	std::getline(ifstr, strAtomNums);
	tl2::get_tokens<std::string>(strAtoms, strDelim, vecAtoms);
	tl2::get_tokens<unsigned int>(strAtomNums, strDelim, vecAtomNums);

	if(vecAtoms.size() != vecAtomNums.size())
	{
		std::cerr << "Atom size mismatch." << std::endl;
		return -1;
	}

	for(std::size_t i=0; i<vecAtoms.size(); ++i)
	{
		std::cout << vecAtomNums[i] << " " << vecAtoms[i] << " atoms." << std::endl;
	}



	std::size_t iNumConfigs = 0;
	while(true)
	{
		std::string strConfig;
		std::getline(ifstr, strConfig);
		tl2::trim(strConfig);

		if(ifstr.eof())
			break;

		if(iNumConfigs % 100)
			std::cout << "\rReading " << strConfig << "..." << "        ";


		for(std::size_t iAtomType=0; iAtomType<vecAtoms.size(); ++iAtomType)
		{
			for(std::size_t iAtom=0; iAtom<vecAtomNums[iAtomType]; ++iAtom)
			{
				std::string strCoords;
				std::getline(ifstr, strCoords);

				std::vector<t_real> vecCoords;
				vecCoords.reserve(3);
				tl2::get_tokens<t_real>(strCoords, strDelim, vecCoords);


				if(vecCoords.size() != 3)
				{
					std::cerr << "Invalid coordinate." << std::endl;
					return -1;
				}
			}
		}

		++iNumConfigs;
	}

	std::cout << "\rRead " << iNumConfigs << " configurations." << "                " << std::endl;


	return 0;
}
