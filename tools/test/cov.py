#
# calculate covariance from neutron events
# @author Tobias Weber <tweber@ill.fr>
# @date 30-mar-2019
# @license see 'LICENSE' file
#

import sys
import numpy as np
import numpy.linalg as la

use_scipy = False


# column indices in mc file
ki_start_idx = 0
kf_start_idx = 3
wi_idx = 9
wf_idx = 10


# constants
sig2hwhm = np.sqrt(2. * np.log(2.))
sig2fwhm = 2.*sig2hwhm

if use_scipy:
	import scipy as sp
	import scipy.constants as co

	hbar_in_meVs = co.Planck/co.elementary_charge*1000./2./np.pi
	E_to_k2 = 2.*co.neutron_mass/hbar_in_meVs**2. / co.elementary_charge*1000. * 1e-20
else:
	E_to_k2 = 0.482596406464	# calculated with scipy, using the formula above
k2_to_E = 1./E_to_k2



#
# loads a list of neutron events in the [ ki_vec, kf_vec, pos_vec, wi, wf ] format
#
def load_events(filename):
	dat = np.loadtxt(filename)
	ki = dat[:, ki_start_idx:ki_start_idx+3]
	kf = dat[:, kf_start_idx:kf_start_idx+3]
	wi = dat[:, wi_idx]
	wf = dat[:, wf_idx]

	w = wi * wf
	Q = ki - kf
	E = k2_to_E * (np.multiply(ki, ki).sum(1) - np.multiply(kf, kf).sum(1))

	return [Q, E, w]



#
# calculates the covariance matrix of the (Q, E) 4-vectors
#
def calc_covar(Q, E, w):
	# make a [Q, E] 4-vector
	Q4 = np.insert(Q, 3, E, axis=1)

	Qmean = [ np.average(Q4[:,i], weights = w) for i in range(4) ]
	print("Mean (Q, E) vector in lab system:\n%s\n" % Qmean)

	Qcov = np.cov(Q4, rowvar = False, aweights = w, ddof=0)
	print("Covariance matrix in lab system:\n%s\n" % Qcov)

	Qres = la.inv(Qcov)
	print("Resolution matrix in lab system:\n%s\n" % Qres)


	# create a matrix to transform into the coordinate system with Q along x
	Qnorm = Qmean[0:3] / la.norm(Qmean[0:3])
	Qup = np.array([0, 1, 0])
	Qside = np.cross(Qup, Qnorm)

	T = np.transpose(np.array([
		np.insert(Qnorm, 3, 0),
		np.insert(Qside, 3, 0),
		np.insert(Qup, 3, 0),
		[0, 0, 0, 1] ]))

	print("Transformation into (Qpara, Qperp, Qup, E) system:\n%s\n" % T)

	Qmean_Q = np.dot(np.transpose(T), Qmean)
	print("Mean (Q, E) vector in (Qpara, Qperp, Qup, E) system:\n%s\n" % Qmean_Q)

	Qcov_Q = np.dot(np.transpose(T), np.dot(Qcov, T))
	print("Covariance matrix in (Qpara, Qperp, Qup, E) system:\n%s\n" % Qcov_Q)

	Qres_Q = la.inv(Qcov_Q)
	print("Resolution matrix in (Qpara, Qperp, Qup, E) system:\n%s\n" % Qres_Q)


	#[ evals, evecs ] = la.eig(Qcov_Q)
	#print("Ellipsoid fwhm radii:\n%s\n" % (np.sqrt(evals) * sig2fwhm))

	return Qres_Q



#
# calculate the characteristics of a given ellipse
#
def descr_ellipse(quadric):
	[ evals, evecs ] = la.eig(quadric)
	fwhms = 1./np.sqrt(evals) * sig2fwhm

	angles = np.array([])
	if len(quadric) == 2:
		angles = np.array([ np.arctan2(evecs[1][0], evecs[0][0]) ])

	return [fwhms, angles/np.pi*180.]



#
# describe the ellipsoid by a principal axis trafo
#
def calc_ellipses(Qres_Q):
	[fwhms, angles] = descr_ellipse(Qres_Q)
	print("4d resolution ellipsoid fwhm lengths:\n%s\n" % fwhms)


	# 2d sliced ellipses
	Qres_QxE = np.delete(np.delete(Qres_Q, 2, axis=0), 2, axis=1)
	Qres_QxE = np.delete(np.delete(Qres_QxE, 1, axis=0), 1, axis=1)
	[fwhms_QxE, angles_QxE] = descr_ellipse(Qres_QxE)
	print("Qx/E ellipse fwhm lengths and slope angle:\n%s, %f\n" % (fwhms_QxE, angles_QxE[0]))

	Qres_QyE = np.delete(np.delete(Qres_Q, 2, axis=0), 2, axis=1)
	Qres_QyE = np.delete(np.delete(Qres_QyE, 0, axis=0), 0, axis=1)
	[fwhms_QyE, angles_QyE] = descr_ellipse(Qres_QyE)
	print("Qy/E ellipse fwhm lengths and slope angle:\n%s, %f\n" % (fwhms_QyE, angles_QyE[0]))

	Qres_QzE = np.delete(np.delete(Qres_Q, 1, axis=0), 1, axis=1)
	Qres_QzE = np.delete(np.delete(Qres_QzE, 0, axis=0), 0, axis=1)
	[fwhms_QzE, angles_QzE] = descr_ellipse(Qres_QzE)
	print("Qz/E ellipse fwhm lengths and slope angle:\n%s, %f\n" % (fwhms_QzE, angles_QzE[0]))



#
# main
#
if __name__ == "__main__":
	if len(sys.argv) <= 1:
		print("Please specify a filename.")
		exit(-1)

	[Q, E, w] = load_events(sys.argv[1])
	Qres = calc_covar(Q, E, w)
	calc_ellipses(Qres)
