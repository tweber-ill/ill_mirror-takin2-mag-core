#
# tascalc gui
# @author Tobias Weber <tweber@ill.fr>
# @date 24-oct-18
# @license see 'LICENSE' file
#

import tascalc as tas

import numpy as np
import numpy.linalg as la

import PyQt5 as qt
import PyQt5.QtCore as qtc
import PyQt5.QtWidgets as qtw

import sys



app = qtw.QApplication(sys.argv)
app.setStyle("Fusion")

sett = qtc.QSettings("tobis_stuff", "qtas")
tabs = qtw.QTabWidget()



# -----------------------------------------------------------------------------
# variables
B = np.array([[1,0,0], [0,1,0], [0,0,1]])
orient_rlu = np.array([1,0,0])
orient2_rlu = np.array([0,1,0])
orient_up_rlu = np.array([0,0,1])
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# helpers
def getfloat(str):
	try:
		return float(str)
	except ValueError:
		return 0.
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# crystal tab

xtalpanel = qtw.QWidget()
xtallayout = qtw.QGridLayout(xtalpanel)

editA = qtw.QLineEdit(xtalpanel)
editB = qtw.QLineEdit(xtalpanel)
editC = qtw.QLineEdit(xtalpanel)
editAlpha = qtw.QLineEdit(xtalpanel)
editBeta = qtw.QLineEdit(xtalpanel)
editGamma = qtw.QLineEdit(xtalpanel)
editBMat = qtw.QPlainTextEdit(xtalpanel)
editBMat.setReadOnly(True)


def xtalChanged():
	global B
	lattice = np.array([getfloat(editA.text()), getfloat(editB.text()), getfloat(editC.text())])
	angles = np.array([getfloat(editAlpha.text()), getfloat(editBeta.text()), getfloat(editGamma.text())])
	try:
		B = tas.get_B(lattice, angles/180.*np.pi)
		invB = la.inv(B)
		editBMat.setPlainText("%10.4f %10.4f %10.4f\n%10.4f %10.4f %10.4f\n%10.4f %10.4f %10.4f\n" \
			% (B[0,0],B[0,1],B[0,2], B[1,0],B[1,1],B[1,2], B[2,0],B[2,1],B[2,2]) \
			+"\n%10.4f %10.4f %10.4f\n%10.4f %10.4f %10.4f\n%10.4f %10.4f %10.4f\n" \
			% (invB[0,0],invB[0,1],invB[0,2], invB[1,0],invB[1,1],invB[1,2], invB[2,0],invB[2,1],invB[2,2]))
	except la.LinAlgError:
		editBMat.setPlainText("invalid")

editA.textEdited.connect(xtalChanged)
editB.textEdited.connect(xtalChanged)
editC.textEdited.connect(xtalChanged)
editAlpha.textEdited.connect(xtalChanged)
editBeta.textEdited.connect(xtalChanged)
editGamma.textEdited.connect(xtalChanged)

editA.setText("%.6g" % sett.value("a", 5., type=float))
editB.setText("%.6g" % sett.value("b", 5., type=float))
editC.setText("%.6g" % sett.value("c", 5., type=float))
editAlpha.setText("%.6g" % sett.value("alpha", 90., type=float))
editBeta.setText("%.6g" % sett.value("beta", 90., type=float))
editGamma.setText("%.6g" % sett.value("gamma", 90., type=float))

xtallayout.addWidget(qtw.QLabel("a (A):", xtalpanel), 0,0, 1,1)
xtallayout.addWidget(editA, 0,1, 1,1)
xtallayout.addWidget(qtw.QLabel("b (A):", xtalpanel), 1,0, 1,1)
xtallayout.addWidget(editB, 1,1, 1,1)
xtallayout.addWidget(qtw.QLabel("c (A):", xtalpanel), 2,0, 1,1)
xtallayout.addWidget(editC, 2,1, 1,1)
xtallayout.addWidget(qtw.QLabel("alpha (A):", xtalpanel), 3,0, 1,1)
xtallayout.addWidget(editAlpha, 3,1, 1,1)
xtallayout.addWidget(qtw.QLabel("beta (A):", xtalpanel), 4,0, 1,1)
xtallayout.addWidget(editBeta, 4,1, 1,1)
xtallayout.addWidget(qtw.QLabel("gamma (A):", xtalpanel), 5,0, 1,1)
xtallayout.addWidget(editGamma, 5,1, 1,1)
xtallayout.addWidget(qtw.QLabel("B matrix and inverse:", xtalpanel), 6,0, 1,2)
xtallayout.addWidget(editBMat, 7,0, 2,2)

