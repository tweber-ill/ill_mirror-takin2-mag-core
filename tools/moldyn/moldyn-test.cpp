/**
 * loads atom dynamics file
 * @author Tobias Weber <tweber@ill.fr>
 * @date Dec-2019
 * @license GPLv3, see 'LICENSE' file
 *
 * g++ -std=c++17  -I ../../tlibs2 -o moldyn-test moldyn-test.cpp
 */

#include "moldyn-loader.h"


using t_real = double;
using t_vec = std::vector<t_real>;


int main(int argc, char** argv)
{
	if(argc <= 1)
		return -1;

	MolDyn<t_real, t_vec> mol;
	mol.LoadFile(argv[1], 100);

	return 0;
}
