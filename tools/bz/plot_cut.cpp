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


void BZCutScene::AddCut(const std::vector<std::pair<t_vec, t_vec>>& lines)
{
	for(const auto& line : lines)
	{
		QPen pen;
		pen.setCosmetic(true);
		pen.setColor(qApp->palette().color(QPalette::WindowText));

		addLine(QLineF(
			line.first[0]*m_scale, line.first[1]*m_scale,
			line.second[0]*m_scale, line.second[1]*m_scale),
			pen);
	}
}
// --------------------------------------------------------------------------------



// --------------------------------------------------------------------------------
BZCutView::BZCutView(BZCutScene* scene)
	: QGraphicsView(scene, static_cast<QWidget*>(scene->parent()))
{
	setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
	setDragMode(QGraphicsView::ScrollHandDrag);
	setInteractive(true);
	setMouseTracking(true);
}


BZCutView::~BZCutView()
{
}


void BZCutView::wheelEvent(QWheelEvent *evt)
{
	t_real sc = std::pow(2., evt->angleDelta().y()/8.*0.01);
	QGraphicsView::scale(sc, sc);
}
// --------------------------------------------------------------------------------
