/**
 * space group test
 * @author Tobias Weber <tweber@ill.fr>
 * @date jan-19
 * @license GPLv3, see 'LICENSE' file
 *
 * clang++ -std=c++17 -o sg sg.cpp -I../../ext/gemmi/include/gemmi -I../../ext/gemmi/third_party
 */

#include <string>
#include <iostream>

#include <symmetry.hpp>


int main()
{
	// testing space group
	for(const auto &sg : gemmi::spacegroup_tables::main)
	{
		auto ops = sg.operations().all_ops_sorted();
		std::cout << "SG #" << sg.number << ", HM: " << sg.hm << ", Hall: " << sg.hall
			<< ", Qualifier: " << sg.qualifier << ", Ext: " << sg.ext
			<< ", Op count: " << ops.size() << std::endl;

		/*for(const auto &op : ops)
		{
			auto M = op.float_seitz();
		}*/
	}

	return 0;
}
