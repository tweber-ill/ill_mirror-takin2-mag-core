#!/usr/bin/env python
#
# calculate covariance from neutron events
# @author Tobias Weber <tweber@ill.fr>
# @date 30-mar-2019
# @license see 'LICENSE' file
#

import numpy as np
import numpy.linalg as la


verbose = True		# console outputs
plot_results = True	# show plot window
plot_neutrons = True	# also plot neutron events
centre_on_Q = False	# centre plots on Q or zero?
ellipse_points = 128	# number of points to draw ellipses


# column indices in mc file
ki_start_idx = 0	# start index of ki 3-vector
kf_start_idx = 3	# start index of kf 3-vector
wi_idx = 9		# start index of ki weight factor
wf_idx = 10		# start index of kf weight factor



# constants
sig2hwhm = np.sqrt(2. * np.log(2.))
sig2fwhm = 2.*sig2hwhm

#import scipy.constants as co
#hbar_in_meVs = co.Planck/co.elementary_charge*1000./2./np.pi
#E_to_k2 = 2.*co.neutron_mass/hbar_in_meVs**2. / co.elementary_charge*1000. * 1e-20
E_to_k2 = 0.482596406464	# E -> k^2, calculated with scipy, using the code above
k2_to_E = 1./E_to_k2		# k^2 -> E



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


	# transform all neutron events
	Q4_Q = np.array([])
	if plot_neutrons:
		Q4_Q = np.dot(Q4, T)
		if not centre_on_Q:
			Q4_Q -= Qmean_Q


	return [Qres_Q, Q4_Q, Qmean_Q]



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
# describes the ellipsoid by a principal axis trafo and by 2d cuts
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
def plot_ellipses(file, Q4, Qmean, fwhms_QxE, rot_QxE, fwhms_QyE, rot_QyE, fwhms_QzE, rot_QzE, fwhms_QxQy, rot_QxQy):
	import matplotlib.pyplot as plot

	ellfkt = lambda rad, rot, phi, Qmean2d : \
		np.dot(rot, np.array([ rad[0]*np.cos(phi), rad[1]*np.sin(phi) ])) + Qmean2d


	# centre plots on zero or average Q vector ?
	QxE = np.array([[0], [0]])
	QyE = np.array([[0], [0]])
	QzE = np.array([[0], [0]])
	QxQy = np.array([[0], [0]])

	if centre_on_Q:
		QxE = np.array([[Qmean[0]], [Qmean[3]]])
		QyE = np.array([[Qmean[1]], [Qmean[3]]])
		QzE = np.array([[Qmean[2]], [Qmean[3]]])
		QxQy = np.array([[Qmean[0]], [Qmean[1]]])


	phi = np.linspace(0, 2.*np.pi, ellipse_points)
	ell_QxE = ellfkt(fwhms_QxE, rot_QxE, phi, QxE)
	ell_QyE = ellfkt(fwhms_QyE, rot_QyE, phi, QyE)
	ell_QzE = ellfkt(fwhms_QzE, rot_QzE, phi, QzE)
	ell_QxQy = ellfkt(fwhms_QxQy, rot_QxQy, phi, QxQy)

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

	if file != "":
		if verbose:
			print("Saving plot to \"%s\"." % file)
		plot.savefig(file)

	if plot_results:
		plot.show()



#
# checks versions of needed packages
#
def check_versions():
	npver = np.version.version.split(".")
	if int(npver[0]) >= 2:
		return
	if int(npver[0]) < 1 or int(npver[1]) < 10:
		print("Numpy version >= 1.10 is required, but installed version is %s." % np.version.version)
		exit(-1)



#
# main
#
if __name__ == "__main__":
	check_versions()

	import argparse as arg

	args = arg.ArgumentParser(description="Calculates the covariance matrix of neutron scattering events.")
	args.add_argument("file", type=str, help="input file")
	args.add_argument("-s", "--save", default="", type=str, nargs="?", help="save plot to file")
	args.add_argument("--ellipse", default=ellipse_points, type=int, nargs="?", help="number of points to draw ellipses")
	args.add_argument("--ki", default=ki_start_idx, type=int, nargs="?", help="index of ki vector's first column in file")
	args.add_argument("--kf", default=kf_start_idx, type=int, nargs="?", help="index of kf vector's first column in file")
	args.add_argument("--wi", default=wi_idx, type=int, nargs="?", help="index of ki weight factor column in file")
	args.add_argument("--wf", default=wf_idx, type=int, nargs="?", help="index of kf weight factor column in file")
	args.add_argument("--centreonQ", action="store_true", help="centre plots on mean Q")
	args.add_argument("--noverbose", action="store_true", help="don't show console logs")
	args.add_argument("--noplot", action="store_true", help="don't show plot window")
	args.add_argument("--noneutrons", action="store_true", help="don't show mc neutrons in plot")
	argv = args.parse_args()

	verbose = (argv.noverbose==False)
	plot_results = (argv.noplot==False)
	plot_neutrons = (argv.noneutrons==False)
	centre_on_Q = argv.centreonQ

	infile = argv.file
	outfile = argv.save
	ellipse_points = argv.ellipse
	ki_start_idx = argv.ki
	kf_start_idx = argv.kf
	wi_idx = argv.wi
	wf_idx = argv.wf

	print("")
	[Q, E, w] = load_events(infile)

	[Qres, Q4, Qmean] = calc_covar(Q, E, w)
	[fwhms_QxE, rot_QxE, fwhms_QyE, rot_QyE, fwhms_QzE, rot_QzE, fwhms_QxQy, rot_QxQy] = calc_ellipses(Qres)

	if plot_results or outfile!="":
		plot_ellipses(outfile, Q4, Qmean, fwhms_QxE, rot_QxE, fwhms_QyE, rot_QyE, fwhms_QzE, rot_QzE, fwhms_QxQy, rot_QxQy)
