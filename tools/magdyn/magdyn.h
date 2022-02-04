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
#include <QtWidgets/QDialog>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QDoubleSpinBox>
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

using namespace tl2_mag;

using t_real_gl = tl2::t_real_gl;
using t_vec2_gl = tl2::t_vec2_gl;
using t_vec3_gl = tl2::t_vec3_gl;
using t_vec_gl = tl2::t_vec_gl;
using t_mat_gl = tl2::t_mat_gl;


extern t_real g_eps;
extern int g_prec;
extern int g_prec_gui;


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
	QLabel *m_status{};

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

	// tab
	QTabWidget *m_tabs{};

	// panels
	QWidget *m_sitespanel{};
	QWidget *m_termspanel{};
	QWidget *m_samplepanel{};
	QWidget *m_disppanel{};
	QWidget *m_hamiltonianpanel{};

	// tables
	QTableWidget *m_sitestab{};
	QTableWidget *m_termstab{};

	//sites
	QComboBox *m_comboSG{};
	std::vector<std::vector<t_mat_real>> m_SGops{};

	// dispersion
	QCustomPlot *m_plot{};
	QDoubleSpinBox *m_q_start[3]{nullptr, nullptr, nullptr};
	QDoubleSpinBox *m_q_end[3]{nullptr, nullptr, nullptr};
	QSpinBox *m_num_points{};
	QDoubleSpinBox *m_weight_scale{};

	// hamiltonian
	QTextEdit *m_hamiltonian{};
	QDoubleSpinBox *m_q[3]{nullptr, nullptr, nullptr};

	// external magnetic field
	QDoubleSpinBox *m_field_dir[3]{nullptr, nullptr, nullptr};
	QDoubleSpinBox *m_field_mag{};
	QCheckBox *m_align_spins{};
	QDoubleSpinBox *m_rot_axis[3]{nullptr, nullptr, nullptr};
	QDoubleSpinBox *m_rot_angle{};

	// bragg peak
	QDoubleSpinBox *m_bragg[3]{nullptr, nullptr, nullptr};

	// temperature
	QDoubleSpinBox *m_temperature{};

	// magnon dynamics calculator
	MagDyn m_dyn{};

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
	std::unordered_map<std::size_t, const tl2_mag::AtomSite*>
		m_structplot_atoms{};
	std::unordered_map<std::size_t, const tl2_mag::ExchangeTerm*>
		m_structplot_terms{};
	std::optional<std::size_t> m_structplot_cur_obj{};
	std::optional<std::size_t> m_structplot_cur_atom{};
	std::optional<std::size_t> m_structplot_cur_term{};


protected:
	// general table operations
	void MoveTabItemUp(QTableWidget *pTab);
	void MoveTabItemDown(QTableWidget *pTab);
	void ShowTableContextMenu(
		QTableWidget *pTab, QMenu *pMenu, QMenu *pMenuNoItem, const QPoint& pt);
	std::vector<int> GetSelectedRows(
		QTableWidget *pTab, bool sort_reversed = false) const;

	void AddSiteTabItem(int row=-1,
		const std::string& name="n/a",
		t_real x=0., t_real y=0., t_real z=0.,
		t_real sx=0., t_real sy=0., t_real sz=1.,
		t_real S=1.);

	void AddTermTabItem(int row=-1,
		const std::string& name="n/a",
		t_size atom_1=0, t_size atom_2=0,
		t_real dist_x=0., t_real dist_y=0., t_real dist_z=0.,
		t_real J=0.,
		t_real dmi_x=0., t_real dmi_y=0., t_real dmi_z=0.);

	void DelTabItem(QTableWidget *pTab, int begin=-2, int end=-2);

	void SitesTableItemChanged(QTableWidgetItem *item);
	void TermsTableItemChanged(QTableWidgetItem *item);

	void Clear();
	void Load();
	void Save();

	bool Load(const QString& filename);
	bool Save(const QString& filename);

	void SavePlotFigure();
	void SaveDispersion();

	void RotateField(bool ccw = true);
	void GenerateFromSG();

	void SyncSitesAndTerms();
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


private:
	int m_sites_cursor_row = -1;
	int m_terms_cursor_row = -1;

	bool m_ignoreTableChanges = 1;
	bool m_ignoreCalc = 0;
};


#endif
