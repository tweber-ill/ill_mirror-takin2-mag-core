/**
 * future math library, developed from scratch to eventually replace tlibs(2)
 * magnetic space group library
 * @author Tobias Weber <tweber@ill.fr>
 * @date 7-apr-18
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 8-Nov-2018 from the privately developed "magtools" project (https://github.com/t-weber/magtools).
 */

#ifndef __MAG_SG_H__
#define __MAG_SG_H__

#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include "math_algos.h"

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
namespace ptree = boost::property_tree;



// ----------------------------------------------------------------------------
/**
 * forward declarations
 */
template<class t_mat, class t_vec>
requires m::is_mat<t_mat> && m::is_vec<t_vec>
class Spacegroups;
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
/**
 * Symmetry operations
 */
template<class t_mat, class t_vec>
requires m::is_mat<t_mat> && m::is_vec<t_vec>
class Symmetry
{
	friend class Spacegroups<t_mat, t_vec>;

private:
	// rotations
	std::vector<t_mat> m_rot;

	// translations
	std::vector<t_vec> m_trans;

	// time inversions
	std::vector<typename t_mat::value_type> m_inv;

public:
	Symmetry() = default;
	~Symmetry() = default;

	const std::vector<t_mat>& GetRotations() const { return m_rot; }
	const std::vector<t_vec>& GetTranslations() const { return m_trans; }
	const std::vector<typename t_mat::value_type>& GetInversions() const { return m_inv; }
};
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
/**
 * Wyckoff positions
 */
template<class t_mat, class t_vec>
requires m::is_mat<t_mat> && m::is_vec<t_vec>
class WycPositions
{
	friend class Spacegroups<t_mat, t_vec>;

private:
	std::string m_letter;

	// multiplicity
	int m_mult = 0;

	// structural & magnetic rotations
	std::vector<t_mat> m_rot, m_rotMag;

	// translations
	std::vector<t_vec> m_trans;

public:
	WycPositions() = default;
	~WycPositions() = default;

	const std::string& GetLetter() const { return m_letter; }
	int GetMultiplicity() const { return m_mult; }
	std::string GetName() const { return std::to_string(m_mult) + m_letter; }

	const std::vector<t_mat>& GetRotations() const { return m_rot; }
	const std::vector<t_mat>& GetRotationsMag() const { return m_rotMag; }
	const std::vector<t_vec>& GetTranslations() const { return m_trans; }
};
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
/**
 * a magnetic space group
 */
template<class t_mat, class t_vec>
requires m::is_mat<t_mat> && m::is_vec<t_vec>
class Spacegroup
{
	friend class Spacegroups<t_mat, t_vec>;

private:
	std::string m_nameBNS, m_nameOG;
	std::string m_nrBNS, m_nrOG;
	int m_sgnrStruct = -1, m_sgnrMag = -1;

	// lattice definition
	std::shared_ptr<std::vector<t_vec>> m_latticeBNS;
	std::shared_ptr<std::vector<t_vec>> m_latticeOG;

	// symmetry operations
	std::shared_ptr<Symmetry<t_mat, t_vec>> m_symBNS;
	std::shared_ptr<Symmetry<t_mat, t_vec>> m_symOG;

	// Wyckoff positions
	std::shared_ptr<std::vector<WycPositions<t_mat, t_vec>>> m_wycBNS;
	std::shared_ptr<std::vector<WycPositions<t_mat, t_vec>>> m_wycOG;

	// BNS to OG trafo
	t_mat m_rotBNS2OG;
	t_vec m_transBNS2OG;

public:
	Spacegroup() = default;
	~Spacegroup() = default;

	const std::string& GetName(bool bBNS=1) const
	{ return bBNS ? m_nameBNS : m_nameOG; }

	const std::string& GetNumber(bool bBNS=1) const
	{ return bBNS ? m_nrBNS : m_nrOG; }

	// structural and magnetic space group number
	int GetStructNumber() const { return m_sgnrStruct; }
	int GetMagNumber() const { return m_sgnrMag; }

	const std::vector<t_vec>* GetLattice(bool bBNS=true) const
	{ return bBNS ? m_latticeBNS.get() : m_latticeOG.get(); }

	const Symmetry<t_mat, t_vec>* GetSymmetries(bool bBNS=true) const
	{ return bBNS ? m_symBNS.get() : m_symOG.get(); }

	const std::vector<WycPositions<t_mat, t_vec>>* GetWycPositions(bool bBNS=true) const
	{ return bBNS ? m_wycBNS.get() : m_wycOG.get(); }
};
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
/**
 * a collection of magnetic space groups
 */
