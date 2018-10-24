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
app.setApplicationName("qtas")
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
scpanel = xtalpanel
sclayout = xtallayout

editA = qtw.QLineEdit(xtalpanel)
editB = qtw.QLineEdit(xtalpanel)
editC = qtw.QLineEdit(xtalpanel)
editAlpha = qtw.QLineEdit(xtalpanel)
editBeta = qtw.QLineEdit(xtalpanel)
editGamma = qtw.QLineEdit(xtalpanel)
editAx = qtw.QLineEdit(scpanel)
editAy = qtw.QLineEdit(scpanel)
editAz = qtw.QLineEdit(scpanel)
editBx = qtw.QLineEdit(scpanel)
editBy = qtw.QLineEdit(scpanel)
editBz = qtw.QLineEdit(scpanel)
editBMat = qtw.QPlainTextEdit(xtalpanel)
editBMat.setReadOnly(True)


def xtalChanged():
	global B
	lattice = np.array([getfloat(editA.text()), getfloat(editB.text()), getfloat(editC.text())])
	angles = np.array([getfloat(editAlpha.text()), getfloat(editBeta.text()), getfloat(editGamma.text())])
	try:
		B = tas.get_B(lattice, angles/180.*np.pi)
		invB = la.inv(B)
		editBMat.setPlainText("B =\n%10.4f %10.4f %10.4f\n%10.4f %10.4f %10.4f\n%10.4f %10.4f %10.4f\n" \
			% (B[0,0],B[0,1],B[0,2], B[1,0],B[1,1],B[1,2], B[2,0],B[2,1],B[2,2]) \
			+"\nB^(-1) =\n%10.4f %10.4f %10.4f\n%10.4f %10.4f %10.4f\n%10.4f %10.4f %10.4f\n" \
			% (invB[0,0],invB[0,1],invB[0,2], invB[1,0],invB[1,1],invB[1,2], invB[2,0],invB[2,1],invB[2,2]))
	except (ArithmeticError, la.LinAlgError) as err:
		editBMat.setPlainText("invalid")
	QChanged()

def planeChanged():
	QChanged()


editA.textEdited.connect(xtalChanged)
editB.textEdited.connect(xtalChanged)
editC.textEdited.connect(xtalChanged)
editAlpha.textEdited.connect(xtalChanged)
editBeta.textEdited.connect(xtalChanged)
editGamma.textEdited.connect(xtalChanged)
editAx.textEdited.connect(planeChanged)
editAy.textEdited.connect(planeChanged)
editAz.textEdited.connect(planeChanged)
editBx.textEdited.connect(planeChanged)
editBy.textEdited.connect(planeChanged)
editBz.textEdited.connect(planeChanged)


editA.setText("%.6g" % sett.value("a", 5., type=float))
editB.setText("%.6g" % sett.value("b", 5., type=float))
editC.setText("%.6g" % sett.value("c", 5., type=float))
editAlpha.setText("%.6g" % sett.value("alpha", 90., type=float))
editBeta.setText("%.6g" % sett.value("beta", 90., type=float))
editGamma.setText("%.6g" % sett.value("gamma", 90., type=float))
editAx.setText("%.6g" % sett.value("ax", 1., type=float))
editAy.setText("%.6g" % sett.value("ay", 0., type=float))
editAz.setText("%.6g" % sett.value("az", 0., type=float))
editBx.setText("%.6g" % sett.value("bx", 0., type=float))
editBy.setText("%.6g" % sett.value("by", 1., type=float))
editBz.setText("%.6g" % sett.value("bz", 0., type=float))

