/**
 * magnon dynamics
 * @author Tobias Weber <tweber@ill.fr>
 * @date Jan-2022
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 28-Dec-2018 from my privately developed "misc" project (https://github.com/t-weber/misc).
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2022  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 * "misc" project
 * Copyright (C) 2017-2022  Tobias WEBER (privately developed).
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

#ifndef __MAG_DYN_GUI_H__
#define __MAG_DYN_GUI_H__

#include <QtCore/QSettings>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QDialog>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QAction>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>

#include <qcustomplot.h>

#include <vector>
#include <unordered_map>
#include <sstream>
#include <optional>

#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/magdyn.h"
#include "tlibs2/libs/qt/numerictablewidgetitem.h"
#include "tlibs2/libs/qt/recent.h"
#include "tlibs2/libs/qt/glplot.h"
#include "graph.h"

using namespace tl2_mag;


using t_size = std::size_t;
using t_real = double;
using t_cplx = std::complex<t_real>;

using t_vec_real = tl2::vec<t_real, std::vector>;
using t_mat_real = tl2::mat<t_real, std::vector>;

using t_vec = tl2::vec<t_cplx, std::vector>;
using t_mat = tl2::mat<t_cplx, std::vector>;

using t_magdyn = MagDyn<t_mat, t_vec, t_mat_real, t_vec_real, t_cplx, t_real, t_size>;

using t_real_gl = tl2::t_real_gl;
using t_vec2_gl = tl2::t_vec2_gl;
using t_vec3_gl = tl2::t_vec3_gl;
using t_vec_gl = tl2::t_vec_gl;
using t_mat_gl = tl2::t_mat_gl;


extern t_real g_eps;
extern int g_prec;
extern int g_prec_gui;


/**
 * columns of the sites table
 */
enum : int
{
	COL_SITE_NAME = 0,
	COL_SITE_POS_X, COL_SITE_POS_Y, COL_SITE_POS_Z,
	COL_SITE_SPIN_X, COL_SITE_SPIN_Y, COL_SITE_SPIN_Z,
	COL_SITE_SPIN_MAG,

	NUM_SITE_COLS
};


/**
 * columns of the exchange terms table
 */
enum : int
{
	COL_XCH_NAME = 0,
	COL_XCH_ATOM1_IDX, COL_XCH_ATOM2_IDX,
	COL_XCH_DIST_X, COL_XCH_DIST_Y, COL_XCH_DIST_Z,
	COL_XCH_INTERACTION,
	COL_XCH_DMI_X, COL_XCH_DMI_Y, COL_XCH_DMI_Z,

	NUM_XCH_COLS
};


/**
 * columns of the variables table
 */
enum : int
{
	COL_VARS_NAME = 0,
	COL_VARS_VALUE_REAL,
	COL_VARS_VALUE_IMAG,

	NUM_VARS_COLS
};


/**
 * columns of table with saved fields
 */
enum : int
{
	COL_FIELD_H = 0, COL_FIELD_K, COL_FIELD_L,
	COL_FIELD_MAG,

	NUM_FIELD_COLS
};



class MagDynDlg : public QDialog
{
public:
	MagDynDlg(QWidget* pParent = nullptr);
	virtual ~MagDynDlg();

	MagDynDlg(const MagDynDlg&) = delete;
	const MagDynDlg& operator=(const MagDynDlg&) = delete;


protected:
	QSettings *m_sett{};
	QMenuBar *m_menu{};
	QSplitter *m_split_inout{};
	QLabel *m_status{};
	QProgressBar *m_progress{};

	QAction *m_autocalc{};
	QAction *m_use_dmi{};
	QAction *m_use_field{};
	QAction *m_use_temperature{};
	QAction *m_use_weights{};
	QAction *m_use_projector{};
	QAction *m_unite_degeneracies{};

