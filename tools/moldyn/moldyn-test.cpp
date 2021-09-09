/**
 * loads atom dynamics file
 * @author Tobias Weber <tweber@ill.fr>
 * @date Dec-2019
 * @license GPLv3, see 'LICENSE' file
 *
 * g++ -std=c++17  -I ../../tlibs2 -o moldyn-test moldyn-test.cpp
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