xtallayout.addWidget(qtw.QLabel(u"a (\u212b):", xtalpanel), 0,0, 1,1)
xtallayout.addWidget(editA, 0,1, 1,3)
xtallayout.addWidget(qtw.QLabel(u"b (\u212b):", xtalpanel), 1,0, 1,1)
xtallayout.addWidget(editB, 1,1, 1,3)
xtallayout.addWidget(qtw.QLabel(u"c (\u212b):", xtalpanel), 2,0, 1,1)
xtallayout.addWidget(editC, 2,1, 1,3)
xtallayout.addWidget(qtw.QLabel(u"\u03b1 (deg):", xtalpanel), 3,0, 1,1)
xtallayout.addWidget(editAlpha, 3,1, 1,3)
xtallayout.addWidget(qtw.QLabel(u"\u03b2 (deg):", xtalpanel), 4,0, 1,1)
xtallayout.addWidget(editBeta, 4,1, 1,3)
xtallayout.addWidget(qtw.QLabel(u"\u03b3 (deg):", xtalpanel), 5,0, 1,1)
xtallayout.addWidget(editGamma, 5,1, 1,3)
sclayout.addWidget(qtw.QLabel("orient 1 (rlu):", scpanel), 6,0, 1,1)
sclayout.addWidget(editAx, 6,1, 1,1)
sclayout.addWidget(editAy, 6,2, 1,1)
sclayout.addWidget(editAz, 6,3, 1,1)
sclayout.addWidget(qtw.QLabel("orient 2 (rlu):", scpanel), 7,0, 1,1)
sclayout.addWidget(editBx, 7,1, 1,1)
sclayout.addWidget(editBy, 7,2, 1,1)
sclayout.addWidget(editBz, 7,3, 1,1)
xtallayout.addWidget(editBMat, 8,0, 2,4)

tabs.addTab(xtalpanel, "Crystal")
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# tas tab

taspanel = qtw.QWidget()
taslayout = qtw.QGridLayout(taspanel)
Qpanel = taspanel
Qlayout = taslayout

editA1 = qtw.QLineEdit(taspanel)
editA2 = qtw.QLineEdit(taspanel)
editA3 = qtw.QLineEdit(taspanel)
editA4 = qtw.QLineEdit(taspanel)
editA5 = qtw.QLineEdit(taspanel)
editA6 = qtw.QLineEdit(taspanel)

editDm = qtw.QLineEdit(taspanel)
editDa = qtw.QLineEdit(taspanel)

edith = qtw.QLineEdit(Qpanel)
editk = qtw.QLineEdit(Qpanel)
editl = qtw.QLineEdit(Qpanel)
editE = qtw.QLineEdit(Qpanel)
editKi = qtw.QLineEdit(Qpanel)
editKf = qtw.QLineEdit(Qpanel)
editQAbs = qtw.QLineEdit(Qpanel)
editQAbs.setReadOnly(True)

separatorTas = qtw.QFrame(Qpanel)
separatorTas.setFrameStyle(qtw.QFrame.HLine)


def TASChanged():
	a1 = getfloat(editA1.text()) / 180. * np.pi
	a2 = a1 * 2.
	a3 = getfloat(editA3.text()) / 180. * np.pi
	a4 = getfloat(editA4.text()) / 180. * np.pi
	a5 = getfloat(editA5.text()) / 180. * np.pi
	a6 = a5 * 2.
	dmono = getfloat(editDm.text())
	dana = getfloat(editDa.text())

	editA2.setText("%.6g" % (a2 / np.pi * 180.))
	editA6.setText("%.6g" % (a6 / np.pi * 180.))

	try:
		ki = tas.get_monok(a1, dmono)
		kf = tas.get_monok(a5, dana)
		E = tas.get_E(ki, kf)
		Qlen = tas.get_Q(ki, kf, a4)
		Qvec = tas.get_hkl(ki, kf, a3, Qlen, orient_rlu, orient_up_rlu, B)

		edith.setText("%.6g" % Qvec[0])
		editk.setText("%.6g" % Qvec[1])
		editl.setText("%.6g" % Qvec[2])
		editQAbs.setText("%.6g" % Qlen)
		editKi.setText("%.6g" % ki)
		editKf.setText("%.6g" % kf)
		editE.setText("%.6g" % E)
	except (ArithmeticError, la.LinAlgError) as err:
		edith.setText("invalid")
		editk.setText("invalid")
		editl.setText("invalid")
		editKi.setText("invalid")
		editKf.setText("invalid")
		editE.setText("invalid")


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

