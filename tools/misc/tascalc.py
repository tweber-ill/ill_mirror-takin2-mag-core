#
# calculates TAS angles from rlu (part of in20tools)
# @author Tobias Weber <tweber@ill.fr>
# @date 1-aug-18
# @license see 'LICENSE' file
#

import numpy as np
import numpy.linalg as la


use_scipy = False
#a3_offs = np.pi
a3_offs = 0.


# -----------------------------------------------------------------------------
if use_scipy:
	import scipy as sp
	import scipy.constants as co

	hbar_in_meVs = co.Planck/co.elementary_charge*1000./2./np.pi
	E_to_k2 = 2.*co.neutron_mass/hbar_in_meVs**2. / co.elementary_charge*1000. * 1e-20
else:
	E_to_k2 = 0.482596406464	# calculated with scipy, using the formula above

#print(1./E_to_k2)
# -----------------------------------------------------------------------------


# A1 & A2 angles (also A5 & A6)
def get_a1a2(k, d):
	s = np.pi/(d*k)
	a1 = np.arcsin(s)
	return [a1, 2.*a1]


# Scattering angle
def get_a4(ki, kf, Q):
	c = (ki**2. + kf**2. - Q**2.) / (2.*ki*kf)
	return np.arccos(c)


# Angle enclosed by ki and Q
def get_psi(ki, kf, Q):
	c = (ki**2. + Q**2. - kf**2.) / (2.*ki*Q)
	return np.arccos(c)


# Crystallographic A matrix converting fractional to lab coordinates
def get_A(lattice, angles):
	cs = np.cos(angles)
	s2 = np.sin(angles[2])

	a = lattice[0] * np.array([1, 0, 0])
	b = lattice[1] * np.array([cs[2], s2, 0])
	c = lattice[2] * np.array([cs[1], \
		(cs[0]-cs[1]*cs[2]) / s2, \
		(np.sqrt(1. - np.dot(cs,cs) + 2.*cs[0]*cs[1]*cs[2])) / s2])

	return np.transpose(np.array([a, b, c]))


# Crystallographic B matrix converting rlu to 1/A
def get_B(lattice, angles):
	A = get_A(lattice, angles)
	B = 2.*np.pi * np.transpose(la.inv(A))
	return B


# A3 & A4 angles
def get_a3a4(ki, kf, Q_rlu, orient_rlu, B):
	metric = np.einsum("ij,ik -> jk", B, B)
	Qlen = np.sqrt(np.dot(Q_rlu, np.dot(metric, Q_rlu)))
	orientlen = np.sqrt(np.dot(orient_rlu, np.dot(metric, orient_rlu)))

	# Angle xi between Q and orientation reflex
	c = np.dot(Q_rlu, np.dot(metric, orient_rlu)) / (Qlen*orientlen)
	xi = np.arccos(c)

	# Angle psi enclosed by ki and Q
	psi = get_psi(ki, kf, Qlen)

	a3 = - psi - xi + a3_offs
	a4 = get_a4(ki, kf, Qlen)

	return [a3, a4]


# Get ki from kf and energy transfer
def get_ki(kf, E):
	return np.sqrt(kf**2. + E_to_k2*E)


# Get kf from ki and energy transfer
def get_kf(ki, E):
	return np.sqrt(ki**2. - E_to_k2*E)



# ------------------------------------------------------------------------------
if __name__ == "__main__":
	lattice = np.array([5, 5, 5])
	angles = np.array([90, 90, 60])
	Q_rlu = np.array([1,1,0])
	orient_rlu = np.array([1,0,0])
	dmono = 3.355
	dana = 3.355
	kf = 1.4
	E = 0.5
	ki = get_ki(kf, E)

	B = get_B(lattice, angles/180.*np.pi)

	[a1, a2] = get_a1a2(ki, dmono)
	[a5, a6] = get_a1a2(kf, dana)
	[a3, a4] = get_a3a4(ki, kf, Q_rlu, orient_rlu, B)

	print("a1 = %.4f, a2 = %.4f, a3 = %.4f, a4 = %.4f, a5 = %.4f, a6 = %.4f" \
		% (a1/np.pi*180., a2/np.pi*180., a3/np.pi*180., a4/np.pi*180., a5/np.pi*180., a6/np.pi*180.))
# ------------------------------------------------------------------------------

