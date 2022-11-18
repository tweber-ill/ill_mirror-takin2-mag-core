/**
 * brillouin zone tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date Maz-2022
 * @license GPLv3, see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2022  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 * "misc" project
 * Copyright (C) 2017-2021  Tobias WEBER (privately developed).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ----------------------------------------------------------------------------
 */

#ifndef __BZTOOL_H__
#define __BZTOOL_H__

#include <QtWidgets/QDialog>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QLabel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QSplitter>
#include <QtCore/QSettings>

#include <vector>
#include <sstream>

#include "globals.h"
#include "plot_cut.h"

#include "tlibs2/libs/qt/recent.h"
#include "tlibs2/libs/qt/glplot.h"
#include "tlibs2/libs/qt/numerictablewidgetitem.h"


/**
 * symmetry operation table column indices
 */
enum : int
{
	COL_OP = 0,
	COL_PROP,

	NUM_SYMOP_COLS
};


/**
 * formulas table column indices
 */
enum : int
{
	COL_FORMULA = 0,

	NUM_FORMULAS_COLS
};


class BZDlg : public QDialog
{
public:
	BZDlg(QWidget* pParent = nullptr);
	~BZDlg() = default;


protected:
	QSettings *m_sett = nullptr;
	QMenuBar *m_menu = nullptr;
	QLabel *m_status = nullptr;
	QSplitter *m_split_inout{};

	// tabs
	QTabWidget *m_tabs_in{}, *m_tabs_out{};

	// 3d plotter
	QDialog *m_dlgPlot = nullptr;
	std::shared_ptr<tl2::GlPlot> m_plot;
	std::size_t m_sphere = 0, m_plane = 0;
	QLabel *m_labelGlInfos[4] = { nullptr, nullptr, nullptr, nullptr };
	QLabel *m_status3D = nullptr;
	QCheckBox *m_plot_coordcross = nullptr;
	QCheckBox *m_plot_labels = nullptr;
	QCheckBox *m_plot_plane = nullptr;

	// symops panel
	QDoubleSpinBox *m_editA = nullptr;
	QDoubleSpinBox *m_editB = nullptr;
	QDoubleSpinBox *m_editC = nullptr;
	QDoubleSpinBox *m_editAlpha = nullptr;
	QDoubleSpinBox *m_editBeta = nullptr;
	QDoubleSpinBox *m_editGamma = nullptr;
	QTableWidget *m_symops = nullptr;
	QComboBox *m_comboSG = nullptr;
	QMenu *m_symOpContextMenu = nullptr;         // menu in case a symop is selected
	QMenu *m_symOpContextMenuNoItem = nullptr;   // menu if nothing is selected

	// brillouin zone and cuts panel
	BZCutScene *m_bzscene = nullptr;
	BZCutView *m_bzview = nullptr;
	QDoubleSpinBox *m_cutX = nullptr;
	QDoubleSpinBox *m_cutY = nullptr;
	QDoubleSpinBox *m_cutZ = nullptr;
	QDoubleSpinBox *m_cutNX = nullptr;
	QDoubleSpinBox *m_cutNY = nullptr;
	QDoubleSpinBox *m_cutNZ = nullptr;
	QDoubleSpinBox *m_cutD = nullptr;
	QSpinBox *m_BZDrawOrder = nullptr;
	QSpinBox *m_BZCalcOrder = nullptr;

	// formulas panel
	QTableWidget *m_formulas = nullptr;
	QMenu *m_formulasContextMenu = nullptr;         // menu in case a symop is selected
	QMenu *m_formulasContextMenuNoItem = nullptr;   // menu if nothing is selected

	// results panel
	QPlainTextEdit *m_bzresults = nullptr;
	std::string m_descrBZ, m_descrBZCut;         // text description of the results

	// menu
	QAction *m_acCutHull = nullptr;

	// recently opened files
	tl2::RecentFiles m_recent{};
	QMenu *m_menuOpenRecent{};
	// function to call for the recent file menu items
	std::function<bool(const QString& filename)> m_open_func
		= [this](const QString& filename) -> bool
	{
		return this->Load(filename);
	};

