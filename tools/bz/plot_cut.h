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

#ifndef __BZCUT_H__
#define __BZCUT_H__

#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsItem>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>

#include <tuple>
#include <array>

#include "globals.h"



class BZCutScene : public QGraphicsScene
{
public:
	BZCutScene(QWidget *parent = nullptr);
	virtual ~BZCutScene();

	void AddCut(const std::vector<
		// [x, y, Q]
		std::tuple<t_vec, t_vec, std::array<t_real, 3>>>& lines);
	void AddCurve(const std::vector<t_vec>& points);

	t_real GetScale() const { return m_scale; }

	void ClearAll();
	void ClearCut();
	void ClearCurves();


protected:
	t_real m_scale = 100.;

	std::vector<QGraphicsItem*> m_bzcut{};
	std::vector<QGraphicsItem*> m_curves{};
};



class BZCutView : public QGraphicsView
{ Q_OBJECT
public:
	BZCutView(BZCutScene* scene);
	virtual ~BZCutView();


protected:
	virtual void mouseMoveEvent(QMouseEvent *evt) override;
	virtual void wheelEvent(QWheelEvent *evt) override;


signals:
	void SignalMouseCoordinates(t_real x, t_real y);


protected:
	BZCutScene* m_scene = nullptr;
};


#endif
