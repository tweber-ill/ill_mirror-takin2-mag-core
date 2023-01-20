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

#include <QtWidgets/QGridLayout>

#include "bz.h"

#include "tlibs2/libs/phys.h"
#include "tlibs2/libs/algos.h"

using namespace tl2_ops;


void BZDlg::ShowBZPlot()
{
	// plot widget
	if(!m_dlgPlot)
	{
		m_dlgPlot = new QDialog(this);
		m_dlgPlot->setWindowTitle("Brillouin Zone - 3D View");
		m_dlgPlot->setFont(this->font());

		m_plot = std::make_shared<tl2::GlPlot>(this);
		m_plot->GetRenderer()->SetRestrictCamTheta(false);
		m_plot->GetRenderer()->SetCull(false);
		m_plot->GetRenderer()->SetBlend(true);
		m_plot->GetRenderer()->SetLight(0, tl2::create<t_vec3_gl>({ 5, 5, 5 }));
		m_plot->GetRenderer()->SetLight(1, tl2::create<t_vec3_gl>({ -5, -5, -5 }));
		m_plot->GetRenderer()->SetCoordMax(1.);
		m_plot->GetRenderer()->GetCamera().SetDist(2.);
		m_plot->GetRenderer()->GetCamera().UpdateTransformation();

		//auto labCoordSys = new QLabel("Coordinate System:", this);
		//auto comboCoordSys = new QComboBox(this);
		//comboCoordSys->addItem("Fractional Units (rlu)");
		//comboCoordSys->addItem("Lab Units (A)");

		m_plot_coordcross = new QCheckBox("Show Coordinates", this);
		m_plot_labels = new QCheckBox("Show Labels", this);
		m_plot_plane = new QCheckBox("Show Plane", this);
		m_plot_coordcross->setChecked(true);
		m_plot_labels->setChecked(true);
		m_plot_plane->setChecked(true);

		// status bar
		m_status3D = new QLabel(this);
		m_status3D->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
		m_status3D->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

		m_plot->setSizePolicy(QSizePolicy{QSizePolicy::Expanding, QSizePolicy::Expanding});
		//labCoordSys->setSizePolicy(QSizePolicy{QSizePolicy::Fixed, QSizePolicy::Fixed});

		auto grid = new QGridLayout(m_dlgPlot);
		grid->setSpacing(2);
		grid->setContentsMargins(4,4,4,4);
		grid->addWidget(m_plot.get(), 0,0,1,3);
		//grid->addWidget(labCoordSys, 1,0,1,1);
		//grid->addWidget(comboCoordSys, 1,1,1,1);
		grid->addWidget(m_plot_coordcross, 1,0,1,1);
		grid->addWidget(m_plot_labels, 1,1,1,1);
		grid->addWidget(m_plot_plane, 1,2,1,1);
		grid->addWidget(m_status3D, 2,0,1,3);

		connect(m_plot.get(), &tl2::GlPlot::AfterGLInitialisation, this, &BZDlg::AfterGLInitialisation);
		connect(m_plot->GetRenderer(), &tl2::GlPlotRenderer::PickerIntersection, this, &BZDlg::PickerIntersection);
		connect(m_plot.get(), &tl2::GlPlot::MouseDown, this, &BZDlg::PlotMouseDown);
		connect(m_plot.get(), &tl2::GlPlot::MouseUp, this, &BZDlg::PlotMouseUp);
		connect(m_plot_coordcross, &QCheckBox::toggled, this, &BZDlg::PlotShowCoordCross);
		connect(m_plot_labels, &QCheckBox::toggled, this, &BZDlg::PlotShowLabels);
		connect(m_plot_plane, &QCheckBox::toggled, this, &BZDlg::PlotShowPlane);
		/*connect(comboCoordSys, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int val)
		{
			if(this->m_plot)
				this->m_plot->GetRenderer()->SetCoordSys(val);
		});*/


		if(m_sett && m_sett->contains("geo_3dview"))
			m_dlgPlot->restoreGeometry(m_sett->value("geo_3dview").toByteArray());
		else
			m_dlgPlot->resize(500,500);
	}

	m_dlgPlot->show();
	m_dlgPlot->raise();
	m_dlgPlot->focusWidget();
}


/**
 * show or hide the coordinate system
 */
void BZDlg::PlotShowCoordCross(bool show)
{
	if(!m_plot) return;

	if(auto obj = m_plot->GetRenderer()->GetCoordCross(); obj)
	{
		m_plot->GetRenderer()->SetObjectVisible(*obj, show);
		m_plot->update();
	}
}