tabs.addTab(xtalpanel, "Crystal")
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# plane tab

scpanel = qtw.QWidget()
sclayout = qtw.QGridLayout(scpanel)

editAx = qtw.QLineEdit(scpanel)
editAy = qtw.QLineEdit(scpanel)
editAz = qtw.QLineEdit(scpanel)
editBx = qtw.QLineEdit(scpanel)
editBy = qtw.QLineEdit(scpanel)
editBz = qtw.QLineEdit(scpanel)


def planeChanged():
	QChanged()

editAx.textEdited.connect(planeChanged)
editAy.textEdited.connect(planeChanged)
editAz.textEdited.connect(planeChanged)
editBx.textEdited.connect(planeChanged)
editBy.textEdited.connect(planeChanged)
editBz.textEdited.connect(planeChanged)


editAx.setText("%.6g" % sett.value("ax", 1., type=float))
editAy.setText("%.6g" % sett.value("ay", 0., type=float))
editAz.setText("%.6g" % sett.value("az", 0., type=float))
editBx.setText("%.6g" % sett.value("bx", 0., type=float))
editBy.setText("%.6g" % sett.value("by", 1., type=float))
editBz.setText("%.6g" % sett.value("bz", 0., type=float))

sclayout.addWidget(qtw.QLabel("ax (rlu):", scpanel), 0,0, 1,1)
sclayout.addWidget(editAx, 0,1, 1,1)
sclayout.addWidget(qtw.QLabel("ay (rlu):", scpanel), 1,0, 1,1)
sclayout.addWidget(editAy, 1,1, 1,1)
sclayout.addWidget(qtw.QLabel("az (rlu):", scpanel), 2,0, 1,1)
sclayout.addWidget(editAz, 2,1, 1,1)
sclayout.addWidget(qtw.QLabel("bx (rlu):", scpanel), 3,0, 1,1)
sclayout.addWidget(editBx, 3,1, 1,1)
sclayout.addWidget(qtw.QLabel("by (rlu):", scpanel), 4,0, 1,1)
sclayout.addWidget(editBy, 4,1, 1,1)
sclayout.addWidget(qtw.QLabel("bz (rlu):", scpanel), 5,0, 1,1)
sclayout.addWidget(editBz, 5,1, 1,1)

tabs.addTab(scpanel, "Plane")
# -----------------------------------------------------------------------------




# -----------------------------------------------------------------------------
# tas tab

taspanel = qtw.QWidget()
taslayout = qtw.QGridLayout(taspanel)

editA1 = qtw.QLineEdit(taspanel)
editA2 = qtw.QLineEdit(taspanel)
editA3 = qtw.QLineEdit(taspanel)
editA4 = qtw.QLineEdit(taspanel)
editA5 = qtw.QLineEdit(taspanel)
editA6 = qtw.QLineEdit(taspanel)
editDm = qtw.QLineEdit(taspanel)
editDa = qtw.QLineEdit(taspanel)


def TASChanged():
	a1 = getfloat(editA1.text()) / 180. * np.pi
	a2 = a1 * 2.
	a3 = getfloat(editA3.text()) / 180. * np.pi
	a4 = getfloat(editA4.text()) / 180. * np.pi
	a5 = getfloat(editA5.text()) / 180. * np.pi
	a6 = a5 * 2.
	dmono = getfloat(editDm.text())
	dana = getfloat(editDa.text())

        ki = tas.get_monok(a1, dmono)
        kf = tas.get_monok(a5, dana)
        E = tas.get_E(ki, kf)
        Qlen = tas.get_Q(ki, kf, a4)
        Qvec = tas.get_hkl(ki, kf, a3, Qlen, orient_rlu, orient_up_rlu, B)

	editA2.setText("%.6g" % (a2 / np.pi * 180.))
	editA6.setText("%.6g" % (a6 / np.pi * 180.))

	edith.setText("%.6g" % Qvec[0])
	editk.setText("%.6g" % Qvec[1])
	editl.setText("%.6g" % Qvec[2])
	editKi.setText("%.6g" % ki)
	editKf.setText("%.6g" % kf)
	editE.setText("%.6g" % E)


