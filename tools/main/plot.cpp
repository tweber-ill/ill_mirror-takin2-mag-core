/**
 * plotter
 * @author Tobias Weber <tweber@ill.fr>
 * @date 15-Jun-2018
 * @license see 'LICENSE' file
 */

#include "plot.h"
#include <QtWidgets/QGridLayout>


using t_real = t_real_dat;


Plotter::Plotter(QWidget *parent) : QWidget(parent)
{
	auto *pGrid = new QGridLayout(this);
	pGrid->setHorizontalSpacing(4);
	pGrid->setVerticalSpacing(4);
	pGrid->setContentsMargins(0,0,0,0);
	pGrid->addWidget(m_pPlotter, 0, 0, 1, 1);
}


Plotter::~Plotter()
{
	Clear();
}


void Plotter::Clear()
{
	m_pPlotter->clearGraphs();
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

PlotterDock::PlotterDock(QWidget* pParent)
	: QDockWidget(pParent), m_pPlot(std::make_unique<Plotter>(this))
{
	this->setObjectName("plotter");
	this->setWindowTitle("Current Plot");
	this->setWidget(m_pPlot.get());
}

PlotterDock::~PlotterDock()
{
}

// ----------------------------------------------------------------------------