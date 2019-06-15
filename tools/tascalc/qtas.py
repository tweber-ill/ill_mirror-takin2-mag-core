#
# tascalc gui
# @author Tobias Weber <tweber@ill.fr>
# @date 24-oct-18
# @license see 'LICENSE' file
#
# __ident__ : TAS Calculator
# __descr__ : Calculates triple-axis angles.
#

# -----------------------------------------------------------------------------
# dependencies
import sys
import tascalc as tas
import numpy as np
import numpy.linalg as la

# try to import qt5...
try:
	import PyQt5 as qt
	import PyQt5.QtCore as qtc
	import PyQt5.QtWidgets as qtw
	qt_ver = 5
except ImportError:
	# ...and if not possible try to import qt4 instead
	try:
		import PyQt4 as qt
		import PyQt4.QtCore as qtc
		import PyQt4.QtGui as qtw
		qt_ver = 4
	except ImportError:
		print("Error: No suitable version of Qt was found!")
		exit(-1)
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# main application
np.set_printoptions(suppress=True, precision=4)

app = qtw.QApplication(sys.argv)
app.setApplicationName("qtas")
#app.setStyle("Fusion")

sett = qtc.QSettings("tobis_stuff", "in20tool")
if sett.contains("mainwnd/theme"):
	app.setStyle(sett.value("mainwnd/theme"))

tabs = qtw.QTabWidget()
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# variables
B = np.array([[1,0,0], [0,1,0], [0,0,1]])
orient_rlu = np.array([1,0,0])
#orient2_rlu = np.array([0,1,0])
orient_up_rlu = np.array([0,0,1])

g_eps = 1e-4
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
separatorXtal = qtw.QFrame(xtalpanel)
separatorXtal.setFrameStyle(qtw.QFrame.HLine)
editAx = qtw.QLineEdit(scpanel)
editAy = qtw.QLineEdit(scpanel)
editAz = qtw.QLineEdit(scpanel)
editBx = qtw.QLineEdit(scpanel)
editBy = qtw.QLineEdit(scpanel)
editBz = qtw.QLineEdit(scpanel)
editBMat = qtw.QPlainTextEdit(xtalpanel)
editBMat.setReadOnly(True)


def xtalChanged():
	global B, orient_rlu, orient_up_rlu
	lattice = np.array([getfloat(editA.text()), getfloat(editB.text()), getfloat(editC.text())])
	angles = np.array([getfloat(editAlpha.text()), getfloat(editBeta.text()), getfloat(editGamma.text())])
	orient_rlu = np.array([getfloat(editAx.text()), getfloat(editAy.text()), getfloat(editAz.text())])
	orient2_rlu = np.array([getfloat(editBx.text()), getfloat(editBy.text()), getfloat(editBz.text())])

	try:
		B = tas.get_B(lattice, angles/180.*np.pi)
		invB = la.inv(B)

		metric = tas.get_metric(B)
		ang = tas.angle(orient_rlu, orient2_rlu, metric)

		orient_up_rlu = tas.cross(orient_rlu, orient2_rlu, B)
		orient_up_rlu_norm = orient_up_rlu / la.norm(orient_up_rlu)

		UB = tas.get_UB(B, orient_rlu, orient2_rlu, orient_up_rlu)
		invUB = la.inv(UB)

		editBMat.setPlainText("Scattering plane normal: %s rlu.\n" % str(orient_up_rlu_norm) \
			+"Angle between orientation vectors 1 and 2: %.4g deg.\n" % (ang/np.pi*180.) \
			+"\nB =\n%10.4f %10.4f %10.4f\n%10.4f %10.4f %10.4f\n%10.4f %10.4f %10.4f\n" \
			% (B[0,0],B[0,1],B[0,2], B[1,0],B[1,1],B[1,2], B[2,0],B[2,1],B[2,2]) \
			+"\nB^(-1) =\n%10.4f %10.4f %10.4f\n%10.4f %10.4f %10.4f\n%10.4f %10.4f %10.4f\n" \
			% (invB[0,0],invB[0,1],invB[0,2], invB[1,0],invB[1,1],invB[1,2], invB[2,0],invB[2,1],invB[2,2]) \
			+"\nUB =\n%10.4f %10.4f %10.4f\n%10.4f %10.4f %10.4f\n%10.4f %10.4f %10.4f\n" \
			% (UB[0,0],UB[0,1],UB[0,2], UB[1,0],UB[1,1],UB[1,2], UB[2,0],UB[2,1],UB[2,2]) \
			+"\n(UB)^(-1) =\n%10.4f %10.4f %10.4f\n%10.4f %10.4f %10.4f\n%10.4f %10.4f %10.4f\n" \
			% (invUB[0,0],invUB[0,1],invUB[0,2], invUB[1,0],invUB[1,1],invUB[1,2], invUB[2,0],invUB[2,1],invUB[2,2]) \
		)
	except (ArithmeticError, la.LinAlgError) as err:
		editBMat.setPlainText("invalid")
	QChanged()