	// recently opened files
	tl2::RecentFiles m_recent{};
	QMenu *m_menuOpenRecent{};
	// function to call for the recent file menu items
	std::function<bool(const QString& filename)> m_open_func
		= [this](const QString& filename) -> bool
	{
		return this->Load(filename);
	};

	QGridLayout *m_maingrid{};

	// tab
	QTabWidget *m_tabs_in{}, *m_tabs_out{};

	// panels
	QWidget *m_sitespanel{};
	QWidget *m_termspanel{};
	QWidget *m_samplepanel{};
	QWidget *m_varspanel{};
	QWidget *m_disppanel{};
	QWidget *m_hamiltonianpanel{};
	QWidget *m_exportpanel{};

	// sites
	QTableWidget *m_sitestab{};
	QComboBox *m_comboSG{};
	std::vector<std::vector<t_mat_real>> m_SGops{};

	// terms, ordering vector, and rotation axis
	QTableWidget *m_termstab{};
	QComboBox *m_comboSG2{};  // copy of m_comboSG
	QDoubleSpinBox *m_ordering[3]{nullptr, nullptr, nullptr};
	QDoubleSpinBox *m_normaxis[3]{nullptr, nullptr, nullptr};

	// variables
	QTableWidget *m_varstab{};

	// dispersion
	QCustomPlot *m_plot{};
	std::vector<GraphWithWeights*> m_graphs{};
	QDoubleSpinBox *m_q_start[3]{nullptr, nullptr, nullptr};
	QDoubleSpinBox *m_q_end[3]{nullptr, nullptr, nullptr};
	QSpinBox *m_num_points{};
	QDoubleSpinBox *m_weight_scale{}, *m_weight_min{}, *m_weight_max{};

	// hamiltonian
	QTextEdit *m_hamiltonian{};
	QDoubleSpinBox *m_q[3]{nullptr, nullptr, nullptr};

	// external magnetic field
	QDoubleSpinBox *m_field_dir[3]{nullptr, nullptr, nullptr};
	QDoubleSpinBox *m_field_mag{};
	QCheckBox *m_align_spins{};
	QDoubleSpinBox *m_rot_axis[3]{nullptr, nullptr, nullptr};
	QDoubleSpinBox *m_rot_angle{};
	QTableWidget *m_fieldstab{};

	// temperature
	QDoubleSpinBox *m_temperature{};

	// export
	QDoubleSpinBox *m_exportStartQ[3]{nullptr, nullptr, nullptr};
	QDoubleSpinBox *m_exportEndQ1[3]{nullptr, nullptr, nullptr};
	QDoubleSpinBox *m_exportEndQ2[3]{nullptr, nullptr, nullptr};
	QDoubleSpinBox *m_exportEndQ3[3]{nullptr, nullptr, nullptr};
	QSpinBox *m_exportNumPoints[3]{nullptr, nullptr, nullptr};
	QComboBox *m_exportFormat{nullptr};

	// magnon dynamics calculator
	t_magdyn m_dyn{};

	// structure plotter
	QDialog *m_structplot_dlg{};
	QLabel *m_structplot_status{};
	QCheckBox *m_structplot_coordcross{};
	QCheckBox *m_structplot_labels{};
	QMenu *m_structplot_context{};
	QLabel *m_labelGlInfos[4]{nullptr, nullptr, nullptr, nullptr};
	tl2::GlPlot *m_structplot{};
	std::size_t m_structplot_sphere = 0;
	std::size_t m_structplot_arrow = 0;
	std::size_t m_structplot_cyl = 0;
	std::unordered_map<std::size_t, const t_magdyn::AtomSite*>
		m_structplot_atoms{};
	std::unordered_map<std::size_t, const t_magdyn::ExchangeTerm*>
		m_structplot_terms{};
	std::optional<std::size_t> m_structplot_cur_obj{};
	std::optional<std::size_t> m_structplot_cur_atom{};
	std::optional<std::size_t> m_structplot_cur_term{};