def A2Changed():
	a2 = getfloat(editA2.text()) / 180. * np.pi
	editA1.setText("%.6g" % (0.5*a2 / np.pi * 180.))
	TASChanged()

def A6Changed():
	a6 = getfloat(editA6.text()) / 180. * np.pi
	editA5.setText("%.6g" % (0.5*a6 / np.pi * 180.))
	TASChanged()

def DChanged():
	QChanged()


editA1.textEdited.connect(TASChanged)
editA3.textEdited.connect(TASChanged)
editA4.textEdited.connect(TASChanged)
editA5.textEdited.connect(TASChanged)
editA2.textEdited.connect(A2Changed)
editA6.textEdited.connect(A6Changed)
editDm.textEdited.connect(DChanged)
editDa.textEdited.connect(DChanged)


editDm.setText("%.6g" % sett.value("dm", 3.355, type=float))
editDa.setText("%.6g" % sett.value("da", 3.355, type=float))

taslayout.addWidget(qtw.QLabel("a1 (deg):", taspanel), 0,0, 1,1)
taslayout.addWidget(editA1, 0,1, 1,1)
taslayout.addWidget(qtw.QLabel("a2 (deg):", taspanel), 1,0, 1,1)
taslayout.addWidget(editA2, 1,1, 1,1)
taslayout.addWidget(qtw.QLabel("a3 (deg):", taspanel), 2,0, 1,1)
taslayout.addWidget(editA3, 2,1, 1,1)
taslayout.addWidget(qtw.QLabel("a4 (deg):", taspanel), 3,0, 1,1)
taslayout.addWidget(editA4, 3,1, 1,1)
taslayout.addWidget(qtw.QLabel("a5 (deg):", taspanel), 4,0, 1,1)
taslayout.addWidget(editA5, 4,1, 1,1)
taslayout.addWidget(qtw.QLabel("a6 (deg):", taspanel), 5,0, 1,1)
taslayout.addWidget(editA6, 5,1, 1,1)
taslayout.addWidget(qtw.QLabel("Mono. d (A):", taspanel), 6,0, 1,1)
taslayout.addWidget(editDm, 6,1, 1,1)
taslayout.addWidget(qtw.QLabel("Ana. d (A):", taspanel), 7,0, 1,1)
taslayout.addWidget(editDa, 7,1, 1,1)

tabs.addTab(taspanel, "TAS")
# -----------------------------------------------------------------------------




# -----------------------------------------------------------------------------
# hkl tab

Qpanel = qtw.QWidget()
Qlayout = qtw.QGridLayout(Qpanel)

edith = qtw.QLineEdit(Qpanel)
editk = qtw.QLineEdit(Qpanel)
editl = qtw.QLineEdit(Qpanel)
editE = qtw.QLineEdit(Qpanel)

editKi = qtw.QLineEdit(Qpanel)
editKf = qtw.QLineEdit(Qpanel)


def QChanged():
	global orient_rlu, orient2_rlu, orient_up_rlu

	Q_rlu = np.array([getfloat(edith.text()), getfloat(editk.text()), getfloat(editl.text())])
	ki = getfloat(editKi.text())
	kf = getfloat(editKf.text())
	E = tas.get_E(ki, kf)

	orient_rlu = np.array([getfloat(editAx.text()), getfloat(editAy.text()), getfloat(editAz.text())])
	orient2_rlu = np.array([getfloat(editBx.text()), getfloat(editBy.text()), getfloat(editBz.text())])
	orient_up_rlu = tas.cross(orient_rlu, orient2_rlu, B)   # up vector in rlu

	[a1, a2] = tas.get_a1a2(ki, getfloat(editDm.text()))
	[a5, a6] = tas.get_a1a2(kf, getfloat(editDa.text()))
	[a3, a4] = tas.get_a3a4(ki, kf, Q_rlu, orient_rlu, orient_up_rlu, B)

	editE.setText("%.6g" % E)
	editA1.setText("%.6g" % (a1 / np.pi * 180.))
	editA2.setText("%.6g" % (a2 / np.pi * 180.))
	editA3.setText("%.6g" % (a3 / np.pi * 180.))
	editA4.setText("%.6g" % (a4 / np.pi * 180.))
	editA5.setText("%.6g" % (a5 / np.pi * 180.))
	editA6.setText("%.6g" % (a6 / np.pi * 180.))


