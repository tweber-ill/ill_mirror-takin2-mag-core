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
use_matplotlib = True
verbose = True
show_neutrons = True
ellipse_points = 128



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
	if verbose:
		print("Mean (Q, E) vector in lab system:\n%s\n" % Qmean)

	Qcov = np.cov(Q4, rowvar = False, aweights = w, ddof=0)
	if verbose:
		print("Covariance matrix in lab system:\n%s\n" % Qcov)

	Qres = la.inv(Qcov)
	if verbose:
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

	if verbose:
		print("Transformation into (Qpara, Qperp, Qup, E) system:\n%s\n" % T)

	Qmean_Q = np.dot(np.transpose(T), Qmean)
	if verbose:
		print("Mean (Q, E) vector in (Qpara, Qperp, Qup, E) system:\n%s\n" % Qmean_Q)

	Qcov_Q = np.dot(np.transpose(T), np.dot(Qcov, T))
	if verbose:
		print("Covariance matrix in (Qpara, Qperp, Qup, E) system:\n%s\n" % Qcov_Q)

	Qres_Q = la.inv(Qcov_Q)
	if verbose:
		print("Resolution matrix in (Qpara, Qperp, Qup, E) system:\n%s\n" % Qres_Q)

	#[ evals, evecs ] = la.eig(Qcov_Q)
	#print("Ellipsoid fwhm radii:\n%s\n" % (np.sqrt(evals) * sig2fwhm))


	# transform events
	Q4_Q = np.array([])
	if show_neutrons:
		Q4_Q = np.dot(Q4, T) - Qmean_Q


	return [Qres_Q, Q4_Q]



#
# calculates the characteristics of a given ellipse
#
def descr_ellipse(quadric):
	[ evals, evecs ] = la.eig(quadric)
	fwhms = 1./np.sqrt(evals) * sig2fwhm

	angles = np.array([])
	if len(quadric) == 2:
		angles = np.array([ np.arctan2(evecs[1][0], evecs[0][0]) ])

	return [fwhms, angles/np.pi*180., evecs]



#
# describes the ellipsoid by a principal axis trafo
#
def calc_ellipses(Qres_Q):
	if verbose:
		print("4d resolution ellipsoid diagonal elements fwhm (coherent-elastic scattering) lengths:\n%s\n" \
			% (1./np.sqrt(np.diag(Qres_Q)) * sig2fwhm))

	# 4d ellipsoid
	[fwhms, angles, rot] = descr_ellipse(Qres_Q)
	if verbose:
		print("4d resolution ellipsoid principal axes fwhm lengths:\n%s\n" % fwhms)


	# 2d sliced ellipses
	Qres_QxE = np.delete(np.delete(Qres_Q, 2, axis=0), 2, axis=1)
	Qres_QxE = np.delete(np.delete(Qres_QxE, 1, axis=0), 1, axis=1)
	[fwhms_QxE, angles_QxE, rot_QxE] = descr_ellipse(Qres_QxE)
	if verbose:
		print("2d Qx/E ellipse fwhm lengths and slope angle:\n%s, %f\n" % (fwhms_QxE, angles_QxE[0]))

	Qres_QyE = np.delete(np.delete(Qres_Q, 2, axis=0), 2, axis=1)
	Qres_QyE = np.delete(np.delete(Qres_QyE, 0, axis=0), 0, axis=1)
	[fwhms_QyE, angles_QyE, rot_QyE] = descr_ellipse(Qres_QyE)
	if verbose:
		print("2d Qy/E ellipse fwhm lengths and slope angle:\n%s, %f\n" % (fwhms_QyE, angles_QyE[0]))

	Qres_QzE = np.delete(np.delete(Qres_Q, 1, axis=0), 1, axis=1)
	Qres_QzE = np.delete(np.delete(Qres_QzE, 0, axis=0), 0, axis=1)
	[fwhms_QzE, angles_QzE, rot_QzE] = descr_ellipse(Qres_QzE)
	if verbose:
		print("2d Qz/E ellipse fwhm lengths and slope angle:\n%s, %f\n" % (fwhms_QzE, angles_QzE[0]))

	Qres_QxQy = np.delete(np.delete(Qres_Q, 3, axis=0), 3, axis=1)
	Qres_QxQy = np.delete(np.delete(Qres_QxQy, 2, axis=0), 2, axis=1)
	[fwhms_QxQy, angles_QxQy, rot_QxQy] = descr_ellipse(Qres_QxQy)
	if verbose:
		print("2d Qx/Qy ellipse fwhm lengths and slope angle:\n%s, %f\n" % (fwhms_QxQy, angles_QxQy[0]))

	return [fwhms_QxE, rot_QxE, fwhms_QyE, rot_QyE, fwhms_QzE, rot_QzE, fwhms_QxQy, rot_QxQy]



