/**
 * magnon dynamics
 * @author Tobias Weber <tweber@ill.fr>
 * @date Jan-2022
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 28-Dec-2018 from my privately developed "misc" project (https://github.com/t-weber/misc).
 *
 * ----------------------------------------------------------------------------
 * mag-core (part of the Takin software suite)
 * Copyright (C) 2018-2022  Tobias WEBER (Institut Laue-Langevin (ILL),
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

#include "magdyn.h"

using namespace tl2_ops;


/**
 * show the 3d view of the atomic structure
 */
void MagDynDlg::ShowStructurePlot()
{
	// plot widget
	if(!m_structplot_dlg)
	{
		m_structplot_dlg = new QDialog(this);
		m_structplot_dlg->setWindowTitle("Unit Cell");

		m_structplot = new tl2::GlPlot(this);
		m_structplot->GetRenderer()->SetLight(
			0, tl2::create<t_vec3_gl>({ 5, 5, 5 }));
		m_structplot->GetRenderer()->SetLight(
			1, tl2::create<t_vec3_gl>({ -5, -5, -5 }));
		m_structplot->GetRenderer()->SetCoordMax(1.);
		m_structplot->GetRenderer()->GetCamera().SetDist(1.5);
		m_structplot->GetRenderer()->GetCamera().UpdateTransformation();
		m_structplot->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Expanding});

		m_structplot_status = new QLabel(this);

		auto grid = new QGridLayout(m_structplot_dlg);
		grid->setSpacing(2);
		grid->setContentsMargins(4,4,4,4);
		grid->addWidget(m_structplot, 0,0,1,2);
		grid->addWidget(m_structplot_status, 1,0,1,2);

		connect(m_structplot, &tl2::GlPlot::AfterGLInitialisation,
			this, &MagDynDlg::StructPlotAfterGLInitialisation);
		connect(m_structplot->GetRenderer(), &tl2::GlPlotRenderer::PickerIntersection,
			this, &MagDynDlg::StructPlotPickerIntersection);
		connect(m_structplot, &tl2::GlPlot::MouseDown,
			this, &MagDynDlg::StructPlotMouseDown);
		connect(m_structplot, &tl2::GlPlot::MouseUp,
			this, &MagDynDlg::StructPlotMouseUp);

		if(m_sett && m_sett->contains("geo_struct_view"))
			m_structplot_dlg->restoreGeometry(
				m_sett->value("geo_struct_view").toByteArray());
		else
			m_structplot_dlg->resize(500, 500);
	}

	m_structplot_dlg->show();
	m_structplot_dlg->raise();
	m_structplot_dlg->focusWidget();
}


/**
 * structure plot picker intersection
 */
void MagDynDlg::StructPlotPickerIntersection(
        const t_vec3_gl* pos, std::size_t objIdx,
        [[maybe_unused]] const t_vec3_gl* posSphere)
{
	m_structplot_status->setText("");
	if(!pos)
		return;

	auto iter_atoms = m_structplot_atoms.find(objIdx);
	if(iter_atoms == m_structplot_atoms.end())
		return;

	const std::string& ident = iter_atoms->second->name;

	// TODO
	m_structplot_status->setText(ident.c_str());
}


/**
 * structure plot mouse button pressed
 */
void MagDynDlg::StructPlotMouseDown(
	[[maybe_unused]] bool left,
	[[maybe_unused]] bool mid,
	[[maybe_unused]] bool right)
{
}


/**
 * structure plot mouse button released
 */
void MagDynDlg::StructPlotMouseUp(
	[[maybe_unused]] bool left,
	[[maybe_unused]] bool mid,
	[[maybe_unused]] bool right)
{
}


/**
 * after structure plot initialisation
 */
void MagDynDlg::StructPlotAfterGLInitialisation()
{
	if(!m_structplot)
		return;

	// reference sphere for linked objects
	m_structplot_sphere = m_structplot->GetRenderer()->AddSphere(
		0.05, 0.,0.,0., 1.,1.,1.,1.);
	m_structplot->GetRenderer()->SetObjectVisible(
		m_structplot_sphere, false);

	// reference arrow for linked objects
	m_structplot_arrow = m_structplot->GetRenderer()->AddArrow(
		0.015, 0.25, 0.,0.,0.5,  1.,1.,1.,1.);
	m_structplot->GetRenderer()->SetObjectVisible(
		m_structplot_arrow, false);

	// GL device info
	auto [strGlVer, strGlShaderVer, strGlVendor, strGlRenderer]
		= m_structplot->GetRenderer()->GetGlDescr();
	m_labelGlInfos[0]->setText(
		QString("GL Version: %1.").arg(strGlVer.c_str()));
	m_labelGlInfos[1]->setText(
		QString("GL Shader Version: %1.").arg(strGlShaderVer.c_str()));
	m_labelGlInfos[2]->setText(
		QString("GL Vendor: %1.").arg(strGlVendor.c_str()));
	m_labelGlInfos[3]->setText(
		QString("GL Device: %1.").arg(strGlRenderer.c_str()));

	StructPlotSync();
}


/**
 * get the sites and exchange terms and
 * transfer them to the structure plotter
 */
void MagDynDlg::StructPlotSync()
{
	if(!m_structplot)
		return;

	const auto& sites = m_dyn.GetAtomSites();


	// clear old atoms
	for(const auto& [atom_idx, atom_site] : m_structplot_atoms)
	{
		m_structplot->GetRenderer()->RemoveObject(atom_idx);
	}

	m_structplot_atoms.clear();


	// add atoms
	for(const auto& site : sites)
	{
		t_real_gl rgb[3]{1., 0., 0.};
		t_real_gl scale = 1.;

		std::size_t obj = m_structplot->GetRenderer()->AddLinkedObject(
			m_structplot_sphere, 0,0,0, rgb[0], rgb[1], rgb[2], 1);

		std::size_t arrow = m_structplot->GetRenderer()->AddLinkedObject(
			m_structplot_arrow, 0,0,0, rgb[0], rgb[1], rgb[2], 1);

		m_structplot_atoms.insert(std::make_pair(obj, &site));
		m_structplot_atoms.insert(std::make_pair(arrow, &site));

		t_vec_gl pos_vec = tl2::create<t_vec_gl>({
			t_real_gl(site.pos[0].real()),
			t_real_gl(site.pos[1].real()),
			t_real_gl(site.pos[2].real()),
		});

		t_vec_gl spin_vec = tl2::create<t_vec_gl>({
			t_real_gl(site.spin_dir[0].real() * site.spin_mag),
			t_real_gl(site.spin_dir[1].real() * site.spin_mag),
			t_real_gl(site.spin_dir[2].real() * site.spin_mag),
		});

		m_structplot->GetRenderer()->SetObjectMatrix(obj,
			tl2::hom_translation<t_mat_gl>(
				pos_vec[0], pos_vec[1], pos_vec[2]) *
			tl2::hom_scaling<t_mat_gl>(scale, scale, scale));

		m_structplot->GetRenderer()->SetObjectMatrix(arrow,
			tl2::get_arrow_matrix<t_vec_gl, t_mat_gl, t_real_gl>(
				spin_vec,                          // to
				1,                                 // post-scale
				tl2::create<t_vec_gl>({0, 0, 0}),  // post-translate
				tl2::create<t_vec_gl>({0, 0, 1}),  // from
				scale,                             // pre-scale
				pos_vec));                         // pre-translate

		m_structplot->GetRenderer()->SetObjectLabel(obj, site.name);
		m_structplot->GetRenderer()->SetObjectLabel(arrow, site.name);
	}

	m_structplot->update();
}