/**
 * show or hide the object labels
 */
void BZDlg::PlotShowLabels(bool show)
{
	if(!m_plot) return;

	m_plot->GetRenderer()->SetLabelsVisible(show);
	m_plot->update();
}


/**
 * show or hide the BZ cut plane
 */
void BZDlg::PlotShowPlane(bool show)
{
	if(!m_plot) return;

	m_plot->GetRenderer()->SetObjectVisible(m_plane, show);
	m_plot->update();
}


/**
 * add a voronoi vertex to the plot
 */
void BZDlg::PlotAddVoronoiVertex(const t_vec& pos)
{
	if(!m_plot) return;

	t_real_gl r = 0, g = 0, b = 1;
	t_real_gl scale = 1;
	t_real_gl posx = static_cast<t_real_gl>(pos[0]);
	t_real_gl posy = static_cast<t_real_gl>(pos[1]);
	t_real_gl posz = static_cast<t_real_gl>(pos[2]);

	auto obj = m_plot->GetRenderer()->AddLinkedObject(m_sphere, 0,0,0, r,g,b,1);
	//auto obj = m_plot->GetRenderer()->AddSphere(0.05, 0,0,0, r,g,b,1);
	m_plot->GetRenderer()->SetObjectMatrix(obj,
		tl2::hom_translation<t_mat_gl>(posx, posy, posz) *
		tl2::hom_scaling<t_mat_gl>(scale, scale, scale));
	//m_plot->GetRenderer()->SetObjectLabel(obj, "Voronoi Vertex");

	m_plotObjs.push_back(obj);
	m_plot->update();
}


/**
 * add a bragg peak to the plot
 */
void BZDlg::PlotAddBraggPeak(const t_vec& pos)
{
	if(!m_plot) return;

	t_real_gl r = 1, g = 0, b = 0;
	t_real_gl scale = 1;
	t_real_gl posx = static_cast<t_real_gl>(pos[0]);
	t_real_gl posy = static_cast<t_real_gl>(pos[1]);
	t_real_gl posz = static_cast<t_real_gl>(pos[2]);

	auto obj = m_plot->GetRenderer()->AddLinkedObject(m_sphere, 0,0,0, r,g,b,1);
	//auto obj = m_plot->GetRenderer()->AddSphere(0.05, 0,0,0, r,g,b,1);
	m_plot->GetRenderer()->SetObjectMatrix(obj,
		tl2::hom_translation<t_mat_gl>(posx, posy, posz) *
		tl2::hom_scaling<t_mat_gl>(scale, scale, scale));
	//m_plot->GetRenderer()->SetObjectLabel(obj, "Voronoi Vertex");

	m_plotObjs.push_back(obj);
	m_plot->update();
}


/**
 * add polygons to the plot
 */
void BZDlg::PlotAddTriangles(const std::vector<t_vec>& _vecs)
{
	if(!m_plot) return;
	if(_vecs.size() < 3) return;

	t_real_gl r = 1, g = 0, b = 0;
	std::vector<t_vec3_gl> vecs, norms;
	vecs.reserve(_vecs.size());
	norms.reserve(_vecs.size());

	for(std::size_t idx=0; idx<_vecs.size()-2; idx+=3)
	{
		t_vec3_gl vec1 = tl2::convert<t_vec3_gl>(_vecs[idx]);
		t_vec3_gl vec2 = tl2::convert<t_vec3_gl>(_vecs[idx+1]);
		t_vec3_gl vec3 = tl2::convert<t_vec3_gl>(_vecs[idx+2]);

		t_vec3_gl norm = tl2::cross<t_vec3_gl>(vec2-vec1, vec3-vec1);
		norm /= tl2::norm(norm);

		t_vec3_gl mid = (vec1+vec2+vec3)/3.;
		mid /= tl2::norm<t_vec3_gl>(mid);

		// change sign of norm / sense of veritices?
		if(tl2::inner<t_vec3_gl>(norm, mid) < 0.)
		{
			vecs.emplace_back(std::move(vec3));
			vecs.emplace_back(std::move(vec2));
			vecs.emplace_back(std::move(vec1));
			norms.push_back(-norm);
		}
		else
		{
			vecs.emplace_back(std::move(vec1));
			vecs.emplace_back(std::move(vec2));
			vecs.emplace_back(std::move(vec3));
			norms.push_back(norm);
		}
	}

	auto obj = m_plot->GetRenderer()->AddTriangleObject(vecs, norms, r,g,b,1);
	m_plotObjs.push_back(obj);

	m_plot->update();
}


