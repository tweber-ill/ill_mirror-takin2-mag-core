/**
 * swig interface
 * @author Tobias Weber <tweber@ill.fr>
 * @date 25-april-2023
 * @license see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2023  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 * "misc" project
 * Copyright (C) 2017-2021  Tobias WEBER (privately developed).
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

%module bzcalc
%{
	#include "bzlib.h"

	using t_matD = tl2::mat<double, std::vector>;
	using t_vecD = tl2::vec<double, std::vector>;
%}


%include "std_vector.i"
%include "std_string.i"

%template(VecD) std::vector<double>;
%template(VecUI) std::vector<unsigned int>;
//%template(Vectvec) std::vector<t_vecD>;


%include "bzlib.h"

%template(BZCalcD) BZCalc<t_matD, t_vecD, double>;