	// info dialog
	QDialog *m_info_dlg{};


protected:
	// set up gui
	void CreateSitesPanel();
	void CreateExchangeTermsPanel();
	void CreateVariablesPanel();
	void CreateSampleEnvPanel();
	void CreateDispersionPanel();
	void CreateHamiltonPanel();
	void CreateExportPanel();
	void CreateInfoDlg();
	void CreateMenuBar();

	// general table operations
	void MoveTabItemUp(QTableWidget *pTab);
	void MoveTabItemDown(QTableWidget *pTab);
	void ShowTableContextMenu(
		QTableWidget *pTab, QMenu *pMenu, QMenu *pMenuNoItem, const QPoint& pt);
	std::vector<int> GetSelectedRows(
		QTableWidget *pTab, bool sort_reversed = false) const;

	void AddSiteTabItem(int row = -1,
		const std::string& name = "n/a",
		t_real x = 0., t_real y = 0., t_real z = 0.,
		const std::string& sx = "0",
		const std::string& sy = "0",
		const std::string& sz = "1",
		t_real S = 1.);

	void AddTermTabItem(int row = -1,
		const std::string& name = "n/a",
		t_size atom_1 = 0, t_size atom_2 = 0,
		t_real dist_x = 0., t_real dist_y = 0., t_real dist_z = 0.,
		const std::string& J = "0",
		const std::string& dmi_x = "0",
		const std::string& dmi_y = "0",
		const std::string& dmi_z = "0");

	void AddVariableTabItem(int row = -1,
		const std::string& name = "var",
		const t_cplx& var = t_cplx{0, 0});

	void AddFieldTabItem(int row = -1,
		t_real Bh = 0., t_real Bk = 0., t_real Bl = 1.,
		t_real Bmag = 1.);

	void SetCurrentField();

	void DelTabItem(QTableWidget *pTab, int begin=-2, int end=-2);
	void UpdateVerticalHeader(QTableWidget *pTab);

	void SitesTableItemChanged(QTableWidgetItem *item);
	void TermsTableItemChanged(QTableWidgetItem *item);
	void VariablesTableItemChanged(QTableWidgetItem *item);

	void Clear();
	void Load();
	void Save();
	void ExportSQE();

	bool Load(const QString& filename);
	bool Save(const QString& filename);
	bool ExportSQE(const QString& filename);

	void SavePlotFigure();
	void SaveDispersion();

	void RotateField(bool ccw = true);
	void GenerateSitesFromSG();
	void GenerateCouplingsFromSG();

	void SyncSitesAndTerms();

	void CalcAll();
	void CalcDispersion();
	void CalcHamiltonian();

	void PlotMouseMove(QMouseEvent* evt);

	virtual void mousePressEvent(QMouseEvent *evt) override;
	virtual void closeEvent(QCloseEvent *evt) override;

	// structure plotter
	void ShowStructurePlot();
	void StructPlotMouseClick(bool left, bool mid, bool right);
	void StructPlotMouseDown(bool left, bool mid, bool right);
	void StructPlotMouseUp(bool left, bool mid, bool right);
	void StructPlotPickerIntersection(
		const t_vec3_gl* pos, std::size_t objIdx,
		const t_vec3_gl* posSphere);
	void StructPlotAfterGLInitialisation();
	void StructPlotSync();
	void StructPlotDelete();
	void StructPlotShowCoordCross(bool show);
	void StructPlotShowLabels(bool show);
	void StructPlotCentreCamera();

	// disable/enable gui input for threaded operations
	void EnableInput();
	void DisableInput();


private:
	int m_sites_cursor_row = -1;
	int m_terms_cursor_row = -1;
	int m_variables_cursor_row = -1;
	int m_fields_cursor_row = -1;

	bool m_ignoreTableChanges = true;
	bool m_ignoreCalc = false;
	bool m_stopRequested = false;
};


#endif
