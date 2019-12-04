/**
 * atom dynamics
 * @author Tobias Weber <tweber@ill.fr>
 * @date Dec-2019
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __MOLDYN_GUI_H__
#define __MOLDYN_GUI_H__

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QDialog>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QMenu>
#include <QtWidgets/QLabel>
#include <QtCore/QSettings>

#include <vector>
#include <sstream>

#include "libs/_cxx20/glplot.h"
#include "libs/_cxx20/math_algos.h"


using t_real = double;
using t_vec = std::vector<t_real>;
using t_mat = m::mat<t_real, std::vector>;


class MolDynDlg : public QMainWindow
{
public:
	MolDynDlg(QWidget* pParent = nullptr);
	~MolDynDlg() = default;

protected:
	QSettings *m_sett = nullptr;
	QMenuBar *m_menu = nullptr;
	QStatusBar *m_status = nullptr;

	GlPlot *m_plot = nullptr;
	std::size_t m_sphere = 0;


protected:
	void Add3DItem(int row=-1);
	void Set3DStatusMsg(const std::string& msg);

	void New();
	void Load();
	void Save();

	void PlotMouseDown(bool left, bool mid, bool right);
	void PlotMouseUp(bool left, bool mid, bool right);
	void PickerIntersection(const t_vec3_gl* pos, std::size_t objIdx, const t_vec3_gl* posSphere);
	void AfterGLInitialisation();

	virtual void closeEvent(QCloseEvent *evt) override;

private:
	long m_curPickedObj = -1;
	bool m_ignoreChanges = 1;
};


#endif
