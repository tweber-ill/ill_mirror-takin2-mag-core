/**
 * magnon dynamics -- calculations for structure plot
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

#include <unordered_set>
#include <functional>
#include <boost/functional/hash.hpp>

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
		m_structplot_dlg->setWindowTitle("Structure Viewer");
		m_structplot_dlg->setFont(this->font());

		m_structplot = new tl2::GlPlot(this);
		m_structplot->GetRenderer()->SetRestrictCamTheta(false);
		m_structplot->GetRenderer()->SetLight(
			0, tl2::create<t_vec3_gl>({ 5, 5, 5 }));
		m_structplot->GetRenderer()->SetLight(
			1, tl2::create<t_vec3_gl>({ -5, -5, -5 }));
		m_structplot->GetRenderer()->SetCoordMax(1.);
		m_structplot->GetRenderer()->GetCamera().SetDist(1.5);
		m_structplot->GetRenderer()->GetCamera().UpdateTransformation();
		m_structplot->setSizePolicy(QSizePolicy{
			QSizePolicy::Expanding, QSizePolicy::Expanding});

		m_structplot_coordcross = new QCheckBox("Show Coordinates", this);
		m_structplot_coordcross->setChecked(true);

		m_structplot_labels = new QCheckBox("Show Labels", this);
		m_structplot_labels->setChecked(true);

		m_structplot_status = new QLabel(this);

		m_structplot_context= new QMenu(this);
		QAction *acDel = new QAction("Delete Object", m_structplot_context);
		QAction *acCentre = new QAction("Centre Camera", m_structplot_context);
		m_structplot_context->addAction(acDel);
		m_structplot_context->addSeparator();
		m_structplot_context->addAction(acCentre);

		auto grid = new QGridLayout(m_structplot_dlg);
		grid->setSpacing(2);
		grid->setContentsMargins(4,4,4,4);
		grid->addWidget(m_structplot, 0,0,1,2);
		grid->addWidget(m_structplot_coordcross, 1,0,1,1);
		grid->addWidget(m_structplot_labels, 1,1,1,1);
		grid->addWidget(m_structplot_status, 2,0,1,2);

		connect(m_structplot, &tl2::GlPlot::AfterGLInitialisation,
			this, &MagDynDlg::StructPlotAfterGLInitialisation);
		connect(m_structplot->GetRenderer(), &tl2::GlPlotRenderer::PickerIntersection,
			this, &MagDynDlg::StructPlotPickerIntersection);
		connect(m_structplot, &tl2::GlPlot::MouseClick,
			this, &MagDynDlg::StructPlotMouseClick);
		connect(m_structplot, &tl2::GlPlot::MouseDown,
			this, &MagDynDlg::StructPlotMouseDown);
		connect(m_structplot, &tl2::GlPlot::MouseUp,
			this, &MagDynDlg::StructPlotMouseUp);
		connect(acDel, &QAction::triggered,
			this, &MagDynDlg::StructPlotDelete);
		connect(acCentre, &QAction::triggered,
			this, &MagDynDlg::StructPlotCentreCamera);
		connect(m_structplot_coordcross, &QCheckBox::toggled,
			this, &MagDynDlg::StructPlotShowCoordCross);
		connect(m_structplot_labels, &QCheckBox::toggled,
			this, &MagDynDlg::StructPlotShowLabels);

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
	m_structplot_cur_obj = std::nullopt;
	m_structplot_cur_atom = std::nullopt;
	m_structplot_cur_term = std::nullopt;

	if(!pos)
		return;

	m_structplot_cur_obj = objIdx;

	// look for atom sites
	if(auto iter_atoms = m_structplot_atoms.find(objIdx);
		iter_atoms != m_structplot_atoms.end())
	{
		m_structplot_cur_atom = iter_atoms->second->index;

		const std::string& ident = iter_atoms->second->name;
		m_structplot_status->setText(ident.c_str());

		return;
	}

	// look for exchange terms
	if(auto iter_terms = m_structplot_terms.find(objIdx);
		iter_terms != m_structplot_terms.end())
	{
		m_structplot_cur_term = iter_terms->second->index;

		const std::string& ident = iter_terms->second->name;
		m_structplot_status->setText(ident.c_str());

		return;
	}
}


/**
 * delete currently selected atom or bond
 */
