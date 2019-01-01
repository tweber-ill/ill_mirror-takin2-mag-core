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
#include <QtCore/QSettings>

#include <vector>
#include <sstream>
#include <complex>

using t_real = double;
using t_cplx = std::complex<t_real>;


template<class T = t_real>
class NumericTableWidgetItem : public QTableWidgetItem
{
public:
	NumericTableWidgetItem(T&& val)
		: QTableWidgetItem(std::to_string(std::forward<T>(val)).c_str())
	{}

	virtual bool operator<(const QTableWidgetItem& item) const override
	{
		T val1{}, val2{};
		std::istringstream{text().toStdString()} >> val1;
		std::istringstream{item.text().toStdString()} >> val2;

		return val1 < val2;
	}
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

	QSpinBox *m_maxBZ = nullptr;

	QMenu *m_pTabContextMenu = nullptr;

protected:
	void AddTabItem(int row = -1);
	void DelTabItem();
	void MoveTabItemUp();
	void MoveTabItemDown();

	void TableCurCellChanged(int rowNew, int colNew, int rowOld, int colOld);
	void TableCellEntered(const QModelIndex& idx);
	void TableItemChanged(QTableWidgetItem *item);
	void ShowTableContextMenu(const QPoint& pt);

	std::vector<NuclPos> GetNuclei() const;
	void Calc();

	virtual void closeEvent(QCloseEvent *evt) override;

private:
	int m_iCursorRow = -1;
	bool m_ignoreChanges = 1;

private:
	std::vector<int> GetSelectedRows(bool sort_reversed = false) const;
};


#endif
