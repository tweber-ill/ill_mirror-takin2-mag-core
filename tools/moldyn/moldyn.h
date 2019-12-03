/**
 * loads atom dynamics file
 * @author Tobias Weber <tweber@ill.fr>
 * @date Dec-2019
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __MOLDYN_H__
#define __MOLDYN_H__


#include <fstream>
#include <iostream>
#include <vector>
#include <string>

#include "libs/str.h"


template<class t_real, class t_vec>
class MolFrame
{
	public:
		MolFrame()
		{
			m_config.reserve(128);
		}


		void AddAtomConfig(std::vector<t_vec>&& config)
		{
			m_config.emplace_back(std::move(config));
		}

		void AddAtomConfig(const std::vector<t_vec>& config)
		{
			m_config.push_back(config);
		}

	private:
		// atoms -> coordinates
		std::vector<std::vector<t_vec>> m_config;
};



template<class t_real, class t_vec>
class MolDyn
{
	public:
		MolDyn() : m_baseA(3), m_baseB(3), m_baseC(3)
		{
			m_frames.reserve(16384);
		}

		void SetBaseA(t_real x, t_real y, t_real z)
		{
			m_baseA[0] = x;
			m_baseA[1] = y;
			m_baseA[2] = z;
		}

		void SetBaseB(t_real x, t_real y, t_real z)
		{
			m_baseB[0] = x;
			m_baseB[1] = y;
			m_baseB[2] = z;
		}

		void SetBaseC(t_real x, t_real y, t_real z)
		{
			m_baseC[0] = x;
			m_baseC[1] = y;
			m_baseC[2] = z;
		}


		void AddAtomType(const std::string& name, unsigned int number)
		{
			m_vecAtoms.push_back(name);
			m_vecAtomNums.push_back(number);
		}


		void AddFrame(MolFrame<t_real, t_vec>&& frame)
		{
			m_frames.emplace_back(std::move(frame));
		}

		void AddFrame(const MolFrame<t_real, t_vec>& frame)
		{
			m_frames.push_back(frame);
		}



		bool LoadFile(const std::string& filename, unsigned int frameskip = 0)
		{
			const std::string strDelim{" \t"};

			std::ifstream ifstr{filename};
			if(!ifstr)
			{
				std::cerr << "Cannot open \"" << filename << "\".";
				return 0;
			}

			std::string strSys;
			std::getline(ifstr, strSys);
			tl2::trim(strSys);
			std::cout << "System: " << strSys << std::endl;



			std::string strScale;
			std::getline(ifstr, strScale);
			t_real scale = tl2::str_to_var<t_real>(strScale);
			std::cout << "scale: " << scale << std::endl;



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
				return 0;
			}


			SetBaseA(_vecBase1[0]*scale, _vecBase2[0]*scale, _vecBase3[0]*scale);
			SetBaseB(_vecBase1[1]*scale, _vecBase2[1]*scale, _vecBase3[1]*scale);
			SetBaseC(_vecBase1[2]*scale, _vecBase2[2]*scale, _vecBase3[2]*scale);


			std::string strAtoms;
			std::string strAtomNums;
			std::getline(ifstr, strAtoms);
			std::getline(ifstr, strAtomNums);
			tl2::get_tokens<std::string>(strAtoms, strDelim, m_vecAtoms);
			tl2::get_tokens<unsigned int>(strAtomNums, strDelim, m_vecAtomNums);

			if(m_vecAtoms.size() != m_vecAtomNums.size())
			{
				std::cerr << "Atom size mismatch." << std::endl;
				return 0;
			}

			for(std::size_t i=0; i<m_vecAtoms.size(); ++i)
			{
				std::cout << m_vecAtomNums[i] << " " << m_vecAtoms[i] << " atoms." << std::endl;
			}



			std::size_t iNumConfigs = 0;
			while(true)
			{
				std::string strConfig;
				std::getline(ifstr, strConfig);
				tl2::trim(strConfig);

				if(ifstr.eof())
					break;

				if(frameskip || iNumConfigs % 100)
				{
					std::cout << "\rReading " << strConfig << "..." << "        ";
					std::cout.flush();
				}


				MolFrame<t_real, t_vec> frame;
				for(std::size_t iAtomType=0; iAtomType<m_vecAtoms.size(); ++iAtomType)
				{
					std::vector<t_vec> atomconf;

					for(std::size_t iAtom=0; iAtom<m_vecAtomNums[iAtomType]; ++iAtom)
					{
						std::string strCoords;
						std::getline(ifstr, strCoords);

						t_vec vecCoords;
						vecCoords.reserve(3);
						tl2::get_tokens<t_real>(strCoords, strDelim, vecCoords);


						if(vecCoords.size() != 3)
						{
							std::cerr << "Invalid coordinate." << std::endl;
							return 0;
						}

						atomconf.emplace_back(std::move(vecCoords));
					}

					frame.AddAtomConfig(std::move(atomconf));
				}

				AddFrame(std::move(frame));
				++iNumConfigs;


				// skip frames
				for(unsigned int skipped=0; skipped<frameskip; ++skipped)
				{
					std::string strTmp;
					std::getline(ifstr, strTmp);
					//std::cout << "Skipping " << strTmp << "..." << "        ";

					for(std::size_t iAtomType=0; iAtomType<m_vecAtoms.size(); ++iAtomType)
					{
						for(std::size_t iAtom=0; iAtom<m_vecAtomNums[iAtomType]; ++iAtom)
							std::getline(ifstr, strTmp);
					}
				}
			}

			std::cout << "\rRead " << iNumConfigs << " configurations." << "                " << std::endl;
			return 1;
		}


	private:
		t_vec m_baseA;
		t_vec m_baseB;
		t_vec m_baseC;

		std::vector<std::string> m_vecAtoms;
		std::vector<unsigned int> m_vecAtomNums;

		std::vector<MolFrame<t_real, t_vec>> m_frames;
};


#endif