def QChanged():
	global orient_rlu, orient2_rlu, orient_up_rlu

	Q_rlu = np.array([getfloat(edith.text()), getfloat(editk.text()), getfloat(editl.text())])
	ki = getfloat(editKi.text())
	kf = getfloat(editKf.text())
	orient_rlu = np.array([getfloat(editAx.text()), getfloat(editAy.text()), getfloat(editAz.text())])
	orient2_rlu = np.array([getfloat(editBx.text()), getfloat(editBy.text()), getfloat(editBz.text())])

	try:
		[a1, a2] = tas.get_a1a2(ki, getfloat(editDm.text()))

		editA1.setText("%.6g" % (a1 / np.pi * 180.))
		editA2.setText("%.6g" % (a2 / np.pi * 180.))
	except (ArithmeticError, la.LinAlgError) as err:
		editA1.setText("invalid")
		editA2.setText("invalid")

	try:
		[a5, a6] = tas.get_a1a2(kf, getfloat(editDa.text()))

		editA5.setText("%.6g" % (a5 / np.pi * 180.))
		editA6.setText("%.6g" % (a6 / np.pi * 180.))
	except (ArithmeticError, la.LinAlgError) as err:
		editA5.setText("invalid")
		editA6.setText("invalid")

	try:
		orient_up_rlu = tas.cross(orient_rlu, orient2_rlu, B)   # up vector in rlu
		[a3, a4] = tas.get_a3a4(ki, kf, Q_rlu, orient_rlu, orient_up_rlu, B)
		Qlen = tas.get_Q(ki, kf, a4)

		editA3.setText("%.6g" % (a3 / np.pi * 180.))
		editA4.setText("%.6g" % (a4 / np.pi * 180.))
		editQAbs.setText("%.6g" % Qlen)
	except (ArithmeticError, la.LinAlgError) as err:
		editA3.setText("invalid")
		editA4.setText("invalid")

def KiKfChanged():
	ki = getfloat(editKi.text())
	kf = getfloat(editKf.text())

	try:
		E = tas.get_E(ki, kf)
		editE.setText("%.6g" % E)

		QChanged()
	except (ArithmeticError, la.LinAlgError) as err:
		editE.setText("invalid")

def EChanged():
	E = getfloat(editE.text())
	kf = getfloat(editKf.text())

	try:
		ki = tas.get_ki(kf, E)
		editKi.setText("%.6g" % ki)

		QChanged()
	except (ArithmeticError, la.LinAlgError) as err:
		editKi.setText("invalid")


editA1.textEdited.connect(TASChanged)
editA3.textEdited.connect(TASChanged)
editA4.textEdited.connect(TASChanged)
editA5.textEdited.connect(TASChanged)
editA2.textEdited.connect(A2Changed)
editA6.textEdited.connect(A6Changed)
editDm.textEdited.connect(DChanged)
editDa.textEdited.connect(DChanged)
edith.textEdited.connect(QChanged)
editk.textEdited.connect(QChanged)
editl.textEdited.connect(QChanged)
editKi.textEdited.connect(KiKfChanged)
editKf.textEdited.connect(KiKfChanged)
editE.textEdited.connect(EChanged)


editDm.setText("%.6g" % sett.value("dm", 3.355, type=float))
editDa.setText("%.6g" % sett.value("da", 3.355, type=float))
edith.setText("%.6g" % sett.value("h", 1., type=float))
editk.setText("%.6g" % sett.value("k", 0., type=float))
editl.setText("%.6g" % sett.value("l", 0., type=float))
#editE.setText("%.6g" % sett.value("E", 0., type=float))
editKi.setText("%.6g" % sett.value("ki", 2.662, type=float))
editKf.setText("%.6g" % sett.value("kf", 2.662, type=float))

