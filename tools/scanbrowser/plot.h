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
#include <memory>

#include "qcustomplot.h"

#include "data.h"



class Plotter : public QWidget
{
private:
	QSettings *m_pSettings = nullptr;

	std::shared_ptr<QCustomPlot> m_pPlotter;
	QMenu *m_pPlotContextMenu = nullptr;

	// copy of current dataset
	Dataset m_dataset;

public:
	Plotter(QWidget *parent, QSettings* = nullptr);
	virtual ~Plotter();

	std::shared_ptr<QCustomPlot> GetPlotter() { return m_pPlotter; }
	std::shared_ptr<QCustomPlot> GetPlotter() const { return m_pPlotter; }

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
