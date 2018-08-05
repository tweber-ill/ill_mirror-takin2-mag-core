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
# rotate a vector around an axis using Rodrigues' formula
# see: https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
def rotate(_axis, vec, phi):
	axis = _axis / la.norm(_axis)

	s = np.sin(phi)
	c = np.cos(phi)

	return c*vec + (1.-c)*np.dot(vec, axis)*axis + s*np.cross(axis, vec)


# get metric from crystal B matrix
def get_metric(B):
	return np.einsum("ij,ik -> jk", B, B)


# cross product in fractional coordinates
def cross(a, b, B):
	# levi-civita in fractional coordinates
	def levi(i,j,k, B):
		M = np.array([B[:,i], B[:,j], B[:,k]])
		return la.det(M)

	metric_inv = la.inv(get_metric(B))
	eps = [[[ levi(i,j,k, B) for k in range(0,3) ] for j in range(0,3) ] for i in range(0,3) ]
	return np.einsum("ijk,j,k,li -> l", eps, a, b, metric_inv)


# dot product in fractional coordinates
def dot(a, b, metric):
	return np.dot(a, np.dot(metric, b))
# -----------------------------------------------------------------------------


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


# -----------------------------------------------------------------------------
# mono (or ana) k  ->  A1 & A2 angles (or A5 & A6)
def get_a1a2(k, d):
	s = np.pi/(d*k)
	a1 = np.arcsin(s)
	return [a1, 2.*a1]


# a1 angle (or a5)  ->  mono (or ana) k
def get_monok(theta, d):
	s = np.sin(theta)
	k = np.pi/(d*s)
	return k
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# scattering angle a4
def get_a4(ki, kf, Q):
	c = (ki**2. + kf**2. - Q**2.) / (2.*ki*kf)
	return np.arccos(c)


# get |Q| from ki, kf and a4
def get_Q(ki, kf, a4):
	c = np.cos(a4)
	return np.sqrt(ki**2. + kf**2. - c*(2.*ki*kf))
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# angle enclosed by ki and Q
def get_psi(ki, kf, Q):
	c = (ki**2. + Q**2. - kf**2.) / (2.*ki*Q)
	return np.arccos(c)


# crystallographic A matrix converting fractional to lab coordinates
# see: https://de.wikipedia.org/wiki/Fraktionelle_Koordinaten
def get_A(lattice, angles):
	cs = np.cos(angles)
	s2 = np.sin(angles[2])

	a = lattice[0] * np.array([1, 0, 0])
	b = lattice[1] * np.array([cs[2], s2, 0])
	c = lattice[2] * np.array([cs[1], \
		(cs[0]-cs[1]*cs[2]) / s2, \
		(np.sqrt(1. - np.dot(cs,cs) + 2.*cs[0]*cs[1]*cs[2])) / s2])

	return np.transpose(np.array([a, b, c]))


# crystallographic B matrix converting rlu to 1/A
def get_B(lattice, angles):
	A = get_A(lattice, angles)
	B = 2.*np.pi * np.transpose(la.inv(A))
	return B


# a3 & a4 angles
def get_a3a4(ki, kf, Q_rlu, orient_rlu, orient_up_rlu, B):
	metric = get_metric(B)
	Qlen = np.sqrt(dot(Q_rlu, Q_rlu, metric))
	orientlen = np.sqrt(dot(orient_rlu, orient_rlu, metric))

	# angle xi between Q and orientation reflex
	c = dot(Q_rlu, orient_rlu, metric) / (Qlen*orientlen)
	xi = np.arccos(c)

	# sign of xi
	if dot(cross(orient_rlu, Q_rlu, B), orient_up_rlu, metric) < 0.:
		xi = -xi

	# angle psi enclosed by ki and Q
	psi = get_psi(ki, kf, Qlen)

	a3 = - psi - xi + a3_offs
	a4 = get_a4(ki, kf, Qlen)

	#print("xi = " + str(xi/np.pi*180.) + ", psi = " + str(psi/np.pi*180.))
	return [a3, a4]


