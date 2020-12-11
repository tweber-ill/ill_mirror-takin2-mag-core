/**
 * structure factor tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date Dec-2018
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 28-Dec-2018 from my privately developed "misc" project (https://github.com/t-weber/misc).
 */

#ifndef __SFACT_H__
#define __SFACT_H__

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

#include "tlibs2/libs/glplot.h"
#include "tlibs2/libs/math20.h"

#include "numerictablewidgetitem.h"


using t_real = double;
using t_cplx = std::complex<t_real>;
using t_vec = tl2::vec<t_real, std::vector>;
using t_vec_cplx = std::vector<t_cplx>;
using t_mat = tl2::mat<t_real, std::vector>;
using t_mat_cplx = tl2::mat<t_cplx, std::vector>;


struct NuclPos
{
	std::string name;
	t_cplx b;
	t_real pos[3];
};


class StructFactDlg : public QDialog
{
public:
	StructFactDlg(QWidget* pParent = nullptr);
	~StructFactDlg() = default;

protected:
	QSettings *m_sett = nullptr;
	QMenuBar *m_menu = nullptr;

	QDialog *m_dlgPlot = nullptr;
	std::shared_ptr<GlPlot> m_plot;
	std::size_t m_sphere = 0;
	QLabel *m_labelGlInfos[4] = { nullptr, nullptr, nullptr, nullptr };
	QLabel *m_status3D = nullptr;

	QTableWidget *m_nuclei = nullptr;
	QPlainTextEdit *m_structfacts = nullptr;
	QPlainTextEdit *m_powderlines = nullptr;
	QTableWidget *m_nuclei_FindSG = nullptr;
	QPlainTextEdit *m_sgmatches = nullptr;

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

	QMenu *m_pTabContextMenu = nullptr;			// menu in case a nucleus is selected
	QMenu *m_pTabContextMenuNoItem = nullptr;	// menu if nothing is selected

	QMenu *m_pTabContextMenu_FindSG = nullptr;			// menu in case a nucleus is selected
	QMenu *m_pTabContextMenuNoItem_FindSG = nullptr;	// menu if nothing is selected

	t_mat m_crystA = tl2::unit<t_mat>(3);
	t_mat m_crystB = tl2::unit<t_mat>(3);

protected:
	// for nuclei tab
	void AddTabItem(int row=-1, const std::string& name="n/a", t_real bRe=0., t_real bIm=0.,
		t_real x=0., t_real y=0., t_real z=0., t_real scale=1., const std::string &col="#ff0000");
	void DelTabItem(int begin=-2, int end=-2);
	void MoveTabItemUp();
	void MoveTabItemDown();

	void Add3DItem(int row=-1);
	void Set3DStatusMsg(const std::string& msg);

	void TableCurCellChanged(int rowNew, int colNew, int rowOld, int colOld);
	void TableCellEntered(const QModelIndex& idx);
	void TableItemChanged(QTableWidgetItem *item);
	void ShowTableContextMenu(const QPoint& pt);


	void Load();
	void Save();
	void ImportCIF();
	void ImportTAZ();
	void ExportTAZ();
	void GenerateFromSG();

	std::vector<NuclPos> GetNuclei() const;
	void CalcB(bool bFullRecalc=true);
	void Calc();

	void PlotMouseDown(bool left, bool mid, bool right);
	void PlotMouseUp(bool left, bool mid, bool right);
	void PickerIntersection(const t_vec3_gl* pos, std::size_t objIdx, const t_vec3_gl* posSphere);
	void AfterGLInitialisation();


	// for space group finder tab
	void AddTabItem_FindSG(int row=-1, t_real x=0., t_real y=0., t_real z=0.);
	void DelTabItem_FindSG(int begin=-2, int end=-2);
	void MoveTabItemUp_FindSG();
	void MoveTabItemDown_FindSG();

	void TableItemChanged_FindSG(QTableWidgetItem *item);
	void ShowTableContextMenu_FindSG(const QPoint& pt);

	void FindSG();


	virtual void closeEvent(QCloseEvent *evt) override;

private:
	int m_iCursorRow = -1;
	bool m_ignoreChanges = 1;
	bool m_ignoreCalc = 0;

	long m_curPickedObj = -1;

	int m_iCursorRow_FindSG = -1;

	static std::vector<std::string> g_default_colours;

private:
	std::vector<int> GetSelectedRows(bool sort_reversed = false) const;
	std::vector<int> GetSelectedRows_FindSG(bool sort_reversed = false) const;
};


#endif
