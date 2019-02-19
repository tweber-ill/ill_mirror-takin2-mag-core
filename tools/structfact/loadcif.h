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
 * gets the symmetry operations from the CIF
 */
template<class t_vec, class t_mat, class t_real = typename t_vec::value_type>
std::vector<t_mat> get_cif_ops(gemmi::cif::Block& block)
{
	std::vector<t_mat> ops;

	auto colOps = block.find_values("_symmetry_equiv_pos_as_xyz");
	for(std::size_t row=0; row<colOps.length(); ++row)
	{
		auto op = gemmi::parse_triplet(colOps[row])/*.wrap()*/;
		auto M = op.float_seitz();

		t_mat mat = m::create<t_mat>({
			std::get<0>(std::get<0>(M)), std::get<1>(std::get<0>(M)), std::get<2>(std::get<0>(M)), std::get<3>(std::get<0>(M)),
			std::get<0>(std::get<1>(M)), std::get<1>(std::get<1>(M)), std::get<2>(std::get<1>(M)), std::get<3>(std::get<1>(M)),
			std::get<0>(std::get<2>(M)), std::get<1>(std::get<2>(M)), std::get<2>(std::get<2>(M)), std::get<3>(std::get<2>(M)),
			std::get<0>(std::get<3>(M)), std::get<1>(std::get<3>(M)), std::get<2>(std::get<3>(M)), std::get<3>(std::get<3>(M)) });

		ops.emplace_back(std::move(mat));
	}

	return ops;
}



/**
 * gets the symmetry operations from the CIF's space group
 */
template<class t_vec, class t_mat, class t_real = typename t_vec::value_type>
std::vector<t_mat> get_cif_sg_ops(gemmi::cif::Block& block)
{
	std::vector<t_mat> ops;

	if(auto val = block.find_values("_symmetry_space_group_name_H-M"); val.length())
	{
		std::string sgname = boost::trim_copy(val[0]);

		// remove quotation marks
		if(sgname[0] == '\'' || sgname[0] == '\"')
			sgname.erase(sgname.begin());
		if(sgname[sgname.size()-1] == '\'' || sgname[sgname.size()-1] == '\"')
			sgname.erase(sgname.begin()+sgname.size()-1);

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
std::tuple<const char*, std::vector<t_vec>, std::vector<std::vector<t_vec>>, std::vector<std::string>, Lattice<t_real>> 
load_cif(const std::string& filename, t_real eps=1e-6)
{
	auto ifstr = std::ifstream(filename);
	if(!ifstr)
		return std::make_tuple("Cannot open CIF.", std::vector<t_vec>{}, std::vector<std::vector<t_vec>>{}, std::vector<std::string>{}, Lattice{});

	// load CIF
	auto cif = gemmi::cif::read_istream(ifstr, 4096, filename.c_str());

	if(!cif.blocks.size())
		return std::make_tuple("No blocks in CIF.", std::vector<t_vec>{}, std::vector<std::vector<t_vec>>{}, std::vector<std::string>{}, Lattice{});

	// get the block
	/*const*/ auto& block = cif.sole_block();


	// lattice
	t_real a{}, b{}, c{}, alpha{}, beta{}, gamma{};
	if(auto val = block.find_values("_cell_length_a"); val.length()) std::istringstream{val[0]} >> a;
	if(auto val = block.find_values("_cell_length_b"); val.length()) std::istringstream{val[0]} >> b;
	if(auto val = block.find_values("_cell_length_c"); val.length()) std::istringstream{val[0]} >> c;
	if(auto val = block.find_values("_cell_angle_alpha"); val.length()) std::istringstream{val[0]} >> alpha;
	if(auto val = block.find_values("_cell_angle_beta"); val.length()) std::istringstream{val[0]} >> beta;
	if(auto val = block.find_values("_cell_angle_gamma"); val.length()) std::istringstream{val[0]} >> gamma;

	Lattice<t_real> latt{.a=a, .b=b, .c=c, .alpha=alpha, .beta=beta, .gamma=gamma};


	// fractional atom positions
	std::vector<t_vec> atoms;
	auto tabAtoms = block.find("_atom_site", {"_type_symbol", "_fract_x", "_fract_y", "_fract_z"});

	std::vector<std::string> atomnames;
	for(std::size_t row=0; row<tabAtoms.length(); ++row)
	{
		atomnames.push_back(tabAtoms[row][0]);

		t_real x{}, y{}, z{};
		std::istringstream{tabAtoms[row][1]} >> x;
		std::istringstream{tabAtoms[row][2]} >> y;
		std::istringstream{tabAtoms[row][3]} >> z;
		atoms.emplace_back(t_vec{{x, y, z}});
	}


	// generate all atoms using symmetry ops
	std::vector<std::vector<t_vec>> generatedatoms;
	auto ops = get_cif_ops<t_vec, t_mat, t_real>(block);
	if(!ops.size()) // if ops are not directly given, use standard ops from space group
		ops = get_cif_sg_ops<t_vec, t_mat, t_real>(block);

	for(t_vec atom : atoms)
	{
		// make homogeneuous 4-vector
		if(atom.size() == 3) atom.push_back(1);

		std::vector<t_vec> newatoms = m::apply_ops_hom<t_vec, t_mat, t_real>(atom, ops, eps);

		// if no ops are given, just output the raw atom position
		if(!ops.size())
			generatedatoms.push_back(std::vector<t_vec>{{atom}});

		generatedatoms.emplace_back(std::move(newatoms));
	}


	return std::make_tuple(nullptr, atoms, generatedatoms, atomnames, latt);
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
