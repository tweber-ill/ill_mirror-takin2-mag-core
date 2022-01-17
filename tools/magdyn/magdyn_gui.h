/**
 * magnon dynamics
 * @author Tobias Weber <tweber@ill.fr>
 * @date Jan-2022
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 28-Dec-2018 from my privately developed "misc" project (https://github.com/t-weber/misc).
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2021  Tobias WEBER (Institut Laue-Langevin (ILL),
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

#ifndef __MAG_DYN_GUI_H__
#define __MAG_DYN_GUI_H__


#include <QtCore/QSettings>
#include <QtWidgets/QDialog>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QAction>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>

#include <qcustomplot.h>

#include <vector>
#include <sstream>

#include "tlibs2/libs/maths.h"
#include "tlibs2/libs/qt/numerictablewidgetitem.h"

#include "magdyn.h"



class MagDynDlg : public QDialog
{
public:
	MagDynDlg(QWidget* pParent = nullptr);
	virtual ~MagDynDlg() = default;

	MagDynDlg(const MagDynDlg&) = delete;
	const MagDynDlg& operator=(const MagDynDlg&) = delete;


protected:
	QSettings *m_sett{};
	QMenuBar *m_menu{};
	QLabel *m_status{};

	QAction *m_use_dmi{};
	QAction *m_use_field{};

	QWidget *m_sitespanel{};
	QWidget *m_termspanel{};
	QWidget *m_fieldpanel{};
	QWidget *m_hamiltonianpanel{};

	QTableWidget *m_sitestab{};
	QTableWidget *m_termstab{};

	QWidget *m_disppanel{};
	QDoubleSpinBox *m_spin_q_start[3]{nullptr, nullptr, nullptr};
	QDoubleSpinBox *m_spin_q_end[3]{nullptr, nullptr, nullptr};
	QSpinBox *m_num_points{};

	QTextEdit *m_hamiltonian{};
	QDoubleSpinBox *m_spin_q[3]{nullptr, nullptr, nullptr};

	MagDyn m_dyn{};
	QCustomPlot *m_plot{};


protected:
	// general table operations
	void MoveTabItemUp(QTableWidget *pTab);
	void MoveTabItemDown(QTableWidget *pTab);
	void ShowTableContextMenu(QTableWidget *pTab, QMenu *pMenu, QMenu *pMenuNoItem, const QPoint& pt);
	std::vector<int> GetSelectedRows(QTableWidget *pTab, bool sort_reversed = false) const;

	void AddSiteTabItem(int row=-1,
		const std::string& name="n/a",
		t_real x=0., t_real y=0., t_real z=0.,
		t_real sx=0., t_real sy=0., t_real sz=1.);

	void AddTermTabItem(int row=-1,
		const std::string& name="n/a",
		t_size atom_1=0, t_size atom_2=0,
		t_real dist_x=0., t_real dist_y=0., t_real dist_z=0.,
		t_real J=0.,
		t_real dmi_x=0., t_real dmi_y=0., t_real dmi_z=0.);

	void DelTabItem(QTableWidget *pTab, int begin=-2, int end=-2);

	void SitesTableItemChanged(QTableWidgetItem *item);
	void TermsTableItemChanged(QTableWidgetItem *item);

	void Load();
	void Save();
	void SavePlotFigure();

	void CalcSitesAndTerms();
	void CalcDispersion();
	void CalcHamiltonian();

	void PlotMouseMove(QMouseEvent* evt);
	virtual void closeEvent(QCloseEvent *evt) override;


private:
	int m_sites_cursor_row = -1;
	int m_terms_cursor_row = -1;

	bool m_ignoreChanges = 1;
	bool m_ignoreCalc = 0;
};


#endif
