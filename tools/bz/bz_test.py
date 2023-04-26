#
# brillouin zone scripting test
# @author Tobias Weber <tweber@ill.fr>
# @date April-2023
# @license GPLv3, see 'LICENSE' file
#

import sys
import os

sys.path.append(os.getcwd())


import bzcalc

bz = bzcalc.BZCalcD()
bz.SetEps(1e-6)

bz.SetCrystal(5, 5, 5, 90, 90, 90)

num_ops = bz.SetSymOpsFromSpaceGroup("F d -3 m")
print("Using %d centring symops." % num_ops)

num_peaks = bz.CalcPeaks(4, True)
print("Using %d reflections." % num_peaks)

calc_ok = bz.CalcBZ()
if calc_ok:
	print("Brillouin zone calculation successful.")
else:
	print("Brillouin zone calculation failed.")

if calc_ok:
	print("\nJSON Output:")
	json = bz.PrintJSON(6)
	print(json)