	t_mat m_crystA = tl2::unit<t_mat>(3);        // crystal A matrix
	t_mat m_crystB = tl2::unit<t_mat>(3);        // crystal B matrix
	t_mat m_cut_plane = tl2::unit<t_mat>(3);     // cutting plane
	t_mat m_cut_plane_inv = tl2::unit<t_mat>(3); // and its inverse
	t_real m_cut_norm_scale = 1.;                // convert 1/A to rlu lengths along the normal

	std::vector<std::vector<t_mat>> m_sg_ops;    // symops per space group
	std::vector<std::vector<t_vec>> m_bz_polys;  // polygons of the 3d bz

	t_real m_min_x = 1., m_max_x = -1.;          // plot ranges for curves
	t_real m_min_y = 1., m_max_y = -1.;          // plot ranges for curves


protected:
	// space group / symops tab
	void AddSymOpTabItem(int row = -1, const t_mat& op = tl2::unit<t_mat>(4));
	void DelSymOpTabItem(int begin=-2, int end=-2);
	void MoveSymOpTabItemUp();
	void MoveSymOpTabItemDown();
	void SymOpTableItemChanged(QTableWidgetItem *item);
	void ShowSymOpTableContextMenu(const QPoint& pt);
	std::vector<t_mat> GetSymOps(bool only_centring = false) const;
	std::vector<int> GetSelectedSymOpRows(bool sort_reversed = false) const;

	// formulas tab
	void AddFormulaTabItem(int row = -1, const std::string& formula = "");
	void DelFormulaTabItem(int begin=-2, int end=-2);
	void MoveFormulaTabItemUp();
	void MoveFormulaTabItemDown();
	void FormulaTableItemChanged(QTableWidgetItem *item);
	void ShowFormulaTableContextMenu(const QPoint& pt);
	std::vector<std::string> GetFormulas() const;
	std::vector<int> GetSelectedFormulaRows(bool sort_reversed = false) const;

	// menu functions
	void NewFile();
	void Load();
	void Save();
	void ImportCIF();
	void GetSymOpsFromSG();
	void SaveCutSVG();

	bool Load(const QString& filename);
	bool Save(const QString& filename);

	// calculation functions
	void CalcB(bool full_recalc = true);
	void CalcBZ(bool full_recalc = true);
	void CalcBZCut();
	void CalcFormulas();

	// 3d bz cut plot
	void BZCutMouseMoved(t_real x, t_real y);

	// 3d bz plot
	void ShowBZPlot();
	void ClearBZPlot();
	void PlotAddVoronoiVertex(const t_vec& pos);
	void PlotAddBraggPeak(const t_vec& pos);
	void PlotAddTriangles(const std::vector<t_vec>& vecs);
	void PlotSetPlane(const t_vec& norm, t_real d);
	void Set3DStatusMsg(const std::string& msg);
	void PlotShowCoordCross(bool show);
	void PlotShowLabels(bool show);
	void PlotShowPlane(bool show);
	void UpdateBZDescription();

	void PlotMouseDown(bool left, bool mid, bool right);
	void PlotMouseUp(bool left, bool mid, bool right);
	void PickerIntersection(const t_vec3_gl* pos,
		std::size_t objIdx, const t_vec3_gl* posSphere);
	void AfterGLInitialisation();

	virtual void closeEvent(QCloseEvent *evt) override;

	// conversion functions
	static std::string OpToStr(const t_mat& rot);
	static t_mat StrToOp(const std::string& str);

	static std::string GetOpProperties(const t_mat& op);


private:
	int m_symOpCursorRow = -1;                   // current sg row
	int m_formulaCursorRow = -1;                 // current sg row
	bool m_symOpIgnoreChanges = 1;               // ignore sg changes
	bool m_formulaIgnoreChanges = 1;             // ignore sg changes
	bool m_ignoreCalc = 0;                       // ignore bz calculation

	long m_curPickedObj = -1;                    // current 3d bz object
	std::vector<std::size_t> m_plotObjs;         // 3d bz plot objects
};


#endif
