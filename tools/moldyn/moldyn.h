/**
 * atom dynamics
 * @author Tobias Weber <tweber@ill.fr>
 * @date Dec-2019
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __MOLDYN_GUI_H__
#define __MOLDYN_GUI_H__

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QSlider>
#include <QtWidgets/QDoubleSpinBox>
#include <QtCore/QSettings>

#include <vector>
#include <tuple>

#include "libs/_cxx20/glplot.h"
#include "libs/_cxx20/math_algos.h"

#include "moldyn-loader.h"


using t_real = double;
using t_vec = std::vector<t_real>;
using t_mat = m::mat<t_real, std::vector>;


class MolDynDlg : public QMainWindow
{
public:
	MolDynDlg(QWidget* pParent = nullptr);
	~MolDynDlg() = default;

protected:
	std::size_t Add3DItem(const t_vec& vec, const t_vec& col, t_real scale, const std::string& label);
	void Change3DItem(std::size_t obj, const t_vec* vec, const t_vec* col=nullptr, const t_real *scale=nullptr, const std::string *label=nullptr);

	void SetStatusMsg(const std::string& msg);

	void New();
	void Load();
	void SaveAs();

	void PlotMouseDown(bool left, bool mid, bool right);
	void PlotMouseUp(bool left, bool mid, bool right);
	void PlotMouseClick(bool left, bool mid, bool right);
	void PickerIntersection(const t_vec3_gl* pos, std::size_t objIdx, const t_vec3_gl* posSphere);
	void AfterGLInitialisation();

	void SliderValueChanged(int val);
	void SelectNone();

	std::tuple<bool, std::size_t, std::size_t, std::size_t> GetAtomIndexFromHandle(std::size_t handle) const;
	void CalculateDistanceBetweenAtoms();

	void DeleteAtomUnderCursor();
	void DeleteAllAtomsOfSameType();
	void KeepAtomsOfSameType();

	virtual void closeEvent(QCloseEvent *evt) override;
	virtual void keyPressEvent(QKeyEvent *evt) override;


protected:
	MolDyn<t_real, t_vec> m_mol;
	t_mat m_crystA = m::unit<t_mat>(3);
	t_mat m_crystB = m::unit<t_mat>(3);

	QSettings *m_sett = nullptr;
	QMenuBar *m_menu = nullptr;
	QStatusBar *m_status = nullptr;
	QSlider *m_slider = nullptr;
	QDoubleSpinBox *m_spinScale = nullptr;
	QMenu *m_atomContextMenu = nullptr;

	GlPlot *m_plot = nullptr;
	std::size_t m_sphere = 0;
	std::vector<std::size_t> m_sphereHandles;


private:
	long m_curPickedObj = -1;
	bool m_ignoreChanges = 1;
};


#endif