def planeChanged():
	xtalChanged()
	#QChanged()


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


editA.setText("%.6g" % sett.value("qtas/a", 5., type=float))
editB.setText("%.6g" % sett.value("qtas/b", 5., type=float))
editC.setText("%.6g" % sett.value("qtas/c", 5., type=float))
editAlpha.setText("%.6g" % sett.value("qtas/alpha", 90., type=float))
editBeta.setText("%.6g" % sett.value("qtas/beta", 90., type=float))
editGamma.setText("%.6g" % sett.value("qtas/gamma", 90., type=float))
editAx.setText("%.6g" % sett.value("qtas/ax", 1., type=float))
editAy.setText("%.6g" % sett.value("qtas/ay", 0., type=float))
editAz.setText("%.6g" % sett.value("qtas/az", 0., type=float))
editBx.setText("%.6g" % sett.value("qtas/bx", 0., type=float))
editBy.setText("%.6g" % sett.value("qtas/by", 1., type=float))
editBz.setText("%.6g" % sett.value("qtas/bz", 0., type=float))

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
xtallayout.addWidget(separatorXtal, 6,0, 1,4)
sclayout.addWidget(qtw.QLabel("Orient. 1 (rlu):", scpanel), 7,0, 1,1)
sclayout.addWidget(editAx, 7,1, 1,1)
sclayout.addWidget(editAy, 7,2, 1,1)
sclayout.addWidget(editAz, 7,3, 1,1)
sclayout.addWidget(qtw.QLabel("Orient. 2 (rlu):", scpanel), 8,0, 1,1)
sclayout.addWidget(editBx, 8,1, 1,1)
sclayout.addWidget(editBy, 8,2, 1,1)
sclayout.addWidget(editBz, 8,3, 1,1)
xtallayout.addWidget(editBMat, 9,0, 2,4)

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

checkA4Sense = qtw.QCheckBox(taspanel)

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

tasstatus = qtw.QLabel(taspanel)

separatorTas = qtw.QFrame(Qpanel)
separatorTas.setFrameStyle(qtw.QFrame.HLine)
separatorTas2 = qtw.QFrame(Qpanel)
separatorTas2.setFrameStyle(qtw.QFrame.HLine)
separatorTas3 = qtw.QFrame(Qpanel)
separatorTas3.setFrameStyle(qtw.QFrame.HLine)


def TASChanged():
	global orient_rlu, orient_up_rlu

	a1 = getfloat(editA1.text()) / 180. * np.pi
	a2 = a1 * 2.
	a3 = getfloat(editA3.text()) / 180. * np.pi
	a4 = getfloat(editA4.text()) / 180. * np.pi
	a5 = getfloat(editA5.text()) / 180. * np.pi
	a6 = a5 * 2.
	dmono = getfloat(editDm.text())
	dana = getfloat(editDa.text())

	sense_sample = 1.
	if checkA4Sense.isChecked() == False:
		sense_sample = -1.

	editA2.setText("%.6g" % (a2 / np.pi * 180.))
	editA6.setText("%.6g" % (a6 / np.pi * 180.))

	try:
		ki = tas.get_monok(a1, dmono)
		kf = tas.get_monok(a5, dana)
		E = tas.get_E(ki, kf)
		Qlen = tas.get_Q(ki, kf, a4)
		Qvec = tas.get_hkl(ki, kf, a3, Qlen, orient_rlu, orient_up_rlu, B, sense_sample)

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
	global orient_rlu, orient_up_rlu, g_eps

	Q_rlu = np.array([getfloat(edith.text()), getfloat(editk.text()), getfloat(editl.text())])
	ki = getfloat(editKi.text())
	kf = getfloat(editKf.text())

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
		sense_sample = 1.
		if checkA4Sense.isChecked() == False:
			sense_sample = -1.

		[a3, a4, dist_Q_plane] = tas.get_a3a4(ki, kf, Q_rlu, orient_rlu, orient_up_rlu, B, sense_sample)
		Qlen = tas.get_Q(ki, kf, a4)
		Q_in_plane = np.abs(dist_Q_plane) < g_eps

		editA3.setText("%.6g" % (a3 / np.pi * 180.))
		editA4.setText("%.6g" % (a4 / np.pi * 180.))
		editQAbs.setText("%.6g" % Qlen)
		if Q_in_plane:
			tasstatus.setText("")
		else:
			tasstatus.setText(u"WARNING: Q is out of the plane by %.4g \u212b\u207b\u00b9!" % dist_Q_plane)
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


editDm.setText("%.6g" % sett.value("qtas/dm", 3.355, type=float))
editDa.setText("%.6g" % sett.value("qtas/da", 3.355, type=float))
edith.setText("%.6g" % sett.value("qtas/h", 1., type=float))
editk.setText("%.6g" % sett.value("qtas/k", 0., type=float))
editl.setText("%.6g" % sett.value("qtas/l", 0., type=float))
#editE.setText("%.6g" % sett.value("qtas/E", 0., type=float))
editKi.setText("%.6g" % sett.value("qtas/ki", 2.662, type=float))
editKf.setText("%.6g" % sett.value("qtas/kf", 2.662, type=float))