def EChanged():
	E = getfloat(editE.text())
	kf = getfloat(editKf.text())
	ki = tas.get_ki(kf, E)
	editKi.setText("%.6g" % ki)

edith.textEdited.connect(QChanged)
editk.textEdited.connect(QChanged)
editl.textEdited.connect(QChanged)
editKi.textEdited.connect(QChanged)
editKf.textEdited.connect(QChanged)
editE.textEdited.connect(EChanged)


edith.setText("%.6g" % sett.value("h", 1., type=float))
editk.setText("%.6g" % sett.value("k", 0., type=float))
editl.setText("%.6g" % sett.value("l", 0., type=float))
#editE.setText("%.6g" % sett.value("E", 0., type=float))

editKi.setText("%.6g" % sett.value("ki", 2.662, type=float))
editKf.setText("%.6g" % sett.value("kf", 2.662, type=float))

Qlayout.addWidget(qtw.QLabel("h (rlu):", Qpanel), 0,0, 1,1)
Qlayout.addWidget(edith, 0,1, 1,1)
Qlayout.addWidget(qtw.QLabel("k (rlu):", Qpanel), 1,0, 1,1)
Qlayout.addWidget(editk, 1,1, 1,1)
Qlayout.addWidget(qtw.QLabel("l (rlu):", Qpanel), 2,0, 1,1)
Qlayout.addWidget(editl, 2,1, 1,1)
Qlayout.addWidget(qtw.QLabel("E (meV):", Qpanel), 3,0, 1,1)
Qlayout.addWidget(editE, 3,1, 1,1)
Qlayout.addWidget(qtw.QLabel("ki (1/A):", Qpanel), 4,0, 1,1)
Qlayout.addWidget(editKi, 4,1, 1,1)
Qlayout.addWidget(qtw.QLabel("kf (1/A):", Qpanel), 5,0, 1,1)
Qlayout.addWidget(editKf, 5,1, 1,1)

tabs.addTab(Qpanel, "hkl")
# -----------------------------------------------------------------------------




# -----------------------------------------------------------------------------
# main dialog window

dlg = qtw.QDialog()
dlg.setWindowTitle("TAS Calculator")
mainlayout = qtw.QGridLayout(dlg)
mainlayout.addWidget(tabs)

dlg.show()
xtalChanged()
QChanged()

app.exec_()


# save settings
sett.setValue("a", getfloat(editA.text()))
sett.setValue("b", getfloat(editB.text()))
sett.setValue("c", getfloat(editC.text()))
sett.setValue("alpha", getfloat(editAlpha.text()))
sett.setValue("beta", getfloat(editBeta.text()))
sett.setValue("gamma", getfloat(editGamma.text()))
sett.setValue("ax", getfloat(editAx.text()))
sett.setValue("ay", getfloat(editAy.text()))
sett.setValue("az", getfloat(editAz.text()))
sett.setValue("bx", getfloat(editBx.text()))
sett.setValue("by", getfloat(editBy.text()))
sett.setValue("bz", getfloat(editBz.text()))
sett.setValue("dm", getfloat(editDm.text()))
sett.setValue("da", getfloat(editDa.text()))
sett.setValue("h", getfloat(edith.text()))
sett.setValue("k", getfloat(editk.text()))
sett.setValue("l", getfloat(editl.text()))
#sett.setValue("E", getfloat(editE.text()))
sett.setValue("ki", getfloat(editKi.text()))
sett.setValue("kf", getfloat(editKf.text()))
# -----------------------------------------------------------------------------
