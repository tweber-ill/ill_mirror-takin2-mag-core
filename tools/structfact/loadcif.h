/**
 * get atom positions from cif
 * @author Tobias Weber <tweber@ill.fr>
 * @date Jan-2019
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __LOAD_CIF_H__
#define __LOAD_CIF_H__

#include <vector>
#include <string>
//#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/algorithm/string.hpp>

#include <gemmi/cif.hpp>
#include <gemmi/symmetry.hpp>

#include "libs/_cxx20/math_algos.h"
using namespace m_ops;



template<class t_real=double>
struct Lattice
{
	t_real a, b, c;
	t_real alpha, beta, gamma;
};



/**
 * removes quotation marks around a string
 */
template<class t_str = std::string>
void remove_quotes(t_str& str)
{
	if(str.length() == 0)
		return;

	if(str[0] == '\'' || str[0] == '\"')
		str.erase(str.begin());
	if(str[str.size()-1] == '\'' || str[str.size()-1] == '\"')
		str.erase(str.begin()+str.size()-1);
}



/**
 * gets the atom positions from the CIF
 */
template<class t_vec, class t_real = typename t_vec::value_type>
std::tuple<std::vector<t_vec>, std::vector<std::string>> 
get_cif_atoms(gemmi::cif::Block& block)
{
	std::vector<t_vec> atoms;
	std::vector<std::string> atomnames;

	// possible names for atom symbol
	const char* loopNames[] =
	{
		"_type_symbol",
		"_symbol"
		"_site_label",
		"_label",
	};

	for(const char* loopName : loopNames)
	{
		atoms.clear();
		atomnames.clear();

		auto tabAtoms = block.find("_atom_site", {loopName, "_fract_x", "_fract_y", "_fract_z"});

		for(std::size_t row=0; row<tabAtoms.length(); ++row)
		{
			auto name = boost::trim_copy(tabAtoms[row][0]);
			remove_quotes(name);
			atomnames.emplace_back(std::move(name));

			const t_real x = m::stoval<t_real>(tabAtoms[row][1]);
			const t_real y = m::stoval<t_real>(tabAtoms[row][2]);
			const t_real z = m::stoval<t_real>(tabAtoms[row][3]);
			atoms.emplace_back(t_vec{{x, y, z}});
		}

		// already found, no need to try another name
		if(tabAtoms.length())
			break;
	}

	return std::make_tuple(atoms, atomnames);
}



/**
 * gets the symmetry operations from the CIF
 */
template<class t_vec, class t_mat, class t_real = typename t_vec::value_type>
std::vector<t_mat> get_cif_ops(gemmi::cif::Block& block)
{
	std::vector<t_mat> ops;

	// possible names for symop loop
	const char* loopNames[] =
	{
		"_symmetry_equiv_pos_as_xyz",
		"_space_group_symop_operation_xyz",
	};

	for(const char* loopName : loopNames)
	{
		ops.clear();

		//std::cerr << "Trying symop loop name " << loopName << std::endl;
	 	auto colOps = block.find_values(loopName);

		for(std::size_t row=0; row<colOps.length(); ++row)
		{
			auto therow = boost::trim_copy(colOps[row]);
			remove_quotes(therow);

			auto op = gemmi::parse_triplet(therow)/*.wrap()*/;
			auto M = op.float_seitz();

			t_mat mat = m::create<t_mat>({
				std::get<0>(std::get<0>(M)), std::get<1>(std::get<0>(M)), std::get<2>(std::get<0>(M)), std::get<3>(std::get<0>(M)),
				std::get<0>(std::get<1>(M)), std::get<1>(std::get<1>(M)), std::get<2>(std::get<1>(M)), std::get<3>(std::get<1>(M)),
				std::get<0>(std::get<2>(M)), std::get<1>(std::get<2>(M)), std::get<2>(std::get<2>(M)), std::get<3>(std::get<2>(M)),
				std::get<0>(std::get<3>(M)), std::get<1>(std::get<3>(M)), std::get<2>(std::get<3>(M)), std::get<3>(std::get<3>(M)) });

			ops.emplace_back(std::move(mat));
		}

		// already found, no need to try another name
		if(colOps.length())
			break;
	}

	return ops;
}



/**
 * gets the symmetry operations from the CIF's space group
 * (use m::equals_all to check if space group operations are the same)
 */
template<class t_vec, class t_mat, class t_real = typename t_vec::value_type>
std::vector<t_mat> get_cif_sg_ops(gemmi::cif::Block& block)
{
	std::vector<t_mat> ops;

	if(auto val = block.find_values("_symmetry_space_group_name_H-M"); val.length())
	{
		std::string sgname = boost::trim_copy(val[0]);
		remove_quotes(sgname);

		if(auto sg = gemmi::find_spacegroup_by_name(sgname))
		{
			auto symops = sg->operations().all_ops_sorted();
			for(const auto &op : symops)
			{
				auto M = op.float_seitz();

				t_mat mat = m::create<t_mat>({
					std::get<0>(std::get<0>(M)), std::get<1>(std::get<0>(M)), std::get<2>(std::get<0>(M)), std::get<3>(std::get<0>(M)),
					std::get<0>(std::get<1>(M)), std::get<1>(std::get<1>(M)), std::get<2>(std::get<1>(M)), std::get<3>(std::get<1>(M)),
					std::get<0>(std::get<2>(M)), std::get<1>(std::get<2>(M)), std::get<2>(std::get<2>(M)), std::get<3>(std::get<2>(M)),
					std::get<0>(std::get<3>(M)), std::get<1>(std::get<3>(M)), std::get<2>(std::get<3>(M)), std::get<3>(std::get<3>(M)) });

				ops.emplace_back(std::move(mat));
			}
		}
	}

	return ops;
}



