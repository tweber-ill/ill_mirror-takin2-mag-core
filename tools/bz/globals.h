/**
 * brillouin zone tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date Maz-2022
 * @license GPLv3, see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2022  Tobias WEBER (Institut Laue-Langevin (ILL),
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

#ifndef __BZTOOL_GLOBALS_H__
#define __BZTOOL_GLOBALS_H__


#include <boost/math/quaternion.hpp>
namespace math = boost::math;

#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/qt/gl.h"

using t_real = double;
using t_vec = tl2::vec<t_real, std::vector>;
using t_mat = tl2::mat<t_real, std::vector>;
using t_quat = math::quaternion<t_real>;

using t_real_gl = tl2::t_real_gl;
using t_vec2_gl = tl2::t_vec2_gl;
using t_vec3_gl = tl2::t_vec3_gl;
using t_vec_gl = tl2::t_vec_gl;
using t_mat_gl = tl2::t_mat_gl;


constexpr t_real g_eps = 1e-6;
constexpr int g_prec = 6;
constexpr int g_prec_gui = 4;


#endif