void MagDynDlg::StructPlotDelete()
{
	if(m_structplot_cur_atom)
	{
		DelTabItem(m_sitestab, *m_structplot_cur_atom, *m_structplot_cur_atom+1);
		m_structplot_cur_atom = std::nullopt;
	}

	if(m_structplot_cur_term)
	{
		DelTabItem(m_termstab, *m_structplot_cur_term, *m_structplot_cur_term+1);
		m_structplot_cur_term = std::nullopt;
	}
}


/**
 * show or hide the coordinate system
 */
void MagDynDlg::StructPlotShowCoordCross(bool show)
{
	if(auto obj = m_structplot->GetRenderer()->GetCoordCross(); obj)
	{
		m_structplot->GetRenderer()->SetObjectVisible(*obj, show);
		m_structplot->update();
	}
}


/**
 * show or hide the object labels
 */
void MagDynDlg::StructPlotShowLabels(bool show)
{
	m_structplot->GetRenderer()->SetLabelsVisible(show);
	m_structplot->update();
}


/**
 * centre camera on currently selected object
 */
void MagDynDlg::StructPlotCentreCamera()
{
	if(!m_structplot_cur_obj)
		return;

	const t_mat_gl& mat = m_structplot->GetRenderer()->
		GetObjectMatrix(*m_structplot_cur_obj);
	m_structplot->GetRenderer()->GetCamera().Centre(mat);
	m_structplot->GetRenderer()->GetCamera().UpdateTransformation();
}


/**
 * structure plot mouse button clicked
 */
void MagDynDlg::StructPlotMouseClick(
	[[maybe_unused]] bool left,
	[[maybe_unused]] bool mid,
	[[maybe_unused]] bool right)
{
	if(right && m_structplot_cur_obj)
	{
		const QPointF& _pt = m_structplot->GetRenderer()->GetMousePosition();
		QPoint pt = m_structplot->mapToGlobal(_pt.toPoint());
		m_structplot_context->popup(pt);
	}
}


/**
 * structure plot mouse button pressed
 */