/**
 * set the brillouin zone cut plane
 */
void BZDlg::PlotSetPlane(const t_vec& _norm, t_real d)
{
	if(!m_plot) return;

	t_vec3_gl norm = tl2::convert<t_vec3_gl>(_norm);
	t_vec3_gl norm_old = tl2::create<t_vec3_gl>({ 0, 0, 1 });
	t_vec3_gl rot_vec = tl2::create<t_vec3_gl>({ 1, 0, 0 });

	t_vec3_gl offs = d * norm;
	t_mat_gl rot = tl2::hom_rotation<t_mat_gl, t_vec3_gl>(norm_old, norm, &rot_vec);
	t_mat_gl trans = tl2::hom_translation<t_mat_gl>(offs[0], offs[1], offs[2]);

	m_plot->GetRenderer()->SetObjectMatrix(m_plane, trans*rot);
	m_plot->update();
}


void BZDlg::ClearBZPlot()
{
	if(!m_plot) return;

	for(std::size_t obj : m_plotObjs)
		m_plot->GetRenderer()->RemoveObject(obj);

	m_plotObjs.clear();
	m_plot->update();
}


/**
 * mouse hovers over 3d object
 */
void BZDlg::PickerIntersection(
	const t_vec3_gl* pos,
	std::size_t objIdx,
	[[maybe_unused]] const t_vec3_gl* posSphere)
{
	if(pos)
		m_curPickedObj = long(objIdx);
	else
		m_curPickedObj = -1;


	if(m_curPickedObj > 0 && pos)
	{
		std::ostringstream ostr;
		ostr.precision(g_prec_gui);

		t_vec QinvA = tl2::convert<t_vec>(*pos);
		t_mat Binv = m_crystA / (t_real(2)*tl2::pi<t_real>);
		t_vec Qrlu = Binv * QinvA;

		tl2::set_eps_0<t_vec>(QinvA, t_real_gl(g_eps));
		tl2::set_eps_0<t_vec>(Qrlu, t_real_gl(g_eps));

		ostr << "Q = (" << QinvA[0] << ", " << QinvA[1] << ", " << QinvA[2] << ") Å⁻¹";
		ostr << " = (" << Qrlu[0] << ", " << Qrlu[1] << ", " << Qrlu[2] << ") rlu.";

		Set3DStatusMsg(ostr.str());
	}
	else
	{
		Set3DStatusMsg("");
	}
}



/**
 * set status label text in 3d dialog
 */
void BZDlg::Set3DStatusMsg(const std::string& msg)
{
	m_status3D->setText(msg.c_str());
}



/**
 * mouse button pressed
 */
void BZDlg::PlotMouseDown(
	[[maybe_unused]] bool left,
	[[maybe_unused]] bool mid,
	[[maybe_unused]] bool right)
{
	if(left && m_curPickedObj > 0)
	{
	}
}


/**
 * mouse button released
 */
void BZDlg::PlotMouseUp(
	[[maybe_unused]] bool left,
	[[maybe_unused]] bool mid,
	[[maybe_unused]] bool right)
{
}


void BZDlg::AfterGLInitialisation()
{
	if(!m_plot) return;

	// reference sphere and plane for linked objects
	m_sphere = m_plot->GetRenderer()->AddSphere(0.05, 0.,0.,0., 1.,1.,1.,1.);
	m_plane = m_plot->GetRenderer()->AddPlane(0.,0.,1., 0.,0.,0., 5., 0.75,0.75,0.75,0.5);
	m_plot->GetRenderer()->SetObjectVisible(m_sphere, false);
	m_plot->GetRenderer()->SetObjectVisible(m_plane, true);
	m_plot->GetRenderer()->SetObjectPriority(m_plane, 0);

	// B matrix
	//m_plot->GetRenderer()->SetBTrafo(m_crystB);

	// GL device info
	auto [strGlVer, strGlShaderVer, strGlVendor, strGlRenderer]
		= m_plot->GetRenderer()->GetGlDescr();
	m_labelGlInfos[0]->setText(QString("GL Version: ") + strGlVer.c_str() + QString("."));
	m_labelGlInfos[1]->setText(QString("GL Shader Version: ") + strGlShaderVer.c_str() + QString("."));
	m_labelGlInfos[2]->setText(QString("GL Vendor: ") + strGlVendor.c_str() + QString("."));
	m_labelGlInfos[3]->setText(QString("GL Device: ") + strGlRenderer.c_str() + QString("."));

	CalcB(true);
}
