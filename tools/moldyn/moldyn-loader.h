/**
 * loads atom dynamics file
 * @author Tobias Weber <tweber@ill.fr>
 * @date Dec-2019
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

#ifndef __MOLDYN_H__
#define __MOLDYN_H__


#include <fstream>
#include <iostream>
#include <vector>
#include <string>

// for progress callback
#include <boost/signals2/signal.hpp>

#include "tlibs2/libs/file.h"
#include "tlibs2/libs/str.h"


template<class t_real, class t_vec>
class MolFrame
{
	public:
		MolFrame()
		{
			m_config.reserve(128);
		}


		void AddAtomConfig(std::vector<t_vec>&& config)
		{ m_config.emplace_back(std::move(config)); }

		void AddAtomConfig(const std::vector<t_vec>& config)
		{ m_config.push_back(config); }


		std::size_t GetNumAtomTypes() const
		{ return m_config.size(); }


		const std::vector<t_vec>& GetCoords(std::size_t idxType) const
		{ return m_config[idxType]; }


		/**
		 * get the atom coordinates
		 */
		const t_vec& GetAtomCoords(std::size_t idxType, std::size_t idxSubType) const
		{
			return m_config[idxType][idxSubType];
		}


		/**
		 * removes one atoms of type idxType and index idxSubType
		 */
		void RemoveAtom(std::size_t idxType, std::size_t idxSubType)
		{
			m_config[idxType].erase(m_config[idxType].begin() + idxSubType);
		}


		/**
		 * removes all atoms at index idx
		 */
		void RemoveAtoms(std::size_t idx)
		{
			m_config.erase(m_config.begin()+idx);
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


		const t_vec& GetBaseA() const { return m_baseA; }
		const t_vec& GetBaseB() const { return m_baseB; }
		const t_vec& GetBaseC() const { return m_baseC; }


		std::size_t GetFrameCount() const
		{ return m_frames.size(); }

		const MolFrame<t_real, t_vec>& GetFrame(std::size_t frame) const
		{ return m_frames[frame]; }


		std::size_t GetNumAtomTypes() const
		{ return m_vecAtoms.size(); }

		std::size_t GetNumAtomsTotal() const
		{
			std::size_t num = 0;

			for(std::size_t numpertype : m_vecAtomNums)
				num += numpertype;

			return num;
		}

		const std::string& GetAtomName(std::size_t idxType) const
		{ return m_vecAtoms[idxType]; }

		unsigned int GetAtomNum(std::size_t idxType) const
		{ return m_vecAtomNums[idxType]; }


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


		/**
		 * get atom coordinates for a specific frame
		 */
		const t_vec& GetAtomCoords(std::size_t idxType, std::size_t idxSubType, std::size_t iFrameIdx) const
		{
			return GetFrame(iFrameIdx).GetAtomCoords(idxType, idxSubType);
		}


		/**
		 * get atom coordinates for all frames
		 */
		std::vector<t_vec> GetAtomCoords(std::size_t idxType, std::size_t idxSubType) const
		{
			std::vector<t_vec> allcoords;
			allcoords.reserve(m_frames.size());

			if(!m_vecAtomNums[idxType])
				return allcoords;

			for(const MolFrame<t_real, t_vec>& frame : m_frames)
				allcoords.push_back(frame.GetAtomCoords(idxType, idxSubType));

			return allcoords;
		}


		/**
		 * removes one atoms of type idxType and index idxSubType
		 */
		void RemoveAtom(std::size_t idxType, std::size_t idxSubType)
		{
			if(!m_vecAtomNums[idxType])
				return;

			for(MolFrame<t_real, t_vec>& frame : m_frames)
				frame.RemoveAtom(idxType, idxSubType);

			--m_vecAtomNums[idxType];
		}


		/**
		 * removes all atoms at index idx
		 */
		void RemoveAtoms(std::size_t idx)
		{
			m_vecAtoms.erase(m_vecAtoms.begin() + idx);
			m_vecAtomNums.erase(m_vecAtomNums.begin() + idx);

			for(MolFrame<t_real, t_vec>& frame : m_frames)
				frame.RemoveAtoms(idx);
		}


		void Clear()
		{
			m_vecAtoms.clear();
			m_vecAtomNums.clear();
			m_frames.clear();

			m_sigLoadProgress.disconnect_all_slots();
			m_sigSaveProgress.disconnect_all_slots();
		}



		/**
		 * loading of files
		 */
		bool LoadFile(const std::string& filename, unsigned int frameskip = 0)
		{
			const std::string strDelim{" \t"};

			std::ifstream ifstr{filename};
			if(!ifstr)
			{
				std::cerr << "Cannot open \"" << filename << "\" for loading.";
				return 0;
			}


			std::size_t filesize = tl2::get_file_size(ifstr);
			std::cout << "File size: " << filesize / 1024 / 1024 << " MB." << std::endl;

			std::getline(ifstr, m_strSys);
			tl2::trim(m_strSys);
			std::cout << "System: " << m_strSys << std::endl;



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
			t_real percentage = 0;
			while(true)
			{
				std::string strConfig;
				std::getline(ifstr, strConfig);
				tl2::trim(strConfig);

				if(ifstr.eof())
					break;

				if(frameskip || iNumConfigs % 100)
				{
					std::cout << "\rReading " << strConfig << ". "
						<< static_cast<unsigned>(percentage) << " %.                ";
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

						// center cell (in rlu)
						vecCoords[0] -= 0.5;
						vecCoords[1] -= 0.5;
						vecCoords[2] -= 0.5;

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


				std::size_t filepos = tl2::get_file_pos(ifstr);
				percentage = static_cast<t_real>(filepos*100) / static_cast<t_real>(filesize);
				if(m_sigLoadProgress.num_slots() && !*m_sigLoadProgress(percentage))
				{
					std::cerr << "\nLoading cancelled." << std::endl;
					return 0;
				}
			}

			std::cout << "\rRead " << iNumConfigs << " configurations. " << "                        " << std::endl;
			return 1;
		}



		/**
		 * saving of files
		 */
		bool SaveFile(const std::string& filename)
		{
			std::cout << m_sigSaveProgress.num_slots() << std::endl;
			std::ofstream ofstr{filename};
			if(!ofstr)
			{
				std::cerr << "Cannot open \"" << filename << "\" for saving.";
				return 0;
			}

			ofstr.precision(8);

			if(m_baseA.size() != 3)
				return 0;

			ofstr << m_strSys << "\n" << "1" << std::endl;
			for(int i=0; i<m_baseA.size(); ++i)
				ofstr << m_baseA[i] << " " << m_baseB[i] << " " << m_baseC[i] << std::endl;

			for(const std::string& strAtom : m_vecAtoms)
				ofstr << strAtom << " ";
			ofstr << std::endl;

			for(unsigned int numAtom : m_vecAtomNums)
				ofstr << numAtom << " ";
			ofstr << std::endl;

			// iterate frames
			t_real percentage = 0;
			for(std::size_t frame=0; frame<m_frames.size(); ++frame)
			{
				ofstr << "Config " << (frame+1) << "\n";
				const MolFrame<t_real, t_vec>& config = m_frames[frame];

				// iterate atom types
				for(std::size_t atomidx=0; atomidx<config.GetNumAtomTypes(); ++atomidx)
				{
					const auto& coords = config.GetCoords(atomidx);
					// iterate coordinates
					for(const auto& vec : coords)
					{
						if(vec.size() != 3)
							return 0;
						ofstr << vec[0]+0.5 << " " << vec[1]+0.5 << " " << vec[2]+0.5 << "\n";
					}
				}

				percentage = static_cast<t_real>((frame+1)*100)/static_cast<t_real>(m_frames.size());
				if(m_sigSaveProgress.num_slots() && !*m_sigSaveProgress(percentage))
				{
					std::cerr << "\nSaving cancelled." << std::endl;
					return 0;
				}

				if(frame % 100)
				{
					std::cout << "\rSaving configuration " << (frame+1) << " of " << m_frames.size() << ". "
						<< static_cast<unsigned>(percentage) << " %.                ";
					std::cout.flush();
				}
			}

			std::cout << "\rSaved " << m_frames.size() << " configurations. " << "                        " << std::endl;
			ofstr.flush();
			return 1;
		}



		template<class subscriber> void SubscribeToLoadProgress(const subscriber& subs)
		{ m_sigLoadProgress.connect(subs); }

		template<class subscriber> void SubscribeToSaveProgress(const subscriber& subs)
		{ m_sigSaveProgress.connect(subs); }


		template<class subscriber> void UnsubscribeFromLoadProgress(const subscriber* subs=nullptr)
		{
			// TODO
			//if(!subs)
				m_sigLoadProgress.disconnect_all_slots();
		}

		template<class subscriber> void UnsubscribeFromSaveProgress(const subscriber* subs=nullptr)
		{
			// TODO
			//if(!subs)
				m_sigSaveProgress.disconnect_all_slots();
		}


	private:
		std::string m_strSys;

		t_vec m_baseA;
		t_vec m_baseB;
		t_vec m_baseC;

		std::vector<std::string> m_vecAtoms;
		std::vector<unsigned int> m_vecAtomNums;

		std::vector<MolFrame<t_real, t_vec>> m_frames;

		boost::signals2::signal<bool (t_real)> m_sigLoadProgress;
		boost::signals2::signal<bool (t_real)> m_sigSaveProgress;
};


#endif
