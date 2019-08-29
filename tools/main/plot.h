/**
 * plotter
 * @author Tobias Weber <tweber@ill.fr>
 * @date 15-Jun-2018
 * @license see 'LICENSE' file
 */

#ifndef __PLOT_H__
#define __PLOT_H__

#include <QtCore/QSettings>
#include <QtWidgets/QWidget>
#include <QtWidgets/QMenu>

#include "qcp/qcustomplot.h"

#include "data.h"



class Plotter : public QWidget
{
private:
	QSettings *m_pSettings = nullptr;

	QCustomPlot *m_pPlotter = new QCustomPlot(this);
	QMenu *m_pPlotContextMenu = new QMenu(m_pPlotter);

	// current dataset
	const Dataset* m_pdataset = nullptr;

public:
	Plotter(QWidget *parent, QSettings* = nullptr);
	virtual ~Plotter();

	QCustomPlot* GetPlotter() { return m_pPlotter; }
	const QCustomPlot* GetPlotter() const { return m_pPlotter; }

	void Plot(const Dataset &dataset);
	void Clear();

	void ShowPlotContextMenu(const QPoint& pt);

	void SavePDF();
	void SaveGpl();
};



/**
 * the dock which contains the plotter
 */
class PlotterDock : public QDockWidget
{
private:
	std::unique_ptr<Plotter> m_pPlot;

public:
	PlotterDock(QWidget* pParent = nullptr, QSettings* = nullptr);
	virtual ~PlotterDock();

	const Plotter* GetWidget() const { return m_pPlot.get(); }
	Plotter* GetWidget() { return m_pPlot.get(); }
};


#endif