#
# shows the 2d ellipses
#
def plot_ellipses(Q4, fwhms_QxE, rot_QxE, fwhms_QyE, rot_QyE, fwhms_QzE, rot_QzE, fwhms_QxQy, rot_QxQy):
	import matplotlib.pyplot as plot

	ellfkt = lambda rad, rot, phi : np.dot(rot, np.array([ rad[0]*np.cos(phi), rad[1]*np.sin(phi) ]))

	phi = np.linspace(0, 2.*np.pi, ellipse_points)
	ell_QxE = ellfkt(fwhms_QxE, rot_QxE, phi)
	ell_QyE = ellfkt(fwhms_QyE, rot_QyE, phi)
	ell_QzE = ellfkt(fwhms_QzE, rot_QzE, phi)
	ell_QxQy = ellfkt(fwhms_QxQy, rot_QxQy, phi)

	fig = plot.figure()
	subplot_QxE = fig.add_subplot(221)
	subplot_QxE.set_xlabel("Qpara (1/A)")
	subplot_QxE.set_ylabel("E (meV")
	if len(Q4.shape)==2 and len(Q4)>0 and len(Q4[0])==4:
		subplot_QxE.scatter(Q4[:, 0], Q4[:, 3], s=0.25)
	subplot_QxE.plot(ell_QxE[0], ell_QxE[1], c="black")

	subplot_QyE = fig.add_subplot(222)
	subplot_QyE.set_xlabel("Qperp (1/A)")
	subplot_QyE.set_ylabel("E (meV)")
	if len(Q4.shape)==2 and len(Q4)>0 and len(Q4[0])==4:
		subplot_QyE.scatter(Q4[:, 1], Q4[:, 3], s=0.25)
	subplot_QyE.plot(ell_QyE[0], ell_QyE[1], c="black")

	subplot_QzE = fig.add_subplot(223)
	subplot_QzE.set_xlabel("Qup (1/A)")
	subplot_QzE.set_ylabel("E (meV)")
	if len(Q4.shape)==2 and len(Q4)>0 and len(Q4[0])==4:
		subplot_QzE.scatter(Q4[:, 2], Q4[:, 3], s=0.25)
	subplot_QzE.plot(ell_QzE[0], ell_QzE[1], c="black")

	subplot_QxQy = fig.add_subplot(224)
	subplot_QxQy.set_xlabel("Qpara (1/A)")
	subplot_QxQy.set_ylabel("Qperp (meV)")
	if len(Q4.shape)==2 and len(Q4)>0 and len(Q4[0])==4:
		subplot_QxQy.scatter(Q4[:, 0], Q4[:, 1], s=0.25)
	subplot_QxQy.plot(ell_QxQy[0], ell_QxQy[1], c="black")
	plot.tight_layout()
	plot.show()



#
# main
#
if __name__ == "__main__":
	if len(sys.argv) <= 1:
		print("Please specify a filename.")
		exit(-1)

	[Q, E, w] = load_events(sys.argv[1])

	[Qres, Q4] = calc_covar(Q, E, w)
	[fwhms_QxE, rot_QxE, fwhms_QyE, rot_QyE, fwhms_QzE, rot_QzE, fwhms_QxQy, rot_QxQy] = calc_ellipses(Qres)

	if use_matplotlib:
		plot_ellipses(Q4, fwhms_QxE, rot_QxE, fwhms_QyE, rot_QyE, fwhms_QzE, rot_QzE, fwhms_QxQy, rot_QxQy)
