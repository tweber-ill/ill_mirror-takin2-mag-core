/**
 * graph with weight factors
 * @author Tobias Weber <tweber@ill.fr>
 * @date Jan-2022
 * @license GPLv3, see 'LICENSE' file
 */

#ifndef __MAG_DYN_GRAPH_H__
#define __MAG_DYN_GRAPH_H__

#include <qcustomplot.h>
#include <QtCore/QVector>


/**
 * a graph with weight factors per data point
 */
class GraphWithWeights : public QCPGraph
{
public:
	GraphWithWeights(QCPAxis *x, QCPAxis *y)
		: QCPGraph(x, y)
	{}


	virtual ~GraphWithWeights()
	{}


	/**
	 * sets the symbol sizes
	 * setData() needs to be called with already_sorted=true,
	 * otherwise points and weights don't match
	 */
	void SetWeights(const QVector<qreal>& weights)
	{
		m_weights = weights;
	}


	/**
	 * scatter plot with variable symbol sizes
	 */
	virtual void drawScatterPlot(
		QCPPainter* paint,
		const QVector<QPointF>& points,
		const QCPScatterStyle& _style) const override
	{
		//QCPGraph::drawScatterPlot(paint, points, _style);

		// need to overwrite point size
		QCPScatterStyle& style = const_cast<QCPScatterStyle&>(_style);

		// see: QCPGraph::drawScatterPlot
		QCPGraph::applyScattersAntialiasingHint(paint);
		style.applyTo(paint, pen());

		const int num_points = points.size();
		const bool has_weights = (m_weights.size() == num_points);
		const qreal size_saved = style.size();

		// iterate all data points
		for(int idx=0; idx<num_points; ++idx)
		{
			// data point and its weight factor
			const QPointF& pt = points[idx];
			qreal weight = has_weights ? m_weights[idx] : size_saved;

			// set symbol sizes per point
			style.setSize(weight);

			// draw the symbol with the modified size
			style.drawShape(paint, pt);
			//paint->drawEllipse(pt, weight, weight);
		}

		// restore original symbol size
		style.setSize(size_saved);
	}


private:
	// symbol sizes
	QVector<qreal> m_weights{};
};


#endif
