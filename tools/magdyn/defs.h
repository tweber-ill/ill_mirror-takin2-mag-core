/**
 * type definitions
 * @author Tobias Weber <tweber@ill.fr>
 * @date jan-2022
 * @license GPLv3
 */

#ifndef __DEFS_H__
#define __DEFS_H__

#include <vector>
#include <complex>

#define USE_LAPACK 1
#include "tlibs2/libs/maths.h"


using t_size = std::size_t;
using t_real = double;
using t_cplx = std::complex<t_real>;

using t_vec = tl2::vec<t_cplx, std::vector>;
using t_mat = tl2::mat<t_cplx, std::vector>;


#endif
