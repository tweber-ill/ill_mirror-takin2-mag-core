/**
 * plotter
 * @author Tobias Weber <tweber@ill.fr>
 * @date 15-Jun-2018
 * @license see 'LICENSE' file
 */

#include "plot.h"

#include <QtWidgets/QGridLayout>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>


using t_real = t_real_dat;


Plotter::Plotter(QWidget *parent, QSettings* pSettings)
	: QWidget(parent), m_pSettings(pSettings), 
	m_pPlotter{std::make_shared<QCustomPlot>(this)}, 
	m_pPlotContextMenu{new QMenu(m_pPlotter.get())}

{
	auto *pGrid = new QGridLayout(this);
	pGrid->setHorizontalSpacing(4);
	pGrid->setVerticalSpacing(4);
	pGrid->setContentsMargins(0,0,0,0);
	pGrid->addWidget(m_pPlotter.get(), 0, 0, 1, 1);

	m_pPlotContextMenu->setTitle("Plot");
	m_pPlotContextMenu->addAction("Export to Gnuplot...", this, &Plotter::SaveGpl);
	m_pPlotContextMenu->addAction("Save as PDF...", this, &Plotter::SavePDF);

	m_pPlotter->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_pPlotter.get(), &QCustomPlot::customContextMenuRequested, this, &Plotter::ShowPlotContextMenu);

}


Plotter::~Plotter()
{
	Clear();
}


/**
 * show the context menu for the plotter
 */
void Plotter::ShowPlotContextMenu(const QPoint& pt)
{
	auto ptGlob = m_pPlotter->mapToGlobal(pt);
	ptGlob.setY(ptGlob.y() + 8);
	m_pPlotContextMenu->popup(ptGlob);
}


/**
 * save plot as PDF file
 */
void Plotter::SavePDF()
{
	QString dirLast = "";
	if(m_pSettings)
		m_pSettings->value("dir_pdf", "").toString();

	QString file = QFileDialog::getSaveFileName(this, "Save as PDF", dirLast, "PDF Files (*.pdf *.PDF)");
	if(file=="")
		return;

	if(!m_pPlotter->savePdf(file))
	{
		QMessageBox::critical(this, "PDF Export", "Could not save PDF file.");
		return;
	}

	if(m_pSettings)
		m_pSettings->setValue("dir_pdf", QFileInfo(file).path());
}


/**
 * export plot to Gnuplot
 */
void Plotter::SaveGpl()
{
	QString dirLast = "";
	if(m_pSettings)
		m_pSettings->value("dir_gpl", "").toString();

	QString file = QFileDialog::getSaveFileName(this, "Export to Gnuplot", dirLast, "Gnuplot Files (*.gpl *.GPL)");
	if(file=="")
		return;

	if(!m_dataset.SaveGpl(file.toStdString()))
	{
		QMessageBox::critical(this, "Gnuplot Export", "Could not save Gnuplot file.");
		return;
	}

	if(m_pSettings)
		m_pSettings->setValue("dir_gpl", QFileInfo(file).path());
}


void Plotter::Clear()
{
	m_pPlotter->clearGraphs();
	m_pPlotter->clearItems();
	m_pPlotter->clearPlottables();
	m_dataset.clear();
}


/**
 * show a dataset
 */
void Plotter::Plot(const Dataset &dataset)
{
	static const std::vector<unsigned> colors = {
		0xffff0000, 0xff0000ff, 0xff009900, 0xff000000,
	};

	Clear();
	m_dataset = dataset;

	t_real xmin = std::numeric_limits<t_real>::max();
	t_real xmax = -xmin;
	t_real ymin = std::numeric_limits<t_real>::max();
	t_real ymax = -xmin;

	bool labels_already_set = false;


	// iterate over (polarisation) channels
	for(std::size_t channel=0; channel<dataset.GetNumChannels(); ++channel)
	{
		const auto &data = dataset.GetChannel(channel);

		if(data.GetNumAxes()==0 || data.GetNumCounters()==0)
			continue;

		const auto &datx = data.GetAxis(0);
		const auto &daty = data.GetCounter(0);
		const auto &datyerr = data.GetCounterErrors(0);


		// graph
		auto *graph = m_pPlotter->addGraph();
		auto *graph_err = new QCPErrorBars(m_pPlotter->xAxis, m_pPlotter->yAxis);
		graph_err->setDataPlottable(graph);

		QPen pen = QPen(QColor(colors[channel % colors.size()]));
		QBrush brush(pen.color());
		t_real ptsize = 8;
		graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, pen, brush, ptsize));
		graph->setPen(pen);
		graph_err->setSymbolGap(ptsize);


		// convert to qvector
		QVector<t_real> _datx, _daty, _datyerr;
		std::copy(datx.begin(), datx.end(), std::back_inserter(_datx));
		std::copy(daty.begin(), daty.end(), std::back_inserter(_daty));
		std::copy(datyerr.begin(), datyerr.end(), std::back_inserter(_datyerr));
	
		graph->setData(_datx, _daty);
		graph_err->setData(_datyerr);


		// ranges
		auto xminmax = std::minmax_element(datx.begin(), datx.end());
		auto yminmax = std::minmax_element(daty.begin(), daty.end());
		auto yerrminmax = std::minmax_element(datyerr.begin(), datyerr.end());

		if(xminmax.first != datx.end() && xminmax.second != datx.end())
		{
			xmin = std::min(*xminmax.first, xmin);
			xmax = std::max(*xminmax.second, xmax);
		}
		if(yminmax.first != daty.end() && yminmax.second != daty.end())
		{
			ymin = std::min(*yminmax.first - *yerrminmax.first, ymin);
			ymax = std::max(*yminmax.second + *yerrminmax.second, ymax);
		}


		// labels
		if(!labels_already_set)
		{
			m_pPlotter->xAxis->setLabel(data.GetAxisName(channel).size() ? data.GetAxisName(channel).c_str() : "x");
			m_pPlotter->yAxis->setLabel("Counts");
			labels_already_set = true;
		}
	}


	m_pPlotter->xAxis->setRange(xmin, xmax);
	m_pPlotter->yAxis->setRange(ymin, ymax);

	m_pPlotter->replot();
}



// ----------------------------------------------------------------------------
// dock

PlotterDock::PlotterDock(QWidget* pParent, QSettings* pSettings)
	: QDockWidget(pParent), m_pPlot(std::make_unique<Plotter>(this, pSettings))
{
	this->setObjectName("plotter");
	this->setWindowTitle("Current Plot");
	this->setWidget(m_pPlot.get());
}

PlotterDock::~PlotterDock()
{
}

// ----------------------------------------------------------------------------