/**
 * loads the lattice parameters and the atom positions from a CIF
 */
template<class t_vec, class t_mat, class t_real = typename t_vec::value_type>
std::tuple<
	std::string /* errors and warnings */, 
	std::vector<t_vec> /* basic atom positions */, 
	std::vector<std::vector<t_vec>> /* all generated atoms */, 
	std::vector<std::string> /* atom names */, 
	Lattice<t_real> /* lattice */ ,
	std::vector<t_mat> /* ops */ > 
load_cif(const std::string& filename, t_real eps=1e-6)
{
	auto ifstr = std::ifstream(filename);
	if(!ifstr)
		return std::make_tuple("Cannot open CIF.", std::vector<t_vec>{}, std::vector<std::vector<t_vec>>{}, std::vector<std::string>{}, Lattice{}, std::vector<t_mat>{});

	// load CIF
	auto cif = gemmi::cif::read_istream(ifstr, 4096, filename.c_str());

	if(!cif.blocks.size())
		return std::make_tuple("No blocks in CIF.", std::vector<t_vec>{}, std::vector<std::vector<t_vec>>{}, std::vector<std::string>{}, Lattice{}, std::vector<t_mat>{});

	// get the block
	/*const*/ auto& block = cif.sole_block();


	// lattice
	t_real a{}, b{}, c{}, alpha{}, beta{}, gamma{};
	if(auto val = block.find_values("_cell_length_a"); val.length()) a = m::stoval<t_real>(val[0]);
	if(auto val = block.find_values("_cell_length_b"); val.length()) b = m::stoval<t_real>(val[0]);
	if(auto val = block.find_values("_cell_length_c"); val.length()) c = m::stoval<t_real>(val[0]);
	if(auto val = block.find_values("_cell_angle_alpha"); val.length()) alpha = m::stoval<t_real>(val[0]);
	if(auto val = block.find_values("_cell_angle_beta"); val.length()) beta = m::stoval<t_real>(val[0]);
	if(auto val = block.find_values("_cell_angle_gamma"); val.length()) gamma = m::stoval<t_real>(val[0]);

	Lattice<t_real> latt{.a=a, .b=b, .c=c, .alpha=alpha, .beta=beta, .gamma=gamma};


	// fractional atom positions
	auto [atoms, atomnames] = get_cif_atoms<t_vec>(block);


	// generate all atoms using symmetry ops
	std::ostringstream errstr;
	std::vector<std::vector<t_vec>> generatedatoms;
	auto ops = get_cif_ops<t_vec, t_mat, t_real>(block);
	if(!ops.size()) // if ops are not directly given, use standard ops from space group
	{
		errstr << "Warning: Could not find CIF symops, trying to use space group defaults instead.\n";
		ops = get_cif_sg_ops<t_vec, t_mat, t_real>(block);
		if(!ops.size())
			errstr << "Warning: No symops found!\n";
	}

	for(t_vec atom : atoms)
	{
		// make homogeneuous 4-vector
		if(atom.size() == 3) atom.push_back(1);

		// if no ops are given, just output the raw atom position
		if(!ops.size())
		{
			generatedatoms.push_back(std::vector<t_vec>{{atom}});
			continue;
		}

		std::vector<t_vec> newatoms = m::apply_ops_hom<t_vec, t_mat, t_real>(atom, ops, eps);
		generatedatoms.emplace_back(std::move(newatoms));
	}

	return std::make_tuple(errstr.str(), atoms, generatedatoms, atomnames, latt, ops);
}



/**
 * gets space group description strings and symmetry operations
 */
template<class t_mat, class t_real = typename t_mat::value_type>
std::vector<std::tuple<int, std::string, std::vector<t_mat>>>
get_sgs(bool bAddNr=true, bool bAddHall=true)
{
	std::vector<std::tuple<int, std::string, std::vector<t_mat>>> sgs;

	for(const auto &sg : gemmi::spacegroup_tables::main)
	{
		std::ostringstream ostrDescr;
		if(bAddNr) ostrDescr << "#" << sg.number << ": ";
		ostrDescr << sg.hm;
		if(bAddHall) ostrDescr << " (" << sg.hall << ")";

		std::vector<t_mat> ops;
		for(const auto &op : sg.operations().all_ops_sorted())
		{
			auto M = op.float_seitz();

			t_mat mat = m::create<t_mat>({
				std::get<0>(std::get<0>(M)), std::get<1>(std::get<0>(M)), std::get<2>(std::get<0>(M)), std::get<3>(std::get<0>(M)),
				std::get<0>(std::get<1>(M)), std::get<1>(std::get<1>(M)), std::get<2>(std::get<1>(M)), std::get<3>(std::get<1>(M)),
				std::get<0>(std::get<2>(M)), std::get<1>(std::get<2>(M)), std::get<2>(std::get<2>(M)), std::get<3>(std::get<2>(M)),
				std::get<0>(std::get<3>(M)), std::get<1>(std::get<3>(M)), std::get<2>(std::get<3>(M)), std::get<3>(std::get<3>(M)) });

			ops.emplace_back(std::move(mat));
		}

		sgs.emplace_back(std::make_tuple(sg.number, ostrDescr.str(), ops));
	}

	std::stable_sort(sgs.begin(), sgs.end(), [](const auto& sg1, const auto& sg2) -> bool
	{
		return std::get<0>(sg1) < std::get<0>(sg2);
	});

	return sgs;
}


#endif
