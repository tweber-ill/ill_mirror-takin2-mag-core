/**
 * graph with weight factors
 * @author Tobias Weber <tweber@ill.fr>
 * @date Jan-2022
 * @license GPLv3, see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2023  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 * "misc" project
 * Copyright (C) 2017-2022  Tobias WEBER (privately developed).
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

#ifndef __MAG_DYN_GRAPH_H__
#define __MAG_DYN_GRAPH_H__

#include <qcustomplot.h>
#include <QtCore/QVector>

#include "tlibs2/libs/maths.h"


/**
 * a graph with weight factors per data point
 */
class GraphWithWeights : public QCPGraph
{
public:
	GraphWithWeights(QCPAxis *x, QCPAxis *y)
		: QCPGraph(x, y)
	{
		// can't use adaptive sampling because we have to match weights to data points
		setAdaptiveSampling(false);
	}


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


	void SetWeightScale(qreal sc, qreal min, qreal max)
	{
		m_weight_scale = sc;
		m_weight_min = min;
		m_weight_max = max;
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
		const int num_points = points.size();
		if(!num_points)
			return;

		const qreal eps = 1e-3;

		// find the absolute data start index for indexing the weight data
		// (more elegant would be if we could get the indices for the data to be drawn directly from qcp)
		int data_start_idx = -1;
		for(auto iter = mDataContainer->constBegin(); iter != mDataContainer->constEnd(); ++iter)
		{
			qreal pt_x = keyAxis()->coordToPixel(iter->mainKey());
			qreal pt_y = valueAxis()->coordToPixel(iter->mainValue());

			if(tl2::equals(pt_x, points[0].x(), eps) &&
				tl2::equals(pt_y, points[0].y(), eps))
			{
				data_start_idx = iter - mDataContainer->constBegin();
				break;
			}
		}

		// need to overwrite point size
		QCPScatterStyle& style = const_cast<QCPScatterStyle&>(_style);

		// see: QCPGraph::drawScatterPlot
		QCPGraph::applyScattersAntialiasingHint(paint);
		style.applyTo(paint, pen());

		const bool has_weights = (data_start_idx >= 0 && m_weights.size()-data_start_idx >= num_points);
		const qreal size_saved = style.size();

		// iterate all data points
		for(int idx=0; idx<num_points; ++idx)
		{
			// data point and its weight factor
			const QPointF& pt = points[idx];
			qreal weight = has_weights ? m_weights[idx + data_start_idx] : size_saved;
			qreal scaled_weight = weight * m_weight_scale;
			if(m_weight_max >= 0. && m_weight_min >= 0. && m_weight_min <= m_weight_max)
				scaled_weight = tl2::clamp(scaled_weight, m_weight_min, m_weight_max);

			// set symbol sizes per point
			style.setSize(scaled_weight);

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

	qreal m_weight_scale = 1;
	qreal m_weight_min = -1;
	qreal m_weight_max = -1;
};


#endif