template<class t_mat, class t_vec>
requires m::is_mat<t_mat> && m::is_vec<t_vec>
class Spacegroups
{
private:
	std::vector<Spacegroup<t_mat, t_vec>> m_sgs;

public:
	Spacegroups() = default;
	~Spacegroups() = default;

	bool Load(const std::string& strFile);

	const std::vector<Spacegroup<t_mat, t_vec>>* GetSpacegroups() const
	{ return &m_sgs; }

	const Spacegroup<t_mat, t_vec>* GetSpacegroupByNumber(int iStruc, int iMag) const
	{
		auto iter = std::find_if(m_sgs.begin(), m_sgs.end(),
			[iStruc, iMag](const auto& sg) -> bool
			{
				return  (sg.GetStructNumber()==iStruc && sg.GetMagNumber()==iMag);
			});

		if(iter == m_sgs.end())
			return nullptr;
		return &*iter;
	}
};
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
// Loader


template<class t_mat, class t_vec>
bool Spacegroups<t_mat, t_vec>::Load(const std::string& strFile)
{
	using t_real = typename t_mat::value_type;

	// load xml database
	ptree::ptree prop;
	try
	{
		//ptree::read_xml(strFile, prop);
		ptree::read_info(strFile, prop);
	}
	catch(const std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
		return false;
	}


	// iterate space groups
	const auto& groups = prop.get_child_optional("mag_groups");
	if(!groups)
	{
		std::cerr << "No space groups defined." << std::endl;
		return false;
	}


	for(const auto& group : *groups)
	{
		// --------------------------------------------------------------------
		auto nameBNS = group.second.get_optional<std::string>("bns.id");
		auto nrBNS = group.second.get_optional<std::string>("bns.nr");
		const auto& lattBNS = group.second.get_child_optional("bns.lat");
		const auto& opsBNS = group.second.get_child_optional("bns.ops");
		const auto& wycBNS = group.second.get_child_optional("bns.wyc");

		auto nameOG = group.second.get_optional<std::string>("og.id");
		auto nrOG = group.second.get_optional<std::string>("og.nr");
		const auto& lattOG = group.second.get_child_optional("og.lat");
		const auto& opsOG = group.second.get_child_optional("og.ops");
		const auto& wycOG = group.second.get_child_optional("og.wyc");

		const auto& bns2og = group.second.get_child_optional("bns2og");
		// --------------------------------------------------------------------


		// --------------------------------------------------------------------
		Spacegroup<t_mat, t_vec> sg;

		sg.m_nameBNS = *nameBNS; boost::trim(sg.m_nameBNS);
		sg.m_nameOG = *nameOG; boost::trim(sg.m_nameOG);
		sg.m_nrBNS = *nrBNS; boost::trim(sg.m_nrBNS);
		sg.m_nrOG = *nrOG; boost::trim(sg.m_nrOG);

		// split BNS space group number in structural and magnetic part
		std::vector<std::string> vecNumbers;
		boost::split(vecNumbers, sg.m_nrBNS, [](auto c)->bool {return c=='.';}, boost::token_compress_on);
		if(vecNumbers.size() < 2)
		{	// purely-structural space group
			std::cerr << "Non-magnetic space group: " << sg.m_nrBNS << std::endl;
		}
		else if(vecNumbers.size() > 2)
		{	// unknown
			std::cerr << "Unknown space group number: " << sg.m_nrBNS << std::endl;
		}
		else
		{	// magnetic space group
			sg.m_sgnrStruct = std::stoi(vecNumbers[0]);
			sg.m_sgnrMag = std::stoi(vecNumbers[1]);
		}
		// --------------------------------------------------------------------



		// --------------------------------------------------------------------
		// reads in a vector
		auto get_vec = [](const std::string& str) -> t_vec
		{
			t_vec vec = m::zero<t_vec>(3);

			// abbreviations
			if(str == "0")
				;
			else if(str == "x")
				vec = m::create<t_vec>({1,0,0});
			else if(str == "y")
				vec = m::create<t_vec>({0,1,0});
			else if(str == "z")
				vec = m::create<t_vec>({0,0,1});
			else if(str == "-x")
				vec = m::create<t_vec>({-1,0,0});
			else if(str == "-y")
				vec = m::create<t_vec>({0,-1,0});
			else if(str == "-z")
				vec = m::create<t_vec>({0,0,-1});
			else
			{
				// read vector
				std::istringstream istr(str);
				for(std::size_t i=0; i<vec.size(); ++i)
					istr >> vec[i];
			}

			return vec;
		};

		// reads in a matrix
		auto get_mat = [](const std::string& str) -> t_mat
		{
			t_mat mat = m::zero<t_mat>(3,3);

			// abbreviations
			if(str == "0")
				;
			else if(str == "1")
				mat = m::unit<t_mat>(3);
			else
			{
				// read matrix
				std::istringstream istr(str);
				for(std::size_t i=0; i<mat.size1(); ++i)
					for(std::size_t j=0; j<mat.size2(); ++j)
						istr >> mat(i,j);
			}

			return mat;
		};

		// transforms a BNS vector to OG
		// TODO: check!
		auto calc_bns2og = [](const auto& rotBNS2OG, const auto& transBNS2OG, auto *vecs) -> void
		{
			for(auto& vec : *vecs)
				vec = rotBNS2OG*vec + transBNS2OG;
		};
		// --------------------------------------------------------------------



		// --------------------------------------------------------------------
		// BNS to OG trafo
		if(bns2og)
		{
			auto opBNS2OGTrafo = bns2og->get_optional<std::string>("R");
			auto opBNS2OGTrans = bns2og->get_optional<std::string>("v");
			auto opBNS2OGdiv = bns2og->get_optional<t_real>("d");

			sg.m_rotBNS2OG = opBNS2OGTrafo ? get_mat(*opBNS2OGTrafo) : m::unit<t_mat>(3,3);
			sg.m_transBNS2OG = opBNS2OGTrans ? get_vec(*opBNS2OGTrans) : m::zero<t_vec>(3);
			t_real divBNS2OG = opBNS2OGdiv ? *opBNS2OGdiv : t_real(1);
			sg.m_transBNS2OG /= divBNS2OG;
		}
		// --------------------------------------------------------------------



		// --------------------------------------------------------------------
		// iterate symmetry trafos
		auto load_ops = [&get_vec, &get_mat](const decltype(opsBNS)& ops)
		-> std::tuple<std::vector<t_mat>, std::vector<t_vec>, std::vector<t_real>>
		{
			std::vector<t_mat> rotations;
			std::vector<t_vec> translations;
			std::vector<t_real> inversions;

			for(std::size_t iOp=1; true; ++iOp)
			{
				std::string strOp = std::to_string(iOp);
				std::string nameTrafo = "R" + strOp;
				std::string nameTrans = "v" + strOp;
				std::string nameDiv = "d" + strOp;
				std::string nameInv = "t" + strOp;

				auto opTrafo = ops->get_optional<std::string>(nameTrafo);
				auto opTrans = ops->get_optional<std::string>(nameTrans);
				auto opdiv = ops->get_optional<t_real>(nameDiv);
				auto opinv = ops->get_optional<t_real>(nameInv);

				if(!opTrafo)
					break;

				t_real div = opdiv ? *opdiv : t_real(1);
				t_real inv = opinv ? *opinv : t_real(1);
				t_mat rot = opTrafo ? get_mat(*opTrafo) : m::unit<t_mat>(3,3);
				t_vec trans = opTrans ? get_vec(*opTrans) : m::zero<t_vec>(3);
				trans /= div;

				rotations.emplace_back(std::move(rot));
				translations.emplace_back(std::move(trans));
				inversions.push_back(inv);
			}

			return std::make_tuple(std::move(rotations), std::move(translations), std::move(inversions));
		};

		if(opsBNS)
		{
			sg.m_symBNS = std::make_shared<Symmetry<t_mat, t_vec>>();
			std::tie(sg.m_symBNS->m_rot, sg.m_symBNS->m_trans, sg.m_symBNS->m_inv) = std::move(load_ops(opsBNS));
		}
		if(opsOG)
		{
			sg.m_symOG = std::make_shared<Symmetry<t_mat, t_vec>>();
			std::tie(sg.m_symOG->m_rot, sg.m_symOG->m_trans, sg.m_symOG->m_inv) = std::move(load_ops(opsOG));
		}
		else
		{
			if(!bns2og)
			{
				// if neither OG nor BNS to OG trafo are defined, OG is identical to BNS
				sg.m_symOG = sg.m_symBNS;
			}
			else
			{
				// calculate OG from BNS using trafo
				sg.m_symOG = std::make_shared<Symmetry<t_mat, t_vec>>(*sg.m_symBNS);
				calc_bns2og(sg.m_rotBNS2OG, sg.m_transBNS2OG, &sg.m_symOG->m_trans);

				//std::cout << "bns2og trafo for sg " << sg.GetNumber() << std::endl;
			}
		}
		// --------------------------------------------------------------------



		// --------------------------------------------------------------------
		// iterate over lattice vectors
		auto load_latt = [&get_vec](const decltype(lattBNS)& latt)
			-> std::vector<t_vec>
		{
			std::vector<t_vec> vectors;

			for(std::size_t iVec=1; true; ++iVec)
			{
				std::string strVec = std::to_string(iVec);
				std::string nameVec = "v" + strVec;
				std::string nameDiv = "d" + strVec;

				auto opVec = latt->get_optional<std::string>(nameVec);
				auto opdiv = latt->get_optional<t_real>(nameDiv);

				if(!opVec)
					break;

				t_real div = opdiv ? *opdiv : t_real(1);
				t_vec vec = opVec ? get_vec(*opVec) : m::zero<t_vec>(3);
				vec /= div;

				vectors.emplace_back(std::move(vec));
			}

			return std::move(vectors);
		};

		if(lattBNS)
		{
			sg.m_latticeBNS = std::make_shared<std::vector<t_vec>>();
			*sg.m_latticeBNS = std::move(load_latt(lattBNS));
		}
		if(lattOG)
		{
			sg.m_latticeOG = std::make_shared<std::vector<t_vec>>();
			*sg.m_latticeOG = std::move(load_latt(lattOG));
		}
		else
		{
			if(!bns2og)
			{
				// if neither OG nor BNS to OG trafo are defined, OG is identical to BNS
				sg.m_latticeOG = sg.m_latticeBNS;
			}
			else
			{
				// calculate OG from BNS using trafo
				sg.m_latticeOG = std::make_shared<std::vector<t_vec>>(*sg.m_latticeBNS);
				calc_bns2og(sg.m_rotBNS2OG, sg.m_transBNS2OG, sg.m_latticeOG.get());
			}
		}
		// --------------------------------------------------------------------


		// --------------------------------------------------------------------
		// iterate wyckoff positions
		auto load_wyc = [&get_vec, &get_mat](const decltype(wycBNS)& wycs)
		-> std::vector<WycPositions<t_mat, t_vec>>
		{
			std::vector<WycPositions<t_mat, t_vec>> vecWyc;

			for(std::size_t iWyc=1; true; ++iWyc)
			{
				std::string nameSite = "s" + std::to_string(iWyc);
				auto wyc = wycs->get_child_optional(nameSite);
				if(!wyc) break;

				WycPositions<t_mat, t_vec> wycpos;

				auto opLetter = wyc->get_optional<std::string>("l");
				auto opMult = wyc->get_optional<int>("m");
				if(opLetter) wycpos.m_letter = *opLetter;
				wycpos.m_mult = opMult ? *opMult : 0;


				for(std::size_t iPos=1; true; ++iPos)
				{
					std::string strPos = std::to_string(iPos);
					std::string nameRot = "R" + strPos;
					std::string nameRotMag = "M" + strPos;
					std::string nameTrans = "v" + strPos;
					std::string nameDiv = "d" + strPos;

					auto opRot = wyc->get_optional<std::string>(nameRot);
					auto opRotMag = wyc->get_optional<std::string>(nameRotMag);
					auto opTrans = wyc->get_optional<std::string>(nameTrans);
					auto opdiv = wyc->get_optional<t_real>(nameDiv);

					if(!opRot)
						break;

					t_real div = opdiv ? *opdiv : t_real(1);
					t_mat rot = opRot ? get_mat(*opRot) : m::unit<t_mat>(3,3);
					t_mat rotMag = opRotMag ? get_mat(*opRotMag) : rot;
					t_vec trans = opTrans ? get_vec(*opTrans) : m::zero<t_vec>(3);
					trans /= div;

					wycpos.m_rot.emplace_back(std::move(rot));
					wycpos.m_rotMag.emplace_back(std::move(rotMag));
					wycpos.m_trans.emplace_back(std::move(trans));
				}

				vecWyc.emplace_back(std::move(wycpos));
			}

			return std::move(vecWyc);
		};


		if(wycBNS)
		{
			sg.m_wycBNS = std::make_shared<std::vector<WycPositions<t_mat, t_vec>>>();
			*sg.m_wycBNS = std::move(load_wyc(wycBNS));
		}
		if(wycOG)
		{
			sg.m_wycOG = std::make_shared<std::vector<WycPositions<t_mat, t_vec>>>();
			*sg.m_wycOG = std::move(load_wyc(wycOG));
		}
		else
		{
			if(!bns2og)
			{
				// if neither OG nor BNS to OG trafo are defined, OG is identical to BNS
				sg.m_wycOG = sg.m_wycBNS;
			}
			else
			{
				// calculate OG from BNS using trafo
				sg.m_wycOG = std::make_shared<std::vector<WycPositions<t_mat, t_vec>>>(*sg.m_wycBNS);

				for(auto& wycpos : *sg.m_wycOG)
					calc_bns2og(sg.m_rotBNS2OG, sg.m_transBNS2OG, &wycpos.m_trans);
			}
		}
		// --------------------------------------------------------------------


		m_sgs.emplace_back(std::move(sg));
	}


	return true;
}
// ----------------------------------------------------------------------------


#endif
