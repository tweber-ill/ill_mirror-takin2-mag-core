/**
 * magnetic structure factor tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date Jan-2019
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 28-Dec-2018 from the privately developed "misc" project (https://github.com/t-weber/misc).
 */

#ifndef __MAG_SFACT_H__
#define __MAG_SFACT_H__

#include <QtWidgets/QDialog>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QLabel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCheckBox>
#include <QtCore/QSettings>

#include <vector>
#include <sstream>
#include <complex>

#include "libs/_cxx20/glplot.h"
#include "libs/_cxx20/math_algos.h"

#include "../structfact/numerictablewidgetitem.h"


using t_real = double;
using t_cplx = std::complex<t_real>;
using t_vec = std::vector<t_real>;
using t_vec_cplx = std::vector<t_cplx>;
using t_mat = m::mat<t_real, std::vector>;
using t_mat_cplx = m::mat<t_cplx, std::vector>;


struct NuclPos
{
	std::string name;
	t_real pos[3];	// position
	t_cplx MAbs;	// scaling of Fourier components
	t_real ReM[3];	// real part of Fourier components
	t_real ImM[3];	// imag part of Fourier components
};


class MagStructFactDlg : public QDialog
{
public:
	MagStructFactDlg(QWidget* pParent = nullptr);
	~MagStructFactDlg() = default;

protected:
	QSettings *m_sett = nullptr;
	QMenuBar *m_menu = nullptr;

	QDialog *m_dlgPlot = nullptr;
	std::shared_ptr<GlPlot> m_plot;
	std::size_t m_sphere = 0;
	std::size_t m_arrow = 0;
	QLabel *m_labelGlInfos[4] = { nullptr, nullptr, nullptr, nullptr };
	QLabel *m_status3D = nullptr;

	QWidget *m_nucleipanel = nullptr;
	QWidget *m_propvecpanel = nullptr;
	QTableWidget *m_nuclei = nullptr;
	QTableWidget *m_propvecs = nullptr;
	QPlainTextEdit *m_structfacts = nullptr;
	QPlainTextEdit *m_powderlines = nullptr;
	QPlainTextEdit *m_moments = nullptr;

	QLineEdit *m_editA = nullptr;
	QLineEdit *m_editB = nullptr;
	QLineEdit *m_editC = nullptr;
	QLineEdit *m_editAlpha = nullptr;
	QLineEdit *m_editBeta = nullptr;
	QLineEdit *m_editGamma = nullptr;

	QComboBox *m_comboSG = nullptr;
	std::vector<std::vector<t_mat>> m_SGops;

	QSpinBox *m_maxBZ = nullptr;
	QCheckBox *m_RemoveZeroes = nullptr;
	QSpinBox *m_maxSC = nullptr;

	t_mat m_crystA = m::unit<t_mat>(3);
	t_mat m_crystB = m::unit<t_mat>(3);

protected:
	// general table operations
	void MoveTabItemUp(QTableWidget *pTab);
	void MoveTabItemDown(QTableWidget *pTab);
	void ShowTableContextMenu(QTableWidget *pTab, QMenu *pMenu, QMenu *pMenuNoItem, const QPoint& pt);
	std::vector<int> GetSelectedRows(QTableWidget *pTab, bool sort_reversed = false) const;

	// Fourier components table
	void AddTabItem(int row=-1, const std::string& name="n/a", t_real MMag=1.,
		t_real x=0., t_real y=0., t_real z=0., 
		t_real ReMx=0., t_real ReMy=0., t_real ReMz=1.,
		t_real ImMx=0., t_real ImMy=0., t_real ImMz=0., 
		t_real scale=1., const std::string &col="#ff0000");
	void DelTabItem(int begin=-2, int end=-2);
	void TableCurCellChanged(int rowNew, int colNew, int rowOld, int colOld);
	void TableCellEntered(const QModelIndex& idx);
	void TableItemChanged(QTableWidgetItem *item);

	// propagation vectors table
	void AddPropItem(int row=-1, const std::string& name="n/a", 
		t_real x=0., t_real y=0., t_real z=0.);
	void DelPropItem(int begin=-2, int end=-2);
	void PropItemChanged(QTableWidgetItem *item);

	void Add3DItem(int row=-1);
	void Sync3DItem(int row=-1);
	void Set3DStatusMsg(const std::string& msg);

	void Load();
	void Save();
	void ImportCIF();
	void GenerateFromSG();

	std::vector<NuclPos> GetNuclei() const;
	void CalcB(bool bFullRecalc=true);
	void Calc();

	void PlotMouseDown(bool left, bool mid, bool right);
	void PlotMouseUp(bool left, bool mid, bool right);
	void PickerIntersection(const t_vec3_gl* pos, std::size_t objIdx, const t_vec3_gl* posSphere);
	void AfterGLInitialisation();

	virtual void closeEvent(QCloseEvent *evt) override;

private:
	int m_iCursorRow = -1;
	bool m_ignoreChanges = 1;
	bool m_ignoreCalc = 0;

	long m_curPickedObj = -1;
};


#endif
