/**
 * structure factor tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date Dec-2018
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 28-Dec-2018 from the privately developed "misc" project (https://github.com/t-weber/misc).
 */

#ifndef __SFACT_H__
#define __SFACT_H__

#include <QtWidgets/QDialog>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QMenu>
#include <QtWidgets/QLabel>
#include <QtWidgets/QComboBox>
#include <QtCore/QSettings>

#include <vector>
#include <sstream>
#include <complex>

#include "tools/glplot/glplot.h"
#include "libs/_cxx20/math_algos.h"


using t_real = double;
using t_cplx = std::complex<t_real>;
using t_vec = std::vector<t_real>;
using t_vec_cplx = std::vector<t_cplx>;
using t_mat = m::mat<t_real, std::vector>;
using t_mat_cplx = m::mat<t_cplx, std::vector>;



template<class T = t_real>
class NumericTableWidgetItem : public QTableWidgetItem
{
public:
	NumericTableWidgetItem(T&& val)
		: QTableWidgetItem(std::to_string(std::forward<T>(val)).c_str())
	{}
	NumericTableWidgetItem(const T& val)
		: QTableWidgetItem(std::to_string(val).c_str())
	{}

	NumericTableWidgetItem(const QString& val) : QTableWidgetItem(val)
	{}

	virtual bool operator<(const QTableWidgetItem& item) const override
	{
		T val1{}, val2{};
		std::istringstream{text().toStdString()} >> val1;
		std::istringstream{item.text().toStdString()} >> val2;

		return val1 < val2;
	}

	virtual QTableWidgetItem* clone() const override
	{
		auto item = new NumericTableWidgetItem<T>(this->text());
		item->setData(Qt::UserRole, this->data(Qt::UserRole));
		return item;
	};
};


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

	QDialog *m_dlgPlot = nullptr;
	std::shared_ptr<GlPlot> m_plot;
	std::size_t m_sphere = 0;
	QLabel *m_labelGlInfos[4] = { nullptr, nullptr, nullptr, nullptr };
	QLabel *m_status3D = nullptr;

	QWidget *m_nucleipanel = nullptr;
	QTableWidget *m_nuclei = nullptr;
	QPlainTextEdit *m_structfacts = nullptr;
	QPlainTextEdit *m_powderlines = nullptr;

	QLineEdit *m_editA = nullptr;
	QLineEdit *m_editB = nullptr;
	QLineEdit *m_editC = nullptr;
	QLineEdit *m_editAlpha = nullptr;
	QLineEdit *m_editBeta = nullptr;
	QLineEdit *m_editGamma = nullptr;

	QComboBox *m_comboSG = nullptr;
	std::vector<std::vector<t_mat>> m_SGops;

	QSpinBox *m_maxBZ = nullptr;

	QMenu *m_pTabContextMenu = nullptr;			// menu in case a nucleus is selected
	QMenu *m_pTabContextMenuNoItem = nullptr;	// menu if nothing is selected

protected:
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
	void GenerateFromSG();

	std::vector<NuclPos> GetNuclei() const;
	void Calc();

	void PlotMouseDown(bool left, bool mid, bool right);
	void PlotMouseUp(bool left, bool mid, bool right);
	void PickerIntersection(const t_vec3_gl* pos, std::size_t objIdx, const t_vec3_gl* posSphere);
	void AfterGLInitialisation();

	virtual void closeEvent(QCloseEvent *evt) override;

private:
	int m_iCursorRow = -1;
	bool m_ignoreChanges = 1;

	long m_curPickedObj = -1;

private:
	std::vector<int> GetSelectedRows(bool sort_reversed = false) const;
};


#endif