Qlayout.addWidget(qtw.QLabel("h (rlu):", Qpanel), 0,0, 1,1)
Qlayout.addWidget(edith, 0,1, 1,2)
Qlayout.addWidget(qtw.QLabel("k (rlu):", Qpanel), 1,0, 1,1)
Qlayout.addWidget(editk, 1,1, 1,2)
Qlayout.addWidget(qtw.QLabel("l (rlu):", Qpanel), 2,0, 1,1)
Qlayout.addWidget(editl, 2,1, 1,2)
Qlayout.addWidget(qtw.QLabel("E (meV):", Qpanel), 3,0, 1,1)
Qlayout.addWidget(editE, 3,1, 1,2)
Qlayout.addWidget(qtw.QLabel(u"ki, kf (1/\u212b):", Qpanel), 4,0, 1,1)
Qlayout.addWidget(editKi, 4,1, 1,1)
Qlayout.addWidget(editKf, 4,2, 1,1)
Qlayout.addWidget(qtw.QLabel(u"|Q| (1/\u212b):", Qpanel), 5,0, 1,1)
Qlayout.addWidget(editQAbs, 5,1, 1,2)
Qlayout.addWidget(separatorTas, 6,0,1,3)
taslayout.addWidget(qtw.QLabel("a1, a2 (deg):", taspanel), 7,0, 1,1)
taslayout.addWidget(editA1, 7,1, 1,1)
taslayout.addWidget(editA2, 7,2, 1,1)
taslayout.addWidget(qtw.QLabel("a3, a4 (deg):", taspanel), 8,0, 1,1)
taslayout.addWidget(editA3, 8,1, 1,1)
taslayout.addWidget(editA4, 8,2, 1,1)
taslayout.addWidget(qtw.QLabel("a5, a6 (deg):", taspanel), 9,0, 1,1)
taslayout.addWidget(editA5, 9,1, 1,1)
taslayout.addWidget(editA6, 9,2, 1,1)
taslayout.addWidget(qtw.QLabel("Mono., Ana. d (A):", taspanel), 10,0, 1,1)
taslayout.addWidget(editDm, 10,1, 1,1)
taslayout.addWidget(editDa, 10,2, 1,1)
taslayout.addItem(qtw.QSpacerItem(16,16, qtw.QSizePolicy.Minimum, qtw.QSizePolicy.Expanding), 11,0, 1,3)

tabs.addTab(taspanel, "TAS")
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# info/settings tab
infopanel = qtw.QWidget()
infolayout = qtw.QGridLayout(infopanel)

comboA3 = qtw.QComboBox(infopanel)
comboA3.addItems(["Takin", "NOMAD", "SICS", "NICOS"])
a3_offs = [np.pi/2., np.pi, 0., 0.]
comboA3.setCurrentIndex(sett.value("a3_conv", 1, type=int))

def comboA3ConvChanged():
	idx = comboA3.currentIndex()
	tas.set_a3_offs(a3_offs[idx])
	QChanged()

comboA3.currentIndexChanged.connect(comboA3ConvChanged)


separatorInfo = qtw.QFrame(infopanel)
separatorInfo.setFrameStyle(qtw.QFrame.HLine)


infolayout.addWidget(qtw.QLabel("TAS Calculator.", infopanel), 0,0, 1,2)
infolayout.addWidget(qtw.QLabel("Written by Tobias Weber <tweber@ill.fr>.", infopanel), 1,0, 1,2)
infolayout.addWidget(qtw.QLabel("Date: October 24, 2018.", infopanel), 2,0, 1,2)
infolayout.addWidget(separatorInfo, 3,0, 1,2)
infolayout.addWidget(qtw.QLabel("Interpreter Version: " + sys.version + ".", infopanel), 4,0, 1,2)
infolayout.addWidget(qtw.QLabel("Numpy Version: " + np.__version__ + ".", infopanel), 5,0, 1,2)
infolayout.addWidget(qtw.QLabel("Qt Version: " + qtc.QT_VERSION_STR + ".", infopanel), 6,0, 1,2)
infolayout.addItem(qtw.QSpacerItem(16,16, qtw.QSizePolicy.Minimum, qtw.QSizePolicy.Expanding), 7,0, 1,2)
infolayout.addWidget(qtw.QLabel("A3 Convention:", infopanel), 8,0, 1,1)
infolayout.addWidget(comboA3, 8,1, 1,1)

tabs.addTab(infopanel, "Infos")
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# main dialog window

dlg = qtw.QDialog()
dlg.setWindowTitle("TAS Calculator")
mainlayout = qtw.QGridLayout(dlg)
mainlayout.addWidget(tabs)

if sett.contains("geo"):
	dlg.restoreGeometry(sett.value("geo"))

xtalChanged()
KiKfChanged()
comboA3ConvChanged()
#QChanged()

dlg.show()
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
sett.setValue("a3_conv", comboA3.currentIndex())
sett.setValue("geo", dlg.saveGeometry())
# -----------------------------------------------------------------------------