def get_hkl(ki, kf, a3, Qlen, orient_rlu, orient_up_rlu, B):
	B_inv = la.inv(B)

	# angle enclosed by ki and Q
	psi = get_psi(ki, kf, Qlen)

	# angle between Q and orientation reflex
	xi = - a3 + a3_offs - psi

	Q_lab = rotate(np.dot(B, orient_up_rlu), np.dot(B, orient_rlu*Qlen), xi)
	Q_lab *= Qlen / la.norm(Q_lab)
	Q_rlu = np.dot(B_inv, Q_lab)

	return Q_rlu
# -----------------------------------------------------------------------------


# -----------------------------------------------------------------------------
# get ki from kf and energy transfer
def get_ki(kf, E):
	return np.sqrt(kf**2. + E_to_k2*E)


# get kf from ki and energy transfer
def get_kf(ki, E):
	return np.sqrt(ki**2. - E_to_k2*E)


# get energy transfer from ki and kf
def get_E(ki, kf):
	return (ki**2. - kf**2.) / E_to_k2
# -----------------------------------------------------------------------------



# ------------------------------------------------------------------------------
# example calculations
# ------------------------------------------------------------------------------
if __name__ == "__main__":
	# --------------------------------------------------------------------------
	# lattice input
	# --------------------------------------------------------------------------
	lattice = np.array([5, 5, 5])
	angles = np.array([90, 90, 90])
	orient_rlu = np.array([1, 0, 0])
	orient2_rlu = np.array([0, 1, 0])
	# --------------------------------------------------------------------------

	# --------------------------------------------------------------------------
	# measurement position and instrument configuration input
	# --------------------------------------------------------------------------
	Q_rlu = np.array([1, -1, 0])
	E = 0.5
	kf = 1.4
	dmono = 3.355
	dana = 3.355
	# --------------------------------------------------------------------------

	# --------------------------------------------------------------------------
	# lattice and TAS angle calculation
	# --------------------------------------------------------------------------
	B = get_B(lattice, angles/180.*np.pi)
	orient_up_rlu = cross(orient_rlu, orient2_rlu, B)	# up vector in rlu

	ki = get_ki(kf, E)
	[a1, a2] = get_a1a2(ki, dmono)
	[a5, a6] = get_a1a2(kf, dana)
	[a3, a4] = get_a3a4(ki, kf, Q_rlu, orient_rlu, orient_up_rlu, B)
	# --------------------------------------------------------------------------

	# --------------------------------------------------------------------------
	# output
	# --------------------------------------------------------------------------
	np.set_printoptions(suppress=True, precision=4)

	print("B [rlu -> 1/A] = \n" + str(B))
	print("scattering plane normal = " + str(orient_up_rlu/la.norm(orient_up_rlu)) + " rlu")
	print("a1 = %.4f deg, a2 = %.4f deg, a3 = %.4f deg, a4 = %.4f deg, a5 = %.4f deg, a6 = %.4f deg" \
		% (a1/np.pi*180., a2/np.pi*180., a3/np.pi*180., a4/np.pi*180., a5/np.pi*180., a6/np.pi*180.))
	# --------------------------------------------------------------------------


	# --------------------------------------------------------------------------
	# CHECK: reproducing input values
	# --------------------------------------------------------------------------
	ki = get_monok(a1, dmono)
	kf = get_monok(a5, dana)
	E = get_E(ki, kf)
	Qlen = get_Q(ki, kf, a4)
	Qvec = get_hkl(ki, kf, a3, Qlen, orient_rlu, orient_up_rlu, B)
	# --------------------------------------------------------------------------

	# --------------------------------------------------------------------------
	# output
	# --------------------------------------------------------------------------
	print("ki = %.4f 1/A, kf = %.4f 1/A, E = %.4f meV, |Q| = %.4f 1/A, "\
		"Q = %s rlu" % (ki, kf, E, Qlen, Qvec))
	# --------------------------------------------------------------------------

# ------------------------------------------------------------------------------
