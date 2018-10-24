#
# calculates TAS angles from rlu (part of in20tools)
# @author Tobias Weber <tweber@ill.fr>
# @date 1-aug-18
# @license see 'LICENSE' file
#

import numpy as np
import numpy.linalg as la


use_scipy = False

# -----------------------------------------------------------------------------
# choose an a3 convention
#a3_offs = 0.			# for sics
#a3_offs = np.pi/2.		# for takin: Q along orient1 => a3:=a4/2
a3_offs = np.pi 		# for nomad: ki along orient1 => a3:=0

def set_a3_offs(offs):
	global a3_offs
	a3_offs = offs
# -----------------------------------------------------------------------------


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


# angle between peaks in fractional coordinates
def angle(a, b, metric):
	len_a = np.sqrt(dot(a, a, metric))
	len_b = np.sqrt(dot(b, b, metric))

	c = dot(a, b, metric) / (len_a * len_b)
	return np.arccos(c)
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

	# angle xi between Q and orientation reflex
	xi = angle(Q_rlu, orient_rlu, metric)

	# sign of xi
	if dot(cross(orient_rlu, Q_rlu, B), orient_up_rlu, metric) < 0.:
		xi = -xi

	Qlen = np.sqrt(dot(Q_rlu, Q_rlu, metric))

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