void MagDynDlg::StructPlotMouseDown(
	[[maybe_unused]] bool left,
	[[maybe_unused]] bool mid,
	[[maybe_unused]] bool right)
{
	if(left && m_structplot_cur_atom)
	{
		// select current site in table
		m_tabs_in->setCurrentWidget(m_sitespanel);
		m_sitestab->setCurrentCell(*m_structplot_cur_atom, 0);
	}

	if(left && m_structplot_cur_term)
	{
		// select current term in table
		m_tabs_in->setCurrentWidget(m_termspanel);
		m_termstab->setCurrentCell(*m_structplot_cur_term, 0);
	}
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

	// reference cylinder for linked objects
	m_structplot_cyl = m_structplot->GetRenderer()->AddCylinder(
		0.01, 1., 0.,0.,0.5,  1.,1.,1.,1.);
	m_structplot->GetRenderer()->SetObjectVisible(
		m_structplot_cyl, false);

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

	// get sites and terms
	const auto& sites = m_dyn.GetAtomSites();
	const auto& sites_calc = m_dyn.GetAtomSitesCalc();
	const auto& terms = m_dyn.GetExchangeTerms();
	const auto& terms_calc = m_dyn.GetExchangeTermsCalc();
	const auto& field = m_dyn.GetExternalField();
	const auto& ordering = m_dyn.GetOrderingWavevector();
	const auto& rotaxis = m_dyn.GetRotationAxis();
	const bool is_incommensurate = m_dyn.IsIncommensurate();
	//const auto [sc_min, sc_max] = m_dyn.GetSupercellMinMax();


	// clear old atoms
	for(const auto& [atom_idx, atom_site] : m_structplot_atoms)
	{
		m_structplot->GetRenderer()->RemoveObject(atom_idx);
	}

	m_structplot_atoms.clear();


	// clear old terms
	for(const auto& [term_idx, term] : m_structplot_terms)
	{
		m_structplot->GetRenderer()->RemoveObject(term_idx);
	}

	m_structplot_terms.clear();


	// hashes of already seen atom sites
	std::unordered_set<std::size_t> atom_hashes;


	// calculate the hash of an atom site
	auto get_atom_hash = [](const t_magdyn::AtomSite& site,
		t_real_gl sc_x, t_real_gl sc_y, t_real_gl sc_z)
			-> std::size_t
	{
		int _sc_x = int(std::round(sc_x));
		int _sc_y = int(std::round(sc_y));
		int _sc_z = int(std::round(sc_z));

		std::size_t hash = std::hash<int>{}(site.index);
		boost::hash_combine(hash, std::hash<int>{}(_sc_x));
		boost::hash_combine(hash, std::hash<int>{}(_sc_y));
		boost::hash_combine(hash, std::hash<int>{}(_sc_z));

		return hash;
	};


	// check if the atom site has already been seen
	auto atom_not_yet_seen = [&atom_hashes, &get_atom_hash](
		const t_magdyn::AtomSite& site,
		t_real_gl sc_x, t_real_gl sc_y, t_real_gl sc_z)
	{
		std::size_t hash = get_atom_hash(site, sc_x, sc_y, sc_z);
		return atom_hashes.find(hash) == atom_hashes.end();
	};


	// add an atom site to the plot
	auto add_atom_site = [this, &atom_hashes, &get_atom_hash,
		is_incommensurate, &ordering, &rotaxis](
		const t_magdyn::AtomSite& site,
		const t_magdyn::AtomSiteCalc& site_calc,
		const t_magdyn::ExternalField& field,
		t_real_gl sc_x, t_real_gl sc_y, t_real_gl sc_z)
	{
		// super cell index
		int _sc_x = int(std::round(sc_x));
		int _sc_y = int(std::round(sc_y));
		int _sc_z = int(std::round(sc_z));

		t_real_gl rgb[3] {0., 0., 1.};
		if(_sc_x == 0 && _sc_y == 0 && _sc_z == 0)
		{
			rgb[0] = t_real_gl(1.);
			rgb[1] = t_real_gl(0.);
			rgb[2] = t_real_gl(0.);
		}

		t_real_gl scale = 1.;

		std::size_t obj = m_structplot->GetRenderer()->AddLinkedObject(
			m_structplot_sphere, 0,0,0, rgb[0], rgb[1], rgb[2], 1);

		std::size_t arrow = m_structplot->GetRenderer()->AddLinkedObject(
			m_structplot_arrow, 0,0,0, rgb[0], rgb[1], rgb[2], 1);

		m_structplot_atoms.insert(std::make_pair(obj, &site));
		m_structplot_atoms.insert(std::make_pair(arrow, &site));

		t_vec_gl pos_vec = tl2::create<t_vec_gl>({
			t_real_gl(site.pos[0]) + sc_x,
			t_real_gl(site.pos[1]) + sc_y,
			t_real_gl(site.pos[2]) + sc_z,
		});

		t_vec_gl spin_vec;

		// align spin to external field?
		if(field.align_spins)
		{
			spin_vec = tl2::create<t_vec_gl>({
				t_real_gl(-field.dir[0] * site.spin_mag),
				t_real_gl(-field.dir[1] * site.spin_mag),
				t_real_gl(-field.dir[2] * site.spin_mag),
			});
		}
		else
		{
			spin_vec = tl2::create<t_vec_gl>({
				t_real_gl(site_calc.spin_dir[0].real() * site.spin_mag),
				t_real_gl(site_calc.spin_dir[1].real() * site.spin_mag),
				t_real_gl(site_calc.spin_dir[2].real() * site.spin_mag),
			});

			if(is_incommensurate)
			{
				// rotate spin vector for incommensurate structures
				t_vec_gl sc_vec = tl2::create<t_vec_gl>({sc_x, sc_y, sc_z});

				tl2_mag::rotate_spin_incommensurate<t_mat_gl, t_vec_gl, t_real_gl>(
					spin_vec, sc_vec,
					tl2::convert<t_vec_gl>(ordering),
					tl2::convert<t_vec_gl>(rotaxis),
					g_eps);
			}
		}

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
		//m_structplot->GetRenderer()->SetObjectLabel(arrow, site.name);

		// mark the atom as already seen
		std::size_t hash = get_atom_hash(site, sc_x, sc_y, sc_z);
		atom_hashes.insert(hash);
	};


	// iterate and add unit cell atom sites
	for(std::size_t site_idx=0; site_idx<sites.size(); ++site_idx)
	{
		add_atom_site(sites[site_idx],
			sites_calc[site_idx],
			field, 0, 0, 0);
	}


	// iterate and add exchange terms
	for(t_size term_idx=0; term_idx<terms.size(); ++term_idx)
	{
		const auto& term = terms[term_idx];
		const auto& term_calc = terms_calc[term_idx];

		if(term.atom1 >= sites.size() || term.atom2 >= sites.size())
			continue;

		const auto& site1 = sites[term.atom1];
		const auto& site2 = sites[term.atom2];
		const auto& site2_calc = sites_calc[term.atom2];

		t_real_gl sc_x = t_real_gl(term.dist[0]);
		t_real_gl sc_y = t_real_gl(term.dist[1]);
		t_real_gl sc_z = t_real_gl(term.dist[2]);

		t_real_gl rgb[3] {0., 0.75, 0.};
		t_real_gl scale = 1.;

		std::size_t obj = m_structplot->GetRenderer()->AddLinkedObject(
			m_structplot_cyl, 0,0,0, rgb[0], rgb[1], rgb[2], 1);

		m_structplot_terms.insert(std::make_pair(obj, &term));

		// connection from unit cell atom site...
		const t_vec_gl pos1_vec = tl2::create<t_vec_gl>({
			t_real_gl(site1.pos[0]),
			t_real_gl(site1.pos[1]),
			t_real_gl(site1.pos[2]),
		});

		// ... to atom in super cell
		const t_vec_gl pos2_vec = tl2::create<t_vec_gl>({
			t_real_gl(site2.pos[0]) + sc_x,
			t_real_gl(site2.pos[1]) + sc_y,
			t_real_gl(site2.pos[2]) + sc_z,
		});

		// add the supercell site if it hasn't been inserted yet
		if(atom_not_yet_seen(site2, sc_x, sc_y, sc_z))
			add_atom_site(site2, site2_calc, field, sc_x, sc_y, sc_z);

		t_vec_gl dir_vec = pos2_vec - pos1_vec;
		t_real_gl dir_len = tl2::norm<t_vec_gl>(dir_vec);

		// coupling bond
		m_structplot->GetRenderer()->SetObjectMatrix(obj,
			tl2::get_arrow_matrix<t_vec_gl, t_mat_gl, t_real_gl>(
				dir_vec,                           // to
				1,                                 // post-scale
				tl2::create<t_vec_gl>({0, 0, 0}),  // post-translate
				tl2::create<t_vec_gl>({0, 0, 1}),  // from
				scale,                             // pre-scale
				pos1_vec)                          // pre-translate
			* tl2::hom_translation<t_mat_gl>(
				t_real_gl(0), t_real_gl(0), dir_len*t_real_gl(0.5))
			* tl2::hom_scaling<t_mat_gl>(
				t_real_gl(1), t_real_gl(1), dir_len));

		m_structplot->GetRenderer()->SetObjectLabel(obj, term.name);


		// dmi vector
		const t_vec_gl dmi_vec = tl2::create<t_vec_gl>({
			t_real_gl(term_calc.dmi[0].real()),
			t_real_gl(term_calc.dmi[1].real()),
			t_real_gl(term_calc.dmi[2].real()),
		});

		if(tl2::norm<t_vec_gl>(dmi_vec) > g_eps)
		{
			std::size_t objDmi = m_structplot->GetRenderer()->AddLinkedObject(
				m_structplot_arrow, 0,0,0, rgb[0], rgb[1], rgb[2], 1);

			m_structplot_terms.insert(std::make_pair(objDmi, &term));

			t_real_gl scale_dmi = 0.5;

			m_structplot->GetRenderer()->SetObjectMatrix(objDmi,
				tl2::get_arrow_matrix<t_vec_gl, t_mat_gl, t_real_gl>(
					dmi_vec,                           // to
					1,                                 // post-scale
					tl2::create<t_vec_gl>({0, 0, 0}),  // post-translate
					tl2::create<t_vec_gl>({0, 0, 1}),  // from
					scale_dmi,                         // pre-scale
					(pos1_vec+pos2_vec)/t_real_gl(2))  // pre-translate
				);

			//m_structplot->GetRenderer()->SetObjectLabel(objDmi, term.name);
		}
	}

	m_structplot->update();
}