checkA4Sense.setText("a4 sense is counter-clockwise")
checkA4Sense.setChecked(sett.value("qtas/a4_sense", 1, type=bool))
checkA4Sense.stateChanged.connect(QChanged)


Qlayout.addWidget(qtw.QLabel("h (rlu):", Qpanel), 0,0, 1,1)
Qlayout.addWidget(edith, 0,1, 1,2)
Qlayout.addWidget(qtw.QLabel("k (rlu):", Qpanel), 1,0, 1,1)
Qlayout.addWidget(editk, 1,1, 1,2)
Qlayout.addWidget(qtw.QLabel("l (rlu):", Qpanel), 2,0, 1,1)
Qlayout.addWidget(editl, 2,1, 1,2)
Qlayout.addWidget(qtw.QLabel("E (meV):", Qpanel), 3,0, 1,1)
Qlayout.addWidget(editE, 3,1, 1,2)
Qlayout.addWidget(qtw.QLabel(u"ki, kf (\u212b\u207b\u00b9):", Qpanel), 4,0, 1,1)
Qlayout.addWidget(editKi, 4,1, 1,1)
Qlayout.addWidget(editKf, 4,2, 1,1)
Qlayout.addWidget(qtw.QLabel(u"|Q| (\u212b\u207b\u00b9):", Qpanel), 5,0, 1,1)
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
taslayout.addWidget(separatorTas2, 10,0, 1,3)
taslayout.addWidget(qtw.QLabel("Sense:", taspanel), 11,0, 1,1)
taslayout.addWidget(checkA4Sense, 11,1, 1,2)
taslayout.addWidget(separatorTas3, 12,0, 1,3)
taslayout.addWidget(qtw.QLabel(u"Mono., Ana. d (\u212b):", taspanel), 13,0, 1,1)
taslayout.addWidget(editDm, 13,1, 1,1)
taslayout.addWidget(editDa, 13,2, 1,1)
taslayout.addItem(qtw.QSpacerItem(16,16, qtw.QSizePolicy.Minimum, qtw.QSizePolicy.Expanding), 14,0, 1,3)
taslayout.addWidget(tasstatus, 15,0, 1,3)

tabs.addTab(taspanel, "TAS")
# -----------------------------------------------------------------------------



# -----------------------------------------------------------------------------
# info/settings tab
infopanel = qtw.QWidget()
infolayout = qtw.QGridLayout(infopanel)

comboA3 = qtw.QComboBox(infopanel)
comboA3.addItems(["Takin", "NOMAD", "SICS", "NICOS"])
a3_offs = [np.pi/2., np.pi, 0., 0.]
comboA3.setCurrentIndex(sett.value("qtas/a3_conv", 1, type=int))

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

if sett.contains("qtas/geo"):
	geo = sett.value("qtas/geo")
	if qt_ver == 4:
		geo = geo.toByteArray()
	dlg.restoreGeometry(geo)

xtalChanged()
KiKfChanged()
comboA3ConvChanged()
#QChanged()

dlg.show()
app.exec_()


# save settings
sett.setValue("qtas/a", getfloat(editA.text()))
sett.setValue("qtas/b", getfloat(editB.text()))
sett.setValue("qtas/c", getfloat(editC.text()))
sett.setValue("qtas/alpha", getfloat(editAlpha.text()))
sett.setValue("qtas/beta", getfloat(editBeta.text()))
sett.setValue("qtas/gamma", getfloat(editGamma.text()))
sett.setValue("qtas/ax", getfloat(editAx.text()))
sett.setValue("qtas/ay", getfloat(editAy.text()))
sett.setValue("qtas/az", getfloat(editAz.text()))
sett.setValue("qtas/bx", getfloat(editBx.text()))
sett.setValue("qtas/by", getfloat(editBy.text()))
sett.setValue("qtas/bz", getfloat(editBz.text()))
sett.setValue("qtas/dm", getfloat(editDm.text()))
sett.setValue("qtas/da", getfloat(editDa.text()))
sett.setValue("qtas/h", getfloat(edith.text()))
sett.setValue("qtas/k", getfloat(editk.text()))
sett.setValue("qtas/l", getfloat(editl.text()))
#sett.setValue("qtas/E", getfloat(editE.text()))
sett.setValue("qtas/ki", getfloat(editKi.text()))
sett.setValue("qtas/kf", getfloat(editKf.text()))
sett.setValue("qtas/a3_conv", comboA3.currentIndex())
sett.setValue("qtas/a4_sense", checkA4Sense.isChecked())
sett.setValue("qtas/geo", dlg.saveGeometry())
# -----------------------------------------------------------------------------
