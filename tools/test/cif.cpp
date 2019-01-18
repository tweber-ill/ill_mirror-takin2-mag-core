/**
 * cif test
 * @author Tobias Weber <tweber@ill.fr>
 * @date jan-19
 * @license GPLv3, see 'LICENSE' file
 *
 * clang++ -std=c++17 -o cif cif.cpp -I../../ext/gemmi/include/gemmi -I../../ext/gemmi/third_party
 */

#include <string>
#include <fstream>
#include <iostream>

#include <cif.hpp>
#include <symmetry.hpp>


std::string get_item_type(gemmi::cif::ItemType ty)
{
	switch(ty)
	{
		case gemmi::cif::ItemType::Pair: return "pair";
		case gemmi::cif::ItemType::Loop: return "loop";
		case gemmi::cif::ItemType::Frame: return "frame";
		case gemmi::cif::ItemType::Comment: return "comment";
		case gemmi::cif::ItemType::Erased: return "erased";
	}

	return "unknown";
}


int main(int argc, char** argv)
{
	if(argc < 2)
	{
		std::cerr << "Please give a CIF file." << std::endl;
		return -1;
	}

	auto ifstr = std::ifstream(argv[1]);
	if(!ifstr)
	{
		std::cerr << "Cannot open file." << std::endl;
		return -1;
	}

	// load CIF
	auto cif = gemmi::cif::read_istream(ifstr, 4096, argv[1]);
	std::cout << cif.blocks.size() << " block(s) in CIF." << std::endl;

	// get the block
	/*const*/ auto& block = cif.sole_block();
	std::cout << "Block \"" << block.name << "\" has " << block.items.size() << " items." << std::endl;

	// iterate all items in block
	for(const auto& item : block.items)
	{
		std::cout << "Item type " << get_item_type(item.type) << " at line " << item.line_number << ": ";

		if(item.type == gemmi::cif::ItemType::Pair)
		{
			std::cout << std::get<0>(item.pair) << " = " << std::get<1>(item.pair);
		}
		else if(item.type == gemmi::cif::ItemType::Loop)
		{
			std::cout << "tags: ";
			for(std::size_t tag=0; tag<item.loop.tags.size(); ++tag)
				std::cout << item.loop.tags[tag] << ", ";
			std::cout << "\nvalues: ";
			for(std::size_t val=0; val<item.loop.values.size(); ++val)
				std::cout << "\"" << item.loop.values[val] << "\", ";
		}

		std::cout << std::endl;
	}

	std::cout << std::endl;


	// testing columns
	{
		auto colSym = block.find_values("_atom_site_type_symbol");
		auto colX = block.find_values("_atom_site_fract_x");
		auto colY = block.find_values("_atom_site_fract_y");
		auto colZ = block.find_values("_atom_site_fract_z");

		for(std::size_t row=0; row<colSym.length(); ++row)
			std::cout << colSym[row] << ": " << colX[row] << ", " << colY[row] << ", " << colZ[row] << std::endl;

	}

	std::cout << std::endl;


	// testing table
	{
		auto tab = block.find("_atom_site", {"_type_symbol", "_fract_x", "_fract_y", "_fract_z"});

		for(std::size_t row=0; row<tab.length(); ++row)
		{
			for(std::size_t col=0; col<tab.width(); ++col)
				std::cout << tab[row][col] << ", ";
			std::cout << "\n";
		}
	}

	std::cout << std::endl;


	// testing symmetry ops
	{
		auto colOps = block.find_values("_symmetry_equiv_pos_as_xyz");
		for(std::size_t row=0; row<colOps.length(); ++row)
		{
			//std::cout << colOps[row] << std::endl;
			auto op = gemmi::parse_triplet(colOps[row]);
			auto M = op.float_seitz();

			std::cout << std::get<0>(std::get<0>(M)) << " " << std::get<1>(std::get<0>(M)) << " " << std::get<2>(std::get<0>(M)) << " " << std::get<3>(std::get<0>(M)) << "\n";
			std::cout << std::get<0>(std::get<1>(M)) << " " << std::get<1>(std::get<1>(M)) << " " << std::get<2>(std::get<1>(M)) << " " << std::get<3>(std::get<1>(M)) << "\n";
			std::cout << std::get<0>(std::get<2>(M)) << " " << std::get<1>(std::get<2>(M)) << " " << std::get<2>(std::get<2>(M)) << " " << std::get<3>(std::get<2>(M)) << "\n";
			std::cout << std::get<0>(std::get<3>(M)) << " " << std::get<1>(std::get<3>(M)) << " " << std::get<2>(std::get<3>(M)) << " " << std::get<3>(std::get<3>(M)) << "\n";
			std::cout << std::endl;
		}
	}

	std::cout << std::endl;


	// testing space group
	{
		auto val = block.find_values("_symmetry_space_group_name_H-M");
		if(val.length())
		{
			std::string sgname = val[0];
			if(sgname[0] == '\'' || sgname[0] == '\"')
			{
				sgname.erase(sgname.begin());
				sgname.erase(sgname.begin()+sgname.size()-1);
			}

			if(auto sg = gemmi::find_spacegroup_by_name(sgname))
				std::cout << "Space group number " << sg->number << std::endl;
		}
	}

	std::cout << std::endl;

	return 0;
}
