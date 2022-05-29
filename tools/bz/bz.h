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
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QLabel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCheckBox>
#include <QtCore/QSettings>

#include <vector>
#include <sstream>

#include <boost/math/quaternion.hpp>
namespace math = boost::math;

#include "tlibs2/libs/qt/glplot.h"
#include "tlibs2/libs/math20.h"
#include "tlibs2/libs/qt/numerictablewidgetitem.h"


using t_real = double;
using t_vec = tl2::vec<t_real, std::vector>;
using t_mat = tl2::mat<t_real, std::vector>;
using t_quat = math::quaternion<t_real>;

using t_real_gl = tl2::t_real_gl;
using t_vec2_gl = tl2::t_vec2_gl;
using t_vec3_gl = tl2::t_vec3_gl;
using t_vec_gl = tl2::t_vec_gl;
using t_mat_gl = tl2::t_mat_gl;


constexpr t_real g_eps = 1e-6;
constexpr int g_prec = 6;


enum : int
{
	COL_OP = 0,

	NUM_COLS
};


class BZDlg : public QDialog
{
public:
	BZDlg(QWidget* pParent = nullptr);
	~BZDlg() = default;


protected:
	QSettings *m_sett = nullptr;
	QMenuBar *m_menu = nullptr;

	QDialog *m_dlgPlot = nullptr;
	std::shared_ptr<tl2::GlPlot> m_plot;
	std::size_t m_sphere = 0;
	QLabel *m_labelGlInfos[4] = { nullptr, nullptr, nullptr, nullptr };
	QLabel *m_status3D = nullptr;

	QTableWidget *m_symops = nullptr;
	QPlainTextEdit *m_bz = nullptr;

	QLineEdit *m_editA = nullptr;
	QLineEdit *m_editB = nullptr;
	QLineEdit *m_editC = nullptr;
	QLineEdit *m_editAlpha = nullptr;
	QLineEdit *m_editBeta = nullptr;
	QLineEdit *m_editGamma = nullptr;

	QComboBox *m_comboSG = nullptr;
	QSpinBox *m_maxBZ = nullptr;

	QMenu *m_pTabContextMenu = nullptr;        // menu in case a symop is selected
	QMenu *m_pTabContextMenuNoItem = nullptr;  // menu if nothing is selected

	t_mat m_crystA = tl2::unit<t_mat>(3);
	t_mat m_crystB = tl2::unit<t_mat>(3);

	std::vector<std::vector<t_mat>> m_SGops;


protected:
	// for space group / symops tab
	void AddTabItem(int row = -1, const t_mat& op = tl2::unit<t_mat>(4));
	void DelTabItem(int begin=-2, int end=-2);
	void MoveTabItemUp();
	void MoveTabItemDown();

	void TableCurCellChanged(int rowNew, int colNew, int rowOld, int colOld);
	void TableCellEntered(const QModelIndex& idx);
	void TableItemChanged(QTableWidgetItem *item);
	void ShowTableContextMenu(const QPoint& pt);

	void Load();
	void Save();
	void ImportCIF();
	void GetSymOpsFromSG();

	std::vector<t_mat> GetSymOps(bool only_centring = false) const;
	void CalcB(bool bFullRecalc=true);
	void CalcBZ();

	void ClearPlot();
	void PlotAddVoronoiVertex(const t_vec& pos);
	void PlotAddBraggPeak(const t_vec& pos);
	void PlotAddPlane(const std::vector<t_vec>& vecs);
	void Set3DStatusMsg(const std::string& msg);

	void ShowBZPlot();
	void PlotMouseDown(bool left, bool mid, bool right);
	void PlotMouseUp(bool left, bool mid, bool right);
	void PickerIntersection(const t_vec3_gl* pos,
		std::size_t objIdx, const t_vec3_gl* posSphere);
	void AfterGLInitialisation();

	virtual void closeEvent(QCloseEvent *evt) override;

	// conversion functions
	static std::string OpToStr(const t_mat& rot);
	static t_mat StrToOp(const std::string& str);


private:
	int m_iCursorRow = -1;
	bool m_ignoreChanges = 1;
	bool m_ignoreCalc = 0;

	long m_curPickedObj = -1;
	std::vector<std::size_t> m_plotObjs;


private:
	std::vector<int> GetSelectedRows(bool sort_reversed = false) const;
};


#endif
