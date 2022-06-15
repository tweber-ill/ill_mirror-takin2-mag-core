/**
 * brillouin zone tool
 * @author Tobias Weber <tweber@ill.fr>
 * @date May-2022
 * @license GPLv3, see 'LICENSE' file
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2022  Tobias WEBER (Institut Laue-Langevin (ILL),
 *                          Grenoble, France).
 * "misc" project
 * Copyright (C) 2017-2021  Tobias WEBER (privately developed).
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

#include "plot_cut.h"
#include <QtWidgets/QApplication>


// --------------------------------------------------------------------------------
BZCutScene::BZCutScene(QWidget *parent) : QGraphicsScene(parent)
{
}


BZCutScene::~BZCutScene()
{
}


void BZCutScene::AddCut(
	const std::vector<std::tuple<t_vec, t_vec, std::array<t_real, 3>>>& lines)
{
	// (000) brillouin zone
	std::vector<const std::tuple<t_vec, t_vec, std::array<t_real, 3>>*> lines000;

	QPen pen;
	pen.setCosmetic(true);
	pen.setColor(qApp->palette().color(QPalette::WindowText));
	pen.setWidthF(1.);

	// draw brillouin zones
	for(const auto& line : lines)
	{
		const auto& Q = std::get<2>(line);

		// (000) BZ?
		if(tl2::equals_0(Q[0], g_eps) &&
			tl2::equals_0(Q[1], g_eps) &&
			tl2::equals_0(Q[2], g_eps))
		{
			lines000.push_back(&line);
			continue;
		}

		addLine(QLineF(
			std::get<0>(line)[0]*m_scale, std::get<0>(line)[1]*m_scale,
			std::get<1>(line)[0]*m_scale, std::get<1>(line)[1]*m_scale),
			pen);
	}


	// draw (000) brillouin zone
	pen.setColor(QColor(0xff, 0x00, 0x00));
	pen.setWidthF(2.);

	for(const auto* line : lines000)
	{
		addLine(QLineF(
			std::get<0>(*line)[0]*m_scale, std::get<0>(*line)[1]*m_scale,
			std::get<1>(*line)[0]*m_scale, std::get<1>(*line)[1]*m_scale),
			pen);
	}
}
// --------------------------------------------------------------------------------



// --------------------------------------------------------------------------------
BZCutView::BZCutView(BZCutScene* scene)
	: QGraphicsView(scene, static_cast<QWidget*>(scene->parent())),
	m_scene(scene)
{
	setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
	setDragMode(QGraphicsView::ScrollHandDrag);
	setInteractive(true);
	setMouseTracking(true);
	scale(1., -1.);
}


BZCutView::~BZCutView()
{
}


void BZCutView::mouseMoveEvent(QMouseEvent *evt)
{
	QPointF pos = mapToScene(evt->pos());
	t_real scale = m_scene->GetScale();
	emit SignalMouseCoordinates(pos.x()/scale, pos.y()/scale);

	QGraphicsView::mouseMoveEvent(evt);
}


void BZCutView::wheelEvent(QWheelEvent *evt)
{
	t_real sc = std::pow(2., evt->angleDelta().y()/8.*0.01);
	QGraphicsView::scale(sc, sc);
}
// --------------------------------------------------------------------------------
